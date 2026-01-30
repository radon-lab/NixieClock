/*
  Arduino IDE 1.8.13 версия прошивки 1.2.9 релиз от 30.01.25
  Специльно для проекта "Часы на ГРИ. Альтернативная прошивка"
  Страница проекта на форуме - https://community.alexgyver.ru/threads/chasy-na-gri-alternativnaja-proshivka.5843/

  Исходник - https://github.com/radon-lab/NixieClock
  Автор Radon-lab & Psyx86.

  Перевод веб интерфейса на Английский язык от AlexPunk

  Если не установлено ядро ESP8266, "Файл -> Настройки -> Дополнительные ссылки для Менеджера плат", в окно ввода вставляете ссылку - https://arduino.esp8266.com/stable/package_esp8266com_index.json
  Далее "Инструменты -> Плата -> Менеджер плат..." находите плату esp8266 и устанавливаете версию 2.7.4!

  В "Инструменты -> Flash Size" необходимо выбрать распределение памяти в зависимости от установленного объёма FLASH:
  1МБ - FS:none OTA:~502KB(только обновление esp по OTA).
  1МБ - FS:512KB OTA:~246KB(только локальные файлы FS и обновление прошивки часов).
  2МБ - FS:1MB OTA:~512KB(обновление esp по OTA, обновление прошивки часов и локальные файлы FS).
  4МБ - FS:2MB OTA:~1019KB(обновление esp по OTA, обновление прошивки часов и локальные файлы FS).
  8МБ - FS:6MB OTA:~1019KB(обновление esp по OTA, обновление прошивки часов и локальные файлы FS).

  Папку с плагином "ESP8266LittleFS" необходимо поместить в .../Program Files/Arduino/tools, затем нужно перезапустить Arduino IDE(если была запущена).
  Сначала загружаете прошивку, затем "Инструменты -> ESP8266 LittleFS Data Upload".

  Подключение к часам производится по шине I2C, так-же необходимо отключить внутреннюю подтяжку шины в часах, а внешнюю подтяжку подключить к 3.3в(подтяжка на шине должна быть строго только в одном месте!)
  Питать модуль ESP8266 от вывода 3v3 ардуино нельзя! Нужно использовать линейный стабилизатор или DC-DC преобразователь на 3.3в!
*/
#include "config.h"
#include "languages.h"

#define GP_NO_DNS
#define GP_NO_MDNS
#define GP_NO_PRESS

#include <LittleFS.h>
#include "web/src/GyverPortalMod.h"
GyverPortalMod ui(&LittleFS);

#include "MEMORY.h"

//переменные
GPdate mainDate; //основная дата
GPtime mainTime; //основное время

boolean clockUpdate = false; //флаг запрета обновления часов
boolean otaUpdate = false; //флаг запрета обновления есп
boolean fsUpdate = false; //флаг запрета обновления фс

boolean climateLocal = false; //флаг локальных скриптов графика
boolean alarmSvgImage = false; //флаг локальных изображений будильника
boolean timerSvgImage = false; //флаг локальных изображений таймера/секундомера
boolean radioSvgImage = false; //флаг локальных изображений радиоприемника

uint8_t passState = 0; //флаг состояния ввода пароля
uint32_t passTimer = 0; //таймер ожидания пароля

uint32_t secondsTimer = 0; //таймер счета секундных интервалов

int8_t syncState = -1; //флаг состояния синхронизации времени
uint8_t syncTimer = 0; //таймер запроса времени с ntp сервера

uint8_t timeState = 0; //флаг состояния актуальности времени

int8_t playbackTimer = -1; //таймер остановки воспроизведения
uint8_t waitTimer = 0; //таймер ожидания опроса шины

uint8_t sensorChart = 0; //буфер обновления источника данных для микроклимата
uint8_t sensorTimer = 0; //таймер обновления микроклимата

uint8_t navMainTab = 0; //флаг открытой страницы основных настроек
uint8_t navInfoTab = 0; //флаг открытой страницы информации об устройстве

uint32_t sysCycleCount = 0; //счетчик циклов процессора

#if (LED_BUILTIN == TWI_SDA_PIN) || (LED_BUILTIN == TWI_SCL_PIN)
#undef STATUS_LED
#define STATUS_LED -1
#endif

#include "NTP.h"
#include "WIRE.h"
#include "UPDATER.h"
#include "CLOCKBUS.h"
#include "WIRELESS.h"

#include "GROUP.h"
#include "WEATHER.h"

#include "WIFI.h"

const char ledColorList[] = {"#FE0100,#E11E00,#C33C00,#A55A00,#877800,#699600,#4BB400,#2DD200,#0FF000,#00F00F,#00D22D,#00B44B,#009669,#007887,#005AA5,#003CC3,#001EE1,#0000FF,#1E00E1,#3C00C3,#5A00A5,#780087,#960069,#B4004B,#D2002D,#F0000F,#FE9A3B,#FECFA9,#FEFEFE"};

const char *climateNamesMain[] = {LANG_CLIMATE_TEMP, LANG_CLIMATE_HUM};
const char *climateNamesExt[] = {LANG_CLIMATE_PRESS};

const char *climateFsData[] = {"/gp_data/PLOT_STOCK.js.gz"};
const char *alarmFsData[] = {"/alarm_add.svg", "/alarm_set.svg", "/alarm_dis.svg"};
const char *timerFsData[] = {"/timer_play.svg", "/timer_stop.svg", "/timer_pause.svg", "/timer_up.svg", "/timer_down.svg"};
const char *radioFsData[] = {"/radio_backward.svg", "/radio_left.svg", "/radio_right.svg", "/radio_forward.svg", "/radio_mode.svg", "/radio_power.svg"};

const char *alarmModeList[] = {LANG_ALARM_MODE_1, LANG_ALARM_MODE_2, LANG_ALARM_MODE_3, LANG_ALARM_MODE_4};
const char *alarmDaysList[] = {LANG_ALARM_DAYS_1, LANG_ALARM_DAYS_2, LANG_ALARM_DAYS_3, LANG_ALARM_DAYS_4, LANG_ALARM_DAYS_5, LANG_ALARM_DAYS_6, LANG_ALARM_DAYS_7};
const char *statusTimerList[] = {LANG_TIMER_OFF, LANG_TIMER_MODE_1, LANG_TIMER_MODE_2, LANG_TIMER_ERROR};

