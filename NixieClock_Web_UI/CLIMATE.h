uint8_t climate_count_avg = 0; //счетчик циклов обновления микроклимата
int16_t climate_temp_avg = 0; //буфер средней температуры микроклимата
uint16_t climate_hum_avg = 0; //буфер средней влажности микроклимата
uint16_t climate_press_avg = 0; //буфер среднего давления микроклимата

int16_t climateArrMain[2][CLIMATE_BUFFER]; //буфер температуры и влажности микроклимата
int16_t climateArrExt[1][CLIMATE_BUFFER]; //буфер давления микроклимата
uint32_t climateDates[CLIMATE_BUFFER]; //буфер отметок времени микроклимата

enum {
  CLIMATE_UPDATE, //добавить данные в статистику микроклимата
  CLIMATE_RESET //сбросить данные в статистике микроклимата
};

struct sensorData {
  int16_t temp[4] = {0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF}; //температура
  uint16_t press[4]; //давление
  uint8_t hum[4]; //влажность
  uint8_t search; //флаги найденых датчиков температуры
  uint8_t status; //флаги активных датчиков температуры
  uint8_t update; //флаги опрошенных датчиков температуры
  uint8_t type; //тип датчика температуры
  boolean init; //флаг инициализации порта
  boolean err; //ошибка сенсора
} sens;

enum {
  SENS_CLOCK,
  SENS_MAIN,
  SENS_WIRELESS,
  SENS_WEATHER,
  SENS_MAX_DATA
};

#define SENS_EXT 0x01
#define SENS_AHT 0x02
#define SENS_SHT 0x04
#define SENS_BME 0x08

const char *climateDataList[] = {"(часы)", "(есп)", "(датчик)", "(погода)", "(недоступно)"};
const char *climateShowList[] = {"Температура", "Влажность", "Давление", "Температура и влажность"};
const char *climateTempSensList[] = {"DS3231", "AHT", "SHT", "BMP/BME", "DS18B20", "DHT"};

void climateAdd(int16_t temp, int16_t hum, int16_t press, uint32_t unix);

