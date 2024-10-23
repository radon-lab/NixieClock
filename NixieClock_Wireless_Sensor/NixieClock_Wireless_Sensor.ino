/*
  Arduino IDE 1.8.13 версия прошивки 1.1.4 релиз от 23.10.24
  Специльно для проекта "Часы на ГРИ v2. Альтернативная прошивка"
  Страница проекта - https://community.alexgyver.ru/threads/chasy-na-gri-v2-alternativnaja-proshivka.5843/

  Исходник - https://github.com/radon-lab/NixieClock
  Автор Radon-lab

  Если не установлено ядро ESP8266, "Файл -> Настройки -> Дополнительные ссылки для Менеджера плат", в окно ввода вставляете ссылку - https://arduino.esp8266.com/stable/package_esp8266com_index.json
  Далее "Инструменты -> Плата -> Менеджер плат..." находите плату esp8266 и устанавливаете версию 2.7.4!

  В "Инструменты -> Управлять библиотеками..." необходимо предварительно установить указанные версии библиотек:
  GyverPortal 3.6.6
  EEManager 2.0.1

  В "Инструменты -> Flash Size" необходимо выбрать распределение памяти в зависимости от установленного объёма FLASH:
  1МБ - FS:64KB OTA:~470KB(обновление esp по OTA).
  2МБ - FS:1MB OTA:~512KB(обновление esp по OTA).
  4МБ - FS:2MB OTA:~1019KB(обновление esp по OTA).
  8МБ - FS:6MB OTA:~1019KB(обновление esp по OTA).
*/
#include "config.h"

#define GP_NO_DNS
#define GP_NO_MDNS

#include <GyverPortal.h>
GyverPortal ui;

struct settingsData {
  uint8_t sensor;
  uint8_t period;
  char ssid[64];
  char pass[64];
  char send[MAX_CLOCK * 2][20];
} settings;

#include <EEManager.h>
EEManager memory(settings, 3000);

#include <WiFiUdp.h>
WiFiUDP udp;

//переменные
char buffSendIp[20]; //буфер ip адреса
char buffSendName[20]; //буфер имени
uint8_t buffSendData[UDP_SEND_SIZE]; //буфер отправки

uint32_t rtcMemory[2]; //память нажатий кнопки сброса

uint8_t sendHostNum = 0; //текущий номер хоста

boolean otaUpdate = true; //флаг запрета обновления есп
boolean sendReady = false; //флаг повторной попытки отправки данных
boolean sensorReady = false; //флаг окончания замера температуры
boolean settingsMode = false; //флаг режима настройки сенсора

uint16_t vccVoltage = 0; //напряжение питания

const uint8_t sleepTime[] = {5, 10, 15, 30, 60};
const char sleepTimeList[] = "Каждые 5 мин,Каждые 10 мин,Каждые 15 мин,Каждые 30 мин,Каждый 1 час";

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
#include "utils.h"

ADC_MODE(ADC_VCC);

