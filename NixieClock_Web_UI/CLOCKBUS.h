//Информация о интефейсе
#define HW_VERSION 0x13 //версия прошивки для интерфейса wire

//Команды интерфейса
#define BUS_WRITE_TIME 0x01
#define BUS_READ_TIME 0x02

#define BUS_WRITE_FAST_SET 0x03
#define BUS_READ_FAST_SET 0x04

#define BUS_WRITE_MAIN_SET 0x05
#define BUS_READ_MAIN_SET 0x06

#define BUS_READ_ALARM_NUM 0x07
#define BUS_WRITE_SELECT_ALARM 0x08
#define BUS_READ_SELECT_ALARM 0x09
#define BUS_DEL_ALARM 0x0C
#define BUS_NEW_ALARM 0x0D

#define BUS_READ_RADIO_SET 0x0E
#define BUS_WRITE_RADIO_STA 0x0F
#define BUS_WRITE_RADIO_VOL 0x10
#define BUS_WRITE_RADIO_FREQ 0x11
#define BUS_WRITE_RADIO_MODE 0x12
#define BUS_WRITE_RADIO_POWER 0x13
#define BUS_SEEK_RADIO_UP 0x14
#define BUS_SEEK_RADIO_DOWN 0x15
#define BUS_READ_RADIO_POWER 0x16

#define BUS_CHECK_TEMP 0x17
#define BUS_READ_TEMP 0x18

#define BUS_WRITE_EXTENDED_SET 0x19
#define BUS_READ_EXTENDED_SET 0x1A

#define BUS_SET_SHOW_TIME 0x1B
#define BUS_SET_BURN_TIME 0x1C
#define BUS_SET_UPDATE 0x1E

#define BUS_WRITE_TIMER_SET 0x1F
#define BUS_READ_TIMER_SET 0x20
#define BUS_WRITE_TIMER_MODE 0x21

#define BUS_WRITE_SENS_DATA 0x22
#define BUS_WRITE_MAIN_SENS_DATA 0x23

#define BUS_READ_FAILURE 0xA0

#define BUS_ALARM_DISABLE 0xDA
#define BUS_CHANGE_BRIGHT 0xDC

#define BUS_TEST_FLIP 0xEA
#define BUS_TEST_SOUND 0xEB
#define BUS_STOP_SOUND 0xEC

#define BUS_CONTROL_DEVICE 0xFA

#define BUS_SELECT_BYTE 0xFD
#define BUS_READ_STATUS 0xFE
#define BUS_READ_DEVICE 0xFF

//-----------------Настройки----------------
struct Settings_1 {
  uint8_t indiBrightNight; //яркость индикаторов
  uint8_t indiBrightDay;
  uint8_t backlBrightNight; //яркость подсветки
  uint8_t backlBrightDay;
  uint8_t dotBrightNight; //яркость точек
  uint8_t dotBrightDay;
  uint8_t timeBrightStart; //время перехода яркости
  uint8_t timeBrightEnd;
  uint8_t timeHourStart; //время звукового оповещения нового часа
  uint8_t timeHourEnd;
  uint8_t timeSleepNight; //время режима сна
  uint8_t timeSleepDay;
  boolean timeFormat; //формат времени
  boolean knockSound; //звук кнопок или озвучка
  uint8_t hourSound; //тип озвучки смены часа
  uint8_t volumeSound; //громкость озвучки
  uint8_t voiceSound; //голос озвучки
  int8_t tempCorrect; //коррекция температуры
  boolean glitchMode; //режим глюков
  uint8_t autoShowTime; //интервал времени автопоказа
  uint8_t autoShowFlip; //режим анимации автопоказа
  uint8_t burnMode; //режим антиотравления индикаторов
  uint8_t burnTime; //интервал антиотравления индикаторов
} mainSettings;

struct Settings_2 {
  uint8_t flipMode; //режим анимации минут
  uint8_t secsMode; //режим анимации секунд
  uint8_t dotMode; //режим точек
  uint8_t backlMode; //режим подсветки
  uint8_t backlColor; //цвет подсветки
} fastSettings;

struct Settings_3 { //расширенные настройки
  uint8_t autoShowModes[5];
  uint8_t autoShowTimes[5];
  uint8_t alarmTime;
  uint8_t alarmWaitTime;
  uint8_t alarmSoundTime;
  uint8_t alarmDotOn;
  uint8_t alarmDotWait;
  uint8_t tempCorrectSensor;
  uint8_t tempMainSensor;
  uint8_t tempHourSensor;
} extendedSettings;

struct Settings_4 { //настройки радио
  boolean powerState; //состояние питания
  uint8_t volume; //текущая громкость
  uint16_t stationsFreq; //текущая частота
  uint16_t stationsSave[10]; //память радиостанций
} radioSettings;

//------------Таймера/Секундомер-----------
struct timerData {
  uint8_t mode; //режим таймера/секундомера
  uint16_t count; //счетчик таймера/секундомера
  uint16_t time; //время таймера сек
  uint8_t hour; //количество часов таймера
  uint8_t mins; //количество минут таймера
  uint8_t secs; //количество секунд таймера
} timer;

//----------------Будильники--------------
struct alarmData {
  uint8_t set; //настройка текущего будильника
  uint8_t all; //всего будильников
  uint8_t now; //текущий будильник
  uint8_t num; //текущий будильник
  uint8_t reload; //флаг обновления страницы будильника
  boolean status; //флаг статуса будильника
} alarm;

enum {
  ALARM_DATA_HOUR, //время будильника
  ALARM_DATA_MINS, //время будильника
  ALARM_DATA_MODE, //режим будильника
  ALARM_DATA_DAYS, //день недели будильника
  ALARM_DATA_SOUND, //мелодия будильника
  ALARM_DATA_STATION, //станция будильника
  ALARM_DATA_VOLUME, //громкость будильника
  ALARM_DATA_RADIO, //радиобудильник
  ALARM_DATA_MAX //всего данных
};
uint8_t alarm_data[MAX_ALARMS][ALARM_DATA_MAX];

enum {
  FIRMWARE_VERSION_1,
  FIRMWARE_VERSION_2,
  FIRMWARE_VERSION_3,
  HARDWARE_VERSION,
  SENS_TEMP,
  BTN_EASY_MAIN_MODE,
  LAMP_NUM,
  BACKL_TYPE,
  NEON_DOT,
  DOTS_PORT_ENABLE,
  DOTS_NUM,
  DOTS_TYPE,
  LIGHT_SENS_ENABLE,
  EXT_BTN_ENABLE,
  DS3231_ENABLE,
  TIMER_ENABLE,
  RADIO_ENABLE,
  ALARM_TYPE,
  PLAYER_TYPE,
  PLAYER_MAX_SOUND,
  PLAYER_MAX_VOICE,
  ALARM_AUTO_VOL_MAX,
  INFORMATION_MAX
};
uint8_t deviceInformation[INFORMATION_MAX]; //информация о часах

