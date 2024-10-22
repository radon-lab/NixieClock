int8_t wifi_scan_state = 2; //статус сканирования сети
uint32_t wifi_scan_timer = 0; //таймер начала поиска сети

uint8_t wifi_status = WL_IDLE_STATUS; //статус соединения wifi
uint32_t wifi_interval = 100; //интервал переподключения к wifi

String wifi_scan_list = "Нет сетей"; //список найденых wifi сетей

//--------------------------------------------------------------------
String wifiGetConnectState(void) {
  String data = "<big><big>";
  if (!settings.ssid[0]) data += "Некорректное имя сети!";
  else {
    if (wifi_status == WL_CONNECTED) data += "Подключено к \"";
    else if (!wifi_interval) data += "Не удалось подключиться к \"";
    else data += "Подключение к \"";
    data += String(settings.ssid);
    if ((wifi_status == WL_CONNECTED) || !wifi_interval) data += "\"";
    else data += "\"...";
  }
  data += "</big></big>";
  return data;
}
//--------------------------------------------------------------------
boolean wifiGetConnectStatus(void) {
  return (wifi_status == WL_CONNECTED);
}
boolean wifiGetConnectWaitStatus(void) {
  return (wifi_interval != 0);
}
void wifiSetConnectWaitInterval(uint32_t time) {
  wifi_interval = time;
}
void wifiResetConnectStatus(void) {
  wifi_interval = 0;
  wifi_status = 255;
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
#if DEBUG_MODE
  Serial.println F("");
#endif

  //настраиваем точку доступа
  IPAddress local(AP_IP);
  IPAddress subnet(255, 255, 255, 0);

  //задаем настройки сети
  WiFi.softAPConfig(local, local, subnet);

  //запускаем точку доступа
  if (!WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL)) {
#if DEBUG_MODE
    Serial.println F("Wifi access point start failed, wrong settings");
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
void wifiUpdate(void) {
  static uint32_t timerWifi = millis(); //таймер попытки подключения к wifi

  if ((wifi_scan_state == 127) && (millis() - wifi_scan_timer) >= 100) { //если необходимо начать поиск
    wifi_scan_state = 0; //сбрасываем статус
    WiFi.scanNetworksAsync(wifiScanResult); //начинаем поиск
  }

  if (wifi_status != WiFi.status()) { //если изменился статус
    if (wifi_status == 255) { //если нужно отключиться
#if DEBUG_MODE
      Serial.println F("Wifi disconnecting...");
#endif
      udp.stop(); //остановить udp
      WiFi.disconnect(); //отключаем wifi
    }
    wifi_status = WiFi.status();
    switch (wifi_status) {
      case WL_CONNECTED:
        timerWifi = millis(); //сбросили таймер
        wifi_interval = 0; //сбрасываем интервал переподключения
#if STATUS_LED > 0
        if (settingsMode == true) digitalWrite(LED_BUILTIN, HIGH); //выключаем индикацию
#endif
        udp.begin(UDP_LOCAL_PORT); //запускаем udp
#if DEBUG_MODE
        Serial.print F("Wifi connected, IP address: ");
        Serial.println(WiFi.localIP());
#endif
        break;
      case WL_IDLE_STATUS:
#if STATUS_LED > 0
        if (settingsMode == true) digitalWrite(LED_BUILTIN, LOW); //включаем индикацию
#endif
#if DEBUG_MODE
        Serial.println F("Wifi idle status");
#endif
        break;
      default:
        if ((wifi_status == WL_DISCONNECTED) || (wifi_status == WL_NO_SSID_AVAIL)) {
          timerWifi = millis(); //сбросили таймер
          if (wifi_status == WL_NO_SSID_AVAIL) wifi_interval = 10000; //устанавливаем интервал переподключения
          else wifi_interval = 5000; //устанавливаем интервал переподключения
          WiFi.disconnect(); //отключаем wifi
        }
        else {
          wifi_interval = 0; //сбрасываем интервал переподключения
#if STATUS_LED > 0
          if (settingsMode == true) digitalWrite(LED_BUILTIN, LOW); //включаем индикацию
#endif
#if DEBUG_MODE
          Serial.println F("Wifi connect error...");
#endif
        }
        udp.stop(); //остановить udp
        break;
    }
  }

  if (wifi_interval && ((millis() - timerWifi) >= wifi_interval)) { //новое поключение
    if (WiFi.SSID().equals(settings.ssid) && WiFi.psk().equals(settings.pass) && (settings.ssid[0] != '\0')) wifi_status = WiFi.begin(); //подключаемся к wifi
    else wifi_status = WiFi.begin(settings.ssid, settings.pass); //подключаемся к wifi
    if (wifi_status != WL_CONNECT_FAILED) {
      timerWifi = millis(); //сбросили таймер
      wifi_interval = 10000; //устанавливаем интервал переподключения
#if DEBUG_MODE
      Serial.print F("Wifi connecting to \"");
      Serial.print(settings.ssid);
      Serial.println F("\"...");
#endif
    }
    else {
      wifi_interval = 0; //сбрасываем интервал
#if STATUS_LED > 0
      if (settingsMode == true) digitalWrite(LED_BUILTIN, LOW); //включаем индикацию
#endif
#if DEBUG_MODE
      Serial.println F("Wifi connection failed, wrong settings");
#endif
    }
  }
}
