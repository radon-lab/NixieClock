/*
  Arduino IDE 1.8.13 версия прошивки 1.2.5 релиз от 28.10.24
  Специльно для проекта "Часы на ГРИ v2. Альтернативная прошивка"
  Страница проекта - https://community.alexgyver.ru/threads/chasy-na-gri-v2-alternativnaja-proshivka.5843/

  Исходник - https://github.com/radon-lab/NixieClock
  Автор Radon-lab & Psyx86.

  Если не установлено ядро ESP8266, "Файл -> Настройки -> Дополнительные ссылки для Менеджера плат", в окно ввода вставляете ссылку - https://arduino.esp8266.com/stable/package_esp8266com_index.json
  Далее "Инструменты -> Плата -> Менеджер плат..." находите плату esp8266 и устанавливаете версию 2.7.4!

  В "Инструменты -> Управлять библиотеками..." необходимо предварительно установить указанные версии библиотек:
  GyverPortal 3.6.6
  EEManager 2.0.1

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

#define GP_NO_DNS
#define GP_NO_MDNS

#include <LittleFS.h>
#include <GyverPortal.h>
GyverPortal ui(&LittleFS);

struct settingsData {
  boolean nameAp;
  boolean nameMenu;
  boolean namePrefix;
  boolean namePostfix;
  uint8_t wirelessId[6];
  uint8_t weatherCity;
  float weatherLat;
  float weatherLon;
  uint8_t climateSend[2];
  uint8_t climateChart;
  uint8_t climateBar;
  uint8_t climateTime;
  boolean climateAvg;
  boolean ntpSync;
  boolean ntpDst;
  uint8_t ntpTime;
  int8_t ntpGMT;
  char host[20];
  char name[20];
  char ssid[64];
  char pass[64];
  char multi[MAX_CLOCK * 2][20];
} settings;

#include <EEManager.h>
EEManager memory(settings, 3000);

//переменные
GPdate mainDate; //основная дата
GPtime mainTime; //основное время
GPtime alarmTime; //время будильника

char buffMultiIp[20]; //буфер ip адреса
char buffMultiName[20]; //буфер имени

boolean clockUpdate = false; //флаг запрета обновления часов
boolean otaUpdate = false; //флаг запрета обновления есп
boolean fsUpdate = false; //флаг запрета обновления фс

boolean climateLocal = false; //флаг локальных скриптов графика
boolean alarmSvgImage = false; //флаг локальных изображений будильника
boolean timerSvgImage = false; //флаг локальных изображений таймера/секундомера
boolean radioSvgImage = false; //флаг локальных изображений радиоприемника

boolean failureWarn = true; //флаг отображения предупреждения об сбоях

uint8_t timeState = 0; //флаг состояния актуальности времени

int8_t syncState = -1; //флаг состояния синхронизации времени
uint8_t syncTimer = 0; //таймер запроса времени с ntp сервера

int8_t playbackTimer = -1; //таймер остановки воспроизведения
uint8_t waitTimer = 0; //таймер ожидания опроса шины

uint8_t sensorChart = 0; //буфер обновления источника данных для микроклимата
uint8_t sensorTimer = 0; //таймер обновления микроклимата

uint8_t navMainTab = 0; //флаг открытой страницы основных настроек
uint8_t navInfoTab = 0; //флаг открытой страницы информации об устройстве

uint32_t secondsTimer = 0; //таймер счета секундных интервалов

#if (LED_BUILTIN == TWI_SDA_PIN) || (LED_BUILTIN == TWI_SCL_PIN)
#undef STATUS_LED
#define STATUS_LED -1
#endif

#include "NTP.h"
#include "WIRE.h"
#include "UPDATER.h"
#include "CLOCKBUS.h"
#include "WIRELESS.h"

#include "WEATHER.h"

#include "WIFI.h"
#include "utils.h"

const char *climateNamesMain[] = {"Температура", "Влажность"};
const char *climateNamesExt[] = {"Давление"};

const char *climateFsData[] = {"/gp_data/PLOT_STOCK.js.gz"};
const char *alarmFsData[] = {"/alarm_add.svg", "/alarm_set.svg", "/alarm_dis.svg"};
const char *timerFsData[] = {"/timer_play.svg", "/timer_stop.svg", "/timer_pause.svg", "/timer_up.svg", "/timer_down.svg"};
const char *radioFsData[] = {"/radio_backward.svg", "/radio_left.svg", "/radio_right.svg", "/radio_forward.svg", "/radio_mode.svg", "/radio_power.svg"};

const char *alarmModeList[] = {"Отключен", "Однократно", "Ежедневно", "По будням"};
const char *alarmDaysList[] = {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"};
const char *statusTimerList[] = {"Отключен", "Секундомер", "Таймер", "Ошибка"};

const char *failureDataList[] = {
  "Нет связи с RTC", "Батарея RTC разряжена", "Короткий сигнал SQW", "Длинный сигнал SQW", "Датчик температуры недоступен", "Напряжение питания вне диапазона",
  "Сбой чтения EEPROM", "Софт перезагрузка", "Сбой преобразователя", "Сбой PWM преобразователя", "Переполнение стека", "Переполнение тиков времени", "Сбой динамической индикации"
};

String backlModeList(void) { //список режимов подсветки
  String str;
  str.reserve(500);
  if (deviceInformation[BACKL_TYPE]) {
    str = F("Выключена,Статичная,Дыхание");
    if (deviceInformation[BACKL_TYPE] >= 3) {
      str += F(",Дыхание со сменой цвета при затухании,Бегущий огонь,Бегущий огонь со сменой цвета,Бегущий огонь с радугой,Бегущий огонь с конфетти,Волна,Волна со сменой цвета,Волна с радугой,Волна с конфетти,Плавная смена цвета,Радуга,Конфетти");
    }
  }
  else {
    str = F("Не используется");
  }
  return str;
}
String dotModeList(boolean alm) { //список режимов основных разделительных точек
  String str;
  str.reserve(500);
  str = F("Выключены,Статичные,Мигают раз в секунду,Мигают два раза в секунду");
  if (deviceInformation[NEON_DOT] != 3) {
    str += F(",Динамичные(плавно мигают)");
  }
  if (deviceInformation[NEON_DOT] == 2) {
    str += F(",Неонки маятник(мигают раз в секунду),Неонки маятник(плавно мигают)");
  }
  if (deviceInformation[DOTS_PORT_ENABLE]) {
    str += F(",Мигающие,Бегущие,Змейка,Резинка");
    if ((deviceInformation[DOTS_NUM] > 4) || (deviceInformation[DOTS_TYPE] == 2)) {
      str += F(",Одинарный маятник");
    }
    if ((deviceInformation[DOTS_NUM] > 4) && (deviceInformation[DOTS_TYPE] == 2)) {
      str += F(",Двойной маятник");
    }
  }
  if (alm) {
    str += F(",Без реакции");
  }
  return str;
}
String secsModeList(void) { //список режимов смены секунд
  String str;
  str.reserve(200);
  if (deviceInformation[LAMP_NUM] < 6) {
    str = F("Не используются");
  }
  else {
    str = F("Без анимации,Плавное угасание и появление,Перемотка по порядку числа,Перемотка по порядку катодов в лампе");
  }
  return str;
}
String flipModeList(void) { //список режимов смены минут
  String str;
  str.reserve(370);
  str = F("Без анимации,Случайная смена эффектов,Плавное угасание и появление,Перемотка по порядку числа,Перемотка по порядку катодов в лампе,Поезд,Резинка,Ворота,Волна,Блики,Испарение,Игровой автомат");
  return str;
}
String playerVoiceList(void) { //список голосов для озвучки
  String str;
  str.reserve(100);
  if (deviceInformation[PLAYER_TYPE]) {
    str = F("Алёна,Филипп");
    for (uint8_t i = 2; i < deviceInformation[PLAYER_MAX_VOICE]; i++) {
      str += F(",Голос_");
      str += i;
    }
  }
  else {
    str = F("Не используется");
  }
  return str;
}

void build(void) {
  GP.BUILD_BEGIN(UI_MAIN_THEME, 500);
  GP_FIX_SCRIPTS(); //фикс скрипта проверки онлайна
  GP_FIX_STYLES(); //фикс стилей страницы

  if (updaterState()) {
    GP_PAGE_TITLE("Обновление");
    GP.BLOCK_BEGIN(GP_THIN, "", "Обновление прошивки часов", UI_BLOCK_COLOR);
    if (!updaterFlash()) {
      GP.SPAN(getUpdaterState(), GP_CENTER, "syncUpdate", GP_YELLOW); //описание
    }
    else {
      GP.SPAN("<big><b>Подключение...</b></big>", GP_CENTER, "syncUpdate", UI_INFO_COLOR); //описание
      GP.SPAN(String("<small>Не выключайте устройство до завершения обновления!</small>") + ((deviceInformation[HARDWARE_VERSION]) ? "" : "<br><small>Для входа в режим прошивки кратковременно нажмите ресет на микроконтроллере часов.</small>"), GP_CENTER, "syncWarn", GP_RED); //описание
      GP.UPDATE("syncUpdate,syncWarn", 300);
    }
    GP.HR(UI_LINE_COLOR);
    M_BOX(GP_CENTER, GP.BUTTON_MINI_LINK("/", "Вернуться на главную", UI_BUTTON_COLOR););
    GP.BLOCK_END();
  }
  else if (deviceInformation[HARDWARE_VERSION] && (deviceInformation[HARDWARE_VERSION] != HW_VERSION)) {
    GP_PAGE_TITLE("Ошибка совместимости");
    GP.BLOCK_BEGIN(GP_THIN, "", "Предупреждение", UI_BLOCK_COLOR);
    GP.SPAN("<big><b>Эта версия веб-интерфейса не может взаимодействовать с этим устройством!</b></big>", GP_CENTER, "", UI_INFO_COLOR);
    GP.BREAK();
    GP.LABEL("Clock HW: 0x" + String(deviceInformation[HARDWARE_VERSION], HEX));
    GP.BREAK();
    GP.LABEL("WebUI HW: 0x" + String(HW_VERSION, HEX));
    if (otaUpdate) {
      GP.HR(UI_LINE_COLOR);
      GP.BUTTON_MINI_LINK("/ota_update", "Обновить прошивку", UI_BUTTON_COLOR);
    }
    GP.BLOCK_END();
  }
  else if (busRebootState()) {
    GP.PAGE_TITLE("Перезагрузка");

    GP.BLOCK_BEGIN(GP_THIN, "", "Перезагрузка устройства", UI_BLOCK_COLOR);
    GP.SPAN("<big><b>Выполняется перезагрузка, подождите...</b></big>", GP_CENTER, "syncReboot", UI_INFO_COLOR); //описание
    GP.SPAN("<small>Не выключайте устройство до завершения перезагрузки!</small>", GP_CENTER, "syncWarn", GP_RED); //описание
    GP.HR(UI_LINE_COLOR);
    M_BOX(GP_CENTER, GP.BUTTON_MINI_LINK("/", "Вернуться на главную", UI_BUTTON_COLOR););
    GP.UPDATE("syncReboot,syncWarn");
    GP.BLOCK_END();
  }
  else {
    //обновления блоков
    String updateList;
    updateList.reserve(500);
    updateList = F("barTime");

    //начать меню
    GP.UI_MENU("Nixie clock", UI_MENU_COLOR);
    if (settings.nameMenu && settings.name[0]) {
      GP.LABEL(settings.name, "", UI_MENU_NAME_COLOR);
      GP_HR(UI_MENU_LINE_COLOR, 6);
    }

    //ссылки меню
    GP.UI_LINK("/", "Главная");
    GP.UI_LINK("/settings", "Настройки");
    if (sensorGetValidStatus()) GP.UI_LINK("/climate", "Микроклимат");
    if (weatherGetValidStatus()) GP.UI_LINK("/weather", "Погода");
    if (deviceInformation[RADIO_ENABLE]) GP.UI_LINK("/radio", "Радио");
    if (fsUpdate || otaUpdate || clockUpdate) GP.UI_LINK("/update", "Обновление");
    GP.UI_LINK("/information", "Об устройстве");
    GP.UI_LINK("/network", "Сетевые настройки");

    //ссылки часов
    if (settings.multi[0][0] != '\0') {
      GP_HR(UI_MENU_LINE_COLOR, 6);
      for (uint8_t i = 0; i < (MAX_CLOCK * 2); i += 2) {
        if (settings.multi[i][0] != '\0') {
          GP_UI_LINK(settings.multi[i], (settings.multi[i + 1][0] != '\0') ? settings.multi[i + 1] : settings.multi[i], UI_MENU_COLOR);
        }
        else break;
      }
    }

    GP_HR(UI_MENU_LINE_COLOR, 6);

    //состояние соединения
    updateList += F(",barLink");
    GP_BLOCK_SHADOW_BEGIN();
    GP.LABEL("Статус часов", "", UI_MENU_TEXT_COLOR, 15);
    GP_LINE_LED("barLink", busGetClockStatus(), UI_MENU_CLOCK_1_COLOR, UI_MENU_CLOCK_2_COLOR);
    GP_BLOCK_SHADOW_END();

    if (!deviceInformation[DS3231_ENABLE] && rtcGetFoundStatus()) {
      updateList += F(",barRtc");
      GP_BLOCK_SHADOW_BEGIN();
      GP.LABEL("Статус RTC", "", UI_MENU_TEXT_COLOR, 15);
      GP_LINE_LED("barRtc", rtcGetNormalStatus(), UI_MENU_CLOCK_1_COLOR, UI_MENU_CLOCK_2_COLOR);
      GP_BLOCK_SHADOW_END();
    }
    if (wirelessGetSensorStastus()) {
      updateList += F(",barSens");
      GP_BLOCK_SHADOW_BEGIN();
      GP.LABEL("Статус датчика", "", UI_MENU_TEXT_COLOR, 15);
      GP_LINE_LED("barSens", (wirelessGetOnlineStastus()), UI_MENU_CLOCK_1_COLOR, UI_MENU_CLOCK_2_COLOR);
      GP_BLOCK_SHADOW_END();
    }
    if (ntpGetRunStatus()) {
      updateList += F(",barNtp");
      GP_BLOCK_SHADOW_BEGIN();
      GP.LABEL("Статус NTP", "", UI_MENU_TEXT_COLOR, 15);
      GP_LINE_LED("barNtp", (ntpGetSyncStatus()), UI_MENU_CLOCK_1_COLOR, UI_MENU_CLOCK_2_COLOR);
      GP_BLOCK_SHADOW_END();
    }
    if (wifiGetConnectStatus()) {
      updateList += F(",bar_wifi");
      GP_BLOCK_SHADOW_BEGIN();
      GP.LABEL("Сигнал WiFi", "", UI_MENU_TEXT_COLOR, 15);
      GP_LINE_BAR("bar_wifi", constrain(2 * (WiFi.RSSI() + 100), 0, 100), 0, 100, 1, UI_MENU_WIFI_COLOR);
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
        GP_PAGE_TITLE("Главная");
        M_GRID(
          GP.BLOCK_BEGIN(GP_THIN, "", "Настройка времени", UI_BLOCK_COLOR);
          M_BOX(GP.LABEL("Время", "", UI_LABEL_COLOR); GP.TIME("mainTime"););
          M_BOX(GP.LABEL("Дата", "", UI_LABEL_COLOR); GP.DATE("mainDate"););
          M_BOX(GP.LABEL("Формат", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.LABEL("24ч", "", UI_LABEL_COLOR);  GP.SWITCH("mainTimeFormat", mainSettings.timeFormat, UI_SWITCH_COLOR); GP.LABEL("12ч", "", UI_LABEL_COLOR);););
          GP.HR(UI_LINE_COLOR);
          M_BOX(GP.LABEL("Часовой пояс", "", UI_LABEL_COLOR); GP.SELECT("syncGmt", "GMT-12,GMT-11,GMT-10,GMT-9,GMT-8,GMT-7,GMT-6,GMT-5,GMT-4,GMT-3,GMT-2,GMT-1,GMT+0,GMT+1,GMT+2,GMT+3,GMT+4,GMT+5,GMT+6,GMT+7,GMT+8,GMT+9,GMT+10,GMT+11,GMT+12", settings.ntpGMT + 12, 0););
          M_BOX(GP.LABEL("Автосинхронизация", "", UI_LABEL_COLOR); GP.SWITCH("syncAuto", settings.ntpSync, UI_SWITCH_COLOR, (boolean)(ntpGetStatus() != NTP_SYNCED)););
          M_BOX(GP.LABEL("Учитывать летнее время", "", UI_LABEL_COLOR); GP.SWITCH("syncDst", settings.ntpDst, UI_SWITCH_COLOR, (boolean)(ntpGetStatus() != NTP_SYNCED)););
          GP.HR(UI_LINE_COLOR);
          GP.BUTTON("syncTime", (ntpGetStatus() != NTP_SYNCED) ? "Время с устройства" : "Синхронизация с сервером", "", UI_BUTTON_COLOR);
          GP.BLOCK_END();

          GP.BLOCK_BEGIN(GP_THIN, "", "Эффекты", UI_BLOCK_COLOR);
          M_BOX(GP.LABEL("Глюки", "", UI_LABEL_COLOR); GP.SWITCH("mainGlitch", mainSettings.glitchMode, UI_SWITCH_COLOR););
          M_BOX(GP.LABEL("Точки", "", UI_LABEL_COLOR); GP.SELECT("fastDot", dotModeList(false), fastSettings.dotMode););
          M_BOX(GP.LABEL("Минуты", "", UI_LABEL_COLOR); GP.SELECT("fastFlip", flipModeList(), fastSettings.flipMode););
          M_BOX(GP.LABEL("Секунды", "", UI_LABEL_COLOR); GP.SELECT("fastSecsFlip", secsModeList(), fastSettings.secsMode, 0, (boolean)(deviceInformation[LAMP_NUM] < 6)););
          GP.HR(UI_LINE_COLOR);
          M_BOX(GP.LABEL("Подсветка", "", UI_LABEL_COLOR); GP.SELECT("fastBackl", backlModeList(), fastSettings.backlMode, 0, (boolean)!deviceInformation[BACKL_TYPE]););
          M_BOX(GP.LABEL("Цвет", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER_C("fastColor", (fastSettings.backlColor < 253) ? (fastSettings.backlColor / 10) : (fastSettings.backlColor - 227), 0, 28, 1, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[BACKL_TYPE]);););
          GP.HR(UI_LINE_COLOR);
          M_BOX(GP.LABEL((deviceInformation[PLAYER_TYPE]) ? "Озвучивать действия" : "Звук кнопок", "", UI_LABEL_COLOR); GP.SWITCH("mainSound", mainSettings.knockSound, UI_SWITCH_COLOR););
          M_BOX(GP.LABEL("Громкость", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER("mainSoundVol", mainSettings.volumeSound, 0, 15, 1, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[PLAYER_TYPE]);););
          GP.BLOCK_END();
        );
      }

      if (deviceInformation[ALARM_TYPE]) {
        if (alarm.reload >= 2) alarm.reload = 0;
        updateList += F(",mainReload");
        GP.RELOAD("mainReload");

        GP.BLOCK_BEGIN(GP_THIN, "", "Будильник", UI_BLOCK_COLOR);
        if (alarm.set) { //если режим настройки будильника
          GP_PAGE_TITLE("Настройка будильника");

          String alarmSoundList;
          alarmSoundList.reserve(200);
          for (uint8_t i = 0; i < deviceInformation[PLAYER_MAX_SOUND]; i++) {
            if (i) alarmSoundList += ',';
            alarmSoundList += F("Звук №");
            alarmSoundList += (i + 1);
          }

          String alarmRadioList;
          alarmRadioList.reserve(100);
          if (deviceInformation[RADIO_ENABLE]) {
            for (uint8_t i = 0; i < 10; i++) {
              if (i) alarmRadioList += ',';
              alarmRadioList += F("CH");
              alarmRadioList += i;
              alarmRadioList += ' ';
              alarmRadioList += String(radioSettings.stationsSave[i] / 10.0, 1);
            }
          }
          else alarmRadioList += F("Пусто");

          alarmTime.hour = alarm_data[alarm.now][ALARM_DATA_HOUR];
          alarmTime.minute = alarm_data[alarm.now][ALARM_DATA_MINS];

          M_BOX(GP_CENTER, GP.TIME("alarmTime", alarmTime); GP.SELECT("alarmMode", "Выключен,Однократно,Ежедневно,По будням,Выбрать дни", alarm_data[alarm.now][ALARM_DATA_MODE]););
          GP.BREAK();

          GP_HR_TEXT("Дни недели", "", UI_LINE_COLOR, UI_HINT_COLOR);
          GP.BLOCK_BEGIN(GP_DIV_RAW, "430px;margin:10px 0px");
          GP.TABLE_BORDER(false);
          GP.TABLE_BEGIN("50px,50px,50px,50px,50px,50px,50px");
          GP.TR(GP_CENTER);
          GP.TD(GP_CENTER);
          GP_LABEL_BLOCK_W("ПН", "", UI_ALARM_WEEK_1_COLOR, 0);
          GP.TD(GP_CENTER);
          GP_LABEL_BLOCK_W("ВТ", "", UI_ALARM_WEEK_1_COLOR, 0);
          GP.TD(GP_CENTER);
          GP_LABEL_BLOCK_W("СР", "", UI_ALARM_WEEK_1_COLOR, 0);
          GP.TD(GP_CENTER);
          GP_LABEL_BLOCK_W("ЧТ", "", UI_ALARM_WEEK_1_COLOR, 0);
          GP.TD(GP_CENTER);
          GP_LABEL_BLOCK_W("ПТ", "", UI_ALARM_WEEK_1_COLOR, 0);
          GP.TD(GP_CENTER);
          GP_LABEL_BLOCK_W("СБ", "", UI_ALARM_WEEK_2_COLOR, 0);
          GP.TD(GP_CENTER);
          GP_LABEL_BLOCK_W("ВС", "", UI_ALARM_WEEK_2_COLOR, 0);

          uint8_t alarmDays = alarm_data[alarm.now][ALARM_DATA_DAYS];
          GP.TR(GP_CENTER);
          String alarmDaysList;
          alarmDaysList.reserve(15);
          for (uint8_t i = 1; i < 8; i++) {
            GP.TD(GP_CENTER);
            alarmDays >>= 1;
            alarmDaysList = F(",alarmDay/");
            alarmDaysList += i;
            GP.CHECK(alarmDaysList, (boolean)(alarmDays & 0x01));
            updateList += alarmDaysList;
          }
          GP.TABLE_END();
          GP.BLOCK_END();

          GP_HR_TEXT("Настройка звука", "", UI_LINE_COLOR, UI_HINT_COLOR);
          M_BOX(GP_CENTER, GP.SELECT("alarmSoundType", (deviceInformation[RADIO_ENABLE]) ? "Мелодия,Радиостанция" : "Мелодия", (boolean)alarm_data[alarm.now][ALARM_DATA_RADIO], 0, (boolean)!deviceInformation[RADIO_ENABLE], true););
          M_BOX(GP_CENTER,
                GP.SELECT("alarmSound", alarmSoundList, alarm_data[alarm.now][ALARM_DATA_SOUND], 0, (boolean)(deviceInformation[RADIO_ENABLE] && alarm_data[alarm.now][ALARM_DATA_RADIO]));
                GP.SELECT("alarmRadio", alarmRadioList, alarm_data[alarm.now][ALARM_DATA_STATION], 0, (boolean)(!deviceInformation[RADIO_ENABLE] || !alarm_data[alarm.now][ALARM_DATA_RADIO]));
               );
          M_BOX(GP_CENTER, GP_SLIDER_MAX("Громкость", "авто", "макс", "alarmVol", alarm_data[alarm.now][ALARM_DATA_VOLUME], 0, 15, 1, 0, UI_SLIDER_COLOR, (boolean)((!deviceInformation[RADIO_ENABLE] || !alarm_data[alarm.now][ALARM_DATA_RADIO]) && !deviceInformation[PLAYER_TYPE])););

          GP.HR(UI_LINE_COLOR);
          M_BOX(GP_CENTER,
                GP.BUTTON_MINI("alarmBack", (alarm.set == 1) ? "Назад" : "Добавить", "", UI_ALARM_BACK_COLOR, "210px!important", false, true);
                GP.BUTTON_MINI("alarmDel", (alarm.all > 1) ? ((alarm.set == 1) ? "Удалить" : "Отмена") : "Отключить", "", (alarm.all > 1) ? UI_ALARM_DEL_COLOR : UI_ALARM_DIS_COLOR, "210px!important", false, (boolean)(alarm.all <= 1));
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

            String alarmStatus = " ";

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
              alarmStatus += " | Через ";
              uint8_t _time = alarmTimeNext / 1440;
              if (_time) {
                alarmStatus += _time;
                alarmStatus += "д ";
              }
              _time = (alarmTimeNext % 1440) / 60;
              if (_time) {
                alarmStatus += _time;
                alarmStatus += "ч ";
              }
              _time = (alarmTimeNext % 1440) % 60;
              if (_time) {
                alarmStatus += _time;
                alarmStatus += "мин";
              }
            }

            GP.BLOCK_BEGIN(GP_THIN, "", "", UI_ALARM_BLOCK_COLOR);
            lableList = F("alarmSet/");
            lableList += i;
            if (alarmSvgImage) {
              M_BOX(GP.LABEL(alarmTime, "", UI_ALARM_TIME_COLOR, 40, 1); GP.ICON_FILE_BUTTON(lableList, alarmFsData[1], 40, UI_ALARM_SET_COLOR););
            }
            else {
              M_BOX(GP.LABEL(alarmTime, "", UI_ALARM_TIME_COLOR, 40, 1); GP.BUTTON_MINI(lableList, " ≡ ", "", UI_ALARM_SET_COLOR, ""););
            }
            M_BOX(GP_LEFT, GP.BOLD(alarmStatus, "", UI_ALARM_INFO_COLOR););
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
          GP.RELOAD_CLICK(reloadList);
        }
        GP.BLOCK_END();
      }

      if (!alarm.set || !deviceInformation[ALARM_TYPE]) { //если не режим настройки будильника
        if (deviceInformation[TIMER_ENABLE] && deviceInformation[EXT_BTN_ENABLE]) {
          updateList += F(",mainTimer,mainTimerState");

          GP.BLOCK_BEGIN(GP_THIN, "", "Таймер/Секундомер", UI_BLOCK_COLOR);

          GP.BLOCK_BEGIN(GP_DIV_RAW , "280px");
          GP.BLOCK_BEGIN(GP_THIN, "", "", UI_TIMER_BLOCK_COLOR);
          GP.LABEL(getTimerState(), "mainTimerState", UI_TIMER_INFO_COLOR, 0, 1);

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
              M_BOX(GP_CENTER, GP.LABEL(convertTimerTime(), "mainTimer", UI_TIMER_TIME_COLOR, 40, 1););
              GP.TR();
            }
          }
          GP.TABLE_END();
          GP.BLOCK_END();
          GP.BLOCK_END();

          if (timerSvgImage) {
            M_BOX(GP_CENTER,
                  GP.ICON_FILE_BUTTON("timerControl/0", timerFsData[0], 60, UI_TIMER_CTRL_COLOR);
                  GP.ICON_FILE_BUTTON("timerControl/1", timerFsData[1], 60, UI_TIMER_CTRL_COLOR);
                  GP.ICON_FILE_BUTTON("timerControl/2", timerFsData[2], 60, UI_TIMER_CTRL_COLOR);
                 );
          }
          else {
            M_BOX(GP_CENTER,
                  GP.BUTTON_MINI("timerControl/0", "⠀⠀►⠀⠀", "", UI_TIMER_CTRL_COLOR);
                  GP.BUTTON_MINI("timerControl/1", "⠀⠀❑⠀⠀", "", UI_TIMER_CTRL_COLOR);
                  GP.BUTTON_MINI("timerControl/2", "⠀⠀||⠀⠀", "", UI_TIMER_CTRL_COLOR);
                 );
          }
          GP.BLOCK_END();

          GP.UPDATE_CLICK("mainTimer", "timerHour/0,timerHour/1,timerHour/2,timerHour/3,timerHour/4,timerHour/5");
          GP.UPDATE_CLICK("mainTimer,mainTimerState", "timerControl/0,timerControl/1,timerControl/2");
        }
      }
    }
    else if (ui.uri("/settings")) { //настройки
      GP_PAGE_TITLE("Настройки");

      String showModeList; //список режимов автопоказа
      showModeList.reserve(350);
      showModeList = F("Пусто,Дата,Год,Дата и год");
      if (deviceInformation[LAMP_NUM] < 6) showModeList += F("(недоступно)");
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
      lightHint = F("Одинаковое время - отключить смену яркости");
      if (deviceInformation[LIGHT_SENS_ENABLE]) lightHint += F(" или активировать датчик освещения");
      else if (weatherGetValidStatus()) lightHint += F(" или активировать автоматическую смену яркости");

      GP_NAV_TABS_M("fastMainTab", "Основные,Дополнительно", navMainTab);

      GP_NAV_BLOCK_BEGIN("fastMainTab", 0, navMainTab);
      M_GRID(
        GP.BLOCK_BEGIN(GP_THIN, "", "Метеостанция", UI_BLOCK_COLOR);
        M_BOX(
          GP.LABEL("По кнопке", "", UI_LABEL_COLOR);
          GP.SELECT("climateMainSens", climateGetSendDataList(), extendedSettings.tempMainSensor, 0, (boolean)((!climateAvailableTemp(settings.climateSend[0]) && !climateAvailableTemp(settings.climateSend[1])) || deviceInformation[BTN_EASY_MAIN_MODE]));
        );
        M_BOX(
          GP.LABEL("Раз в час", "", UI_LABEL_COLOR);
          GP.SELECT("climateHourSens", climateGetSendDataList(), extendedSettings.tempHourSensor, 0, (boolean)((!climateAvailableTemp(settings.climateSend[0]) && !climateAvailableTemp(settings.climateSend[1])) || !deviceInformation[PLAYER_TYPE]));
        );
        GP.BREAK();
        GP_HR_TEXT("Автопоказ", "", UI_LINE_COLOR, UI_HINT_COLOR);
        M_BOX(GP.LABEL("Включить", "", UI_LABEL_COLOR); GP.SWITCH("mainAutoShow", (boolean)!(mainSettings.autoShowTime & 0x80), UI_SWITCH_COLOR););
        M_BOX(GP.LABEL("Интервал, мин", "", UI_LABEL_COLOR); GP_SPINNER_MID("mainAutoShowTime", mainSettings.autoShowTime & 0x7F, 1, 15, 1, 0, UI_SPINNER_COLOR););
        M_BOX(GP.LABEL("Эффект", "", UI_LABEL_COLOR); GP.SELECT("mainAutoShowFlip", "Основной эффект,Случайная смена эффектов,Плавное угасание и появление,Перемотка по порядку числа,Перемотка по порядку катодов в лампе,Поезд,Резинка,Ворота,Волна,Блики,Испарение,Игровой автомат", mainSettings.autoShowFlip););
        GP.BREAK();
        GP_HR_TEXT("Отображение", "hint4", UI_LINE_COLOR, UI_HINT_COLOR);
        GP.HINT("hint4", "Источник и время в секундах"); //всплывающая подсказка
      for (uint8_t i = 0; i < 5; i++) {
      M_BOX(
        M_BOX(GP_LEFT, GP.LABEL(String(i + 1), "hint4", UI_LABEL_COLOR); GP.SELECT(String("extShowMode/") + i, showModeList, extendedSettings.autoShowModes[i]););
        GP_SPINNER_MID(String("extShowTime/") + i, extendedSettings.autoShowTimes[i], 1, 5, 1, 0, UI_SPINNER_COLOR);
      );
      }
      GP.BLOCK_END();

      GP.BLOCK_BEGIN(GP_THIN, "", "Индикаторы", UI_BLOCK_COLOR);
      GP.LABEL("Яркость", "", UI_HINT_COLOR);
      M_BOX(GP.LABEL("День", "", UI_LABEL_COLOR); GP.SLIDER_C("mainIndiBrtDay", mainSettings.indiBrightDay, 5, 30, 1, 0, UI_SLIDER_COLOR););
      M_BOX(GP.LABEL("Ночь", "", UI_LABEL_COLOR); GP.SLIDER_C("mainIndiBrtNight", mainSettings.indiBrightNight, 5, 30, 1, 0, UI_SLIDER_COLOR););
      GP.BREAK();
      GP_HR_TEXT("Эффекты", "", UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL("Глюки", "", UI_LABEL_COLOR); GP.SWITCH("mainGlitch", mainSettings.glitchMode, UI_SWITCH_COLOR););
      M_BOX(GP.LABEL("Минуты", "", UI_LABEL_COLOR); GP.SELECT("fastFlip", flipModeList(), fastSettings.flipMode););
      M_BOX(GP.LABEL("Секунды", "", UI_LABEL_COLOR); GP.SELECT("fastSecsFlip", secsModeList(), fastSettings.secsMode, 0, (boolean)(deviceInformation[LAMP_NUM] < 6)););
      GP.BREAK();
      GP_HR_TEXT("Антиотравление", "", UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL("Период, мин", "", UI_LABEL_COLOR); GP_SPINNER_MID("mainBurnTime", mainSettings.burnTime, 10, 180, 5, 0, UI_SPINNER_COLOR););
      M_BOX(GP.LABEL("Метод", "", UI_LABEL_COLOR); GP.SELECT("mainBurnFlip", "Перебор всех индикаторов,Перебор одного индикатора,Перебор одного индикатора с отображением времени", mainSettings.burnMode););
      GP.BREAK();
      GP_HR_TEXT("Время смены яркости", "hint1", UI_LINE_COLOR, UI_HINT_COLOR);
      GP.HINT("hint1", lightHint); //всплывающая подсказка
      M_BOX(GP_CENTER, GP.LABEL(" С", "", UI_LABEL_COLOR); GP_SPINNER_LEFT("mainTimeBrightS", mainSettings.timeBrightStart, 0, 23, 1, 0, UI_SPINNER_COLOR); GP_SPINNER_RIGHT("mainTimeBrightE", mainSettings.timeBrightEnd, 0, 23, 1, 0, UI_SPINNER_COLOR); GP.LABEL("До", "", UI_LABEL_COLOR););
      GP.BREAK();
      GP_HR_TEXT("Режим сна", "hint2", UI_LINE_COLOR, UI_HINT_COLOR);
      GP.HINT("hint2", "0 - отключить режим сна для выбранного промежутка времени"); //всплывающая подсказка
      M_BOX(GP_CENTER, GP.LABEL("День", "", UI_LABEL_COLOR); GP_SPINNER_LEFT("mainSleepD", mainSettings.timeSleepDay, 0, 90, 15, 0, UI_SPINNER_COLOR); GP_SPINNER_RIGHT("mainSleepN", mainSettings.timeSleepNight, 0, 30, 5, 0, UI_SPINNER_COLOR); GP.LABEL("Ночь", "", UI_LABEL_COLOR););
      GP.BLOCK_END();
      );

      M_GRID(
        GP.BLOCK_BEGIN(GP_THIN, "", "Подсветка", UI_BLOCK_COLOR);
        M_BOX(GP.LABEL("Цвет", "", UI_LABEL_COLOR); GP.SLIDER_C("fastColor", (fastSettings.backlColor < 253) ? (fastSettings.backlColor / 10) : (fastSettings.backlColor - 227), 0, 28, 1, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[BACKL_TYPE]););
        M_BOX(GP.LABEL("Режим", "", UI_LABEL_COLOR); GP.SELECT("fastBackl", backlModeList(), fastSettings.backlMode, 0, (boolean)!deviceInformation[BACKL_TYPE]););
        GP.BREAK();
        GP_HR_TEXT("Яркость", "", UI_LINE_COLOR, UI_HINT_COLOR);
        M_BOX(GP.LABEL("День", "", UI_LABEL_COLOR); GP.SLIDER_C("mainBacklBrightDay", mainSettings.backlBrightDay, 10, 250, 10, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[BACKL_TYPE]););
        M_BOX(GP.LABEL("Ночь", "", UI_LABEL_COLOR); GP.SLIDER_C("mainBacklBrightNight", mainSettings.backlBrightNight, 0, 250, 10, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[BACKL_TYPE]););
        GP.BLOCK_END();

        GP.BLOCK_BEGIN(GP_THIN, "", "Точки", UI_BLOCK_COLOR);
        M_BOX(GP.LABEL("Режим", "", UI_LABEL_COLOR); GP.SELECT("fastDot", dotModeList(false), fastSettings.dotMode););
        GP.BREAK();
        GP_HR_TEXT("Яркость", "", UI_LINE_COLOR, UI_HINT_COLOR);
        M_BOX(GP.LABEL("День", "", UI_LABEL_COLOR); GP.SLIDER_C("mainDotBrtDay", mainSettings.dotBrightDay, 10, 250, 10, 0, UI_SLIDER_COLOR, (boolean)(deviceInformation[NEON_DOT] == 3)););
        M_BOX(GP.LABEL("Ночь", "", UI_LABEL_COLOR); GP.SLIDER_C("mainDotBrtNight", mainSettings.dotBrightNight, 0, (deviceInformation[NEON_DOT] == 3) ? 1 : 250, (deviceInformation[NEON_DOT] == 3) ? 1 : 10, 0, UI_SLIDER_COLOR););
        GP.BLOCK_END();
      );
      GP.NAV_BLOCK_END();

      GP_NAV_BLOCK_BEGIN("fastMainTab", 1, navMainTab);
      M_GRID(
        GP.BLOCK_BEGIN(GP_THIN, "", "Звуки", UI_BLOCK_COLOR);
        M_BOX(GP.LABEL((deviceInformation[PLAYER_TYPE]) ? "Озвучивать действия" : "Звук кнопок", "", UI_LABEL_COLOR); GP.SWITCH("mainSound", mainSettings.knockSound, UI_SWITCH_COLOR););
        M_BOX(GP.LABEL("Голос озвучки", "", UI_LABEL_COLOR); GP.SELECT("mainVoice", playerVoiceList(), mainSettings.voiceSound, 0, (boolean)!deviceInformation[PLAYER_TYPE]););
        M_BOX(GP_JUSTIFY, GP.LABEL("Громкость", "", UI_LABEL_COLOR); GP.SLIDER("mainSoundVol", mainSettings.volumeSound, 0, 15, 1, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[PLAYER_TYPE]););
        GP.BREAK();
        GP_HR_TEXT("Звук смены часа", "hint3", UI_LINE_COLOR, UI_HINT_COLOR);
        GP.HINT("hint3", "Одниаковое время - отключить звук смены часа"); //всплывающая подсказка
        M_BOX(GP_CENTER, GP.LABEL(" С", "", UI_LABEL_COLOR); GP_SPINNER_LEFT("mainHourSoundS", mainSettings.timeHourStart, 0, 23, 1, 0, UI_SPINNER_COLOR); GP_SPINNER_RIGHT("mainHourSoundE", mainSettings.timeHourEnd, 0, 23, 1, 0, UI_SPINNER_COLOR); GP.LABEL("До", "", UI_LABEL_COLOR););
        GP.BREAK();
        GP_HR_TEXT("Озвучка смены часа", "", UI_LINE_COLOR, UI_HINT_COLOR);
        M_BOX(GP.LABEL("Температура", "", UI_LABEL_COLOR); GP.SWITCH("mainHourTemp", mainSettings.hourSound & 0x80, UI_SWITCH_COLOR, (boolean)!(deviceInformation[PLAYER_TYPE] && sensorAvaibleData())););
        M_BOX(GP.LABEL("Новый час", "", UI_LABEL_COLOR); GP.SELECT("mainHourSound", "Автоматически,Только мелодия,Только озвучка,Мелодия и озвучка", mainSettings.hourSound & 0x03, 0, (boolean)!deviceInformation[PLAYER_TYPE]););
        GP.BLOCK_END();

        GP.BLOCK_BEGIN(GP_THIN, "", "Будильник", UI_BLOCK_COLOR);
        M_BOX(GP.LABEL("Автоотключение, мин", "", UI_LABEL_COLOR); GP_SPINNER_MID("extAlarmTimeout", extendedSettings.alarmTime, 1, 240, 1, 0, UI_SPINNER_COLOR, "", (boolean)!deviceInformation[ALARM_TYPE]););

        GP.BREAK();
        GP_HR_TEXT("Дополнительно", "", UI_LINE_COLOR, UI_HINT_COLOR);
        M_BOX(GP.LABEL("Повтор сигнала, мин", "", UI_LABEL_COLOR); GP_SPINNER_MID("extAlarmWaitTime", extendedSettings.alarmWaitTime, 0, 240, 1, 0, UI_SPINNER_COLOR, "", (boolean)!deviceInformation[ALARM_TYPE]););
        M_BOX(GP.LABEL("Отключить звук, мин", "", UI_LABEL_COLOR); GP_SPINNER_MID("extAlarmSoundTime", extendedSettings.alarmSoundTime, 0, 240, 1, 0, UI_SPINNER_COLOR, "", (boolean)!deviceInformation[ALARM_TYPE]););

        GP.BREAK();
        GP_HR_TEXT("Индикация", "", UI_LINE_COLOR, UI_HINT_COLOR);
        M_BOX(GP.LABEL("Активный", "", UI_LABEL_COLOR); GP.SELECT("extAlarmDotOn", dotModeList(true), extendedSettings.alarmDotOn, 0, (boolean)!deviceInformation[ALARM_TYPE]););
        M_BOX(GP.LABEL("Ожидание", "", UI_LABEL_COLOR); GP.SELECT("extAlarmDotWait", dotModeList(true), extendedSettings.alarmDotWait, 0, (boolean)!deviceInformation[ALARM_TYPE]););
        GP.BLOCK_END();
      );

      M_GRID(
        GP.BLOCK_BEGIN(GP_THIN, "", "Микроклимат", UI_BLOCK_COLOR);
        M_BOX(GP.LABEL("Усреднение", "", UI_LABEL_COLOR); GP.SWITCH("climateAvg", settings.climateAvg, UI_SWITCH_COLOR, (boolean)(settings.climateChart == SENS_WIRELESS)););
        M_BOX(GP.LABEL("Интервал, мин", "", UI_LABEL_COLOR); GP_SPINNER_MID("climateTime", (settings.climateChart == SENS_WIRELESS) ? wirelessGetInterval() : settings.climateTime, 1, 60, 1, 0, UI_SPINNER_COLOR, "", (boolean)(settings.climateChart == SENS_WIRELESS)););
        GP.BREAK();
        GP_HR_TEXT("Отображение", "", UI_LINE_COLOR, UI_HINT_COLOR);
        M_BOX(GP.LABEL("Бар", "", UI_LABEL_COLOR); GP.SELECT("climateBar", climateGetMainDataList(SENS_CLOCK, SENS_MAX_DATA), settings.climateBar, 0, false, true););
        M_BOX(GP.LABEL("График", "", UI_LABEL_COLOR); GP.SELECT("climateChart", climateGetMainDataList(SENS_CLOCK, SENS_WEATHER), settings.climateChart, 0););
        GP.BLOCK_END();

        GP.BLOCK_BEGIN(GP_THIN, "", "Датчики", UI_BLOCK_COLOR);
        M_BOX(GP.LABEL("Датчик 1", "", UI_LABEL_COLOR); GP.SELECT("climateSend/0", (deviceInformation[SENS_TEMP]) ? "Датчик в часах" : climateGetMainDataList(SENS_MAIN, SENS_MAX_DATA), settings.climateSend[0] - 1, 0, (boolean)(deviceInformation[SENS_TEMP]), true););
        M_BOX(GP.LABEL("Датчик 2", "", UI_LABEL_COLOR); GP.SELECT("climateSend/1", climateGetMainDataList(SENS_MAIN, SENS_MAX_DATA), settings.climateSend[1] - 1, 0, false, true););
        GP.BREAK();
        GP_HR_TEXT("Коррекция", "", UI_LINE_COLOR, UI_HINT_COLOR);
        M_BOX(GP.LABEL("Температура, °C", "", UI_LABEL_COLOR); GP_SPINNER_MID("mainTempCorrect", mainSettings.tempCorrect / 10.0, -12.7, 12.7, 0.1, 1, UI_SPINNER_COLOR););
        M_BOX(GP.LABEL("Корректировать", "", UI_LABEL_COLOR); GP.SELECT("climateCorrectType", "Ничего," + climateGetSendDataList(), extendedSettings.tempCorrectSensor););
        GP.BLOCK_END();
      );
      GP.NAV_BLOCK_END();

      updateList += F(",climateReload");
      GP.RELOAD("climateReload");

      GP.CONFIRM("climateWarn", "Статистика микроклимата будет сброшена, продолжить?");
      GP.UPDATE_CLICK("climateWarn", "climateChart");
    }
    else if (ui.uri("/climate")) { //микроклимат
      GP_PAGE_TITLE("Микроклимат");

      uint16_t heightSize = 500;
      if (climateGetChartPress()) heightSize = 300;

      GP.BLOCK_BEGIN(GP_THIN, "", "Микроклимат", UI_BLOCK_COLOR);
      GP_PLOT_STOCK_BEGIN(climateLocal);

      if (climateGetChartHum()) {
        GP_PLOT_STOCK_DARK("climateDataMain", climateNamesMain, climateDates, climateArrMain[0], climateArrMain[1], CLIMATE_BUFFER, 10, heightSize, UI_BAR_TEMP_COLOR, UI_BAR_HUM_COLOR);
      }
      else {
        GP_PLOT_STOCK_DARK("climateDataMain", climateNamesMain, climateDates, climateArrMain[0], NULL, CLIMATE_BUFFER, 10, heightSize, UI_BAR_TEMP_COLOR, UI_BAR_HUM_COLOR);
      }
      if (climateGetChartPress()) {
        GP_PLOT_STOCK_DARK("climateDataExt", climateNamesExt, climateDates, climateArrExt[0], NULL, CLIMATE_BUFFER, 0, heightSize, UI_BAR_PRESS_COLOR);
      }

      GP.BREAK();
      GP_HR_TEXT("Датчик в часах", "", UI_LINE_COLOR, UI_HINT_COLOR);
      if (sens.temp[SENS_CLOCK] != 0x7FFF) {
        M_BOX(GP.LABEL("Данные", "", UI_LABEL_COLOR); GP.TEXT("", "", climateGetSensDataStr(sens.temp[SENS_CLOCK], sens.press[SENS_CLOCK], sens.hum[SENS_CLOCK]), "", 0, "", true););
        M_BOX(GP.LABEL("Тип датчика", "", UI_LABEL_COLOR); GP.TEXT("", "", (sens.type < 6) ? climateTempSensList[sens.type] : climateGetSensList(sens.type, false), "", 0, "", true););
      }
      else {
        M_BOX(GP.LABEL("Состояние", "", UI_LABEL_COLOR); GP.NUMBER("", "Не обнаружен...", INT32_MAX, "", true););
      }

      GP.BREAK();
      GP_HR_TEXT("Датчик в есп", "", UI_LINE_COLOR, UI_HINT_COLOR);
      if (sens.temp[SENS_MAIN] != 0x7FFF) {
        M_BOX(GP.LABEL("Данные", "", UI_LABEL_COLOR); GP.TEXT("", "", climateGetSensDataStr(sens.temp[SENS_MAIN], sens.press[SENS_MAIN], sens.hum[SENS_MAIN]), "", 0, "", true););
        M_BOX(GP.LABEL("Тип датчика", "", UI_LABEL_COLOR); GP.TEXT("", "", climateGetSensList(sens.search, true), "", 0, "", true););
      }
      else {
        M_BOX(GP.LABEL("Состояние", "", UI_LABEL_COLOR); GP.NUMBER("", "Не обнаружен...", INT32_MAX, "", true););
      }

      GP.BREAK();
      GP_HR_TEXT("Беспроводной датчик", "", UI_LINE_COLOR, UI_HINT_COLOR);
      if (!wirelessGetOnlineStastus()) {
        M_BOX(GP.LABEL("Состояние", "", UI_LABEL_COLOR); GP.NUMBER("", wirelessGetStrStastus(), INT32_MAX, "", true););
      }
      else {
        M_BOX(GP.LABEL("Данные", "", UI_LABEL_COLOR); GP.TEXT("", "", climateGetSensDataStr(sens.temp[SENS_WIRELESS], sens.press[SENS_WIRELESS], sens.hum[SENS_WIRELESS]), "", 0, "", true););
      }
      if (wirelessGetSensorStastus()) {
        M_BOX(GP.LABEL("Интервал", "", UI_LABEL_COLOR); GP.TEXT("", "", String(wirelessGetInterval()) + " мин", "", 0, "", true););
      }
      GP.BLOCK_END();
    }
    else if (ui.uri("/weather")) { //погода
      GP_PAGE_TITLE("Погода");

      GP.BLOCK_BEGIN(GP_THIN, "", "Погода на сутки", UI_BLOCK_COLOR);
      GP_PLOT_STOCK_BEGIN(climateLocal);
      GP_PLOT_STOCK_DARK("weatherDataMain", climateNamesMain, weatherDates, weatherArrMain[0], weatherArrMain[1], WEATHER_BUFFER, 10, 300, UI_BAR_TEMP_COLOR, UI_BAR_HUM_COLOR);
      GP_PLOT_STOCK_DARK("weatherDataExt", climateNamesExt, weatherDates, weatherArrExt[0], NULL, WEATHER_BUFFER, 10, 300, UI_BAR_PRESS_COLOR);
      GP.BREAK();
      GP.BLOCK_END();

      uint8_t time_start = (weatherDates[0] % 86400UL) / 3600UL;

      GP.BLOCK_BEGIN(GP_THIN, "", "Погода по часам", UI_BLOCK_COLOR);
      for (uint8_t i = 0; i < WEATHER_BUFFER; i++) {
        if (i) {
          GP.HR(UI_MENU_LINE_COLOR);
          GP.BREAK();
        }
        M_BOX(
          GP.LABEL(((time_start >= 10) ? String(time_start) : ('0' + String(time_start))) + ":00", "", GP_DEFAULT, 30);
          GP.LABEL(String(weatherArrMain[0][i] / 10.0, 1) + "°С", "", UI_BAR_TEMP_COLOR);
          GP.LABEL(String(weatherArrMain[1][i] / 10) + "%", "", UI_BAR_HUM_COLOR);
          GP.LABEL(String(weatherArrExt[0][i] / 10) + "mm.Hg", "", UI_BAR_PRESS_COLOR);
        );
        GP.BREAK();
        if (++time_start > 23) time_start = 0;
      }
      GP.BLOCK_END();
    }
    else if (ui.uri("/radio")) { //радиоприемник
      GP_PAGE_TITLE("Радио");

      updateList += F(",radioVol,radioFreq,radioPower");

      GP.BLOCK_BEGIN(GP_THIN, "", "Радиоприёмник", UI_BLOCK_COLOR);
      GP.BLOCK_BEGIN(GP_DIV_RAW, "450px");
      if (!radioSvgImage) {
        M_BOX(M_BOX(GP_LEFT, GP.BUTTON_MINI("radioMode", "Часы ⇋ Радио", "", UI_RADIO_BACK_COLOR);); M_BOX(GP_RIGHT, GP.LABEL("Питание", "", UI_LABEL_COLOR); GP.SWITCH("radioPower", radioSettings.powerState, UI_RADIO_POWER_2_COLOR);););
      }
      M_BOX(GP_CENTER, GP_SLIDER_MAX("Громкость", "мин", "макс", "radioVol", radioSettings.volume, 0, 15, 1, 0, UI_RADIO_VOL_COLOR, false, true););
      M_BOX(GP_CENTER, GP_SLIDER_MAX("Частота", "", "", "radioFreq", radioSettings.stationsFreq / 10.0, 87.5, 108, 0.1, 1, UI_RADIO_FREQ_1_COLOR, false, true););
      GP.BLOCK_END();

      if (radioSvgImage) {
        GP.SEND("<style>#radioMode .i_mask{margin-left:5px;margin-right:4px;}\n"
                "#radioFreqDown .i_mask{margin-left:0px;margin-right:3px;}\n"
                "#radioFreqUp .i_mask{margin-left:3px;margin-right:0px;}</style>\n"
               );
        M_BOX(GP_CENTER,
              GP.ICON_FILE_BUTTON("radioMode", radioFsData[4], 40, UI_RADIO_BACK_COLOR);
              GP.ICON_FILE_BUTTON("radioSeekDown", radioFsData[0], 30, UI_RADIO_FREQ_2_COLOR);
              GP.ICON_FILE_BUTTON("radioFreqDown", radioFsData[1], 30, UI_RADIO_FREQ_2_COLOR);
              GP.ICON_FILE_BUTTON("radioFreqUp", radioFsData[2], 30, UI_RADIO_FREQ_2_COLOR);
              GP.ICON_FILE_BUTTON("radioSeekUp", radioFsData[3], 30, UI_RADIO_FREQ_2_COLOR);
              GP_CHECK_ICON("radioPower", radioFsData[5], radioSettings.powerState, 50, UI_RADIO_POWER_1_COLOR, UI_RADIO_POWER_2_COLOR);
             );
      }
      else {
        M_BOX(GP_CENTER,
              GP.BUTTON("radioSeekDown", "|◄◄", "", UI_RADIO_FREQ_2_COLOR, "100px");
              GP.BUTTON("radioFreqDown", "◄", "", UI_RADIO_FREQ_2_COLOR, "100px");
              GP.BUTTON("radioFreqUp", "►", "", UI_RADIO_FREQ_2_COLOR, "100px");
              GP.BUTTON("radioSeekUp", "►►|", "", UI_RADIO_FREQ_2_COLOR, "100px");
             );
      }
      GP.BLOCK_END();

      GP.BLOCK_BEGIN(GP_THIN, "", "Станции", UI_BLOCK_COLOR);
      GP.TABLE_BEGIN("20%,30%,20%,30%", GP_ALS(GP_RIGHT, GP_LEFT, GP_RIGHT, GP_LEFT));
      for (int i = 0; i < 10; i += 2) {
        M_TR(
          GP.BUTTON_MINI(String("radioCh/") + i, String("CH") + i, "", UI_RADIO_CHANNEL_COLOR),
          GP.NUMBER_F(String("radioSta/") + i, "Пусто", (radioSettings.stationsSave[i]) ? (radioSettings.stationsSave[i] / 10.0) : NAN, 1),
          GP.BUTTON_MINI(String("radioCh/") + (i + 1), String("CH") + (i + 1), "", UI_RADIO_CHANNEL_COLOR),
          GP.NUMBER_F(String("radioSta/") + (i + 1), "Пусто", (radioSettings.stationsSave[i + 1]) ? (radioSettings.stationsSave[i + 1] / 10.0) : NAN, 1)
        );
      }
      GP.TABLE_END();
      GP.BLOCK_END();

      GP.UPDATE_CLICK("radioSta/0,radioSta/1,radioSta/2,radioSta/3,radioSta/4,radioSta/5,radioSta/6,radioSta/7,radioSta/8,radioSta/9,radioFreq",
                      "radioSta/0,radioSta/1,radioSta/2,radioSta/3,radioSta/4,radioSta/5,radioSta/6,radioSta/7,radioSta/8,radioSta/9,radioCh/0,radioCh/1,radioCh/2,radioCh/3,radioCh/4,radioCh/5,radioCh/6,radioCh/7,radioCh/8,radioCh/9,");
    }
    else if (ui.uri("/update") && (fsUpdate || otaUpdate || clockUpdate)) { //обновление ESP
      GP_PAGE_TITLE("Обновление");

      GP.BLOCK_BEGIN(GP_THIN, "", "Обновление прошивки", UI_BLOCK_COLOR);
      GP.SPAN("Прошивку можно получить в Arduino IDE: Скетч -> Экспорт бинарного файла (сохраняется в папку с прошивкой).", GP_CENTER, "", UI_INFO_COLOR); //описание
      GP.BREAK();
      if (fsUpdate) {
        GP.SPAN("Файловую систему можно получить в Arduino IDE: Инструменты -> ESP8266 LittleFS Data Upload, в логе необходимо найти: [LittleFS] upload, файл находится по этому пути.", GP_CENTER, "", UI_INFO_COLOR); //описание
        GP.BREAK();
      }
      String formatText;
      formatText.reserve(100);
      formatText = F("Поддерживаемые форматы файлов: ");
      if (clockUpdate) formatText += F("hex");
      if (fsUpdate || otaUpdate) {
        if (clockUpdate) formatText += F(", ");
        formatText += F("bin и bin.gz.");
      }
      else formatText += '.';
      GP.SPAN(formatText, GP_CENTER, "", UI_INFO_COLOR); //описание
      GP.BREAK();
      GP_HR_TEXT("Загрузить файлы", "", UI_LINE_COLOR, UI_HINT_COLOR);
      if (clockUpdate) {
        M_BOX(GP.LABEL("Прошивка часов", "", UI_LABEL_COLOR); GP.FILE_UPLOAD("updater", "", ".hex", UI_BUTTON_COLOR););
      }
      if (otaUpdate) {
        M_BOX(GP.LABEL("Прошивка ESP", "", UI_LABEL_COLOR); GP.OTA_FIRMWARE("", UI_BUTTON_COLOR, true););
      }
      if (fsUpdate) {
        M_BOX(GP.LABEL("Файловая система ESP", "", UI_LABEL_COLOR); GP.OTA_FILESYSTEM("", UI_BUTTON_COLOR, true););
      }
      GP.BLOCK_END();
    }
    else if (ui.uri("/information")) { //информация о системе
      GP_PAGE_TITLE("Об устройстве");

      GP_NAV_TABS_M("fastInfoTab", "Информация,Управление", navInfoTab);

      GP_NAV_BLOCK_BEGIN("fastInfoTab", 0, navInfoTab);
      GP.BLOCK_BEGIN(GP_THIN, "", "Системная информация", UI_BLOCK_COLOR);
      M_BOX(GP.LABEL("ID чипа", "", UI_LABEL_COLOR); GP.LABEL("0x" + String(ESP.getChipId(), HEX), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("Частота процессора", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getCpuFreqMHz()) + F(" MHz"), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("Циклов в секунду", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getCycleCount()), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("Время работы", "", UI_LABEL_COLOR); GP.LABEL(getTimeFromMs(millis()), "", UI_INFO_COLOR););

      GP.BREAK();
      GP_HR_TEXT("Память устройства", "", UI_LINE_COLOR, UI_HINT_COLOR);

      M_BOX(GP.LABEL("Фрагментировано(Heap)", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getHeapFragmentation()) + '%', "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("Свободно(Heap)", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getFreeHeap() / 1000.0, 3) + " kB", "", UI_INFO_COLOR););

      M_BOX(GP.LABEL("Всего(Flash)", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getFlashChipSize() / 1000.0, 1) + " kB", "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("Занято(Flash)", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getSketchSize() / 1000.0, 1) + " kB", "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("Свободно(Flash)", "", UI_LABEL_COLOR); GP.LABEL(String(ESP.getFreeSketchSpace() / 1000.0, 1) + " kB", "", UI_INFO_COLOR););

      GP.BREAK();
      GP_HR_TEXT("Локальная сеть", "", UI_LINE_COLOR, UI_HINT_COLOR);

      M_BOX(GP.LABEL("Уровень сигнала", "", UI_LABEL_COLOR); GP.LABEL("📶 " + String(constrain(2 * (WiFi.RSSI() + 100), 0, 100)) + '%', "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("Режим модема", "", UI_LABEL_COLOR); GP.LABEL(WiFi.getMode() == WIFI_AP ? "AP" : (WiFi.getMode() == WIFI_STA ? "STA" : "AP_STA"), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("MAC адрес", "", UI_LABEL_COLOR); GP.LABEL(WiFi.macAddress(), "", UI_INFO_COLOR););

      if (wifiGetConnectStatus()) {
        M_BOX(GP.LABEL("Маска подсети", "", UI_LABEL_COLOR); GP.LABEL(WiFi.subnetMask().toString(), "", UI_INFO_COLOR););
        M_BOX(GP.LABEL("Шлюз", "", UI_LABEL_COLOR); GP.LABEL(WiFi.gatewayIP().toString(), "", UI_INFO_COLOR););
        M_BOX(GP.LABEL("SSID сети", "", UI_LABEL_COLOR); GP.LABEL(StrLengthConstrain(WiFi.SSID(), 12), "", UI_INFO_COLOR););
        M_BOX(GP.LABEL("IP сети", "", UI_LABEL_COLOR); GP.LABEL(WiFi.localIP().toString(), "", UI_INFO_COLOR););
      }
      if (WiFi.getMode() != WIFI_STA) {
        M_BOX(GP.LABEL("SSID точки доступа", "", UI_LABEL_COLOR); GP.LABEL(StrLengthConstrain((settings.nameAp) ? (AP_SSID + String(" - ") + settings.name) : AP_SSID, 12), "", UI_INFO_COLOR););
        M_BOX(GP.LABEL("IP точки доступа", "", UI_LABEL_COLOR); GP.LABEL(WiFi.softAPIP().toString(), "", UI_INFO_COLOR););
      }

      GP.BREAK();
      GP_HR_TEXT("Версия ПО", "", UI_LINE_COLOR, UI_HINT_COLOR);

      M_BOX(GP.LABEL("SDK", "", UI_LABEL_COLOR); GP.LABEL(ESP.getSdkVersion(), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("CORE", "", UI_LABEL_COLOR); GP.LABEL(ESP.getCoreVersion(), "", UI_INFO_COLOR););
      M_BOX(GP.LABEL("GyverPortal", "", UI_LABEL_COLOR); GP.LABEL(GP_VERSION, "", UI_INFO_COLOR););

      M_BOX(GP.LABEL("Прошивка ESP", "", UI_LABEL_COLOR); GP.LABEL(ESP_FIRMWARE_VERSION, "", UI_INFO_COLOR););
      if (deviceInformation[HARDWARE_VERSION]) {
        M_BOX(GP.LABEL("Прошивка часов", "", UI_LABEL_COLOR); GP.LABEL(String(deviceInformation[FIRMWARE_VERSION_1]) + "." + deviceInformation[FIRMWARE_VERSION_2] + "." + deviceInformation[FIRMWARE_VERSION_3], "", UI_INFO_COLOR););
      }

      if (!(device.failure & 0x80)) {
        GP.BREAK();
        GP_HR_TEXT("Состояние", "", UI_LINE_COLOR, UI_HINT_COLOR);
        if (!device.failure) {
          M_BOX(GP.LABEL("Связь с часами", "", UI_LABEL_COLOR); GP.LABEL((busGetClockStatus()) ? "Работает нормально..." : "Отсутствует...", "", UI_INFO_COLOR, 0, false, true););
        }
        else {
          for (uint8_t i = 0; i < 13; i++) {
            if (device.failure & (0x01 << i)) {
              M_BOX(GP.LABEL(String("Ошибка 00") + ((i < 10) ? "0" : "") + (i + 1), "", UI_LABEL_COLOR); GP.LABEL(failureDataList[i], "", UI_INFO_COLOR, 0, false, true););
            }
          }
        }
      }
      GP.BREAK();
      GP.BLOCK_END();
      GP.NAV_BLOCK_END();

      GP_NAV_BLOCK_BEGIN("fastInfoTab", 1, navInfoTab);
      GP.BLOCK_BEGIN(GP_THIN, "", "Устройство", UI_BLOCK_COLOR);
      M_BOX(GP.LABEL("Имя", "", UI_LABEL_COLOR); GP.TEXT("extDeviceName", "Без названия", settings.name, "", 19););
      GP.BREAK();
      GP_HR_TEXT("Отображение", "", UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.LABEL("Меню", "", UI_LABEL_COLOR); GP.SWITCH("extDeviceMenu", settings.nameMenu, UI_SWITCH_COLOR););
      M_BOX(GP.LABEL("Префикс", "", UI_LABEL_COLOR); GP.SWITCH("extDevicePrefix", settings.namePrefix, UI_SWITCH_COLOR););
      M_BOX(GP.LABEL("Постфикс", "", UI_LABEL_COLOR); GP.SWITCH("extDevicePostfix", settings.namePostfix, UI_SWITCH_COLOR););
      M_BOX(GP.LABEL("Точка доступа", "", UI_LABEL_COLOR); GP.SWITCH("extDeviceAp", settings.nameAp, UI_SWITCH_COLOR););

      GP.BREAK();
      GP_HR_TEXT("Групповое управление", "", UI_LINE_COLOR, UI_HINT_COLOR);
      for (uint8_t i = 0; i < (MAX_CLOCK * 2); i += 2) {
        if (i) {
          GP.HR(UI_MENU_LINE_COLOR);
        }
        if (settings.multi[i][0] != '\0') {
          M_BOX(
            M_BOX(GP_LEFT, GP.TEXT("", "IP адрес", settings.multi[i], "", 20, "", true); GP.TEXT("", "Название", (settings.multi[i + 1][0] != '\0') ? settings.multi[i + 1] : settings.multi[i], "", 20, "", true););
            GP.BUTTON_MINI(String("extMultiDel/") + i, "Удалить", "", UI_BUTTON_COLOR, "115px!important", false, true);
          );
        }
        else {
          M_BOX(
            M_BOX(GP_LEFT, GP.TEXT("extMultiIp", "IP адрес", "", "", 15); GP.TEXT("extMultiName", "Название", "", "", 19););
            GP.BUTTON_MINI("extMultiAdd", "Добавить", "", UI_BUTTON_COLOR, "115px!important", false, true);
          );
          buffMultiIp[0] = '\0';
          buffMultiName[0] = '\0';
          break;
        }
      }

      GP.BREAK();
      GP_HR_TEXT("WiFi датчик температуры", "", UI_LINE_COLOR, UI_HINT_COLOR);
      if (!wirelessGetOnlineStastus()) {
        M_BOX(GP.LABEL("Состояние", "", UI_LABEL_COLOR); GP.NUMBER("", wirelessGetStrStastus(), INT32_MAX, "", true););
      }
      if (wirelessGetSensorStastus()) {
        M_BOX(GP.LABEL("UID", "", UI_LABEL_COLOR); GP.NUMBER("", wirelessGetId(settings.wirelessId), INT32_MAX, "", true););
        M_BOX(GP.LABEL("Сигнал", "", UI_LABEL_COLOR); GP.NUMBER("", String(wirelessGetSignal()) + "%", INT32_MAX, "", true););
        M_BOX(GP.LABEL("Батарея", "", UI_LABEL_COLOR); GP.NUMBER("", String(wirelessGetBattery()) + "%", INT32_MAX, "", true););
      }

      String rtcStatus;
      rtcStatus.reserve(50);
      rtcStatus = F("Не обнаружен...");
      GP.BREAK();
      GP_HR_TEXT("Модуль RTC", "", UI_LINE_COLOR, UI_HINT_COLOR);
      if (deviceInformation[DS3231_ENABLE]) {
        if ((device.failure & 0x80) || !(device.failure & 0x03)) rtcStatus = F("Подключен к часам");
        else if (device.failure & 0x02) rtcStatus = F("Батарея разряжена");
      }
      else if (rtcGetFoundStatus()) {
        M_BOX(GP.LABEL("Коррекция", "", UI_LABEL_COLOR); GP.NUMBER("syncAging", "-128..127", rtc_aging););
        GP.UPDATE_CLICK("syncAging", "syncAging");
        if (!rtcGetNormalStatus()) rtcStatus = F("Батарея разряжена");
        else rtcStatus = F("Работает исправно");
      }
      M_BOX(GP.LABEL("Состояние", "", UI_LABEL_COLOR); GP.NUMBER("", rtcStatus, INT32_MAX, "", true););

      GP.BREAK();
      GP_HR_TEXT("Управление", "", UI_LINE_COLOR, UI_HINT_COLOR);
      M_BOX(GP.BUTTON("resetButton", "Сброс настроек", "", UI_BUTTON_COLOR); GP.BUTTON("rebootButton", "Перезагрузка", "", UI_BUTTON_COLOR););
      GP.BLOCK_END();
      GP.NAV_BLOCK_END();

      GP.CONFIRM("extReset", "Сбросить все настройки устройства?");
      GP.CONFIRM("extReboot", "Перезагрузить устройство?");

      GP.UPDATE_CLICK("extReset", "resetButton");
      GP.UPDATE_CLICK("extReboot", "rebootButton");
      GP.RELOAD_CLICK(String("extReset,extReboot,extDeviceMenu,extDevicePrefix,extDevicePostfix") + ((settings.nameMenu || settings.namePrefix || settings.namePostfix || (settings.multi[0][0] != '\0')) ? ",extDeviceName" : ""));
    }
    else { //подключение к роутеру
      GP_PAGE_TITLE("Сетевые настройки");

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
          updateList += F(",syncNetwork");
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

        updateList += F(",syncReload");
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

      if (ui.uri("/network")) { //сетевые настройки
        updateList += F(",syncStatus,syncWeather");

        GP.BLOCK_BEGIN(GP_THIN, "", "Сервер NTP", UI_BLOCK_COLOR);
        GP.TEXT("syncHost", "Хост", settings.host, "", 19);
        GP.BREAK();
        GP.SELECT("syncPer", String("Каждые 15 мин,Каждые 30 мин,Каждый 1 час") + ((settings.ntpDst) ? "" : ",Каждые 2 часа,Каждые 3 часа"), (settings.ntpDst && (settings.ntpTime > 2)) ? 2 : settings.ntpTime);
        GP.SPAN(getNtpState(), GP_CENTER, "syncStatus", UI_INFO_COLOR); //описание
        GP.HR(UI_LINE_COLOR);
        GP.BUTTON("syncCheck", "Синхронизировать сейчас", "", (!ntpGetRunStatus()) ? GP_GRAY : UI_BUTTON_COLOR, "", (boolean)(!ntpGetRunStatus()));
        GP.BLOCK_END();

        GP.UPDATE_CLICK("syncStatus", "syncCheck");

        GP.BLOCK_BEGIN(GP_THIN, "", "Регион погоды", UI_BLOCK_COLOR);
        GP.SELECT("weatherCity", String(weatherCityList) + ",- По координатам -", settings.weatherCity, 0, false, true);
        GP.BREAK();
        M_BOX(GP_CENTER, "209px",
              GP.NUMBER_F("weatherLat", "Широта", (settings.weatherCity < WEATHER_CITY_ARRAY) ? weatherCoordinatesList[0][settings.weatherCity] : settings.weatherLat, 4, "", (boolean)(settings.weatherCity < WEATHER_CITY_ARRAY));
              GP.NUMBER_F("weatherLon", "Долгота", (settings.weatherCity < WEATHER_CITY_ARRAY) ? weatherCoordinatesList[1][settings.weatherCity] : settings.weatherLon, 4, "", (boolean)(settings.weatherCity < WEATHER_CITY_ARRAY));
             );
        GP.SPAN(getWeatherState(), GP_CENTER, "syncWeather", UI_INFO_COLOR); //описание
        GP.HR(UI_LINE_COLOR);
        GP.BUTTON("weatherUpdate", "Обновить погоду", "", (!weatherGetRunStatus()) ? GP_GRAY : UI_BUTTON_COLOR, "", (boolean)(!weatherGetRunStatus()));
        GP.BLOCK_END();

        GP.UPDATE_CLICK("syncWeather", "weatherUpdate");
      }
    }

    if (!(device.failure & 0x80) && device.failure && failureWarn) {
      updateList += F(",mainFailWarn");
      GP.ALERT("mainFailWarn", "Внимание! Обнаружен сбой при запуске устройства!\nПодробнее во вкладке - Об устройстве.");
    }

    wirelessResetFoundState();

    updateList += F(",extReload,extFound");
    GP.RELOAD("extReload");
    GP.CONFIRM("extFound");

    GP.UPDATE(updateList);
    GP.UI_END(); //завершить окно панели управления
  }
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
    if (ui.clickSub("sync")) {
      if (ui.click("syncGmt")) {
        settings.ntpGMT = ui.getInt("syncGmt") - 12;
        if (settings.ntpSync && (ntpGetSyncStatus())) {
          ntpRequest(); //запросить текущее время
          syncState = 0; //сбросили флаг синхронизации
        }
        memory.update(); //обновить данные в памяти
      }
      if (ui.clickBool("syncAuto", settings.ntpSync)) {
        if (settings.ntpSync && (ntpGetSyncStatus())) {
          ntpRequest(); //запросить текущее время
          syncState = 0; //сбросили флаг синхронизации
        }
        memory.update(); //обновить данные в памяти
      }
      if (ui.clickBool("syncDst", settings.ntpDst)) {
        if (settings.ntpSync && (ntpGetSyncStatus())) {
          ntpRequest(); //запросить текущее время
          syncState = 0; //сбросили флаг синхронизации
        }
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("syncTime")) {
        if (ntpGetSyncStatus()) {
          ntpRequest(); //запросить текущее время
          syncState = 0; //сбросили флаг синхронизации
        }
        else {
          mainTime = ui.getSystemTime(); //запросить время браузера
          mainDate = ui.getSystemDate(); //запросить дату браузера
          busSetComand(WRITE_TIME_DATE);
          if (rtcGetFoundStatus()) busSetComand(WRITE_RTC_TIME); //отправить время в RTC
        }
      }

      if (ui.click("syncHost")) {
        if (ui.getString("syncHost").length() > 0) strncpy(settings.host, ui.getString("syncHost").c_str(), 20); //копируем себе
        else strncpy(settings.host, DEFAULT_NTP_HOST, 20); //установить хост по умолчанию
        settings.host[19] = '\0'; //устанавливаем последний символ
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("syncPer")) {
        settings.ntpTime = ui.getInt("syncPer");
        if (settings.ntpTime > (sizeof(ntpSyncTime) - 1)) settings.ntpTime = sizeof(ntpSyncTime) - 1;
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("syncCheck")) {
        ntpRequest(); //запросить текущее время
        syncState = 0; //сбросили флаг синхронизации
      }

      if (ui.click("syncAging")) {
        if (rtcGetFoundStatus()) {
          rtc_aging = constrain(ui.getInt("syncAging"), -128, 127);
          busSetComand(WRITE_RTC_AGING);
        }
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("weather")) {
      if (ui.click("weatherCity")) {
        settings.weatherCity = ui.getInt("weatherCity");
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("weatherLat")) {
        settings.weatherLat = (ui.getString("weatherLat").length()) ? ui.getFloat("weatherLat") : NAN;
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("weatherLon")) {
        settings.weatherLon = (ui.getString("weatherLon").length()) ? ui.getFloat("weatherLon") : NAN;
        memory.update(); //обновить данные в памяти
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
          busSetComand(WRITE_SELECT_ALARM, ALARM_VOLUME);
          if ((!alarm_data[alarm.now][ALARM_DATA_RADIO] || !deviceInformation[RADIO_ENABLE]) && alarm_data[alarm.now][ALARM_DATA_MODE]) busSetComand(WRITE_TEST_ALARM_VOL);
        }
        else if (playbackTimer > -1) playbackTimer = 0; //сбросили воспроизведение
        if (ui.click("alarmSoundType")) {
          alarm_data[alarm.now][ALARM_DATA_RADIO] = ui.getBool("alarmSoundType");
          busSetComand(WRITE_SELECT_ALARM, ALARM_RADIO);
          busSetComand(WRITE_SELECT_ALARM, ALARM_SOUND);
          busSetComand(WRITE_SELECT_ALARM, ALARM_VOLUME);
        }
        if (ui.click("alarmSound")) {
          if (!alarm_data[alarm.now][ALARM_DATA_RADIO]) {
            alarm_data[alarm.now][ALARM_DATA_SOUND] = ui.getInt("alarmSound");
            if ((!deviceInformation[PLAYER_TYPE] || alarm_data[alarm.now][ALARM_DATA_VOLUME]) && (!deviceInformation[RADIO_ENABLE] || !radioSettings.powerState)) {
              busSetComand(WRITE_TEST_ALARM_SOUND);
              playbackTimer = 5;
            }
            busSetComand(WRITE_SELECT_ALARM, ALARM_SOUND);
          }
        }
        if (ui.click("alarmRadio")) {
          if (alarm_data[alarm.now][ALARM_DATA_RADIO]) {
            alarm_data[alarm.now][ALARM_DATA_STATION] = ui.getInt("alarmRadio");
            busSetComand(WRITE_SELECT_ALARM, ALARM_SOUND);
          }
        }
        if (ui.clickTime("alarmTime", alarmTime)) {
          alarm_data[alarm.now][ALARM_DATA_HOUR] = alarmTime.hour;
          alarm_data[alarm.now][ALARM_DATA_MINS] = alarmTime.minute;
          busSetComand(WRITE_SELECT_ALARM, ALARM_TIME);
        }
        if (ui.click("alarmMode")) {
          alarm_data[alarm.now][ALARM_DATA_MODE] = ui.getInt("alarmMode");
          busSetComand(WRITE_SELECT_ALARM, ALARM_MODE);
        }
        if (ui.clickSub("alarmDay")) {
          uint8_t day = constrain(ui.clickNameSub(1).toInt(), 1, 7);
          if (ui.getBool(String("alarmDay/") + day)) alarm_data[alarm.now][ALARM_DATA_DAYS] |= (0x01 << day);
          else alarm_data[alarm.now][ALARM_DATA_DAYS] &= ~(0x01 << day);
          busSetComand(WRITE_SELECT_ALARM, ALARM_DAYS);
        }
        if (ui.click("alarmDel") && !alarm.reload) {
          if (alarm.all > 1) {
            alarm.all--;
            busSetComand(DEL_ALARM, alarm.now);
            busSetComand(READ_ALARM_ALL);
          }
          else {
            alarm_data[alarm.now][ALARM_DATA_MODE] = 0;
            busSetComand(WRITE_SELECT_ALARM, ALARM_MODE);
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
            busSetComand(NEW_ALARM);
            busSetComand(READ_ALARM_ALL);
          }
        }
        if (ui.click("alarmDis")) {
          busSetComand(WRITE_ALARM_DISABLE);
        }
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("fast")) {
      if (ui.clickInt("fastDot", fastSettings.dotMode)) {
        busSetComand(WRITE_FAST_SET, FAST_DOT_MODE);
      }
      if (ui.clickInt("fastFlip", fastSettings.flipMode)) {
        busSetComand(WRITE_FAST_SET, FAST_FLIP_MODE);
      }
      if (ui.clickInt("fastSecsFlip", fastSettings.secsMode)) {
        busSetComand(WRITE_FAST_SET, FAST_SECS_MODE);
      }
      if (ui.clickInt("fastBackl", fastSettings.backlMode)) {
        busSetComand(WRITE_FAST_SET, FAST_BACKL_MODE);
      }
      if (ui.click("fastColor")) {
        uint8_t color = constrain(ui.getInt("fastColor"), 0, 28);
        fastSettings.backlColor = (color > 25) ? (color + 227) : (color * 10);
        busSetComand(WRITE_FAST_SET, FAST_BACKL_COLOR);
      }

      if (ui.clickSub("fastMainTab")) {
        navMainTab = constrain(ui.clickNameSub(1).toInt(), 0, 1);
      }
      if (ui.clickSub("fastInfoTab")) {
        navInfoTab = constrain(ui.clickNameSub(1).toInt(), 0, 1);
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("main")) {
      if (ui.clickDate("mainDate", mainDate)) {
        busSetComand(WRITE_DATE);
      }
      if (ui.clickTime("mainTime", mainTime)) {
        busSetComand(WRITE_TIME);
      }
      if (ui.clickBool("mainTimeFormat", mainSettings.timeFormat)) {
        busSetComand(WRITE_MAIN_SET, MAIN_TIME_FORMAT);
      }
      if (ui.clickBool("mainGlitch", mainSettings.glitchMode)) {
        busSetComand(WRITE_MAIN_SET, MAIN_GLITCH_MODE);
      }

      if (ui.clickInt("mainIndiBrtDay", mainSettings.indiBrightDay)) {
        busSetComand(WRITE_MAIN_SET, MAIN_INDI_BRIGHT_D);
      }
      if (ui.clickInt("mainIndiBrtNight", mainSettings.indiBrightNight)) {
        busSetComand(WRITE_MAIN_SET, MAIN_INDI_BRIGHT_N);
      }
      if (ui.clickInt("mainBurnFlip", mainSettings.burnMode)) {
        busSetComand(WRITE_MAIN_SET, MAIN_BURN_MODE);
      }
      if (ui.clickInt("mainBurnTime", mainSettings.burnTime)) {
        busSetComand(WRITE_MAIN_SET, MAIN_BURN_TIME);
      }

      if (ui.click("mainAutoShow")) {
        if (ui.getBool("mainAutoShow")) mainSettings.autoShowTime &= 0x7F;
        else mainSettings.autoShowTime |= 0x80;
        busSetComand(WRITE_MAIN_SET, MAIN_AUTO_SHOW_TIME);
      }
      if (ui.click("mainAutoShowTime")) {
        if (mainSettings.autoShowTime & 0x80) {
          mainSettings.autoShowTime = ui.getInt("mainAutoShowTime") | 0x80;
        }
        else {
          mainSettings.autoShowTime = ui.getInt("mainAutoShowTime");
          busSetComand(WRITE_MAIN_SET, MAIN_AUTO_SHOW_TIME);
        }
      }
      if (ui.clickInt("mainAutoShowFlip", mainSettings.autoShowFlip)) {
        busSetComand(WRITE_MAIN_SET, MAIN_AUTO_SHOW_FLIP);
      }
      if (ui.click("mainTempCorrect")) {
        mainSettings.tempCorrect = constrain((int16_t)(ui.getFloat("mainTempCorrect") * 10), -127, 127);
        busSetComand(WRITE_MAIN_SET, MAIN_TEMP_CORRECT);
      }

      if (ui.clickInt("mainTimeBrightS", mainSettings.timeBrightStart)) {
        busSetComand(WRITE_MAIN_SET, MAIN_TIME_BRIGHT_S);
      }
      if (ui.clickInt("mainTimeBrightE", mainSettings.timeBrightEnd)) {
        busSetComand(WRITE_MAIN_SET, MAIN_TIME_BRIGHT_E);
      }

      if (ui.clickInt("mainHourSoundS", mainSettings.timeHourStart)) {
        busSetComand(WRITE_MAIN_SET, MAIN_TIME_HOUR_S);
      }
      if (ui.clickInt("mainHourSoundE", mainSettings.timeHourEnd)) {
        busSetComand(WRITE_MAIN_SET, MAIN_TIME_HOUR_E);
      }

      if (ui.clickInt("mainSleepD", mainSettings.timeSleepDay)) {
        busSetComand(WRITE_MAIN_SET, MAIN_TIME_SLEEP_D);
      }
      if (ui.clickInt("mainSleepN", mainSettings.timeSleepNight)) {
        busSetComand(WRITE_MAIN_SET, MAIN_TIME_SLEEP_N);
      }

      if (ui.clickInt("mainDotBrtDay", mainSettings.dotBrightDay)) {
        busSetComand(WRITE_MAIN_SET, MAIN_DOT_BRIGHT_D);
      }
      if (ui.clickInt("mainDotBrtNight", mainSettings.dotBrightNight)) {
        busSetComand(WRITE_MAIN_SET, MAIN_DOT_BRIGHT_N);
      }

      if (ui.clickInt("mainBacklBrightDay", mainSettings.backlBrightDay)) {
        busSetComand(WRITE_MAIN_SET, MAIN_BACKL_BRIGHT_D);
      }
      if (ui.clickInt("mainBacklBrightNight", mainSettings.backlBrightNight)) {
        busSetComand(WRITE_MAIN_SET, MAIN_BACKL_BRIGHT_N);
      }

      if (ui.click("mainSound")) {
        mainSettings.knockSound = ui.getBool("mainSound");
        busSetComand(WRITE_MAIN_SET, MAIN_KNOCK_SOUND);
      }
      if (ui.clickInt("mainVoice", mainSettings.voiceSound)) {
        busSetComand(WRITE_MAIN_SET, MAIN_VOICE_SOUND);
        busSetComand(WRITE_TEST_MAIN_VOICE);
      }
      if (ui.clickInt("mainSoundVol", mainSettings.volumeSound)) {
        busSetComand(WRITE_MAIN_SET, MAIN_VOLUME_SOUND);
        busSetComand(WRITE_TEST_MAIN_VOL);
      }
      if (ui.click("mainHourTemp")) {
        if (ui.getBool("mainHourTemp")) mainSettings.hourSound |= 0x80;
        else mainSettings.hourSound &= ~0x80;
        busSetComand(WRITE_MAIN_SET, MAIN_HOUR_SOUND);
      }
      if (ui.click("mainHourSound")) {
        mainSettings.hourSound = (mainSettings.hourSound & 0x80) | constrain(ui.getInt("mainHourSound"), 0, 3);
        busSetComand(WRITE_MAIN_SET, MAIN_HOUR_SOUND);
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("ext")) {
      if (ui.clickSub("extShowMode")) {
        uint8_t pos = ui.clickNameSub(1).toInt();
        uint8_t mode = ui.getInt(String("extShowMode/") + pos);
        extendedSettings.autoShowModes[pos] = mode;
        busSetComand(WRITE_EXTENDED_SHOW_MODE, pos);
      }
      if (ui.clickSub("extShowTime")) {
        uint8_t pos = ui.clickNameSub(1).toInt();
        uint8_t time = ui.getInt(String("extShowTime/") + pos);
        extendedSettings.autoShowTimes[pos] = time;
        busSetComand(WRITE_EXTENDED_SHOW_TIME, pos);
      }

      if (ui.click("extAlarmTimeout")) {
        extendedSettings.alarmTime = ui.getInt("extAlarmTimeout");
        busSetComand(WRITE_EXTENDED_ALARM, EXT_ALARM_TIMEOUT);
      }
      if (ui.click("extAlarmWaitTime")) {
        extendedSettings.alarmWaitTime = ui.getInt("extAlarmWaitTime");
        busSetComand(WRITE_EXTENDED_ALARM, EXT_ALARM_WAIT);
      }
      if (ui.click("extAlarmSoundTime")) {
        extendedSettings.alarmSoundTime = ui.getInt("extAlarmSoundTime");
        busSetComand(WRITE_EXTENDED_ALARM, EXT_ALARM_TIMEOUT_SOUND);
      }
      if (ui.click("extAlarmDotOn")) {
        extendedSettings.alarmDotOn = ui.getInt("extAlarmDotOn");
        busSetComand(WRITE_EXTENDED_ALARM, EXT_ALARM_DOT_ON);
      }
      if (ui.click("extAlarmDotWait")) {
        extendedSettings.alarmDotWait = ui.getInt("extAlarmDotWait");
        busSetComand(WRITE_EXTENDED_ALARM, EXT_ALARM_DOT_WAIT);
      }

      if (ui.click("extDeviceName")) {
        strncpy(settings.name, ui.getString("extDeviceName").c_str(), 20); //копируем себе
        settings.name[19] = '\0'; //устанавливаем последний символ

        for (uint8_t i = 0; i < (MAX_CLOCK * 2); i += 2) {
          if (settings.multi[i][0] != '\0') {
            if (WiFi.localIP().toString().equals(settings.multi[i])) {
              strncpy(settings.multi[i + 1], settings.name, 20); //копируем себе
              settings.multi[i + 1][19] = '\0'; //устанавливаем последний символ
              break;
            }
          }
          else break;
        }

        memory.update(); //обновить данные в памяти
      }
      if (ui.clickBool("extDeviceAp", settings.nameAp)) {
        memory.update(); //обновить данные в памяти
      }
      if (ui.clickBool("extDeviceMenu", settings.nameMenu)) {
        memory.update(); //обновить данные в памяти
      }
      if (ui.clickBool("extDevicePrefix", settings.namePrefix)) {
        memory.update(); //обновить данные в памяти
      }
      if (ui.clickBool("extDevicePostfix", settings.namePostfix)) {
        memory.update(); //обновить данные в памяти
      }

      if (ui.clickSub("extMultiDel")) {
        uint8_t multiNum = constrain(ui.clickNameSub(1).toInt(), 0, ((MAX_CLOCK - 1) * 2));
        if (WiFi.localIP().toString().equals(settings.multi[multiNum])) {
          for (uint8_t i = 0; i < (MAX_CLOCK * 2); i++) settings.multi[i][0] = '\0';
        }
        else {
          for (; multiNum < ((MAX_CLOCK - 1) * 2); multiNum++) {
            strncpy(settings.multi[multiNum], settings.multi[multiNum + 2], 20); //копируем себе
          }
          settings.multi[((MAX_CLOCK - 1) * 2)][0] = '\0'; //устанавливаем последний символ
          settings.multi[((MAX_CLOCK - 1) * 2) + 1][0] = '\0'; //устанавливаем последний символ

          if (settings.multi[2][0] == '\0') {
            settings.multi[0][0] = '\0'; //устанавливаем последний символ
            settings.multi[1][0] = '\0'; //устанавливаем последний символ
          }
        }
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("extMultiAdd")) {
        if (buffMultiIp[0] != '\0') { //если строка не пустая
          if (!WiFi.localIP().toString().equals(buffMultiIp)) { //если не собственный адрес
            for (uint8_t i = 0; i < (MAX_CLOCK * 2); i += 2) {
              if (settings.multi[i][0] == '\0') { //если ячейка не заполнена
                if (!i) { //если список пуст
                  strncpy(settings.multi[i], WiFi.localIP().toString().c_str(), 20); //копируем себе
                  settings.multi[i][19] = '\0'; //устанавливаем последний символ
                  strncpy(settings.multi[i + 1], settings.name, 20); //копируем себе
                  settings.multi[i + 1][19] = '\0'; //устанавливаем последний символ
                  i += 2; //сместили указатель
                }
                strncpy(settings.multi[i], buffMultiIp, 20); //копируем себе
                settings.multi[i][19] = '\0'; //устанавливаем последний символ
                strncpy(settings.multi[i + 1], buffMultiName, 20); //копируем себе
                settings.multi[i + 1][19] = '\0'; //устанавливаем последний символ
                memory.update(); //обновить данные в памяти
                break;
              }
              else if (String(settings.multi[i]).equals(buffMultiIp)) {
                break;
              }
            }
          }
        }
      }

      if (ui.click("extMultiIp")) {
        strncpy(buffMultiIp, ui.getString("extMultiIp").c_str(), 20); //копируем себе
        buffMultiIp[19] = '\0'; //устанавливаем последний символ
      }
      if (ui.click("extMultiName")) {
        strncpy(buffMultiName, ui.getString("extMultiName").c_str(), 20); //копируем себе
        buffMultiName[19] = '\0'; //устанавливаем последний символ
      }

      if (ui.click("extReset")) {
        if (ui.getBool("extReset")) {
          resetMainSettings(); //устанавливаем настройки по умолчанию
          memory.updateNow(); //обновить данные в памяти
          busRebootDevice(DEVICE_RESET);
        }
      }
      if (ui.click("extReboot")) {
        if (ui.getBool("extReboot")) {
          memory.updateNow(); //обновить данные в памяти
          busRebootDevice(DEVICE_REBOOT);
        }
      }

      if (ui.click("extFound")) {
        if (ui.getBool("extFound")) {
          wirelessSetNewId();
          wirelessSetData(wireless_found_buffer);
          memory.update(); //обновить данные в памяти
        }
        else wirelessResetFoundState();
      }

      if (ui.click("extClear")) {
        settings.ssid[0] = '\0'; //устанавливаем последний символ
        settings.pass[0] = '\0'; //устанавливаем последний символ
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("extScan")) {
        if (wifiGetScanAllowStatus()) wifiStartScanNetworks(); //начинаем поиск
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
                busSetComand(WRITE_TIMER_SET);
              }
              else timer.mode = 1;
              busSetComand(WRITE_TIMER_TIME);
            }
            else if (timer.mode & 0x80) {
              timer.mode &= 0x7F;
              busSetComand(WRITE_TIMER_STATE);
            }
            busSetComand(WRITE_TIMER_MODE);
            break;
          case 1: //остановка
            if (timer.mode) {
              timer.mode = 0;
              timer.count = 0;
              busSetComand(WRITE_TIMER_TIME);
              busSetComand(SET_UPDATE);
            }
            else {
              timer.hour = timer.mins = timer.secs = 0;
            }
            break;
          case 2: //пауза
            if (timer.mode && !(timer.mode & 0x80)) {
              timer.mode |= 0x80;
              busSetComand(WRITE_TIMER_STATE);
            }
            break;
        }
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("climate")) {
      if (ui.clickInt("climateMainSens", extendedSettings.tempMainSensor)) {
        busSetComand(WRITE_EXTENDED_SENSOR_SET, EXT_SENSOR_MAIN);
      }
      if (ui.clickInt("climateHourSens", extendedSettings.tempHourSensor)) {
        busSetComand(WRITE_EXTENDED_SENSOR_SET, EXT_SENSOR_HOUR);
      }
      if (ui.clickInt("climateCorrectType", extendedSettings.tempCorrectSensor)) {
        busSetComand(WRITE_EXTENDED_SENSOR_SET, EXT_SENSOR_CORRECT);
      }

      if (ui.clickInt("climateBar", settings.climateBar)) {
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("climateChart")) {
        sensorChart = ui.getInt("climateChart");
      }
      if (ui.clickSub("climateSend")) {
        int num = ui.clickNameSub(1).toInt();
        settings.climateSend[constrain(num, 0, 1)] = ui.getInt(String("climateSend/") + num) + 1;
        sensorSendData(settings.climateSend[constrain(num, 0, 1)]); //отправить данные
        memory.update(); //обновить данные в памяти
      }

      if (ui.clickInt("climateTime", settings.climateTime)) {
        memory.update(); //обновить данные в памяти
      }
      if (ui.clickBool("climateAvg", settings.climateAvg)) {
        memory.update(); //обновить данные в памяти
      }

      if (ui.click("climateWarn")) {
        if (ui.getBool("climateWarn")) {
          settings.climateChart = sensorChart;
          climateUpdate(CLIMATE_RESET);
          memory.update(); //обновить данные в памяти
        }
        sensorChart = SENS_MAX_DATA;
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("radio")) {
      if (ui.click("radioPower")) {
        radioSettings.powerState = ui.getBool("radioPower");
        busSetComand(WRITE_RADIO_POWER);
      }
      if (radioSettings.powerState) {
        if (ui.click("radioMode")) {
          busSetComand(WRITE_RADIO_MODE);
        }
        if (ui.click("radioSeekDown")) {
          busSetComand(RADIO_SEEK_DOWN);
        }
        if (ui.click("radioSeekUp")) {
          busSetComand(RADIO_SEEK_UP);
        }
        if (ui.click("radioFreqDown")) {
          busSetComand(READ_RADIO_FREQ);
          busSetComand(RADIO_FREQ_DOWN);
        }
        if (ui.click("radioFreqUp")) {
          busSetComand(READ_RADIO_FREQ);
          busSetComand(RADIO_FREQ_UP);
        }
        if (ui.click("radioVol")) {
          radioSettings.volume = constrain(ui.getInt("radioVol"), 0, 15);
          busSetComand(WRITE_RADIO_VOL);
        }
        if (ui.click("radioFreq")) {
          uint16_t stationFreq = (uint16_t)(ui.getFloat("radioFreq") * 10);
          radioSettings.stationsFreq = constrain(stationFreq, 870, 1080);
          busSetComand(WRITE_RADIO_FREQ);
        }
      }
      if (ui.clickSub("radioCh")) {
        uint8_t stationNum = constrain(ui.clickNameSub(1).toInt(), 0, 9);
        if (radioSettings.stationsSave[stationNum]) {
          radioSettings.stationsFreq = radioSettings.stationsSave[stationNum];
          if (!radioSettings.powerState) {
            radioSettings.powerState = true;
            busSetComand(WRITE_RADIO_POWER);
          }
          busSetComand(WRITE_RADIO_FREQ);
        }
      }
      if (ui.clickSub("radioSta")) {
        uint8_t stationNum = constrain(ui.clickNameSub(1).toInt(), 0, 9);
        uint16_t stationFreq = (uint16_t)(ui.getFloat(String("radioSta/") + stationNum) * 10);
        stationFreq = (stationFreq) ? constrain(stationFreq, 870, 1080) : 0;
        if (radioSettings.stationsSave[stationNum] != stationFreq) {
          radioSettings.stationsSave[stationNum] = stationFreq;
          busSetComand(WRITE_RADIO_STA, stationNum);
        }
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
      if (ui.update("syncStatus")) { //если было обновление
        ui.answer(getNtpState());
      }
      if (ui.update("syncWeather")) { //если было обновление
        ui.answer(getWeatherState());
      }

      if (ui.update("syncUpdate")) { //если было обновление
        ui.answer((busRebootFail()) ? " " : ((!busRebootState()) ? getUpdaterState() : "<big><b>Выполняется перезагрузка, подождите...</b></big>"));
      }
      if (ui.update("syncReboot") && !busRebootState()) { //если было обновление
        ui.answer((busRebootFail()) ? " " : "<big><b>Перезагрузка завершена!</b></big>");
      }
      if (ui.update("syncWarn") && !updaterState() && !busRebootState()) { //если было обновление
        ui.answer((busRebootFail()) ? "<big><b>Ошибка перезагрузки!</b></big>" : " ");
      }

      if (ui.update("syncNetwork")) { //если было обновление
        ui.answer(wifiGetConnectState());
      }
      if (ui.update("syncReload") && wifiGetScanCompleteStatus()) { //если было обновление
        ui.answer(1);
        wifiResetScanCompleteStatus();
      }

      if (ui.update("syncAging")) { //если было обновление
        ui.answer(rtc_aging);
      }
    }
    //--------------------------------------------------------------------
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

      if (ui.update("bar_wifi")) { //если было обновление
        ui.answer(constrain(2 * (WiFi.RSSI() + 100), 0, 100));
      }
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("main")) {
      if (ui.update("mainTimerState")) { //если было обновление
        ui.answer(getTimerState());
        if (!timer.mode) busSetComand(READ_TIMER_STATE);
      }
      if (ui.update("mainTimer")) { //если было обновление
        ui.answer(convertTimerTime());
      }

      if (ui.update("mainReload") && (alarm.reload >= 2)) { //если было обновление
        ui.answer(1);
        alarm.reload = 0;
      }
      if (ui.update("mainFailWarn") && (failureWarn == true)) { //если было обновление
        ui.answer(1);
        failureWarn = false;
      }
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("ext")) {
      if (ui.update("extReset")) { //если было обновление
        ui.answer(1);
      }
      if (ui.update("extReboot")) { //если было обновление
        ui.answer(1);
      }

      if (ui.update("extReload") && wirelessGetFoundSuccessState()) { //если было обновление
        ui.answer(1);
      }
      if (ui.update("extFound") && wirelessGetFoundState()) { //если было обновление
        ui.answer("Обнаружен беспроводной датчик температуры, подключить?\nUID: " + wirelessGetId(wireless_found_buffer));
      }
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("climate")) {
      if (ui.update("climateWarn")) { //если было обновление
        ui.answer(1);
      }
      if (ui.update("climateReload") && (sensorChart >= SENS_MAX_DATA)) { //если было обновление
        ui.answer(1);
        sensorChart = 0;
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
        busSetComand(READ_RADIO_POWER);
      }
      if (ui.updateSub("radioSta")) {
        uint8_t stationNum = constrain(ui.updateNameSub(1).toInt(), 0, 9);
        ui.answer((radioSettings.stationsSave[stationNum]) ? String(radioSettings.stationsSave[stationNum] / 10.0, 1) : " ");
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
      if (deviceInformation[HARDWARE_VERSION]) busSetComand(UPDATE_FIRMWARE);
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
//----------------------------Получить состояние таймера---------------------------------
String getTimerState(void) { //получить состояние таймера
  String str;
  str.reserve(50);

  str = statusTimerList[timer.mode & 0x03];
  if (((timer.mode & 0x03) == 2) && !timer.count) str += " - тревога";
  else if (timer.mode & 0x80) str += " - пауза";

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
String getTimeFromMs(uint32_t data) {
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
String StrLengthConstrain(String str, uint8_t size) {
  if (str.length() > size) {
    str.remove(size);
    str += "…";
  }
  return str;
}
//--------------------------------------------------------------------
boolean checkFsData(const char** data, int8_t size) {
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

    if (checkFsData(climateFsData, 1)) {
      climateLocal = true; //работаем локально
      Serial.println F("Script file found");
    }
    if (checkFsData(alarmFsData, 3)) {
      alarmSvgImage = true; //работаем локально
      Serial.println F("Alarm svg files found");
    }
    if (checkFsData(timerFsData, 5)) {
      timerSvgImage = true; //работаем локально
      Serial.println F("Timer svg files found");
    }
    if (checkFsData(radioFsData, 6)) {
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
}
//--------------------------------------------------------------------
void resetMainSettings(void) {
  strncpy(settings.host, DEFAULT_NTP_HOST, 20); //установить хост по умолчанию
  settings.host[19] = '\0'; //устанавливаем последний символ

  settings.nameAp = DEFAULT_NAME_AP; //установить отображение имени после названия точки доступа wifi по умолчанию
  settings.nameMenu = DEFAULT_NAME_MENU; //установить отображение имени в меню по умолчанию
  settings.namePrefix = DEFAULT_NAME_PREFIX; //установить отображение имени перед названием вкладки по умолчанию
  settings.namePostfix = DEFAULT_NAME_POSTFIX; //установить отображение имени после названием вкладки по умолчанию

  strncpy(settings.name, DEFAULT_NAME, 20); //установить имя по умолчанию
  settings.name[19] = '\0'; //устанавливаем последний символ

  settings.weatherCity = DEFAULT_WEATHER_CITY; //установить город по умолчанию
  settings.weatherLat = NAN; //установить широту по умолчанию
  settings.weatherLon = NAN; //установить долготу по умолчанию

  for (uint8_t i = 0; i < sizeof(settings.wirelessId); i++) settings.wirelessId[i] = 0; //сбрасываем id беспроводного датчика
  for (uint8_t i = 0; i < sizeof(settings.climateSend); i++) settings.climateSend[i] = SENS_MAIN; //сбрасываем типы датчиков
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
void sensorSendData(uint8_t sens) {
  if (!deviceInformation[SENS_TEMP] && (settings.climateSend[0] == sens)) busSetComand(WRITE_SENS_1_DATA, settings.climateSend[0]); //отправить данные
  if (settings.climateSend[1] == sens) busSetComand(WRITE_SENS_2_DATA, settings.climateSend[1]); //отправить данные
  if (settings.climateChart == sens) climateUpdate(CLIMATE_UPDATE); //обновляем показания графиков
}
//--------------------------------------------------------------------
void sensorUpdateData(void) {
  if (deviceInformation[SENS_TEMP]) busSetComand(WRITE_CHECK_SENS);
  else sens.update |= SENS_EXT;
  sens.status = 0;
}
//--------------------------------------------------------------------
void sensorInitData(void) {
  static boolean first_start = false;
  if (!first_start) sens.search = sens.status; //установить показания датчиков
  else sens.search |= 0x80;
  first_start = true;
}
//--------------------------------------------------------------------
boolean sensorAvaibleData(void) {
  return (boolean)((sens.search & 0x7F) || !sens.search);
}
//--------------------------------------------------------------------
boolean sensorGetValidStatus(void) {
  return (boolean)(climateGetChartTemp() != 0x7FFF);
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
    light_now = 2; //сбросили текущую яркость
    sens.temp[SENS_WEATHER] = 0x7FFF; //сбросили температуру погоды
    sens.hum[SENS_WEATHER] = 0; //сбросили влажность погоды
    sens.press[SENS_WEATHER] = 0; //сбросили давление погоды
  }
  else {
    sens.temp[SENS_WEATHER] = map(mainTime.minute, 0, 59, weatherArrMain[0][time_now], weatherArrMain[0][time_next]); //температура погоды
    sens.hum[SENS_WEATHER] = map(mainTime.minute, 0, 59, weatherArrMain[1][time_now], weatherArrMain[1][time_next]) / 10; //влажность погоды
    sens.press[SENS_WEATHER] = map(mainTime.minute, 0, 59, weatherArrExt[0][time_now], weatherArrExt[0][time_next]) / 10; //давление погоды
  }

  if (!deviceInformation[LIGHT_SENS_ENABLE]) { //если сенсор яркости освещения не используется
    if (lightState != light_now) { //если яркость изменилась
      lightState = light_now; //установили состояние яркости
      busSetComand(WRITE_CHANGE_BRIGHT); //оправка состояния яркости
    }
  }

  sensorSendData(SENS_WEATHER); //отправить данные
}
//--------------------------------------------------------------------
void timeUpdate(void) {
  if ((millis() - secondsTimer) >= 1000) {
    if (!secondsTimer) secondsTimer = millis();
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

        if (!(mainTime.minute % 15) && rtcGetFoundStatus() && (!settings.ntpSync || !ntpGetSyncStatus())) busSetComand(READ_RTC_TIME); //отправить время в RTC
        else busSetComand(READ_TIME_DATE, 0); //прочитали время из часов

        if (settings.ntpSync) {
          if (!settings.ntpDst) {
            if (!syncTimer) {
              syncTimer = ntpSyncTime[settings.ntpTime];
              ntpRequest(); //запросить время с ntp сервера
            }
            else syncTimer--;
          }
          else {
            if (!(mainTime.minute % ntpSyncTime[(settings.ntpDst && (settings.ntpTime > 2)) ? 2 : settings.ntpTime])) {
              ntpRequest(); //запросить время с ntp сервера
            }
          }
        }
      }
      if (sensorAvaibleData()) {
        if (!sensorTimer) {
          sensorTimer = 59;
          sensorUpdateData();
        }
        else sensorTimer--;
      }
      secondsTimer += 1000; //прибавили секунду
    }
    if (timer.mode) busSetComand(READ_TIMER_TIME);
    if (!waitTimer) { //если пришло время опросить статус часов
      waitTimer = 4; //установили таймер ожидания
      busUpdateClockStatus(); //обновить статус часов
      busSetComand(READ_STATUS); //запрос статуса часов
    }
    else waitTimer--;
    if (playbackTimer > -1) {
      if (!playbackTimer) busSetComand(WRITE_STOP_SOUND); //остановка воспроизведения
      playbackTimer--;
    }
#if STATUS_LED == 1
    if (!wifiGetConnectStatus() && wifiGetConnectWaitStatus()) digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); //мигаем индикацией
#elif STATUS_LED == 2
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); //мигаем индикацией
#endif
  }

  if (ntpUpdate()) { //обработка ntp
    if (settings.ntpSync || !syncState) {
      syncTimer = ntpSyncTime[settings.ntpTime];
      busSetComand(SYNC_TIME_DATE); //проверить и отправить время ntp сервера
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
          case STATUS_UPDATE_MAIN_SET: busSetComand(READ_MAIN_SET); break;
          case STATUS_UPDATE_FAST_SET: busSetComand(READ_FAST_SET); break;
          case STATUS_UPDATE_RADIO_SET: busSetComand(READ_RADIO_SET); break;
          case STATUS_UPDATE_ALARM_SET: busSetComand(READ_ALARM_ALL); break;
          case STATUS_UPDATE_TIME_SET: busSetComand(READ_TIME_DATE, 1); break;
          case STATUS_UPDATE_SENS_DATA:
            if (deviceInformation[SENS_TEMP]) {
              busSetComand(READ_SENS_INFO);
              busSetComand(READ_SENS_DATA);
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
      busSetComand(CHECK_INTERNAL_AHT);
      busSetComand(CHECK_INTERNAL_SHT);
      busSetComand(CHECK_INTERNAL_BME);
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

  //инициализация файловой системы
  initFileSystemData();

  //сбрасываем настройки группового управления
  for (uint8_t i = 0; i < (MAX_CLOCK * 2); i++) settings.multi[i][0] = '\0';

  //устанавливаем указатель будильниака
  alarm.now = 0;

  //восстанавливаем настройки сети
  strncpy(settings.ssid, WiFi.SSID().c_str(), 64);
  settings.ssid[63] = '\0';
  strncpy(settings.pass, WiFi.psk().c_str(), 64);
  settings.pass[63] = '\0';

  //устанавливаем настройки по умолчанию
  resetMainSettings();

  //читаем настройки из памяти
  EEPROM.begin(memory.blockSize());
  memory.begin(0, 0xBF);

  //инициализируем строки
  weatherInitStr();
  wifiScanInitStr();

  //настраиваем wifi
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(true);
  wifiStartAP();

  //подключаем конструктор и запускаем веб интерфейс
  ui.setBufferSize(5000);
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

  //остановили ntp
  ntpStop();

  //запутили поиск беспроводного датчика
  wirelessStart();

  //отключились от сервера погоды
  weatherDisconnect();

  //запрашиваем настройки часов
  busSetComand(READ_MAIN_SET);
  busSetComand(READ_FAST_SET);
  busSetComand(READ_EXTENDED_SET);
  busSetComand(READ_RADIO_SET);
  busSetComand(READ_ALARM_ALL);
  busSetComand(READ_TIME_DATE, 0);
  busSetComand(READ_FAILURE);
  busSetComand(READ_DEVICE);

  busSetComand(WRITE_RTC_INIT);

  busTimerSetInterval(1500);
}
//--------------------------------------------------------------------
void loop() {
  wifiUpdate(); //обработка состояния wifi

  if (deviceInformation[HARDWARE_VERSION] == HW_VERSION) { //если связь с часами установлена
    timeUpdate(); //обработка времени
    deviceUpdate(); //обработка статусов устройства
  }

  if (wirelessUpdate()) sensorSendData(SENS_WIRELESS); //обработка беспроводного датчика

  if (!updaterFlash()) busUpdate(); //обработка шины
  else if (updaterRun()) busRebootDevice(SYSTEM_REBOOT); //загрузчик прошивки

  ui.tick(); //обработка веб интерфейса
  memory.tick(); //обработка еепром
}
