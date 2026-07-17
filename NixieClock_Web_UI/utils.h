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

const char *weatherSummaryList[] = {LANG_WEATHER_SUMMARY_1, LANG_WEATHER_SUMMARY_2, LANG_WEATHER_SUMMARY_3, LANG_WEATHER_SUMMARY_4};

const char *failureDataList[] = {
  LANG_FAIL_DATA_1, LANG_FAIL_DATA_2, LANG_FAIL_DATA_3, LANG_FAIL_DATA_4, LANG_FAIL_DATA_5,
  LANG_FAIL_DATA_6, LANG_FAIL_DATA_7, LANG_FAIL_DATA_8, LANG_FAIL_DATA_9, LANG_FAIL_DATA_10,
  LANG_FAIL_DATA_11, LANG_FAIL_DATA_12, LANG_FAIL_DATA_13, LANG_FAIL_DATA_14, LANG_FAIL_DATA_15
};

//--------------------------------------------------------------------
boolean stringCheckCorrect(char* str, uint8_t size) {
  uint8_t data = 0;
  for (uint8_t i = 0; i < size; i++) {
    data = str[i];
    if (!data) return (boolean)i;
    else if ((data < 32) || (data > 126)) return false;
  }
  return true;
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
String stringEncodeTime(GPtime data) {
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
  str = deviceInformation[FIRMWARE_VER_H];
  str += '.';
  str += deviceInformation[FIRMWARE_VER_M];
  str += '.';
  str += deviceInformation[FIRMWARE_VER_L];
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
uint8_t backlGetColorCode(uint8_t color) {
  return (color < 253) ? (color / 10) : (color - 227);
}
//--------------------------------------------------------------------
uint8_t backlConvertColorCode(uint8_t color) {
  return (color > 25) ? (color + 227) : (color * 10);
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
  if (deviceInformation[SECS_TYPE] < 3) {
    str += F(LANG_DOTS_MODE_2);
  }
  if (deviceInformation[SECS_TYPE] == 2) {
    str += F(LANG_DOTS_MODE_3);
  }
  if (deviceInformation[SECS_TYPE] == 4) {
    str += F(LANG_DOTS_MODE_4);
  }
  if (deviceInformation[DOTS_ENABLE]) {
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
  if (deviceInformation[SECS_TYPE] < 3) {
    if (!deviceInformation[DOTS_ENABLE]) {
      str = F(LANG_INDI_DOTS_MODE_1);
    }
    else {
      str = F(LANG_INDI_DOTS_MODE_2);
      if (deviceInformation[SECS_TYPE] == 2) {
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
