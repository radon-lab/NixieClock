int8_t wifi_scan_state = 2; //статус сканирования сети
uint32_t wifi_scan_timer = 0; //таймер начала поиска сети

uint8_t wifi_status = WL_IDLE_STATUS; //статус соединения wifi
uint32_t wifi_interval = 5000; //интервал переподключения к wifi

String wifi_scan_list; //список найденых wifi сетей

//--------------------------------------------------------------------
String wifiGetConnectState(void) {
  String str;
  str.reserve(200);
  str = F("<big><big>");

  if (!settings.ssid[0]) str += F("Некорректное имя сети!");
  else {
    if (wifi_status == WL_CONNECTED) str += F("Подключено к \"");
    else if (!wifi_interval) str += F("Не удалось подключиться к \"");
    else str += F("Подключение к \"");
    str += settings.ssid;
    if ((wifi_status == WL_CONNECTED) || !wifi_interval) str += F("\"");
    else str += F("\"...");
  }
  str += F("</big></big>");
  return str;
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
void wifiScanInitStr(void) {
  wifi_scan_list.reserve(500);
  wifi_scan_list = F("Нет сетей");
}
//--------------------------------------------------------------------
void wifiScanResult(int networksFound) {
  wifi_scan_list = "";
  if (networksFound) {
    wifi_scan_state = -1;
    for (int i = 0; i < networksFound; i++) {
      if (i) wifi_scan_list += ',';
      wifi_scan_list += WiFi.SSID(i);
      if (WiFi.encryptionType(i) != ENC_TYPE_NONE) wifi_scan_list += F(" 🔒");
    }
  }
  else {
    wifi_scan_state = -2;
    wifi_scan_list = F("Нет сетей");
  }
}
//--------------------------------------------------------------------
void wifiSetConnectMode(void) {
  if (WiFi.getAutoConnect() != false) WiFi.setAutoConnect(false);
  if (WiFi.getAutoReconnect() != true) WiFi.setAutoReconnect(true);
}
//--------------------------------------------------------------------
void wifiStartAP(void) {
  //настраиваем режим работы
  WiFi.mode(WIFI_AP_STA);
  Serial.println F("");

  //настраиваем точку доступа
  IPAddress local(AP_IP);
  IPAddress subnet(255, 255, 255, 0);

  //задаем настройки сети
  WiFi.softAPConfig(local, local, subnet);

  //запускаем точку доступа
  if (!WiFi.softAP((settings.nameAp) ? (AP_SSID + String(" - ") + settings.name) : AP_SSID, AP_PASS, AP_CHANNEL)) Serial.println F("Wifi access point start failed, wrong settings");
  else {
    Serial.print F("Wifi access point enable, [ ssid: ");
    Serial.print((settings.nameAp) ? (AP_SSID + String(" - ") + settings.name) : AP_SSID);
    if (AP_PASS[0] != '\0') {
      Serial.print F(" ][ pass: ");
      Serial.print(AP_PASS);
    }
    else Serial.print F(" ][ open ");
    Serial.print F(" ][ ip: ");
    Serial.print(WiFi.softAPIP());
    Serial.println F(" ]");
  }

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
      Serial.println F("Wifi disconnecting...");
      ntpStop(); //остановили ntp
      weatherDisconnect(); //отключились от сервера погоды
      WiFi.disconnect(); //отключаемся от точки доступа
      if (WiFi.getMode() != WIFI_AP_STA) wifiStartAP(); //включаем точку доступа
    }
    wifi_status = WiFi.status();
    switch (wifi_status) {
      case WL_CONNECTED:
        timerWifi = millis(); //сбросили таймер
        wifi_interval = 300000; //устанавливаем интервал отключения точки доступа

        ntpStart(); //запустить ntp
        weatherCheck(); //запросить прогноз погоды

#if STATUS_LED == 1
        digitalWrite(LED_BUILTIN, HIGH); //выключаем индикацию
#endif
        Serial.print F("Wifi connected, IP address: ");
        Serial.println(WiFi.localIP());
        break;
      case WL_IDLE_STATUS:
#if STATUS_LED == 1
        digitalWrite(LED_BUILTIN, LOW); //включаем индикацию
#endif
        Serial.println F("Wifi idle status");
        break;
      default:
        if ((wifi_status == WL_DISCONNECTED) || (wifi_status == WL_NO_SSID_AVAIL)) {
          timerWifi = millis(); //сбросили таймер
          if (wifi_status == WL_NO_SSID_AVAIL) wifi_interval = 30000; //устанавливаем интервал ожидания
          else wifi_interval = 5000; //устанавливаем интервал переподключения
          wifi_station_disconnect(); //отключаемся от точки доступа
          Serial.println F("Wifi reconnect...");
        }
        else {
          wifi_interval = 0; //сбрасываем интервал переподключения
#if STATUS_LED == 1
          digitalWrite(LED_BUILTIN, LOW); //включаем индикацию
#endif
          Serial.println F("Wifi connect error...");
        }
        ntpStop(); //остановили ntp
        weatherDisconnect(); //отключились от сервера погоды
        break;
    }
  }

  if (wifi_interval && ((millis() - timerWifi) >= wifi_interval)) {
    if (wifi_status == WL_CONNECTED) { //если подключены
      wifi_interval = 0; //сбрасываем интервал переподключения
      WiFi.mode(WIFI_STA); //отключили точку доступа
      Serial.println F("Wifi access point disabled");
    }
    else { //иначе новое поключение
      wifi_status = WiFi.begin(settings.ssid, settings.pass); //подключаемся к wifi
      if (wifi_status != WL_CONNECT_FAILED) {
        timerWifi = millis(); //сбросили таймер
        wifi_interval = 30000; //устанавливаем интервал ожидания
        Serial.print F("Wifi connecting to \"");
        Serial.print(settings.ssid);
        Serial.println F("\"...");
      }
      else {
        wifi_interval = 0; //сбрасываем интервал переподключения
#if STATUS_LED == 1
        digitalWrite(LED_BUILTIN, LOW); //включаем индикацию
#endif
        Serial.println F("Wifi connection failed, wrong settings");
      }
    }
  }
}