const char *failureDataList[] = {
  LANG_FAIL_DATA_1, LANG_FAIL_DATA_2, LANG_FAIL_DATA_3, LANG_FAIL_DATA_4, LANG_FAIL_DATA_5, LANG_FAIL_DATA_6,
  LANG_FAIL_DATA_7, LANG_FAIL_DATA_8, LANG_FAIL_DATA_9, LANG_FAIL_DATA_10, LANG_FAIL_DATA_11, LANG_FAIL_DATA_12, LANG_FAIL_DATA_13
};

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
String encodeTime(GPtime data) {
  String str;
  str.reserve(15);

  if (mainSettings.timeFormat) {
    if (data.hour > 12) data.hour -= 12;
    else if (!data.hour) data.hour = 12;
  }

  str = data.hour / 10;
  str += data.hour % 10;
  str += ':';
  str += data.minute / 10;
  str += data.minute % 10;
  str += ':';
  str += data.second / 10;
  str += data.second % 10;

  return str;
}
//--------------------------------------------------------------------
String getClockFirmwareVersion(void) {
  String str;
  str.reserve(10);
  str = deviceInformation[FIRMWARE_VERSION_1];
  str += '.';
  str += deviceInformation[FIRMWARE_VERSION_2];
  str += '.';
  str += deviceInformation[FIRMWARE_VERSION_3];
  return str;
}
//----------------------------Получить состояние таймера---------------------------------
String getTimerState(void) { //получить состояние таймера
  String str;
  str.reserve(50);

  str = statusTimerList[timer.mode & 0x03];
  if (((timer.mode & 0x03) == 2) && !timer.count) str += LANG_TIMER_STATUS_ALARM;
  else if (timer.mode & 0x80) str += LANG_TIMER_STATUS_PAUSE;

  return str;
}
//------------------------Преобразовать время в формат ЧЧ:ММ:СС--------------------------
String convertTimerTime(void) { //преобразовать время в формат ЧЧ:ММ:СС
  String str;
  str.reserve(15);
  str = "";

  uint8_t buff = 0;
  if (timer.mode) buff = timer.count / 3600;
  else buff = timer.hour;
  if (buff < 10) str += '0';
  str += buff;
  str += ':';

  if (timer.mode) buff = (timer.count / 60) % 60;
  else buff = timer.mins;
  if (buff < 10) str += '0';
  str += buff;
  str += ':';

  if (timer.mode) buff = timer.count % 60;
  else buff = timer.secs;
  if (buff < 10) str += '0';
  str += buff;

  return str;
}
//--------------------------------------------------------------------
String backlModeList(void) { //список режимов подсветки
  String str;
  str.reserve(500);
  if (deviceInformation[BACKL_TYPE]) {
    str = F(LANG_BACKL_MODE_1);
    if (deviceInformation[BACKL_TYPE] >= 3) {
      str += F(LANG_BACKL_MODE_2);
    }
  }
  else {
    str = F(LANG_BACKL_DISABLE);
  }
  return str;
}
//--------------------------------------------------------------------
String dotModeList(boolean alm) { //список режимов основных разделительных точек
  String str;
  str.reserve(500);
  str = F(LANG_DOTS_MODE_1);
  if (deviceInformation[NEON_DOT] < 3) {
    str += F(LANG_DOTS_MODE_2);
  }
  if (deviceInformation[NEON_DOT] == 2) {
    str += F(LANG_DOTS_MODE_3);
  }
  if (deviceInformation[NEON_DOT] == 4) {
    str += F(LANG_DOTS_MODE_4);
  }
  if (deviceInformation[DOTS_PORT_ENABLE]) {
    str += F(LANG_DOTS_MODE_5);
    if ((deviceInformation[DOTS_NUM] > 4) || (deviceInformation[DOTS_TYPE] == 2)) {
      str += F(LANG_DOTS_MODE_6);
    }
    if ((deviceInformation[DOTS_NUM] > 4) && (deviceInformation[DOTS_TYPE] == 2)) {
      str += F(LANG_DOTS_MODE_7);
    }
  }
  if (alm) {
    str += F(LANG_DOTS_MODE_8);
  }
  return str;
}
//--------------------------------------------------------------------
String neonDotModeList(void) { //список режимов неоновых разделительных точек
  String str;
  str.reserve(400);
  if (deviceInformation[NEON_DOT] < 3) {
    if (!deviceInformation[DOTS_PORT_ENABLE]) {
      str = F(LANG_INDI_DOTS_MODE_1);
    }
    else {
      str = F(LANG_INDI_DOTS_MODE_2);
      if (deviceInformation[NEON_DOT] == 2) {
        str += F(LANG_INDI_DOTS_MODE_3);
      }
    }
  }
  else {
    str = F(LANG_INDI_DOTS_DISABLE);
  }
  return str;
}
//--------------------------------------------------------------------
String flipModeList(boolean set) { //список режимов смены минут
  String str;
  str.reserve(370);
  str = (set) ? F(LANG_FLIP_MODE_1) : F(LANG_FLIP_MODE_2);
  str += F(LANG_FLIP_MODE_3);
  return str;
}
//--------------------------------------------------------------------
String secsModeList(void) { //список режимов смены секунд
  String str;
  str.reserve(200);
  if (deviceInformation[LAMP_NUM] < 6) {
    str = F(LANG_SECS_DISABLE);
  }
  else {
    str = F(LANG_SECS_MODE_1);
  }
  return str;
}
//--------------------------------------------------------------------
String playerVoiceList(void) { //список голосов для озвучки
  String str;
  str.reserve(100);
  if (deviceInformation[PLAYER_TYPE]) {
    str = F(LANG_PLAYER_VOICE_MAIN);
    for (uint8_t i = 2; i < deviceInformation[PLAYER_MAX_VOICE]; i++) {
      str += F(LANG_PLAYER_VOICE_OTHER);
      str += i;
    }
  }
  else {
    str = F(LANG_PLAYER_DISABLE);
  }
  return str;
}
//--------------------------------------------------------------------
void PAGE_TITLE_NAME(const String& title) {
  String str;
  str.reserve(70);
  str = "";
  if (settings.namePrefix) {
    str += settings.nameDevice;
    str += F(" - ");
  }
  str += title;
  if (settings.namePostfix) {
    str += settings.nameDevice;
    str += F(" - ");
  }
  GP.PAGE_TITLE(str);
}
//--------------------------------------------------------------------
void PAGE_ALERT_BLOCK(const String& id, const String& title, const String& desc, const String& sign = "", boolean rl = false, boolean al = false) {
  GP.POPUP_BEGIN(id, "370px");
  GP.BLOCK_BEGIN(GP_THIN, "", title, UI_BLOCK_COLOR);
  GP.SPAN(desc, GP_LEFT, "", UI_LABEL_COLOR);
  if (sign.length()) GP.SPAN(sign, GP_LEFT, id + "Text", GP_RED, 13);
  GP.BOX_BEGIN(GP_RIGHT);
  if (!al) {
    GP.BUTTON_MICRO(id + "Ok", LANG_ALERT_YES, "", GP_GREEN, "60px", false, true);
    GP.BUTTON_MICRO(id + "Cancel", LANG_ALERT_CANCEL, "", GP_RED, "90px", false, rl);
  }
  else GP.BUTTON_MICRO(id + "Ok", LANG_ALERT_OK, "", UI_BUTTON_COLOR, "60px");
  GP.BOX_END();
  GP.BLOCK_END();
  GP.POPUP_END();

  GP.POPUP_CLOSE(id + "Cancel," + id + "Ok");
}
//--------------------------------------------------------------------
void build(void) {
  GP.BUILD_BEGIN(GP_DEFAULT_THEME);

  GP.SELECT_LIST_STYLE(UI_BLOCK_COLOR, UI_BUTTON_COLOR);

  GP.PAGE_ZOOM(90, 390);
  GP.PAGE_BLOCK_BEGIN(500);

  GP.GRID_RESPONSIVE(1100);

  if (!webShowUpdateState() && !webShowHardwareInfo() && !webShowReloadInfo()) {
    //обновления блоков
    String updateList;
    updateList.reserve(500);
    updateList = F("barTime");

    //начать меню
    GP.UI_MENU("Nixie clock", (settings.nameMenu) ? settings.nameDevice : "", UI_MENU_COLOR, UI_MENU_NAME_COLOR);
    GP.HR(UI_MENU_LINE_COLOR, 6);

    //ссылки меню
    GP.UI_LINK("/", LANG_PAGE_MENU_LINK_HOME);
    GP.UI_LINK("/settings", LANG_PAGE_MENU_LINK_SETTINGS);
    if (sensorGetValidStatus()) GP.UI_LINK("/climate", LANG_PAGE_MENU_LINK_CLIMATE);
    if (weatherGetValidStatus()) GP.UI_LINK("/weather", LANG_PAGE_MENU_LINK_WEATHER);
    if (deviceInformation[RADIO_ENABLE]) GP.UI_LINK("/radio", LANG_PAGE_MENU_LINK_RADIO);
    if (fsUpdate || otaUpdate || clockUpdate) GP.UI_LINK("/update", LANG_PAGE_MENU_LINK_UPDATE);
    GP.UI_LINK("/information", LANG_PAGE_MENU_LINK_INFO);
    if (ui.uri("/connection")) GP.UI_LINK("/connection", LANG_PAGE_MENU_LINK_NETWORK);
    else if (ui.uri("/manual")) GP.UI_LINK("/manual", LANG_PAGE_MENU_LINK_NETWORK);
    else GP.UI_LINK("/network", LANG_PAGE_MENU_LINK_NETWORK);

    //ссылки часов
    GP.UI_LINKS_BEGIN("extGroup");
    GP.HR(UI_MENU_LINE_COLOR, 6);
    GP.UI_LINKS_BLOCK();
    GP.UI_LINKS_END();

    GP.UI_LINKS_SEND("extGroup", groupGetList());

    GP.HR(UI_MENU_LINE_COLOR, 6);

    //состояние соединения
    updateList += F(",barLink");
    GP.BLOCK_SHADOW_BEGIN();
    GP.LABEL(LANG_PAGE_MENU_STATE_CLOCK, "", UI_MENU_TEXT_COLOR, 15);
    GP.LINE_LED("barLink", busGetClockStatus(), UI_MENU_CLOCK_1_COLOR, UI_MENU_CLOCK_2_COLOR);
    GP.BLOCK_SHADOW_END();

    if (wirelessGetSensorStastus()) {
      updateList += F(",barSens");
      GP.BLOCK_SHADOW_BEGIN();
      GP.LABEL(LANG_PAGE_MENU_STATE_SENS, "", UI_MENU_TEXT_COLOR, 15);
      GP.LINE_LED("barSens", (wirelessGetOnlineStastus()), UI_MENU_SENS_1_COLOR, UI_MENU_SENS_2_COLOR);
      GP.BLOCK_SHADOW_END();
    }
    if (!deviceInformation[DS3231_ENABLE] && rtcGetFoundStatus()) {
      updateList += F(",barRtc");
      GP.BLOCK_SHADOW_BEGIN();
      GP.LABEL(LANG_PAGE_MENU_STATE_RTC, "", UI_MENU_TEXT_COLOR, 15);
      GP.LINE_LED("barRtc", rtcGetNormalStatus(), UI_MENU_RTC_1_COLOR, UI_MENU_RTC_2_COLOR);
      GP.BLOCK_SHADOW_END();
    }
    if (ntpGetRunStatus()) {
      updateList += F(",barNtp");
      GP.BLOCK_SHADOW_BEGIN();
      GP.LABEL(LANG_PAGE_MENU_STATE_NTP, "", UI_MENU_TEXT_COLOR, 15);
      GP.LINE_LED("barNtp", (ntpGetSyncStatus()), UI_MENU_NTP_1_COLOR, UI_MENU_NTP_2_COLOR);
      GP.BLOCK_SHADOW_END();
    }
    if (wifiGetConnectStatus()) {
      updateList += F(",barWifi");
      GP.BLOCK_SHADOW_BEGIN();
      GP.LABEL(LANG_PAGE_MENU_STATE_WIFI, "", UI_MENU_TEXT_COLOR, 15);
      GP.LINE_BAR("barWifi", wifiGetSignalStrength(), 0, 100, UI_MENU_WIFI_COLOR);
      GP.BLOCK_SHADOW_END();
    }

    GP.FOOTER_BEGIN();
    GP.HR(UI_MENU_LINE_COLOR, 0);
    GP.TEXT_LINK("https://github.com/radon-lab/", "@radon_lab", "user", "#bbb");
    GP.BREAK();
    GP.TEXT_LINK("https://community.alexgyver.ru/threads/chasy-na-gri-alternativnaja-proshivka.5843/", LANG_PAGE_MENU_FOOTER_FORUM, "forum", "#e67b09");
    GP.FOOTER_END();

    GP.UI_BODY(1000, UI_LOAD_COLOR); //начать основное окно

    GP.BOX_BEGIN(GP_JUSTIFY, "100%;width:auto;padding-left:2%;padding-right:2%");
    GP.LABEL_BLOCK(encodeTime(mainTime), "barTime", UI_BAR_CLOCK_COLOR, 18, 1);

    GP.BOX_BEGIN(GP_RIGHT, "100%");
    if (climateGetBarTemp() != 0x7FFF) {
      updateList += F(",barTemp");
      GP.LABEL_BLOCK(climateGetBarTempStr(), "barTemp", UI_BAR_TEMP_COLOR, 18, 1);
      if (climateGetBarHum()) {
        updateList += F(",barHum");
        GP.LABEL_BLOCK(climateGetBarHumStr(), "barHum", UI_BAR_HUM_COLOR, 18, 1);
      }
      if (climateGetBarPress()) {
        updateList += F(",barPress");
        GP.LABEL_BLOCK(climateGetBarPressStr(), "barPress", UI_BAR_PRESS_COLOR, 18, 1);
      }
    }
    else {
      GP.LABEL_BLOCK("-.-°С", "barTemp", UI_BAR_TEMP_COLOR, 18, 1);
    }
    GP.BOX_END();

    GP.BOX_END();
    GP.HR(UI_BAR_LINE_COLOR);

    if (playbackTimer > -1) playbackTimer = 0; //сбросили воспроизведение

    if (ui.uri("/")) { //основная страница
      if (!alarm.set || !deviceInformation[ALARM_TYPE]) { //если не режим настройки будильника
        PAGE_TITLE_NAME(LANG_PAGE_HOME_TITLE);
        GP.GRID_BEGIN();
        GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_HOME_BLOCK_TIME, UI_BLOCK_COLOR);
        M_BOX(
          GP.LABEL(LANG_PAGE_HOME_GUI_TIME, "", UI_LABEL_COLOR);
          M_BOX(GP_RIGHT,
                GP.NUMBER_C("setsTimeH", LANG_PAGE_HOME_GUI_HOUR, -1, 0, 23, "56px", "setsTimeM");
                GP.LABEL_M(":", -1, 9);
                GP.NUMBER_C("setsTimeM", LANG_PAGE_HOME_GUI_MIN, -1, 0, 59, "56px", "setsTimeS");
                GP.LABEL_M(":", -1, 9);
                GP.NUMBER_C("setsTimeS", LANG_PAGE_HOME_GUI_SEC, -1, 0, 59, "56px"););
        );
        M_BOX(
          GP.LABEL(LANG_PAGE_HOME_GUI_DATE, "", UI_LABEL_COLOR);
          M_BOX(GP_RIGHT,
                GP.NUMBER_C("setsDateD", LANG_PAGE_HOME_GUI_DAY, -1, 1, 31, "56px", "setsDateM");
                GP.LABEL_M(".", 0, 7);
                GP.NUMBER_C("setsDateM", LANG_PAGE_HOME_GUI_MOUN, -1, 1, 12, "56px", "setsDateY");
                GP.LABEL_M(".", 0, 7);
                GP.NUMBER_C("setsDateY", LANG_PAGE_HOME_GUI_YEAR, -1, 0, 99, "56px"););
        );
        M_BOX(
          GP.LABEL(LANG_PAGE_HOME_GUI_FORMAT, "", UI_LABEL_COLOR);
          M_BOX(GP_RIGHT,
                GP.LABEL(LANG_PAGE_HOME_GUI_FORMAT_1, "", UI_LABEL_COLOR);
                GP.SWITCH("setsTimeFormat", mainSettings.timeFormat, UI_SWITCH_COLOR);
                GP.LABEL(LANG_PAGE_HOME_GUI_FORMAT_2, "", UI_LABEL_COLOR);
               );
        );
        GP.HR(UI_LINE_COLOR);
        M_BOX(GP.LABEL(LANG_PAGE_HOME_GUI_ZONE, "", UI_LABEL_COLOR); GP.SELECT_LIST("syncGmt", LANG_PAGE_HOME_GUI_GMT, settings.ntpGMT + 12, 0););
        M_BOX(GP.LABEL(LANG_PAGE_HOME_GUI_SYNC, "", UI_LABEL_COLOR); GP.SWITCH("syncAuto", settings.ntpSync, UI_SWITCH_COLOR, !ntpGetRunStatus()););
        M_BOX(GP.LABEL(LANG_PAGE_HOME_GUI_SUMMER, "", UI_LABEL_COLOR); GP.SWITCH("syncDst", settings.ntpDst, UI_SWITCH_COLOR, !ntpGetRunStatus()););
        GP.HR(UI_LINE_COLOR);
        M_BOX(GP_CENTER, "100%;height:96px", GP.BUTTON("syncTime", (ntpGetStatus() != NTP_SYNCED) ? LANG_PAGE_HOME_GUI_TIME_DEVICE : LANG_PAGE_HOME_GUI_TIME_SERVER, "", UI_BUTTON_COLOR, "90%"););
        GP.BLOCK_END();

        GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_HOME_BLOCK_EFFECTS, UI_BLOCK_COLOR);
        M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_GLITCH, "", UI_LABEL_COLOR); GP.SWITCH("setsGlitch", mainSettings.glitchMode, UI_SWITCH_COLOR););
        M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_DOTS, "", UI_LABEL_COLOR); GP.SELECT_LIST("setsDot", dotModeList(false), fastSettings.dotMode););
        M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_FLIP, "", UI_LABEL_COLOR); GP.SELECT_LIST("setsFlip", flipModeList(false), fastSettings.flipMode););
        M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_SECS, "", UI_LABEL_COLOR); GP.SELECT_LIST("setsSecsFlip", secsModeList(), fastSettings.secsMode, 0, (boolean)(deviceInformation[LAMP_NUM] < 6)););
        GP.HR(UI_LINE_COLOR);
        M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_BACKL, "", UI_LABEL_COLOR); GP.SELECT_LIST("setsBackl", backlModeList(), fastSettings.backlMode, 0, (boolean)!deviceInformation[BACKL_TYPE]););
        M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_COLOR, "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, "330px", GP.SLIDER_COLOR_C("setsColor", ledColorList, (fastSettings.backlColor < 253) ? (fastSettings.backlColor / 10) : (fastSettings.backlColor - 227), 0, 28, UI_SLIDER_COLOR, (boolean)(deviceInformation[BACKL_TYPE] != 3));););
        GP.HR(UI_LINE_COLOR);
        if (!deviceInformation[PLAYER_TYPE]) {
          M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_KNOCK, "", UI_LABEL_COLOR); GP.SWITCH("setsSound", mainSettings.baseSound, UI_SWITCH_COLOR, false, "setsTick", false););
          M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_TICK, "", UI_LABEL_COLOR); GP.SWITCH("setsTick", mainSettings.baseSound == 0x02, UI_SWITCH_COLOR, false, "setsSound", true););
        }
        else {
          M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_ACTION, "", UI_LABEL_COLOR); GP.SWITCH("setsSound", mainSettings.baseSound, UI_SWITCH_COLOR););
          M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_VOLUME, "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, "330px", GP.SLIDER("setsSoundVol", LANG_PAGE_SETTINGS_GUI_MIN, LANG_PAGE_SETTINGS_GUI_MAX, mainSettings.volumeSound, 0, 15, 1, UI_SLIDER_COLOR, (boolean)!deviceInformation[PLAYER_TYPE]);););
        }
        GP.BLOCK_END();
        GP.GRID_END();
      }

      if (deviceInformation[ALARM_TYPE]) {
        if (alarm.reload >= 2) alarm.reload = 0;
        updateList += F(",extAlarm");
        GP.RELOAD("extAlarm");

        GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_ALARM_BLOCK, UI_BLOCK_COLOR);
        if (alarm.set) { //если режим настройки будильника
          PAGE_TITLE_NAME(LANG_PAGE_ALARM_TITLE);

          String alarmSoundList;
          alarmSoundList.reserve(200);
          for (uint8_t i = 0; i < deviceInformation[PLAYER_MAX_SOUND]; i++) {
            if (i) alarmSoundList += ',';
            alarmSoundList += F(LANG_PAGE_ALARM_GUI_SOUND);
            alarmSoundList += (i + 1);
          }

          String alarmRadioList;
          alarmRadioList.reserve(100);
          if (deviceInformation[RADIO_ENABLE]) {
            for (uint8_t i = 0; i < 10; i++) {
              if (i) alarmRadioList += ',';
              alarmRadioList += F(LANG_PAGE_ALARM_GUI_CHANNEL);
              alarmRadioList += i;
              alarmRadioList += ' ';
              alarmRadioList += String(radioSettings.stationsSave[i] / 10.0, 1);
            }
          }
          else alarmRadioList += F(LANG_PAGE_ALARM_GUI_CHANNEL_NULL);

          GP.SEND(F("<style>#alarmMode{width:100%}</style>\n"));

          GP.BREAK();
          M_BOX(GP_CENTER,
                M_BOX(GP_RIGHT,
                      GP.NUMBER_C("alarmTimeH", LANG_PAGE_HOME_GUI_HOUR, alarm_data[alarm.now][ALARM_DATA_HOUR], 0, 23, "92px", "alarmTimeM");
                      GP.LABEL_M(":", -1, 9);
                      GP.NUMBER_C("alarmTimeM", LANG_PAGE_HOME_GUI_MIN, alarm_data[alarm.now][ALARM_DATA_MINS], 0, 59, "92px");
                     );
                M_BOX(GP_LEFT, GP.SELECT_LIST("alarmMode", LANG_PAGE_ALARM_GUI_MODE, alarm_data[alarm.now][ALARM_DATA_MODE]););
               );

          GP.HR_TEXT(LANG_PAGE_ALARM_GUI_DAYS, UI_LINE_COLOR, UI_HINT_COLOR, "", GP_CENTER);
          GP.BLOCK_BEGIN(GP_DIV_RAW, "430px;margin:10px 0px");
          GP.TABLE_BORDER(false);
          GP.TABLE_BEGIN("50px,50px,50px,50px,50px,50px,50px");
          GP.TR(GP_CENTER);
          GP.TD(GP_CENTER);
          GP.LABEL_BLOCK_W(LANG_PAGE_ALARM_GUI_DAYS_1, "", UI_ALARM_WEEK_1_COLOR, 0);
          GP.TD(GP_CENTER);
          GP.LABEL_BLOCK_W(LANG_PAGE_ALARM_GUI_DAYS_2, "", UI_ALARM_WEEK_1_COLOR, 0);
          GP.TD(GP_CENTER);
          GP.LABEL_BLOCK_W(LANG_PAGE_ALARM_GUI_DAYS_3, "", UI_ALARM_WEEK_1_COLOR, 0);
          GP.TD(GP_CENTER);
          GP.LABEL_BLOCK_W(LANG_PAGE_ALARM_GUI_DAYS_4, "", UI_ALARM_WEEK_1_COLOR, 0);
          GP.TD(GP_CENTER);
          GP.LABEL_BLOCK_W(LANG_PAGE_ALARM_GUI_DAYS_5, "", UI_ALARM_WEEK_1_COLOR, 0);
          GP.TD(GP_CENTER);
          GP.LABEL_BLOCK_W(LANG_PAGE_ALARM_GUI_DAYS_6, "", UI_ALARM_WEEK_2_COLOR, 0);
          GP.TD(GP_CENTER);
          GP.LABEL_BLOCK_W(LANG_PAGE_ALARM_GUI_DAYS_7, "", UI_ALARM_WEEK_2_COLOR, 0);

          uint8_t alarmDays = alarm_data[alarm.now][ALARM_DATA_DAYS];
          GP.TR(GP_CENTER);
          String alarmDaysId;
          alarmDaysId.reserve(15);
          for (uint8_t i = 1; i < 8; i++) {
            GP.TD(GP_CENTER);
            alarmDays >>= 1;
            alarmDaysId = F("alarmDay/");
            alarmDaysId += i;
            GP.CHECK(alarmDaysId, (boolean)(alarmDays & 0x01));
            updateList += ',';
            updateList += alarmDaysId;
          }
          GP.TABLE_END();
          GP.BLOCK_END();

          GP.HR_TEXT(LANG_PAGE_ALARM_GUI_HR_SOUND, UI_LINE_COLOR, UI_HINT_COLOR, "", GP_CENTER);
          GP.BLOCK_OFFSET_BEGIN();
          M_BOX(GP_CENTER, GP.SELECT_LIST("alarmSoundType", (deviceInformation[RADIO_ENABLE]) ? LANG_PAGE_ALARM_GUI_SOUND_TYPE_2 : LANG_PAGE_ALARM_GUI_SOUND_TYPE_1, (boolean)alarm_data[alarm.now][ALARM_DATA_RADIO], 0, (boolean)!deviceInformation[RADIO_ENABLE], true););
          M_BOX(GP_CENTER,
                GP.SELECT_LIST("alarmSound", alarmSoundList, alarm_data[alarm.now][ALARM_DATA_SOUND], 0, (boolean)(deviceInformation[RADIO_ENABLE] && alarm_data[alarm.now][ALARM_DATA_RADIO]));
                GP.SELECT_LIST("alarmRadio", alarmRadioList, alarm_data[alarm.now][ALARM_DATA_STATION], 0, (boolean)(!deviceInformation[RADIO_ENABLE] || !alarm_data[alarm.now][ALARM_DATA_RADIO]));
               );
          M_BOX(GP_CENTER, GP.SLIDER_MAX(LANG_PAGE_ALARM_GUI_SOUND_VOLUME, LANG_PAGE_ALARM_GUI_SOUND_VOL_AUTO, LANG_PAGE_ALARM_GUI_SOUND_VOL_MAX, "alarmVol", alarm_data[alarm.now][ALARM_DATA_VOLUME], 0, 15, 1, UI_SLIDER_COLOR, (boolean)((!deviceInformation[RADIO_ENABLE] || !alarm_data[alarm.now][ALARM_DATA_RADIO]) && !deviceInformation[PLAYER_TYPE])););
          GP.BLOCK_END();

          GP.HR(UI_LINE_COLOR);
          M_BOX(GP_CENTER,
                GP.BUTTON_MINI("alarmBack", (alarm.set == 1) ? LANG_PAGE_ALARM_GUI_BACK : LANG_PAGE_ALARM_GUI_ADD, "", UI_ALARM_BACK_COLOR, "210px", false, true);
                GP.BUTTON_MINI("alarmDel", (alarm.all > 1) ? ((alarm.set == 1) ? LANG_PAGE_ALARM_GUI_DELETE : LANG_PAGE_ALARM_GUI_CANCEL) : LANG_PAGE_ALARM_GUI_DISABLE, "", (alarm.all > 1) ? UI_ALARM_DEL_COLOR : UI_ALARM_DIS_COLOR, "210px", false, (boolean)(alarm.all <= 1));
               );
        }
        else { //иначе режим отображения
          String reloadList;
          reloadList.reserve(100);

          String lableList;
          lableList.reserve(15);

          for (uint8_t i = 0; i < alarm.all; i++) {
            uint8_t alarmHour = alarm_data[i][ALARM_DATA_HOUR];
            uint8_t alarmMins = alarm_data[i][ALARM_DATA_MINS];

            String alarmTime;
            alarmTime += alarmHour;
            alarmTime += ':';
            if (alarmMins < 10) alarmTime += '0';
            alarmTime += alarmMins;

            uint8_t alarmMode = alarm_data[i][ALARM_DATA_MODE];

            uint16_t alarmTimeNow = (mainTime.hour * 60) + mainTime.minute;
            uint16_t alarmTimeNext = (alarmHour * 60) + alarmMins;

            uint8_t alarmDays = (alarmMode == 4) ? alarm_data[i][ALARM_DATA_DAYS] : ((alarmMode == 3) ? 0x3E : 0xFE);
            uint8_t nowWeekDay = getWeekDay(mainDate.year, mainDate.month, mainDate.day); //получить день недели

            for (uint8_t dl = 0; dl < 7; dl++) {
              if (alarmDays & (0x01 << nowWeekDay)) {
                if (dl || (alarmTimeNow < alarmTimeNext)) {
                  alarmTimeNext = (alarmTimeNext + (1440 * dl)) - alarmTimeNow;
                  break;
                }
              }
              if (++nowWeekDay > 7) nowWeekDay = 1;
            }

            String alarmStatus = "";

            if (alarmMode == 4) {
              for (uint8_t dw = 0; dw < 7; dw++) {
                alarmDays >>= 1;
                if (alarmDays & 0x01) {
                  if (alarmStatus.length() > 3) alarmStatus += ", ";
                  alarmStatus += alarmDaysList[dw];
                }
              }
            }

            if (alarmMode < 4) {
              alarmStatus += alarmModeList[alarmMode];
            }

            if (alarmMode) {
              alarmStatus += F(" | <wbr>");
              alarmStatus += LANG_ALARM_AFTER;
              uint8_t _time = alarmTimeNext / 1440;
              if (_time) {
                alarmStatus += _time;
                alarmStatus += LANG_ALARM_DAY;
              }
              _time = (alarmTimeNext % 1440) / 60;
              if (_time) {
                alarmStatus += _time;
                alarmStatus += LANG_ALARM_HOUR;
              }
              _time = (alarmTimeNext % 1440) % 60;
              if (_time) {
                alarmStatus += _time;
                alarmStatus += LANG_ALARM_MINS;
              }
            }

            GP.BLOCK_THIN_BOLD("90%", UI_ALARM_BLOCK_COLOR);
            lableList = F("alarmSet/");
            lableList += i;
            GP.BOX_BEGIN();
            M_BLOCK(GP_DIV_RAW, "100%;width:100%;margin-bottom:8px", M_BOX(GP_LEFT, GP.LABEL(alarmTime, "", UI_ALARM_TIME_COLOR, 40, 1);); M_BOX(GP_LEFT, GP.LABEL_W(alarmStatus, "", UI_ALARM_INFO_COLOR, 0, GP_LEFT, 14, 1, 1);););
            GP.BOX_BEGIN(GP_RIGHT, "100%;width:60px");
            if (alarmSvgImage) {
              GP.ICON_FILE_BUTTON(lableList, alarmFsData[1], 30, UI_ALARM_SET_COLOR);
            }
            else {
              GP.BUTTON_MINI(lableList, " ≡ ", "", UI_ALARM_SET_COLOR, "");
            }
            GP.BOX_END();
            GP.BOX_END();
            GP.BLOCK_END();
            if (i) reloadList += ',';
            reloadList += lableList;
          }

          if (!alarm.status) {
            if (alarm.all < MAX_ALARMS) {
              if (alarmSvgImage) {
                GP.ICON_FILE_BUTTON("alarmAdd", alarmFsData[0], 50, UI_ALARM_ADD_COLOR);
              }
              else {
                M_BOX(GP_CENTER, GP.BUTTON("alarmAdd", "✚", "", UI_ALARM_ADD_COLOR, "80px;font-size:30px"););
              }
            }
          }
          else {
            if (alarmSvgImage) {
              GP.ICON_FILE_BUTTON("alarmDis", alarmFsData[2], 50, UI_ALARM_DIS_COLOR);
            }
            else {
              M_BOX(GP_CENTER, GP.BUTTON("alarmDis", "✖", "", UI_ALARM_DIS_COLOR, "80px;font-weight:bold"););
            }
          }
          GP.BREAK();
          GP.RELOAD_CLICK(reloadList);
        }
        GP.BLOCK_END();
      }

      if (!alarm.set || !deviceInformation[ALARM_TYPE]) { //если не режим настройки будильника
        if (deviceInformation[TIMER_ENABLE] && deviceInformation[EXT_BTN_ENABLE]) {
          updateList += F(",timerTime,timerState");

          GP.BLOCK_BEGIN(GP_THIN, "", LANG_TIMER_BLOCK, UI_BLOCK_COLOR);

          GP.BLOCK_THIN_BOLD("280px", UI_TIMER_BLOCK_COLOR);
          GP.LABEL(getTimerState(), "timerState", UI_TIMER_INFO_COLOR, 0, 1);

          GP.TABLE_BEGIN("15%,15%,15%", GP_ALS(GP_CENTER, GP_CENTER, GP_CENTER), "200px");
          GP.TR();
          String btnList;
          btnList.reserve(15);
          for (uint8_t i = 0; i < 6; i++) {
            btnList = "timerHour/";
            btnList += i;
            GP.TD();
            if (timerSvgImage) {
              GP.ICON_FILE_BUTTON(btnList, (i < 3) ? timerFsData[3] : timerFsData[4], 40, UI_TIMER_SET_COLOR);
            }
            else {
              GP.BUTTON_MINI(btnList, (i < 3) ? " ▲ " : " ▼ ", "", UI_TIMER_SET_COLOR);
            }
            if (i == 2) {
              GP.TR();
              GP.TD(GP_CENTER, 3);
              M_BOX(GP_CENTER, GP.LABEL(convertTimerTime(), "timerTime", UI_TIMER_TIME_COLOR, 40, 1););
              GP.TR();
            }
          }
          GP.TABLE_END();
          GP.BLOCK_END();

          if (timerSvgImage) {
            M_BOX(GP_JUSTIFY, "254px",
                  GP.ICON_FILE_BUTTON("timerControl/0", timerFsData[0], 50, UI_TIMER_CTRL_COLOR);
                  GP.ICON_FILE_BUTTON("timerControl/1", timerFsData[1], 60, UI_TIMER_CTRL_COLOR);
                  GP.ICON_FILE_BUTTON("timerControl/2", timerFsData[2], 50, UI_TIMER_CTRL_COLOR);
                 );
          }
          else {
            M_BOX(GP_CENTER,
                  GP.BUTTON_MINI("timerControl/0", "⠀⠀►⠀⠀", "", UI_TIMER_CTRL_COLOR);
                  GP.BUTTON_MINI("timerControl/1", "⠀⠀❑⠀⠀", "", UI_TIMER_CTRL_COLOR);
                  GP.BUTTON_MINI("timerControl/2", "⠀⠀||⠀⠀", "", UI_TIMER_CTRL_COLOR);
                 );
          }
          GP.BREAK();
          GP.BLOCK_END();

          GP.UPDATE_CLICK("timerTime", "timerHour/0,timerHour/1,timerHour/2,timerHour/3,timerHour/4,timerHour/5");
          GP.UPDATE_CLICK("timerTime,timerState", "timerControl/0,timerControl/1,timerControl/2");
        }
      }
    }
    else if (ui.uri("/settings")) { //настройки
      PAGE_TITLE_NAME(LANG_PAGE_SETTINGS_TITLE);

      String showModeList; //список режимов автопоказа
      showModeList.reserve(350);
      showModeList = F(LANG_PAGE_SETTINGS_GUI_SHOW_D_MODE);
      if (deviceInformation[LAMP_NUM] < 6) showModeList += F(GP_ITEM_DISABLE);
      if (deviceInformation[SENS_TEMP]) {
        showModeList += climateGetShowDataList((climateAvailableTemp(SENS_CLOCK)) ? SENS_CLOCK : SENS_MAX_DATA);
      }
      else if (climateAvailableTemp(settings.climateSend[0])) {
        showModeList += climateGetShowDataList(settings.climateSend[0]);
      }
      else if (climateAvailableTemp(settings.climateSend[1])) {
        showModeList += climateGetShowDataList(SENS_MAX_DATA);
      }
      if (climateAvailableTemp(settings.climateSend[1])) {
        showModeList += climateGetShowDataList(settings.climateSend[1]);
      }

      String lightHint; //подсказка времени смены яркости
      lightHint.reserve(170);
      if (deviceInformation[LIGHT_SENS_ENABLE]) lightHint = F(LANG_PAGE_SETTINGS_GUI_HINT_LIGHT_2);
      else if (weatherGetValidStatus()) lightHint = F(LANG_PAGE_SETTINGS_GUI_HINT_LIGHT_3);
      else lightHint = F(LANG_PAGE_SETTINGS_GUI_HINT_LIGHT_1);

      GP.NAV_TABS_M("extMainTab", LANG_PAGE_SETTINGS_MENU, navMainTab, UI_BUTTON_COLOR);

      GP.NAV_BLOCK_BEGIN("extMainTab", 0, navMainTab);
      GP.GRID_BEGIN();
      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_SETTINGS_BLOCK_WEATHER, UI_BLOCK_COLOR);
      M_BOX(
        GP.LABEL(LANG_PAGE_SETTINGS_GUI_ON_BUTTON, "", UI_LABEL_COLOR);
        GP.SELECT_LIST("climateMainSens", (!deviceInformation[BTN_EASY_MAIN_MODE]) ? climateGetSendDataList() : LANG_PAGE_SETTINGS_GUI_SENS_DISABLE, extendedSettings.tempMainSensor, 0, (boolean)(deviceInformation[BTN_EASY_MAIN_MODE] || sensorGetDisabledStatus()));
      );
      M_BOX(
        GP.LABEL(LANG_PAGE_SETTINGS_GUI_ONCE_HOUR, "", UI_LABEL_COLOR);
        GP.SELECT_LIST("climateHourSens", (deviceInformation[PLAYER_TYPE]) ? climateGetSendDataList() : LANG_PAGE_SETTINGS_GUI_SENS_DISABLE, extendedSettings.tempHourSensor, 0, (boolean)(!deviceInformation[PLAYER_TYPE] || sensorGetDisabledStatus()));
      );
      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_SETTINGS_GUI_HR_SHOW, UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_SHOW_EBABLE, "", UI_LABEL_COLOR); GP.SWITCH("setsAutoShow", (boolean)!(mainSettings.autoShowTime & 0x80), UI_SWITCH_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_TIME_MINS, "", UI_LABEL_COLOR); GP.SPINNER("setsAutoShowTime", mainSettings.autoShowTime & 0x7F, 1, 15, 1, 0, UI_SPINNER_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_SHOW_EFFECT, "", UI_LABEL_COLOR); GP.SELECT_LIST("setsAutoShowFlip", flipModeList(true), mainSettings.autoShowFlip););
      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_SETTINGS_GUI_HR_DISPLAY, UI_LINE_COLOR, UI_HINT_COLOR, "hint4");
      GP.HINT("hint4", LANG_PAGE_SETTINGS_GUI_HINT_DISPLAY); //всплывающая подсказка
      for (uint8_t i = 0; i < 5; i++) {
        M_BOX(GP_JUSTIFY, "100%;height:60px",
              M_BOX(GP_LEFT, GP.LABEL(String(i + 1), "hint4", UI_LABEL_COLOR); GP.SELECT_LIST(String("extShowMode/") + i, showModeList, extendedSettings.autoShowModes[i]););
              GP.SPINNER(String("extShowTime/") + i, extendedSettings.autoShowTimes[i], 1, 5, 1, 0, UI_SPINNER_COLOR);
             );
      }
      GP.BLOCK_END();

      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_SETTINGS_BLOCK_INDI, UI_BLOCK_COLOR);
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_GLITCH, "", UI_LABEL_COLOR); GP.SWITCH("setsGlitch", mainSettings.glitchMode, UI_SWITCH_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_FLIP, "", UI_LABEL_COLOR); GP.SELECT_LIST("setsFlip", flipModeList(false), fastSettings.flipMode););
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_SECS, "", UI_LABEL_COLOR); GP.SELECT_LIST("setsSecsFlip", secsModeList(), fastSettings.secsMode, 0, (boolean)(deviceInformation[LAMP_NUM] < 6)););
      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_SETTINGS_GUI_HR_BRIGHT, UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL_W(LANG_PAGE_SETTINGS_GUI_DAY, "", UI_LABEL_COLOR, 52, GP_LEFT); GP.SLIDER_C("setsIndiBrtDay", LANG_PAGE_SETTINGS_GUI_MIN, LANG_PAGE_SETTINGS_GUI_MAX, mainSettings.indiBrightDay, 5, 30, 1, UI_SLIDER_COLOR););
      M_BOX(GP.LABEL_W(LANG_PAGE_SETTINGS_GUI_NIGHT, "", UI_LABEL_COLOR, 52, GP_LEFT); GP.SLIDER_C("setsIndiBrtNight", LANG_PAGE_SETTINGS_GUI_MIN, LANG_PAGE_SETTINGS_GUI_MAX, mainSettings.indiBrightNight, 5, 30, 1, UI_SLIDER_COLOR););
      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_SETTINGS_GUI_HR_TIME_BRIGHT, UI_LINE_COLOR, UI_HINT_COLOR, "hint1");
      GP.HINT("hint1", LANG_PAGE_SETTINGS_GUI_HINT_LIGHT); //всплывающая подсказка
      M_BOX(GP_CENTER,
            GP.LABEL_W(LANG_PAGE_SETTINGS_GUI_START, "", UI_LABEL_COLOR, 52); GP.SPINNER("setsTimeBrightS", mainSettings.timeBrightStart, 0, 23, 1, 0, UI_SPINNER_COLOR);
            GP.VOID_BOX("20px");
            GP.SPINNER("setsTimeBrightE", mainSettings.timeBrightEnd, 0, 23, 1, 0, UI_SPINNER_COLOR); GP.LABEL_W(LANG_PAGE_SETTINGS_GUI_END, "", UI_LABEL_COLOR, 52);
           );
      GP.HINT_BOX("hintB1", "setsTimeBrightS", "setsTimeBrightE", lightHint);
      GP.HR_TEXT(LANG_PAGE_SETTINGS_GUI_HR_SLEEP, UI_LINE_COLOR, UI_HINT_COLOR, "hint2");
      GP.HINT("hint2", LANG_PAGE_SETTINGS_GUI_HINT_SLEEP); //всплывающая подсказка
      M_BOX(GP_CENTER,
            GP.LABEL_W(LANG_PAGE_SETTINGS_GUI_DAY, "", UI_LABEL_COLOR, 52); GP.SPINNER("setsSleepD", mainSettings.timeSleepDay, 0, 90, 15, 0, UI_SPINNER_COLOR);
            GP.VOID_BOX("20px");
            GP.SPINNER("setsSleepN", mainSettings.timeSleepNight, 0, 30, 5, 0, UI_SPINNER_COLOR); GP.LABEL_W(LANG_PAGE_SETTINGS_GUI_NIGHT, "", UI_LABEL_COLOR, 52);
           );
      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_SETTINGS_GUI_HR_BURN, UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_TIME_MINS, "", UI_LABEL_COLOR); GP.SPINNER("setsBurnTime", mainSettings.burnTime, 10, 180, 5, 0, UI_SPINNER_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_METHOD, "", UI_LABEL_COLOR); GP.SELECT_LIST("setsBurnFlip", LANG_PAGE_SETTINGS_GUI_BURN_MODE, mainSettings.burnMode););
      GP.BLOCK_END();
      GP.GRID_END();

      GP.GRID_BEGIN();
      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_SETTINGS_BLOCK_BRIGHT, UI_BLOCK_COLOR);
      M_BOX(GP_JUSTIFY, "100%;height:52px", GP.LABEL(LANG_PAGE_SETTINGS_GUI_COLOR, "", UI_LABEL_COLOR); GP.SLIDER_COLOR_C("setsColor", ledColorList, (fastSettings.backlColor < 253) ? (fastSettings.backlColor / 10) : (fastSettings.backlColor - 227), 0, 28, UI_SLIDER_COLOR, (boolean)(deviceInformation[BACKL_TYPE] != 3)););
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_MODE, "", UI_LABEL_COLOR); GP.SELECT_LIST("setsBackl", backlModeList(), fastSettings.backlMode, 0, (boolean)!deviceInformation[BACKL_TYPE]););
      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_SETTINGS_GUI_HR_BRIGHT, UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL_W(LANG_PAGE_SETTINGS_GUI_DAY, "", UI_LABEL_COLOR, 52, GP_LEFT); GP.SLIDER_C("setsBacklBrightDay", LANG_PAGE_SETTINGS_GUI_MIN, LANG_PAGE_SETTINGS_GUI_MAX, mainSettings.backlBrightDay / 10, 1, 25, 1, UI_SLIDER_COLOR, (boolean)!deviceInformation[BACKL_TYPE]););
      M_BOX(GP.LABEL_W(LANG_PAGE_SETTINGS_GUI_NIGHT, "", UI_LABEL_COLOR, 52, GP_LEFT); GP.SLIDER_C("setsBacklBrightNight", LANG_PAGE_SETTINGS_GUI_DISABLE, LANG_PAGE_SETTINGS_GUI_MAX, mainSettings.backlBrightNight / 10, 0, 25, 1, UI_SLIDER_COLOR, (boolean)!deviceInformation[BACKL_TYPE]););
      GP.BLOCK_END();

      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_SETTINGS_BLOCK_DOTS, UI_BLOCK_COLOR);
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_NEON, "", UI_LABEL_COLOR); GP.SELECT_LIST("setsNeonDot", neonDotModeList(), fastSettings.neonDotMode, 0, (boolean)((deviceInformation[NEON_DOT] >= 3) || !deviceInformation[DOTS_PORT_ENABLE])););
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_MODE, "", UI_LABEL_COLOR); GP.SELECT_LIST("setsDot", dotModeList(false), fastSettings.dotMode););
      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_SETTINGS_GUI_HR_BRIGHT, UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL_W(LANG_PAGE_SETTINGS_GUI_DAY, "", UI_LABEL_COLOR, 52, GP_LEFT); GP.SLIDER_C("setsDotBrtDay", LANG_PAGE_SETTINGS_GUI_MIN, LANG_PAGE_SETTINGS_GUI_MAX, mainSettings.dotBrightDay / 10, 1, 25, 1, UI_SLIDER_COLOR, (boolean)(deviceInformation[NEON_DOT] >= 3)););
      M_BOX(GP.LABEL_W(LANG_PAGE_SETTINGS_GUI_NIGHT, "", UI_LABEL_COLOR, 52, GP_LEFT); GP.SLIDER_C("setsDotBrtNight", LANG_PAGE_SETTINGS_GUI_DISABLE, (deviceInformation[NEON_DOT] >= 3) ? LANG_PAGE_SETTINGS_GUI_ENABLE : LANG_PAGE_SETTINGS_GUI_MAX, mainSettings.dotBrightNight / ((deviceInformation[NEON_DOT] >= 3) ? 1 : 10), 0, (deviceInformation[NEON_DOT] >= 3) ? 1 : 25, 1, UI_SLIDER_COLOR););
      GP.BLOCK_END();
      GP.GRID_END();
      GP.NAV_BLOCK_END();

      GP.NAV_BLOCK_BEGIN("extMainTab", 1, navMainTab);
      GP.GRID_BEGIN();
      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_SETTINGS_BLOCK_SOUND, UI_BLOCK_COLOR);
      if (!deviceInformation[PLAYER_TYPE]) {
        M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_KNOCK, "", UI_LABEL_COLOR); GP.SWITCH("setsSound", mainSettings.baseSound, UI_SWITCH_COLOR, false, "setsTick", false););
        M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_TICK, "", UI_LABEL_COLOR); GP.SWITCH("setsTick", mainSettings.baseSound == 0x02, UI_SWITCH_COLOR, false, "setsSound", true););
      }
      else {
        M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_ACTION, "", UI_LABEL_COLOR); GP.SWITCH("setsSound", mainSettings.baseSound, UI_SWITCH_COLOR););
        M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_VOICE, "", UI_LABEL_COLOR); GP.SELECT_LIST("setsVoice", playerVoiceList(), mainSettings.voiceSound, 0, (boolean)!deviceInformation[PLAYER_TYPE]););
        M_BOX(GP_JUSTIFY, GP.LABEL(LANG_PAGE_SETTINGS_GUI_VOLUME, "", UI_LABEL_COLOR); GP.SLIDER("setsSoundVol", LANG_PAGE_SETTINGS_GUI_MIN, LANG_PAGE_SETTINGS_GUI_MAX, mainSettings.volumeSound, 0, 15, 1, UI_SLIDER_COLOR, (boolean)!deviceInformation[PLAYER_TYPE]););
      }
      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_SETTINGS_GUI_HR_HOUR, UI_LINE_COLOR, UI_HINT_COLOR, "hint3");
      GP.HINT("hint3", LANG_PAGE_SETTINGS_GUI_HINT_HOUR); //всплывающая подсказка
      M_BOX(GP_CENTER,
            GP.LABEL_W(LANG_PAGE_SETTINGS_GUI_START, "", UI_LABEL_COLOR, 52); GP.SPINNER("setsHourSoundS", mainSettings.timeHourStart, 0, 23, 1, 0, UI_SPINNER_COLOR);
            GP.VOID_BOX("20px");
            GP.SPINNER("setsHourSoundE", mainSettings.timeHourEnd, 0, 23, 1, 0, UI_SPINNER_COLOR); GP.LABEL_W(LANG_PAGE_SETTINGS_GUI_END, "", UI_LABEL_COLOR, 52);
           );
      GP.HINT_BOX("hintB2", "setsHourSoundS", "setsHourSoundE", LANG_PAGE_SETTINGS_GUI_HINT_HOUR_1);
      GP.HR_TEXT(LANG_PAGE_SETTINGS_GUI_HR_SOUND, UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_TEMP, "", UI_LABEL_COLOR); GP.SWITCH("setsHourTemp", mainSettings.hourSound & 0x80, UI_SWITCH_COLOR, (boolean)(!deviceInformation[PLAYER_TYPE] || sensorGetDisabledStatus())););
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_HOUR, "", UI_LABEL_COLOR); GP.SELECT_LIST("setsHourSound", LANG_PAGE_SETTINGS_GUI_HOUR_MODE, mainSettings.hourSound & 0x03, 0, (boolean)!deviceInformation[PLAYER_TYPE]););
      GP.BLOCK_END();

      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_SETTINGS_BLOCK_ALARM, UI_BLOCK_COLOR);
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_AUTO_DIS_MINS, "", UI_LABEL_COLOR); GP.SPINNER("extAlarmTimeout", extendedSettings.alarmTime, 1, 240, 1, 0, UI_SPINNER_COLOR, "", (boolean)!deviceInformation[ALARM_TYPE]););

      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_SETTINGS_GUI_HR_ADD, UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_REPEAT_MINS, "", UI_LABEL_COLOR); GP.SPINNER("extAlarmWaitTime", extendedSettings.alarmWaitTime, 0, 240, 1, 0, UI_SPINNER_COLOR, "", (boolean)!deviceInformation[ALARM_TYPE]););
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_SOUND_DIS_MINS, "", UI_LABEL_COLOR); GP.SPINNER("extAlarmSoundTime", extendedSettings.alarmSoundTime, 0, 240, 1, 0, UI_SPINNER_COLOR, "", (boolean)!deviceInformation[ALARM_TYPE]););

      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_SETTINGS_GUI_HR_INDI, UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_ACTIVE, "", UI_LABEL_COLOR); GP.SELECT_LIST("extAlarmDotOn", dotModeList(true), extendedSettings.alarmDotOn, 0, (boolean)!deviceInformation[ALARM_TYPE]););
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_WAIT, "", UI_LABEL_COLOR); GP.SELECT_LIST("extAlarmDotWait", dotModeList(true), extendedSettings.alarmDotWait, 0, (boolean)!deviceInformation[ALARM_TYPE]););
      GP.BLOCK_END();
      GP.GRID_END();

      GP.GRID_BEGIN();
      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_SETTINGS_BLOCK_CLIMATE, UI_BLOCK_COLOR);
      M_BOX(
        GP.LABEL(LANG_PAGE_SETTINGS_GUI_AVG, "", UI_LABEL_COLOR);
        GP.SWITCH("climateAvg", settings.climateAvg, UI_SWITCH_COLOR, (boolean)(settings.climateChart == SENS_WIRELESS));
      );
      M_BOX(
        GP.LABEL(LANG_PAGE_SETTINGS_GUI_TIME_MINS, "", UI_LABEL_COLOR);
        GP.SPINNER("climateTime", (settings.climateChart == SENS_WIRELESS) ? wirelessGetInterval() : settings.climateTime, 1, 60, 1, 0, UI_SPINNER_COLOR, "", (boolean)(settings.climateChart == SENS_WIRELESS));
      );
      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_SETTINGS_GUI_HR_DISPLAY, UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_BAR, "", UI_LABEL_COLOR); GP.SELECT_LIST("climateBar", climateGetMainDataList(SENS_CLOCK, SENS_MAX_DATA), settings.climateBar, 0, false, true););
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_CHART, "", UI_LABEL_COLOR); GP.SELECT_LIST("climateChart", climateGetMainDataList(SENS_CLOCK, SENS_WEATHER), settings.climateChart, 0););
      GP.BLOCK_END();

      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_SETTINGS_BLOCK_SENS, UI_BLOCK_COLOR);
      M_BOX(
        GP.LABEL(LANG_PAGE_SETTINGS_GUI_SENS_1, "", UI_LABEL_COLOR);
        GP.SELECT_LIST("climateSend/0", (deviceInformation[SENS_TEMP]) ? LANG_PAGE_SETTINGS_GUI_SENS_CLOCK : climateGetMainDataList(SENS_MAIN, SENS_MAX_DATA), settings.climateSend[0] - 1, 0, (boolean)(deviceInformation[SENS_TEMP]), true);
      );
      M_BOX(
        GP.LABEL(LANG_PAGE_SETTINGS_GUI_SENS_2, "", UI_LABEL_COLOR);
        GP.SELECT_LIST("climateSend/1", climateGetMainDataList(SENS_MAIN, SENS_MAX_DATA), settings.climateSend[1] - 1, 0, false, true);
      );
      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_SETTINGS_GUI_HR_CORRECT, UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_TEMP_C, "", UI_LABEL_COLOR); GP.SPINNER("setsTempCorrect", mainSettings.tempCorrect / 10.0, -12.7, 12.7, 0.1, 1, UI_SPINNER_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_SETTINGS_GUI_CORRECT, "", UI_LABEL_COLOR); GP.SELECT_LIST("climateCorrectType", LANG_PAGE_SETTINGS_GUI_SENS_NULL + climateGetSendDataList(), extendedSettings.tempCorrectSensor););
      GP.BLOCK_END();
      GP.GRID_END();
      GP.NAV_BLOCK_END();

      PAGE_ALERT_BLOCK("alertClimate", LANG_PAGE_SETTINGS_GUI_ALERT_CLIMATE_TITLE, LANG_PAGE_SETTINGS_GUI_ALERT_CLIMATE_1, "", true);
      GP.UPDATE_CLICK("alertClimate", "climateChart");
    }
    else if (ui.uri("/climate")) { //микроклимат
      PAGE_TITLE_NAME(LANG_PAGE_CLIMATE_TITLE);

      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_CLIMATE_BLOCK, UI_BLOCK_COLOR);
      if (climateDates[CLIMATE_BUFFER - 1] != 0) {
        uint16_t heightSize = 500;
        if (climateGetChartPress()) heightSize = 300;

#ifdef RUSSIAN
        GP.PLOT_STOCK_BEGIN(climateLocal, true);
#else
        GP.PLOT_STOCK_BEGIN(climateLocal);
#endif

        if (climateGetChartHum()) {
          GP.PLOT_STOCK_DARK("climateDataMain", climateNamesMain, climateDates, climateArrMain[0], climateArrMain[1], CLIMATE_BUFFER, 1, 10, heightSize, UI_BAR_TEMP_COLOR, UI_BAR_HUM_COLOR);
        }
        else {
          GP.PLOT_STOCK_DARK("climateDataMain", climateNamesMain, climateDates, climateArrMain[0], NULL, CLIMATE_BUFFER, 1, 10, heightSize, UI_BAR_TEMP_COLOR, UI_BAR_HUM_COLOR);
        }
        if (climateGetChartPress()) {
          GP.PLOT_STOCK_DARK("climateDataExt", climateNamesExt, climateDates, climateArrExt[0], NULL, CLIMATE_BUFFER, 2, 0, heightSize, UI_BAR_PRESS_COLOR);
        }
      }
      else {
        GP.BLOCK_BEGIN(GP_TAB, "93%;padding:20% 5px", "", GP_DEFAULT);
        GP.LABEL(LANG_PAGE_CLIMATE_NULL, "", GP_GRAY, 40, true, true); //описание
        GP.BLOCK_END();
      }

      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_CLIMATE_GUI_HR_CLOCK, UI_LINE_COLOR, UI_HINT_COLOR);
      if (sens.temp[SENS_CLOCK] != 0x7FFF) {
        M_BOX(GP.LABEL(LANG_PAGE_CLIMATE_GUI_SENS_DATA, "", UI_LABEL_COLOR); GP.TEXT("", "", climateGetSensDataStr(sens.temp[SENS_CLOCK], sens.press[SENS_CLOCK], sens.hum[SENS_CLOCK]), "", 0, "", true););
        M_BOX(GP.LABEL(LANG_PAGE_CLIMATE_GUI_SENS_TYPE, "", UI_LABEL_COLOR); GP.TEXT("", "", (sens.type < 6) ? climateTempSensList[sens.type] : climateGetSensList(sens.type, false), "", 0, "", true););
      }
      else {
        M_BOX(GP.LABEL(LANG_PAGE_CLIMATE_GUI_SENS_STATE, "", UI_LABEL_COLOR); GP.NUMBER("", LANG_PAGE_CLIMATE_GUI_SENS_NULL, INT32_MAX, "", true););
      }

      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_CLIMATE_GUI_HR_ESP, UI_LINE_COLOR, UI_HINT_COLOR);
      if (sens.temp[SENS_MAIN] != 0x7FFF) {
        M_BOX(GP.LABEL(LANG_PAGE_CLIMATE_GUI_SENS_DATA, "", UI_LABEL_COLOR); GP.TEXT("", "", climateGetSensDataStr(sens.temp[SENS_MAIN], sens.press[SENS_MAIN], sens.hum[SENS_MAIN]), "", 0, "", true););
        M_BOX(GP.LABEL(LANG_PAGE_CLIMATE_GUI_SENS_TYPE, "", UI_LABEL_COLOR); GP.TEXT("", "", climateGetSensList(sens.search, true), "", 0, "", true););
      }
      else {
        M_BOX(GP.LABEL(LANG_PAGE_CLIMATE_GUI_SENS_STATE, "", UI_LABEL_COLOR); GP.NUMBER("", "Не обнаружен...", INT32_MAX, "", true););
      }

      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_CLIMATE_GUI_HR_WIRELESS, UI_LINE_COLOR, UI_HINT_COLOR);
      if (!wirelessGetOnlineStastus()) {
        M_BOX(GP.LABEL(LANG_PAGE_CLIMATE_GUI_SENS_STATE, "", UI_LABEL_COLOR); GP.NUMBER("", wirelessGetStrStastus(), INT32_MAX, "", true););
      }
      else {
        M_BOX(GP.LABEL(LANG_PAGE_CLIMATE_GUI_SENS_DATA, "", UI_LABEL_COLOR); GP.TEXT("", "", climateGetSensDataStr(sens.temp[SENS_WIRELESS], sens.press[SENS_WIRELESS], sens.hum[SENS_WIRELESS]), "", 0, "", true););
      }
      if (wirelessGetSensorStastus()) {
        M_BOX(GP.LABEL(LANG_PAGE_CLIMATE_GUI_SENS_TIME, "", UI_LABEL_COLOR); GP.TEXT("", "", String(wirelessGetInterval()) + LANG_PAGE_CLIMATE_GUI_SENS_MINS, "", 0, "", true););
      }

      GP.BREAK("35px");
      GP.BLOCK_END();
    }
    else if (ui.uri("/weather")) { //погода
      PAGE_TITLE_NAME(LANG_PAGE_WEATHER_TITLE);

      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_WEATHER_BLOCK_CHART, UI_BLOCK_COLOR);
      if (weatherGetValidStatus()) {
#ifdef RUSSIAN
        GP.PLOT_STOCK_BEGIN(climateLocal, true);
#else
        GP.PLOT_STOCK_BEGIN(climateLocal);
#endif
        GP.PLOT_STOCK_DARK("weatherDataMain", climateNamesMain, weatherDates, weatherArrMain[0], weatherArrMain[1], WEATHER_BUFFER, 1, 10, 300, UI_BAR_TEMP_COLOR, UI_BAR_HUM_COLOR);
        GP.PLOT_STOCK_DARK("weatherDataExt", climateNamesExt, weatherDates, weatherArrExt[0], NULL, WEATHER_BUFFER, 2, 10, 300, UI_BAR_PRESS_COLOR);
        GP.BREAK();
        GP.BLOCK_END();

        uint8_t time_start = (weatherDates[0] % 86400UL) / 3600UL;

        GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_WEATHER_BLOCK_HOURS, UI_BLOCK_COLOR);
        for (uint8_t i = 0; i < WEATHER_BUFFER; i++) {
          GP.BLOCK_THIN_BOLD("90%", UI_WEATHER_BLOCK_COLOR);
          M_BOX(GP_EDGES,
                GP.LABEL(((time_start < 10) ? '0' : '\0') + String(time_start) + F(":00"), "", UI_WEATHER_TIME_COLOR, 40);
                GP.BLOCK_BEGIN(GP_DIV_RAW);
                GP.LABEL(String(weatherArrMain[0][i] / 10.0, 1) + F("°С"), "", UI_WEATHER_TEMP_COLOR);
                GP.LABEL(String(weatherArrMain[1][i]) + '%', "", UI_WEATHER_HUM_COLOR);
                GP.BREAK();
                GP.LABEL(String(weatherArrExt[0][i] / 10) + F("mm.Hg"), "", UI_WEATHER_PRESS_COLOR);
                GP.BLOCK_END();
               );
          GP.BLOCK_END();
          if (++time_start > 23) time_start = 0;
        }
      }
      else {
        GP.BLOCK_BEGIN(GP_TAB, "93%;padding:20% 5px", "", GP_DEFAULT);
        GP.LABEL(LANG_PAGE_WEATHER_NULL, "", GP_GRAY, 40, true, true); //описание
        GP.BLOCK_END();
      }
      GP.BLOCK_END();
    }
    else if (ui.uri("/radio")) { //радиоприемник
      PAGE_TITLE_NAME(LANG_PAGE_RADIO_TITLE);

      updateList += F(",radioVol,radioFreq,radioPower");

      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_RADIO_BLOCK_SETTINGS, UI_BLOCK_COLOR);
      GP.BLOCK_BEGIN(GP_DIV_RAW, "450px");
      if (!radioSvgImage) {
        M_BOX(GP_JUSTIFY, "100%;max-width:440px",
              M_BOX(GP_LEFT, GP.BUTTON_MINI("radioMode", LANG_PAGE_RADIO_GUI_MODE, "", UI_RADIO_BACK_COLOR););
              M_BOX(GP_RIGHT, GP.LABEL(LANG_PAGE_RADIO_GUI_POWER, "", UI_LABEL_COLOR); GP.SWITCH("radioPower", radioSettings.powerState, UI_RADIO_POWER_2_COLOR););
             );
      }
      M_BOX(GP_CENTER, GP.SLIDER_MAX_C(LANG_PAGE_RADIO_GUI_VOLUME, LANG_PAGE_RADIO_GUI_MIN, LANG_PAGE_RADIO_GUI_MAX, "radioVol", radioSettings.volume, 0, 15, 1, UI_RADIO_VOL_COLOR););
      M_BOX(GP_CENTER, GP.SLIDER_MAX_C(LANG_PAGE_RADIO_GUI_FREQ, "", "", "radioFreq", radioSettings.stationsFreq / 10.0, 87.5, 108, 0.1, UI_RADIO_FREQ_1_COLOR););
      GP.BLOCK_END();

      if (radioSvgImage) {
        GP.SEND("<style>.i_mask{margin:0;}#radioMode .i_mask{margin-right:4px;}</style>\n");
        M_BOX(GP_JUSTIFY, "380px;height:70px",
              GP.ICON_FILE_BUTTON("radioMode", radioFsData[4], 40, UI_RADIO_BACK_COLOR);
              GP.ICON_FILE_BUTTON("radioSeekDown", radioFsData[0], 30, UI_RADIO_FREQ_2_COLOR);
              GP.ICON_FILE_BUTTON("radioFreqDown", radioFsData[1], 30, UI_RADIO_FREQ_2_COLOR);
              GP.ICON_FILE_BUTTON("radioFreqUp", radioFsData[2], 30, UI_RADIO_FREQ_2_COLOR);
              GP.ICON_FILE_BUTTON("radioSeekUp", radioFsData[3], 30, UI_RADIO_FREQ_2_COLOR);
              GP.ICON_CHECK("radioPower", radioFsData[5], radioSettings.powerState, 50, UI_RADIO_POWER_1_COLOR, UI_RADIO_POWER_2_COLOR);
             );
      }
      else {
        M_BOX(GP_CENTER, "100%;margin-bottom:5px",
              GP.BUTTON_MINI("radioSeekDown", "|◄◄", "", UI_RADIO_FREQ_2_COLOR, "100px;font-family:fantasy;white-space:nowrap;letter-spacing:-4px");
              GP.BUTTON_MINI("radioFreqDown", "◄", "", UI_RADIO_FREQ_2_COLOR, "100px;font-family:fantasy");
              GP.BUTTON_MINI("radioFreqUp", "►", "", UI_RADIO_FREQ_2_COLOR, "100px;font-family:fantasy");
              GP.BUTTON_MINI("radioSeekUp", "►►|", "", UI_RADIO_FREQ_2_COLOR, "100px;font-family:fantasy;white-space:nowrap;letter-spacing:-4px");
             );
      }
      GP.BLOCK_END();

      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_RADIO_BLOCK_CHANNEL, UI_BLOCK_COLOR);
      GP.TABLE_BEGIN("20%,30%,20%,30%", GP_ALS(GP_RIGHT, GP_LEFT, GP_RIGHT, GP_LEFT));
      for (int i = 0; i < 10; i += 2) {
        M_TR(
          GP.BUTTON_MINI(String("radioCh/") + i, String(LANG_PAGE_RADIO_GUI_CHANNEL) + i, "", UI_RADIO_CHANNEL_COLOR),
          GP.NUMBER_F(String("radioSta/") + i, LANG_PAGE_RADIO_GUI_CHANNEL_NULL, (radioSettings.stationsSave[i]) ? (radioSettings.stationsSave[i] / 10.0) : NAN, 1),
          GP.BUTTON_MINI(String("radioCh/") + (i + 1), String(LANG_PAGE_RADIO_GUI_CHANNEL) + (i + 1), "", UI_RADIO_CHANNEL_COLOR),
          GP.NUMBER_F(String("radioSta/") + (i + 1), LANG_PAGE_RADIO_GUI_CHANNEL_NULL, (radioSettings.stationsSave[i + 1]) ? (radioSettings.stationsSave[i + 1] / 10.0) : NAN, 1)
        );
      }
      GP.TABLE_END();
      GP.BLOCK_END();

      GP.UPDATE_CLICK("radioSta/0,radioSta/1,radioSta/2,radioSta/3,radioSta/4,radioSta/5,radioSta/6,radioSta/7,radioSta/8,radioSta/9,radioFreq",
                      "radioSta/0,radioSta/1,radioSta/2,radioSta/3,radioSta/4,radioSta/5,radioSta/6,radioSta/7,radioSta/8,radioSta/9,radioCh/0,radioCh/1,radioCh/2,radioCh/3,radioCh/4,radioCh/5,radioCh/6,radioCh/7,radioCh/8,radioCh/9,");
    }
    else if (ui.uri("/update") && (fsUpdate || otaUpdate || clockUpdate)) { //обновление прошивки
      PAGE_TITLE_NAME(LANG_PAGE_UPDATE_TITLE);

      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_UPDATE_BLOCK_UPL, UI_BLOCK_COLOR);
      webShowUpdateInfo();
      if (passGetOtaState()) webShowUpdateUI();
      else webShowUpdateAuth();
      GP.BLOCK_END();
    }
    else if (ui.uri("/information")) { //информация о системе
      PAGE_TITLE_NAME(LANG_PAGE_INFO_TITLE);

      updateList += F(",infUsage,infUptime,infFrag,infFree,infSignal,infNettime");

      GP.NAV_TABS_M("extInfoTab", LANG_PAGE_INFO_MENU, navInfoTab, UI_BUTTON_COLOR);

      GP.NAV_BLOCK_BEGIN("extInfoTab", 0, navInfoTab);
      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_INFO_BLOCK_SYSTEM, UI_BLOCK_COLOR);
      GP.BLOCK_HIDE_BEGIN();
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_ID, "", UI_LABEL_COLOR); GP.LABEL(stringGetHex(ESP.getChipId()), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_CPU, "", UI_LABEL_COLOR); GP.LABEL(stringGetFreq(ESP.getCpuFreqMHz()), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_USAGE, "", UI_LABEL_COLOR); GP.LABEL(stringGetPercent(systemGetUsage()), "infUsage", UI_INFO_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_UPTIME, "", UI_LABEL_COLOR); GP.LABEL(stringGetTimeFromMs(millis()), "infUptime", UI_INFO_COLOR););

      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_INFO_HR_MEMORY, UI_LINE_COLOR, UI_HINT_COLOR);

      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_HEAP_FRAG, "", UI_LABEL_COLOR); GP.LABEL(stringGetPercent(ESP.getHeapFragmentation()), "infFrag", UI_INFO_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_HEAP_FREE, "", UI_LABEL_COLOR); GP.LABEL(stringGetKilobyte(ESP.getFreeHeap(), 3), "infFree", UI_INFO_COLOR););

      GP.BREAK();
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_FLASH_ALL, "", UI_LABEL_COLOR); GP.LABEL(stringGetKilobyte(ESP.getFlashChipSize(), 1), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_FLASH_FULL, "", UI_LABEL_COLOR); GP.LABEL(stringGetKilobyte(ESP.getSketchSize(), 1), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_FLASH_FREE, "", UI_LABEL_COLOR); GP.LABEL(stringGetKilobyte(ESP.getFreeSketchSpace(), 1), "", UI_INFO_COLOR););

      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_INFO_HR_NETWORK, UI_LINE_COLOR, UI_HINT_COLOR);

      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_SIGNAL, "", UI_LABEL_COLOR); GP.LABEL(stringGetPercent(wifiGetSignalStrength()), "infSignal", UI_INFO_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_MODE, "", UI_LABEL_COLOR); GP.LABEL(wifiGetCurrentMode(), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_MAC, "", UI_LABEL_COLOR); GP.LABEL(WiFi.macAddress(), "", UI_INFO_COLOR););

      if (wifiGetConnectStatus()) {
        GP.BREAK();
        M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_NET_SSID, "", UI_LABEL_COLOR); GP.LABEL(stringLengthConstrain(WiFi.SSID(), 12), "hint1", UI_INFO_COLOR););
        M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_NET_IP, "", UI_LABEL_COLOR); GP.LABEL(WiFi.localIP().toString(), "", UI_INFO_COLOR););
        M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_NET_GATEWAY, "", UI_LABEL_COLOR); GP.LABEL(WiFi.gatewayIP().toString(), "", UI_INFO_COLOR););
        M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_NET_SUBNET, "", UI_LABEL_COLOR); GP.LABEL(WiFi.subnetMask().toString(), "", UI_INFO_COLOR););
        M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_NET_TIME, "", UI_LABEL_COLOR); GP.LABEL(stringGetTimeFromMs(wifiGetConnectTime()), "infNettime", UI_INFO_COLOR););
        GP.HINT("hint1", WiFi.SSID()); //всплывающая подсказка
      }
      if (WiFi.getMode() != WIFI_STA) {
        GP.BREAK();
        M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_AP_SSID, "", UI_LABEL_COLOR); GP.LABEL(stringLengthConstrain(wifiGetApSSID(), 12), "hint2", UI_INFO_COLOR););
        M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_AP_IP, "", UI_LABEL_COLOR); GP.LABEL(WiFi.softAPIP().toString(), "", UI_INFO_COLOR););
        GP.HINT("hint2", wifiGetApSSID()); //всплывающая подсказка
      }

      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_INFO_HR_VERSION, UI_LINE_COLOR, UI_HINT_COLOR);

      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_SDK, "", UI_LABEL_COLOR); GP.LABEL(ESP.getSdkVersion(), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_CORE, "", UI_LABEL_COLOR); GP.LABEL(ESP.getCoreVersion(), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_GP, "", UI_LABEL_COLOR); GP.LABEL(GP_VERSION, "", UI_INFO_COLOR););

      GP.BREAK();
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_ESP, "", UI_LABEL_COLOR); GP.LABEL(ESP_FIRMWARE_VERSION, "", UI_INFO_COLOR););
      if (deviceInformation[HARDWARE_VERSION]) {
        M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_CLOCK, "", UI_LABEL_COLOR); GP.LABEL(getClockFirmwareVersion(), "", UI_INFO_COLOR););
      }

      if (!(device.failure & 0x8000)) {
        GP.BREAK();
        GP.HR_TEXT(LANG_PAGE_INFO_HR_STATE, UI_LINE_COLOR, UI_HINT_COLOR);
        if (!device.failure) {
          M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_CONNECT, "", UI_LABEL_COLOR); GP.LABEL((busGetClockStatus()) ? LANG_PAGE_INFO_GUI_STATE_OK : LANG_PAGE_INFO_GUI_STATE_DIS, "", UI_INFO_COLOR););
        }
        else {
          for (uint8_t i = 0; i < 13; i++) {
            if (device.failure & (0x01 << i)) {
              M_BOX(GP.LABEL(String(LANG_PAGE_INFO_GUI_STATE_ERR) + ((i < 9) ? '0' : '\0') + (i + 1), "", UI_LABEL_COLOR); GP.LABEL_W(failureDataList[i], "", UI_INFO_COLOR, 0, GP_RIGHT, 0, false, true););
            }
          }
        }
      }
      GP.BREAK();
      GP.BLOCK_END();
      GP.BLOCK_END();
      GP.NAV_BLOCK_END();

      GP.NAV_BLOCK_BEGIN("extInfoTab", 1, navInfoTab);
      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_INFO_BLOCK_DEVICE, UI_BLOCK_COLOR);
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_NAME, "", UI_LABEL_COLOR); GP.TEXT_EN("extDeviceName", LANG_PAGE_INFO_GUI_NONE, settings.nameDevice, "", 19););
      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_INFO_HR_SHOW, UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_MENU, "", UI_LABEL_COLOR); GP.SWITCH("extDeviceMenu", settings.nameMenu, UI_SWITCH_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_PREFIX, "", UI_LABEL_COLOR); GP.SWITCH("extDevicePrefix", settings.namePrefix, UI_SWITCH_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_POSTFIX, "", UI_LABEL_COLOR); GP.SWITCH("extDevicePostfix", settings.namePostfix, UI_SWITCH_COLOR););
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_AP_NAME, "", UI_LABEL_COLOR); GP.SWITCH("extDeviceAp", settings.nameAp, UI_SWITCH_COLOR););

      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_INFO_HR_GROUP, UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_DETECT, "", UI_LABEL_COLOR); GP.SWITCH("extDeviceGroup", settings.groupFind, UI_SWITCH_COLOR););

      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_INFO_HR_WIRELESS, UI_LINE_COLOR, UI_HINT_COLOR);
      if (!wirelessGetOnlineStastus()) {
        M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_STATE, "", UI_LABEL_COLOR); GP.NUMBER("", wirelessGetStrStastus(), INT32_MAX, "", true););
      }
      if (wirelessGetSensorStastus()) {
        M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_UID, "", UI_LABEL_COLOR); GP.NUMBER("", wirelessGetId(settings.wirelessId), INT32_MAX, "", true););
        M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_SIGNAL, "", UI_LABEL_COLOR); GP.NUMBER("", stringGetPercent(wirelessGetSignal()), INT32_MAX, "", true););
        M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_BATTERY, "", UI_LABEL_COLOR); GP.NUMBER("", stringGetPercent(wirelessGetBattery()), INT32_MAX, "", true););
      }

      String rtcStatus;
      rtcStatus.reserve(50);
      rtcStatus = F(LANG_RTC_STATUS_1);
      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_INFO_HR_RTC, UI_LINE_COLOR, UI_HINT_COLOR);
      if (deviceInformation[DS3231_ENABLE] && !(device.failure & 0x8000)) {
        if (!(device.failure & 0x03)) rtcStatus = F(LANG_RTC_STATUS_3);
        else if (device.failure & 0x02) rtcStatus = F(LANG_RTC_STATUS_2);
      }
      else if (rtcGetFoundStatus()) {
        M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_CORRECT, "", UI_LABEL_COLOR); GP.NUMBER("syncAging", "-128..127", rtc_aging, "-128", "127"););
        if (!rtcGetNormalStatus()) rtcStatus = F(LANG_RTC_STATUS_2);
        else rtcStatus = F(LANG_RTC_STATUS_4);
      }
      M_BOX(GP.LABEL(LANG_PAGE_INFO_GUI_STATE, "", UI_LABEL_COLOR); GP.NUMBER("", rtcStatus, INT32_MAX, "", true););

      GP.BREAK();
      GP.HR_TEXT(LANG_PAGE_INFO_HR_CONTROL, UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP_JUSTIFY, M_BOX(GP_LEFT, "200px", GP.LABEL(LANG_PAGE_INFO_GUI_RESET, "", UI_LABEL_COLOR);); GP.BUTTON_MINI("resetButton", LANG_PAGE_INFO_GUI_EXECUTE, "", UI_BUTTON_COLOR, "200px"););
      M_BOX(GP_JUSTIFY, M_BOX(GP_LEFT, "200px", GP.LABEL(LANG_PAGE_INFO_GUI_REBOOT, "", UI_LABEL_COLOR);); GP.BUTTON_MINI("rebootButton", LANG_PAGE_INFO_GUI_EXECUTE, "", UI_BUTTON_COLOR, "200px"););
      GP.VOID_BOX("0;height:8px");
      GP.BLOCK_END();
      GP.NAV_BLOCK_END();

      PAGE_ALERT_BLOCK("alertReset", LANG_PAGE_INFO_ALERT_RESET_TITLE, LANG_PAGE_INFO_ALERT_RESET_1);
      PAGE_ALERT_BLOCK("alertReboot", LANG_PAGE_INFO_ALERT_REBOOT_TITLE, LANG_PAGE_INFO_ALERT_REBOOT_1);

      GP.UPDATE_CLICK("alertReset", "resetButton");
      GP.UPDATE_CLICK("alertReboot", "rebootButton");
      GP.RELOAD_CLICK(String("extDeviceMenu,extDevicePrefix,extDevicePostfix") + ((settings.nameMenu || settings.namePrefix || settings.namePostfix) ? ",extDeviceName" : ""));
    }
    else { //сетевые настройки
      PAGE_TITLE_NAME(LANG_PAGE_NETWORK_TITLE);

      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_NETWORK_BLOCK_WIFI, UI_BLOCK_COLOR);
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
        GP.SUBMIT((wifiGetConnectStatus()) ? LANG_PAGE_NETWORK_GUI_DISCONNECT : LANG_PAGE_NETWORK_GUI_CANCEL, UI_BUTTON_COLOR);
        GP.FORM_END();
      }
      else { //выбор сети
        if (wifiGetScanCompleteStatus()) wifiResetScanCompleteStatus();

        updateList += F(",extScan");
        GP.RELOAD("extScan");

        GP.FORM_BEGIN("/connection");
        if (ui.uri("/manual")) { //ручной режим ввода сети
          GP.TEXT("wifiSsid", LANG_PAGE_NETWORK_GUI_SSID, settings.wifiSSID, "", 64);
          GP.BREAK();
          GP.PASS_EYE("wifiPass", LANG_PAGE_NETWORK_GUI_PASS, settings.wifiPASS, 64);
          GP.BREAK();
          GP.TEXT_LINK("/network", LANG_PAGE_NETWORK_GUI_LIST, "net", UI_LINK_COLOR);
          GP.HR(UI_LINE_COLOR);
          GP.BOX_BEGIN(GP_CENTER, "300px");
          GP.SUBMIT(LANG_PAGE_NETWORK_GUI_CONNECT, UI_BUTTON_COLOR);
          GP.BUTTON("extClear", "✕", "", (!settings.wifiSSID[0] && !settings.wifiPASS[0]) ? GP_GRAY : UI_BUTTON_COLOR, "65px", (boolean)(!settings.wifiSSID[0] && !settings.wifiPASS[0]), true);
          GP.BOX_END();
        }
        else { //выбор сети из списка
          GP.SELECT_LIST("wifiSsid", wifi_scan_list, 0, 0, wifiGetScanFoundStatus());
          GP.BREAK();
          GP.PASS_EYE("wifiPass", LANG_PAGE_NETWORK_GUI_PASS, settings.wifiPASS, 64);
          GP.BREAK();
          GP.TEXT_LINK("/manual", LANG_PAGE_NETWORK_GUI_MANUAL, "net", UI_LINK_COLOR);
          GP.HR(UI_LINE_COLOR);
          GP.BOX_BEGIN(GP_CENTER, "300px");
          if (wifiGetScanFoundStatus()) GP.BUTTON("", LANG_PAGE_NETWORK_GUI_CONNECT, "", GP_GRAY, "90%", true);
          else GP.SUBMIT(LANG_PAGE_NETWORK_GUI_CONNECT, UI_BUTTON_COLOR);
          GP.BUTTON("extScan", "<big><big>↻</big></big>", "", UI_BUTTON_COLOR, "65px", false, true);
          GP.BOX_END();
        }
        GP.FORM_END();
      }
      GP.BLOCK_END();

      updateList += F(",extNtp,extWeather");

      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_NETWORK_BLOCK_NTP, UI_BLOCK_COLOR);
      GP.TEXT("syncHost", LANG_PAGE_NETWORK_GUI_HOST, settings.ntpHost, "", 19);
      GP.BREAK();
      GP.SELECT_LIST("syncPer", String(LANG_PAGE_NETWORK_TIME_MODE_1) + ((settings.ntpDst) ? "" : LANG_PAGE_NETWORK_TIME_MODE_2), (settings.ntpDst && (settings.ntpTime > 2)) ? 2 : settings.ntpTime);
      GP.SPAN(ntpGetState(), GP_CENTER, "extNtp", UI_INFO_COLOR); //описание
      GP.HR(UI_LINE_COLOR);
      GP.BUTTON("syncCheck", LANG_PAGE_NETWORK_GUI_SYNC, "", (!ntpGetRunStatus() || !wifiGetConnectStatus()) ? GP_GRAY : UI_BUTTON_COLOR, "90%", (boolean)(!ntpGetRunStatus() || !wifiGetConnectStatus()));
      GP.BLOCK_END();

      GP.UPDATE_CLICK("extNtp", "syncCheck");

      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_NETWORK_BLOCK_WEATHER, UI_BLOCK_COLOR);
      GP.SELECT_LIST("weatherCity", String(weatherCityList) + LANG_PAGE_NETWORK_WEATHER_MODE, settings.weatherCity, 0, false, true);
      GP.BREAK();
      M_BOX(GP_CENTER, "209px",
            GP.NUMBER_F("weatherLat", LANG_PAGE_NETWORK_GUI_LAT, (settings.weatherCity < WEATHER_CITY_ARRAY) ? weatherCoordinatesList[0][settings.weatherCity] : settings.weatherLat, 4, "", (boolean)(settings.weatherCity < WEATHER_CITY_ARRAY));
            GP.NUMBER_F("weatherLon", LANG_PAGE_NETWORK_GUI_LON, (settings.weatherCity < WEATHER_CITY_ARRAY) ? weatherCoordinatesList[1][settings.weatherCity] : settings.weatherLon, 4, "", (boolean)(settings.weatherCity < WEATHER_CITY_ARRAY));
           );
      GP.SPAN(weatherGetState(), GP_CENTER, "extWeather", UI_INFO_COLOR); //описание
      GP.HR(UI_LINE_COLOR);
      GP.BUTTON("weatherUpdate", LANG_PAGE_NETWORK_GUI_UPDATE, "", (!weatherGetRunStatus() || !wifiGetConnectStatus()) ? GP_GRAY : UI_BUTTON_COLOR, "90%", (boolean)(!weatherGetRunStatus() || !wifiGetConnectStatus()));
      GP.BLOCK_END();

      GP.UPDATE_CLICK("extWeather", "weatherUpdate");
    }

    static boolean failureWarn = true; //флаг отображения предупреждения об сбоях
    if (!(device.failure & 0x8000) && device.failure && failureWarn) {
      failureWarn = false;
      PAGE_ALERT_BLOCK("alertFail", LANG_FAIL_ALERT_TITLE, LANG_FAIL_ALERT_1, LANG_FAIL_ALERT_2, false, true);
      GP.POPUP_OPEN("alertFail");
    }

    updateList += F(",extGroup,alertFound,alertFoundText");

    wirelessResetFoundState();
    PAGE_ALERT_BLOCK("alertFound", LANG_WIRELESS_ALERT_TITLE, LANG_WIRELESS_ALERT_FOUND, "UID: 00:00:00:00:00:00");

    GP.UPDATE(updateList);
    GP.UI_END(); //завершить окно панели управления
  }

  GP.PAGE_BLOCK_END();
  GP.BUILD_END();
}
//--------------------------------------------------------------------
void buildUpdate(bool UpdateEnd, const String& UpdateError) {
  GP.BUILD_BEGIN(GP_DEFAULT_THEME);

  GP.PAGE_ZOOM(90, 390);
  GP.PAGE_BLOCK_BEGIN(500);

  if (!webShowUpdateState()) {
    PAGE_TITLE_NAME(LANG_PAGE_UPDATE_TITLE);

    GP.PAGE_MIDDLE_ALIGN();

    if (!UpdateEnd) {
      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_UPDATE_BLOCK_UPL, UI_BLOCK_COLOR);
      webShowUpdateInfo();
      webShowUpdateUI();
    }
    else {
      GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_UPDATE_BLOCK_WEB, UI_BLOCK_COLOR);
      GP.BLOCK_OFFSET_BEGIN();
      if (UpdateError.length()) {
        GP.SPAN(String(LANG_PAGE_UPDATE_WARN_ERR) + "<small>[" + UpdateError + "]</small>", GP_CENTER, "", GP_RED); //описание
      }
      else {
        GP.SPAN(LANG_PAGE_UPDATE_WAIT, GP_CENTER, "extUpdate", UI_INFO_COLOR); //описание
        GP.SPAN(LANG_PAGE_UPDATE_WARN_PWR, GP_CENTER, "extWarn", GP_RED); //описание
        GP.UPDATE("extUpdate,extWarn");
      }
      GP.BLOCK_END();
    }
    GP.HR(UI_LINE_COLOR);
    GP.BOX_BEGIN(GP_CENTER);
    GP.BUTTON_MINI_LINK("/", LANG_PAGE_UPDATE_GUI_HOME, UI_BUTTON_COLOR);
    GP.BOX_END();
    GP.BLOCK_END();
  }

  GP.PAGE_BLOCK_END();
  GP.BUILD_END();
}
//--------------------------------------------------------------------
void webShowUpdateInfo(void) {
  GP.BREAK();
  GP.SPAN(LANG_PAGE_UPDATE_INFO_FW, GP_CENTER, "", UI_INFO_COLOR); //описание
  GP.BREAK();
  if (fsUpdate) {
    GP.SPAN(LANG_PAGE_UPDATE_INFO_FS, GP_CENTER, "", UI_INFO_COLOR); //описание
    GP.BREAK();
  }
  String formatText;
  formatText.reserve(100);
  formatText = F(LANG_PAGE_UPDATE_INFO_FORMAT);
  if (clockUpdate) formatText += F(LANG_PAGE_UPDATE_INFO_HEX);
  if (fsUpdate || otaUpdate) {
    if (clockUpdate) formatText += F(", ");
    formatText += F(LANG_PAGE_UPDATE_INFO_BIN);
  }
  else formatText += '.';
  GP.SPAN(formatText, GP_CENTER, "", UI_INFO_COLOR); //описание
  GP.BREAK();
}
void webShowUpdateUI(void) {
  GP.HR_TEXT(LANG_PAGE_UPDATE_HR_FILE, UI_LINE_COLOR, UI_HINT_COLOR);
  if (clockUpdate) {
    M_BOX(GP.LABEL(LANG_PAGE_UPDATE_GUI_FW_CLOCK, "", UI_LABEL_COLOR); GP.FILE_UPLOAD("updater", "📟", ".hex", UI_BUTTON_COLOR););
  }
  if (otaUpdate) {
    M_BOX(GP.LABEL(LANG_PAGE_UPDATE_GUI_FW_ESP, "", UI_LABEL_COLOR); GP.OTA_FIRMWARE("📥", UI_BUTTON_COLOR, true););
  }
  if (fsUpdate) {
    M_BOX(GP.LABEL(LANG_PAGE_UPDATE_GUI_FS_ESP, "", UI_LABEL_COLOR); GP.OTA_FILESYSTEM("📼", UI_BUTTON_COLOR, true););
  }
  GP.VOID_BOX("0;height:10px");
}
void webShowUpdateAuth(void) {
  GP.HR_TEXT(LANG_PAGE_UPDATE_HR_AUTH, UI_LINE_COLOR, UI_HINT_COLOR, "", GP_CENTER);
  if (!passGetWriteTimeout()) {
    M_BOX(GP_CENTER, GP.PASS_EYE("extOtaPass", LANG_PAGE_UPDATE_GUI_PASS, "", 8); GP.BUTTON_MINI("extOtaCheck", LANG_PAGE_UPDATE_GUI_LOGIN, "extOtaPass", UI_BUTTON_COLOR, "200px!important", false, true););
    if (passGetCheckError()) GP.SPAN(LANG_PAGE_UPDATE_WARN_PASS, GP_CENTER, "", GP_RED); //описание
    else GP.SPAN(LANG_PAGE_UPDATE_INFO_AUTH, GP_CENTER, "", GP_YELLOW); //описание
  }
  else {
    M_BOX(GP_CENTER, GP.TEXT("", "", LANG_PAGE_UPDATE_GUI_PASS, "", 0, "", true); GP.BUTTON_MINI("", LANG_PAGE_UPDATE_GUI_LOGIN, "", GP_GRAY, "200px!important", true););
    GP.SPAN(String((passGetCheckError()) ? LANG_PAGE_UPDATE_WARN_PASS : "") + LANG_PAGE_UPDATE_WARN_TIME, GP_CENTER, "", GP_RED); //описание
  }
  GP.VOID_BOX("0;height:10px");
}
//--------------------------------------------------------------------
boolean webShowUpdateState(void) {
  if (updaterState()) {
    PAGE_TITLE_NAME(LANG_PAGE_UPDATE_CLOCK_TITLE);

    GP.PAGE_MIDDLE_ALIGN();

    GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_UPDATE_CLOCK_BLOCK, UI_BLOCK_COLOR);
    GP.BLOCK_OFFSET_BEGIN();
    if (!updaterFlash()) {
      GP.SPAN(getUpdaterState(), GP_CENTER, "extUpdate", GP_YELLOW); //описание
    }
    else {
      GP.SPAN(LANG_PAGE_UPDATE_CLOCK_CONNECT, GP_CENTER, "extUpdate", UI_INFO_COLOR); //описание
      GP.SPAN(String(LANG_PAGE_UPDATE_CLOCK_WARN) + ((deviceInformation[HARDWARE_VERSION]) ? "" : LANG_PAGE_UPDATE_CLOCK_HINT), GP_CENTER, "extWarn", GP_RED); //описание
      GP.UPDATE("extUpdate,extWarn", 300);
    }
    GP.BLOCK_END();
    GP.HR(UI_LINE_COLOR);
    GP.BOX_BEGIN(GP_CENTER);
    GP.BUTTON_MINI_LINK("/", LANG_PAGE_UPDATE_CLOCK_HOME, UI_BUTTON_COLOR);
    GP.BOX_END();
    GP.BLOCK_END();

    return true;
  }
  return false;
}
//--------------------------------------------------------------------
boolean webShowHardwareInfo(void) {
  if (deviceInformation[HARDWARE_VERSION] && (deviceInformation[HARDWARE_VERSION] != HW_VERSION)) {
    PAGE_TITLE_NAME(LANG_PAGE_COMPATIBILITY_TITLE);

    GP.PAGE_MIDDLE_ALIGN();

    GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_COMPATIBILITY_BLOCK, UI_BLOCK_COLOR);
    GP.BLOCK_OFFSET_BEGIN();
    GP.SPAN(LANG_PAGE_COMPATIBILITY_WARN, GP_CENTER, "", UI_INFO_COLOR);
    GP.BREAK();
    GP.LABEL(LANG_PAGE_COMPATIBILITY_HW_C + String(deviceInformation[HARDWARE_VERSION], HEX));
    GP.BREAK();
    GP.LABEL(LANG_PAGE_COMPATIBILITY_HW_W + String(HW_VERSION, HEX));
    GP.BLOCK_END();
    if (otaUpdate) {
      GP.HR(UI_LINE_COLOR);
      GP.BOX_BEGIN(GP_CENTER);
      GP.BUTTON_MINI_LINK("/ota_update", LANG_PAGE_COMPATIBILITY_UPDATE, UI_BUTTON_COLOR);
      GP.BOX_END();
    }
    GP.BLOCK_END();

    passSetOtaState();
    return true;
  }
  return false;
}
//--------------------------------------------------------------------
boolean webShowReloadInfo(void) {
  if (busRebootState()) {
    PAGE_TITLE_NAME(LANG_PAGE_RELOAD_TITLE);

    GP.PAGE_MIDDLE_ALIGN();

    GP.BLOCK_BEGIN(GP_THIN, "", LANG_PAGE_RELOAD_BLOCK, UI_BLOCK_COLOR);
    GP.BLOCK_OFFSET_BEGIN();
    GP.SPAN(LANG_PAGE_RELOAD_WAIT, GP_CENTER, "extReboot", UI_INFO_COLOR); //описание
    GP.SPAN(LANG_PAGE_RELOAD_HINT, GP_CENTER, "extWarn", GP_RED); //описание
    GP.BLOCK_END();
    GP.HR(UI_LINE_COLOR);
    GP.BOX_BEGIN(GP_CENTER);
    GP.BUTTON_MINI_LINK("/", LANG_PAGE_RELOAD_HOME, UI_BUTTON_COLOR);
    GP.BOX_END();
    GP.UPDATE("extReboot,extWarn");
    GP.BLOCK_END();

    return true;
  }
  return false;
}
//--------------------------------------------------------------------
void action() {
  if (ui.click()) {
    if (ui.clickSub("sync")) {
      if (ui.click("syncGmt")) {
        settings.ntpGMT = ui.getInt("syncGmt") - 12;
        if (settings.ntpSync && (ntpGetSyncStatus())) {
          ntpRequest(); //запросить текущее время
          syncState = 0; //сбросили флаг синхронизации
        }
        memorySaveSettings(); //обновить данные в памяти
      }
      if (ui.clickBool("syncAuto", settings.ntpSync)) {
        if (settings.ntpSync && (ntpGetSyncStatus())) {
          ntpRequest(); //запросить текущее время
          syncState = 0; //сбросили флаг синхронизации
        }
        memorySaveSettings(); //обновить данные в памяти
      }
      if (ui.clickBool("syncDst", settings.ntpDst)) {
        if (settings.ntpSync && (ntpGetSyncStatus())) {
          ntpRequest(); //запросить текущее время
          syncState = 0; //сбросили флаг синхронизации
        }
        memorySaveSettings(); //обновить данные в памяти
      }
      if (ui.click("syncTime")) {
        if (ntpGetSyncStatus()) {
          ntpRequest(); //запросить текущее время
          syncState = 0; //сбросили флаг синхронизации
        }
        else {
          mainTime = ui.getSystemTime(); //запросить время браузера
          mainDate = ui.getSystemDate(); //запросить дату браузера
          busSetCommand(WRITE_TIME_DATE);
          if (rtcGetFoundStatus()) busSetCommand(WRITE_RTC_TIME); //отправить время в RTC
        }
      }

      if (ui.click("syncHost")) {
        if (ui.getString("syncHost").length() > 0) strncpy(settings.ntpHost, ui.getString("syncHost").c_str(), 20); //копируем себе
        else strncpy(settings.ntpHost, DEFAULT_NTP_HOST, 20); //установить хост по умолчанию
        settings.ntpHost[19] = '\0'; //устанавливаем последний символ
        memorySaveSettings(); //обновить данные в памяти
      }
      if (ui.click("syncPer")) {
        settings.ntpTime = ui.getInt("syncPer");
        if (settings.ntpTime > (sizeof(ntpSyncTime) - 1)) settings.ntpTime = sizeof(ntpSyncTime) - 1;
        memorySaveSettings(); //обновить данные в памяти
      }
      if (ui.click("syncCheck")) {
        ntpRequest(); //запросить текущее время
        syncState = 0; //сбросили флаг синхронизации
      }

      if (ui.click("syncAging")) {
        if (rtcGetFoundStatus()) {
          rtc_aging = constrain(ui.getInt("syncAging"), -128, 127);
          busSetCommand(WRITE_RTC_AGING);
        }
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("weather")) {
      if (ui.click("weatherCity")) {
        settings.weatherCity = ui.getInt("weatherCity");
        memorySaveSettings(); //обновить данные в памяти
      }
      if (ui.click("weatherLat")) {
        settings.weatherLat = (ui.getString("weatherLat").length()) ? ui.getFloat("weatherLat") : NAN;
        memorySaveSettings(); //обновить данные в памяти
      }
      if (ui.click("weatherLon")) {
        settings.weatherLon = (ui.getString("weatherLon").length()) ? ui.getFloat("weatherLon") : NAN;
        memorySaveSettings(); //обновить данные в памяти
      }

      if (ui.click("weatherUpdate")) {
        weatherCheck(); //запросить прогноз погоды
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("alarm")) {
      if (alarm.set) { //если режим настройки будильника
        if (ui.click("alarmVol")) {
          alarm_data[alarm.now][ALARM_DATA_VOLUME] = ui.getInt("alarmVol");
          busSetCommand(WRITE_SELECT_ALARM, ALARM_VOLUME);
          if ((!alarm_data[alarm.now][ALARM_DATA_RADIO] || !deviceInformation[RADIO_ENABLE]) && alarm_data[alarm.now][ALARM_DATA_MODE]) busSetCommand(WRITE_TEST_ALARM_VOL);
        }
        else if (playbackTimer > -1) playbackTimer = 0; //сбросили воспроизведение
        if (ui.click("alarmSoundType")) {
          alarm_data[alarm.now][ALARM_DATA_RADIO] = ui.getBool("alarmSoundType");
          busSetCommand(WRITE_SELECT_ALARM, ALARM_RADIO);
          busSetCommand(WRITE_SELECT_ALARM, ALARM_SOUND);
          busSetCommand(WRITE_SELECT_ALARM, ALARM_VOLUME);
        }
        if (ui.click("alarmSound")) {
          if (!alarm_data[alarm.now][ALARM_DATA_RADIO]) {
            alarm_data[alarm.now][ALARM_DATA_SOUND] = ui.getInt("alarmSound");
            if ((!deviceInformation[PLAYER_TYPE] || alarm_data[alarm.now][ALARM_DATA_VOLUME]) && (!deviceInformation[RADIO_ENABLE] || !radioSettings.powerState)) {
              busSetCommand(WRITE_TEST_ALARM_SOUND);
              playbackTimer = 5;
            }
            busSetCommand(WRITE_SELECT_ALARM, ALARM_SOUND);
          }
        }
        if (ui.click("alarmRadio")) {
          if (alarm_data[alarm.now][ALARM_DATA_RADIO]) {
            alarm_data[alarm.now][ALARM_DATA_STATION] = ui.getInt("alarmRadio");
            busSetCommand(WRITE_SELECT_ALARM, ALARM_SOUND);
          }
        }
        if (ui.click("alarmTimeH")) {
          alarm_data[alarm.now][ALARM_DATA_HOUR] = constrain(ui.getInt("alarmTimeH"), 0, 23);
          busSetCommand(WRITE_SELECT_ALARM, ALARM_TIME);
        }
        if (ui.click("alarmTimeM")) {
          alarm_data[alarm.now][ALARM_DATA_MINS] = constrain(ui.getInt("alarmTimeM"), 0, 59);
          busSetCommand(WRITE_SELECT_ALARM, ALARM_TIME);
        }
        if (ui.click("alarmMode")) {
          alarm_data[alarm.now][ALARM_DATA_MODE] = ui.getInt("alarmMode");
          busSetCommand(WRITE_SELECT_ALARM, ALARM_MODE);
        }
        if (ui.clickSub("alarmDay")) {
          uint8_t day = constrain(ui.clickNameSub(1).toInt(), 1, 7);
          if (ui.getBool(String("alarmDay/") + day)) alarm_data[alarm.now][ALARM_DATA_DAYS] |= (0x01 << day);
          else alarm_data[alarm.now][ALARM_DATA_DAYS] &= ~(0x01 << day);
          busSetCommand(WRITE_SELECT_ALARM, ALARM_DAYS);
        }
        if (ui.click("alarmDel") && !alarm.reload) {
          if (alarm.all > 1) {
            alarm.all--;
            busSetCommand(DEL_ALARM, alarm.now);
            busSetCommand(READ_ALARM_ALL);
          }
          else {
            alarm_data[alarm.now][ALARM_DATA_MODE] = 0;
            busSetCommand(WRITE_SELECT_ALARM, ALARM_MODE);
          }
          alarm.set = 0;
        }
        if (ui.click("alarmBack")) {
          alarm.set = 0;
        }
      }
      else {
        if (ui.clickSub("alarmSet")) {
          alarm.now = constrain(ui.clickNameSub(1).toInt(), 0, MAX_ALARMS - 1);
          alarm.set = 1;
        }
        if (ui.click("alarmAdd") && !alarm.reload) {
          if (alarm.all < MAX_ALARMS) {
            alarm.now = alarm.all;
            alarm.all++;
            alarm.set = 2;
            busSetCommand(NEW_ALARM);
            busSetCommand(READ_ALARM_ALL);
          }
        }
        if (ui.click("alarmDis")) {
          busSetCommand(WRITE_ALARM_DISABLE);
        }
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("sets")) {
      if (ui.click("setsDateD")) {
        mainDate.day = constrain(ui.getInt("setsDateD"), 1, maxDays(mainDate.year, mainDate.month));
        busSetCommand(WRITE_DATE);
      }
      if (ui.click("setsDateM")) {
        mainDate.month = constrain(ui.getInt("setsDateM"), 1, 12);
        mainDate.day = constrain(mainDate.day, 1, maxDays(mainDate.year, mainDate.month));
        busSetCommand(WRITE_DATE);
      }
      if (ui.click("setsDateY")) {
        mainDate.year = constrain(ui.getInt("setsDateY"), 1, 12);
        mainDate.day = constrain(mainDate.day, 1, maxDays(mainDate.year, mainDate.month));
        busSetCommand(WRITE_DATE);
      }
      if (ui.click("setsTimeH")) {
        mainTime.hour = constrain(ui.getInt("setsTimeH"), 0, 23);
        busSetCommand(WRITE_TIME);
      }
      if (ui.click("setsTimeM")) {
        mainTime.minute = constrain(ui.getInt("setsTimeM"), 0, 59);
        busSetCommand(WRITE_TIME);
      }
      if (ui.click("setsTimeS")) {
        mainTime.second = constrain(ui.getInt("setsTimeS"), 0, 59);
        busSetCommand(WRITE_TIME);
      }

      if (ui.clickBool("setsTimeFormat", mainSettings.timeFormat)) {
        busSetCommand(WRITE_MAIN_SET, MAIN_TIME_FORMAT);
      }
      if (ui.clickBool("setsGlitch", mainSettings.glitchMode)) {
        busSetCommand(WRITE_MAIN_SET, MAIN_GLITCH_MODE);
      }

      if (ui.clickInt("setsDot", fastSettings.dotMode)) {
        busSetCommand(WRITE_FAST_SET, FAST_DOT_MODE);
      }
      if (ui.clickInt("setsNeonDot", fastSettings.neonDotMode)) {
        busSetCommand(WRITE_FAST_SET, FAST_NEON_DOT_MODE);
      }
      if (ui.clickInt("setsFlip", fastSettings.flipMode)) {
        busSetCommand(WRITE_FAST_SET, FAST_FLIP_MODE);
      }
      if (ui.clickInt("setsSecsFlip", fastSettings.secsMode)) {
        busSetCommand(WRITE_FAST_SET, FAST_SECS_MODE);
      }
      if (ui.clickInt("setsBackl", fastSettings.backlMode)) {
        busSetCommand(WRITE_FAST_SET, FAST_BACKL_MODE);
      }
      if (ui.click("setsColor")) {
        uint8_t color = constrain(ui.getInt("setsColor"), 0, 28);
        fastSettings.backlColor = (color > 25) ? (color + 227) : (color * 10);
        busSetCommand(WRITE_FAST_SET, FAST_BACKL_COLOR);
      }

      if (ui.clickInt("setsIndiBrtDay", mainSettings.indiBrightDay)) {
        busSetCommand(WRITE_MAIN_SET, MAIN_INDI_BRIGHT_D);
      }
      if (ui.clickInt("setsIndiBrtNight", mainSettings.indiBrightNight)) {
        busSetCommand(WRITE_MAIN_SET, MAIN_INDI_BRIGHT_N);
      }

      if (ui.clickInt("setsBurnFlip", mainSettings.burnMode)) {
        busSetCommand(WRITE_MAIN_SET, MAIN_BURN_MODE);
      }
      if (ui.clickInt("setsBurnTime", mainSettings.burnTime)) {
        busSetCommand(WRITE_MAIN_SET, MAIN_BURN_TIME);
      }

      if (ui.click("setsAutoShow")) {
        if (ui.getBool("setsAutoShow")) mainSettings.autoShowTime &= 0x7F;
        else mainSettings.autoShowTime |= 0x80;
        busSetCommand(WRITE_MAIN_SET, MAIN_AUTO_SHOW_TIME);
      }
      if (ui.click("setsAutoShowTime")) {
        if (mainSettings.autoShowTime & 0x80) {
          mainSettings.autoShowTime = ui.getInt("setsAutoShowTime") | 0x80;
        }
        else {
          mainSettings.autoShowTime = ui.getInt("setsAutoShowTime");
          busSetCommand(WRITE_MAIN_SET, MAIN_AUTO_SHOW_TIME);
        }
      }
      if (ui.clickInt("setsAutoShowFlip", mainSettings.autoShowFlip)) {
        busSetCommand(WRITE_MAIN_SET, MAIN_AUTO_SHOW_FLIP);
      }
      if (ui.click("setsTempCorrect")) {
        mainSettings.tempCorrect = constrain((int16_t)(ui.getFloat("setsTempCorrect") * 10), -127, 127);
        busSetCommand(WRITE_MAIN_SET, MAIN_TEMP_CORRECT);
      }

      if (ui.clickInt("setsTimeBrightS", mainSettings.timeBrightStart)) {
        busSetCommand(WRITE_MAIN_SET, MAIN_TIME_BRIGHT_S);
      }
      if (ui.clickInt("setsTimeBrightE", mainSettings.timeBrightEnd)) {
        busSetCommand(WRITE_MAIN_SET, MAIN_TIME_BRIGHT_E);
      }

      if (ui.clickInt("setsHourSoundS", mainSettings.timeHourStart)) {
        busSetCommand(WRITE_MAIN_SET, MAIN_TIME_HOUR_S);
      }
      if (ui.clickInt("setsHourSoundE", mainSettings.timeHourEnd)) {
        busSetCommand(WRITE_MAIN_SET, MAIN_TIME_HOUR_E);
      }

      if (ui.clickInt("setsSleepD", mainSettings.timeSleepDay)) {
        busSetCommand(WRITE_MAIN_SET, MAIN_TIME_SLEEP_D);
      }
      if (ui.clickInt("setsSleepN", mainSettings.timeSleepNight)) {
        busSetCommand(WRITE_MAIN_SET, MAIN_TIME_SLEEP_N);
      }

      if (ui.click("setsDotBrtDay")) {
        mainSettings.dotBrightDay = constrain(ui.getInt("setsDotBrtDay"), 1, 25) * 10;
        busSetCommand(WRITE_MAIN_SET, MAIN_DOT_BRIGHT_D);
      }
      if (ui.click("setsDotBrtNight")) {
        mainSettings.dotBrightNight = constrain(ui.getInt("setsDotBrtNight"), 0, 25);
        if (deviceInformation[NEON_DOT] < 3) mainSettings.dotBrightNight *= 10;
        busSetCommand(WRITE_MAIN_SET, MAIN_DOT_BRIGHT_N);
      }

      if (ui.click("setsBacklBrightDay")) {
        mainSettings.backlBrightDay = constrain(ui.getInt("setsBacklBrightDay"), 1, 25) * 10;
        busSetCommand(WRITE_MAIN_SET, MAIN_BACKL_BRIGHT_D);
      }
      if (ui.click("setsBacklBrightNight")) {
        mainSettings.backlBrightNight = constrain(ui.getInt("setsBacklBrightNight"), 0, 25) * 10;
        busSetCommand(WRITE_MAIN_SET, MAIN_BACKL_BRIGHT_N);
      }

      if (ui.click("setsTick")) {
        mainSettings.baseSound = (ui.getBool("setsTick")) ? 2 : 1;
        busSetCommand(WRITE_MAIN_SET, MAIN_BASE_SOUND);
      }
      if (ui.click("setsSound")) {
        mainSettings.baseSound = (ui.getBool("setsSound")) ? 1 : 0;
        busSetCommand(WRITE_MAIN_SET, MAIN_BASE_SOUND);
      }
      if (ui.clickInt("setsSoundVol", mainSettings.volumeSound)) {
        busSetCommand(WRITE_MAIN_SET, MAIN_VOLUME_SOUND);
        busSetCommand(WRITE_TEST_MAIN_VOL);
      }
      if (ui.clickInt("setsVoice", mainSettings.voiceSound)) {
        busSetCommand(WRITE_MAIN_SET, MAIN_VOICE_SOUND);
        busSetCommand(WRITE_TEST_MAIN_VOICE);
      }

      if (ui.click("setsHourTemp")) {
        if (ui.getBool("setsHourTemp")) mainSettings.hourSound |= 0x80;
        else mainSettings.hourSound &= ~0x80;
        busSetCommand(WRITE_MAIN_SET, MAIN_HOUR_SOUND);
      }
      if (ui.click("setsHourSound")) {
        mainSettings.hourSound = (mainSettings.hourSound & 0x80) | constrain(ui.getInt("setsHourSound"), 0, 3);
        busSetCommand(WRITE_MAIN_SET, MAIN_HOUR_SOUND);
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("ext")) {
      if (ui.clickSub("extShowMode")) {
        uint8_t pos = ui.clickNameSub(1).toInt();
        uint8_t mode = ui.getInt(String("extShowMode/") + pos);
        extendedSettings.autoShowModes[pos] = mode;
        busSetCommand(WRITE_EXTENDED_SHOW_MODE, pos);
      }
      if (ui.clickSub("extShowTime")) {
        uint8_t pos = ui.clickNameSub(1).toInt();
        uint8_t time = ui.getInt(String("extShowTime/") + pos);
        extendedSettings.autoShowTimes[pos] = time;
        busSetCommand(WRITE_EXTENDED_SHOW_TIME, pos);
      }

      if (ui.clickSub("extAlarm")) {
        if (ui.click("extAlarmTimeout")) {
          extendedSettings.alarmTime = ui.getInt("extAlarmTimeout");
          busSetCommand(WRITE_EXTENDED_ALARM, EXT_ALARM_TIMEOUT);
        }
        if (ui.click("extAlarmWaitTime")) {
          extendedSettings.alarmWaitTime = ui.getInt("extAlarmWaitTime");
          busSetCommand(WRITE_EXTENDED_ALARM, EXT_ALARM_WAIT);
        }
        if (ui.click("extAlarmSoundTime")) {
          extendedSettings.alarmSoundTime = ui.getInt("extAlarmSoundTime");
          busSetCommand(WRITE_EXTENDED_ALARM, EXT_ALARM_TIMEOUT_SOUND);
        }
        if (ui.click("extAlarmDotOn")) {
          extendedSettings.alarmDotOn = ui.getInt("extAlarmDotOn");
          busSetCommand(WRITE_EXTENDED_ALARM, EXT_ALARM_DOT_ON);
        }
        if (ui.click("extAlarmDotWait")) {
          extendedSettings.alarmDotWait = ui.getInt("extAlarmDotWait");
          busSetCommand(WRITE_EXTENDED_ALARM, EXT_ALARM_DOT_WAIT);
        }
      }

      if (ui.clickSub("extDevice")) {
        if (ui.click("extDeviceName")) {
          String _name;
          _name.reserve(20);
          _name = ui.getString("extDeviceName");

          _name.replace(",", ""); //удаляем лишние символы
          _name.replace(":", ""); //удаляем лишние символы
          _name.trim(); //удаляем пробелы

          strncpy(settings.nameDevice, _name.c_str(), 20); //копируем себе
          settings.nameDevice[19] = '\0'; //устанавливаем последний символ

          if (groupGetSearchStatus()) { //если сервис обнаружения устройств запущен
            groupSearch(); //запустить поиск устройств поблизости
            groupReload(); //обновить состояние для устройств поблизости
          }

          memorySaveSettings(); //обновить данные в памяти
        }
        if (ui.clickBool("extDeviceAp", settings.nameAp)) {
          memorySaveSettings(); //обновить данные в памяти
        }
        if (ui.clickBool("extDeviceMenu", settings.nameMenu)) {
          memorySaveSettings(); //обновить данные в памяти
        }
        if (ui.clickBool("extDevicePrefix", settings.namePrefix)) {
          memorySaveSettings(); //обновить данные в памяти
        }
        if (ui.clickBool("extDevicePostfix", settings.namePostfix)) {
          memorySaveSettings(); //обновить данные в памяти
        }

        if (ui.clickBool("extDeviceGroup", settings.groupFind)) {
          if (settings.groupFind) groupStart(wifiGetConnectStatus()); //запустили обнаружение устройств поблизости
          else groupLocal(); //остановить обнаружение устройств поблизости
          memorySaveSettings(); //обновить данные в памяти
        }
      }

      if (ui.click("extClear")) {
        settings.wifiSSID[0] = '\0'; //устанавливаем последний символ
        settings.wifiPASS[0] = '\0'; //устанавливаем последний символ
        memorySaveSettings(); //обновить данные в памяти
      }
      if (ui.click("extScan")) {
        if (wifiGetScanAllowStatus()) wifiStartScanNetworks(); //начинаем поиск
      }

      if (ui.click("extOtaCheck")) {
        passCheckAttempt(ui.getString("extOtaCheck"));
      }

      if (ui.clickSub("extMainTab")) {
        navMainTab = constrain(ui.clickNameSub(1).toInt(), 0, 1);
      }
      if (ui.clickSub("extInfoTab")) {
        navInfoTab = constrain(ui.clickNameSub(1).toInt(), 0, 1);
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("timer")) {
      if (!timer.mode || (timer.mode == 0x82)) {
        if (ui.clickSub("timerHour")) {
          switch (ui.clickNameSub(1).toInt()) {
            case 0: if (timer.hour < 17) timer.hour++; else timer.hour = 0; break; //час прибавить
            case 1: if (timer.mins < 59) timer.mins++; else timer.mins = 0; break; //минута прибавить
            case 2: if (timer.secs < 59) timer.secs++; else timer.secs = 0; break; //секунда прибавить
            case 3: if (timer.hour > 0) timer.hour--; else timer.hour = 17; break; //час убавить
            case 4: if (timer.mins > 0) timer.mins--; else timer.mins = 59; break; //минута убавить
            case 5: if (timer.secs > 0) timer.secs--; else timer.secs = 59; break; //секунда убавить
          }
        }
      }
      if (ui.clickSub("timerControl")) {
        switch (ui.clickNameSub(1).toInt()) {
          case 0: //запуск
            if (!timer.mode) {
              timer.count = (timer.hour * 3600) + (timer.mins * 60) + timer.secs;
              if (timer.count) {
                timer.mode = 2;
                if (timer.count) timer.time = timer.count;
                busSetCommand(WRITE_TIMER_SET);
              }
              else timer.mode = 1;
              busSetCommand(WRITE_TIMER_TIME);
            }
            else if (timer.mode & 0x80) {
              timer.mode &= 0x7F;
              busSetCommand(WRITE_TIMER_STATE);
            }
            busSetCommand(WRITE_TIMER_MODE);
            break;
          case 1: //остановка
            if (timer.mode) {
              timer.mode = 0;
              timer.count = 0;
              busSetCommand(WRITE_TIMER_TIME);
              busSetCommand(SET_UPDATE);
            }
            else {
              timer.hour = timer.mins = timer.secs = 0;
            }
            break;
          case 2: //пауза
            if (timer.mode && !(timer.mode & 0x80)) {
              timer.mode |= 0x80;
              busSetCommand(WRITE_TIMER_STATE);
            }
            break;
        }
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("climate")) {
      if (ui.clickInt("climateMainSens", extendedSettings.tempMainSensor)) {
        busSetCommand(WRITE_EXTENDED_SENSOR_SET, EXT_SENSOR_MAIN);
      }
      if (ui.clickInt("climateHourSens", extendedSettings.tempHourSensor)) {
        busSetCommand(WRITE_EXTENDED_SENSOR_SET, EXT_SENSOR_HOUR);
      }
      if (ui.clickInt("climateCorrectType", extendedSettings.tempCorrectSensor)) {
        busSetCommand(WRITE_EXTENDED_SENSOR_SET, EXT_SENSOR_CORRECT);
      }

      if (ui.clickInt("climateBar", settings.climateBar)) {
        memorySaveSettings(); //обновить данные в памяти
      }
      if (ui.click("climateChart")) {
        sensorChart = ui.getInt("climateChart");
      }
      if (ui.clickSub("climateSend")) {
        int num = ui.clickNameSub(1).toInt();
        settings.climateSend[constrain(num, 0, 1)] = ui.getInt(String("climateSend/") + num) + 1;
        sensorSendData(settings.climateSend[constrain(num, 0, 1)]); //отправить данные
        memorySaveSettings(); //обновить данные в памяти
      }

      if (ui.clickInt("climateTime", settings.climateTime)) {
        memorySaveSettings(); //обновить данные в памяти
      }
      if (ui.clickBool("climateAvg", settings.climateAvg)) {
        memorySaveSettings(); //обновить данные в памяти
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("radio")) {
      if (ui.click("radioPower")) {
        radioSettings.powerState = ui.getBool("radioPower");
        busSetCommand(WRITE_RADIO_POWER);
      }
      if (radioSettings.powerState) {
        if (ui.click("radioMode")) {
          busSetCommand(WRITE_RADIO_MODE);
        }
        if (ui.click("radioSeekDown")) {
          busSetCommand(RADIO_SEEK_DOWN);
        }
        if (ui.click("radioSeekUp")) {
          busSetCommand(RADIO_SEEK_UP);
        }
        if (ui.click("radioFreqDown")) {
          busSetCommand(READ_RADIO_FREQ);
          busSetCommand(RADIO_FREQ_DOWN);
        }
        if (ui.click("radioFreqUp")) {
          busSetCommand(READ_RADIO_FREQ);
          busSetCommand(RADIO_FREQ_UP);
        }
        if (ui.click("radioVol")) {
          radioSettings.volume = constrain(ui.getInt("radioVol"), 0, 15);
          busSetCommand(WRITE_RADIO_VOL);
        }
        if (ui.click("radioFreq")) {
          uint16_t stationFreq = (uint16_t)(ui.getFloat("radioFreq") * 10);
          radioSettings.stationsFreq = constrain(stationFreq, 870, 1080);
          busSetCommand(WRITE_RADIO_FREQ);
        }
      }
      if (ui.clickSub("radioCh")) {
        uint8_t stationNum = constrain(ui.clickNameSub(1).toInt(), 0, 9);
        if (radioSettings.stationsSave[stationNum]) {
          radioSettings.stationsFreq = radioSettings.stationsSave[stationNum];
          if (!radioSettings.powerState) {
            radioSettings.powerState = true;
            busSetCommand(WRITE_RADIO_POWER);
          }
          busSetCommand(WRITE_RADIO_FREQ);
        }
      }
      if (ui.clickSub("radioSta")) {
        uint8_t stationNum = constrain(ui.clickNameSub(1).toInt(), 0, 9);
        uint16_t stationFreq = (uint16_t)(ui.getFloat(String("radioSta/") + stationNum) * 10);
        stationFreq = (stationFreq) ? constrain(stationFreq, 870, 1080) : 0;
        if (radioSettings.stationsSave[stationNum] != stationFreq) {
          radioSettings.stationsSave[stationNum] = stationFreq;
          busSetCommand(WRITE_RADIO_STA, stationNum);
        }
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("alert")) {
      if (ui.click("alertResetOk")) {
        memoryResetSettings(); //устанавливаем настройки по умолчанию
        memoryWriteSettings(); //записать данные в память
        busRebootDevice(DEVICE_RESET);
      }
      if (ui.click("alertRebootOk")) {
        memoryWriteSettings(); //записать данные в память
        busRebootDevice(DEVICE_REBOOT);
      }

      if (ui.clickSub("alertClimate")) {
        if (ui.click("alertClimateOk")) {
          settings.climateChart = sensorChart;
          climateUpdate(CLIMATE_RESET);
          memorySaveSettings(); //обновить данные в памяти
        }
        sensorChart = 0;
      }

      if (ui.clickSub("alertFound")) {
        if (ui.click("alertFoundOk")) {
          wirelessSetNewId();
          wirelessSetData(wireless_found_buffer);
          memorySaveSettings(); //обновить данные в памяти
        }
        else wirelessResetFoundState();
      }
    }
  }
  /**************************************************************************/
  if (ui.form()) {
    if (!wifiGetConnectWaitStatus() && !wifiGetConnectStatus()) {
      if (ui.form("/connection")) {
        wifiSetConnectStatus(true); //устанавливаем интервал переподключения
        String _ssid; //временная строка ssid сети
        _ssid.reserve(64); //резервируем всю длинну
        if (ui.copyString("wifiSsid", _ssid)) { //копируем ssid сети
          _ssid.replace(LANG_WIFI_SSID_LOCK, ""); //удаляем лишние символы
          _ssid.trim(); //удаляем пробелы
          strncpy(settings.wifiSSID, _ssid.c_str(), 64); //копируем ssid сети
        }
        else wifiSetConnectStatus(false); //сбрасываем интервал переподключения
        settings.wifiSSID[63] = '\0'; //устанавливаем последний символ
        ui.copyStr("wifiPass", settings.wifiPASS, 64); //копируем пароль сети
        settings.wifiPASS[63] = '\0'; //устанавливаем последний символ
        memorySaveSettings(); //обновить данные в памяти
      }
    }
    else if (ui.form("/network")) {
      wifiResetConnectStatus(); //отключаемся от точки доступа
      settings.wifiSSID[0] = '\0'; //устанавливаем последний символ
      settings.wifiPASS[0] = '\0'; //устанавливаем последний символ
      memorySaveSettings(); //обновить данные в памяти
    }
  }
  /**************************************************************************/
  if (ui.update()) {
    if (ui.updateSub("bar")) {
      if (ui.update("barTime")) { //если было обновление
        ui.answer(encodeTime(mainTime));
        waitTimer = 0; //установили таймер ожидания
      }

      if (ui.update("barTemp")) { //если было обновление
        ui.answer(climateGetBarTempStr());
      }
      if (ui.update("barHum")) { //если было обновление
        ui.answer(climateGetBarHumStr());
      }
      if (ui.update("barPress")) { //если было обновление
        ui.answer(climateGetBarPressStr());
      }

      if (ui.update("barLink")) { //если было обновление
        ui.answer(busGetClockStatus());
      }
      if (ui.update("barSens")) { //если было обновление
        ui.answer(wirelessGetOnlineStastus());
      }
      if (ui.update("barRtc")) { //если было обновление
        ui.answer(rtcGetNormalStatus());
      }
      if (ui.update("barNtp")) { //если было обновление
        ui.answer(ntpGetSyncStatus());
      }

      if (ui.update("barWifi")) { //если было обновление
        ui.answer(wifiGetSignalStrength());
      }
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("ext")) {
      if (ui.update("extUpdate")) { //если было обновление
        ui.answer((busRebootFail()) ? " " : ((!busRebootState()) ? getUpdaterState() : LANG_PAGE_RELOAD_WAIT));
      }
      if (ui.update("extReboot") && !busRebootState()) { //если было обновление
        ui.answer((busRebootFail()) ? " " : LANG_PAGE_RELOAD_END);
      }
      if (ui.update("extWarn") && !updaterState() && !busRebootState()) { //если было обновление
        ui.answer((busRebootFail()) ? LANG_PAGE_RELOAD_ERR : " ");
      }

      if (ui.update("extNtp")) { //если было обновление
        ui.answer(ntpGetState());
      }
      if (ui.update("extWeather")) { //если было обновление
        ui.answer(weatherGetState());
      }

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

      if (ui.update("extAlarm") && (alarm.reload >= 2)) { //если было обновление
        ui.answer(1);
        alarm.reload = 0;
      }
      if (ui.update("extGroup") && groupGetUpdateStatus()) { //если было обновление
        ui.answer(groupGetList());
      }
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("inf")) {
      if (ui.update("infUsage")) { //если было обновление
        ui.answer(stringGetPercent(systemGetUsage()));
      }
      if (ui.update("infUptime")) { //если было обновление
        ui.answer(stringGetTimeFromMs(millis()));
      }
      if (ui.update("infFrag")) { //если было обновление
        ui.answer(stringGetPercent(ESP.getHeapFragmentation()));
      }
      if (ui.update("infFree")) { //если было обновление
        ui.answer(stringGetKilobyte(ESP.getFreeHeap(), 3));
      }
      if (ui.update("infSignal")) { //если было обновление
        ui.answer(stringGetPercent(wifiGetSignalStrength()));
      }
      if (ui.update("infNettime")) { //если было обновление
        ui.answer(stringGetTimeFromMs(wifiGetConnectTime()));
      }
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("timer")) {
      if (ui.update("timerState")) { //если было обновление
        ui.answer(getTimerState());
        if (!timer.mode) busSetCommand(READ_TIMER_STATE);
      }
      if (ui.update("timerTime")) { //если было обновление
        ui.answer(convertTimerTime());
      }
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("radio")) {
      if (ui.update("radioVol")) { //если было обновление
        ui.answer(constrain((int8_t)radioSettings.volume, 0, 15));
      }
      if (ui.update("radioFreq")) { //если было обновление
        ui.answer(constrain(radioSettings.stationsFreq, 870, 1080) / 10.0, 1);
      }
      if (ui.update("radioPower")) { //если было обновление
        ui.answer(radioSettings.powerState);
        busSetCommand(READ_RADIO_POWER);
      }
      if (ui.updateSub("radioSta")) {
        uint8_t stationNum = constrain(ui.updateNameSub(1).toInt(), 0, 9);
        ui.answer((radioSettings.stationsSave[stationNum]) ? String(radioSettings.stationsSave[stationNum] / 10.0, 1) : " ");
      }
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("alert")) {
      if (ui.update("alertReset")) { //если было обновление
        ui.answer(1);
      }
      if (ui.update("alertReboot")) { //если было обновление
        ui.answer(1);
      }

      if (ui.update("alertClimate")) { //если было обновление
        ui.answer(1);
      }

      if (ui.update("alertFound") && wirelessGetFoundState()) { //если было обновление
        ui.answer(1);
      }
      if (ui.update("alertFoundText") && wirelessGetFoundState()) { //если было обновление
        ui.answer("UID: " + wirelessGetId(wireless_found_buffer));
      }
    }
  }
  /**************************************************************************/
  if (ui.upload()) {
    updaterSetStatus(UPDATER_UPL_ABORT); //установили флаг ошибки загрузки файла
    ui.saveFile(LittleFS.open("/update/firmware.hex", "w"));
  }
  if (ui.uploadEnd()) {
    if (ui.fileName().endsWith(".hex") || ui.fileName().endsWith(".HEX")) {
      updaterSetIdle(); //сбросили флаг ошибки
      Serial.println("Updater load file: " + ui.fileName());
      if (deviceInformation[HARDWARE_VERSION]) busSetCommand(UPDATE_FIRMWARE);
      else updaterStart(); //запуск обновления
    }
    else {
      updaterSetStatus(UPDATER_NOT_HEX); //установили флаг ошибки расширения
      LittleFS.remove("/update/firmware.hex"); //удаляем файл
      Serial.println("Updater file extension error " + ui.fileName());
    }
  }
  if (ui.uploadAbort()) {
    updaterSetStatus(UPDATER_UPL_ABORT); //установили флаг ошибки загрузки файла
    LittleFS.remove("/update/firmware.hex"); //удаляем файл
    Serial.println F("Updater file upload abort");
  }
}
//--------------------------------------------------------------------
void sensorSendData(uint8_t sens) {
  if (!deviceInformation[SENS_TEMP] && (settings.climateSend[0] == sens)) busSetCommand(WRITE_SENS_1_DATA, settings.climateSend[0]); //отправить данные
  if (settings.climateSend[1] == sens) busSetCommand(WRITE_SENS_2_DATA, settings.climateSend[1]); //отправить данные
  if (settings.climateChart == sens) climateUpdate(CLIMATE_UPDATE); //обновляем показания графиков
}
//--------------------------------------------------------------------
void sensorUpdateData(void) {
  if (deviceInformation[SENS_TEMP]) busSetCommand(WRITE_CHECK_SENS);
  else sens.update |= SENS_EXT;
  sens.status = 0;
}
//--------------------------------------------------------------------
void sensorInitData(void) {
  static boolean sens_init = false;
  if (!sens_init) {
    if (deviceInformation[SENS_TEMP]) settings.climateSend[0] = SENS_CLOCK; //установить сенсор в часах
    else if (settings.climateSend[0] == SENS_CLOCK) settings.climateSend[0] = SENS_MAIN; //иначе сенсор в есп
    sens.search = sens.status; //установить показания датчиков
    sens_init = true;
  }
  else sens.search |= 0x80;
}
//--------------------------------------------------------------------
boolean sensorAvaibleData(void) {
  return (boolean)((sens.search & 0x7F) || !sens.search);
}
//--------------------------------------------------------------------
boolean sensorGetValidStatus(void) {
  return (boolean)(climateAvailableTemp(settings.climateChart) || ((settings.climateChart == SENS_WIRELESS) && wirelessGetSensorStastus()));
}
//--------------------------------------------------------------------
boolean sensorGetDisabledStatus(void) {
  return (boolean)(!climateAvailableTemp(settings.climateSend[0]) && !climateAvailableTemp(settings.climateSend[1]));
}
//--------------------------------------------------------------------
void weatherAveragData(void) {
  int8_t time_diff = mainTime.hour - ((weatherDates[0] % 86400UL) / 3600UL);
  if (time_diff < 0) time_diff += 24;
  uint8_t time_now = constrain(time_diff, 0, 23);
  uint8_t time_next = constrain(time_now + 1, 0, 23);

  if (timeState != 0x03) time_now = time_next = 0; //если нет актуального времени

  uint8_t light_now = weatherArrDay[0][time_now];
  if (light_now != weatherArrDay[0][time_next]) light_now = 1;
  else if (light_now) light_now = 0;
  else light_now = 2;

  if (!weatherGetGoodStatus() && (time_now > 12)) {
    weatherResetValidStatus(); //сбросили статус погоды
    light_now = 0; //сбросили текущую яркость
    sens.temp[SENS_WEATHER] = 0x7FFF; //сбросили температуру погоды
    sens.hum[SENS_WEATHER] = 0; //сбросили влажность погоды
    sens.press[SENS_WEATHER] = 0; //сбросили давление погоды
  }
  else {
    sens.temp[SENS_WEATHER] = map(mainTime.minute, 0, 59, weatherArrMain[0][time_now], weatherArrMain[0][time_next]); //температура погоды
    sens.hum[SENS_WEATHER] = map(mainTime.minute, 0, 59, weatherArrMain[1][time_now], weatherArrMain[1][time_next]); //влажность погоды
    sens.press[SENS_WEATHER] = map(mainTime.minute, 0, 59, weatherArrExt[0][time_now], weatherArrExt[0][time_next]) / 10; //давление погоды
  }

  if (!deviceInformation[LIGHT_SENS_ENABLE]) { //если сенсор яркости освещения не используется
    if (device.light != light_now) { //если яркость изменилась
      device.light = light_now; //установили состояние яркости
      busSetCommand(WRITE_CHANGE_BRIGHT); //оправка состояния яркости
    }
  }

  sensorSendData(SENS_WEATHER); //отправить данные
}
//--------------------------------------------------------------------
void timeUpdate(void) {
  if ((millis() - secondsTimer) >= 1000) { //если прошла секунда
    if (!secondsTimer) secondsTimer = millis(); //инициализировали таймер
    else { //счет времени
      if (++mainTime.second > 59) { //секунды
        mainTime.second = 0; //сбросили секунды
        if (++mainTime.minute > 59) { //минуты
          mainTime.minute = 0; //сбросили минуты
          if (++mainTime.hour > 23) { //часы
            mainTime.hour = 0; //сбросили часы
            if (++mainDate.day > maxDays(mainDate.year, mainDate.month)) { //дата
              mainDate.day = 1; //сбросили день
              if (++mainDate.month > 12) { //месяц
                mainDate.month = 1; //сбросили месяц
                if (++mainDate.year > 2099) { //год
                  mainDate.year = 2000; //сбросили год
                }
              }
            }
          }
          if (weatherGetRunStatus()) weatherCheck(); //запросить прогноз погоды
        }
        if (weatherGetValidStatus()) weatherAveragData(); //усреднить показания погоды

        if (!(mainTime.minute % 15) && rtcGetFoundStatus() && (!settings.ntpSync || !ntpGetSyncStatus())) busSetCommand(READ_RTC_TIME); //отправить время в RTC
        else busSetCommand(READ_TIME_DATE, 0); //прочитали время из часов

        if (settings.ntpSync) { //если включена автосинхронизация
          if (!settings.ntpDst) { //если выключен учет летнего времени
            if (!syncTimer) { //если пришло время отправить новый запрос
              syncTimer = ntpSyncTime[settings.ntpTime]; //сбросили таймер синхронизации
              ntpRequest(); //запросить время с ntp сервера
            }
            else syncTimer--; //иначе убавили таймер
          }
          else { //иначе запрос не реже чем раз в час
            if (!(mainTime.minute % ntpSyncTime[(settings.ntpDst && (settings.ntpTime > 2)) ? 2 : settings.ntpTime])) {
              ntpRequest(); //запросить время с ntp сервера
            }
          }
        }
      }
      if (sensorAvaibleData()) { //если датчик температуры доступен
        if (!sensorTimer) { //если пришло время обновить показания
          sensorTimer = 59; //сбросили таймер
          sensorUpdateData(); //обновили показания датчика
        }
        else sensorTimer--; //иначе убавили таймер
      }
      secondsTimer += 1000; //прибавили секунду
    }
    if (timer.mode) busSetCommand(READ_TIMER_TIME); //отправляем запрос оновления показаний таймера
    if (!waitTimer) { //если пришло время опросить статус часов
      waitTimer = 4; //установили таймер ожидания
      busUpdateClockStatus(); //обновить статус часов
      busSetCommand(READ_STATUS); //запрос статуса часов
    }
    else waitTimer--; //иначе убавили таймер
    if (playbackTimer > -1) { //если запущено воспроизведение
      if (!playbackTimer) busSetCommand(WRITE_STOP_SOUND); //остановка воспроизведения
      playbackTimer--; //убавили таймер воспроизведения
    }
#if STATUS_LED == 2
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); //мигаем индикацией
#endif
  }

  if (ntpUpdate()) { //обработка ntp
    if (settings.ntpSync || !syncState) { //если включена автосинхронизация или необходимо обновить немедленно
      syncTimer = ntpSyncTime[settings.ntpTime]; //сбросили таймер синхронизации
      busSetCommand(SYNC_TIME_DATE); //проверить и отправить время ntp сервера
    }
  }

  if (weatherUpdate()) {
    weatherGetUnixData(weatherDates, WEATHER_BUFFER);
    weatherGetParseData(weatherArrMain[0], WEATHER_GET_TEMP, WEATHER_BUFFER);
    weatherGetParseData(weatherArrMain[1], WEATHER_GET_HUM, WEATHER_BUFFER);
    weatherGetParseData(weatherArrExt[0], WEATHER_GET_PRESS, WEATHER_BUFFER);
    weatherGetParseData(weatherArrDay[0], WEATHER_GET_DAY, WEATHER_BUFFER);
    if (weatherGetValidStatus()) weatherAveragData();
  }
}
//--------------------------------------------------------------------
void deviceUpdate(void) {
  if (device.status) { //если статус обновился
    for (uint8_t i = 0; i < STATUS_MAX_DATA; i++) { //проверяем все флаги
      if (device.status & 0x01) { //если флаг установлен
        switch (i) { //выбираем действие
          case STATUS_UPDATE_MAIN_SET: busSetCommand(READ_MAIN_SET); break;
          case STATUS_UPDATE_FAST_SET: busSetCommand(READ_FAST_SET); break;
          case STATUS_UPDATE_RADIO_SET: busSetCommand(READ_RADIO_SET); break;
          case STATUS_UPDATE_ALARM_SET: busSetCommand(READ_ALARM_ALL); break;
          case STATUS_UPDATE_TIME_SET: busSetCommand(READ_TIME_DATE, 1); break;
          case STATUS_UPDATE_SENS_DATA:
            if (deviceInformation[SENS_TEMP]) {
              busSetCommand(READ_SENS_INFO);
              busSetCommand(READ_SENS_DATA);
            }
            break;
        }
      }
      device.status >>= 1; //сместили буфер флагов
    }
    device.status = 0; //сбрасываем все флаги
  }

  switch (sens.update) { //если все датчики опрошены
    case SENS_EXT:
      sens.update = 0; //сбрасываем флаги опроса
      busSetCommand(CHECK_INTERNAL_AHT);
      busSetCommand(CHECK_INTERNAL_SHT);
      busSetCommand(CHECK_INTERNAL_BME);
      break;
    case (SENS_AHT | SENS_SHT | SENS_BME):
      sens.update = 0; //сбрасываем флаги опроса
      sensorInitData(); //инициализация датчиков
      sensorSendData(SENS_CLOCK); //отправить данные
      sensorSendData(SENS_MAIN); //отправить данные
      break;
  }
}
//--------------------------------------------------------------------
boolean passGetCheckError(void) {
  if (passState == 0x01) {
    passState = 0x00;
    return true;
  }
  else if (passState == 0x03) {
    passState = 0x02;
    return true;
  }
  return false;
}
//--------------------------------------------------------------------
boolean passGetWriteTimeout(void) {
  return ((passState & 0x02) && ((millis() - passTimer) < OTA_PASS_TIMEOUT));
}
//--------------------------------------------------------------------
boolean passGetOtaState(void) {
  if (OTA_PASS[0] == '\0') passSetOtaState();
  return (passState >= 0xFC);
}
//--------------------------------------------------------------------
void passSetOtaState(void) {
  static boolean auth = false;

  if (!auth) {
    auth = true;
    ui.uploadMode(true);
    ui.uploadAuto(false);
    ui.enableOTA();
    ui.OTA.attachUpdateBuild(buildUpdate);
  }

  passState = 0xFC;
}
//--------------------------------------------------------------------
void passCheckAttempt(const String& pass) {
  static uint8_t attempt = 0;

  if (((millis() - passTimer) >= OTA_PASS_TIMEOUT) || (attempt < OTA_PASS_ATTEMPT)) {
    if (pass.compareTo(OTA_PASS)) {
      if (attempt++ >= OTA_PASS_ATTEMPT) attempt = 0;
      else if (attempt == OTA_PASS_ATTEMPT) passState = 0x03;
      else passState = 0x01;
    }
    else passSetOtaState();
    passTimer = millis();
  }
}
//--------------------------------------------------------------------
boolean checkFileSystemData(const char** data, int8_t size) {
  File file;
  while (size > 0) {
    size--;
    file = LittleFS.open(data[size], "r");
    if (!file) {
      Serial.print(data[size]);
      Serial.println F(" not found");
      return false;
    }
    else file.close();
  }
  return true;
}
//--------------------------------------------------------------------
void initFileSystemData(void) {
  if (!LittleFS.begin()) Serial.println F("File system error");
  else {
    fsUpdate = true; //включаем обновление
    Serial.println F("File system init");

    if (checkFileSystemData(climateFsData, 1)) {
      climateLocal = true; //работаем локально
      Serial.println F("Script file found");
    }
    if (checkFileSystemData(alarmFsData, 3)) {
      alarmSvgImage = true; //работаем локально
      Serial.println F("Alarm svg files found");
    }
    if (checkFileSystemData(timerFsData, 5)) {
      timerSvgImage = true; //работаем локально
      Serial.println F("Timer svg files found");
    }
    if (checkFileSystemData(radioFsData, 6)) {
      radioSvgImage = true; //работаем локально
      Serial.println F("Radio svg files found");
    }

    if (LittleFS.remove("/update/firmware.hex")) Serial.println F("Clock update file remove"); //удаляем файл прошивки

    FSInfo fs_info;
    LittleFS.info(fs_info);
    if ((fs_info.totalBytes - fs_info.usedBytes) >= 120000) {
      clockUpdate = true; //включаем обновление
      Serial.println F("Clock update enable");
    }
    else Serial.println F("Clock update disable, running out of memory");
  }

  if (ESP.getFreeSketchSpace() >= ESP.getSketchSize()) {
    otaUpdate = true; //выключаем обновление
    Serial.println F("OTA update enable");
  }
  else Serial.println F("OTA update disable, running out of memory");

  if (OTA_PASS[0] != '\0') {
    Serial.print F("Update is locked, pass: ");
    Serial.println(OTA_PASS);
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
void memoryResetSettings(void) {
  strncpy(settings.ntpHost, DEFAULT_NTP_HOST, 20); //установить хост по умолчанию
  settings.ntpHost[19] = '\0'; //устанавливаем последний символ

  settings.groupFind = DEFAULT_GROUP_FOUND; //обнаружение устройств поблизости

  settings.nameAp = DEFAULT_NAME_AP; //установить отображение имени после названия точки доступа wifi по умолчанию
  settings.nameMenu = DEFAULT_NAME_MENU; //установить отображение имени в меню по умолчанию
  settings.namePrefix = DEFAULT_NAME_PREFIX; //установить отображение имени перед названием вкладки по умолчанию
  settings.namePostfix = DEFAULT_NAME_POSTFIX; //установить отображение имени после названием вкладки по умолчанию

  strncpy(settings.nameDevice, DEFAULT_NAME, 20); //установить имя по умолчанию
  settings.nameDevice[19] = '\0'; //устанавливаем последний символ

  settings.weatherCity = DEFAULT_WEATHER_CITY; //установить город по умолчанию
  settings.weatherLat = NAN; //установить широту по умолчанию
  settings.weatherLon = NAN; //установить долготу по умолчанию

  for (uint8_t i = 0; i < sizeof(settings.wirelessId); i++) settings.wirelessId[i] = 0; //сбрасываем id беспроводного датчика
  settings.climateSend[0] = SENS_MAIN; //сбрасываем тип датчика
  settings.climateSend[1] = SENS_WEATHER; //сбрасываем тип датчика
  settings.climateBar = SENS_MAIN; //установить режим по умолчанию
  settings.climateChart = SENS_MAIN; //установить режим по умолчанию
  settings.climateTime = DEFAULT_CLIMATE_TIME; //установить период по умолчанию
  settings.climateAvg = DEFAULT_CLIMATE_AVG; //установить усреднение по умолчанию
  settings.ntpGMT = DEFAULT_GMT; //установить часовой по умолчанию
  settings.ntpSync = DEFAULT_SYNC; //выключаем авто-синхронизацию
  settings.ntpDst = DEFAULT_DST; //установить учет летнего времени по умолчанию
  settings.ntpTime = DEFAULT_NTP_TIME; //установить период по умолчанию
  if (settings.ntpTime > (sizeof(ntpSyncTime) - 1)) settings.ntpTime = sizeof(ntpSyncTime) - 1;
}
//--------------------------------------------------------------------
void setup() {
  //инициализация индикатора
#if STATUS_LED > 0
  pinMode(LED_BUILTIN, OUTPUT);
#if STATUS_LED < 2
  digitalWrite(LED_BUILTIN, HIGH);
#else
  digitalWrite(LED_BUILTIN, LOW);
#endif
#endif

  //инициализация шины
  pinMode(TWI_SDA_PIN, INPUT_PULLUP);
  pinMode(TWI_SCL_PIN, INPUT_PULLUP);
  twi_init();

  Serial.begin(115200);
  Serial.println F("");
  Serial.println F("Startup...");
  Serial.print F("Firmware version ");
  Serial.print F(ESP_FIRMWARE_VERSION);
  Serial.println F("...");

  //устанавливаем указатель будильниака
  alarm.now = 0;

  //восстанавливаем настройки сети
  wifiReadSettings();

  //устанавливаем настройки по умолчанию
  memoryResetSettings();

  //читаем настройки из памяти
  memoryReadSettings();

  //инициализация файловой системы
  initFileSystemData();

  //инициализируем строки
  groupInitStr();
  weatherInitStr();
  wifiScanInitStr();

  //настраиваем wifi
  wifiSetConnectMode();
  wifiStartAP();

  //подключаем конструктор и запускаем веб интерфейс
  ui.setBufferSize(1500);
  ui.attachBuild(build);
  ui.attach(action);
  ui.start();

  //настраиваем работу с файлами
  ui.downloadAuto(true);
  ui.uploadMode(false);

  //остановили ntp
  ntpStop();

  //запутили обнаружение устройств поблизости
  groupStart(false);

  //запутили поиск беспроводного датчика
  wirelessStart();

  //отключились от сервера погоды
  weatherDisconnect();

  //запрашиваем настройки часов
  busSetCommand(READ_MAIN_SET);
  busSetCommand(READ_FAST_SET);
  busSetCommand(READ_EXTENDED_SET);
  busSetCommand(READ_RADIO_SET);
  busSetCommand(READ_ALARM_ALL);
  busSetCommand(READ_TIME_DATE, 0);
  busSetCommand(READ_FAILURE);
  busSetCommand(READ_DEVICE);

  busSetCommand(WRITE_RTC_INIT);

  busTimerSetInterval(1500);
}
//--------------------------------------------------------------------
void loop() {
  wifiUpdate(); //обработка состояния wifi
  groupUpdate(); //обработка группового взаимодействия

  if (deviceInformation[HARDWARE_VERSION] == HW_VERSION) { //если связь с часами установлена
    timeUpdate(); //обработка времени
    deviceUpdate(); //обработка статусов устройства
  }

  if (wirelessUpdate()) sensorSendData(SENS_WIRELESS); //обработка беспроводного датчика

  if (!updaterFlash()) busUpdate(); //обработка шины
  else if (updaterRun()) busRebootDevice(SYSTEM_REBOOT); //загрузчик прошивки

  memoryUpdate(); //обработка памяти настроек
  systemUpdate(); //обработка системных функций
}
