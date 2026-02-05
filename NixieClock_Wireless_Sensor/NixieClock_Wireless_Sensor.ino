/*
  Arduino IDE 1.8.13 версия прошивки 1.1.8 релиз от 30.01.26
  Специльно для проекта "Часы на ГРИ. Альтернативная прошивка"
  Страница проекта на форуме - https://community.alexgyver.ru/threads/chasy-na-gri-alternativnaja-proshivka.5843/

  Исходник - https://github.com/radon-lab/NixieClock
  Автор Radon-lab

  Если не установлено ядро ESP8266, "Файл -> Настройки -> Дополнительные ссылки для Менеджера плат", в окно ввода вставляете ссылку - https://arduino.esp8266.com/stable/package_esp8266com_index.json
  Далее "Инструменты -> Плата -> Менеджер плат..." находите плату esp8266 и устанавливаете версию 2.7.4!

  В "Инструменты -> Flash Size" необходимо выбрать распределение памяти в зависимости от установленного объёма FLASH:
  1МБ - FS:64KB OTA:~470KB(обновление esp по OTA).
  2МБ - FS:1MB OTA:~512KB(обновление esp по OTA).
  4МБ - FS:2MB OTA:~1019KB(обновление esp по OTA).
  8МБ - FS:6MB OTA:~1019KB(обновление esp по OTA).
*/
#include "config.h"

#define GP_NO_DNS
#define GP_NO_MDNS

#include "web/src/GyverPortalMod.h"
GyverPortalMod ui;

#include "MEMORY.h"

#include <WiFiUdp.h>
WiFiUDP udp;

//переменные
char buffSendIp[20]; //буфер ip адреса
uint8_t buffSendAttempt; //буфер количества попыток
uint8_t buffSendData[UDP_SEND_SIZE]; //буфер отправки

uint32_t rtcMemory[9]; //память нажатий кнопки сброса

boolean sendAnswerWait = false; //флаг ожидания ответа от хоста
uint8_t sendHostNum = 0; //текущий номер хоста
uint8_t sendHostAttempt = 0; //текущий номер попытки отправки данных хоста
uint32_t sendAnswerTimer = 0; //таймер ожидания ответа от хоста

boolean otaUpdate = true; //флаг запрета обновления есп
boolean settingsMode = false; //флаг режима настройки сенсора

boolean sensorReady = false; //флаг окончания замера температуры
uint8_t sensorStartWait = 0; //время ожидания после первого запуска

uint16_t vccVoltage = 0; //напряжение питания

uint32_t sysCycleCount = 0; //счетчик циклов процессора

const uint8_t sleepTime[] = {1, 5, 10, 15, 30, 60};
const char sleepTimeList[] = "Каждую 1 мин,Каждые 5 мин,Каждые 10 мин,Каждые 15 мин,Каждые 30 мин,Каждый 1 час";

//температура
struct sensorData {
  int16_t temp = 0x7FFF; //температура
  uint16_t press = 0; //давление
  uint8_t hum = 0; //влажность
} sens;

enum {
  SENS_DHT,
  SENS_DS,
  SENS_BME,
  SENS_SHT,
  SENS_AHT,
  SENS_MAX
};

const char *tempSensList[] = {"DHT", "DS18B20", "BMP/BME", "SHT", "AHT"};

#define REG_READ(reg) (*(volatile uint32*)(reg))

#define WIFI_SETTINGS ((uint8_t*)&rtcMemory[2])
#define WIFI_LOCAL_IP ((uint8_t*)&rtcMemory[4])
#define WIFI_GATEWAY_IP ((uint8_t*)&rtcMemory[5])
#define WIFI_SUBNET_MASK ((uint8_t*)&rtcMemory[6])
#define WIFI_DNS_1 ((uint8_t*)&rtcMemory[7])
#define WIFI_DNS_2 ((uint8_t*)&rtcMemory[8])

#if (LED_BUILTIN == TWI_SDA_PIN) || (LED_BUILTIN == TWI_SCL_PIN)
#undef STATUS_LED
#define STATUS_LED -1
#endif

#include "WIRE.h"
#include "AHT.h"
#include "SHT.h"
#include "BME.h"
#include "DHT.h"
#include "DS.h"

#include "WIFI.h"
#include "PING.h"

ADC_MODE(ADC_VCC); //режим измерения напряжения питания

