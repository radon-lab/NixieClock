uint8_t climateCountAvg = 0; //счетчик циклов обновления микроклимата
int16_t climateTempAvg = 0; //буфер средней температуры микроклимата
uint16_t climateHumAvg = 0; //буфер средней влажности микроклимата
uint16_t climatePressAvg = 0; //буфер среднего давления микроклимата

int8_t climateState = -1; //флаг состояние активации микроклимата

int16_t climateArrMain[2][CLIMATE_BUFFER]; //буфер температуры и влажности микроклимата
int16_t climateArrExt[1][CLIMATE_BUFFER]; //буфер давления микроклимата
uint32_t climateDates[CLIMATE_BUFFER]; //буфер отметок времени микроклимата

enum {
  CLIMATE_UPDATE, //добавить данные в статистику микроклимата
  CLIMATE_RESET //сбросить данные в статистике микроклимата
};

const char *climateDataList[] = {"(часы)", "(есп)", "(датчик)", "(погода)", "(недоступно)"};
const char *climateShowList[] = {"Температура", "Влажность", "Давление", "Температура и влажность"};
const char *climateTempSensList[] = {"DS3231", "AHT", "SHT", "BMP/BME", "DS18B20", "DHT"};

void climateAdd(int16_t temp, int16_t hum, int16_t press, uint32_t unix);

//--------------------------------------------------------------------
String climateGetMainSensList(void) {
  String str;
  str.reserve(50);
  str = "";

  uint8_t sensor = 0x02;
  for (uint8_t i = 1; i < 4; i++) {
    if (sens.search & sensor) {
      if (str[0] != '\0') str += '+';
      str += climateTempSensList[i];
    }
    sensor <<= 1;
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
String climateGetSensList(uint8_t data) {
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
void climateReset(void) {
  climateTempAvg = 0;
  climateHumAvg = 0;
  climatePressAvg = 0;
  climateCountAvg = 0;
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
    uint32_t unixNow = GPunix(mainDate.year, mainDate.month, mainDate.day, mainTime.hour, mainTime.minute, 0, 0);

    if ((first_start < timeState) || (mode == CLIMATE_RESET)) {
      first_start = timeState;
      climateDefault(climateGetChartTemp(), climateGetChartHum(), climateGetChartPress(), unixNow);
      climateReset(); //сброс усреднения
    }
    else if (settings.climateChart == SENS_WIRELESS) {
      climateAdd(climateGetChartTemp(), climateGetChartHum(), climateGetChartPress(), unixNow);
    }
    else {
      climateTempAvg += climateGetChartTemp();
      climatePressAvg += climateGetChartPress();
      climateHumAvg += climateGetChartHum();

      if (++climateCountAvg >= settings.climateTime) {
        if (settings.climateAvg && (climateCountAvg > 1)) {
          if (climateTempAvg) climateTempAvg /= climateCountAvg;
          if (climatePressAvg) climatePressAvg /= climateCountAvg;
          if (climateHumAvg) climateHumAvg /= climateCountAvg;
          climateAdd(climateTempAvg, climateHumAvg, climatePressAvg, unixNow);
        }
        else climateAdd(climateGetChartTemp(), climateGetChartHum(), climateGetChartPress(), unixNow);
        climateReset(); //сброс усреднения
      }
    }
  }
}
