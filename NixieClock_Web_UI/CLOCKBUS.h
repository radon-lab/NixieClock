//Информация о интефейсе
#define HW_VERSION 0x04 //версия прошивки для интерфейса wire

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
#define BUS_SET_MAIN_DOT 0x1D
#define BUS_SET_ALARM_DOT 0x1E

#define BUS_TEST_FLIP 0xFB
#define BUS_TEST_SOUND 0xFC

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
  int8_t tempCorrect; //коррекция температуры
  boolean glitchMode; //режим глюков
  uint8_t autoShowTime; //интервал времени автопоказа
  uint8_t autoShowFlip; //режим анимации автопоказа
  uint8_t burnMode; //режим антиотравления индикаторов
  uint8_t secsMode; //режим анимации секунд индикаторов
} mainSettings;

struct Settings_2 {
  uint8_t flipMode; //режим анимации
  uint8_t backlMode; //режим подсветки
  uint8_t backlColor; //цвет подсветки
  uint8_t dotMode; //режим точек
} fastSettings;

struct Settings_3 { //расширенные настройки
  uint8_t autoShowModes[5];
  uint8_t autoShowTimes[5];
  uint8_t burnTime;
  uint8_t alarmTime;
  uint8_t alarmWaitTime;
  uint8_t alarmSoundTime;
  uint8_t alarmDotOn;
  uint8_t alarmDotWait;
} extendedSettings;

struct Settings_4 { //настройки радио
  boolean powerState; //состояние питания
  uint8_t volume; //текущая громкость
  uint16_t stationsFreq; //текущая частота
  uint16_t stationsSave[10]; //память радиостанций
} radioSettings;

//----------------Температура--------------
struct sensorData {
  uint16_t temp; //температура
  uint16_t press; //давление
  uint8_t hum; //влажность
  uint8_t type; //тип датчика температуры
  boolean init; //флаг инициализации порта
  boolean err; //ошибка сенсора
} sens;

//----------------Будильники--------------
struct alarmData {
  uint8_t set; //настройка текущего будильника
  uint8_t all; //всего будильников
  uint8_t now; //текущий будильник
  uint8_t num; //текущий будильник
  uint8_t volume; //текущая громкость
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
  STATUS_UPDATE_TIME_SET,
  STATUS_UPDATE_MAIN_SET,
  STATUS_UPDATE_FAST_SET,
  STATUS_UPDATE_RADIO_SET,
  STATUS_UPDATE_EXTENDED_SET,
  STATUS_UPDATE_ALARM_SET,
  STATUS_UPDATE_SENS_DATA,
  STATUS_MAX_DATA
};
uint8_t deviceStatus; //состояние часов

enum {
  FIRMWARE_VERSION_1,
  FIRMWARE_VERSION_2,
  FIRMWARE_VERSION_3,
  HARDWARE_VERSION,
  SENS_TEMP,
  LAMP_NUM,
  BACKL_TYPE,
  NEON_DOT,
  DOTS_PORT_ENABLE,
  DOTS_NUM,
  DOTS_TYPE,
  RADIO_ENABLE,
  ALARM_TYPE,
  PLAYER_TYPE,
  PLAYER_MAX_SOUND,
  PLAYER_MAX_VOL,
  INFORMATION_MAX
};
uint8_t deviceInformation[INFORMATION_MAX]; //информация о часах

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
  MAIN_TEMP_CORRECT,
  MAIN_GLITCH_MODE,
  MAIN_AUTO_SHOW_TIME,
  MAIN_AUTO_SHOW_FLIP,
  MAIN_BURN_MODE,
  MAIN_SECS_MODE
};

enum {
  FAST_FLIP_MODE,
  FAST_BACKL_MODE,
  FAST_BACKL_COLOR,
  FAST_DOT_MODE
};