enum {
  STATUS_UPDATE_MAIN_SET,
  STATUS_UPDATE_FAST_SET,
  STATUS_UPDATE_RADIO_SET,
  STATUS_UPDATE_EXTENDED_SET,
  STATUS_UPDATE_ALARM_SET,
  STATUS_UPDATE_TIME_SET,
  STATUS_UPDATE_SENS_DATA,
  STATUS_MAX_DATA
};

enum {
  MAIN_INDI_BRIGHT_N,
  MAIN_INDI_BRIGHT_D,
  MAIN_BACKL_BRIGHT_N,
  MAIN_BACKL_BRIGHT_D,
  MAIN_DOT_BRIGHT_N,
  MAIN_DOT_BRIGHT_D,
  MAIN_TIME_BRIGHT_S,
  MAIN_TIME_BRIGHT_E,
  MAIN_TIME_HOUR_S,
  MAIN_TIME_HOUR_E,
  MAIN_TIME_SLEEP_N,
  MAIN_TIME_SLEEP_D,
  MAIN_TIME_FORMAT,
  MAIN_KNOCK_SOUND,
  MAIN_HOUR_SOUND,
  MAIN_VOLUME_SOUND,
  MAIN_VOICE_SOUND,
  MAIN_TEMP_CORRECT,
  MAIN_GLITCH_MODE,
  MAIN_AUTO_SHOW_TIME,
  MAIN_AUTO_SHOW_FLIP,
  MAIN_BURN_MODE,
  MAIN_BURN_TIME
};

enum {
  FAST_FLIP_MODE,
  FAST_SECS_MODE,
  FAST_DOT_MODE,
  FAST_BACKL_MODE,
  FAST_BACKL_COLOR
};

enum {
  ALARM_TIME, //время будильника
  ALARM_MODE, //режим будильника
  ALARM_DAYS, //день недели будильника
  ALARM_SOUND, //мелодия будильника
  ALARM_VOLUME, //громкость будильника
  ALARM_RADIO //радиобудильник
};

enum {
  EXT_ALARM_TIMEOUT,
  EXT_ALARM_WAIT,
  EXT_ALARM_TIMEOUT_SOUND,
  EXT_ALARM_DOT_ON,
  EXT_ALARM_DOT_WAIT
};

enum {
  EXT_SENSOR_CORRECT,
  EXT_SENSOR_MAIN,
  EXT_SENSOR_HOUR
};
enum {
  SENS_DATA_MAIN,
  SENS_DATA_EXT
};

enum {
  SYNC_TIME_DATE,
  READ_TIME_DATE,
  WRITE_TIME_DATE,
  WRITE_TIME,
  WRITE_DATE,

  READ_FAST_SET,
  WRITE_FAST_SET,

  READ_MAIN_SET,
  WRITE_MAIN_SET,

  READ_ALARM_ALL,
  READ_ALARM_NUM,
  READ_ALARM_DATA,
  READ_SELECT_ALARM,
  WRITE_SELECT_ALARM,
  DEL_ALARM,
  NEW_ALARM,

  READ_RADIO_SET,
  READ_RADIO_STA,
  WRITE_RADIO_STA,
  READ_RADIO_VOL,
  WRITE_RADIO_VOL,
  READ_RADIO_FREQ,
  WRITE_RADIO_FREQ,
  WRITE_RADIO_MODE,
  READ_RADIO_POWER,
  WRITE_RADIO_POWER,

  RADIO_FREQ_UP,
  RADIO_FREQ_DOWN,
  RADIO_SEEK_UP,
  RADIO_SEEK_DOWN,

  READ_SENS_DATA,
  READ_SENS_INFO,
  WRITE_CHECK_SENS,

  READ_EXTENDED_SET,
  WRITE_EXTENDED_SHOW_MODE,
  WRITE_EXTENDED_SHOW_TIME,
  WRITE_EXTENDED_SENSOR_SET,
  WRITE_EXTENDED_ALARM,

  SET_SHOW_TIME,
  SET_BURN_TIME,
  SET_UPDATE,

  READ_TIMER_STATE,
  WRITE_TIMER_STATE,
  READ_TIMER_TIME,
  WRITE_TIMER_TIME,
  READ_TIMER_SET,
  WRITE_TIMER_SET,
  WRITE_TIMER_MODE,

  WRITE_ALARM_DISABLE,
  WRITE_CHANGE_BRIGHT,

  WRITE_STOP_SOUND,
  WRITE_TEST_MAIN_VOL,
  WRITE_TEST_ALARM_VOL,
  WRITE_TEST_ALARM_SOUND,
  WRITE_TEST_MAIN_VOICE,
  WRITE_TEST_MAIN_FLIP,

  READ_STATUS,
  READ_DEVICE,
  READ_FAILURE,

  WRITE_SENS_1_DATA,
  WRITE_SENS_2_DATA,

  CONTROL_SYSTEM,
  CONTROL_DEVICE,
  UPDATE_FIRMWARE,

  WRITE_RTC_INIT,
  WRITE_RTC_TIME,
  READ_RTC_TIME,
  WRITE_RTC_AGING,
  READ_RTC_AGING,

  CHECK_INTERNAL_AHT,
  CHECK_INTERNAL_SHT,
  CHECK_INTERNAL_BME
};

struct deviceData {
  uint8_t status = 0x00; //состояние часов
  uint8_t light = 0xFF; //яркость подсветки часов
  uint16_t failure = 0x00; //сбои при запуске часов
} device;

struct busData {
  uint8_t buffer[256];
  uint8_t bufferStart = 0;
  uint8_t bufferEnd = 0;
  uint8_t bufferLastCommand = 0;
  uint8_t bufferLastArgument = 0;
  uint8_t status = 0;
  uint16_t timerInterval = 0;
  uint32_t timerStart = 0;
} bus;
int8_t clockState = 0; //флаг состояния соединения с часами

#define DEVICE_RESET 0xCC
#define DEVICE_UPDATE 0xDD
#define DEVICE_REBOOT 0xEE

#define SYSTEM_REBOOT 100

#define BUS_STATUS_REBOOT 100
#define BUS_STATUS_REBOOT_FAIL 255

#include "CLIMATE.h"

#include "AHT.h"
#include "SHT.h"
#include "BME.h"

#include "RTC.h"

void busWriteTwiRegByte(uint8_t data, uint8_t command, uint8_t pos = 0x00);
void busWriteTwiRegWord(uint16_t data, uint8_t command, uint8_t pos = 0x00);

