int8_t wifi_scan_state = 2; //статус сканирования сети
uint32_t wifi_scan_timer = 0; //таймер начала поиска сети

uint8_t wifi_status = 254; //статус соединения wifi
uint32_t wifi_interval = 0; //интервал переподключения к wifi

String wifi_scan_list = "Нет сетей"; //список найденых wifi сетей

//--------------------------------------------------------------------
uint8_t wifiGetSignal(void) {
  return constrain(2 * (WiFi.RSSI() + 100), 0, 100);
}
//--------------------------------------------------------------------
String wifiGetLocalSSID(void) {
  String str;
  str.reserve(70);
  if (settings.ssid[0]) str = settings.ssid;
  else str = F("unset");
  return str;
}
//--------------------------------------------------------------------
String wifiGetLocalIP(void) {
  String str;
  str.reserve(30);
  if (wifi_status == WL_CONNECTED) str = WiFi.localIP().toString();
  else str = F("0.0.0.0");
  return str;
}
//--------------------------------------------------------------------
String wifiGetConnectState(void) {
  String str;
  str.reserve(100);

  if (settings.ssid[0]) {
    if (wifi_status == WL_CONNECTED) str = F("Подключение установлено");
    else if (!wifi_interval && (wifi_status != 254)) str = F("Не удалось подключиться");
    else str = F("Подключение к сети...");
  }
  else str = F("Некорректное имя сети");
  return str;
}
//--------------------------------------------------------------------
String wifiGetCurrentMode(void) {
  String str;
  str.reserve(30);

  if (WiFi.getMode() == WIFI_AP) str = F("AP");
  else if (WiFi.getMode() == WIFI_STA) str = F("STA");
  else str = F("AP_STA");

  switch (WiFi.getPhyMode()) {
    case WIFI_PHY_MODE_11B: str += F("_11B"); break;
    case WIFI_PHY_MODE_11G: str += F("_11G"); break;
    case WIFI_PHY_MODE_11N: str += F("_11N"); break;
  }
  return str;
}
//--------------------------------------------------------------------
IPAddress wifiGetBroadcastIP(void) {
  struct ip_info data;
  wifi_get_ip_info(STATION_IF, &data);
  return IPAddress(data.ip.addr | ~(data.netmask.addr));
}
//--------------------------------------------------------------------
boolean wifiGetConnectStatus(void) {
  return (wifi_status == WL_CONNECTED);
}
boolean wifiGetConnectWaitStatus(void) {
  return ((wifi_interval != 0) || (wifi_status == 254));
}
void wifiSetConnectWaitInterval(uint32_t time) {
  wifi_interval = time;
}
void wifiResetConnectStatus(void) {
  wifi_interval = 0;
  wifi_status = 255;
}
void wifiSetConnectStatus(void) {
  wifi_interval = 0;
  wifi_status = 254;
}
//--------------------------------------------------------------------
boolean wifiGetScanAllowStatus(void) {
  return (wifi_scan_state > 0);
}
boolean wifiGetScanCompleteStatus(void) {
  return (wifi_scan_state < 0);
}
boolean wifiGetScanFoundStatus(void) {
  return (wifi_scan_state != 1);
}
void wifiResetScanCompleteStatus(void) {
  wifi_scan_state = -wifi_scan_state;
}
//--------------------------------------------------------------------
void wifiStartScanNetworks(void) {
  wifi_scan_list = "Поиск...";
  wifi_scan_state = 127;
  wifi_scan_timer = millis();
}
//--------------------------------------------------------------------
void wifiScanResult(int networksFound) {
  wifi_scan_list = "";
  if (networksFound) {
    wifi_scan_state = -1;
    for (int i = 0; i < networksFound; i++) {
      if (i) wifi_scan_list += ',';
      wifi_scan_list += WiFi.SSID(i);
      if (WiFi.encryptionType(i) != ENC_TYPE_NONE) wifi_scan_list += " 🔒";
    }
  }
  else {
    wifi_scan_state = -2;
    wifi_scan_list = "Нет сетей";
  }
}
//--------------------------------------------------------------------
void wifiStartAP(void) {
  //настраиваем режим работы
  WiFi.mode(WIFI_AP_STA);
  WiFi.setPhyMode(WIFI_PHY_MODE_11G);

  //настраиваем точку доступа
  IPAddress local(AP_IP);
  IPAddress subnet(255, 255, 255, 0);

  //задаем настройки сети
  WiFi.softAPConfig(local, local, subnet);

  //запускаем точку доступа
  if (!WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL)) {
#if DEBUG_MODE
    Serial.println F("Wifi access point start failed, wrong settings!");
#endif
  }
#if DEBUG_MODE
  else {
    Serial.print F("Wifi access point enable, [ ssid: ");
    Serial.print(AP_SSID);
    if (AP_PASS[0] != '\0') {
      Serial.print F(" ][ pass: ");
      Serial.print(AP_PASS);
    }
    else Serial.print F(" ][ open ");
    Serial.print F(" ][ ip: ");
    Serial.print(WiFi.softAPIP());
    Serial.println F(" ]");
  }