void build(void) {
  GP.BUILD_BEGIN(GP_DEFAULT_THEME);

  GP.SELECT_LIST_STYLE(UI_BLOCK_COLOR, UI_BUTTON_COLOR);

  GP.PAGE_ZOOM(90, 390);
  GP.PAGE_BLOCK_BEGIN(700);

  //обновления блоков
  String updateList;
  updateList.reserve(500);
  updateList = F("barBat");

  //ссылки меню
  String menuLinks;
  menuLinks.reserve(100);
  menuLinks = F("/,/update,");

  //начать меню
  if (ui.uri("/connection")) menuLinks += F("/connection");
  else if (ui.uri("/manual")) menuLinks += F("/manual");
  else menuLinks += F("/network");
  GP.NAV_BAR_LINKS(menuLinks, "Инфо,Обновление,Сеть", 750, UI_MENU_COLOR);

  GP.BOX_BEGIN(GP_JUSTIFY, "100%;width:auto;padding-left:2%;padding-right:2%");
  GP.LABEL_BLOCK(getBatteryState(), "barBat", UI_BAR_BATTERY_COLOR, 18, 1);

  GP.BOX_BEGIN(GP_RIGHT, "100%");
  if (settings.sensor) {
    if (sens.temp != 0x7FFF) {
      updateList += F(",barTemp");
      GP.LABEL_BLOCK(String(sens.temp / 10.0, 1) + "°С", "barTemp", UI_BAR_TEMP_COLOR, 18, 1);
    }
    if (sens.hum) {
      updateList += F(",barHum");
      GP.LABEL_BLOCK(String(sens.hum) + "%", "barHum", UI_BAR_HUM_COLOR, 18, 1);
    }
    if (sens.press) {
      updateList += F(",barPress");
      GP.LABEL_BLOCK(String(sens.press) + "mm.Hg", "barPress", UI_BAR_PRESS_COLOR, 18, 1);
    }
  }
  else {
    GP.LABEL_BLOCK("-.-°С", "barTemp", UI_BAR_TEMP_COLOR, 18, 1);
  }
  GP.BOX_END();

  GP.BOX_END();
  GP.HR(UI_BAR_LINE_COLOR);

  if (ui.uri("/")) { //информация о системе
    GP.PAGE_TITLE("Информация");

    updateList += F(",infSignal,infPower,infUptime,infUsage");

    String sensorsList;
    sensorsList.reserve(150);
    sensorsList = "";

    GP.BLOCK_BEGIN(GP_THIN, "", "Датчик температуры", UI_BLOCK_COLOR);
    for (uint8_t i = 0; i < SENS_MAX; i++) {
      if (settings.sensor & (0x01 << i)) {
        if (sensorsList[0] != '\0') sensorsList += '+';
        sensorsList += tempSensList[i];
      }
    }
    M_BOX(GP.LABEL("Датчик", "", UI_LABEL_COLOR); GP.NUMBER("", (sensorsList[0] == '\0') ? "Отсутсвует" : sensorsList, INT32_MAX, "", true););
    M_BOX(GP.LABEL("Интервал", "", UI_LABEL_COLOR); GP.SELECT_LIST("extPeriod", sleepTimeList, settings.period););
    GP.BLOCK_END();

    GP.BLOCK_BEGIN(GP_THIN, "", "Системная информация", UI_BLOCK_COLOR);
    GP.BLOCK_HIDE_BEGIN();
    M_BOX(GP.LABEL("Уровень сигнала", "", UI_LABEL_COLOR); GP.LABEL(stringGetPercent(wifiGetSignal()), "infSignal", UI_INFO_COLOR););
    M_BOX(GP.LABEL("Режим модема", "", UI_LABEL_COLOR); GP.LABEL(wifiGetCurrentMode(), "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("MAC адрес", "", UI_LABEL_COLOR); GP.LABEL(WiFi.macAddress(), "", UI_INFO_COLOR););

    if (wifiGetConnectStatus()) {
      GP.BREAK();
      M_BOX(GP.LABEL("SSID сети", "", UI_LABEL_COLOR); GP.LABEL(stringLengthConstrain(WiFi.SSID(), 12), "hint1", UI_INFO_COLOR););
      M_BOX(GP.LABEL("IP сети", "", UI_LABEL_COLOR); GP.LABEL(WiFi.localIP().toString(), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("Шлюз сети", "", UI_LABEL_COLOR); GP.LABEL(WiFi.gatewayIP().toString(), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("Маска подсети", "", UI_LABEL_COLOR); GP.LABEL(WiFi.subnetMask().toString(), "", UI_INFO_COLOR););
      GP.HINT("hint1", WiFi.SSID()); //всплывающая подсказка
    }
    if (WiFi.getMode() != WIFI_STA) {
      GP.BREAK();
      M_BOX(GP.LABEL("SSID точки доступа", "", UI_LABEL_COLOR); GP.LABEL(stringLengthConstrain(AP_SSID, 12), "hint2", UI_INFO_COLOR););
      M_BOX(GP.LABEL("IP точки доступа", "", UI_LABEL_COLOR); GP.LABEL(WiFi.softAPIP().toString(), "", UI_INFO_COLOR););
      GP.HINT("hint2", AP_SSID); //всплывающая подсказка
    }

    GP.BREAK();
    GP.HR_TEXT("Память устройства", UI_LINE_COLOR, UI_HINT_COLOR);

    M_BOX(GP.LABEL("Фрагментировано(Heap)", "", UI_LABEL_COLOR); GP.LABEL(stringGetPercent(ESP.getHeapFragmentation()), "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("Свободно(Heap)", "", UI_LABEL_COLOR); GP.LABEL(stringGetKilobyte(ESP.getFreeHeap(), 3), "", UI_INFO_COLOR););

    GP.BREAK();
    M_BOX(GP.LABEL("Всего(Flash)", "", UI_LABEL_COLOR); GP.LABEL(stringGetKilobyte(ESP.getFlashChipSize(), 1), "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("Занято(Flash)", "", UI_LABEL_COLOR); GP.LABEL(stringGetKilobyte(ESP.getSketchSize(), 1), "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("Свободно(Flash)", "", UI_LABEL_COLOR); GP.LABEL(stringGetKilobyte(ESP.getFreeSketchSpace(), 1), "", UI_INFO_COLOR););

    GP.BREAK();
    GP.HR_TEXT("Параметры", UI_LINE_COLOR, UI_HINT_COLOR);

    M_BOX(GP.LABEL("ID чипа", "", UI_LABEL_COLOR); GP.LABEL(stringGetHex(ESP.getChipId()), "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("Частота процессора", "", UI_LABEL_COLOR); GP.LABEL(stringGetFreq(ESP.getCpuFreqMHz()), "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("Загрузка процессора", "", UI_LABEL_COLOR); GP.LABEL(stringGetPercent(systemGetUsage()), "infUsage", UI_INFO_COLOR););

    GP.BREAK();
    M_BOX(GP.LABEL("Напряжение питания", "", UI_LABEL_COLOR); GP.LABEL(stringGetVoltage(getBatteryVoltage()), "infPower", UI_INFO_COLOR););
    M_BOX(GP.LABEL("Время работы", "", UI_LABEL_COLOR); GP.LABEL(stringGetTimeFromMs(millis()), "infUptime", UI_INFO_COLOR););

    GP.BREAK();
    GP.HR_TEXT("Версия ПО", UI_LINE_COLOR, UI_HINT_COLOR);

    M_BOX(GP.LABEL("SDK", "", UI_LABEL_COLOR); GP.LABEL(ESP.getSdkVersion(), "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("CORE", "", UI_LABEL_COLOR); GP.LABEL(ESP.getCoreVersion(), "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("GyverPortal", "", UI_LABEL_COLOR); GP.LABEL(GP_VERSION, "", UI_INFO_COLOR););

    GP.BREAK();
    M_BOX(GP.LABEL("UID", "", UI_LABEL_COLOR); GP.LABEL(getSensorId(), "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("Firmware", "", UI_LABEL_COLOR); GP.LABEL(ESP_FIRMWARE_VERSION, "", UI_INFO_COLOR););

    GP.BREAK();
    GP.BLOCK_END();
    GP.BLOCK_END();

    GP.FOOTER_BEGIN();
    GP.TEXT_LINK("https://github.com/radon-lab/", "@radon_lab", "user", "#bbb");
    GP.BREAK();
    GP.TEXT_LINK("https://community.alexgyver.ru/threads/chasy-na-gri-v2-alternativnaja-proshivka.5843/", "Обсуждение на форуме", "forum", "#e67b09");
    GP.FOOTER_END();
  }
  else if (ui.uri("/update") && otaUpdate) { //обновление ESP
    GP.PAGE_TITLE("Обновление");

    GP.BLOCK_BEGIN(GP_THIN, "", "Обновление прошивки", UI_BLOCK_COLOR);
    GP.SPAN("Прошивку можно получить в Arduino IDE: Скетч -> Экспорт бинарного файла (сохраняется в папку с прошивкой).", GP_CENTER, "", UI_INFO_COLOR); //описание
    GP.BREAK();
    GP.SPAN("Поддерживаемые форматы файлов: bin и bin.gz.", GP_CENTER, "", UI_INFO_COLOR); //описание
    GP.BREAK();
    GP.HR_TEXT("Загрузить файлы", UI_LINE_COLOR, UI_HINT_COLOR);
    M_BOX(GP.LABEL("Прошивка ESP", "", UI_LABEL_COLOR); GP.OTA_FIRMWARE("📥", UI_BUTTON_COLOR, true););
    GP.VOID_BOX("0;height:10px");
    GP.BLOCK_END();
  }
  else { //подключение к роутеру
    GP.PAGE_TITLE("Сетевые настройки");

    GP.BLOCK_BEGIN(GP_THIN, "", "Локальная сеть WIFI", UI_BLOCK_COLOR);
    if (wifiGetConnectStatus() || wifiGetConnectWaitStatus()) { //подключение к сети
      if (!wifiGetConnectStatus()) {
        updateList += F(",extConnect,extNetwork");
        GP.RELOAD("extConnect");
      }

      GP.FORM_BEGIN("/network");
      GP.TEXT("", "", wifiGetLocalSSID(), "", 0, "", true);
      GP.BREAK();
      GP.TEXT("", "", wifiGetLocalIP(), "", 0, "", true);
      GP.SPAN(wifiGetConnectState(), GP_CENTER, "extNetwork", UI_INFO_COLOR); //описание
      GP.HR(UI_LINE_COLOR);
      GP.SUBMIT((wifiGetConnectStatus()) ? "Отключиться" : "Отмена", UI_BUTTON_COLOR);
      GP.FORM_END();
    }
    else { //выбор сети
      if (wifiGetScanCompleteStatus()) wifiResetScanCompleteStatus();

      updateList += F(",extScan");
      GP.RELOAD("extScan");

      GP.FORM_BEGIN("/connection");
      if (ui.uri("/manual")) { //ручной режим ввода сети
        GP.TEXT("wifiSsid", "SSID", settings.ssid, "", 64);
        GP.BREAK();
        GP.PASS_EYE("wifiPass", "Пароль", settings.pass, 64);
        GP.BREAK();
        GP.TEXT_LINK("/network", "Список сетей", "net", UI_LINK_COLOR);
        GP.HR(UI_LINE_COLOR);
        GP.BOX_BEGIN(GP_CENTER, "300px");
        GP.SUBMIT("Подключиться", UI_BUTTON_COLOR);
        GP.BUTTON("extClear", "✕", "", (!settings.ssid[0] && !settings.pass[0]) ? GP_GRAY : UI_BUTTON_COLOR, "65px", (boolean)(!settings.ssid[0] && !settings.pass[0]), true);
        GP.BOX_END();
      }
      else { //выбор сети из списка
        GP.SELECT_LIST("wifiSsid", wifi_scan_list, 0, 0, wifiGetScanFoundStatus());
        GP.BREAK();
        GP.PASS_EYE("wifiPass", "Пароль", settings.pass, 64);
        GP.BREAK();
        GP.TEXT_LINK("/manual", "Ручной режим", "net", UI_LINK_COLOR);
        GP.HR(UI_LINE_COLOR);
        GP.BOX_BEGIN(GP_CENTER, "300px");
        if (wifiGetScanFoundStatus()) GP.BUTTON("", "Подключиться", "", GP_GRAY, "90%", true);
        else GP.SUBMIT("Подключиться", UI_BUTTON_COLOR);
        GP.BUTTON("extScan", "<big><big>↻</big></big>", "", UI_BUTTON_COLOR, "65px", false, true);
        GP.BOX_END();
      }
      GP.FORM_END();
    }
    GP.BLOCK_END();

    if (wifiGetConnectStatus()) { //сетевые настройки
      GP.BLOCK_BEGIN(GP_THIN, "", "Отправка данных", UI_BLOCK_COLOR);

      IPAddress addrSend;
      for (uint8_t i = 0; i < MAX_CLOCK; i++) {
        if (i) {
          GP.HR(UI_LINE_COLOR);
        }
        addrSend = settings.send[i];
        if (addrSend) {
          M_BOX(
            GP.TEXT("", "IP адрес", addrSend.toString() + " [x" + settings.attempt[i] + ']', "90%;max-width:408px", 20, "", true);
            GP.BUTTON_MINI(String("extSendDel/") + i, "Удалить", "", UI_BUTTON_COLOR, "auto;min-width:110px", false, true);
          );
        }
        else {
          M_BOX(
            M_BOX(GP_LEFT, GP.TEXT("extSendIp", "IP адрес", "", "", 15); GP.NUMBER("extSendAttempt", "Попыток", INT32_MAX, "", false););
            GP.BUTTON_MINI("extSendAdd", "Добавить", "", UI_BUTTON_COLOR, "auto;min-width:110px", false, true);
          );
          buffSendIp[0] = '\0';
          buffSendAttempt = 1;
          break;
        }
      }
      GP.BLOCK_END();
    }
  }

  GP.UPDATE(updateList);

  GP.PAGE_BLOCK_END();
  GP.BUILD_END();
}
//--------------------------------------------------------------------
void buildUpdate(bool UpdateEnd, const String & UpdateError) {
  GP.BUILD_BEGIN(GP_DEFAULT_THEME);

  GP.PAGE_ZOOM(90, 390);
  GP.PAGE_MIDDLE_ALIGN();
  GP.PAGE_BLOCK_BEGIN(500);

  GP.PAGE_TITLE("Обновление");

  if (!UpdateEnd) {
    GP.BLOCK_BEGIN(GP_THIN, "", "Загрузка прошивки", UI_BLOCK_COLOR);
    GP.SPAN("Прошивку можно получить в Arduino IDE: Скетч -> Экспорт бинарного файла (сохраняется в папку с прошивкой).", GP_CENTER, "", UI_INFO_COLOR); //описание
    GP.BREAK();
    GP.SPAN("Поддерживаемые форматы файлов: bin и bin.gz.", GP_CENTER, "", UI_INFO_COLOR); //описание
    GP.BREAK();
    GP.HR_TEXT("Загрузить файлы", UI_LINE_COLOR, UI_HINT_COLOR);
    M_BOX(GP.LABEL("Прошивка ESP", "", UI_LABEL_COLOR); GP.OTA_FIRMWARE("📥", UI_BUTTON_COLOR, true););
    GP.VOID_BOX("0;height:10px");
  }
  else {
    GP.BLOCK_BEGIN(GP_THIN, "", "Обновление прошивки", UI_BLOCK_COLOR);
    GP.BLOCK_OFFSET_BEGIN();
    if (UpdateError.length()) {
      GP.SPAN("<big><b>Произошла ошибка при обновлении...</b></big><br><small>[" + UpdateError + "]</small>", GP_CENTER, "", GP_RED); //описание
    }
    else {
      GP.SPAN("<big><b>Выполняется обновление прошивки...</b></big>", GP_CENTER, "syncUpdate", UI_INFO_COLOR); //описание
      GP.SPAN("<small>Не выключайте устройство до завершения обновления!</small>", GP_CENTER, "syncWarn", GP_RED); //описание
      GP.UPDATE("syncUpdate,syncWarn");
      setUpdateCompleted();
    }
    GP.BLOCK_END();
  }
  GP.HR(UI_LINE_COLOR);
  GP.BOX_BEGIN(GP_CENTER);
  GP.BUTTON_MINI_LINK("/", "Вернуться на главную", UI_BUTTON_COLOR);
  GP.BOX_END();
  GP.BLOCK_END();

  GP.PAGE_BLOCK_END();
  GP.BUILD_END();
}
//--------------------------------------------------------------------
void action() {
  if (ui.click()) {
    if (ui.clickSub("ext")) {
      if (ui.clickSub("extSendDel")) {
        for (uint8_t multiNum = constrain(ui.clickNameSub(1).toInt(), 0, (MAX_CLOCK - 1)); multiNum < (MAX_CLOCK - 1); multiNum++) {
          settings.send[multiNum] = settings.send[multiNum + 1]; //смещаем адреса
          settings.attempt[multiNum] = settings.attempt[multiNum + 1]; //смещаем количество попыток
        }
        settings.send[MAX_CLOCK - 1] = IPAddress(0, 0, 0, 0); //сбрасываем адрес отправки
        settings.attempt[MAX_CLOCK - 1] = 1; //устанавливаем попытки по умолчанию
        memorySaveSettings(); //обновить данные в памяти
      }
      if (ui.click("extSendAdd")) {
        if (buffSendIp[0] != '\0') { //если строка не пустая
          IPAddress _send_ip; //ip адрес нового хоста
          if (_send_ip.fromString(buffSendIp)) { //если адрес действительный
            if (WiFi.localIP() != _send_ip) { //если не собственный адрес
              for (uint8_t i = 0; i < MAX_CLOCK; i++) {
                if (!settings.send[i]) { //если ячейка не заполнена
                  sendHostNum = i; //установили текущий хост
                  settings.send[i] = _send_ip; //копируем адрес
                  settings.attempt[i] = constrain(buffSendAttempt, 1, 5); //копируем количество попыток
                  memorySaveSettings(); //обновить данные в памяти
                  break;
                }
                else if (settings.send[i] == _send_ip) break;
              }
            }
          }
        }
      }

      if (ui.click("extSendIp")) {
        strncpy(buffSendIp, ui.getString("extSendIp").c_str(), 20); //копируем себе
        buffSendIp[19] = '\0'; //устанавливаем последний символ
      }
      if (ui.click("extSendAttempt")) {
        buffSendAttempt = constrain(ui.getInt("extSendAttempt"), 1, 5); //копируем себе
      }

      if (ui.click("extClear")) {
        settings.ssid[0] = '\0'; //устанавливаем последний символ
        settings.pass[0] = '\0'; //устанавливаем последний символ
        memorySaveSettings(); //обновить данные в памяти
      }
      if (ui.click("extScan")) {
        if (wifiGetScanAllowStatus()) wifiStartScanNetworks(); //начинаем поиск
      }

      if (ui.click("extPeriod")) {
        settings.period = ui.getInt("extPeriod");
        memorySaveSettings(); //обновить данные в памяти
      }
    }
  }
  /**************************************************************************/
  if (ui.form()) {
    if (!wifiGetConnectWaitStatus() && !wifiGetConnectStatus()) {
      if (ui.form("/connection")) {
        wifiSetConnectStatus(); //устанавливаем интервал переподключения
        String _ssid; //временная строка ssid сети
        _ssid.reserve(64); //резервируем всю длинну
        if (ui.copyString("wifiSsid", _ssid)) { //копируем ssid сети
          _ssid.replace(" 🔒", ""); //удаляем лишние символы
          _ssid.trim(); //удаляем пробелы
          strncpy(settings.ssid, _ssid.c_str(), 64); //копируем ssid сети
        }
        else wifiResetConnectStatus(); //сбрасываем интервал переподключения
        settings.ssid[63] = '\0'; //устанавливаем последний символ
        ui.copyStr("wifiPass", settings.pass, 64); //копируем пароль сети
        settings.pass[63] = '\0'; //устанавливаем последний символ
        memorySaveSettings(); //обновить данные в памяти
      }
    }
    else if (ui.form("/network")) {
      wifiResetConnectStatus(); //отключаемся от точки доступа
      settings.ssid[0] = '\0'; //устанавливаем последний символ
      settings.pass[0] = '\0'; //устанавливаем последний символ
      memorySaveSettings(); //обновить данные в памяти
    }
  }
  /**************************************************************************/
  if (ui.update()) {
    if (ui.updateSub("ext")) {
      if (ui.update("extNetwork")) { //если было обновление
        ui.answer(wifiGetConnectState());
      }
      if (ui.update("extConnect") && wifiGetConnectStatus()) { //если было обновление
        ui.answer(1);
      }
      if (ui.update("extScan") && wifiGetScanCompleteStatus()) { //если было обновление
        ui.answer(1);
        wifiResetScanCompleteStatus();
      }
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("inf")) {
      if (ui.update("infSignal")) { //если было обновление
        ui.answer(stringGetPercent(wifiGetSignal()));
      }
      if (ui.update("infPower")) { //если было обновление
        ui.answer(stringGetVoltage(getBatteryVoltage()));
      }
      if (ui.update("infUsage")) { //если было обновление
        ui.answer(stringGetPercent(systemGetUsage()));
      }
      if (ui.update("infUptime")) { //если было обновление
        ui.answer(stringGetTimeFromMs(millis()));
      }
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("bar")) {
      if (ui.update("barBat")) { //если было обновление
        ui.answer(getBatteryState());
      }

      if (ui.update("barTemp")) { //если было обновление
        ui.answer(String(sens.temp / 10.0, 1) + "°С");
      }
      if (ui.update("barHum")) { //если было обновление
        ui.answer(String(sens.hum) + "%");
      }
      if (ui.update("barPress")) { //если было обновление
        ui.answer(String(sens.press) + "mm.Hg");
      }
    }
  }
}
//--------------------------------------------------------------------
String stringLengthConstrain(String str, uint8_t size) {
  if (str.length() > size) {
    str.remove(size);
    str += F("…");
  }
  return str;
}
//--------------------------------------------------------------------
String stringGetPercent(uint8_t num) {
  String str;
  str.reserve(10);
  str = num;
  str += '%';
  return str;
}
//--------------------------------------------------------------------
String stringGetVoltage(float num) {
  String str;
  str.reserve(10);
  str = String(num, 2);
  str += F(" V");
  return str;
}
//--------------------------------------------------------------------
String stringGetFreq(uint32_t num) {
  String str;
  str.reserve(15);
  str = num;
  str += F(" MHz");
  return str;
}
//--------------------------------------------------------------------
String stringGetHex(uint32_t num) {
  String str;
  str.reserve(15);
  str = String(num, HEX);
  str.toUpperCase();
  str = "0x" + str;
  return str;
}
//--------------------------------------------------------------------
String stringGetKilobyte(uint32_t num, uint8_t dec) {
  String str;
  str.reserve(15);
  str = String(num / 1000.0, dec);
  str += F(" kB");
  return str;
}
//--------------------------------------------------------------------
String stringGetTimeFromMs(uint32_t data) {
  data /= 1000;

  uint8_t second = data % 60;
  data /= 60;
  uint8_t minute = data % 60;
  data /= 60;
  uint16_t hour = data % 24;
  data /= 24;

  String str;
  str.reserve(20);

  str = data;
  str += ':';
  str += hour;
  str += ':';
  str += minute / 10;
  str += minute % 10;
  str += ':';
  str += second / 10;
  str += second % 10;

  return str;
}
//--------------------------------------------------------------------
float getBatteryVoltage(void) { //получить напряжение батареи
  return (vccVoltage / 1000.0);
}
//--------------------------------------------------------------------
uint8_t getBatteryCharge(void) { //получить уровень заряда батареи
  if (vccVoltage <= BAT_VOLTAGE_MIN) return 0;
  return constrain((uint8_t)(((vccVoltage - BAT_VOLTAGE_MIN) / ((BAT_VOLTAGE_MAX - BAT_VOLTAGE_MIN) / 20.0)) + 1) * 5, 5, 100);
}
//--------------------------------------------------------------------
String getBatteryState(void) { //получить состояние батареи
  String data = "🔋 ";
  data += getBatteryCharge();
  data += '%';
  return data;
}
//--------------------------------------------------------------------
char getHexChar(uint8_t hex) {
  if (hex > 15) return 'F';
  if (hex > 9) return ('A' + (hex - 10));
  return ('0' + hex);
}
//--------------------------------------------------------------------
String getSensorId(void) {
  String str;
  str.reserve(20);
  str = "";

  uint8_t mac[6];
  efuseGetDefaultMacAddress(mac);

  for (uint8_t i = 0; i < 6; i++) {
    str += getHexChar(mac[i] >> 4);
    str += getHexChar(mac[i] & 0x0F);
    if (i < 5) str += ':';
  }

  return str;
}
//--------------------------------------------------------------------
void efuseGetDefaultMacAddress(uint8_t* mac) {
  uint32_t _efuse_low = REG_READ(0x3FF00050);
  uint32_t _efuse_mid = REG_READ(0x3FF00054);
  uint32_t _efuse_high = REG_READ(0x3FF0005C);

  mac[0] = _efuse_high >> 16;
  mac[1] = _efuse_high >> 8;
  mac[2] = _efuse_high;
  mac[3] = _efuse_mid >> 8;
  mac[4] = _efuse_mid;
  mac[5] = _efuse_low >> 24;
}
//--------------------------------------------------------------------
void checkSettingsButton(void) {
  if (ESP.rtcUserMemoryRead(32, rtcMemory, sizeof(rtcMemory))) {
    if ((rtcMemory[0] ^ rtcMemory[1]) != 0xFFFFFFFF) {
      rtcMemory[0] = 0; //сбросили нажатия кнопки
      rtcMemory[1] = 0xFFFFFFFF; //установили контрольную сумму
      rtcMemory[2] = 0x00; //сбросили блок настроек wifi
      rtcMemory[3] = 0x00; //сбросили блок настроек wifi
      rtcMemory[4] = 0x00; //сбросили блок настроек dhcp
      rtcMemory[5] = 0x00; //сбросили блок настроек dhcp
      rtcMemory[6] = 0x00; //сбросили блок настроек dhcp
      rtcMemory[7] = 0x00; //сбросили блок настроек dns
      rtcMemory[8] = 0x00; //сбросили блок настроек dns

      sensorStartWait = 2;
#if DEBUG_MODE
      Serial.println F("Settings button reset!");
#endif
    }
    else if (rtcMemory[0] < 64) {
      rtcMemory[0]++; //добавили нажатие кнопки
      rtcMemory[1] = rtcMemory[0] ^ 0xFFFFFFFF; //установили контрольную сумму
#if DEBUG_MODE
      Serial.print F("Settings button click ");
      Serial.print(rtcMemory[0]);
      Serial.println F("...");
#endif
    }
    if (rtcMemory[0] >= 3) { //если было достаточное количество нажатий
      rtcMemory[0] = 0; //сбросили нажатия кнопки
      rtcMemory[1] = 0xFFFFFFFF; //сбросили контрольную сумму
      settingsMode = true; //включить режим настройки
#if DEBUG_MODE
      Serial.println F("Settings mode enable...");
#endif
    }
  }
  if (ESP.rtcUserMemoryWrite(32, rtcMemory, sizeof(rtcMemory))) {
#if DEBUG_MODE
    Serial.println F("Settings button update!");
#endif
  }
}
//--------------------------------------------------------------------
void resetSettingsButton(void) {
  if (rtcMemory[0]) { //если кнопка была нажата
    rtcMemory[0] = 0; //сбросили нажатия кнопки
    rtcMemory[1] = 0xFFFFFFFF; //сбросили контрольную сумму
    if (ESP.rtcUserMemoryWrite(32, rtcMemory, sizeof(rtcMemory))) {
#if DEBUG_MODE
      Serial.println F("Settings button reset!");
#endif
    }
  }
}
//--------------------------------------------------------------------
void setUpdateCompleted(void) {
  rtcMemory[0] = 64; //установили флаг завершения обновления
  rtcMemory[1] = rtcMemory[0] ^ 0xFFFFFFFF; //установили контрольную сумму
  if (ESP.rtcUserMemoryWrite(32, rtcMemory, sizeof(rtcMemory))) {
#if DEBUG_MODE
    Serial.println F("Firmware mark update!");
#endif
  }
}
//--------------------------------------------------------------------
void sleepMode(void) {
#if DEBUG_MODE
#if SLEEP_MODE
  Serial.print F("Sleep mode, wake after ");
#else
  Serial.print F("Wait mode, wake after ");
#endif
  Serial.print(sleepTime[settings.period]);
  Serial.println F(" min...");
#endif
#if STATUS_LED > 0
  if (settingsMode == true) digitalWrite(LED_BUILTIN, HIGH); //выключаем индикацию
#endif

  ui.stop(); //остановить ui
  udp.stop(); //остановить udp

  WiFi.disconnect(); //отключаемся от сети
  WiFi.mode(WIFI_OFF); //отключаем wifi

#if SLEEP_MODE
  ESP.deepSleep((60e6 * sleepTime[settings.period]) - (millis() * 1000)); //уходим в сон
#else
  WiFi.forceSleepBegin(); //выключаем питание wifi
  delay((60e3 * sleepTime[settings.period]) - millis()); //ждем окончания сна
  ESP.restart(); //перезагрузка
#endif
}
//--------------------------------------------------------------------
void lowBattery(void) {
#if DEBUG_MODE
  Serial.println F("Battery low, power down...");
#endif
#if STATUS_LED > 0
  for (uint8_t i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN, (boolean)(i & 0x01));
    delay(200);
  }
  digitalWrite(LED_BUILTIN, HIGH);
#endif
  resetSettingsButton(); //сбросить нажатия кнопки настроек
  ESP.deepSleep(0); //уходим в сон
}
//--------------------------------------------------------------------
void checkBattery(void) {
  vccVoltage = ESP.getVcc();
  if (vccVoltage > BAT_VOLTAGE_CORRECT) vccVoltage -= BAT_VOLTAGE_CORRECT;
  if (!getBatteryCharge()) lowBattery();
}
//--------------------------------------------------------------------
void checkSettings(void) {
  if (settings.ssid[0] == '\0') {
    delay(1000);
    resetSettingsButton(); //сбросить нажатия кнопки настроек
#if DEBUG_MODE
    Serial.println F("Wifi is not configured, power down...");
#endif
#if STATUS_LED > 0
    for (uint8_t i = 0; i < 3; i++) {
      digitalWrite(LED_BUILTIN, (boolean)(i & 0x01));
      delay(500);
    }
    digitalWrite(LED_BUILTIN, HIGH);
#endif
    ESP.deepSleep(0); //уходим в сон
  }
}
//--------------------------------------------------------------------
void checkSensors(void) {
#if DEBUG_MODE
  Serial.println F("Sensor check start...");
#endif

  if (settingsMode == true) settings.sensor = 0; //сбросили найденные сенсоры

  sens.temp = 0x7FFF; //сбросили температуру
  sens.press = 0; //сбросили давление
  sens.hum = 0; //сбросили влажность

  sensorReady = false; //сбросили флаг готовности замера

  if (!settings.sensor) {
    if (!readTempDHT()) {
      if (!readTempDS()) {
        readTempBME();
        twi_write_stop(); //остановили шину
        readTempSHT();
        twi_write_stop(); //остановили шину
        readTempAHT();
        twi_write_stop(); //остановили шину
      }
    }
    if (settings.sensor) {
      memoryWriteSettings(); //записать данные в память
#if DEBUG_MODE
      Serial.println F("Sensor found, update memory...");
#endif
    }
    else {
#if DEBUG_MODE
      Serial.print F("Sensor not found");
#endif
      if (settingsMode == true) {
        memorySaveSettings(); //обновить данные в памяти
#if DEBUG_MODE
        Serial.println F(", update memory...");
#endif
      }
#if DEBUG_MODE
      else Serial.println F("!");
#endif
    }
  }
  else {
    for (uint8_t sensor = 0; sensor < SENS_MAX; sensor++) {
      if (settings.sensor & (0x01 << sensor)) {
        switch (sensor) {
          case SENS_DHT: readTempDHT(); return;
          case SENS_DS: readTempDS(); return;
          case SENS_BME: readTempBME(); twi_write_stop(); break;
          case SENS_SHT: readTempSHT(); twi_write_stop(); break;
          case SENS_AHT: readTempAHT(); twi_write_stop(); break;
        }
      }
    }
  }
}
//--------------------------------------------------------------------
void updateSensors(void) {
  updateTempDS(); //забрать температуру DS18B20

  if (sens.temp != 0x7FFF) { //если температура доступна
    if (sens.temp > 850) sens.temp = 850; //если вышли за предел
    if (sens.temp < -850) sens.temp = -850; //если вышли за предел
  }
  if (sens.hum > 99) sens.hum = 99; //если вышли за предел

  sensorReady = true; //установили флаг готовности замера

#if DEBUG_MODE
  Serial.println F("Sensor check end...");
#endif
}
//--------------------------------------------------------------------
void updateBuffer(void) {
  static boolean _init = false; //флаг инициализации данных

  if (_init != true) { //если данные не сформированы
    _init = true; //устанавливаем флаг инициализации данных

    efuseGetDefaultMacAddress(buffSendData); //получить mac адрес

    if (settingsMode == true) buffSendData[6] = UDP_FOUND_CMD;
    else if (settings.send[0]) buffSendData[6] = UDP_ANSWER_CMD;
    else buffSendData[6] = UDP_WRITE_CMD;

    buffSendData[7] = (uint8_t)sens.temp;
    buffSendData[8] = (uint8_t)(sens.temp >> 8);

    buffSendData[9] = (uint8_t)sens.press;
    buffSendData[10] = (uint8_t)(sens.press >> 8);

    buffSendData[11] = (uint8_t)sens.hum;

    buffSendData[12] = (uint8_t)getBatteryCharge();
    buffSendData[13] = (uint8_t)wifiGetSignal();
    buffSendData[14] = (uint8_t)sleepTime[settings.period];

    uint8_t crc = 0;
    for (uint8_t i = 0; i < (UDP_SEND_SIZE - 1); i++) checkCRC(&crc, buffSendData[i]);

    buffSendData[15] = (uint8_t)crc;

#if DEBUG_MODE
    Serial.println F("Send buffer update...");
#endif
  }
}
//--------------------------------------------------------------------
boolean sendCheck(void) {
  if (sendAnswerWait) {
    if (udp.parsePacket() == UDP_ANSWER_SIZE) {
      if (udp.remotePort() == UDP_CLOCK_PORT) {
        if (udp.remoteIP() == settings.send[sendHostNum]) {
          if (udp.read() == UDP_ANSWER_CMD) {
#if DEBUG_MODE
            Serial.println F("Send answer ok...");
#endif
            sendHostAttempt = 0;
            sendHostNum++;
          }
#if DEBUG_MODE
          else {
            Serial.println F("Send answer error!");
          }
#endif
          sendAnswerWait = false;
        }
      }
    }
    else if ((millis() - sendAnswerTimer) >= UDP_ANSWER_WAIT_TIME) {
#if DEBUG_MODE
      Serial.println F("Send answer timeout!");
#endif
      sendAnswerWait = false;
    }
  }
  return !sendAnswerWait;
}
//--------------------------------------------------------------------
void sendReset(void) {
#if DEBUG_MODE
  Serial.println F("Send wait answer...");
#endif
  udp.flush();
  sendAnswerWait = true;
  sendAnswerTimer = millis();
}
//--------------------------------------------------------------------
void sendUpdate(void) {
  if (wifiGetConnectStatus() && pingCheck() && sendCheck() && (sensorReady == true)) {
    if (sendHostNum < MAX_CLOCK) {
      if (settings.send[sendHostNum] || !sendHostNum) {
        updateBuffer(); //обновить буфер отправки
#if DEBUG_MODE
        Serial.print F("Send package to IP address: ");
        if (settings.send[sendHostNum]) {
          IPAddress addr = settings.send[sendHostNum];
          Serial.println(addr.toString());
        }
        else Serial.println(wifiGetBroadcastIP().toString());
#endif
        if (!udp.beginPacket((settings.send[sendHostNum]) ? settings.send[sendHostNum] : wifiGetBroadcastIP(), UDP_CLOCK_PORT) || (udp.write(buffSendData, UDP_SEND_SIZE) != UDP_SEND_SIZE) || !udp.endPacket()) {
#if DEBUG_MODE
          Serial.println F("Send package fail!");
#endif
        }
#if DEBUG_MODE
        else {
          Serial.println F("Send package ok...");
        }
#endif
        if (buffSendData[6] == UDP_ANSWER_CMD) {
          if (++sendHostAttempt >= settings.attempt[sendHostNum]) {
            sendHostAttempt = 0;
            sendHostNum++;
          }
          sendReset();
        }
        else sendHostNum++;
      }
      else {
        sendHostNum = MAX_CLOCK;
        if (buffSendData[6] != UDP_ANSWER_CMD) pingReset();
#if DEBUG_MODE
        Serial.println F("Send all packages completed...");
#endif
      }
    }
    else if (settingsMode == false) sleepMode(); //отключить питание
  }
}
//--------------------------------------------------------------------
void timeUpdate(void) {
  static uint8_t updateTimer = 0;
  static uint32_t secondsTimer = 0;

  if (!secondsTimer || ((millis() - secondsTimer) >= 1000)) {
    secondsTimer = millis();

    if (updateTimer == sensorStartWait) checkSensors(); //проверка доступности сенсоров
    else if (updateTimer == (sensorStartWait + 1)) updateSensors(); //обновить показания сенсоров
    else if ((updateTimer > (SETTINGS_MODE_TIME - 15)) && (settingsMode == true) && ui.online()) updateTimer = (SETTINGS_MODE_TIME - 15); //сбросить таймер
    else if (updateTimer > ((settingsMode == false) ? 15 : SETTINGS_MODE_TIME)) sleepMode(); //отключить питание

    if (updateTimer < 255) updateTimer++; //прибавили таймер секунд

    if (updateTimer > 1) {
      resetSettingsButton(); //сбросить нажатия кнопки настроек
#if STATUS_LED > 0
      if ((settingsMode == true) && wifiGetConnectWaitStatus()) digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); //мигаем индикацией
#endif
    }
  }
}
//--------------------------------------------------------------------
void systemUpdate(void) {
  ui.tick(); //обработка веб интерфейса
  if (sysCycleCount != 0xFFFFFFFF) sysCycleCount++; //посчет циклов
}
//--------------------------------------------------------------------
uint8_t systemGetUsage(void) {
  static uint32_t timer = 0;
  static uint8_t usage = 0;

  uint32_t ms = millis();
  if ((ms - timer) >= 1000) {
    usage = map(constrain(sysCycleCount / (ms - timer), 1, 25), 1, 25, 100, 0);
    sysCycleCount = 0;
    timer = ms;
  }
  return usage;
}
//--------------------------------------------------------------------
void setup() {
  //выключить питание wifi
  WiFi.forceSleepBegin();

  //инициализация индикатора
#if STATUS_LED > 0
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
#endif

  //обновить напряжение питания
  checkBattery();

#if DEBUG_MODE
  Serial.begin(115200);
  Serial.println F("");
  Serial.println F("Startup...");
  Serial.print F("Supply voltage ");
  Serial.print(getBatteryVoltage(), 2);
  Serial.println F("...");
  Serial.print F("Firmware version ");
  Serial.print F(ESP_FIRMWARE_VERSION);
  Serial.println F("...");
#endif

  //проверка кнопки режима настройки
  checkSettingsButton();

  //инициализация индикатора
#if STATUS_LED > 0
  if (settingsMode == true) digitalWrite(LED_BUILTIN, LOW);
#endif

  //инициализация шины
  pinMode(TWI_SDA_PIN, INPUT_PULLUP);
  pinMode(TWI_SCL_PIN, INPUT_PULLUP);
  twi_init();

  //инициализация обновления по OTA
  if (settingsMode == true) {
    if ((getBatteryCharge() < 50) || (ESP.getFreeSketchSpace() < ESP.getSketchSize())) {
      otaUpdate = false; //выключаем обновление
#if DEBUG_MODE
      Serial.println F("OTA update disable...");
#endif
    }
#if DEBUG_MODE
    else Serial.println F("OTA update enable...");
#endif
  }

  //устанавливаем период по умолчанию
  settings.period = SEND_DATA_PERIOD;

  //сбрасываем настройки отправки данных
  for (uint8_t i = 0; i < MAX_CLOCK; i++) {
    settings.send[i] = IPAddress(0, 0, 0, 0);
    settings.attempt[i] = 1;
  }

  //сбрасываем настройки сети
  settings.ssid[0] = '\0';
  settings.pass[0] = '\0';

  //читаем настройки из памяти
  memoryReadSettings();

  //проверяем настройки сети
  if (settingsMode == false) checkSettings();

  //подключаем конструктор и запускаем веб интерфейс
  if (settingsMode == true) {
    ui.attachBuild(build);
    ui.attach(action);
    ui.start();

    //настраиваем обновление без пароля
    if (otaUpdate) {
      ui.enableOTA();
      ui.OTA.attachUpdateBuild(buildUpdate);
    }
    ui.downloadAuto(true);
    ui.uploadAuto(false);
  }

  //включить питание wifi
  WiFi.forceSleepWake();

  //настраиваем wifi
  WiFi.persistent(false);
  if (WiFi.getAutoConnect() != false) WiFi.setAutoConnect(false);
  if (WiFi.getAutoReconnect() != false) WiFi.setAutoReconnect(false);
  if (settingsMode == true) wifiStartAP();
  else wifiStartSTA();
}
//--------------------------------------------------------------------
void loop() {
  if (wifiUpdate()) sleepMode(); //обработка статусов wifi

  timeUpdate(); //обработка времени
  sendUpdate(); //обработка канала связи

  if (settingsMode == true) {
    memoryUpdate(); //обработка памяти настроек
    systemUpdate(); //обработка системных функций
  }
}