//--------------------------------------------------------------------
String climateGetSensList(uint8_t sens, boolean shift) {
  String str;
  str.reserve(50);
  str = "";
  
  if (shift) sens >>= 1;
  for (uint8_t i = shift; i < 6; i++) {
    if (sens & 0x01) {
      if (str[0] != '\0') str += '+';
      str += climateTempSensList[i];
    }
    sens >>= 1;
  }
  if (str[0] == '\0') str = "Нет данных";

  return str;
}
//--------------------------------------------------------------------
String climateGetSensDataStr(float temp, uint16_t press, uint8_t hum) {
  String str;
  str.reserve(30);
  str = F("Нет данных");

  if (temp != 0x7FFF) {
    str = String(temp / 10.0, 1) + F("°С");
    if (hum) {
      str += ' ';
      str += hum;
      str += '%';
    }
    if (press) {
      str += ' ';
      str += press;
      str += F("mm.Hg");
    }
  }

  return str;
}
//--------------------------------------------------------------------
String climateGetShowDataList(uint8_t data) {
  String str;
  str.reserve(100);
  str = "";

  for (uint8_t i = 0; i < 4; i++) {
    str += ',';
    str += climateShowList[i];
    if ((i < 3) || (deviceInformation[LAMP_NUM] > 4)) str += climateDataList[data];
    else str += climateDataList[4];
  }

  return str;
}
//--------------------------------------------------------------------
int16_t climateGetTemp(uint8_t data) {
  if (sens.temp[data] == 0x7FFF) return 0x7FFF;
  if (extendedSettings.tempCorrectSensor) {
    if (settings.climateSend[extendedSettings.tempCorrectSensor - 1] == data) return sens.temp[data] + mainSettings.tempCorrect;
  }
  return sens.temp[data];
}
//--------------------------------------------------------------------
boolean climateAvailableTemp(uint8_t data) {
  return (sens.temp[data] != 0x7FFF);
}
//--------------------------------------------------------------------
int16_t climateGetChartTemp(void) {
  return climateGetTemp(settings.climateChart);
}
uint16_t climateGetChartPress(void) {
  return sens.press[settings.climateChart];
}
uint8_t climateGetChartHum(void) {
  return sens.hum[settings.climateChart];
}
//--------------------------------------------------------------------
int16_t climateGetBarTemp(void) {
  return climateGetTemp(settings.climateBar);
}
float climateGetBarTempFloat(void) {
  return (climateGetBarTemp() != 0x7FFF) ? (climateGetBarTemp() / 10.0) : 0;
}
uint16_t climateGetBarPress(void) {
  return sens.press[settings.climateBar];
}
uint8_t climateGetBarHum(void) {
  return sens.hum[settings.climateBar];
}
//--------------------------------------------------------------------
String climateGetBarTempStr(void) {
  return String(climateGetBarTempFloat(), 1) + "°С";
}
String climateGetBarHumStr(void) {
  return String(climateGetBarHum()) + "%";
}
String climateGetBarPressStr(void) {
  return String(climateGetBarPress()) + "mm.Hg";
}
//--------------------------------------------------------------------
void climateReset(void) {
  climate_temp_avg = 0;
  climate_hum_avg = 0;
  climate_press_avg = 0;
  climate_count_avg = 0;
}
//--------------------------------------------------------------------
void climateDefault(int16_t temp, int16_t hum, int16_t press, uint32_t unix) {
  for (uint8_t i = 0; i < CLIMATE_BUFFER; i++) {
    climateAdd(temp, hum, press, unix);
  }
}
//--------------------------------------------------------------------
void climateAdd(int16_t temp, int16_t hum, int16_t press, uint32_t unix) {
  if (climateDates[CLIMATE_BUFFER - 1] > unix) {
    climateDefault(temp, hum, press, unix);
  }
  else {
    GPaddInt(temp, climateArrMain[0], CLIMATE_BUFFER);
    if (hum) {
      GPaddInt(hum * 10, climateArrMain[1], CLIMATE_BUFFER);
    }
    if (press) {
      GPaddInt(press, climateArrExt[0], CLIMATE_BUFFER);
    }
    GPaddUnix(unix, climateDates, CLIMATE_BUFFER);
  }
}
//--------------------------------------------------------------------
void climateUpdate(boolean mode) {
  static int8_t first_start = -1;

  if (climateGetChartTemp() != 0x7FFF) {
    uint32_t unix_now = GPunix(mainDate.year, mainDate.month, mainDate.day, mainTime.hour, mainTime.minute, 0, 0);

    if ((first_start < timeState) || (mode == CLIMATE_RESET)) {
      first_start = timeState;
      climateDefault(climateGetChartTemp(), climateGetChartHum(), climateGetChartPress(), unix_now);
      climateReset(); //сброс усреднения
    }
    else if (settings.climateChart == SENS_WIRELESS) {
      climateAdd(climateGetChartTemp(), climateGetChartHum(), climateGetChartPress(), unix_now);
    }
    else {
      climate_temp_avg += climateGetChartTemp();
      climate_press_avg += climateGetChartPress();
      climate_hum_avg += climateGetChartHum();

      if (++climate_count_avg >= settings.climateTime) {
        if (settings.climateAvg && (climate_count_avg > 1)) {
          if (climate_temp_avg) climate_temp_avg /= climate_count_avg;
          if (climate_press_avg) climate_press_avg /= climate_count_avg;
          if (climate_hum_avg) climate_hum_avg /= climate_count_avg;
          climateAdd(climate_temp_avg, climate_hum_avg, climate_press_avg, unix_now);
        }
        else climateAdd(climateGetChartTemp(), climateGetChartHum(), climateGetChartPress(), unix_now);
        climateReset(); //сброс усреднения
      }
    }
  }
}
