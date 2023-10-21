/*
  Arduino IDE 1.8.13 версия прошивки 1.0.7 релиз от 20.10.23
  Специльно для проекта "Часы на ГРИ v2. Альтернативная прошивка"
  Страница проекта - https://community.alexgyver.ru/threads/chasy-na-gri-v2-alternativnaja-proshivka.5843/

  Исходник - https://github.com/radon-lab/NixieClock
  Автор Radon-lab & Psyx86.

  Если не установлено ядро ESP8266, "Файл -> Настройки -> Дополнительные ссылки для Менеджера плат", в окно ввода вставляете ссылку - https://arduino.esp8266.com/stable/package_esp8266com_index.json
  Далее "Инструменты -> Плата -> Менеджер плат..." находите плату esp8266 и устанавливаете версию 2.7.4!

  В "Инструменты -> Управлять библиотеками..." необходимо предварительно установить последние версии библиотек:
  GyverPortal
  GyverNTP
  EEManager

  В "Инструменты -> Flash Size" необходимо выбрать распределение памяти в зависимости от установленного объёма FLASH:
  1МБ - FS:64KB OTA:~470KB(только обновление по OTA).
  1МБ - FS:512KB OTA:~246KB(только локальные файлы FS(в папке data должно быть только - alarm_add, alarm_set, favicon и папка gp_data(с её содержимым))).
  2МБ - FS:1MB OTA:~512KB(обновление по OTA и локальные файлы FS).
  4МБ - FS:2MB OTA:~1019KB(обновление по OTA и локальные файлы FS).
  8МБ - FS:6MB OTA:~1019KB(обновление по OTA и локальные файлы FS).

  Папку с плагином "ESP8266LittleFS" необходимо поместить в .../Program Files/Arduino/tools, затем нужно перезапустить Arduino IDE(если была запущена).
  Сначала загружаете прошивку, затем "Инструменты -> ESP8266 LittleFS Data Upload".

  Подключение к часам производится по шине I2C, так-же необходимо отключить внутреннюю подтяжку шины в часах, а внешнюю подтяжку подключить к 3.3в(подтяжка на шине должна быть строго только в одном месте!)
  Питать модуль ESP8266 от вывода 3v3 ардуино нельзя! Нужно использовать линейный стабилизатор или DC-DC преобразователь на 3.3в!
*/
#include "config.h"

#include <LittleFS.h>
#include <GyverPortal.h>
GyverPortal ui(&LittleFS);

#include <GyverNTP.h>
GyverNTP ntp(DEFAULT_GMT, 5);

struct settingsData {
  uint8_t climateTime;
  boolean climateAvg;
  boolean ntpSync;
  boolean ntpDst;
  uint32_t ntpTime;
  int8_t ntpGMT;
  char host[20];
  char ssid[20];
  char pass[20];
} settings;

#include <EEManager.h>
EEManager memory(settings);

//переменные
GPdate mainDate; //основная дата
GPtime mainTime; //основное время
GPtime alarmTime; //время будильника

boolean otaUpdate = true; //флаг запрета обновления
boolean alarmSvgImage = false; //флаг локальных изоражений будильника
boolean timerSvgImage = false; //флаг локальных изоражений таймера/секундомера
boolean radioSvgImage = false; //флаг локальных изоражений радиоприемника

boolean sensDataWait = true; //флаг ожидания температуры

uint8_t wifiStatus = WL_IDLE_STATUS; //статус соединения wifi
uint32_t wifiInterval = 5000; //интервал переподключения к wifi

boolean sendNtpTime = false; //флаг отправки времени с ntp сервера
uint8_t statusNtp = 4; //флаг состояние ntp сервера
uint8_t attemptsNtp = 0; //текущее количество попыток подключение к ntp серверу

uint8_t climateCountAvg;
uint8_t climateTimeAvg;
uint16_t climateTempAvg;
uint16_t climateHumAvg;
uint16_t climatePressAvg;
uint32_t climateTimer;

boolean climateLocal = false; //флаг локальных скриптов графика
int16_t climateArrMain[2][CLIMATE_BUFFER];
int16_t climateArrExt[1][CLIMATE_BUFFER];
uint32_t climateDates[CLIMATE_BUFFER];

#include "WIRE.h"
#include "CLOCKBUS.h"

const char *climateNamesMain[] = {"Температура", "Влажность"};
const char *climateNamesExt[] = {"Давление"};

const char *climateFsData[] = {"/gp_data/PLOT_STOCK.js"};
const char *alarmFsData[] = {"/alarm_add.svg", "/alarm_set.svg"};
const char *timerFsData[] = {"/timer_play.svg", "/timer_stop.svg", "/timer_pause.svg", "/timer_up.svg", "/timer_down.svg"};
const char *radioFsData[] = {"/radio_backward.svg", "/radio_left.svg", "/radio_right.svg", "/radio_forward.svg"};

