int8_t wifiScanState = 2; //статус сканирования сети
uint8_t wifiStatus = WL_IDLE_STATUS; //статус соединения wifi

uint32_t wifiScanTimer = 0; //таймер начала поиска сети
uint32_t wifiInterval = 5000; //интервал переподключения к wifi

String wifiScanList = "Нет сетей"; //список найденых wifi сетей

//--------------------------------------------------------------------
String getWifiState(void) {
  String data = "<big><big>";
  if (!settings.ssid[0]) data += "Некорректное имя сети!";
  else {
    if (wifiStatus == WL_CONNECTED) data += "Подключено к \"";
    else if (!wifiInterval) data += "Не удалось подключиться к \"";
    else data += "Подключение к \"";
    data += String(settings.ssid);
    if ((wifiStatus == WL_CONNECTED) || !wifiInterval) data += "\"";
    else data += "\"...";
  }
  data += "</big></big>";
  return data;
}
//--------------------------------------------------------------------
void wifiScanResult(int networksFound) {
  wifiScanList = "";
  if (networksFound) {
    wifiScanState = -1;
    for (int i = 0; i < networksFound; i++) {
      if (i) wifiScanList += ',';
      wifiScanList += WiFi.SSID(i);
      if (WiFi.encryptionType(i) != ENC_TYPE_NONE) wifiScanList += " 🔒";
    }
  }
  else {
    wifiScanState = -2;
    wifiScanList = "Нет сетей";
  }
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

  if ((wifiScanState == 127) && (millis() - wifiScanTimer) >= 100) { //если необходимо начать поиск
    wifiScanState = 0; //сбрасываем статус
    WiFi.scanNetworksAsync(wifiScanResult); //начинаем поиск
  }

  if (wifiStatus != WiFi.status()) { //если изменился статус
    if (wifiStatus == 255) { //если нужно отключиться
      Serial.println F("Wifi disconnecting...");
      ntpStop(); //остановили ntp
      weatherDisconnect(); //отключились от сервера погоды
      WiFi.disconnect(); //отключаем wifi
      if (WiFi.getMode() != WIFI_AP_STA) wifiStartAP(); //включаем точку доступа
    }
    wifiStatus = WiFi.status();
    switch (wifiStatus) {
      case WL_CONNECTED:
        timerWifi = millis(); //сбросили таймер
        wifiInterval = 300000; //устанавливаем интервал отключения точки доступа
#if STATUS_LED == 1
        digitalWrite(LED_BUILTIN, HIGH); //выключаем индикацию
#endif
        ntpStart(); //запустить ntp
        weatherCheck(); //запросить прогноз погоды

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
        if ((wifiStatus == WL_DISCONNECTED) || (wifiStatus == WL_NO_SSID_AVAIL)) {
          timerWifi = millis(); //сбросили таймер
          if (wifiStatus == WL_NO_SSID_AVAIL) wifiInterval = 30000; //устанавливаем интервал переподключения
          else wifiInterval = 5000; //устанавливаем интервал переподключения
          WiFi.disconnect(); //отключаем wifi
        }
        else {
          wifiInterval = 0; //сбрасываем интервал переподключения
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

  if (wifiInterval && ((millis() - timerWifi) >= wifiInterval)) {
    if (wifiStatus == WL_CONNECTED) { //если подключены
      wifiInterval = 0; //сбрасываем интервал переподключения
      WiFi.mode(WIFI_STA); //отключили точку доступа
      Serial.println F("Wifi access point disabled");
    }
    else { //иначе новое поключение
      wifiStatus = WiFi.begin(settings.ssid, settings.pass); //подключаемся к wifi
      if (wifiStatus != WL_CONNECT_FAILED) {
        timerWifi = millis(); //сбросили таймер
        wifiInterval = 30000; //устанавливаем интервал переподключения
        Serial.print F("Wifi connecting to \"");
        Serial.print(settings.ssid);
        Serial.println F("\"...");
      }
      else {
        wifiInterval = 0; //сбрасываем интервал
#if STATUS_LED == 1
        digitalWrite(LED_BUILTIN, LOW); //включаем индикацию
#endif
        Serial.println F("Wifi connection failed, wrong settings");
      }
    }
  }
}
