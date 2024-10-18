uint8_t climateCountAvg = 0; //счетчик циклов обновления микроклимата
int16_t climateTempAvg = 0; //буфер средней температуры микроклимата
uint16_t climateHumAvg = 0; //буфер средней влажности микроклимата
uint16_t climatePressAvg = 0; //буфер среднего давления микроклимата

int8_t climateState = -1; //флаг состояние активации микроклимата

int16_t climateArrMain[2][CLIMATE_BUFFER]; //буфер температуры и влажности микроклимата
int16_t climateArrExt[1][CLIMATE_BUFFER]; //буфер давления микроклимата
uint32_t climateDates[CLIMATE_BUFFER]; //буфер отметок времени микроклимата

String climateSensorsList = "Отсутсвует"; //список подключенных сенсоров температуры
const char *climateTempSensList[] = {"DS3231", "AHT", "SHT", "BMP/BME", "DS18B20", "DHT"};
const char *sensDataList[] = {"CLOCK", "AHT", "SHT", "BMP", "BME"};

void climateAdd(int16_t temp, int16_t hum, int16_t press, uint32_t unix);

//--------------------------------------------------------------------
String climateGetSensList(void) {
  String str = "";

  if (deviceInformation[SENS_TEMP]) {
    str += "Датчик в часах,";
    if (weatherGetValidStatus() && settings.climateSend) str += "Данные о погоде";
    else str += "Датчик в есп";
  }
  else if (climateState != 0) str += "Датчик в есп,Данные о погоде";
  else str += "Данные о погоде,Датчик в есп";

  return str;
}
//--------------------------------------------------------------------
int16_t climateGetTemp(void) {
  return sens.mainTemp[0] + mainSettings.tempCorrect;
}
float climateGetTempFloat(void) {
  return (climateGetTemp()) ? (climateGetTemp() / 10.0) : 0;
}
uint16_t climateGetPress(void) {
  return sens.mainPress[0];
}
uint8_t climateGetHum(void) {
  return sens.mainHum[0];
}
//--------------------------------------------------------------------
void climateSet(void) {
  static boolean firstStart;

  if (!firstStart) {
    firstStart = true;
    climateState = 0;

    if (sens.status) {
      if (!(sens.status & (0x01 << settings.climateType[0]))) {
        settings.climateType[0] = 0;
        for (uint8_t i = 0; i < 4; i++) {
          if (sens.status & (0x01 << i)) {
            settings.climateType[0] = i;
            climateState = 1;
            break;
          }
        }
      }
      else climateState = 1;
      sens.search = sens.status;

      if (!sens.press[settings.climateType[1]]) {
        settings.climateType[1] = 0;
        for (uint8_t i = 0; i < 4; i++) {
          if (sens.press[i]) {
            settings.climateType[1] = i;
            break;
          }
        }
      }

      if (!sens.hum[settings.climateType[2]]) {
        settings.climateType[2] = 0;
        for (uint8_t i = 0; i < 4; i++) {
          if (sens.hum[i]) {
            settings.climateType[2] = i;
            break;
          }
        }
      }

      climateSensorsList = "";
      memory.update(); //обновить данные в памяти
    }

    for (uint8_t i = 0; i < 4; i++) {
      if (sens.status & (0x01 << i)) {
        if (i) {
          if (climateSensorsList.length() > 0) climateSensorsList += "+";
          climateSensorsList += climateTempSensList[i];
        }
        else climateSensorsList += (sens.err) ? "Ошибка" : climateTempSensList[sens.type];
      }
      else if (!i && deviceInformation[SENS_TEMP]) climateSensorsList = "Ошибка";
    }
  }

  sens.mainTemp[0] = sens.temp[settings.climateType[0]];
  sens.mainPress[0] = sens.press[settings.climateType[1]];
  sens.mainHum[0] = sens.hum[settings.climateType[2]];
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
void climateReset(void) {
  climateTempAvg = 0;
  climateHumAvg = 0;
  climatePressAvg = 0;
  climateCountAvg = 0;
}
//--------------------------------------------------------------------
void climateUpdate(void) {
  static int8_t firstStart = -1;

  uint32_t unixNow = GPunix(mainDate.year, mainDate.month, mainDate.day, mainTime.hour, mainTime.minute, 0, 0);

  if (firstStart < timeState) {
    firstStart = timeState;
    climateDefault(climateGetTemp(), climateGetHum(), climateGetPress(), unixNow);
    climateReset(); //сброс усреднения
  }
  else {
    climateTempAvg += climateGetTemp();
    climatePressAvg += climateGetPress();
    climateHumAvg += climateGetHum();

    if (++climateCountAvg >= settings.climateTime) {
      if (settings.climateAvg && (climateCountAvg > 1)) {
        if (climateTempAvg) climateTempAvg /= climateCountAvg;
        if (climatePressAvg) climatePressAvg /= climateCountAvg;
        if (climateHumAvg) climateHumAvg /= climateCountAvg;
        climateAdd(climateTempAvg, climateHumAvg, climatePressAvg, unixNow);
      }
      else climateAdd(climateGetTemp(), climateGetHum(), climateGetPress(), unixNow);
      climateReset(); //сброс усреднения
    }
  }
}