enum {
  EXT_ALARM_TIMEOUT,
  EXT_ALARM_WAIT,
  EXT_ALARM_TIMEOUT_SOUND,
  EXT_ALARM_DOT_ON,
  EXT_ALARM_DOT_WAIT
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
  READ_TIME_DATE,
  WRITE_TIME,
  WRITE_DATE,

  READ_FAST_SET,
  WRITE_FAST_SET,

  READ_MAIN_SET,
  WRITE_MAIN_SET,

  READ_ALARM,
  READ_ALARM_ALL,
  READ_ALARM_NUM,
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
  WRITE_EXTENDED_BURN_TIME,
  WRITE_EXTENDED_ALARM,

  SET_SHOW_TIME,
  SET_BURN_TIME,
  SET_ALARM_DOT,
  SET_MAIN_DOT,

  WRITE_TEST_MAIN_VOL,
  WRITE_TEST_ALARM_VOL,
  WRITE_TEST_ALARM_SOUND,
  WRITE_TEST_MAIN_FLIP,

  READ_STATUS,
  READ_DEVICE
};

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

void climateUpdate(void);
void busWriteTwiRegByte(uint8_t data, uint8_t command, uint8_t pos = 0x00);
void busWriteTwiRegWord(uint16_t data, uint8_t command, uint8_t pos = 0x00);

