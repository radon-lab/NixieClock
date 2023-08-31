/*
  Arduino IDE 1.8.13 версия прошивки 1.0.4 релиз от 31.08.23
  Специльно для проекта "Часы на ГРИ v2. Альтернативная прошивка"
  Страница проекта - https://community.alexgyver.ru/threads/chasy-na-gri-v2-alternativnaja-proshivka.5843/

  Исходник - https://github.com/radon-lab/NixieClock
  Автор Radon-lab & Psyx86.

  Если не установлено ядро ESP8266, "Файл -> Настройки -> Дополнительные ссылки для Менеджера плат", в окно ввода вставляете ссылку - https://arduino.esp8266.com/stable/package_esp8266com_index.json
  Далее "Инструменты -> Плата -> Менеджер плат..." находите плату esp8266 и устанавливаете последнюю версию!

  В "Инструменты -> Управлять библиотеками..." необходимо предварительно установить последние версии библиотек:
  GyverPortal
  GyverNTP
  EEManager

  При выборе распределения памяти("Инструменты -> Flash Size") минимальный объем OTA должен быть - 470KB, минимальный объем FS - 512KB(или меньше если не нужно загружать файловую систему).

  Папку с плагином "ESP8266LittleFS" необходимо поместить в .../Program Files/Arduino/tools, затем нужно перезапустить Arduino IDE(если была запущена).
  Сначала загружаете прошивку, затем "Инструменты -> ESP8266 LittleFS Data Upload".

  Подключение к часам производится по шине I2C, так-же необходимо отключить внутреннюю подтяжку шины в часах, а внешнюю подтяжку подключить к 3.3в(подтяжка на шине должна быть строго только в одном месте!)
*/
#include "config.h"

#include <LittleFS.h>
#include <GyverPortal.h>
GyverPortal ui(&LittleFS);

#include <GyverNTP.h>
GyverNTP ntp(DEFAULT_GMT, NTP_TIME);

struct settingsData {
  uint8_t climateTime;
  boolean climateAvg;
  boolean ntpSync;
  int8_t ntpGMT;
  char ssid[20];
  char pass[20];
} settings;

#include <EEManager.h>
EEManager memory(settings);

//переменные
GPdate mainDate;
GPtime mainTime;
GPtime alarmTime;

boolean otaUpdate = true; //флаг запрете обновления
boolean alarmSvgImage = false; //флаг локальных изоражений будильника
uint8_t alarmReload = 0; //флаг обновления страницы будильника

uint8_t climateCountAvg;
uint16_t climateTempAvg;
uint16_t climateHumAvg;
uint16_t climatePressAvg;

boolean climateLocal = false; //флаг локальных скриптов графика
int16_t climateArrMain[2][CLIMATE_BUFFER];
int16_t climateArrExt[1][CLIMATE_BUFFER];
uint32_t climateDates[CLIMATE_BUFFER];

#include "WIRE.h"
#include "CLOCKBUS.h"

const char *climateNamesMain[] = {"Температура", "Влажность"};
const char *climateNamesExt[] = {"Давление"};

String tempSensList[] = {"DS3231", "AHT", "SHT", "BMP/BME", "DS18B20", "DHT"};