//--------------------------------------------------------------------
void busWriteTwiRegByte(uint8_t data, uint8_t command, uint8_t pos) {
  if (pos) {
    twi_write_byte(BUS_SELECT_BYTE);
    twi_write_byte(pos);
  }
  twi_write_byte(command); //регистр команды
  twi_write_byte(data); //отправляем дополнительные натройки
}
//--------------------------------------------------------------------
void busWriteTwiRegWord(uint16_t data, uint8_t command, uint8_t pos) {
  if (pos) {
    twi_write_byte(BUS_SELECT_BYTE);
    twi_write_byte(pos * 2);
  }
  twi_write_byte(command); //регистр команды
  twi_write_byte(data & 0xFF); //отправляем дополнительные натройки
  twi_write_byte((data >> 8) & 0xFF); //отправляем дополнительные натройки
}
//--------------------------------------------------------------------
boolean busGetClockStatus(void) {
  return (boolean)(clockState != 0);
}
//--------------------------------------------------------------------
void busUpdateClockStatus(void) {
  if (clockState > 0) clockState--; //если есть попытки подключения
  else if (clockState < 0) clockState = 3; //иначе первый запрос состояния
}
//--------------------------------------------------------------------
void busTimerSetInterval(uint16_t time) {
  bus.timerInterval = time;
  bus.timerStart = millis();
}
//--------------------------------------------------------------------
inline boolean busTimerCheck(void) {
  if ((millis() - bus.timerStart) >= bus.timerInterval) return true;
  return false;
}
//--------------------------------------------------------------------
boolean busStatusBuffer(void) {
  return (boolean)(bus.bufferStart != bus.bufferEnd);
}
//--------------------------------------------------------------------
uint8_t busFreeBuffer(void) {
  return (uint8_t)(bus.bufferStart - bus.bufferEnd - 1);
}
//--------------------------------------------------------------------
void busWriteBuffer(uint8_t data) {
  if (busFreeBuffer()) {
    bus.buffer[bus.bufferEnd] = data;
    bus.bufferEnd++;
  }
}
//--------------------------------------------------------------------
uint8_t busReadBuffer(void) {
  return bus.buffer[bus.bufferStart];
}
//--------------------------------------------------------------------
uint8_t busReadBufferArg(void) {
  return bus.buffer[(uint8_t)(bus.bufferStart + 1)];
}
//--------------------------------------------------------------------
void busShiftBuffer(void) {
  if (busStatusBuffer()) bus.bufferStart++;
}
//--------------------------------------------------------------------
void busSetCommand(uint8_t cmd) {
  if (!busStatusBuffer() || (busFreeBuffer() && (bus.bufferLastCommand != cmd))) {
    bus.bufferLastCommand = cmd;
    busWriteBuffer(cmd);
  }
}
//--------------------------------------------------------------------
void busSetCommand(uint8_t cmd, uint8_t arg) {
  if (!busStatusBuffer() || ((busFreeBuffer() >= 2) && ((bus.bufferLastCommand != cmd) || (bus.bufferLastArgument != arg)))) {
    bus.bufferLastCommand = cmd;
    bus.bufferLastArgument = arg;
    busWriteBuffer(cmd);
    busWriteBuffer(arg);
  }
}
//--------------------------------------------------------------------
void busRebootDevice(uint8_t arg) {
  bus.bufferStart = bus.bufferEnd = 0;
  bus.status = BUS_STATUS_REBOOT;
  if (deviceInformation[HARDWARE_VERSION] && (arg != SYSTEM_REBOOT)) busSetCommand(CONTROL_DEVICE, arg);
  else busSetCommand(CONTROL_SYSTEM, SYSTEM_REBOOT);
  busTimerSetInterval(500);
}
//--------------------------------------------------------------------
boolean busRebootState(void) {
  return bus.status == BUS_STATUS_REBOOT;
}
//--------------------------------------------------------------------
boolean busRebootFail(void) {
  return bus.status == BUS_STATUS_REBOOT_FAIL;
}
//--------------------------------------------------------------------
void busUpdate(void) {
  if (busTimerCheck()) { //если таймаут истек
    if (busStatusBuffer()) { //если есть новая команда
      switch (busReadBuffer()) {
        case SYNC_TIME_DATE: {
            if ((timeState != 0x03) || ntpCheckTime(GPunix(mainDate, mainTime, settings.ntpGMT), syncState)) {

              time_t unix = ntpGetUnix() + (settings.ntpGMT * 3600UL);
              secondsTimer = millis() - ntpGetMillis();

              tm time; //буфер времени
              gmtime_r(&unix, &time);

              mainTime.second = time.tm_sec;
              mainTime.minute = time.tm_min;
              mainTime.hour = time.tm_hour;
              uint8_t dayWeek = time.tm_wday;
              if (!dayWeek) dayWeek = 7;
              mainDate.day = time.tm_mday;
              mainDate.month = time.tm_mon + 1;
              mainDate.year = time.tm_year + 1900;

              if (settings.ntpDst && DST(mainDate.month, mainDate.day, dayWeek, mainTime.hour)) { //если учет летнего времени включен
                syncState = 1; //летнее время
                if (mainTime.hour != 23) mainTime.hour += 1; //прибавили час
                else {
                  mainTime.hour = 0; //сбросили час
                  if (++dayWeek > 7) dayWeek = 1; //день недели
                  if (++mainDate.day > maxDays(mainDate.year, mainDate.month)) { //день
                    mainDate.day = 1; //сбросили день
                    if (++mainDate.month > 12) { //месяц
                      mainDate.month = 1; //сбросили месяц
                      if (++mainDate.year > 2099) { //год
                        mainDate.year = 2000; //сбросили год
                      }
                    }
                  }
                }
              }
              else syncState = 2; //зимнее время

              if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
                twi_write_byte(BUS_WRITE_TIME); //регистр команды
                twi_write_byte(mainTime.second); //отправляем время
                twi_write_byte(mainTime.minute);
                twi_write_byte(mainTime.hour);
                twi_write_byte(mainDate.day); //отправляем дату
                twi_write_byte(mainDate.month);
                twi_write_byte(mainDate.year & 0xFF);
                twi_write_byte((mainDate.year >> 8) & 0xFF);
                twi_write_byte(dayWeek); //отправляем день недели
                if (!twi_error()) { //если передача была успешной
                  if (rtcGetFoundStatus()) busSetCommand(WRITE_RTC_TIME); //отправить время в RTC
                  if (timeState != 0x03) sensorTimer = 0; //обновляем состояние микроклимата
                  timeState = 0x03; //установили флаги актуального времени
                  busShiftBuffer(); //сместили буфер команд
                }
              }
            }
            else {
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case READ_TIME_DATE:
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_TIME)) { //начинаем передачу
            mainTime.second = twi_read_byte(TWI_ACK); //принимаем время
            mainTime.minute = twi_read_byte(TWI_ACK);
            mainTime.hour = twi_read_byte(TWI_ACK);
            mainDate.day = twi_read_byte(TWI_ACK);
            mainDate.month = twi_read_byte(TWI_ACK);
            mainDate.year = twi_read_byte(TWI_ACK) | ((uint16_t)twi_read_byte(TWI_NACK) << 8);
            if (!twi_error()) { //если передача была успешной
              if (busReadBufferArg()) { //если время обновлено в часах
                if (rtcGetFoundStatus()) busSetCommand(WRITE_RTC_TIME); //отправить время в RTC
                if (timeState != 0x03) sensorTimer = 0; //обновляем состояние микроклимата
                timeState = 0x03; //установили флаги актуального времени
              }
              busShiftBuffer(); //сместили буфер команд
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_TIME_DATE:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_WRITE_TIME); //регистр команды
            twi_write_byte(mainTime.second); //отправляем время
            twi_write_byte(mainTime.minute);
            twi_write_byte(mainTime.hour);
            twi_write_byte(mainDate.day); //отправляем дату
            twi_write_byte(mainDate.month);
            twi_write_byte(mainDate.year & 0xFF);
            twi_write_byte((mainDate.year >> 8) & 0xFF);
            twi_write_byte(getWeekDay(mainDate.year, mainDate.month, mainDate.day));
            if (!twi_error()) { //если передача была успешной
              if (timeState != 0x03) sensorTimer = 0; //обновляем состояние микроклимата
              timeState = 0x03; //установили флаги актуального времени
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_TIME:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_WRITE_TIME); //регистр команды
            twi_write_byte(mainTime.second); //отправляем время
            twi_write_byte(mainTime.minute);
            twi_write_byte(mainTime.hour);
            if (!twi_error()) { //если передача была успешной
              if (rtcGetFoundStatus()) busSetCommand(WRITE_RTC_TIME); //отправить время в RTC
              if (timeState != 0x03) sensorTimer = 0; //обновляем состояние микроклимата
              timeState |= 0x01; //установили флаг актуального времени
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_DATE:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_SELECT_BYTE);
            twi_write_byte(0x03);
            twi_write_byte(BUS_WRITE_TIME); //регистр команды
            twi_write_byte(mainDate.day); //отправляем дату
            twi_write_byte(mainDate.month);
            twi_write_byte(mainDate.year & 0xFF);
            twi_write_byte((mainDate.year >> 8) & 0xFF);
            twi_write_byte(getWeekDay(mainDate.year, mainDate.month, mainDate.day));
            if (!twi_error()) { //если передача была успешной
              if (rtcGetFoundStatus()) busSetCommand(WRITE_RTC_TIME); //отправить время в RTC
              if (timeState != 0x03) sensorTimer = 0; //обновляем состояние микроклимата
              timeState |= 0x02; //установили флаг актуальной даты
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;

        case READ_FAST_SET:
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_FAST_SET)) { //начинаем передачу
            fastSettings.flipMode = twi_read_byte(TWI_ACK); //принимаем дополнительные натройки
            fastSettings.secsMode = twi_read_byte(TWI_ACK);
            fastSettings.dotMode = twi_read_byte(TWI_ACK);
            fastSettings.backlMode = twi_read_byte(TWI_ACK);
            fastSettings.backlColor = twi_read_byte(TWI_NACK);
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_FAST_SET:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            switch (busReadBufferArg()) {
              case FAST_FLIP_MODE: busWriteTwiRegByte(fastSettings.flipMode, BUS_WRITE_FAST_SET, 0); busWriteBuffer(WRITE_TEST_MAIN_FLIP); break; //отправляем дополнительные натройки
              case FAST_SECS_MODE: busWriteTwiRegByte(fastSettings.secsMode, BUS_WRITE_FAST_SET, 1); break; //отправляем дополнительные натройки
              case FAST_DOT_MODE: busWriteTwiRegByte(fastSettings.dotMode, BUS_WRITE_FAST_SET, 2); break; //отправляем дополнительные натройки
              case FAST_BACKL_MODE: busWriteTwiRegByte(fastSettings.backlMode, BUS_WRITE_FAST_SET, 3); break; //отправляем дополнительные натройки
              case FAST_BACKL_COLOR: busWriteTwiRegByte(fastSettings.backlColor, BUS_WRITE_FAST_SET, 4); break; //отправляем дополнительные натройки
            }
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;

        case READ_MAIN_SET:
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_MAIN_SET)) { //начинаем передачу
            mainSettings.indiBrightNight = twi_read_byte(TWI_ACK); //принимаем основные настройки
            mainSettings.indiBrightDay = twi_read_byte(TWI_ACK);
            mainSettings.backlBrightNight = twi_read_byte(TWI_ACK);
            mainSettings.backlBrightDay = twi_read_byte(TWI_ACK);
            mainSettings.dotBrightNight = twi_read_byte(TWI_ACK);
            mainSettings.dotBrightDay = twi_read_byte(TWI_ACK);
            mainSettings.timeBrightStart = twi_read_byte(TWI_ACK);
            mainSettings.timeBrightEnd = twi_read_byte(TWI_ACK);
            mainSettings.timeHourStart = twi_read_byte(TWI_ACK);
            mainSettings.timeHourEnd = twi_read_byte(TWI_ACK);
            mainSettings.timeSleepNight = twi_read_byte(TWI_ACK);
            mainSettings.timeSleepDay = twi_read_byte(TWI_ACK);
            mainSettings.timeFormat = twi_read_byte(TWI_ACK);
            mainSettings.knockSound = twi_read_byte(TWI_ACK);
            mainSettings.hourSound = twi_read_byte(TWI_ACK);
            mainSettings.volumeSound = twi_read_byte(TWI_ACK);
            mainSettings.voiceSound = twi_read_byte(TWI_ACK);
            mainSettings.tempCorrect = twi_read_byte(TWI_ACK);
            mainSettings.glitchMode = twi_read_byte(TWI_ACK);
            mainSettings.autoShowTime = twi_read_byte(TWI_ACK);
            if (!mainSettings.autoShowTime) mainSettings.autoShowTime = 0x81;
            mainSettings.autoShowFlip = twi_read_byte(TWI_ACK);
            mainSettings.burnMode = twi_read_byte(TWI_ACK);
            mainSettings.burnTime = twi_read_byte(TWI_NACK);
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_MAIN_SET:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            switch (busReadBufferArg()) {
              case MAIN_INDI_BRIGHT_N: busWriteTwiRegByte(mainSettings.indiBrightNight, BUS_WRITE_MAIN_SET, 0); break; //отправляем дополнительные натройки
              case MAIN_INDI_BRIGHT_D: busWriteTwiRegByte(mainSettings.indiBrightDay, BUS_WRITE_MAIN_SET, 1); break; //отправляем дополнительные натройки
              case MAIN_BACKL_BRIGHT_N: busWriteTwiRegByte(mainSettings.backlBrightNight, BUS_WRITE_MAIN_SET, 2); break; //отправляем дополнительные натройки
              case MAIN_BACKL_BRIGHT_D: busWriteTwiRegByte(mainSettings.backlBrightDay, BUS_WRITE_MAIN_SET, 3); break; //отправляем дополнительные натройки
              case MAIN_DOT_BRIGHT_N: busWriteTwiRegByte(mainSettings.dotBrightNight, BUS_WRITE_MAIN_SET, 4); break; //отправляем дополнительные натройки
              case MAIN_DOT_BRIGHT_D: busWriteTwiRegByte(mainSettings.dotBrightDay, BUS_WRITE_MAIN_SET, 5); break; //отправляем дополнительные натройки
              case MAIN_TIME_BRIGHT_S: busWriteTwiRegByte(mainSettings.timeBrightStart, BUS_WRITE_MAIN_SET, 6); break; //отправляем дополнительные натройки
              case MAIN_TIME_BRIGHT_E: busWriteTwiRegByte(mainSettings.timeBrightEnd, BUS_WRITE_MAIN_SET, 7); break; //отправляем дополнительные натройки
              case MAIN_TIME_HOUR_S: busWriteTwiRegByte(mainSettings.timeHourStart, BUS_WRITE_MAIN_SET, 8); break; //отправляем дополнительные натройки
              case MAIN_TIME_HOUR_E: busWriteTwiRegByte(mainSettings.timeHourEnd, BUS_WRITE_MAIN_SET, 9); break; //отправляем дополнительные натройки
              case MAIN_TIME_SLEEP_N: busWriteTwiRegByte(mainSettings.timeSleepNight, BUS_WRITE_MAIN_SET, 10); break; //отправляем дополнительные натройки
              case MAIN_TIME_SLEEP_D: busWriteTwiRegByte(mainSettings.timeSleepDay, BUS_WRITE_MAIN_SET, 11); break; //отправляем дополнительные натройки
              case MAIN_TIME_FORMAT: busWriteTwiRegByte(mainSettings.timeFormat, BUS_WRITE_MAIN_SET, 12); break; //отправляем дополнительные натройки
              case MAIN_KNOCK_SOUND: busWriteTwiRegByte(mainSettings.knockSound, BUS_WRITE_MAIN_SET, 13); break; //отправляем дополнительные натройки
              case MAIN_HOUR_SOUND: busWriteTwiRegByte(mainSettings.hourSound, BUS_WRITE_MAIN_SET, 14); break; //отправляем дополнительные натройки
              case MAIN_VOLUME_SOUND: busWriteTwiRegByte(mainSettings.volumeSound, BUS_WRITE_MAIN_SET, 15); break; //отправляем дополнительные натройки
              case MAIN_VOICE_SOUND: busWriteTwiRegByte(mainSettings.voiceSound, BUS_WRITE_MAIN_SET, 16); break; //отправляем дополнительные натройки
              case MAIN_TEMP_CORRECT: busWriteTwiRegByte(mainSettings.tempCorrect, BUS_WRITE_MAIN_SET, 17); break; //отправляем дополнительные натройки
              case MAIN_GLITCH_MODE: busWriteTwiRegByte(mainSettings.glitchMode, BUS_WRITE_MAIN_SET, 18); break; //отправляем дополнительные натройки
              case MAIN_AUTO_SHOW_TIME: busWriteTwiRegByte((mainSettings.autoShowTime & 0x80) ? 0 : mainSettings.autoShowTime, BUS_WRITE_MAIN_SET, 19); busWriteBuffer(SET_SHOW_TIME); break; //отправляем дополнительные натройки
              case MAIN_AUTO_SHOW_FLIP: busWriteTwiRegByte(mainSettings.autoShowFlip, BUS_WRITE_MAIN_SET, 20); break; //отправляем дополнительные натройки
              case MAIN_BURN_MODE: busWriteTwiRegByte(mainSettings.burnMode, BUS_WRITE_MAIN_SET, 21); break; //отправляем дополнительные натройки
              case MAIN_BURN_TIME: busWriteTwiRegByte(mainSettings.burnTime, BUS_WRITE_MAIN_SET, 22); break; //отправляем дополнительные натройки
            }
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;

        case READ_ALARM_ALL: {
            if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_ALARM_NUM)) { //начинаем передачу
              uint8_t tempAll = twi_read_byte(TWI_NACK);
              if (tempAll > MAX_ALARMS) tempAll = MAX_ALARMS;
              if (alarm.all != tempAll) alarm.now = alarm.set = 0;
              alarm.reload = 1;
              alarm.num = 0;
              alarm.all = tempAll;
              if (!twi_error()) { //если передача была успешной
                busShiftBuffer(); //сместили буфер команд
                busWriteBuffer(READ_ALARM_DATA);
              }
            }
          }
          break;
        case READ_ALARM_NUM: {
            if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_ALARM_NUM)) { //начинаем передачу
              uint8_t tempAll = twi_read_byte(TWI_NACK);
              if (alarm.all != tempAll) alarm.now = 0;
              alarm.all = tempAll;
              if (!twi_error()) { //если передача была успешной
                busShiftBuffer(); //сместили буфер команд
              }
            }
          }
          break;
        case READ_ALARM_DATA: {
            if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_SELECT_ALARM, alarm.num)) { //начинаем передачу
              alarm_data[alarm.num][ALARM_DATA_HOUR] = twi_read_byte(TWI_ACK);
              alarm_data[alarm.num][ALARM_DATA_MINS] = twi_read_byte(TWI_ACK);
              alarm_data[alarm.num][ALARM_DATA_MODE] = twi_read_byte(TWI_ACK);
              alarm_data[alarm.num][ALARM_DATA_DAYS] = twi_read_byte(TWI_ACK);
              uint8_t tempSound = twi_read_byte(TWI_ACK);
              alarm_data[alarm.num][ALARM_DATA_VOLUME] = twi_read_byte(TWI_ACK);
              alarm_data[alarm.num][ALARM_DATA_RADIO] = twi_read_byte(TWI_NACK);
              if (!alarm_data[alarm.num][ALARM_DATA_RADIO]) alarm_data[alarm.num][ALARM_DATA_SOUND] = tempSound;
              else alarm_data[alarm.num][ALARM_DATA_STATION] = tempSound;
              if (!twi_error()) { //если передача была успешной
                if (++alarm.num >= alarm.all) {
                  alarm.reload = 2;
                  busShiftBuffer(); //сместили буфер команд
                }
              }
            }
          }
          break;
        case READ_SELECT_ALARM: {
            if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_SELECT_ALARM, alarm.now)) { //начинаем передачу
              alarm_data[alarm.now][ALARM_DATA_HOUR] = twi_read_byte(TWI_ACK);
              alarm_data[alarm.now][ALARM_DATA_MINS] = twi_read_byte(TWI_ACK);
              alarm_data[alarm.now][ALARM_DATA_MODE] = twi_read_byte(TWI_ACK);
              alarm_data[alarm.now][ALARM_DATA_DAYS] = twi_read_byte(TWI_ACK);
              uint8_t tempSound = twi_read_byte(TWI_ACK);
              alarm_data[alarm.now][ALARM_DATA_VOLUME] = twi_read_byte(TWI_ACK);
              alarm_data[alarm.now][ALARM_DATA_RADIO] = twi_read_byte(TWI_NACK);
              if (!alarm_data[alarm.now][ALARM_DATA_RADIO]) alarm_data[alarm.now][ALARM_DATA_SOUND] = tempSound;
              else alarm_data[alarm.now][ALARM_DATA_STATION] = tempSound;
              if (!twi_error()) { //если передача была успешной
                busShiftBuffer(); //сместили буфер команд
              }
            }
          }
          break;
        case WRITE_SELECT_ALARM:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            switch (busReadBufferArg()) {
              case ALARM_TIME: busWriteTwiRegByte(alarm.now, BUS_WRITE_SELECT_ALARM, 0); twi_write_byte(alarm_data[alarm.now][ALARM_DATA_HOUR]); twi_write_byte(alarm_data[alarm.now][ALARM_DATA_MINS]); break; //время будильника
              case ALARM_MODE: busWriteTwiRegByte(alarm.now, BUS_WRITE_SELECT_ALARM, 2); twi_write_byte(alarm_data[alarm.now][ALARM_DATA_MODE]); break; //режим будильника
              case ALARM_DAYS: busWriteTwiRegByte(alarm.now, BUS_WRITE_SELECT_ALARM, 3); twi_write_byte(alarm_data[alarm.now][ALARM_DATA_DAYS]); break; //день недели будильника
              case ALARM_SOUND: busWriteTwiRegByte(alarm.now, BUS_WRITE_SELECT_ALARM, 4); twi_write_byte((!alarm_data[alarm.now][ALARM_DATA_RADIO]) ? alarm_data[alarm.now][ALARM_DATA_SOUND] : alarm_data[alarm.now][ALARM_DATA_STATION]); break; //мелодия будильника
              case ALARM_VOLUME: busWriteTwiRegByte(alarm.now, BUS_WRITE_SELECT_ALARM, 5); twi_write_byte(alarm_data[alarm.now][ALARM_DATA_VOLUME]); break; //громкость будильника
              case ALARM_RADIO: busWriteTwiRegByte(alarm.now, BUS_WRITE_SELECT_ALARM, 6); twi_write_byte(alarm_data[alarm.now][ALARM_DATA_RADIO]); break; //радиобудильник
            }
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case DEL_ALARM:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_DEL_ALARM); //регистр команды
            twi_write_byte(busReadBufferArg()); //регистр команды
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case NEW_ALARM:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_NEW_ALARM); //регистр команды
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;

        case READ_RADIO_SET:
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_RADIO_SET)) { //начинаем передачу
            radioSettings.volume = twi_read_byte(TWI_ACK);
            twi_read_byte(TWI_ACK);
            radioSettings.stationsFreq = twi_read_byte(TWI_ACK) | ((uint16_t)twi_read_byte(TWI_ACK) << 8);
            for (uint8_t i = 0; i < 10; i++) {
              radioSettings.stationsSave[i] = twi_read_byte(TWI_ACK) | ((uint16_t)twi_read_byte((i < 9) ? TWI_ACK : TWI_NACK) << 8); //отправляем натройки радиостанций
            }
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case READ_RADIO_STA:
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_SELECT_BYTE, 0x04, BUS_READ_RADIO_SET)) { //начинаем передачу
            for (uint8_t i = 0; i < 10; i++) {
              radioSettings.stationsSave[i] = twi_read_byte(TWI_ACK) | ((uint16_t)twi_read_byte((i < 9) ? TWI_ACK : TWI_NACK) << 8); //отправляем натройки радиостанций
            }
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_RADIO_STA:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            busWriteTwiRegWord(radioSettings.stationsSave[busReadBufferArg()], BUS_WRITE_RADIO_STA, busReadBufferArg()); //отправляем дополнительные натройки
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case READ_RADIO_VOL:
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_RADIO_SET)) { //начинаем передачу
            radioSettings.volume = twi_read_byte(TWI_NACK);
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_RADIO_VOL:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_WRITE_RADIO_VOL); //регистр команды
            twi_write_byte(radioSettings.volume); //регистр команды
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case READ_RADIO_FREQ:
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_SELECT_BYTE, 0x02, BUS_READ_RADIO_SET)) { //начинаем передачу
            radioSettings.stationsFreq = twi_read_byte(TWI_ACK) | ((uint16_t)twi_read_byte(TWI_NACK) << 8);
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_RADIO_FREQ:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_WRITE_RADIO_FREQ); //регистр команды
            twi_write_byte(radioSettings.stationsFreq & 0xFF); //регистр команды
            twi_write_byte((radioSettings.stationsFreq >> 8) & 0xFF);
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_RADIO_MODE:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_WRITE_RADIO_MODE); //регистр команды
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_RADIO_POWER:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_WRITE_RADIO_POWER); //регистр команды
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case READ_RADIO_POWER:
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_RADIO_POWER)) { //начинаем передачу
            radioSettings.powerState = twi_read_byte(TWI_NACK);
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case RADIO_FREQ_UP:
          busShiftBuffer(); //сместили буфер команд
          if (radioSettings.stationsFreq < 1080) {
            radioSettings.stationsFreq++;
            busSetCommand(WRITE_RADIO_FREQ);
          }
          else radioSettings.stationsFreq = 1080;
          break;
        case RADIO_FREQ_DOWN:
          busShiftBuffer(); //сместили буфер команд
          if (radioSettings.stationsFreq > 870) {
            radioSettings.stationsFreq--;
            busSetCommand(WRITE_RADIO_FREQ);
          }
          else radioSettings.stationsFreq = 870;
          break;
        case RADIO_SEEK_UP:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_SEEK_RADIO_UP); //регистр команды
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case RADIO_SEEK_DOWN:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_SEEK_RADIO_DOWN); //регистр команды
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;

        case READ_SENS_DATA:
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_TEMP)) { //начинаем передачу
            sens.temp[SENS_CLOCK] = twi_read_byte(TWI_ACK) | ((uint16_t)twi_read_byte(TWI_ACK) << 8);
            sens.press[SENS_CLOCK] = twi_read_byte(TWI_ACK) | ((uint16_t)twi_read_byte(TWI_ACK) << 8);
            sens.hum[SENS_CLOCK] = twi_read_byte(TWI_NACK);
            if (!twi_error()) { //если передача была успешной
              if (sens.temp[SENS_CLOCK] != 0x7FFF) sens.status |= SENS_EXT;
              sens.update |= SENS_EXT;
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case READ_SENS_INFO:
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_SELECT_BYTE, 0x05, BUS_READ_TEMP)) { //начинаем передачу
            sens.type = twi_read_byte(TWI_ACK); //тип датчика температуры
            sens.init = twi_read_byte(TWI_ACK); //флаг инициализации порта
            sens.err = twi_read_byte(TWI_NACK); //ошибка сенсора
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_CHECK_SENS:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_CHECK_TEMP); //регистр команды
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;

        case READ_EXTENDED_SET:
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_EXTENDED_SET)) { //начинаем передачу
            for (uint8_t i = 0; i < 5; i++) {
              extendedSettings.autoShowModes[i] = twi_read_byte(TWI_ACK);
            }
            for (uint8_t i = 0; i < 5; i++) {
              extendedSettings.autoShowTimes[i] = twi_read_byte(TWI_ACK);
            }
            extendedSettings.alarmTime = twi_read_byte(TWI_ACK);
            extendedSettings.alarmWaitTime = twi_read_byte(TWI_ACK);
            extendedSettings.alarmSoundTime = twi_read_byte(TWI_ACK);
            extendedSettings.alarmDotOn = twi_read_byte(TWI_ACK);
            extendedSettings.alarmDotWait = twi_read_byte(TWI_ACK);
            extendedSettings.tempCorrectSensor = twi_read_byte(TWI_ACK);
            extendedSettings.tempMainSensor = twi_read_byte(TWI_ACK);
            extendedSettings.tempHourSensor = twi_read_byte(TWI_NACK);
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_EXTENDED_SHOW_MODE:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            busWriteTwiRegByte(extendedSettings.autoShowModes[busReadBufferArg()], BUS_WRITE_EXTENDED_SET, busReadBufferArg());
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_EXTENDED_SHOW_TIME:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            busWriteTwiRegByte(extendedSettings.autoShowTimes[busReadBufferArg()], BUS_WRITE_EXTENDED_SET, busReadBufferArg() + 5);
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_EXTENDED_SENSOR_SET:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            switch (busReadBufferArg()) {
              case EXT_SENSOR_CORRECT: busWriteTwiRegByte(extendedSettings.tempCorrectSensor, BUS_WRITE_EXTENDED_SET, 15); break; //отправляем дополнительные натройки
              case EXT_SENSOR_MAIN: busWriteTwiRegByte(extendedSettings.tempMainSensor, BUS_WRITE_EXTENDED_SET, 16); break; //отправляем дополнительные натройки
              case EXT_SENSOR_HOUR: busWriteTwiRegByte(extendedSettings.tempHourSensor, BUS_WRITE_EXTENDED_SET, 17); break; //отправляем дополнительные натройки
            }
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_EXTENDED_ALARM:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            switch (busReadBufferArg()) {
              case EXT_ALARM_TIMEOUT: busWriteTwiRegByte(extendedSettings.alarmTime, BUS_WRITE_EXTENDED_SET, 10); break; //отправляем дополнительные натройки
              case EXT_ALARM_WAIT: busWriteTwiRegByte(extendedSettings.alarmWaitTime, BUS_WRITE_EXTENDED_SET, 11); break; //отправляем дополнительные натройки
              case EXT_ALARM_TIMEOUT_SOUND: busWriteTwiRegByte(extendedSettings.alarmSoundTime, BUS_WRITE_EXTENDED_SET, 12); break; //отправляем дополнительные натройки
              case EXT_ALARM_DOT_ON: busWriteTwiRegByte(extendedSettings.alarmDotOn, BUS_WRITE_EXTENDED_SET, 13); busWriteBuffer(SET_UPDATE); break; //отправляем дополнительные натройки
              case EXT_ALARM_DOT_WAIT: busWriteTwiRegByte(extendedSettings.alarmDotWait, BUS_WRITE_EXTENDED_SET, 14); busWriteBuffer(SET_UPDATE); break; //отправляем дополнительные натройки
            }
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;

        case SET_SHOW_TIME:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_SET_SHOW_TIME); //регистр команды
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case SET_BURN_TIME:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_SET_BURN_TIME); //регистр команды
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case SET_UPDATE:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_SET_UPDATE); //регистр команды
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;

        case READ_TIMER_STATE:
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_TIMER_SET)) { //начинаем передачу
            timer.mode = twi_read_byte(TWI_ACK); //принимаем данные
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_TIMER_STATE:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_WRITE_TIMER_SET); //регистр команды
            twi_write_byte(timer.mode);
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;

        case READ_TIMER_TIME:
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_TIMER_SET)) { //начинаем передачу
            timer.mode = twi_read_byte(TWI_ACK); //принимаем данные
            timer.count = twi_read_byte(TWI_ACK) | ((uint16_t)twi_read_byte(TWI_NACK) << 8);
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_TIMER_TIME:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_WRITE_TIMER_SET); //регистр команды
            twi_write_byte(timer.mode);
            twi_write_byte(timer.count & 0xFF);
            twi_write_byte((timer.count >> 8) & 0xFF);
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;

        case READ_TIMER_SET:
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_SELECT_BYTE, 0x03, BUS_READ_TIMER_SET)) { //начинаем передачу
            timer.time = twi_read_byte(TWI_ACK) | ((uint16_t)twi_read_byte(TWI_NACK) << 8);
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_TIMER_SET:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_SELECT_BYTE); //регистр команды
            twi_write_byte(0x03);
            twi_write_byte(BUS_WRITE_TIMER_SET); //регистр команды
            twi_write_byte(timer.time & 0xFF);
            twi_write_byte((timer.time >> 8) & 0xFF);
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_TIMER_MODE:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_WRITE_TIMER_MODE); //регистр команды
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;

        case WRITE_ALARM_DISABLE:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_ALARM_DISABLE); //регистр команды
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_CHANGE_BRIGHT:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_CHANGE_BRIGHT); //регистр команды
            twi_write_byte((device.light > 2) ? 0 : device.light);
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;

        case WRITE_STOP_SOUND:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_STOP_SOUND); //регистр команды
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_TEST_MAIN_VOL:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_TEST_SOUND); //регистр команды
            twi_write_byte(mainSettings.volumeSound); //громкость
            twi_write_byte(22); //номер трека
            twi_write_byte(5); //номер папки
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_TEST_ALARM_VOL:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_TEST_SOUND); //регистр команды
            twi_write_byte((alarm_data[alarm.now][ALARM_DATA_VOLUME]) ? alarm_data[alarm.now][ALARM_DATA_VOLUME] : deviceInformation[ALARM_AUTO_VOL_MAX]); //громкость
            twi_write_byte(22); //номер трека
            twi_write_byte(5); //номер папки
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_TEST_ALARM_SOUND:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_TEST_SOUND); //регистр команды
            twi_write_byte((alarm_data[alarm.now][ALARM_DATA_VOLUME]) ? alarm_data[alarm.now][ALARM_DATA_VOLUME] : deviceInformation[ALARM_AUTO_VOL_MAX]); //громкость
            twi_write_byte(alarm_data[alarm.now][ALARM_DATA_SOUND] + ((deviceInformation[PLAYER_TYPE]) ? 1 : 0)); //номер трека
            twi_write_byte(1); //номер папки
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_TEST_MAIN_VOICE:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_TEST_SOUND); //регистр команды
            twi_write_byte(mainSettings.volumeSound); //громкость
            twi_write_byte(18); //номер трека
            twi_write_byte(5); //номер папки
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case WRITE_TEST_MAIN_FLIP:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_TEST_FLIP); //регистр команды
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;

        case READ_STATUS:
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_STATUS)) { //начинаем передачу
            device.status = twi_read_byte(TWI_NACK);
            if ((boolean)(device.status & (0x01 << STATUS_MAX_DATA)) != alarm.status) {
              alarm.status = !alarm.status;
              alarm.reload = 2;
            }
            device.status &= ~(0x01 << STATUS_MAX_DATA);
            if (!twi_error()) { //если передача была успешной
              clockState = -1; //установили флаг ответа от часов
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case READ_DEVICE:
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_DEVICE)) { //начинаем передачу
            deviceInformation[FIRMWARE_VERSION_1] = twi_read_byte(TWI_ACK);
            deviceInformation[FIRMWARE_VERSION_2] = twi_read_byte(TWI_ACK);
            deviceInformation[FIRMWARE_VERSION_3] = twi_read_byte(TWI_ACK);
            deviceInformation[HARDWARE_VERSION] = twi_read_byte(TWI_ACK);
            deviceInformation[SENS_TEMP] = twi_read_byte(TWI_ACK);
            deviceInformation[BTN_EASY_MAIN_MODE] = twi_read_byte(TWI_ACK);
            deviceInformation[LAMP_NUM] = twi_read_byte(TWI_ACK);
            deviceInformation[BACKL_TYPE] = twi_read_byte(TWI_ACK);
            deviceInformation[NEON_DOT] = twi_read_byte(TWI_ACK);
            deviceInformation[DOTS_PORT_ENABLE] = twi_read_byte(TWI_ACK);
            deviceInformation[DOTS_NUM] = twi_read_byte(TWI_ACK);
            deviceInformation[DOTS_TYPE] = twi_read_byte(TWI_ACK);
            deviceInformation[LIGHT_SENS_ENABLE] = twi_read_byte(TWI_ACK);
            deviceInformation[EXT_BTN_ENABLE] = twi_read_byte(TWI_ACK);
            deviceInformation[DS3231_ENABLE] = twi_read_byte(TWI_ACK);
            deviceInformation[TIMER_ENABLE] = twi_read_byte(TWI_ACK);
            deviceInformation[RADIO_ENABLE] = twi_read_byte(TWI_ACK);
            deviceInformation[ALARM_TYPE] = twi_read_byte(TWI_ACK);
            deviceInformation[PLAYER_TYPE] = twi_read_byte(TWI_ACK);
            deviceInformation[PLAYER_MAX_SOUND] = twi_read_byte(TWI_ACK);
            deviceInformation[PLAYER_MAX_VOICE] = twi_read_byte(TWI_ACK);
            deviceInformation[ALARM_AUTO_VOL_MAX] = twi_read_byte(TWI_NACK);
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;
        case READ_FAILURE:
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_FAILURE)) { //начинаем передачу
            device.failure = twi_read_byte(TWI_ACK) | ((uint16_t)twi_read_byte(TWI_NACK) << 8);
            if (!twi_error()) { //если передача была успешной
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;

        case WRITE_SENS_1_DATA: {
            if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
              uint8_t _sens = busReadBufferArg();
              if (_sens < SENS_MAX_DATA) {
                twi_write_byte(BUS_WRITE_SENS_DATA); //регистр команды
                twi_write_byte(sens.temp[_sens] & 0xFF);
                twi_write_byte((sens.temp[_sens] >> 8) & 0xFF);
                twi_write_byte(sens.press[_sens] & 0xFF);
                twi_write_byte((sens.press[_sens] >> 8) & 0xFF);
                twi_write_byte(sens.hum[_sens]);
              }
              if (!twi_error()) { //если передача была успешной
                busShiftBuffer(); //сместили буфер команд
                busShiftBuffer(); //сместили буфер команд
              }
            }
          }
          break;

        case WRITE_SENS_2_DATA: {
            if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
              uint8_t _sens = busReadBufferArg();
              if (_sens < SENS_MAX_DATA) {
                twi_write_byte(BUS_WRITE_MAIN_SENS_DATA); //регистр команды
                twi_write_byte(sens.temp[_sens] & 0xFF);
                twi_write_byte((sens.temp[_sens] >> 8) & 0xFF);
                twi_write_byte(sens.press[_sens] & 0xFF);
                twi_write_byte((sens.press[_sens] >> 8) & 0xFF);
                twi_write_byte(sens.hum[_sens]);
              }
              if (!twi_error()) { //если передача была успешной
                busShiftBuffer(); //сместили буфер команд
                busShiftBuffer(); //сместили буфер команд
              }
            }
          }
          break;

        case CONTROL_SYSTEM:
          if (busReadBufferArg() == SYSTEM_REBOOT) {
            twi_write_stop(); //остановили шину
            if (!twi_running()) ESP.reset(); //перезагрузка
            else bus.status = BUS_STATUS_REBOOT_FAIL; //сбросили статус
          }
          else bus.status = 0; //сбросили статус
          busShiftBuffer(); //сместили буфер команд
          busShiftBuffer(); //сместили буфер команд
          break;
        case CONTROL_DEVICE:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_CONTROL_DEVICE); //регистр команды
            twi_write_byte(busReadBufferArg());
            if (!twi_error()) { //если передача была успешной
              twi_write_stop(); //остановили шину
              busShiftBuffer(); //сместили буфер команд
              busShiftBuffer(); //сместили буфер команд
              if (!twi_running()) ESP.reset(); //перезагрузка
              else bus.status = BUS_STATUS_REBOOT_FAIL; //сбросили статус
            }
          }
          break;
        case UPDATE_FIRMWARE:
          if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
            twi_write_byte(BUS_CONTROL_DEVICE); //регистр команды
            twi_write_byte(DEVICE_UPDATE);
            if (!twi_error()) { //если передача была успешной
              updaterStart(); //запуск обновления
              busShiftBuffer(); //сместили буфер команд
            }
          }
          break;

        case WRITE_RTC_INIT:
          busShiftBuffer(); //сместили буфер команд
          switch (rtcInitTime()) {
            case 0: busSetCommand(WRITE_TIME_DATE); break; //отправляем время в часы
            case 1: busSetCommand(WRITE_RTC_INIT); break; //инициализируем модуль RTC
          }
          break;
        case WRITE_RTC_TIME:
          busShiftBuffer(); //сместили буфер команд
          if (rtcSendTime()) busSetCommand(WRITE_RTC_TIME); //отправляем время в RTC
          break;
        case READ_RTC_TIME:
          busShiftBuffer(); //сместили буфер команд
          switch (rtcGetTime(RTC_CHECK_OSF)) {
            case 0: busSetCommand(WRITE_TIME_DATE); break; //отправляем время в часы
            case 1: busSetCommand(READ_RTC_TIME); break; //считываем время из RTC
          }
          break;
        case WRITE_RTC_AGING:
          busShiftBuffer(); //сместили буфер команд
          if (rtcWriteAging()) busSetCommand(WRITE_RTC_AGING); //запись коррекции хода в RTC
          break;
        case READ_RTC_AGING:
          busShiftBuffer(); //сместили буфер команд
          if (rtcReadAging()) busSetCommand(READ_RTC_AGING); //чтение коррекции хода из RTC
          break;

        case CHECK_INTERNAL_AHT:
          busShiftBuffer(); //сместили буфер команд
          if (readTempAHT() == 1) busSetCommand(CHECK_INTERNAL_AHT); //чтение температуры/влажности
          else sens.update |= SENS_AHT;
          break;
        case CHECK_INTERNAL_SHT:
          busShiftBuffer(); //сместили буфер команд
          if (readTempSHT() == 1) busSetCommand(CHECK_INTERNAL_SHT); //чтение температуры/влажности
          else sens.update |= SENS_SHT;
          break;
        case CHECK_INTERNAL_BME:
          busShiftBuffer(); //сместили буфер команд
          if (readTempBME() == 1) busSetCommand(CHECK_INTERNAL_BME); //чтение температуры/давления/влажности
          else sens.update |= SENS_BME;
          break;
        default: busShiftBuffer(); break; //сместили буфер команд
      }
    }
    if (twi_running()) { //если шина не остановлена
      twi_write_stop(); //остановили шину
    }
    busTimerSetInterval(35); //установили интервал выполнения следующей команды
  }
}