#endif

  //начинаем поиск сетей
  WiFi.scanNetworksAsync(wifiScanResult);
}
//--------------------------------------------------------------------
void wifiStartSTA(void) {
  WiFi.mode(WIFI_STA);
  WiFi.setPhyMode(WIFI_PHY_MODE_11G);
}
//--------------------------------------------------------------------
boolean wifiUpdate(void) {
  static uint32_t timerWifi = millis(); //таймер попытки подключения к wifi

  if ((wifi_scan_state == 127) && (millis() - wifi_scan_timer) >= 100) { //если необходимо начать поиск
    wifi_scan_state = 0; //сбрасываем статус
    WiFi.scanNetworksAsync(wifiScanResult); //начинаем поиск
  }

  if (wifi_status != WiFi.status()) { //если изменился статус
    if (wifi_status == 255) { //если нужно отключиться
      udp.stop(); //остановить udp
      WiFi.disconnect(); //отключаем wifi

      WIFI_SETTINGS[7] = 0x00;
      if (ESP.rtcUserMemoryWrite(32, rtcMemory, sizeof(rtcMemory))) {
#if DEBUG_MODE
        Serial.println F("Wifi settings reset!");
#endif
      }
#if DEBUG_MODE
      Serial.println F("Wifi disconnecting...");
#endif
    }
    else if (wifi_status == 254) { //если нужно подключиться
      if ((WIFI_SETTINGS[7] == 0xCC) && (settingsMode == false)) {
        wifi_status = WiFi.begin(settings.ssid, settings.pass, WIFI_SETTINGS[6], WIFI_SETTINGS); //подключаемся к wifi
        WiFi.config(WIFI_LOCAL_IP, WIFI_GATEWAY_IP, WIFI_SUBNET_MASK, WIFI_DNS_1, WIFI_DNS_2); //восстанавливаем настройки dhcp
      }
      else wifi_status = WiFi.begin(settings.ssid, settings.pass); //подключаемся к wifi

      timerWifi = millis(); //сбросили таймер
      if (wifi_status != WL_CONNECT_FAILED) {
        if ((WIFI_SETTINGS[7] == 0xCC) && (settingsMode == false)) {
          wifi_interval = 3000; //устанавливаем интервал ожидания
#if DEBUG_MODE
          Serial.print F("Wifi fast connecting to \"");
#endif
        }
        else {
          wifi_interval = 15000; //устанавливаем интервал ожидания
#if DEBUG_MODE
          Serial.print F("Wifi connecting to \"");
#endif
        }
#if DEBUG_MODE
        Serial.print(settings.ssid);
        Serial.println F("\"...");
#endif
      }
      else {
        if (settingsMode == false) wifi_interval = 2000; //устанавливаем интервал ожидания
        else wifi_interval = 0; //сбрасываем интервал ожидания
#if DEBUG_MODE
        Serial.println F("Wifi connection failed, wrong settings!");
#endif
      }
    }

    wifi_status = WiFi.status();

    if (wifi_status == WL_CONNECTED) {
      timerWifi = millis(); //сбросили таймер
      wifi_interval = 0; //сбрасываем интервал ожидания

      udp.begin(UDP_LOCAL_PORT); //запускаем udp
      for (uint8_t i = 0; i < 6; i++) WIFI_SETTINGS[i] = WiFi.BSSID()[i];
      WIFI_SETTINGS[6] = WiFi.channel();
      WIFI_SETTINGS[7] = 0xCC;
      for (uint8_t i = 0; i < 4; i++) {
        WIFI_LOCAL_IP[i] = WiFi.localIP()[i];
        WIFI_GATEWAY_IP[i] = WiFi.gatewayIP()[i];
        WIFI_SUBNET_MASK[i] = WiFi.subnetMask()[i];
        WIFI_DNS_1[i] = WiFi.dnsIP(0)[i];
        WIFI_DNS_2[i] = WiFi.dnsIP(1)[i];
      }
      if (ESP.rtcUserMemoryWrite(32, rtcMemory, sizeof(rtcMemory))) {
#if DEBUG_MODE
        Serial.println F("Wifi settings update!");
#endif
      }
#if DEBUG_MODE
      Serial.print F("Wifi connected, IP address: ");
      Serial.println(WiFi.localIP());
#endif
    }
  }

  if (wifi_interval && ((millis() - timerWifi) >= wifi_interval)) { //новое поключение
    wifi_interval = 0; //сбрасываем интервал переподключения
    WiFi.disconnect(); //отключаем wifi

    if (WIFI_SETTINGS[7] == 0xCC) {
      wifi_status = 254;
      WIFI_SETTINGS[7] = 0x00;
      if (ESP.rtcUserMemoryWrite(32, rtcMemory, sizeof(rtcMemory))) {
#if DEBUG_MODE
        Serial.println F("Wifi settings reset!");
#endif
      }
#if DEBUG_MODE
      Serial.println F("Wifi connect timeout!");
#endif
    }
    else if (settingsMode == false) return true;
  }

  return false;
}