void build(void) {
  GP.BUILD_BEGIN(UI_MAIN_THEME, 500);
  GP_FIX_SCRIPTS(); //фикс скрипта проверки онлайна
  GP_FIX_STYLES(); //фикс стилей страницы

  //обновления блоков
  String updateList = "barBat";

  //начать меню
  GP.UI_MENU("Wireless Sensor", UI_MENU_COLOR);

  //ссылки меню
  GP.UI_LINK("/", "Об устройстве");
  if (otaUpdate) GP.UI_LINK("/update", "Обновление ПО");
  GP.UI_LINK("/network", "Сетевые настройки");

  GP_HR(UI_MENU_LINE_COLOR, 6);

  //состояние соединения
  if (wifiGetConnectStatus()) {
    updateList += ",bar_wifi";
    GP_BLOCK_SHADOW_BEGIN();
    GP.LABEL("Сигнал WiFi", "", UI_MENU_TEXT_COLOR, 15);
    GP_LINE_BAR("bar_wifi", getWiFiSignal(), 0, 100, 1, UI_MENU_WIFI_COLOR);
    GP_BLOCK_SHADOW_END();
  }

  GP_FOOTER_BEGIN();
  GP.HR(UI_MENU_LINE_COLOR);
  GP_TEXT_LINK("https://github.com/radon-lab/", "@radon_lab", "user", "#bbb");
  GP.BREAK();
  GP_TEXT_LINK("https://community.alexgyver.ru/threads/chasy-na-gri-v2-alternativnaja-proshivka.5843/", "Обсуждение на форуме", "forum", "#e67b09");
  GP_FOOTER_END();

  GP.UI_BODY(); //начать основное окно

  GP.BOX_BEGIN(GP_JUSTIFY, "auto;padding-left:2%;padding-right:2%");
  GP.LABEL_BLOCK(getBatteryState(), "barBat", UI_BAR_BATTERY_COLOR, 18, 1);

  GP.BOX_BEGIN(GP_RIGHT, "100%");
  if (settings.sensor) {
    if (sens.temp != 0x7FFF) {
      updateList += ",barTemp";
      GP.LABEL_BLOCK(String(sens.temp / 10.0, 1) + "°С", "barTemp", UI_BAR_TEMP_COLOR, 18, 1);
    }
    if (sens.hum) {
      updateList += ",barHum";
      GP.LABEL_BLOCK(String(sens.hum) + "%", "barHum", UI_BAR_HUM_COLOR, 18, 1);
    }
    if (sens.press) {
      updateList += ",barPress";
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
    GP.PAGE_TITLE("Об устройстве");

    GP.BLOCK_BEGIN(GP_THIN, "", "Системная информация", UI_BLOCK_COLOR);
    M_BOX(GP.LABEL("Уровень сигнала", "", UI_LABEL_COLOR); GP.LABEL("📶 " + String(getWiFiSignal()) + '%', "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("Режим модема", "", UI_LABEL_COLOR); GP.LABEL(WiFi.getMode() == WIFI_AP ? "AP" : (WiFi.getMode() == WIFI_STA ? "STA" : "AP_STA"), "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("MAC адрес", "", UI_LABEL_COLOR); GP.LABEL(WiFi.macAddress(), "", UI_INFO_COLOR););

    if (wifiGetConnectStatus()) {
      M_BOX(GP.LABEL("Маска подсети", "", UI_LABEL_COLOR); GP.LABEL(WiFi.subnetMask().toString(), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("Шлюз", "", UI_LABEL_COLOR); GP.LABEL(WiFi.gatewayIP().toString(), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("SSID сети", "", UI_LABEL_COLOR); GP.LABEL(StrLengthConstrain(WiFi.SSID(), 12), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("IP сети", "", UI_LABEL_COLOR); GP.LABEL(WiFi.localIP().toString(), "", UI_INFO_COLOR););
    }
    if (WiFi.getMode() != WIFI_STA) {
      M_BOX(GP.LABEL("SSID точки доступа", "", UI_LABEL_COLOR); GP.LABEL(StrLengthConstrain(AP_SSID, 12), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("IP точки доступа", "", UI_LABEL_COLOR); GP.LABEL(WiFi.softAPIP().toString(), "", UI_INFO_COLOR););
    }

    GP.BREAK();
    GP_HR_TEXT("Память устройства", "", UI_LINE_COLOR, UI_HINT_COLOR);

    M_BOX(GP.LABEL("Фрагментировано(Heap)", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getHeapFragmentation()) + '%', "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("Свободно(Heap)", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getFreeHeap() / 1000.0, 3) + " kB", "", UI_INFO_COLOR););

    M_BOX(GP.LABEL("Всего(Flash)", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getFlashChipSize() / 1000.0, 1) + " kB", "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("Занято(Flash)", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getSketchSize() / 1000.0, 1) + " kB", "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("Свободно(Flash)", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getFreeSketchSpace() / 1000.0, 1) + " kB", "", UI_INFO_COLOR););

    GP.BREAK();
    GP_HR_TEXT("О системе", "", UI_LINE_COLOR, UI_HINT_COLOR);

    M_BOX(GP.LABEL("ID чипа", "", UI_LABEL_COLOR); GP.LABEL("0x" + String(ESP.getChipId(), HEX), "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("Напряжение питания", "", UI_LABEL_COLOR); GP.LABEL(String(getBatteryVoltage(), 2) +  + F(" V"), "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("Частота процессора", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getCpuFreqMHz()) + F(" MHz"), "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("Циклов в секунду", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getCycleCount()), "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("Время работы", "", UI_LABEL_COLOR); GP.LABEL(getTimeFromMs(millis()), "", UI_INFO_COLOR););

    GP.BREAK();
    GP_HR_TEXT("Версия ПО", "", UI_LINE_COLOR, UI_HINT_COLOR);

    M_BOX(GP.LABEL("UID", "", UI_LABEL_COLOR); GP.LABEL(getSensorId(), "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("SDK", "", UI_LABEL_COLOR); GP.LABEL(ESP.getSdkVersion(), "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("CORE", "", UI_LABEL_COLOR); GP.LABEL(ESP.getCoreVersion(), "", UI_INFO_COLOR););
    M_BOX(GP.LABEL("GyverPortal", "", UI_LABEL_COLOR); GP.LABEL(GP_VERSION, "", UI_INFO_COLOR););

    M_BOX(GP.LABEL("Прошивка ESP", "", UI_LABEL_COLOR); GP.LABEL(ESP_FIRMWARE_VERSION, "", UI_INFO_COLOR););
    GP.BLOCK_END();

    String sensorsList = "";

    GP.BLOCK_BEGIN(GP_THIN, "", "Информация о датчике", UI_BLOCK_COLOR);
    for (uint8_t i = 0; i < SENS_MAX; i++) {
      if (settings.sensor & (0x01 << i)) {
        if (sensorsList[0] != '\0') sensorsList += '+';
        sensorsList += tempSensList[i];
      }
    }
    M_BOX(GP.LABEL("Датчик", "", UI_LABEL_COLOR); GP.NUMBER("", (sensorsList[0] == '\0') ? "Отсутсвует" : sensorsList, INT32_MAX, "", true););
    M_BOX(GP.LABEL("Интервал", "", UI_LABEL_COLOR); GP.SELECT("extPeriod", sleepTimeList, settings.period););
    GP.BLOCK_END();
  }
  else if (ui.uri("/update") && otaUpdate) { //обновление ESP
    GP.PAGE_TITLE("Обновление");

    GP.BLOCK_BEGIN(GP_THIN, "", "Обновление прошивки", UI_BLOCK_COLOR);
    GP.SPAN("Прошивку можно получить в Arduino IDE: Скетч -> Экспорт бинарного файла (сохраняется в папку с прошивкой).", GP_CENTER, "", UI_INFO_COLOR); //описание
    GP.BREAK();
    GP.SPAN("Поддерживаемые форматы файлов: bin и bin.gz.", GP_CENTER, "", UI_INFO_COLOR); //описание
    GP.BREAK();
    GP_HR_TEXT("Загрузить файлы", "", UI_LINE_COLOR, UI_HINT_COLOR);
    M_BOX(GP.LABEL("Прошивка ESP", "", UI_LABEL_COLOR); GP.OTA_FIRMWARE("", UI_BUTTON_COLOR, true););
    GP.BLOCK_END();
  }
  else { //подключение к роутеру
    GP.PAGE_TITLE("Сетевые настройки");

    GP.BLOCK_BEGIN(GP_THIN, "", "Локальная сеть WIFI", UI_BLOCK_COLOR);
    if (wifiGetConnectStatus() || wifiGetConnectWaitStatus()) {
      GP.FORM_BEGIN("/network");
      if (wifiGetConnectStatus()) {
        GP.TEXT("", "", settings.ssid, "", 0, "", true);
        GP.BREAK();
        GP.TEXT("", "", WiFi.localIP().toString(), "", 0, "", true);
        GP.SPAN("Подключение установлено", GP_CENTER, "", UI_INFO_COLOR); //описание
      }
      else {
        GP.SPAN(wifiGetConnectState(), GP_CENTER, "syncNetwork", UI_INFO_COLOR); //описание
        updateList += ",syncNetwork";
      }

      GP.HR(UI_LINE_COLOR);
      if (ui.uri("/connection")) {
        GP.BUTTON_LINK("/", "Вернуться на главную", UI_BUTTON_COLOR);
      }
      else {
        GP.SUBMIT("Отключиться", UI_BUTTON_COLOR);
      }
      GP.FORM_END();
    }
    else {
      if (wifiGetScanCompleteStatus()) wifiResetScanCompleteStatus();

      updateList += ",syncReload";
      GP.RELOAD("syncReload");

      GP.FORM_BEGIN("/connection");
      if (ui.uri("/manual")) {
        GP.TEXT("wifiSsid", "SSID", settings.ssid, "", 64);
        GP.BREAK();
        GP.PASS_EYE("wifiPass", "Пароль", settings.pass, "100%", 64);
        GP.BREAK();
        GP_TEXT_LINK("/network", "Список сетей", "net", UI_LINK_COLOR);
        GP.HR(UI_LINE_COLOR);
        GP.SEND("<div style='max-width:300px;justify-content:center' class='inliner'>\n");
        GP.SUBMIT("Подключиться", UI_BUTTON_COLOR);
        GP.BUTTON("extClear", "✕", "", (!settings.ssid[0] && !settings.pass[0]) ? GP_GRAY : UI_BUTTON_COLOR, "65px", (boolean)(!settings.ssid[0] && !settings.pass[0]), true);
        GP.SEND("</div>\n");
      }
      else {
        GP.SELECT("wifiNetwork", wifi_scan_list, 0, 0, wifiGetScanFoundStatus());
        GP.BREAK();
        GP.PASS_EYE("wifiPass", "Пароль", settings.pass, "100%", 64);
        GP.BREAK();
        GP_TEXT_LINK("/manual", "Ручной режим", "net", UI_LINK_COLOR);
        GP.HR(UI_LINE_COLOR);
        GP.SEND("<div style='max-width:300px;justify-content:center' class='inliner'>\n");
        if (wifiGetScanFoundStatus()) GP.BUTTON("", "Подключиться", "", GP_GRAY, "", true);
        else GP.SUBMIT("Подключиться", UI_BUTTON_COLOR);
        GP.BUTTON("extScan", "<big><big>↻</big></big>", "", UI_BUTTON_COLOR, "65px", false, true);
        GP.SEND("</div>\n");
      }
      GP.FORM_END();
    }
    GP.BLOCK_END();

    if (ui.uri("/network") && wifiGetConnectStatus()) { //сетевые настройки
      GP.BLOCK_BEGIN(GP_THIN, "", "Отправка данных", UI_BLOCK_COLOR);
      for (uint8_t i = 0; i < (MAX_CLOCK * 2); i += 2) {
        if (i) {
          GP.HR(UI_MENU_LINE_COLOR);
        }
        if (settings.send[i][0] != '\0') {
          M_BOX(
            M_BOX(GP_LEFT, GP.TEXT("", "IP адрес", settings.send[i], "", 20, "", true); GP.TEXT("", "Название", (settings.send[i + 1][0] != '\0') ? settings.send[i + 1] : settings.send[i], "", 20, "", true););
            GP.BUTTON_MINI(String("extSendDel/") + i, "Удалить", "", UI_BUTTON_COLOR, "115px!important", false, true);
          );
        }
        else {
          M_BOX(
            M_BOX(GP_LEFT, GP.TEXT("extSendIp", "IP адрес", "", "", 15); GP.TEXT("extSendName", "Название", "", "", 19););
            GP.BUTTON_MINI("extSendAdd", "Добавить", "", UI_BUTTON_COLOR, "115px!important", false, true);
          );
          buffSendIp[0] = '\0';
          buffSendName[0] = '\0';
          break;
        }
      }
      GP.BLOCK_END();
    }
  }

  GP.UPDATE(updateList);
  GP.UI_END(); //завершить окно панели управления

  GP_BUILD_END();
}

void buildUpdater(bool UpdateEnd, const String & UpdateError) {
  GP.BUILD_BEGIN(UI_MAIN_THEME, 500);
  GP_FIX_SCRIPTS(); //фикс скрипта проверки онлайна
  GP_FIX_STYLES(); //фикс стилей страницы

  GP.PAGE_TITLE("Обновление");

  GP.BLOCK_BEGIN(GP_THIN, "", "Обновление прошивки", UI_BLOCK_COLOR);
  if (!UpdateEnd) {
    GP.SPAN("<b>Прошивку можно получить в Arduino IDE: Скетч -> Экспорт бинарного файла (сохраняется в папку с прошивкой).</b><br>Поддерживаемые форматы файлов bin и bin.gz.", GP_CENTER, "", UI_INFO_COLOR); //описание
    GP.HR(UI_LINE_COLOR);
    M_BOX(GP_CENTER, GP.OTA_FIRMWARE("", UI_BUTTON_COLOR, true); GP.BUTTON_MINI_LINK("/", "Вернуться на главную", UI_BUTTON_COLOR););
  }
  else if (UpdateError.length()) {
    GP.SPAN("<big><b>Произошла ошибка при обновлении...</b></big><br><small>[" + UpdateError + "]</small>", GP_CENTER, "", GP_RED); //описание
    GP.HR(UI_LINE_COLOR);
    M_BOX(GP_CENTER, GP_BUTTON_MINI_LINK("/ota_update", "⠀<big><big>↻</big></big>⠀", UI_BUTTON_COLOR); GP.BUTTON_MINI_LINK("/", "Вернуться на главную", UI_BUTTON_COLOR););
  }
  else {
    GP.SPAN("<big><b>Выполняется обновление прошивки...</b></big>", GP_CENTER, "syncUpdate", UI_INFO_COLOR); //описание
    GP.SPAN("<small>Не выключайте устройство до завершения обновления!</small>", GP_CENTER, "syncWarn", GP_RED); //описание
    GP.HR(UI_LINE_COLOR);
    M_BOX(GP_CENTER, GP.BUTTON_MINI_LINK("/", "Вернуться на главную", UI_BUTTON_COLOR););
    GP.UPDATE("syncUpdate,syncWarn");
  }
  GP.BLOCK_END();

  GP_BUILD_END();
}

void action() {
  if (ui.click()) {
    if (ui.clickSub("ext")) {
      if (ui.clickSub("extSendDel")) {
        for (uint8_t multiNum = constrain(ui.clickNameSub(1).toInt(), 0, ((MAX_CLOCK - 1) * 2)); multiNum < ((MAX_CLOCK - 1) * 2); multiNum++) {
          strncpy(settings.send[multiNum], settings.send[multiNum + 2], 20); //копируем себе
        }
        settings.send[((MAX_CLOCK - 1) * 2)][0] = '\0'; //устанавливаем последний символ
        settings.send[((MAX_CLOCK - 1) * 2) + 1][0] = '\0'; //устанавливаем последний символ

        if (settings.send[2][0] == '\0') {
          settings.send[0][0] = '\0'; //устанавливаем последний символ
          settings.send[1][0] = '\0'; //устанавливаем последний символ
        }
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("extSendAdd")) {
        if (buffSendIp[0] != '\0') { //если строка не пустая
          if (!WiFi.localIP().toString().equals(buffSendIp)) { //если не собственный адрес
            for (uint8_t i = 0; i < (MAX_CLOCK * 2); i += 2) {
              if (settings.send[i][0] == '\0') { //если ячейка не заполнена
                sendHostNum = 0; //сбросили текущий хост
                strncpy(settings.send[i], buffSendIp, 20); //копируем себе
                settings.send[i][19] = '\0'; //устанавливаем последний символ
                strncpy(settings.send[i + 1], buffSendName, 20); //копируем себе
                settings.send[i + 1][19] = '\0'; //устанавливаем последний символ
                memory.update(); //обновить данные в памяти
                break;
              }
              else if (String(settings.send[i]).equals(buffSendIp)) break;
            }
          }
        }
      }

      if (ui.click("extSendIp")) {
        strncpy(buffSendIp, ui.getString("extSendIp").c_str(), 20); //копируем себе
        buffSendIp[19] = '\0'; //устанавливаем последний символ
      }
      if (ui.click("extSendName")) {
        strncpy(buffSendName, ui.getString("extSendName").c_str(), 20); //копируем себе
        buffSendName[19] = '\0'; //устанавливаем последний символ
      }

      if (ui.click("extClear")) {
        settings.ssid[0] = '\0'; //устанавливаем последний символ
        settings.pass[0] = '\0'; //устанавливаем последний символ
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("extScan")) {
        if (wifiGetScanAllowStatus()) wifiStartScanNetworks(); //начинаем поиск
      }

      if (ui.click("extPeriod")) {
        settings.period = ui.getInt("extPeriod");
        memory.update(); //обновить данные в памяти
      }
    }
  }
  /**************************************************************************/
  if (ui.form()) {
    if (!wifiGetConnectWaitStatus() && !wifiGetConnectStatus()) {
      if (ui.form("/connection")) {
        wifiSetConnectWaitInterval(1); //устанавливаем интервал переподключения
        if (!ui.copyStr("wifiSsid", settings.ssid, 64)) { //копируем из строки
          int network = 0; //номер сети из списка
          if (ui.copyInt("wifiNetwork", network)) strncpy(settings.ssid, WiFi.SSID(network).c_str(), 64); //копируем из списка
          else wifiSetConnectWaitInterval(0); //сбрасываем интервал переподключения
        }
        settings.ssid[63] = '\0'; //устанавливаем последний символ
        ui.copyStr("wifiPass", settings.pass, 64); //копируем пароль сети
        settings.pass[63] = '\0'; //устанавливаем последний символ
        memory.update(); //обновить данные в памяти
      }
    }
    else if (ui.form("/network")) {
      wifiResetConnectStatus(); //отключаемся от точки доступа
      settings.ssid[0] = '\0'; //устанавливаем последний символ
      settings.pass[0] = '\0'; //устанавливаем последний символ
      memory.update(); //обновить данные в памяти
    }
  }
  /**************************************************************************/
  if (ui.update()) {
    if (ui.updateSub("sync")) {
      if (ui.update("syncNetwork")) { //если было обновление
        ui.answer(wifiGetConnectState());
      }
      if (ui.update("syncReload") && wifiGetScanCompleteStatus()) { //если было обновление
        ui.answer(1);
        wifiResetScanCompleteStatus();
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

      if (ui.update("bar_wifi")) { //если было обновление
        ui.answer(getWiFiSignal());
      }
    }
  }
}
//---------------------------Получить напряжение батареи---------------------------------
float getBatteryVoltage(void) { //получить состояние батареи
  return (vccVoltage / 1000.0);
}
//---------------------------Получить состояние батареи----------------------------------
uint8_t getBatteryCharge(void) { //получить состояние батареи
  if (vccVoltage < BAT_VOLTAGE_MIN) return 0;
  return map(constrain(vccVoltage, BAT_VOLTAGE_MIN, BAT_VOLTAGE_MAX), BAT_VOLTAGE_MAX, BAT_VOLTAGE_MIN, 20, 0) * 5;
}
//---------------------------Получить состояние батареи----------------------------------
String getBatteryState(void) { //получить состояние батареи
  String data = "🔋 ";
  data += getBatteryCharge();
  data += '%';
  return data;
}
//---------------------------Получить состояние батареи----------------------------------
uint8_t getWiFiSignal(void) { //получить состояние батареи
  return constrain(2 * (WiFi.RSSI() + 100), 0, 100);
}
//--------------------------------------------------------------------
String getTimeFromMs(uint32_t data) {
  data /= 1000;

  uint8_t second = data % 60;
  data /= 60;
  uint8_t minute = data % 60;
  data /= 60;
  uint16_t hour = data % 24;
  data /= 24;

  String str = "";

  str += data;
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
String StrLengthConstrain(String data, uint8_t size) {
  if (data.length() > size) {
    data.remove(size);
    data += "…";
  }
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
void checkCRC(uint8_t* crc, uint8_t data) { //сверка контрольной суммы
  for (uint8_t i = 0; i < 8; i++) { //считаем для всех бит
    *crc = ((*crc ^ data) & 0x01) ? (*crc >> 0x01) ^ 0x8C : (*crc >> 0x01); //рассчитываем значение
    data >>= 0x01; //сдвигаем буфер
  }
}
//--------------------------------------------------------------------
void checkSettingsButton(void) {
  if (ESP.rtcUserMemoryRead(32, rtcMemory, sizeof(rtcMemory))) {
    if ((rtcMemory[0] ^ rtcMemory[1]) != 0xFFFFFFFF) {
      rtcMemory[0] = 0; //сбросили нажатия кнопки
      rtcMemory[1] = 0xFFFFFFFF; //сбросили контрольную сумму
#if DEBUG_MODE
      Serial.println F("Settings button reset!");
#endif
    }
    else {
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
void powerDown(void) {
#if DEBUG_MODE
  Serial.println F("Power down...");
#endif

  delay(100); //ждем окончания передачи

  ui.stop(); //остановить ui
  udp.stop(); //остановить udp

  WiFi.disconnect(); //отключаемся от сети
  WiFi.mode(WIFI_OFF); //отключаем wifi

  ESP.deepSleep(60e6 * sleepTime[settings.period]); //уходим в сон
}
//--------------------------------------------------------------------
void lowBattery(void) {
#if STATUS_LED > 0
  for (uint8_t i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN, (boolean)(i & 0x01));
    delay(200);
  }
  digitalWrite(LED_BUILTIN, HIGH);
#endif
  ESP.deepSleep(0); //уходим в сон
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
      memory.updateNow(); //обновить данные в памяти
#if DEBUG_MODE
      Serial.println F("Sensor found, update memory...");
#endif
    }
    else {
#if DEBUG_MODE
      Serial.print F("Sensor not found");
#endif
      if (settingsMode == true) {
        memory.update(); //обновить данные в памяти
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

  efuseGetDefaultMacAddress(buffSendData); //получить mac адрес

  buffSendData[6] = (settingsMode == true) ? UDP_FOUND_CMD : UDP_WRITE_CMD;

  buffSendData[7] = (uint8_t)sens.temp;
  buffSendData[8] = (uint8_t)(sens.temp >> 8);

  buffSendData[9] = (uint8_t)sens.press;
  buffSendData[10] = (uint8_t)(sens.press >> 8);

  buffSendData[11] = (uint8_t)sens.hum;

  buffSendData[12] = (uint8_t)getBatteryCharge();
  buffSendData[13] = (uint8_t)getWiFiSignal();
  buffSendData[14] = (uint8_t)sleepTime[settings.period];

  uint8_t crc = 0;
  for (uint8_t i = 0; i < (UDP_SEND_SIZE - 1); i++) checkCRC(&crc, buffSendData[i]);

  buffSendData[15] = (uint8_t)crc;

  sensorReady = true; //установили флаг готовности замера

#if DEBUG_MODE
  Serial.println F("Sensor check end...");
#endif
}
//--------------------------------------------------------------------
void sendUpdate(void) {
  if (wifiGetConnectStatus() && (sendReady == true) && (sensorReady == true)) {
    if (sendHostNum < (MAX_CLOCK * 2)) {
      if ((settings.send[sendHostNum][0] != '\0') || !sendHostNum) {
#if DEBUG_MODE
        Serial.print F("Send data to [ ");
        Serial.print((settings.send[sendHostNum][0] != '\0') ? settings.send[sendHostNum] : UDP_BROADCAST_ADDR);
        Serial.println F(" ]...");
#endif
        if (!udp.beginPacket((settings.send[sendHostNum][0] != '\0') ? settings.send[sendHostNum] : UDP_BROADCAST_ADDR, UDP_CLOCK_PORT) || (udp.write(buffSendData, UDP_SEND_SIZE) != UDP_SEND_SIZE) || !udp.endPacket()) {
          sendReady = false; //сбросили флаг повторной попытки отправки данных
#if DEBUG_MODE
          Serial.println F("Send data fail!");
#endif
        }
        else {
          sendHostNum += 2;
#if DEBUG_MODE
          Serial.println F("Send data ok...");
#endif
        }
      }
      else {
        sendHostNum = (MAX_CLOCK * 2);
#if DEBUG_MODE
        Serial.println F("Send all data completed...");
#endif
      }
    }
    else if (settingsMode == false) powerDown(); //отключить питание
  }
}
//--------------------------------------------------------------------
void timeUpdate(void) {
  static uint8_t updateTimer = 0;
  static uint32_t secondsTimer = millis();

  if ((millis() - secondsTimer) >= 1000) {
    secondsTimer = millis();

    if (updateTimer < 255) updateTimer++; //прибавили таймер секунд

    if (updateTimer == 2) checkSensors(); //проверка доступности сенсоров
    else if (updateTimer == 3) updateSensors(); //обновить показания сенсоров
    else if ((updateTimer > 15) && (settingsMode == true) && ui.online()) updateTimer = 15; //сбросить таймер
    else if (updateTimer > 30) powerDown(); //отключить питание

    sendReady = true; //установили флаг повторной попытки отправки данных

    resetSettingsButton(); //сбросить нажатия кнопки настроек

#if STATUS_LED > 0
    if (settingsMode == true) {
      if (!wifiGetConnectStatus() && wifiGetConnectWaitStatus()) digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); //мигаем индикацией
    }
#endif
  }
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
  vccVoltage = ESP.getVcc();
  if (vccVoltage > BAT_VOLTAGE_CORRECT) vccVoltage -= BAT_VOLTAGE_CORRECT;
  if (!getBatteryCharge()) lowBattery();

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
    if ((getBatteryCharge() >= 50) && (ESP.getFreeSketchSpace() < ESP.getSketchSize())) {
      otaUpdate = false; //выключаем обновление
#if DEBUG_MODE
      Serial.println F("OTA update disable, running out of memory");
#endif
    }
#if DEBUG_MODE
    else Serial.println F("OTA update enable");
#endif
  }

  //сбрасываем настройки группового управления
  for (uint8_t i = 0; i < (MAX_CLOCK * 2); i++) settings.send[i][0] = '\0';

  //устанавливаем период по умолчанию
  settings.period = SEND_DATA_PERIOD;

  //восстанавливаем настройки сети
  strncpy(settings.ssid, WiFi.SSID().c_str(), 64);
  settings.ssid[63] = '\0';
  strncpy(settings.pass, WiFi.psk().c_str(), 64);
  settings.pass[63] = '\0';

  //читаем настройки из памяти
  EEPROM.begin(memory.blockSize());
  memory.begin(0, 0xAB);

  //подключаем конструктор и запускаем веб интерфейс
  if (settingsMode == true) {
    ui.attachBuild(build);
    ui.attach(action);
    ui.start();

    //настраиваем обновление без пароля
    if (otaUpdate) {
      ui.enableOTA();
      ui.OTA.attachUpdateBuild(buildUpdater);
    }
    ui.downloadAuto(true);
    ui.uploadAuto(false);
  }

  //включить питание wifi
  WiFi.forceSleepWake();

  //настраиваем wifi
  if (WiFi.getAutoConnect() != false) WiFi.setAutoConnect(false);
  if (WiFi.getAutoReconnect() != true) WiFi.setAutoReconnect(true);
  if (settingsMode == true) wifiStartAP();
  else WiFi.mode(WIFI_STA);
}
//--------------------------------------------------------------------
void loop() {
  wifiUpdate(); //обработка статусов wifi
  timeUpdate(); //обработка времени
  sendUpdate(); //обработка канала связи

  if (settingsMode == true) {
    ui.tick(); //обработка веб интерфейса
    memory.tick(); //обработка еепром
  }
}