const char *tempSensList[] = {"DS3231", "AHT", "SHT", "BMP/BME", "DS18B20", "DHT"};
const char *alarmModeList[] = {"Отключен", "Однократно", "Ежедневно", "По будням"};
const char *alarmDaysList[] = {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"};
const char *statusNtpList[] = {"Подключение...", "Ожидание ответа...", "Синхронизировано", "Нет сети", "Сервер не отвечает"};
const char *statusTimerList[] = {"Отключен", "Секундомер", "Таймер", "Ошибка"};

String dotModeList = ""; //список режимов основных разделительных точек
String backlModeList = ""; //список режимов подсветки
String alarmDotModeList = ""; //список режимов разделительных точек будильника

enum {
  NTP_NOT_STATUS,
  NTP_WAIT_ANSWER,
  NTP_SYNCED,
  NTP_STOPPED,
  NTP_ERROR,
  NTP_TRYING
};

void build(void) {
  static boolean listInit = false;

  GP.BUILD_BEGIN(UI_MAIN_THEME);
  GP.ONLINE_CHECK(); //проверять статус платы

  if (deviceInformation[HARDWARE_VERSION] && (deviceInformation[HARDWARE_VERSION] != HW_VERSION)) {
    GP.SPAN("Внимание! Эта версия веб-интерфейса не может взаимодействовать с этим устройством!", GP_CENTER, "", UI_INFO_COLOR);
    GP.SPAN("Для обновления перейдите по ссылке /ota_update", GP_CENTER, "", UI_INFO_COLOR);
    GP.BREAK();
    GP.LABEL("Clock HW: 0x" + String(deviceInformation[HARDWARE_VERSION], HEX));
    GP.BREAK();
    GP.LABEL("WebUI HW: 0x" + String(HW_VERSION, HEX));
  }
  else {
    GP.UI_MENU("Nixie clock", UI_MENU_COLOR); //начать меню

    //ссылки меню
    GP.UI_LINK("/", "Главная");
    GP.UI_LINK("/settings", "Настройки");
    if (deviceInformation[SENS_TEMP]) GP.UI_LINK("/climate", "Климат");
    if (deviceInformation[RADIO_ENABLE]) GP.UI_LINK("/radio", "Радио");
    GP.UI_LINK("/information", "О системе");
    if (otaUpdate) GP.UI_LINK("/update", "Обновление");
    GP.UI_LINK("/network", "Сетевые настройки");

    //состояние соединения
    GP.HR(UI_MENU_LINE_COLOR);
    if (deviceInformation[HARDWARE_VERSION]) {
      GP.LABEL_BLOCK("Firmware: " + String(deviceInformation[FIRMWARE_VERSION_1]) + "." + String(deviceInformation[FIRMWARE_VERSION_2]) + "." + String(deviceInformation[FIRMWARE_VERSION_3]), "", UI_MENU_FW_COLOR, 0, 1);
      GP.BREAK();
      GP.LABEL_BLOCK("Clock online", "", UI_MENU_CLOCK_1_COLOR, 0, 1);
    }
    else {
      GP.LABEL_BLOCK("Clock offline", "", UI_MENU_CLOCK_2_COLOR, 0, 1);
    }
    GP.BREAK();
    if ((statusNtp == NTP_ERROR) || (statusNtp == NTP_STOPPED)) {
      GP.LABEL_BLOCK("NTP disconnect", "", UI_MENU_NTP_3_COLOR, 0, 1);
    }
    else if (statusNtp == NTP_SYNCED) {
      GP.LABEL_BLOCK("NTP synced", "", UI_MENU_NTP_1_COLOR, 0, 1);
    }
    else {
      GP.LABEL_BLOCK("NTP connecting...", "", UI_MENU_NTP_2_COLOR, 0, 1);
    }

    if (!listInit && deviceInformation[HARDWARE_VERSION]) {
      listInit = true;
      backlModeList += "Выключена";
      if (deviceInformation[BACKL_TYPE]) {
        backlModeList += ",Статичная,Дыхание";
      }
      if (deviceInformation[BACKL_TYPE] >= 3) {
        backlModeList += ",Дыхание со сменой цвета при затухании,Бегущий огонь,Бегущий огонь со сменой цвета,Бегущий огонь с радугой,Бегущий огонь с конфетти,Волна,Волна со сменой цвета,Волна с радугой,Волна с конфетти,Плавная смена цвета,Радуга,Конфетти";
      }

      dotModeList += "Выключены,Статичные";
      if (deviceInformation[NEON_DOT] != 3) {
        dotModeList += ",Динамичные(плавно мигают)";
      }
      if (deviceInformation[NEON_DOT] == 2) {
        dotModeList += ",Маятник(неонки)";
      }
      if (deviceInformation[DOTS_PORT_ENABLE]) {
        dotModeList += ",Мигающие,Бегущие,Змейка,Резинка";
        if ((deviceInformation[DOTS_NUM] > 4) || (deviceInformation[DOTS_TYPE] == 2)) {
          dotModeList += ",Одинарный маятник";
        }
        if ((deviceInformation[DOTS_NUM] > 4) && (deviceInformation[DOTS_TYPE] == 2)) {
          dotModeList += ",Двойной маятник";
        }
      }
      alarmDotModeList = dotModeList + ",Без реакции,Мигают один раз в секунду,Мигают два раза в секунду";
    }

    //обновления блоков
    String updateList;

    updateList += "barTime";
    if (deviceInformation[SENS_TEMP]) {
      updateList += ",barTemp";
      if (sens.hum) {
        updateList += ",barHum";
      }
      if (sens.press) {
        updateList += ",barPress";
      }
    }
    GP.UI_BODY(); //начать основное окно
    GP.GRID_RESPONSIVE(600); //позволяет "отключить" таблицу при ширине экрана меньше 600px

    M_TABLE(
      "",
      GP_ALS(GP_LEFT, GP_RIGHT),
      M_TR(GP.LABEL(" "); GP.LABEL(" "); GP.LABEL_BLOCK(ui.getSystemTime().encode(), "barTime", UI_BAR_CLOCK_COLOR, 18, 1););
      M_TD(
        if (deviceInformation[SENS_TEMP]) GP.LABEL_BLOCK(String((sens.temp) ? ((sens.temp + mainSettings.tempCorrect) / 10.0) : 0, 1) + "°С", "barTemp", UI_BAR_TEMP_COLOR, 18, 1);
        if (sens.hum) GP.LABEL_BLOCK(String(sens.hum) + "%", "barHum", UI_BAR_HUM_COLOR, 18, 1);
          if (sens.press) GP.LABEL_BLOCK(String(sens.press) + "mm.Hg", "barPress", UI_BAR_PRESS_COLOR, 18, 1);
            GP.LABEL(" ");
            GP.LABEL(" ");
          );
        );
    GP.HR(UI_BAR_LINE_COLOR);

    if (ui.uri("/")) { //основная страница
      if (!alarm.set || !deviceInformation[ALARM_TYPE]) { //если не режим настройки будильника
        GP.PAGE_TITLE("Главная");
        M_GRID(
          GP.BLOCK_BEGIN(GP_THIN, "", "Настройка времени", UI_BLOCK_COLOR);
          M_BOX(GP.LABEL("Время", "", UI_LABEL_COLOR); GP.TIME("mainTime"););
          M_BOX(GP.LABEL("Дата", "", UI_LABEL_COLOR); GP.DATE("mainDate"););
          M_BOX(GP.LABEL("Формат", "", UI_LABEL_COLOR);  M_BOX(GP_RIGHT, GP.LABEL("24ч", "", UI_LABEL_COLOR);  GP.SWITCH("mainTimeFormat", mainSettings.timeFormat, UI_SWITCH_COLOR); GP.LABEL("12ч", "", UI_LABEL_COLOR);););
          GP.HR(UI_LINE_COLOR);
          M_BOX(GP.LABEL("Часовой пояс", "", UI_LABEL_COLOR); GP.SELECT("syncGmt", "GMT-12,GMT-11,GMT-10,GMT-9,GMT-8,GMT-7,GMT-6,GMT-5,GMT-4,GMT-3,GMT-2,GMT-1,GMT+0,GMT+1,GMT+2,GMT+3,GMT+4,GMT+5,GMT+6,GMT+7,GMT+8,GMT+9,GMT+10,GMT+11,GMT+12", settings.ntpGMT + 12, 0, (boolean)(ntp.status())););
          M_BOX(GP.LABEL("Автосинхронизация", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SWITCH("syncAuto", settings.ntpSync, UI_SWITCH_COLOR, (boolean)(ntp.status()));););
          M_BOX(GP.LABEL("Учитывать летнее время", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SWITCH("syncDst", settings.ntpDst, UI_SWITCH_COLOR, (boolean)(ntp.status()));););
          GP.BUTTON("syncTime", (ntp.status()) ? "Время с устройства" : "Синхронизация с сервером", "", UI_BUTTON_COLOR);
          GP.BLOCK_END();

          GP.BLOCK_BEGIN(GP_THIN, "", "Эффекты", UI_BLOCK_COLOR);
          M_BOX(GP.LABEL("Глюки", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SWITCH("mainGlitch", mainSettings.glitchMode, UI_SWITCH_COLOR);););
          M_BOX(GP.LABEL("Точки", "", UI_LABEL_COLOR); GP.SELECT("fastDot", dotModeList, fastSettings.dotMode););
          M_BOX(GP.LABEL("Минуты", "", UI_LABEL_COLOR); GP.SELECT("fastFlip", "Без анимации,Случайная смена эффектов,Плавное угасание и появление,Перемотка по порядку числа,Перемотка по порядку катодов в лампе,Поезд,Резинка,Ворота,Волна,Блики,Испарение,Игровой автомат", fastSettings.flipMode););
          M_BOX(GP.LABEL("Секунды", "", UI_LABEL_COLOR); GP.SELECT("mainSecsFlip", "Без анимации,Плавное угасание и появление,Перемотка по порядку числа,Перемотка по порядку катодов в лампе", mainSettings.secsMode, 0, (boolean)(deviceInformation[LAMP_NUM] < 6)););
          GP.HR(UI_LINE_COLOR);
          M_BOX(GP.LABEL("Подсветка", "", UI_LABEL_COLOR); GP.SELECT("fastBackl", backlModeList, fastSettings.backlMode, 0, (boolean)!deviceInformation[BACKL_TYPE]););
          M_BOX(GP.LABEL("Цвет", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER_C("fastColor", (fastSettings.backlColor < 253) ? (fastSettings.backlColor / 10) : (fastSettings.backlColor - 227), 0, 28, 1, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[BACKL_TYPE]); ); );
          GP.HR(UI_LINE_COLOR);
          M_BOX(GP_JUSTIFY, GP.LABEL((deviceInformation[PLAYER_TYPE]) ? "Озвучивать действия" : "Звук кнопок", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SWITCH("mainSound", mainSettings.knockSound, UI_SWITCH_COLOR);););
          M_BOX(GP_JUSTIFY, GP.LABEL("Громкость", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER("mainSoundVol", mainSettings.volumeSound, 0, (deviceInformation[PLAYER_TYPE] == 2) ? 9 : 30, 1, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[PLAYER_TYPE]);););
          GP.BLOCK_END();
        );
      }

      if (deviceInformation[ALARM_TYPE]) {
        if (alarm.reload >= 2) alarm.reload = 0;

        updateList += ",alarmReload";
        GP.RELOAD("alarmReload");

        GP.BLOCK_BEGIN(GP_THIN, "", "Будильник", UI_BLOCK_COLOR);
        if (alarm.set) { //если режим настройки будильника
          GP.PAGE_TITLE("Настройка будильника");

          updateList += ",alarmVol,alarmSoundType,alarmSound,alarmRadio,alarmTime,alarmMode";

          String alarmSoundList;
          for (uint8_t i = 0; i < deviceInformation[PLAYER_MAX_SOUND]; i++) {
            if (i) alarmSoundList += ',';
            alarmSoundList += String("№") + (i + 1);
          }

          String alarmRadioList;
          if (deviceInformation[RADIO_ENABLE]) {
            for (uint8_t i = 0; i < 10; i++) {
              if (i) alarmRadioList += ',';
              alarmRadioList += String("CH") + i + String(" ") + String(radioSettings.stationsSave[i] / 10.0, 1);
            }
          }
          else alarmRadioList += "Пусто";

          M_BOX(GP.LABEL("Звук", "", UI_LABEL_COLOR); GP.SELECT("alarmSoundType", (deviceInformation[RADIO_ENABLE]) ? "Мелодия, Радиостанция" : "Мелодия", (boolean)alarm_data[alarm.now][ALARM_DATA_RADIO], 0, (boolean)!deviceInformation[RADIO_ENABLE]););
          M_BOX(GP.LABEL("Мелодия", "", UI_LABEL_COLOR); GP.SELECT("alarmSound", alarmSoundList, alarm_data[alarm.now][ALARM_DATA_SOUND], 0););
          M_BOX(GP.LABEL("Радиостанция", "", UI_LABEL_COLOR); GP.SELECT("alarmRadio", alarmRadioList, alarm_data[alarm.now][ALARM_DATA_STATION], 0, (boolean)!deviceInformation[RADIO_ENABLE]););
          M_BOX(GP.LABEL("Громкость", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER("alarmVol", alarm_data[alarm.now][ALARM_DATA_VOLUME], 0, 100, 10, 0, UI_SLIDER_COLOR, (boolean)(!deviceInformation[RADIO_ENABLE] && !deviceInformation[PLAYER_TYPE])); ); ); //Ползунки

          GP.HR(UI_LINE_COLOR);
          M_BOX(GP.LABEL("Время", "", UI_LABEL_COLOR); GP.TIME("alarmTime", alarmTime););
          M_BOX(GP.LABEL("Режим", "", UI_LABEL_COLOR); GP.SELECT("alarmMode", "Выключен,Однократно,Ежедневно,По будням,Выбрать дни", alarm_data[alarm.now][ALARM_DATA_MODE]););

          GP.HR(UI_LINE_COLOR);
          GP.TABLE_BORDER(false);
          GP.TABLE_BEGIN("200px,50px,50px,50px,50px,50px,50px,50px,200px");
          GP.TR(GP_CENTER);
          GP.TD(GP_CENTER);
          GP.LABEL("");
          GP.TD(GP_CENTER);
          GP.LABEL_BLOCK("ПН", "", UI_ALARM_WEEK_1_COLOR, 0, 1);
          GP.TD(GP_CENTER);
          GP.LABEL_BLOCK("ВТ", "", UI_ALARM_WEEK_1_COLOR, 0, 1);
          GP.TD(GP_CENTER);
          GP.LABEL_BLOCK("СР", "", UI_ALARM_WEEK_1_COLOR, 0, 1);
          GP.TD(GP_CENTER);
          GP.LABEL_BLOCK("ЧТ", "", UI_ALARM_WEEK_1_COLOR, 0, 1);
          GP.TD(GP_CENTER);
          GP.LABEL_BLOCK("ПТ", "", UI_ALARM_WEEK_1_COLOR, 0, 1);
          GP.TD(GP_CENTER);
          GP.LABEL_BLOCK("СБ", "", UI_ALARM_WEEK_2_COLOR, 0, 1);
          GP.TD(GP_CENTER);
          GP.LABEL_BLOCK("ВС", "", UI_ALARM_WEEK_2_COLOR, 0, 1);
          GP.TD(GP_CENTER);
          GP.LABEL("");

          uint8_t alarmDays = alarm_data[alarm.now][ALARM_DATA_DAYS];
          GP.TR(GP_CENTER);
          GP.TD(GP_CENTER);
          GP.LABEL("");
          for (uint8_t i = 1; i < 8; i++) {
            GP.TD(GP_CENTER);
            alarmDays >>= 1;
            GP.CHECK(String("alarmDay/") + i, (boolean)(alarmDays & 0x01), UI_CHECK_COLOR);
            updateList += String(",alarmDay/") + i;
          }
          GP.TD(GP_CENTER);
          GP.LABEL("");
          GP.TABLE_END();

          boolean alarmDelStatus = (boolean)(alarm.all > 1);

          GP.HR(UI_LINE_COLOR);
          M_BOX(GP_CENTER, M_BOX(GP_CENTER, GP.BUTTON_MINI("alarmBack", (alarm.set == 1) ? "Назад" : "Добавить", "", UI_ALARM_BACK_COLOR, "", false, true); GP.BUTTON_MINI("alarmDel", (alarmDelStatus) ? ((alarm.set == 1) ? "Удалить" : "Отмена") : "Отключить", "", (alarmDelStatus) ? UI_ALARM_DEL_COLOR : UI_ALARM_DIS_COLOR, "", false, !alarmDelStatus);););
        }
        else { //иначе режим отображения
          String reloadList;

          for (uint8_t i = 0; i < alarm.all; i++) {
            uint8_t alarmHour = alarm_data[i][ALARM_DATA_HOUR];
            uint8_t alarmMins = alarm_data[i][ALARM_DATA_MINS];
            uint8_t nowWeekDay = 0;

            String alarmTime;
            alarmTime += alarmHour;
            alarmTime += ':';
            if (alarmMins < 10) alarmTime += '0';
            alarmTime += alarmMins;

            uint8_t alarmMode = alarm_data[i][ALARM_DATA_MODE];

            String alarmStatus;
            if (alarmMode >= 3) {
              boolean alarmDaysFirst = false;
              uint8_t alarmDays = (alarmMode != 3) ? alarm_data[i][ALARM_DATA_DAYS] : 0x3E;
              nowWeekDay = getWeekDay(ui.getSystemDate().year, ui.getSystemDate().month, ui.getSystemDate().day); //получить день недели;
              for (uint8_t dl = 0; dl < 7; dl++) {
                if (alarmDays & (0x01 << nowWeekDay)) {
                  nowWeekDay = dl;
                  break;
                }
                if (++nowWeekDay > 7) nowWeekDay = 1;
              }
              if (alarmMode != 3) {
                for (uint8_t dw = 0; dw < 7; dw++) {
                  alarmDays >>= 1;
                  if (alarmDays & 0x01) {
                    if (alarmDaysFirst) alarmStatus += ", ";
                    else alarmDaysFirst = true;
                    alarmStatus += alarmDaysList[dw];
                  }
                }
              }
            }
            if (alarmMode < 4) {
              alarmStatus += alarmModeList[alarmMode];
            }

            if (alarmMode) {
              GPtime nowTime = ui.getSystemTime();
              if ((nowTime.hour > alarmHour) && nowWeekDay) nowWeekDay -= 1;
              if (nowTime.hour < alarmHour) alarmHour -= nowTime.hour;
              else if (nowTime.hour != alarmHour) alarmHour = (24 - nowTime.hour) + alarmHour;
              else alarmHour = 0;

              if ((nowTime.minute > alarmMins) && alarmHour) alarmHour -= 1;
              if (nowTime.minute < alarmMins) alarmMins -= nowTime.minute;
              else if (nowTime.minute != alarmMins) alarmMins = (60 - nowTime.minute) + alarmMins;
              else alarmMins = 0;

              alarmStatus += " | Через ";
              if (nowWeekDay) {
                alarmStatus += nowWeekDay;
                alarmStatus += "д ";
              }
              if (alarmHour) {
                alarmStatus += alarmHour;
                alarmStatus += "ч ";
              }
              alarmStatus += alarmMins;
              alarmStatus += "мин";
            }

            GP.BLOCK_BEGIN(GP_THIN, "", "", UI_ALARM_BLOCK_COLOR);
            if (alarmSvgImage) {
              M_BOX(GP.LABEL(alarmTime, "", UI_ALARM_TIME_COLOR, 40, 1); GP.ICON_FILE_BUTTON(String("alarmSet/") + i, alarmFsData[1], 40, UI_ALARM_SET_COLOR););
            }
            else {
              M_BOX(GP.LABEL(alarmTime, "", UI_ALARM_TIME_COLOR, 40, 1); GP.BUTTON_MINI(String("alarmSet/") + i, "≡", "", UI_ALARM_SET_COLOR, ""););
            }
            M_BOX(GP_LEFT, GP.BOLD(alarmStatus, "", UI_ALARM_INFO_COLOR););
            GP.BLOCK_END();
            if (i) reloadList += ',';
            reloadList += String("alarmSet/") + i;
          }

          if (alarm.all < MAX_ALARMS) {
            if (alarmSvgImage) {
              GP.ICON_FILE_BUTTON("alarmAdd", alarmFsData[0], 50, UI_ALARM_ADD_COLOR);
            }
            else {
              M_BOX(GP_CENTER, GP.BUTTON("alarmAdd", "✚", "", UI_ALARM_ADD_COLOR, "80px"););
            }
          }
          GP.RELOAD_CLICK(reloadList);
        }
        GP.BLOCK_END();
      }

      if (!alarm.set || !deviceInformation[ALARM_TYPE]) { //если не режим настройки будильника
        if (deviceInformation[TIMER_ENABLE] && deviceInformation[EXT_BTN_ENABLE]) {
          updateList += ",mainTimer,mainTimerState";

          GP.BLOCK_BEGIN(GP_THIN, "", "Таймер/Секундомер", UI_BLOCK_COLOR);

          GP.BLOCK_BEGIN(GP_DIV_RAW , "280px");
          GP.BLOCK_BEGIN(GP_THIN, "", "", UI_TIMER_BLOCK_COLOR);
          GP.LABEL(getTimerState(), "mainTimerState", UI_TIMER_INFO_COLOR, 0, 1);

          GP.TABLE_BEGIN("15%,15%,15%", GP_ALS(GP_CENTER, GP_CENTER, GP_CENTER), "200px");
          GP.TR();
          for (uint8_t i = 0; i < 6; i++) {
            String btn = String("timerHour/") + i;
            GP.TD();
            if (timerSvgImage) {
              GP.ICON_FILE_BUTTON(btn, (i < 3) ? timerFsData[3] : timerFsData[4], 40, UI_TIMER_SET_COLOR);
            }
            else {
              GP.BUTTON_MINI(btn, (i < 3) ? " ▲ " : " ▼ ", "", UI_TIMER_SET_COLOR);
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
            M_BOX(GP_CENTER, GP.ICON_FILE_BUTTON("timerControl/0", timerFsData[0], 60, UI_TIMER_CTRL_COLOR); GP.ICON_FILE_BUTTON("timerControl/1", timerFsData[1], 60, UI_TIMER_CTRL_COLOR); GP.ICON_FILE_BUTTON("timerControl/2", timerFsData[2], 60, UI_TIMER_CTRL_COLOR););
          }
          else {
            M_BOX(GP_CENTER, GP.BUTTON_MINI("timerControl/0", "⠀⠀►⠀⠀", "", UI_TIMER_CTRL_COLOR, "75px"); GP.BUTTON_MINI("timerControl/1", "⠀⠀❑⠀⠀", "", UI_TIMER_CTRL_COLOR, "75px"); GP.BUTTON_MINI("timerControl/2", "⠀⠀||⠀⠀", "", UI_TIMER_CTRL_COLOR, "75px"););
          }
          GP.BLOCK_END();

          GP.UPDATE_CLICK("mainTimer", "timerHour/0,timerHour/1,timerHour/2,timerHour/3,timerHour/4,timerHour/5");
          GP.UPDATE_CLICK("mainTimer,mainTimerState", "timerControl/0,timerControl/1,timerControl/2");
        }
      }
    }
    else if (ui.uri("/settings")) { //настройки
      GP.PAGE_TITLE("Настройки");

      updateList += ",mainAutoShow,mainAutoShowTime";

      M_GRID(
        GP.BLOCK_BEGIN(GP_THIN, "", "Автопоказ", UI_BLOCK_COLOR);
        M_BOX(GP_JUSTIFY, GP.LABEL("Включить", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SWITCH("mainAutoShow", (boolean)mainSettings.autoShowTime, UI_SWITCH_COLOR);););
        M_BOX(GP_CENTER, GP.LABEL("Интервал, мин", "", UI_LABEL_COLOR);  M_BOX(GP_RIGHT, GP.SPINNER("mainAutoShowTime", mainSettings.autoShowTime, 1, 15, 1, 0, UI_SPINNER_COLOR);););
        M_BOX(GP.LABEL("Эффект", "", UI_LABEL_COLOR); GP.SELECT("mainAutoShowFlip", "Основной эффект,Случайная смена эффектов,Плавное угасание и появление,Перемотка по порядку числа,Перемотка по порядку катодов в лампе,Поезд,Резинка,Ворота,Волна,Блики,Испарение,Игровой автомат", mainSettings.autoShowFlip););
        GP.HR(UI_LINE_COLOR);
        GP.LABEL("Отображение", "hint4", UI_HINT_COLOR);
        GP.HINT("hint4", "Источник и время в секундах"); //всплывающая подсказка
        //GP.SPAN("(источник и время в сек)", GP_CENTER, "", UI_HINT_COLOR);
        M_BOX(GP.LABEL("1", "", UI_LABEL_COLOR); GP.SELECT("extShowMode/0", "Температура,Влажность,Давление,Температура и влажность,Дата,Год,Дата и год", (extendedSettings.autoShowModes[0]) ? (extendedSettings.autoShowModes[0] - 1) : 0); M_BOX(GP_RIGHT, GP.SPINNER("extShowTime/0", extendedSettings.autoShowTimes[0], 1, 5, 1, 0, UI_SPINNER_COLOR);););
      for (uint8_t i = 1; i < 5; i++) {
      M_BOX(GP.LABEL(String(i + 1), "", UI_LABEL_COLOR); GP.SELECT(String("extShowMode/") + i, "Пусто,Температура,Влажность,Давление,Температура и влажность,Дата,Год,Дата и год", extendedSettings.autoShowModes[i]); M_BOX(GP_RIGHT, GP.SPINNER(String("extShowTime/") + i, extendedSettings.autoShowTimes[i], 1, 5, 1, 0, UI_SPINNER_COLOR);););
      }
      GP.HR(UI_LINE_COLOR);
      GP.LABEL("Дополнительно", "", UI_HINT_COLOR);
      M_BOX(GP_LEFT, GP.LABEL("Коррекция, °C", "", UI_LABEL_COLOR);  M_BOX(GP_RIGHT, GP.SPINNER("mainTempCorrect", mainSettings.tempCorrect / 10.0, -12.7, 12.7, 0.1, 1, UI_SPINNER_COLOR, "", (boolean)!deviceInformation[SENS_TEMP]);););
      M_BOX(GP_LEFT, GP.LABEL("Тип датчика", "", UI_LABEL_COLOR);  M_BOX(GP_RIGHT, GP.NUMBER("", (deviceInformation[SENS_TEMP]) ? ((sens.err) ? "Ошибка" : tempSensList[sens.type]) : "Отсутсвует", INT32_MAX, "", true);););
      GP.BLOCK_END();

      GP.BLOCK_BEGIN(GP_THIN, "", "Индикаторы", UI_BLOCK_COLOR);
      GP.LABEL("Яркость", "", UI_HINT_COLOR);
      M_BOX(GP.LABEL("День", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER_C("mainIndiBrtDay", mainSettings.indiBrightDay, 5, 30, 1, 0, UI_SLIDER_COLOR);););//Ползунки
      M_BOX(GP.LABEL("Ночь", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER_C("mainIndiBrtNight", mainSettings.indiBrightNight, 5, 30, 1, 0, UI_SLIDER_COLOR);););//Ползунки
      GP.HR(UI_LINE_COLOR);
      GP.LABEL("Эффекты", "", UI_HINT_COLOR);
      M_BOX(GP_JUSTIFY, GP.LABEL("Глюки", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SWITCH("mainGlitch", mainSettings.glitchMode, UI_SWITCH_COLOR);););
      M_BOX(GP.LABEL("Минуты", "", UI_LABEL_COLOR); GP.SELECT("fastFlip", "Без анимации,Случайная смена эффектов,Плавное угасание и появление,Перемотка по порядку числа,Перемотка по порядку катодов в лампе,Поезд,Резинка,Ворота,Волна,Блики,Испарение,Игровой автомат", fastSettings.flipMode););
      M_BOX(GP.LABEL("Секунды", "", UI_LABEL_COLOR); GP.SELECT("mainSecsFlip", "Без анимации,Плавное угасание и появление,Перемотка по порядку числа,Перемотка по порядку катодов в лампе", mainSettings.secsMode, 0, (boolean)(deviceInformation[LAMP_NUM] < 6)););
      GP.HR(UI_LINE_COLOR);
      GP.LABEL("Антиотравление", "", UI_HINT_COLOR);
      M_BOX(GP.LABEL("Период, мин", "", UI_LABEL_COLOR); GP.SPINNER("extBurnTime", extendedSettings.burnTime, 10, 180, 1, 0, UI_SPINNER_COLOR););
      M_BOX(GP.LABEL("Метод", "", UI_LABEL_COLOR); GP.SELECT("mainBurnFlip", "Перебор всех индикаторов,Перебор одного индикатора,Перебор одного индикатора с отображением времени", mainSettings.burnMode););
      GP.HR(UI_LINE_COLOR);
      GP.LABEL("Время смены яркости", "hint1", UI_HINT_COLOR);
      GP.HINT("hint1", "Одниаковое время - отключить смену яркости или активировать датчик освещения"); //всплывающая подсказка
      M_BOX(GP_CENTER, GP.LABEL(" С", "", UI_LABEL_COLOR); GP.SPINNER("mainTimeBrightS", mainSettings.timeBrightStart, 0, 23, 1, 0, UI_SPINNER_COLOR); GP.SPINNER("mainTimeBrightE", mainSettings.timeBrightEnd, 0, 23, 1, 0, UI_SPINNER_COLOR); GP.LABEL("До", "", UI_LABEL_COLOR););
      GP.HR(UI_LINE_COLOR);
      GP.LABEL("Режим сна", "hint2", UI_HINT_COLOR);
      GP.HINT("hint2", "0 - отключить режим сна для выбранного промежутка времени"); //всплывающая подсказка
      M_BOX(GP_CENTER, GP.LABEL("День", "", UI_LABEL_COLOR); GP.SPINNER("mainSleepD", mainSettings.timeSleepDay, 0, 90, 15, 0, UI_SPINNER_COLOR); GP.SPINNER("mainSleepN", mainSettings.timeSleepNight, 0, 30, 5, 0, UI_SPINNER_COLOR); GP.LABEL("Ночь", "", UI_LABEL_COLOR););
      GP.BLOCK_END();
      );

      M_GRID(
        GP.BLOCK_BEGIN(GP_THIN, "", "Подсветка", UI_BLOCK_COLOR);
        M_BOX(GP.LABEL("Цвет", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER_C("fastColor", (fastSettings.backlColor < 253) ? (fastSettings.backlColor / 10) : (fastSettings.backlColor - 227), 0, 28, 1, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[BACKL_TYPE]);););
        M_BOX(GP.LABEL("Режим", "", UI_LABEL_COLOR); GP.SELECT("fastBackl", backlModeList, fastSettings.backlMode, 0, (boolean)!deviceInformation[BACKL_TYPE]););
        GP.HR(UI_LINE_COLOR);
        GP.LABEL("Яркость", "", UI_HINT_COLOR);
        M_BOX(GP.LABEL("День", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER_C("mainBacklBrightDay", mainSettings.backlBrightDay, 10, 250, 1, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[BACKL_TYPE]););); //ползунки
        M_BOX(GP.LABEL("Ночь", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER_C("mainBacklBrightNight", mainSettings.backlBrightNight, 0, 250, 1, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[BACKL_TYPE]););); //ползунки
        GP.BLOCK_END();

        GP.BLOCK_BEGIN(GP_THIN, "", "Точки", UI_BLOCK_COLOR);
        M_BOX(GP.LABEL("Режим", "", UI_LABEL_COLOR); GP.SELECT("fastDot", dotModeList, fastSettings.dotMode););
        GP.HR(UI_LINE_COLOR);
        GP.LABEL("Яркость", "", UI_HINT_COLOR);
        M_BOX(GP.LABEL("День", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER_C("mainDotBrtDay", mainSettings.dotBrightDay, 10, 250, 10, 0, UI_SLIDER_COLOR, (boolean)(deviceInformation[NEON_DOT] == 3)););); //ползунки
        M_BOX(GP.LABEL("Ночь", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER_C("mainDotBrtNight", mainSettings.dotBrightNight, 0, (deviceInformation[NEON_DOT] == 3) ? 1 : 250, (deviceInformation[NEON_DOT] == 3) ? 1 : 10, 0, UI_SLIDER_COLOR););); //ползунки
        GP.BLOCK_END();
      );

      M_GRID(
        GP.BLOCK_BEGIN(GP_THIN, "", "Звуки", UI_BLOCK_COLOR);
        M_BOX(GP_JUSTIFY, GP.LABEL((deviceInformation[PLAYER_TYPE]) ? "Озвучивать действия" : "Звук кнопок", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SWITCH("mainSound", mainSettings.knockSound, UI_SWITCH_COLOR);););
        M_BOX(GP_JUSTIFY, GP.LABEL("Громкость", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER("mainSoundVol", mainSettings.volumeSound, 0, (deviceInformation[PLAYER_TYPE] == 2) ? 9 : 30, 1, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[PLAYER_TYPE]);););
        GP.HR(UI_LINE_COLOR);
        GP.LABEL("Звук смены часа ", "hint3", UI_HINT_COLOR);
        GP.HINT("hint3", "Одниаковое время - отключить звук смены часа"); //всплывающая подсказка
        M_BOX(GP_CENTER, GP.LABEL(" С", "", UI_LABEL_COLOR); GP.SPINNER("mainHourSoundS", mainSettings.timeHourStart, 0, 23, 1, 0, UI_SPINNER_COLOR);  GP.SPINNER("mainHourSoundE", mainSettings.timeHourEnd, 0, 23, 1, 0, UI_SPINNER_COLOR); GP.LABEL("До", "", UI_LABEL_COLOR););
        GP.HR(UI_LINE_COLOR);
        GP.LABEL("Озвучка смены часа", "", UI_HINT_COLOR);
        M_BOX(GP.LABEL("Температура", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SWITCH("mainHourTemp", mainSettings.hourSound & 0x80, UI_SWITCH_COLOR, (boolean)!(deviceInformation[PLAYER_TYPE] && deviceInformation[SENS_TEMP]));););
        M_BOX(GP.LABEL("Новый час", "", UI_LABEL_COLOR); GP.SELECT("mainHourSound", "Автоматически,Только мелодия,Только озвучка,Мелодия и озвучка", mainSettings.hourSound & 0x03, 0, (boolean)!deviceInformation[PLAYER_TYPE]););
        GP.BLOCK_END();

        GP.BLOCK_BEGIN(GP_THIN, "", "Будильник", UI_BLOCK_COLOR);
        M_BOX(GP.LABEL("Автоотключение, мин", "", UI_LABEL_COLOR); GP.SPINNER("extAlarmTimeout", extendedSettings.alarmTime, 1, 240, 1, 0, UI_SPINNER_COLOR, "", (boolean)!deviceInformation[ALARM_TYPE]););
        GP.HR(UI_LINE_COLOR);
        GP.LABEL("Дополнительно", "", UI_HINT_COLOR);
        M_BOX(GP.LABEL("Повтор сигнала, мин", "", UI_LABEL_COLOR); GP.SPINNER("extAlarmWaitTime", extendedSettings.alarmWaitTime, 0, 240, 1, 0, UI_SPINNER_COLOR, "", (boolean)!deviceInformation[ALARM_TYPE]););
        M_BOX(GP.LABEL("Отключить звук, мин", "", UI_LABEL_COLOR); GP.SPINNER("extAlarmSoundTime", extendedSettings.alarmSoundTime, 0, 240, 1, 0, UI_SPINNER_COLOR, "", (boolean)!deviceInformation[ALARM_TYPE]););
        GP.HR(UI_LINE_COLOR);
        GP.LABEL("Индикация", "", UI_HINT_COLOR);
        M_BOX(GP.LABEL("Активный", "", UI_LABEL_COLOR); GP.SELECT("extAlarmDotOn", alarmDotModeList, extendedSettings.alarmDotOn, 0, (boolean)!deviceInformation[ALARM_TYPE]););
        M_BOX(GP.LABEL("Ожидание", "", UI_LABEL_COLOR); GP.SELECT("extAlarmDotWait", alarmDotModeList, extendedSettings.alarmDotWait, 0, (boolean)!deviceInformation[ALARM_TYPE]););
        GP.BLOCK_END();
      );
    }
    else if (ui.uri("/climate")) { //климат
      GP.PAGE_TITLE("Климат");

      int heightSize = 500;
      if (sens.press) heightSize = 300;

      if (sens.hum) {
        GP.PLOT_STOCK_DARK<2, CLIMATE_BUFFER>("climateDataMain", climateNamesMain, climateDates, climateArrMain, 10, heightSize, climateLocal);
      }
      else {
        GP.PLOT_STOCK_DARK<1, CLIMATE_BUFFER>("climateDataMain", climateNamesMain, climateDates, climateArrMain, 10, heightSize, climateLocal);
      }

      if (sens.press) {
        GP.PLOT_STOCK_DARK<1, CLIMATE_BUFFER>("climateDataExt", climateNamesExt, climateDates, climateArrExt, 0, heightSize, climateLocal);
      }

      GP.BLOCK_BEGIN(GP_DIV_RAW, "92.3%");
      M_GRID(
        M_BLOCK_THIN(
          M_BOX(GP.LABEL("Усреднение", "", UI_LABEL_COLOR); GP.SWITCH("climateAvg", settings.climateAvg, UI_SWITCH_COLOR););
        );

        M_BLOCK_THIN(
          M_BOX(GP.LABEL("Интервал, мин", "", UI_LABEL_COLOR); GP.SPINNER("climateTime", settings.climateTime, 1, 60, 1, 0, UI_SPINNER_COLOR););
        );
      );
      GP.BLOCK_END();
    }
    else if (ui.uri("/radio")) { //радиоприемник
      GP.PAGE_TITLE("Радио");

      updateList += ",radioVol,radioFreq,radioPower";

      GP.BLOCK_BEGIN(GP_THIN, "", "Радиоприёмник", UI_BLOCK_COLOR);
      GP.BLOCK_BEGIN(GP_DIV_RAW, "400px");

      GP.TABLE_BEGIN();
      GP.TR();
      GP.TD();
      M_BOX(GP_LEFT, GP.BUTTON_MINI("radioMode", "Часы ⇋ Радио", "", UI_RADIO_BACK_COLOR););
      GP.TD();
      M_BOX(GP_RIGHT, GP.LABEL("Питание", "", UI_LABEL_COLOR); GP.SWITCH("radioPower", radioSettings.powerState, UI_RADIO_POWER_COLOR);)
      GP.TR();
      GP.TD(GP_CENTER, 2);
      M_BOX(GP.LABEL("Громкость", "", UI_LABEL_COLOR); GP.SLIDER_C("radioVol", radioSettings.volume, 0, 15, 1, 0, UI_RADIO_VOL_COLOR););
      GP.TR();
      GP.TD(GP_CENTER, 2);
      M_BOX(GP.LABEL("Частота", "", UI_LABEL_COLOR); GP.SLIDER_C("radioFreq", radioSettings.stationsFreq / 10.0, 87.5, 108, 0.1, 1, UI_RADIO_FREQ_1_COLOR););
      GP.TR();
      GP.TD(GP_CENTER, 2);
      if (radioSvgImage) {
        M_BOX(GP_CENTER, GP.ICON_FILE_BUTTON("radioSeekDown", radioFsData[0], 40, UI_RADIO_FREQ_2_COLOR); GP.ICON_FILE_BUTTON("radioFreqDown", radioFsData[1], 55, UI_RADIO_FREQ_2_COLOR); GP.ICON_FILE_BUTTON("radioFreqUp", radioFsData[2], 55, UI_RADIO_FREQ_2_COLOR); GP.ICON_FILE_BUTTON("radioSeekUp", radioFsData[3], 40, UI_RADIO_FREQ_2_COLOR););
      }
      else {
        M_BOX(GP.BUTTON("radioSeekDown", "|◄◄", "", UI_RADIO_FREQ_2_COLOR); GP.BUTTON("radioFreqDown", "◄", "", UI_RADIO_FREQ_2_COLOR); GP.BUTTON("radioFreqUp", "►", "", UI_RADIO_FREQ_2_COLOR); GP.BUTTON("radioSeekUp", "►►|", "", UI_RADIO_FREQ_2_COLOR););
      }
      GP.TABLE_END();

      GP.BLOCK_END();
      GP.BLOCK_END();

      GP.BLOCK_BEGIN(GP_THIN, "", "Станции", UI_BLOCK_COLOR);
      M_TABLE(
        "20%,30%,20%,30%",
        GP_ALS(GP_RIGHT, GP_LEFT, GP_RIGHT, GP_LEFT),
      for (int i = 0; i < 10; i += 2) {
      M_TR(
        GP.BUTTON_MINI(String("radioCh/") + i, String("CH") + i, "", UI_RADIO_CHANNEL_COLOR),
        GP.NUMBER_F(String("radioSta/") + i, "number", radioSettings.stationsSave[i] / 10.0),
        GP.BUTTON_MINI(String("radioCh/") + (i + 1), String("CH") + (i + 1), "", UI_RADIO_CHANNEL_COLOR),
        GP.NUMBER_F(String("radioSta/") + (i + 1), "number", radioSettings.stationsSave[i + 1] / 10.0)
      );
      }
      );
      GP.BLOCK_END();
    }
    else if (ui.uri("/information")) { //информация о системе
      GP.PAGE_TITLE("О системе");

      GP.BLOCK_BEGIN(GP_THIN, "", "Системная информация", UI_BLOCK_COLOR);
      GP.SYSTEM_INFO(ESP_FIRMWARE_VERSION);
      GP.BLOCK_END();
    }
    else if (ui.uri("/update") && otaUpdate) { //обновление ESP
      GP.PAGE_TITLE("Обновление");

      GP.BLOCK_BEGIN(GP_THIN, "", "Обновление прошивки", UI_BLOCK_COLOR);
      GP.SPAN("Прошивку можно получить в Arduino IDE: Скетч -> Экспорт бинарного файла (сохраняется в папку с прошивкой).", GP_CENTER, "", UI_INFO_COLOR); //описание
      GP.BREAK();
      GP.SPAN("Файловую систему можно получить в Arduino IDE: Инструменты -> ESP8266 LittleFS Data Upload, в логе необходимо найти: [LittleFS] upload, файл находится по этому пути.", GP_CENTER, "", UI_INFO_COLOR); //описание
      GP.BREAK();
      GP.SPAN("Поддерживаемые форматы файлов bin и bin.gz.", GP_CENTER, "", UI_INFO_COLOR); //описание
      GP.HR(UI_LINE_COLOR);
      GP.LABEL("Загрузить", "", UI_HINT_COLOR);
      M_BOX(GP.LABEL("Прошивку ESP", "", UI_LABEL_COLOR); GP.OTA_FIRMWARE(""););
      M_BOX(GP.LABEL("Файловую систему ESP", "", UI_LABEL_COLOR); GP.OTA_FILESYSTEM(""););
      GP.BLOCK_END();
    }
    else { //подключение к роутеру
      GP.PAGE_TITLE("Сетевые настройки");

      GP.BLOCK_BEGIN(GP_THIN, "", "Локальная сеть WIFI", UI_BLOCK_COLOR);
      if ((wifiStatus == WL_CONNECTED) || wifiInterval) {
        GP.FORM_BEGIN("/network");
        if (wifiStatus == WL_CONNECTED) {
          GP.LABEL("Подключено к \"" + String(settings.ssid) + "\"", "", UI_INFO_COLOR, 25, false, true);
          GP.BREAK();
          GP.LABEL("IP адрес \"" + WiFi.localIP().toString() + "\"", "", UI_INFO_COLOR, 25, false, true);
        }
        else GP.LABEL("Подключение к \"" + String(settings.ssid) + "\"...", "", UI_INFO_COLOR, 25, false, true);

        GP.HR(UI_LINE_COLOR);
        GP.SUBMIT("Отключиться", UI_BUTTON_COLOR);
        GP.FORM_END();
      }
      else {
        GP.FORM_BEGIN("/");
        GP.TEXT("login", "Логин", settings.ssid);
        GP.BREAK();
        GP.PASS_EYE("pass", "Пароль", settings.pass, "100%");
        GP.HR(UI_LINE_COLOR);
        GP.SUBMIT("Подключиться", UI_BUTTON_COLOR);
        GP.FORM_END();
      }
      GP.BLOCK_END();

      if (ui.uri("/network")) { //сетевые настройки
        updateList += ",syncStatus";

        GP.BLOCK_BEGIN(GP_THIN, "", "Сервер NTP", UI_BLOCK_COLOR);
        GP.TEXT("syncHost", "Хост", settings.host);
        GP.BREAK();
        GP.TEXT("syncPer", "Период", String((settings.ntpDst) ? 3600 : settings.ntpTime), "", 0, "", (boolean)settings.ntpDst);
        GP.BREAK();
        GP.LABEL(getNtpState(), "syncStatus", UI_INFO_COLOR);
        GP.HR(UI_LINE_COLOR);
        GP.BUTTON("syncCheck", "Синхронизировать сейчас", "", (statusNtp == NTP_STOPPED) ? GP_GRAY : UI_BUTTON_COLOR, "", (boolean)(statusNtp == NTP_STOPPED));
        GP.BLOCK_END();

        GP.UPDATE_CLICK("syncStatus", "syncCheck");
      }
    }

    GP.UPDATE(updateList);
    GP.UI_END();    //завершить окно панели управления
  }
  GP.BUILD_END();
}

void action() {
  if (ui.click()) {
    if (ui.clickSub("sync")) {
      if (ui.click("syncGmt")) {
        settings.ntpGMT = ui.getInt("syncGmt") - 12;
        ntp.setGMT(settings.ntpGMT); //установить часовой пояс в часах
        if (settings.ntpSync && !ntp.status() && setSyncTime()) {
          busSetComand(WRITE_TIME);
          busSetComand(WRITE_DATE);
        }
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("syncTime")) {
        if (!ntp.status()) {
          if (!sendNtpTime) {
            ntp.setPeriod(1); //запросить текущее время
            sendNtpTime = true;
            statusNtp = NTP_NOT_STATUS;
          }
        }
        else {
          mainTime = ui.getSystemTime(); //запросить время браузера
          mainDate = ui.getSystemDate(); //запросить дату браузера
          busSetComand(WRITE_TIME);
          busSetComand(WRITE_DATE);
        }
      }
      if (ui.clickBool("syncAuto", settings.ntpSync)) {
        memory.update(); //обновить данные в памяти
      }
      if (ui.clickBool("syncDst", settings.ntpDst)) {
        if (settings.ntpSync && !ntp.status() && setSyncTime()) {
          busSetComand(WRITE_TIME);
          busSetComand(WRITE_DATE);
        }
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("syncHost")) {
        if (ui.getString("syncHost").length() > 0) strncpy(settings.host, ui.getString("syncHost").c_str(), 20); //копируем себе
        else strncpy(settings.host, DEFAULT_NTP_HOST, 20); //установить хост по умолчанию
        settings.host[19] = '\0'; //устанавливаем последний символ
        ntp.setHost(settings.host); //установить хост
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("syncPer")) {
        settings.ntpTime = constrain(ui.getInt("syncPer"), 3600, 86400);
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("syncCheck")) {
        ntp.setPeriod(1); //запросить текущее время
        sendNtpTime = true;
        attemptsNtp = 0;
        statusNtp = NTP_NOT_STATUS;
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("alarm")) {
      if (alarm.set) { //если режим настройки будильника
        if (ui.click("alarmVol")) {
          alarm_data[alarm.now][ALARM_DATA_VOLUME] = ui.getInt("alarmVol");
          alarm.volume = alarm_data[alarm.now][ALARM_DATA_VOLUME];
          busSetComand(WRITE_SELECT_ALARM, ALARM_VOLUME);
          if (!alarm_data[alarm.now][ALARM_DATA_RADIO] && alarm_data[alarm.now][ALARM_DATA_VOLUME] && alarm_data[alarm.now][ALARM_DATA_MODE]) busSetComand(WRITE_TEST_ALARM_VOL);
        }
        if (ui.click("alarmSoundType")) {
          alarm_data[alarm.now][ALARM_DATA_RADIO] = ui.getBool("alarmSoundType");
          busSetComand(WRITE_SELECT_ALARM, ALARM_RADIO);
          busSetComand(WRITE_SELECT_ALARM, ALARM_SOUND);
          busSetComand(WRITE_SELECT_ALARM, ALARM_VOLUME);
        }
        if (ui.click("alarmSound")) {
          if (!alarm_data[alarm.now][ALARM_DATA_RADIO]) {
            alarm_data[alarm.now][ALARM_DATA_SOUND] = ui.getInt("alarmSound");
            busSetComand(WRITE_SELECT_ALARM, ALARM_SOUND);
            busSetComand(WRITE_TEST_ALARM_SOUND);
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
            alarm.reload = 1;
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
            alarm.reload = 1;
            busSetComand(NEW_ALARM);
            busSetComand(READ_ALARM_ALL);
          }
        }
      }
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("fast")) {
      if (ui.clickInt("fastFlip", fastSettings.flipMode)) {
        busSetComand(WRITE_FAST_SET, FAST_FLIP_MODE);
      }
      if (ui.clickInt("fastDot", fastSettings.dotMode)) {
        busSetComand(WRITE_FAST_SET, FAST_DOT_MODE);
      }
      if (ui.clickInt("fastBackl", fastSettings.backlMode)) {
        busSetComand(WRITE_FAST_SET, FAST_BACKL_MODE);
      }
      if (ui.click("fastColor")) {
        uint8_t color = constrain(ui.getInt("fastColor"), 0, 28);
        fastSettings.backlColor = (color > 25) ? (color + 227) : (color * 10);
        busSetComand(WRITE_FAST_SET, FAST_BACKL_COLOR);
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
      //--------------------------------------------------------------------
      if (ui.clickInt("mainIndiBrtDay", mainSettings.indiBrightDay)) {
        busSetComand(WRITE_MAIN_SET, MAIN_INDI_BRIGHT_D);
      }
      if (ui.clickInt("mainIndiBrtNight", mainSettings.indiBrightNight)) {
        busSetComand(WRITE_MAIN_SET, MAIN_INDI_BRIGHT_N);
      }
      if (ui.clickInt("mainBurnFlip", mainSettings.burnMode)) {
        busSetComand(WRITE_MAIN_SET, MAIN_BURN_MODE);
      }
      if (ui.clickInt("mainSecsFlip", mainSettings.secsMode)) {
        busSetComand(WRITE_MAIN_SET, MAIN_SECS_MODE);
      }

      if (ui.click("mainAutoShow")) {
        mainSettings.autoShowTime = ui.getBool("mainAutoShow");
        busSetComand(WRITE_MAIN_SET, MAIN_AUTO_SHOW_TIME);
      }
      if (ui.clickInt("mainAutoShowTime", mainSettings.autoShowTime)) {
        busSetComand(WRITE_MAIN_SET, MAIN_AUTO_SHOW_TIME);
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
      if (ui.click("extBurnTime")) {
        extendedSettings.burnTime = ui.getInt("extBurnTime");
        busSetComand(WRITE_EXTENDED_BURN_TIME);
      }
      if (ui.clickSub("extShowMode")) {
        uint8_t pos = ui.clickNameSub(1).toInt();
        uint8_t mode = ui.getInt(String("extShowMode/") + pos);
        extendedSettings.autoShowModes[pos] = (!pos) ? (mode + 1) : mode;
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
    }
    //--------------------------------------------------------------------
    if (ui.clickSub("timer")) {
      if (!timer.mode || (timer.mode & 0x82)) {
        if (ui.clickSub("timerHour")) {
          switch (ui.clickNameSub(1).toInt()) {
            case 0: if (timer.hour < 23) timer.hour++; break; //час прибавить
            case 1: if (timer.mins < 59) timer.mins++; break; //минута прибавить
            case 2: if (timer.secs < 59) timer.secs++; break; //секунда прибавить
            case 3: if (timer.hour > 0) timer.hour--; break; //час убавить
            case 4: if (timer.mins > 0) timer.mins--; break; //минута убавить
            case 5: if (timer.secs > 0) timer.secs--; break; //секунда убавить
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
                timer.time = timer.count;
                busSetComand(WRITE_TIMER_SET);
              }
              else timer.mode = 1;
              busSetComand(WRITE_TIMER_TIME);
              busSetComand(WRITE_TIMER_MODE);
            }
            else if (timer.mode & 0x80) {
              timer.mode &= 0x7F;
              busSetComand(WRITE_TIMER_STATE);
              busSetComand(WRITE_TIMER_MODE);
            }
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
      if (ui.clickInt("climateTime", settings.climateTime)) {
        memory.update(); //обновить данные в памяти
      }
      if (ui.clickBool("climateAvg", settings.climateAvg)) {
        memory.update(); //обновить данные в памяти
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
    //проверяем, была ли это форма "/network"
    if (ui.form("/network")) {
      wifiInterval = 0; //сбрасываем интервал переподключения
      statusNtp = NTP_STOPPED; //устанавливаем флаг остановки ntp сервера
      ntp.end(); //остановили ntp
      WiFi.disconnect(); //отключаем wifi
      if (WiFi.getMode() != WIFI_AP_STA) wifiStartAP();
    }
    else if (ui.form("/")) {
      if (wifiStatus != WL_CONNECTED) {
        strncpy(settings.ssid, ui.getString("login").c_str(), 20); //копируем себе
        settings.ssid[19] = '\0'; //устанавливаем последний символ
        strncpy(settings.pass, ui.getString("pass").c_str(), 20); //копируем себе
        settings.pass[19] = '\0'; //устанавливаем последний символ
        wifiInterval = 1; //устанавливаем интервал переподключения
        memory.update(); //обновить данные в памяти
      }
    }
  }
  /**************************************************************************/
  if (ui.update()) {
    if (ui.updateSub("sync")) {
      if (ui.update("syncStatus")) { //если было обновление
        ui.answer(getNtpState());
      }
    }
    if (ui.updateSub("bar")) {
      if (ui.update("barTime")) { //если было обновление
        ui.answer(ui.getSystemTime().encode());
        if (!sensDataWait) busSetComand(READ_STATUS);
      }
      if (ui.update("barTemp")) { //если было обновление
        ui.answer(String((sens.temp) ? ((sens.temp + mainSettings.tempCorrect) / 10.0) : 0, 1) + "°С");
      }
      if (ui.update("barHum")) { //если было обновление
        ui.answer(String(sens.hum) + "%");
      }
      if (ui.update("barPress")) { //если было обновление
        ui.answer(String(sens.press) + "mm.Hg");
      }
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("alarm")) {
      if (alarm.set) { //если режим настройки будильника
        if (ui.update("alarmVol")) {
          if (alarm.volume != alarm_data[alarm.now][ALARM_DATA_VOLUME]) {
            alarm.volume = alarm_data[alarm.now][ALARM_DATA_VOLUME];
            ui.answer(alarm_data[alarm.now][ALARM_DATA_VOLUME]);
          }
        }
        if (ui.update("alarmSoundType")) {
          ui.answer(alarm_data[alarm.now][ALARM_DATA_RADIO]);
        }
        if (ui.update("alarmSound")) {
          if (!alarm_data[alarm.now][ALARM_DATA_RADIO]) {
            ui.answer(alarm_data[alarm.now][ALARM_DATA_SOUND]);
          }
        }
        if (ui.update("alarmRadio")) {
          if (alarm_data[alarm.now][ALARM_DATA_RADIO]) {
            ui.answer(alarm_data[alarm.now][ALARM_DATA_STATION]);
          }
        }
        if (ui.update("alarmTime")) {
          if ((alarm_data[alarm.now][ALARM_DATA_HOUR] != alarmTime.hour) || (alarm_data[alarm.now][ALARM_DATA_MINS] != alarmTime.minute)) {
            alarmTime.hour = alarm_data[alarm.now][ALARM_DATA_HOUR];
            alarmTime.minute = alarm_data[alarm.now][ALARM_DATA_MINS];
            ui.answer(alarmTime);
          }
        }
        if (ui.update("alarmMode")) {
          ui.answer(alarm_data[alarm.now][ALARM_DATA_MODE]);
        }
        if (ui.updateSub("alarmDay")) {
          uint8_t day = constrain(ui.updateNameSub(1).toInt(), 1, 7);
          ui.answer((boolean)(alarm_data[alarm.now][ALARM_DATA_DAYS] & (0x01 << day)));
        }
      }
      if (ui.update("alarmReload") && (alarm.reload >= 2)) { //если было обновление
        ui.answer(1);
        alarm.reload = 0;
      }
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("main")) {
      if (ui.update("mainAutoShow")) { //если было обновление
        ui.answer((boolean)mainSettings.autoShowTime);
      }
      if (ui.update("mainAutoShowTime")) { //если было обновление
        ui.answer((mainSettings.autoShowTime) ? mainSettings.autoShowTime : 1);
      }
      if (ui.update("mainTimerState")) { //если было обновление
        ui.answer(getTimerState());
        if (!timer.mode) busSetComand(READ_TIMER_STATE);
        else busSetComand(READ_TIMER_TIME);
      }
      if (ui.update("mainTimer")) { //если было обновление
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
        busSetComand(READ_RADIO_POWER);
      }
    }
  }
}

//------------------------------Получить состояние ntp-----------------------------------
String getNtpState(void) { //получить состояние ntp
  String data = "Статус: " + ((statusNtp < NTP_TRYING) ? statusNtpList[statusNtp] : "Попытка[" + String((attemptsNtp >> 1) + 1) + "]...");
  return data;
}
//----------------------------Получить состояние таймера---------------------------------
String getTimerState(void) { //получить состояние таймера
  String data = statusTimerList[timer.mode & 0x03];
  if (((timer.mode & 0x03) == 2) && !timer.count) data += " - тревога";
  else if (timer.mode & 0x80) data += " - пауза";
  return data;
}
//------------------------Преобразовать время в формат ЧЧ:ММ:СС--------------------------
String convertTimerTime(void) { //преобразовать время в формат ЧЧ:ММ:СС
  String data = "";

  uint8_t buff = 0;
  if (timer.mode) buff = timer.count / 3600;
  else buff = timer.hour;
  if (buff < 10) data += '0';
  data += buff;
  data += ':';

  if (timer.mode) buff = (timer.count / 60) % 60;
  else buff = timer.mins;
  if (buff < 10) data += '0';
  data += buff;
  data += ':';

  if (timer.mode) buff = timer.count % 60;
  else buff = timer.secs;
  if (buff < 10) data += '0';
  data += buff;

  return data;
}

const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //дней в месяце
//------------------------------Максимальное количество дней------------------------------
uint8_t maxDays(uint16_t YY, uint8_t MM) { //максимальное количество дней
  return (((MM == 2) && !(YY % 4)) ? 1 : 0) + daysInMonth[MM - 1]; //возвращаем количество дней в месяце
}
//---------------------------------Получить день недели-----------------------------------
uint8_t getWeekDay(uint16_t YY, uint8_t MM, uint8_t DD) { //получить день недели
  if (YY >= 2000) YY -= 2000; //если год больше 2000
  uint16_t days = DD; //записываем дату
  for (uint8_t i = 1; i < MM; i++) days += daysInMonth[i - 1]; //записываем сколько дней прошло до текущего месяца
  if ((MM > 2) && !(YY % 4)) days++; //если високосный год, прибавляем день
  return (days + 365 * YY + (YY + 3) / 4 - 2 + 6) % 7 + 1; //возвращаем день недели
}
//--------------------------------Получить летнее время-----------------------------------
boolean DST(uint8_t MM, uint8_t DD, uint8_t DW, uint8_t HH) { //получить летнее время
  if (MM < 3 || MM > 10) return 0; //зима
  switch (MM) {
    case 3:
      if (DD < 25) return 0; //зима
      else if (DW == 7) {
        if (HH >= 1) return 1; //лето
        else return 0; //зима
      }
      else if ((DD - 25) < DW) return 0; //зима
      else return 1; //лето
      break;
    case 10:
      if (DD < 25) return 1; //лето
      else if (DW == 7) {
        if (HH >= 2) return 0; //зима
        else return 1; //лето
      }
      else if ((DD - 25) < DW) return 1; //лето
      else return 0; //зима
      break;
  }
  return 1; //лето
}

boolean setSyncTime(void) {
  if (ntp.synced()) {
    mainTime.second = ntp.second();
    mainTime.minute = ntp.minute();
    mainTime.hour = ntp.hour();
    mainDate.day = ntp.day();
    mainDate.month = ntp.month();
    mainDate.year = ntp.year();
    uint8_t dayWeek = ntp.dayWeek();

    if (!settings.ntpDst || !DST(mainDate.month, mainDate.day, dayWeek, mainTime.hour)) return true;
    if (mainTime.hour != 23) {
      mainTime.hour += 1;
      return true;
    }
    mainTime.hour = 0; //сбросили час
    if (++mainDate.day > maxDays(mainDate.year, mainDate.month)) { //день
      mainDate.day = 1; //сбросили день
      if (++mainDate.month > 12) { //месяц
        mainDate.month = 1; //сбросили месяц
        if (++mainDate.year > 2099) { //год
          mainDate.year = 2000; //сбросили год
        }
      }
    }
    return true;
  }
  return false;
}

void climateAdd(int16_t temp, int16_t hum, int16_t press, uint32_t unix) {
  GPaddInt(temp, climateArrMain[0], CLIMATE_BUFFER);
  if (hum) {
    GPaddInt(hum * 10, climateArrMain[1], CLIMATE_BUFFER);
  }
  if (press) {
    GPaddInt(press, climateArrExt[0], CLIMATE_BUFFER);
  }
  GPaddUnix(unix, climateDates, CLIMATE_BUFFER);
}

void climateReset(void) {
  climateTimeAvg = 0;
  climateTempAvg = 0;
  climateHumAvg = 0;
  climatePressAvg = 0;
  climateCountAvg = 0;
}

void climateUpdate(void) {
  static boolean firstStart;

  uint32_t unixNow = 0;
  if (!ntp.synced() || ntp.status()) unixNow = GPunix(mainDate.year, mainDate.month, mainDate.day, mainTime.hour, mainTime.minute, mainTime.second, settings.ntpGMT);
  else unixNow = ntp.unix();

  uint16_t temperature = (sens.temp) ? (sens.temp + mainSettings.tempCorrect) : 0;
  if (!firstStart) {
    firstStart = true;
    climateTimer = millis();
    for (uint8_t i = 0; i < CLIMATE_BUFFER; i++) {
      climateAdd(temperature, sens.hum, sens.press, unixNow);
    }
    climateReset(); //сброс усреднения
  }
  else {
    climateTimeAvg++;

    climateTempAvg += temperature;
    climateHumAvg += sens.hum;
    climatePressAvg += sens.press;

    if (++climateCountAvg >= settings.climateTime) {
      if (settings.climateAvg && (climateTimeAvg > 1)) {
        if (climateTempAvg) climateTempAvg /= climateCountAvg;
        if (climateHumAvg) climateHumAvg /= climateCountAvg;
        if (climatePressAvg) climatePressAvg /= climateCountAvg;
        climateAdd(climateTempAvg, climateHumAvg, climatePressAvg, unixNow);
      }
      else climateAdd(temperature, sens.hum, sens.press, unixNow);
      climateReset(); //сброс усреднения
    }
  }
}

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

void wifiStartAP(void) {
  //настраиваем режим работы
  WiFi.mode(WIFI_AP_STA);

  //настраиваем точку доступа
  IPAddress apIP(AP_IP);
  IPAddress subnet(255, 255, 255, 0);
  //запускаем точку доступа
  Serial.println F("");
  Serial.print F("Wifi access point enable, [ ssid: ");
  Serial.print(AP_SSID);
  Serial.print F(" pass: ");
  Serial.print(AP_PASS);
  Serial.println F(" ]");
  //задаем настройки сети
  WiFi.softAPConfig(apIP, apIP, subnet);
  //включаем wifi в режиме точки доступа с именем и паролем по умолчанию
  WiFi.softAP(AP_SSID, AP_PASS);
}

void setup() {
  //инициализация шины
  twi_init();

  Serial.begin(115200);
  Serial.println F("");
  Serial.println F("Startup...");
  Serial.print F("Firmware version ");
  Serial.print F(ESP_FIRMWARE_VERSION);
  Serial.println F("...");

  //инициализация файловой системы
  if (!LittleFS.begin()) Serial.println F("File system error");
  else {
    Serial.println F("File system init");
    if (checkFsData(climateFsData, 1)) {
      climateLocal = true; //работаем локально
      Serial.println F("Script file found");
    }
    if (checkFsData(alarmFsData, 2)) {
      alarmSvgImage = true; //работаем локально
      Serial.println F("Alarm svg files found");
    }
    if (checkFsData(timerFsData, 5)) {
      timerSvgImage = true; //работаем локально
      Serial.println F("Timer svg files found");
    }
    if (checkFsData(radioFsData, 4)) {
      radioSvgImage = true; //работаем локально
      Serial.println F("Radio svg files found");
    }
  }

  if (ESP.getFreeSketchSpace() < ESP.getSketchSize()) {
    otaUpdate = false; //выключаем обновление
    Serial.println F("OTA update disable, running out of memory");
  }
  else Serial.println F("OTA update enable");

  //читаем логин пароль из памяти
  EEPROM.begin(memory.blockSize());
  if (memory.begin(0, 0xAC) == 1) {
    settings.ssid[0] = '\0'; //устанавливаем последний символ
    settings.pass[0] = '\0'; //устанавливаем последний символ

    strncpy(settings.host, DEFAULT_NTP_HOST, 20); //установить хост по умолчанию
    settings.host[19] = '\0'; //устанавливаем последний символ

    settings.climateTime = DEFAULT_CLIMATE_TIME; //установить период по умолчанию
    settings.climateAvg = DEFAULT_CLIMATE_AVG; //установить усреднение по умолчанию
    settings.ntpGMT = DEFAULT_GMT; //установить часовой по умолчанию
    settings.ntpSync = false; //выключаем авто-синхронизацию
    settings.ntpDst = DEFAULT_DST; //установить учет летнего времени по умолчанию
    settings.ntpTime = DEFAULT_NTP_TIME; //установить период по умолчанию
    memory.update(); //обновить данные в памяти
  }
  alarm.now = 0; //сбросить текущий будильник

  //настраиваем wifi
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(true);
  wifiStartAP();

  //подключаем конструктор и запускаем веб интерфейс
  ui.attachBuild(build);
  ui.attach(action);
  ui.start();

  //обновление без пароля
  if (otaUpdate) ui.enableOTA();
  ui.downloadAuto(true);

  //запрашиваем настройки часов
  busSetComand(WRITE_CHECK_SENS);
  busSetComand(READ_TIME_DATE);
  busSetComand(READ_FAST_SET);
  busSetComand(READ_MAIN_SET);
  busSetComand(READ_EXTENDED_SET);
  busSetComand(READ_RADIO_SET);
  busSetComand(READ_ALARM_ALL);
  busSetComand(READ_SENS_INFO);
  busSetComand(READ_DEVICE);

  busTimerSetInterval(5000);
}

void loop() {
  static uint32_t timerWait = millis(); //таймер ожидания опроса шины
  static uint32_t timerWifi = millis(); //таймер попытки подключения к wifi

  if (wifiStatus != WiFi.status()) { //если изменился статус
    wifiStatus = WiFi.status();
    switch (wifiStatus) {
      case WL_CONNECTED:
        Serial.print F("Wifi connected, IP address: ");
        Serial.println(WiFi.localIP());
        timerWifi = millis(); //сбросили таймер
        wifiInterval = 300000; //устанавливаем интервал отключения точки доступа

        ntp.begin(); //запустить ntp
        ntp.setHost(settings.host); //установить хост
        ntp.setGMT(settings.ntpGMT); //установить часовой пояс в часах
        ntp.setPeriod(5); //устанавливаем период
        attemptsNtp = 0; //сбрасываем попытки
        statusNtp = NTP_NOT_STATUS; //установили статус подключения к ntp серверу
        break;
      case WL_IDLE_STATUS:
        Serial.println F("Wifi idle status");
        break;
      default:
        if ((wifiStatus == WL_DISCONNECTED) || (wifiStatus == WL_NO_SSID_AVAIL)) {
          timerWifi = millis(); //сбросили таймер
          if (wifiStatus == WL_NO_SSID_AVAIL) wifiInterval = 30000; //устанавливаем интервал переподключения
          else wifiInterval = 5000; //устанавливаем интервал переподключения
          WiFi.disconnect(); //отключаем wifi
        }
        else wifiInterval = 0; //сбрасываем интервал переподключения
        statusNtp = NTP_STOPPED; //устанавливаем флаг остановки ntp сервера
        ntp.end(); //остановили ntp
        break;
    }
  }

  if (wifiInterval && ((millis() - timerWifi) >= wifiInterval)) {
    if (wifiStatus == WL_CONNECTED) { //если подключены
      Serial.println F("Wifi access point disabled");
      WiFi.mode(WIFI_STA); //отключили точку доступа
      wifiInterval = 0; //сбрасываем интервал переподключения
    }
    else { //иначе новое поключение
      wifiStatus = WiFi.begin(settings.ssid, settings.pass); //подключаемся к wifi
      if (wifiStatus != WL_CONNECT_FAILED) {
        Serial.print F("Wifi connecting to \"");
        Serial.print(settings.ssid);
        Serial.println F("\"...");
        timerWifi = millis(); //сбросили таймер
        wifiInterval = 30000; //устанавливаем интервал переподключения
      }
      else {
        Serial.println F("Wifi connection failed, wrong settings");
        wifiInterval = 0; //сбрасываем интервал
      }
    }
  }

  if (deviceInformation[HARDWARE_VERSION] == HW_VERSION) {
    if (ntp.tick()) { //обработка ntp
      if (ntp.status()) {
        if (attemptsNtp < 9) {
          ntp.setPeriod(5);
          attemptsNtp++;
          statusNtp = NTP_TRYING;
        }
        else {
          ntp.setPeriod((settings.ntpDst) ? 3600 : settings.ntpTime);
          sendNtpTime = false;
          attemptsNtp = 0;
          statusNtp = NTP_ERROR;
        }
      }
      else if (!ntp.busy()) {
        if (setSyncTime()) {
          if (settings.ntpSync || sendNtpTime) {
            busSetComand(WRITE_TIME);
            busSetComand(WRITE_DATE);
          }
        }
        ntp.setPeriod((settings.ntpDst) ? 3600 : settings.ntpTime);
        sendNtpTime = false;
        attemptsNtp = 0;
        statusNtp = NTP_SYNCED;
      }
      else statusNtp = NTP_WAIT_ANSWER;
    }

    if ((millis() - climateTimer) >= 60000) {
      climateTimer = millis();
      busSetComand(WRITE_CHECK_SENS);
      if (!ntp.synced() || ntp.status()) busSetComand(READ_TIME_DATE);
      if (!sensDataWait) timerWait = millis();
      sensDataWait = true;
    }

    if ((millis() - timerWait) >= ((sensDataWait) ? 1000 : 5000)) {
      timerWait = millis();
      busSetComand(READ_STATUS);
    }

    if (deviceStatus) {
      for (uint8_t i = 0; i < STATUS_MAX_DATA; i++) { //проверяем все флаги
        if (deviceStatus & 0x01) { //если флаг установлен
          switch (i) { //выбираем действие
            case STATUS_UPDATE_TIME_SET: busSetComand(READ_TIME_DATE); break;
            case STATUS_UPDATE_MAIN_SET: busSetComand(READ_MAIN_SET); break;
            case STATUS_UPDATE_FAST_SET: busSetComand(READ_FAST_SET); break;
            case STATUS_UPDATE_RADIO_SET: busSetComand(READ_RADIO_SET); break;
            case STATUS_UPDATE_ALARM_SET: busSetComand(READ_ALARM_ALL); break;
            case STATUS_UPDATE_SENS_DATA: busSetComand(READ_SENS_DATA); sensDataWait = false; break;
          }
        }
        deviceStatus >>= 1; //сместили буфер флагов
      }
      deviceStatus = 0;
    }
  }

  busUpdate(); //обработка шины

  ui.tick(); //обработка веб интерфейса
  memory.tick(); //обработка еепром
}