void build(void) {
  GP.BUILD_BEGIN(UI_MAIN_THEME);
  GP.ONLINE_CHECK(); //проверять статус платы

  if (deviceInformation[HARDWARE_VERSION] && (deviceInformation[HARDWARE_VERSION] != HW_VERSION)) {
    GP.SPAN("Внимание! Эта версия веб-интерфейса не может взаимодействовать с этим устройством!", GP_CENTER, "", UI_INFO_COLOR);
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
    if (ntp.synced()) {
      GP.LABEL_BLOCK("NTP synced", "", UI_MENU_NTP_1_COLOR, 0, 1);
    }
    else if (!ntp.status()) {
      GP.LABEL_BLOCK("NTP connect", "", UI_MENU_NTP_2_COLOR, 0, 1);
    }
    else {
      GP.LABEL_BLOCK("NTP disconnect", "", UI_MENU_NTP_3_COLOR, 0, 1);
    }
    GP.BREAK();
    GP.HR(UI_MENU_LINE_COLOR);

    String backlModeList;

    backlModeList += "Выключена";
    if (deviceInformation[BACKL_TYPE]) {
      backlModeList += ',';
      backlModeList += "Статичная,Дыхание";
    }
    if (deviceInformation[BACKL_TYPE] >= 3) {
      backlModeList += ',';
      backlModeList += "Дыхание со сменой цвета при затухании,Бегущий огонь,Бегущий огонь со сменой цвета,Бегущий огонь с радугой,Бегущий огонь с конфетти,Волна,Волна со сменой цвета,Волна с радугой,Волна с конфетти,Плавная смена цвета,Радуга,Конфетти";
    }

    String dotModeList;

    dotModeList += "Выключены,";
    dotModeList += "Статичные";
    if (deviceInformation[NEON_DOT] != 3) {
      dotModeList += ',';
      dotModeList += "Динамичные(плавно мигают)";
    }
    if (deviceInformation[NEON_DOT] == 2) {
      dotModeList += ',';
      dotModeList += "Маятник(неонки)";
    }
    if (deviceInformation[DOTS_PORT_ENABLE]) {
      dotModeList += ',';
      dotModeList += "Мигающие,";
      dotModeList += "Бегущие,";
      dotModeList += "Змейка,";
      dotModeList += "Резинка";
      if ((deviceInformation[DOTS_NUM] > 4) || (deviceInformation[DOTS_TYPE] == 2)) {
        dotModeList += ',';
        dotModeList += "Одинарный маятник";
      }
      if ((deviceInformation[DOTS_NUM] > 4) && (deviceInformation[DOTS_TYPE] == 2)) {
        dotModeList += ',';
        dotModeList += "Двойной маятник";
      }
    }

    //обновления блоков
    String updateList;

    updateList += "barTime";
    if (deviceInformation[SENS_TEMP]) {
      updateList += ',';
      updateList += "barTemp";
      if (sens.hum) {
        updateList += ',';
        updateList += "barHum";
      }
      if (sens.press) {
        updateList += ',';
        updateList += "barPress";
      }
    }
    GP.UI_BODY(); //начать основное окно
    GP.GRID_RESPONSIVE(600); //позволяет "отключить" таблицу при ширине экрана меньше 600px

    M_TABLE(
      "",
      GP_ALS(GP_LEFT, GP_RIGHT),
      M_TR(GP.LABEL(" "); GP.LABEL(" "); GP.LABEL_BLOCK(ui.getSystemTime().encode(), "barTime", UI_BAR_CLOCK_COLOR, 18, 1););
      M_TD(
        if (deviceInformation[SENS_TEMP]) GP.LABEL_BLOCK(String((sens.temp + mainSettings.tempCorrect) / 10.0, 1) + "°С", "barTemp", UI_BAR_TEMP_COLOR, 18, 1);
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
          M_BOX(GP_JUSTIFY, GP.LABEL("Формат", "", UI_LABEL_COLOR);  M_BOX(GP_RIGHT, GP.LABEL("24ч", "", UI_LABEL_COLOR);  GP.SWITCH("mainTimeFormat", mainSettings.timeFormat, UI_SWITCH_COLOR); GP.LABEL("12ч", "", UI_LABEL_COLOR);););
          GP.HR(UI_LINE_COLOR);
          M_BOX(GP.LABEL("Часовой пояс", "", UI_LABEL_COLOR); GP.SELECT("syncGmt", "GMT-12,GMT-11,GMT-10,GMT-9,GMT-8,GMT-7,GMT-6,GMT-5,GMT-4,GMT-3,GMT-2,GMT-1,GMT+0,GMT+1,GMT+2,GMT+3,GMT+4,GMT+5,GMT+6,GMT+7,GMT+8,GMT+9,GMT+10,GMT+11,GMT+12", settings.ntpGMT + 12, 0, (boolean)(ntp.status())););
          M_BOX(GP_JUSTIFY, GP.LABEL("Автосинхронизация", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SWITCH("syncAuto", settings.ntpSync, UI_SWITCH_COLOR, (boolean)(ntp.status()));););
          GP.BUTTON("syncTime", (ntp.status()) ? "Время с устройства" : "Синхронизация с сервером", "", UI_BUTTON_COLOR);
          GP.BLOCK_END();

          GP.BLOCK_BEGIN(GP_THIN, "", "Эффекты", UI_BLOCK_COLOR);
          M_BOX(GP_JUSTIFY, GP.LABEL("Глюки", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SWITCH("mainGlitch", mainSettings.glitchMode, UI_SWITCH_COLOR);););
          M_BOX(GP.LABEL("Точки", "", UI_LABEL_COLOR); GP.SELECT("fastDot", dotModeList, fastSettings.dotMode););
          M_BOX(GP.LABEL("Минуты", "", UI_LABEL_COLOR); GP.SELECT("fastFlip", "Без анимации,Случайная смена эффектов,Плавное угасание и появление,Перемотка по порядку числа,Перемотка по порядку катодов в лампе,Поезд,Резинка,Ворота,Волна,Блики,Испарение,Игровой автомат", fastSettings.flipMode););
          M_BOX(GP.LABEL("Секунды", "", UI_LABEL_COLOR); GP.SELECT("mainSecsFlip", "Без анимации,Плавное угасание и появление,Перемотка по порядку числа,Перемотка по порядку катодов в лампе", mainSettings.secsMode, 0, (boolean)(deviceInformation[LAMP_NUM] < 6)););
          GP.HR(UI_LINE_COLOR);
          M_BOX(GP.LABEL("Подсветка", "", UI_LABEL_COLOR); GP.SELECT("fastBackl", backlModeList, fastSettings.backlMode, 0, (boolean)!deviceInformation[BACKL_TYPE]););
          M_BOX(GP.LABEL("Цвет", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER_C("fastColor", (fastSettings.backlColor < 253) ? (fastSettings.backlColor / 10) : (fastSettings.backlColor - 227), 0, 28, 1, 0, UI_SLIDER_COLOR, (boolean)!deviceInformation[BACKL_TYPE]); ); );
          GP.BLOCK_END();
        );
      }

      if (deviceInformation[ALARM_TYPE]) {
        if (alarmReload >= 2) alarmReload = 0;

        GP.BLOCK_BEGIN(GP_THIN, "", "Будильник", UI_BLOCK_COLOR);
        if (alarm.set) { //если режим настройки будильника
          GP.PAGE_TITLE("Настройка будильника");

          updateList += ',';
          updateList += "alarmVol";
          updateList += ',';
          updateList += "alarmSoundType";
          updateList += ',';
          updateList += "alarmSound";
          updateList += ',';
          updateList += "alarmRadio";
          updateList += ',';
          updateList += "alarmTime";
          updateList += ',';
          updateList += "alarmMode";

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
            updateList += ',';
            updateList += String("alarmDay/") + i;
          }
          GP.TD(GP_CENTER);
          GP.LABEL("");
          GP.TABLE_END();

          boolean alarmDelStatus = (boolean)(alarm.all > 1);

          GP.HR(UI_LINE_COLOR);
          M_BOX(GP_CENTER, M_BOX(GP_CENTER, GP.BUTTON_MINI("alarmBack", "Назад", "", UI_ALARM_BACK_COLOR, "", false, true); GP.BUTTON_MINI("alarmDel", "Удалить", "", (alarmDelStatus) ? UI_ALARM_DEL_COLOR : GP_GRAY, "", !alarmDelStatus);););
        }
        else { //иначе режим отображения
          updateList += ',';
          updateList += "alarmReload";

          GP.RELOAD("alarmReload");

          String alarmModeList[] = {"Однократно", "Ежедневно", "По будням"};
          String alarmDaysList[] = {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"};

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
            if (alarmMode) {
              if (alarmMode >= 4) {
                boolean alarmDaysFirst = false;
                uint8_t alarmDays = alarm_data[i][ALARM_DATA_DAYS];
                nowWeekDay = getWeekDay(ui.getSystemDate().year, ui.getSystemDate().month, ui.getSystemDate().day); //получить день недели;
                for (uint8_t dl = 0; dl < 7; dl++) {
                  if (alarmDays & (0x01 << nowWeekDay)) {
                    nowWeekDay = dl;
                    break;
                  }
                  if (++nowWeekDay > 7) nowWeekDay = 0;
                }
                for (uint8_t dw = 0; dw < 7; dw++) {
                  alarmDays >>= 1;
                  if (alarmDays & 0x01) {
                    if (alarmDaysFirst) alarmStatus += ", ";
                    else alarmDaysFirst = true;
                    alarmStatus += alarmDaysList[dw];
                  }
                }
              }
              else {
                alarmStatus += alarmModeList[alarmMode - 1];
              }

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
            else alarmStatus += "Отключен";

            GP.BLOCK_BEGIN(GP_THIN, "", "", UI_ALARM_BLOCK_COLOR);
            if (alarmSvgImage) {
              M_BOX(GP.LABEL(alarmTime, "", UI_ALARM_TIME_COLOR, 40, 1); GP.ICON_FILE_BUTTON(String("alarmSet/") + i, "/alarm_set.svg", 40, UI_ALARM_SET_COLOR););
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
              GP.ICON_FILE_BUTTON("alarmAdd", "/alarm_add.svg", 40, UI_ALARM_ADD_COLOR);
            }
            else {
              M_BOX(GP_CENTER, GP.BUTTON("alarmAdd", "✚", "", UI_ALARM_ADD_COLOR, "80px"););
            }
          }
          GP.RELOAD_CLICK(reloadList);
        }
        GP.BLOCK_END();
      }
    }
    else if (ui.uri("/settings")) { //настройки
      GP.PAGE_TITLE("Настройки");

      updateList += ',';
      updateList += "mainAutoShow";
      updateList += ',';
      updateList += "mainAutoShowTime";

      String alarmDotModeList = dotModeList + ",Без реакции,Мигают один раз в секунду,Мигают два раза в секунду";

      M_GRID(
        GP.BLOCK_BEGIN(GP_THIN, "", "Автопоказ", UI_BLOCK_COLOR);
        M_BOX(GP_JUSTIFY, GP.LABEL("Включить", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SWITCH("mainAutoShow", (boolean)mainSettings.autoShowTime, UI_SWITCH_COLOR);););
        M_BOX(GP_CENTER, GP.LABEL("Интервал, мин", "", UI_LABEL_COLOR);  M_BOX(GP_RIGHT, GP.SPINNER("mainAutoShowTime", mainSettings.autoShowTime, 1, 15, 1, 0, UI_SPINNER_COLOR);););
        M_BOX(GP.LABEL("Эффект", "", UI_LABEL_COLOR); GP.SELECT("mainAutoShowFlip", "Основной эффект,Случайная смена эффектов,Плавное угасание и появление,Перемотка по порядку числа,Перемотка по порядку катодов в лампе,Поезд,Резинка,Ворота,Волна,Блики,Испарение,Игровой автомат"););
        GP.HR(UI_LINE_COLOR);
        GP.LABEL("Тип данных", "", UI_HINT_COLOR);
        GP.SPAN("(источник и время в сек)", GP_CENTER, "", UI_HINT_COLOR);
        M_BOX(GP.LABEL("1", "", UI_LABEL_COLOR); GP.SELECT("extShowMode/0", "Температура,Влажность,Давление,Температура и влажность,Дата,Год,Дата и год", (extendedSettings.autoShowModes[0]) ? (extendedSettings.autoShowModes[0] - 1) : 0); M_BOX(GP_RIGHT, GP.SPINNER("extShowTime/0", extendedSettings.autoShowTimes[0], 1, 5, 1, 0, UI_SPINNER_COLOR);););
        M_BOX(GP.LABEL("2", "", UI_LABEL_COLOR); GP.SELECT("extShowMode/1", "Пусто,Температура,Влажность,Давление,Температура и влажность,Дата,Год,Дата и год", extendedSettings.autoShowModes[1]); M_BOX(GP_RIGHT, GP.SPINNER("extShowTime/1", extendedSettings.autoShowTimes[1], 1, 5, 1, 0, UI_SPINNER_COLOR);););
        M_BOX(GP.LABEL("3", "", UI_LABEL_COLOR); GP.SELECT("extShowMode/2", "Пусто,Температура,Влажность,Давление,Температура и влажность,Дата,Год,Дата и год", extendedSettings.autoShowModes[2]); M_BOX(GP_RIGHT, GP.SPINNER("extShowTime/2", extendedSettings.autoShowTimes[2], 1, 5, 1, 0, UI_SPINNER_COLOR);););
        M_BOX(GP.LABEL("4", "", UI_LABEL_COLOR); GP.SELECT("extShowMode/3", "Пусто,Температура,Влажность,Давление,Температура и влажность,Дата,Год,Дата и год", extendedSettings.autoShowModes[3]); M_BOX(GP_RIGHT, GP.SPINNER("extShowTime/3", extendedSettings.autoShowTimes[3], 1, 5, 1, 0, UI_SPINNER_COLOR);););
        M_BOX(GP.LABEL("5", "", UI_LABEL_COLOR); GP.SELECT("extShowMode/4", "Пусто,Температура,Влажность,Давление,Температура и влажность,Дата,Год,Дата и год", extendedSettings.autoShowModes[4]); M_BOX(GP_RIGHT, GP.SPINNER("extShowTime/4", extendedSettings.autoShowTimes[4], 1, 5, 1, 0, UI_SPINNER_COLOR);););
        GP.HR(UI_LINE_COLOR);
        GP.LABEL("Дополнительно", "", UI_HINT_COLOR);
        M_BOX(GP_LEFT, GP.LABEL("Коррекция, °C", "", UI_LABEL_COLOR);  M_BOX(GP_RIGHT, GP.SPINNER("mainTempCorrect", mainSettings.tempCorrect / 10.0, -12.7, 12.7, 0.1, 1, UI_SPINNER_COLOR, "", (boolean)!deviceInformation[SENS_TEMP]);););
        M_BOX(GP_LEFT, GP.LABEL("Тип датчика", "", UI_LABEL_COLOR);  M_BOX(GP_RIGHT, GP.NUMBER("", (sens.err) ? "Ошибка" : tempSensList[sens.type], INT32_MAX, "", true);););
        GP.BLOCK_END();

        GP.BLOCK_BEGIN(GP_THIN, "", "Индикаторы", UI_BLOCK_COLOR);
        GP.LABEL("Яркость", "", UI_HINT_COLOR);
        M_BOX(GP.LABEL("День", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER_C("mainIndiBrtDay", mainSettings.indiBrightDay, 5, 30, 1, 0, UI_SLIDER_COLOR); ); );//Ползунки
        M_BOX(GP.LABEL("Ночь", "", UI_LABEL_COLOR); M_BOX(GP_RIGHT, GP.SLIDER_C("mainIndiBrtNight", mainSettings.indiBrightNight, 5, 30, 1, 0, UI_SLIDER_COLOR); ); );//Ползунки
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
        GP.LABEL("Время смены яркости", "", UI_HINT_COLOR);
        M_BOX(GP_CENTER, GP.LABEL(" С", "", UI_LABEL_COLOR); GP.SPINNER("mainTimeBrightS", mainSettings.timeBrightStart, 0, 23, 1, 0, UI_SPINNER_COLOR); GP.SPINNER("mainTimeBrightE", mainSettings.timeBrightEnd, 0, 23, 1, 0, UI_SPINNER_COLOR); GP.LABEL("До", "", UI_LABEL_COLOR););
        GP.HR(UI_LINE_COLOR);
        GP.LABEL("Режим сна", "", UI_HINT_COLOR);
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
        GP.LABEL("Звук смены часа ", "", UI_HINT_COLOR);
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
        M_BOX(GP.LABEL("Время ожидания, мин", "", UI_LABEL_COLOR); GP.SPINNER("extAlarmWaitTime", extendedSettings.alarmWaitTime, 0, 240, 1, 0, UI_SPINNER_COLOR, "", (boolean)!deviceInformation[ALARM_TYPE]););
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
      GP.PAGE_TITLE("Радиоприёмник");

      updateList += ',';
      updateList += "radioVol";
      updateList += ',';
      updateList += "radioFreq";
      updateList += ',';
      updateList += "radioPower";

      M_BOX(M_BOX(GP_LEFT, GP.BUTTON_MINI("radioMode", "Часы ↻ Радио", "", UI_RADIO_BACK_COLOR);); M_BOX(GP_RIGHT, GP.LABEL("Питание", "", UI_LABEL_COLOR); GP.SWITCH("radioPower", radioSettings.powerState, UI_RADIO_POWER_COLOR););)

      M_TABLE(
        "35%",
        GP_ALS(GP_RIGHT, GP_LEFT),
        M_TR(
          GP.LABEL("Громкость", "", UI_LABEL_COLOR),
          GP.SLIDER_C("radioVol", radioSettings.volume, 0, 15, 1, 0, UI_RADIO_VOL_COLOR)
        );
        M_TR(
          GP.LABEL("Частота", "", UI_LABEL_COLOR),
          GP.SLIDER_C("radioFreq", radioSettings.stationsFreq / 10.0, 87.5, 108, 0.1, 1, UI_RADIO_FREQ_1_COLOR)
        );
      );
      M_BOX(GP.BUTTON("radioSeekDown", "|◄◄", "", UI_RADIO_FREQ_2_COLOR); GP.BUTTON("radioFreqDown", "◄", "", UI_RADIO_FREQ_2_COLOR); GP.BUTTON("radioFreqUp", "►", "", UI_RADIO_FREQ_2_COLOR); GP.BUTTON("radioSeekUp", "►►|", "", UI_RADIO_FREQ_2_COLOR););
      /*
        //средние кнопки
        GP.BOX_BEGIN(GP_CENTER, "70%", 1); GP.BUTTON("radioSeekDown", "|◄◄"); GP.BUTTON("radioFreqDown", "◄"); GP.BUTTON("radioFreqUp", "►"); GP.BUTTON("radioSeekUp", "►►|"); GP.BOX_END();
        //маленькие кнопки
        GP.BOX_BEGIN(GP_CENTER, "100%", 1); GP.BUTTON_MINI("radioSeekDown", "|◄◄"); GP.BUTTON_MINI("radioFreqDown", " ◄ "); GP.BUTTON_MINI("radioFreqUp", " ► "); GP.BUTTON_MINI("radioSeekUp", "►►|"); GP.BOX_END();
      */
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
      GP.SPAN("Здесь можно обновить прошивку ESP, формат файла bin. Его можно получить открыв прошивку в Arduino IDE: Скетч -> Экспорт бинарного файла (сохраняется в папку с прошивкой)", GP_CENTER, "", UI_INFO_COLOR);     //выравнивание (GP_CENTER, GP_LEFT, GP_RIGHT, GP_JUSTIFY), умолч. GP_CENTER
      GP.HR(UI_LINE_COLOR);
      M_BOX(GP.LABEL("Обновить прошивку ESP", "", UI_LABEL_COLOR); GP.OTA_FIRMWARE(""););
      //M_BOX(GP.LABEL("Обновить файловую систему ESP", "", UI_LABEL_COLOR); GP.OTA_FILESYSTEM(""););
      GP.BLOCK_END();
    }
    else if (ui.uri("/network")) { //подключение к роутеру
      GP.PAGE_TITLE("Сетевые настройки");

      GP.BLOCK_BEGIN(GP_THIN, "", "Локальная сеть WIFI", UI_BLOCK_COLOR);
      if (WiFi.status() != WL_CONNECTED) {
        GP.FORM_BEGIN("/");
        GP.TEXT("login", "Логин", settings.ssid);
        GP.BREAK();
        GP.TEXT("pass", "Пароль", settings.pass);
        GP.HR(UI_LINE_COLOR);
        GP.SUBMIT("Подключиться", UI_BUTTON_COLOR);
        GP.FORM_END();
      }
      else {
        GP.FORM_BEGIN("/network");
        GP.LABEL("Подключено к \"" + String(settings.ssid) + "\"", "", UI_INFO_COLOR);
        GP.LABEL("IP адрес \"" + WiFi.localIP().toString() + "\"", "", UI_INFO_COLOR);
        GP.HR(UI_LINE_COLOR);
        GP.SUBMIT("Отключиться", UI_BUTTON_COLOR);
        GP.FORM_END();
      }
      GP.BLOCK_END();
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
        if (setSyncTime()) {
          if (deviceInformation[HARDWARE_VERSION] == HW_VERSION) {
            busSetComand(WRITE_TIME);
            busSetComand(WRITE_DATE);
          }
        }
        memory.update(); //обновить данные в памяти
      }
      if (ui.click("syncTime")) {
        if ((WiFi.status() != WL_CONNECTED) || !updateTime()) { //запросить текущее время
          mainTime = ui.getSystemTime(); //запросить время браузера
        }
        if (deviceInformation[HARDWARE_VERSION] == HW_VERSION) {
          busSetComand(WRITE_TIME);
          busSetComand(WRITE_DATE);
        }
      }
      if (ui.clickBool("syncAuto", settings.ntpSync)) {
        memory.update(); //обновить данные в памяти
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
        if (ui.click("alarmDel") && !alarmReload) {
          if (alarm.all > 1) {
            alarm.all--;
            busSetComand(DEL_ALARM, alarm.now);
            if (alarm.now >= alarm.all) alarm.now = alarm.all - 1;
            alarm.set = 0;
            alarmReload = 1;
            busSetComand(READ_ALARM_ALL);
          }
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
        if (ui.click("alarmAdd") && !alarmReload) {
          if (alarm.all < MAX_ALARMS) {
            alarm.all++;
            alarm.now = alarm.all - 1;
            alarm.set = 0;
            alarmReload = 1;
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
        if (deviceInformation[HARDWARE_VERSION] == HW_VERSION) busSetComand(WRITE_DATE);
      }
      if (ui.clickTime("mainTime", mainTime)) {
        if (deviceInformation[HARDWARE_VERSION] == HW_VERSION) busSetComand(WRITE_TIME);
      }
      if (ui.clickBool("mainTimeFormat", mainSettings.timeFormat)) {
        if (deviceInformation[HARDWARE_VERSION] == HW_VERSION) busSetComand(WRITE_MAIN_SET, MAIN_TIME_FORMAT);
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
      //отключаем wifi
      WiFi.disconnect();
      //включаем точку доступа
      WiFi.mode(WIFI_AP);
      ntp.end(); //остановили ntp
    }
    else if (ui.form("/")) {
      if (WiFi.status() != WL_CONNECTED) {
        ui.copyStr("login", settings.ssid); //копируем себе
        ui.copyStr("pass", settings.pass);
        memory.update(); //обновить данные в памяти
        ui.stop(); //остановили портал
      }
    }
  }
  /**************************************************************************/
  if (ui.update()) {
    if (ui.updateSub("bar")) {
      if (ui.update("barTime")) {   //начинается
        ui.answer(ui.getSystemTime().encode());
        if (deviceInformation[HARDWARE_VERSION] == HW_VERSION) busSetComand(READ_STATUS);
      }
      if (ui.update("barTemp")) {   //начинается
        ui.answer(String((sens.temp + mainSettings.tempCorrect) / 10.0, 1) + "°С");
      }
      if (ui.update("barHum")) {   //начинается
        ui.answer(String(sens.hum) + "%");
      }
      if (ui.update("barPress")) {   //начинается
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
      else {
        if (ui.update("alarmReload") && (alarmReload == 2)) { //если было обновление
          ui.answer(1);
          alarmReload = 0;
        }
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
    }
    //--------------------------------------------------------------------
    if (ui.updateSub("radio")) {
      if (ui.update("radioVol")) { //если было обновление
        ui.answer(constrain(radioSettings.volume, 0, 15));
      }
      if (ui.update("radioFreq")) { //если было обновление
        ui.answer(constrain(radioSettings.stationsFreq, 870, 1080) / 10.0, 1);
      }
      if (ui.update("radioPower")) { //если было обновление
        ui.answer(radioSettings.powerState);
        if (deviceInformation[HARDWARE_VERSION] == HW_VERSION) busSetComand(READ_RADIO_POWER);
      }
    }
  }
}

const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //дней в месяце
//---------------------------------Получить день недели-----------------------------------
uint8_t getWeekDay(uint16_t YY, uint8_t MM, uint8_t DD) //получить день недели
{
  if (YY >= 2000) YY -= 2000; //если год больше 2000
  uint16_t days = DD; //записываем дату
  for (uint8_t i = 1; i < MM; i++) days += daysInMonth[i - 1]; //записываем сколько дней прошло до текущего месяца
  if ((MM > 2) && !(YY % 4)) days++; //если високосный год, прибавляем день
  return (days + 365 * YY + (YY + 3) / 4 - 2 + 6) % 7 + 1; //возвращаем день недели
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
  climateTempAvg = 0;
  climateHumAvg = 0;
  climatePressAvg = 0;
  climateCountAvg = 0;
}

void climateUpdate(void) {
  static boolean firstStart;

  if (mainDate.year) {
    uint32_t unixNow = 0;
    if (!ntp.synced() || ntp.status()) unixNow = GPunix(mainDate.year, mainDate.month, mainDate.day, mainTime.hour, mainTime.minute, mainTime.second, settings.ntpGMT);
    else unixNow = ntp.unix();

    if (!firstStart) {
      firstStart = true;
      for (uint8_t i = 0; i < CLIMATE_BUFFER; i++) {
        climateAdd(sens.temp + mainSettings.tempCorrect, sens.hum, sens.press, unixNow);
      }
      climateReset(); //сброс усреднения
    }
    else {
      if (settings.climateAvg) {
        climateTempAvg += sens.temp + mainSettings.tempCorrect;
        climateHumAvg += sens.hum;
        climatePressAvg += sens.press;
      }
      else {
        climateTempAvg = sens.temp + mainSettings.tempCorrect;
        climateHumAvg = sens.hum;
        climatePressAvg = sens.press;
      }

      if (++climateCountAvg >= settings.climateTime) {
        if (settings.climateAvg) {
          if (climateTempAvg) climateTempAvg /= climateCountAvg;
          if (climateHumAvg) climateHumAvg /= climateCountAvg;
          if (climatePressAvg) climatePressAvg /= climateCountAvg;
        }
        climateAdd(climateTempAvg, climateHumAvg, climatePressAvg, unixNow);
        climateReset(); //сброс усреднения
      }
    }
  }
}

boolean updateTime(void) {
  if (!ntp.updateNow()) {
    mainTime.second = ntp.second();
    mainTime.minute = ntp.minute();
    mainTime.hour = ntp.hour();
    mainDate.day = ntp.day();
    mainDate.month = ntp.month();
    mainDate.year = ntp.year();
    return true;
  }
  return false;
}
boolean setSyncTime(void) {
  if (ntp.synced() && !ntp.status()) {
    mainTime.second = ntp.second();
    mainTime.minute = ntp.minute();
    mainTime.hour = ntp.hour();
    mainDate.day = ntp.day();
    mainDate.month = ntp.month();
    mainDate.year = ntp.year();
    return true;
  }
  return false;
}

void wifi_config(void) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println F("");
    Serial.print F("Connecting to settings \"");
    Serial.print(settings.ssid);
    Serial.print F("\"...");

    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(settings.ssid, settings.pass);
    for (int8_t i = 50; i > 0; i--) {
      Serial.print F(".");
      delay(500);
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println F("");
        Serial.println F("Wifi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        ntp.begin(); //запустить ntp
        ntp.setHost(NTP_HOST); //установить хост
        ntp.setGMT(settings.ntpGMT); //установить часовой пояс в часах
        break;
      }
    }
  }

  IPAddress apIP(AP_IP);
  IPAddress subnet(255, 255, 255, 0);
  //если не удалось подключиться запускаем в режиме точки доступа
  Serial.println F("");
  Serial.print F("Access Point ssid: ");
  Serial.println(AP_SSID);
  Serial.print F("Access Point pass: ");
  Serial.println(AP_PASS);
  if (WiFi.status() != WL_CONNECTED) {
    //отключаем wifi
    WiFi.disconnect();
    //включаем точку доступа
    WiFi.mode(WIFI_AP);
  }
  //задаем настройки сети
  WiFi.softAPConfig(apIP, apIP, subnet);
  //включаем wifi в режиме точки доступа с именем и паролем по умолчанию
  WiFi.softAP(AP_SSID, AP_PASS);
}

void setup() {
  twi_init(); //инициализация шины

  Serial.begin(115200);
  Serial.println("");
  if (!LittleFS.begin()) Serial.println("FS error");
  else {
    Serial.println("FS init");

    FSInfo fs_info;
    LittleFS.info(fs_info);
    if (fs_info.totalBytes < 500000) Serial.println("FS running out of memory");
    else {
      File file = LittleFS.open("/gp_data/PLOT_STOCK.js", "r");
      if (!file) Serial.println("Script file error");
      else {
        file.close();
        climateLocal = true; //работаем локально
        Serial.println("Script file found");
      }

      file = LittleFS.open("/alarm_add.svg", "r");
      if (!file) Serial.println("Alarm add svg not found");
      else {
        file.close();
        file = LittleFS.open("/alarm_set.svg", "r");
        if (!file) Serial.println("Alarm set svg not found");
        else {
          file.close();
          alarmSvgImage = true; //работаем локально
          Serial.println("Alarm svg files found");
        }
      }
    }
  }

  if (ESP.getFreeSketchSpace() < ESP.getSketchSize()) {
    otaUpdate = false; //выключаем обновление
    Serial.println("OTA disable, running out of memory");
  }
  else Serial.println("OTA enable");

  //читаем логин пароль из памяти
  EEPROM.begin(memory.blockSize());
  if (memory.begin(0, 0xA9) == 1) {
    for (uint8_t i = 0; i < 20; i++) {
      settings.ssid[i] = '\0';
      settings.pass[i] = '\0';
    }
    settings.climateTime = DEFAULT_CLIMATE_TIME;
    settings.climateAvg = DEFAULT_CLIMATE_AVG;
    settings.ntpGMT = DEFAULT_GMT; //установить часовой по умолчанию
    settings.ntpSync = false; //выключаем авто-синхронизацию
    memory.update(); //обновить данные в памяти
  }
  alarm.now = 0;

  busSetComand(READ_DEVICE);
  busSetComand(WRITE_CHECK_SENS);
  busSetComand(READ_FAST_SET);
  busSetComand(READ_MAIN_SET);
  busSetComand(READ_EXTENDED_SET);
  busSetComand(READ_RADIO_SET);
  busSetComand(READ_ALARM_ALL);
  busSetComand(READ_SENS_DATA);
  busSetComand(READ_SENS_INFO);
  busTimerSetInterval(5000);
}

void loop() {
  if (!ui.state()) { //если портал не запущен
    //поключаемся к wifi
    wifi_config();
    //подключаем конструктор и запускаем
    ui.attachBuild(build);
    ui.attach(action);
    ui.start();
    if (otaUpdate) ui.enableOTA(); //обновление без пароля
    ui.downloadAuto(true);
  }

  if (ntp.tick()) {
    if (!ntp.status()) {
      mainTime.second = ntp.second();
      mainTime.minute = ntp.minute();
      mainTime.hour = ntp.hour();
      mainDate.day = ntp.day();
      mainDate.month = ntp.month();
      mainDate.year = ntp.year();
      if (settings.ntpSync) {
        busSetComand(WRITE_TIME);
        busSetComand(WRITE_DATE);
      }
    }
  }

  ui.tick();

  static uint32_t timer = millis();
  if ((millis() - timer) >= 60000) {
    timer = millis();
    if (deviceInformation[HARDWARE_VERSION] == HW_VERSION) {
      busSetComand(WRITE_CHECK_SENS);
      if (!ntp.synced() || ntp.status()) busSetComand(READ_TIME_DATE);
      busSetComand(READ_SENS_DATA);
    }
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
        }
      }
      deviceStatus >>= 1; //сместили буфер флагов
    }
    deviceStatus = 0;
  }

  busUpdate();
  memory.tick();
}