//--------------------------------------------------------------------
void busWriteTwiRegByte(uint8_t data, uint8_t command, uint8_t pos) {
  if (pos) {
    twi_write_byte(BUS_SELECT_BYTE);
    twi_write_byte(pos);
  }
  twi_write_byte(command); //регистр времени
  twi_write_byte(data); //отправляем дополнительные натройки
}
//--------------------------------------------------------------------
void busWriteTwiRegWord(uint16_t data, uint8_t command, uint8_t pos) {
  if (pos) {
    twi_write_byte(BUS_SELECT_BYTE);
    twi_write_byte(pos * 2);
  }
  twi_write_byte(command); //регистр времени
  twi_write_byte(data & 0xFF); //отправляем дополнительные натройки
  twi_write_byte((data >> 8) & 0xFF); //отправляем дополнительные натройки
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
void busShiftBuffer(void) {
  if (busStatusBuffer()) bus.bufferStart++;
}
//--------------------------------------------------------------------
void busSetComand(uint8_t cmd) {
  if (!busStatusBuffer() || (busFreeBuffer() && (bus.bufferLastCommand != cmd))) {
    bus.bufferLastCommand = cmd;
    busWriteBuffer(cmd);
  }
}
//--------------------------------------------------------------------
void busSetComand(uint8_t cmd, uint8_t arg) {
  if (!busStatusBuffer() || ((busFreeBuffer() >= 2) && ((bus.bufferLastCommand != cmd) || (bus.bufferLastArgument != arg)))) {
    bus.bufferLastCommand = cmd;
    bus.bufferLastArgument = arg;
    busWriteBuffer(cmd);
    busWriteBuffer(arg);
  }
}
//--------------------------------------------------------------------
void busUpdate(void) {
  if (busStatusBuffer() && busTimerCheck()) {
    busTimerSetInterval(30);
    switch (busReadBuffer()) {
      case READ_TIME_DATE:
        if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_TIME)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          mainTime.second = twi_read_byte(TWI_ACK); //принимаем время
          mainTime.minute = twi_read_byte(TWI_ACK);
          mainTime.hour = twi_read_byte(TWI_ACK);
          mainDate.day = twi_read_byte(TWI_ACK);
          mainDate.month = twi_read_byte(TWI_ACK);
          mainDate.year = twi_read_byte(TWI_ACK) | ((uint16_t)twi_read_byte(TWI_NACK) << 8);
          twi_write_stop(); //завершаем передачу
        }
        break;
      case WRITE_TIME:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_WRITE_TIME); //регистр времени
          twi_write_byte(mainTime.second); //отправляем время
          twi_write_byte(mainTime.minute);
          twi_write_byte(mainTime.hour);
          twi_write_stop(); //завершаем передачу
        }
        break;
      case WRITE_DATE:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_SELECT_BYTE);
          twi_write_byte(0x03);
          twi_write_byte(BUS_WRITE_TIME); //регистр времени
          twi_write_byte(mainDate.day); //отправляем дату
          twi_write_byte(mainDate.month);
          twi_write_byte(mainDate.year & 0xFF);
          twi_write_byte((mainDate.year >> 8) & 0xFF);
          twi_write_stop(); //завершаем передачу
        }
        break;

      case READ_FAST_SET:
        if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_FAST_SET)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          fastSettings.flipMode = twi_read_byte(TWI_ACK); //принимаем дополнительные натройки
          fastSettings.backlMode = twi_read_byte(TWI_ACK);
          fastSettings.backlColor = twi_read_byte(TWI_ACK);
          fastSettings.dotMode = twi_read_byte(TWI_NACK);
          twi_write_stop(); //завершаем передачу
        }
        break;
      case WRITE_FAST_SET:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          switch (busReadBuffer()) {
            case FAST_FLIP_MODE: busWriteTwiRegByte(fastSettings.flipMode, BUS_WRITE_FAST_SET, 0); busWriteBuffer(WRITE_TEST_MAIN_FLIP); break; //отправляем дополнительные натройки
            case FAST_BACKL_MODE: busWriteTwiRegByte(fastSettings.backlMode, BUS_WRITE_FAST_SET, 1); break; //отправляем дополнительные натройки
            case FAST_BACKL_COLOR: busWriteTwiRegByte(fastSettings.backlColor, BUS_WRITE_FAST_SET, 2); break; //отправляем дополнительные натройки
            case FAST_DOT_MODE: busWriteTwiRegByte(fastSettings.dotMode, BUS_WRITE_FAST_SET, 3); busWriteBuffer(SET_MAIN_DOT); break; //отправляем дополнительные натройки
          }
          busShiftBuffer(); //сместили буфер команд
          twi_write_stop(); //завершаем передачу
        }
        break;

      case READ_MAIN_SET:
        if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_MAIN_SET)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
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
          mainSettings.tempCorrect = twi_read_byte(TWI_ACK);
          mainSettings.glitchMode = twi_read_byte(TWI_ACK);
          mainSettings.autoShowTime = twi_read_byte(TWI_ACK);
          mainSettings.autoShowFlip = twi_read_byte(TWI_ACK);
          mainSettings.burnMode = twi_read_byte(TWI_ACK);
          mainSettings.secsMode = twi_read_byte(TWI_NACK);
          twi_write_stop(); //завершаем передачу
        }
        break;
      case WRITE_MAIN_SET:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          switch (busReadBuffer()) {
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
            case MAIN_TEMP_CORRECT: busWriteTwiRegByte(mainSettings.tempCorrect, BUS_WRITE_MAIN_SET, 16); break; //отправляем дополнительные натройки
            case MAIN_GLITCH_MODE: busWriteTwiRegByte(mainSettings.glitchMode, BUS_WRITE_MAIN_SET, 17); break; //отправляем дополнительные натройки
            case MAIN_AUTO_SHOW_TIME: busWriteTwiRegByte(mainSettings.autoShowTime, BUS_WRITE_MAIN_SET, 18); busWriteBuffer(SET_SHOW_TIME); break; //отправляем дополнительные натройки
            case MAIN_AUTO_SHOW_FLIP: busWriteTwiRegByte(mainSettings.autoShowFlip, BUS_WRITE_MAIN_SET, 19); break; //отправляем дополнительные натройки
            case MAIN_BURN_MODE: busWriteTwiRegByte(mainSettings.burnMode, BUS_WRITE_MAIN_SET, 20); break; //отправляем дополнительные натройки
            case MAIN_SECS_MODE: busWriteTwiRegByte(mainSettings.secsMode, BUS_WRITE_MAIN_SET, 21); break; //отправляем дополнительные натройки
          }
          busShiftBuffer(); //сместили буфер команд
          twi_write_stop(); //завершаем передачу
        }
        break;

      case READ_ALARM: {
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_SELECT_ALARM, alarm.num)) { //начинаем передачу
            if (++alarm.num >= alarm.all) {
              alarmReload = 2;
              busShiftBuffer(); //сместили буфер команд
            }
            alarm_data[alarm.num][ALARM_DATA_HOUR] = twi_read_byte(TWI_ACK);
            alarm_data[alarm.num][ALARM_DATA_MINS] = twi_read_byte(TWI_ACK);
            alarm_data[alarm.num][ALARM_DATA_MODE] = twi_read_byte(TWI_ACK);
            alarm_data[alarm.num][ALARM_DATA_DAYS] = twi_read_byte(TWI_ACK);
            uint8_t tempSound = twi_read_byte(TWI_ACK);
            uint8_t tempVolume = twi_read_byte(TWI_ACK);
            alarm_data[alarm.num][ALARM_DATA_RADIO] = twi_read_byte(TWI_NACK);
            if (!alarm_data[alarm.num][ALARM_DATA_RADIO]) {
              alarm_data[alarm.num][ALARM_DATA_SOUND] = tempSound;
              alarm_data[alarm.num][ALARM_DATA_VOLUME] = ((100.0 / deviceInformation[PLAYER_MAX_VOL]) * tempVolume);
            }
            else {
              alarm_data[alarm.num][ALARM_DATA_STATION] = tempSound;
              alarm_data[alarm.num][ALARM_DATA_VOLUME] = ((100.0 / 30.0) * tempVolume);
            }
            twi_write_stop(); //завершаем передачу
          }
        }
        break;
      case READ_ALARM_ALL: {
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_ALARM_NUM)) { //начинаем передачу
            busShiftBuffer(); //сместили буфер команд
            uint8_t tempAll = twi_read_byte(TWI_NACK);
            if (alarm.all != tempAll) alarm.now = alarm.set = 0;
            if (tempAll > MAX_ALARMS) tempAll = MAX_ALARMS;
            alarm.num = 0;
            alarm.all = tempAll;
            twi_write_stop(); //завершаем передачу
            busWriteBuffer(READ_ALARM);
          }
        }
        break;
      case READ_ALARM_NUM: {
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_ALARM_NUM)) { //начинаем передачу
            busShiftBuffer(); //сместили буфер команд
            uint8_t tempAll = twi_read_byte(TWI_NACK);
            if (alarm.all != tempAll) alarm.now = 0;
            alarm.all = tempAll;
            twi_write_stop(); //завершаем передачу
          }
        }
        break;
      case READ_SELECT_ALARM: {
          if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_SELECT_ALARM, alarm.now)) { //начинаем передачу
            busShiftBuffer(); //сместили буфер команд
            alarm_data[alarm.now][ALARM_DATA_HOUR] = twi_read_byte(TWI_ACK);
            alarm_data[alarm.now][ALARM_DATA_MINS] = twi_read_byte(TWI_ACK);
            alarm_data[alarm.now][ALARM_DATA_MODE] = twi_read_byte(TWI_ACK);
            alarm_data[alarm.now][ALARM_DATA_DAYS] = twi_read_byte(TWI_ACK);
            uint8_t tempSound = twi_read_byte(TWI_ACK);
            uint8_t tempVolume = twi_read_byte(TWI_ACK);
            alarm_data[alarm.now][ALARM_DATA_RADIO] = twi_read_byte(TWI_NACK);
            if (!alarm_data[alarm.now][ALARM_DATA_RADIO]) {
              alarm_data[alarm.now][ALARM_DATA_SOUND] = tempSound;
              alarm_data[alarm.now][ALARM_DATA_VOLUME] = ((100.0 / deviceInformation[PLAYER_MAX_VOL]) * tempVolume);
            }
            else {
              alarm_data[alarm.now][ALARM_DATA_STATION] = tempSound;
              alarm_data[alarm.now][ALARM_DATA_VOLUME] = ((100.0 / 30.0) * tempVolume);
            }
            twi_write_stop(); //завершаем передачу
          }
        }
        break;
      case WRITE_SELECT_ALARM:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          switch (busReadBuffer()) {
            case ALARM_TIME: busWriteTwiRegByte(alarm.now, BUS_WRITE_SELECT_ALARM, 0); twi_write_byte(alarm_data[alarm.now][ALARM_DATA_HOUR]); twi_write_byte(alarm_data[alarm.now][ALARM_DATA_MINS]); break; //время будильника
            case ALARM_MODE: busWriteTwiRegByte(alarm.now, BUS_WRITE_SELECT_ALARM, 2); twi_write_byte(alarm_data[alarm.now][ALARM_DATA_MODE]); break; //режим будильника
            case ALARM_DAYS: busWriteTwiRegByte(alarm.now, BUS_WRITE_SELECT_ALARM, 3); twi_write_byte(alarm_data[alarm.now][ALARM_DATA_DAYS]); break; //день недели будильника
            case ALARM_SOUND: busWriteTwiRegByte(alarm.now, BUS_WRITE_SELECT_ALARM, 4); twi_write_byte((!alarm_data[alarm.now][ALARM_DATA_RADIO]) ? alarm_data[alarm.now][ALARM_DATA_SOUND] : alarm_data[alarm.now][ALARM_DATA_STATION]); break; //мелодия будильника
            case ALARM_VOLUME: { //громкость будильника
                float volume = alarm_data[alarm.now][ALARM_DATA_VOLUME] / 100.0;
                busWriteTwiRegByte(alarm.now, BUS_WRITE_SELECT_ALARM, 5);
                twi_write_byte(((alarm_data[alarm.now][ALARM_DATA_RADIO]) ? 30 : deviceInformation[PLAYER_MAX_VOL]) * volume);
              }
              break;
            case ALARM_RADIO: busWriteTwiRegByte(alarm.now, BUS_WRITE_SELECT_ALARM, 6); twi_write_byte(alarm_data[alarm.now][ALARM_DATA_RADIO]); break; //радиобудильник
          }
          busShiftBuffer(); //сместили буфер команд
          twi_write_stop(); //завершаем передачу
        }
        break;
      case DEL_ALARM:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_DEL_ALARM); //регистр времени
          twi_write_byte(busReadBuffer()); //регистр времени
          busShiftBuffer(); //сместили буфер команд
          twi_write_stop(); //завершаем передачу
        }
        break;
      case NEW_ALARM:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_NEW_ALARM); //регистр времени
          twi_write_stop(); //завершаем передачу
        }
        break;

      case READ_RADIO_SET:
        if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_RADIO_SET)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          radioSettings.volume = twi_read_byte(TWI_ACK);
          twi_read_byte(TWI_ACK);
          radioSettings.stationsFreq = twi_read_byte(TWI_ACK) | ((uint16_t)twi_read_byte(TWI_ACK) << 8);
          for (uint8_t i = 0; i < 10; i++) {
            radioSettings.stationsSave[i] = twi_read_byte(TWI_ACK) | ((uint16_t)twi_read_byte((i < 9) ? TWI_ACK : TWI_NACK) << 8); //отправляем натройки радиостанций
          }
          twi_write_stop(); //завершаем передачу
        }
        break;
      case READ_RADIO_STA:
        if (!twi_requestFrom(CLOCK_ADDRESS, BUS_SELECT_BYTE, 0x04, BUS_READ_RADIO_SET)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          for (uint8_t i = 0; i < 10; i++) {
            radioSettings.stationsSave[i] = twi_read_byte(TWI_ACK) | ((uint16_t)twi_read_byte((i < 9) ? TWI_ACK : TWI_NACK) << 8); //отправляем натройки радиостанций
          }
          twi_write_stop(); //завершаем передачу
        }
        break;
      case WRITE_RADIO_STA:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          busWriteTwiRegWord(radioSettings.stationsSave[busReadBuffer()], BUS_WRITE_RADIO_STA, busReadBuffer()); //отправляем дополнительные натройки
          busShiftBuffer(); //сместили буфер команд
          twi_write_stop(); //завершаем передачу
        }
        break;
      case READ_RADIO_VOL:
        if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_RADIO_SET)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          radioSettings.volume = twi_read_byte(TWI_NACK);
          twi_write_stop(); //завершаем передачу
        }
        break;
      case WRITE_RADIO_VOL:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_WRITE_RADIO_VOL); //регистр времени
          twi_write_byte(radioSettings.volume); //регистр времени
          twi_write_stop(); //завершаем передачу
        }
        break;
      case READ_RADIO_FREQ:
        if (!twi_requestFrom(CLOCK_ADDRESS, BUS_SELECT_BYTE, 0x02, BUS_READ_RADIO_SET)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          radioSettings.stationsFreq = twi_read_byte(TWI_ACK) | ((uint16_t)twi_read_byte(TWI_NACK) << 8);
          twi_write_stop(); //завершаем передачу
        }
        break;
      case WRITE_RADIO_FREQ:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_WRITE_RADIO_FREQ); //регистр времени
          twi_write_byte(radioSettings.stationsFreq & 0xFF); //регистр времени
          twi_write_byte((radioSettings.stationsFreq >> 8) & 0xFF);
          twi_write_stop(); //завершаем передачу
        }
        break;
      case WRITE_RADIO_MODE:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_WRITE_RADIO_MODE); //регистр времени
          twi_write_stop(); //завершаем передачу
        }
        break;
      case WRITE_RADIO_POWER:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_WRITE_RADIO_POWER); //регистр времени
          twi_write_stop(); //завершаем передачу
        }
        break;
      case READ_RADIO_POWER:
        if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_RADIO_POWER)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          radioSettings.powerState = twi_read_byte(TWI_NACK);
          twi_write_stop(); //завершаем передачу
        }
        break;
      case RADIO_FREQ_UP:
        busShiftBuffer(); //сместили буфер команд
        if (radioSettings.stationsFreq < 1080) {
          radioSettings.stationsFreq++;
          busSetComand(WRITE_RADIO_FREQ);
        }
        else radioSettings.stationsFreq = 1080;
        break;
      case RADIO_FREQ_DOWN:
        busShiftBuffer(); //сместили буфер команд
        if (radioSettings.stationsFreq > 870) {
          radioSettings.stationsFreq--;
          busSetComand(WRITE_RADIO_FREQ);
        }
        else radioSettings.stationsFreq = 870;
        break;
      case RADIO_SEEK_UP:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_SEEK_RADIO_UP); //регистр времени
          twi_write_stop(); //завершаем передачу
        }
        break;
      case RADIO_SEEK_DOWN:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_SEEK_RADIO_DOWN); //регистр времени
          twi_write_stop(); //завершаем передачу
        }
        break;

      case READ_SENS_DATA:
        if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_TEMP)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          sens.temp = twi_read_byte(TWI_ACK) | ((uint16_t)twi_read_byte(TWI_ACK) << 8);
          sens.press = twi_read_byte(TWI_ACK) | ((uint16_t)twi_read_byte(TWI_ACK) << 8);
          sens.hum = twi_read_byte(TWI_NACK);
          twi_write_stop(); //завершаем передачу
          climateUpdate(); //обновляем показания графиков
        }
        break;
      case READ_SENS_INFO:
        if (!twi_requestFrom(CLOCK_ADDRESS, BUS_SELECT_BYTE, 0x05, BUS_READ_TEMP)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          sens.type = twi_read_byte(TWI_ACK); //тип датчика температуры
          sens.init = twi_read_byte(TWI_ACK); //флаг инициализации порта
          sens.err = twi_read_byte(TWI_NACK); //ошибка сенсора
          twi_write_stop(); //завершаем передачу
        }
        break;
      case WRITE_CHECK_SENS:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_CHECK_TEMP); //регистр времени
          twi_write_stop(); //завершаем передачу
        }
        break;

      case READ_EXTENDED_SET:
        if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_EXTENDED_SET)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          for (uint8_t i = 0; i < 5; i++) {
            extendedSettings.autoShowModes[i] = twi_read_byte(TWI_ACK);
          }
          for (uint8_t i = 0; i < 5; i++) {
            extendedSettings.autoShowTimes[i] = twi_read_byte(TWI_ACK);
          }
          extendedSettings.burnTime = twi_read_byte(TWI_ACK);
          extendedSettings.alarmTime = twi_read_byte(TWI_ACK);
          extendedSettings.alarmWaitTime = twi_read_byte(TWI_ACK);
          extendedSettings.alarmSoundTime = twi_read_byte(TWI_ACK);
          extendedSettings.alarmDotOn = twi_read_byte(TWI_ACK);
          extendedSettings.alarmDotWait = twi_read_byte(TWI_NACK);
          twi_write_stop(); //завершаем передачу
        }
        break;
      case WRITE_EXTENDED_SHOW_MODE:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          busWriteTwiRegByte(extendedSettings.autoShowModes[busReadBuffer()], BUS_WRITE_EXTENDED_SET, busReadBuffer());
          busShiftBuffer(); //сместили буфер команд
          twi_write_stop(); //завершаем передачу
        }
        break;
      case WRITE_EXTENDED_SHOW_TIME:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          busWriteTwiRegByte(extendedSettings.autoShowTimes[busReadBuffer()], BUS_WRITE_EXTENDED_SET, busReadBuffer() + 5);
          busShiftBuffer(); //сместили буфер команд
          twi_write_stop(); //завершаем передачу
        }
        break;
      case WRITE_EXTENDED_BURN_TIME:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          busWriteTwiRegByte(extendedSettings.burnTime, BUS_WRITE_EXTENDED_SET, 10);
          busWriteBuffer(SET_BURN_TIME);
          twi_write_stop(); //завершаем передачу
        }
        break;
      case WRITE_EXTENDED_ALARM:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          switch (busReadBuffer()) {
            case EXT_ALARM_TIMEOUT: busWriteTwiRegByte(extendedSettings.alarmTime, BUS_WRITE_EXTENDED_SET, 11); break; //отправляем дополнительные натройки
            case EXT_ALARM_WAIT: busWriteTwiRegByte(extendedSettings.alarmWaitTime, BUS_WRITE_EXTENDED_SET, 12); break; //отправляем дополнительные натройки
            case EXT_ALARM_TIMEOUT_SOUND: busWriteTwiRegByte(extendedSettings.alarmSoundTime, BUS_WRITE_EXTENDED_SET, 13); break; //отправляем дополнительные натройки
            case EXT_ALARM_DOT_ON: busWriteTwiRegByte(extendedSettings.alarmDotOn, BUS_WRITE_EXTENDED_SET, 14); busWriteBuffer(SET_ALARM_DOT); break; //отправляем дополнительные натройки
            case EXT_ALARM_DOT_WAIT: busWriteTwiRegByte(extendedSettings.alarmDotWait, BUS_WRITE_EXTENDED_SET, 15); busWriteBuffer(SET_ALARM_DOT); break; //отправляем дополнительные натройки
          }
          busShiftBuffer(); //сместили буфер команд
          twi_write_stop(); //завершаем передачу
        }
        break;

      case SET_SHOW_TIME:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_SET_SHOW_TIME); //регистр времени
          twi_write_stop(); //завершаем передачу
        }
        break;
      case SET_BURN_TIME:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_SET_BURN_TIME); //регистр времени
          twi_write_stop(); //завершаем передачу
        }
        break;
      case SET_ALARM_DOT:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_SET_ALARM_DOT); //регистр времени
          twi_write_stop(); //завершаем передачу
        }
        break;
      case SET_MAIN_DOT:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_SET_MAIN_DOT); //регистр времени
          twi_write_stop(); //завершаем передачу
        }
        break;

      case WRITE_TEST_MAIN_VOL:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_TEST_SOUND); //регистр времени
          twi_write_byte(mainSettings.volumeSound); //громкость
          twi_write_byte(21); //номер трека
          twi_write_byte(4); //номер папки
          twi_write_stop(); //завершаем передачу
        }
        break;
      case WRITE_TEST_ALARM_VOL:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_TEST_SOUND); //регистр времени
          twi_write_byte((uint8_t)(deviceInformation[PLAYER_MAX_VOL] * (alarm_data[alarm.now][ALARM_DATA_VOLUME] / 100.0))); //громкость
          twi_write_byte(21); //номер трека
          twi_write_byte(4); //номер папки
          twi_write_stop(); //завершаем передачу
        }
        break;
      case WRITE_TEST_ALARM_SOUND:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_TEST_SOUND); //регистр времени
          twi_write_byte((uint8_t)(deviceInformation[PLAYER_MAX_VOL] * (alarm_data[alarm.now][ALARM_DATA_VOLUME] / 100.0))); //громкость
          twi_write_byte(alarm_data[alarm.now][ALARM_DATA_SOUND] + 1); //номер трека
          twi_write_byte(5); //номер папки
          twi_write_stop(); //завершаем передачу
        }
        break;
      case WRITE_TEST_MAIN_FLIP:
        if (!twi_beginTransmission(CLOCK_ADDRESS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          twi_write_byte(BUS_TEST_FLIP); //регистр времени
          twi_write_stop(); //завершаем передачу
        }
        break;

      case READ_STATUS:
        if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_STATUS)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          deviceStatus = twi_read_byte(TWI_NACK);
          twi_write_stop(); //завершаем передачу
        }
        break;
      case READ_DEVICE:
        if (!twi_requestFrom(CLOCK_ADDRESS, BUS_READ_DEVICE)) { //начинаем передачу
          busShiftBuffer(); //сместили буфер команд
          deviceInformation[FIRMWARE_VERSION_1] = twi_read_byte(TWI_ACK);
          deviceInformation[FIRMWARE_VERSION_2] = twi_read_byte(TWI_ACK);
          deviceInformation[FIRMWARE_VERSION_3] = twi_read_byte(TWI_ACK);
          deviceInformation[HARDWARE_VERSION] = twi_read_byte(TWI_ACK);
          deviceInformation[SENS_TEMP] = twi_read_byte(TWI_ACK);
          deviceInformation[LAMP_NUM] = twi_read_byte(TWI_ACK);
          deviceInformation[BACKL_TYPE] = twi_read_byte(TWI_ACK);
          deviceInformation[NEON_DOT] = twi_read_byte(TWI_ACK);
          deviceInformation[DOTS_PORT_ENABLE] = twi_read_byte(TWI_ACK);
          deviceInformation[DOTS_NUM] = twi_read_byte(TWI_ACK);
          deviceInformation[DOTS_TYPE] = twi_read_byte(TWI_ACK);
          deviceInformation[RADIO_ENABLE] = twi_read_byte(TWI_ACK);
          deviceInformation[ALARM_TYPE] = twi_read_byte(TWI_ACK);
          deviceInformation[PLAYER_TYPE] = twi_read_byte(TWI_ACK);
          deviceInformation[PLAYER_MAX_SOUND] = twi_read_byte(TWI_ACK);
          deviceInformation[PLAYER_MAX_VOL] = twi_read_byte(TWI_NACK);
          twi_write_stop(); //завершаем передачу
        }
        break;
      default: busShiftBuffer(); break; //сместили буфер команд
    }
  }
}
