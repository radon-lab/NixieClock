#define BME_ADDR 0x76 //адрес датчика

#define MODE 0x03 //нормальный режим
#define STANDBY 0x05 //замер каждую секунду
#define FILTER_COEF 0x04 //коэффициент фильтрации 16

#define PRESS_OVERSAMP 0x02 //разрешение датчика давления 2
#define TEMP_OVERSAMP 0x03 //разрешение датчика температуры 4
#define HUM_OVERSAMP 0x01 //разрешение датчика влажности 1

struct CalibrationData { //структура калибровок датчика
  uint16_t TEMP_1;
  int16_t TEMP_2;
  int16_t TEMP_3;
  uint16_t PRESS_1;
  int16_t PRESS_2;
  int16_t PRESS_3;
  int16_t PRESS_4;
  int16_t PRESS_5;
  int16_t PRESS_6;
  int16_t PRESS_7;
  int16_t PRESS_8;
  int16_t PRESS_9;
  uint8_t HUM_1;
  int16_t HUM_2;
  uint8_t HUM_3;
  int16_t HUM_4;
  int16_t HUM_5;
  int8_t HUM_6;
} Calibration;

struct temp { //структура температуры
  uint16_t temp = 0;
  uint16_t press = 0;
  uint8_t hum = 0;
} tempSens;

boolean initDev = 0; //флаг инициализации датчика

//--------------------------------------Запись одного байта------------------------------------------
void writeREG(uint8_t address , uint8_t data) //Запись одного байта
{
  WireBeginTransmission(BME_ADDR); //начало передачи
  WireWrite(address); //устанавливаем адрес записи
  WireWrite(data); //записываем байт
  if (WireEndTransmission() != 0) return; //если нет ответа выходим
}
//--------------------------------------Чтение калибровок датчика------------------------------------------
void readCalibrat(void) //чтение калибровок датчика
{
  WireBeginTransmission(BME_ADDR); //начало передачи
  WireWrite(0x88); //устанавливаем адрес чтения
  if (WireEndTransmission() != 0) return; //если нет ответа выходим
  WireRequestFrom(BME_ADDR , 25); //ожидаем 25 байт

  Calibration.TEMP_1 = (WireRead() | (WireRead() << 8)); //читаем
  Calibration.TEMP_2 = (WireRead() | (WireRead() << 8));
  Calibration.TEMP_3 = (WireRead() | (WireRead() << 8));
  Calibration.PRESS_1 = (WireRead() | (WireRead() << 8));
  Calibration.PRESS_2 = (WireRead() | (WireRead() << 8));
  Calibration.PRESS_3 = (WireRead() | (WireRead() << 8));
  Calibration.PRESS_4 = (WireRead() | (WireRead() << 8));
  Calibration.PRESS_5 = (WireRead() | (WireRead() << 8));
  Calibration.PRESS_6 = (WireRead() | (WireRead() << 8));
  Calibration.PRESS_7 = (WireRead() | (WireRead() << 8));
  Calibration.PRESS_8 = (WireRead() | (WireRead() << 8));
  Calibration.PRESS_9 = (WireRead() | (WireRead() << 8));
  Calibration.HUM_1 = WireRead();


  WireBeginTransmission(BME_ADDR); //начало передачи
  WireWrite(0xE1); //устанавливаем адрес чтения
  if (WireEndTransmission() != 0) return; //если нет ответа выходим
  WireRequestFrom(BME_ADDR , 8); //ожидаем 8 байт

  Calibration.HUM_2 = (WireRead() | (WireRead() << 8)); //читаем
  Calibration.HUM_3 = WireRead();
  Calibration.HUM_4 = (WireRead() << 4);
  uint8_t interVal = WireRead();
  Calibration.HUM_4 |= (interVal & 0x0F);
  Calibration.HUM_5 = (((interVal & 0xF0) >> 4) | (WireRead() << 4));
  Calibration.HUM_6 = WireRead();
}
//--------------------------------------Чтение температуры/давления/влажности------------------------------------------
void readTempBME(void) //чтение температуры/давления/влажности
{
  if (!initDev) { //если датчик не настроен
    initDev = 1; //устанавливаем флаг инициализации датчика
    readCalibrat(); //читаем калибровки датчика

    writeREG(0xF2, HUM_OVERSAMP); //устанавливаем разрешение датчика влажности
    writeREG(0xF4, ((TEMP_OVERSAMP << 5) | (PRESS_OVERSAMP << 2) | MODE)); //устанавливаем разрешение датчика температуры и датчика давления, устанавливаем режим работы
    writeREG(0xF5, ((STANDBY << 5) | (FILTER_COEF << 2))); //устанавливаем частоту опроса и коэффициент фильтрации
  }

  WireBeginTransmission(BME_ADDR); //начало передачи
  WireWrite(0xF7); //устанавливаем адрес чтения
  if (WireEndTransmission() != 0) return; //если нет ответа выходим
  WireRequestFrom(BME_ADDR, 8); //ожидаем 8 байт

  uint32_t press_raw = (((uint32_t)WireRead() << 16) | ((uint32_t)WireRead() << 8) | (uint32_t)WireRead()) >> 4; //читаем 24-х битное значение ацп давления
  int32_t temp_raw = (((uint32_t)WireRead() << 16) | ((uint32_t)WireRead() << 8) | (uint32_t)WireRead()) >> 4; //читаем 24-х битное значение ацп температуры
  int32_t hum_raw = ((uint16_t)WireRead() << 8) | (uint16_t)WireRead(); //читаем 16-и битное значение ацп влажности


  int32_t temp_val_1 = ((((temp_raw >> 3) - ((int32_t)Calibration.TEMP_1 << 1))) * ((int32_t)Calibration.TEMP_2)) >> 11;
  int32_t temp_val_2 = (((((temp_raw >> 4) - ((int32_t)Calibration.TEMP_1)) * ((temp_raw >> 4) - ((int32_t)Calibration.TEMP_1))) >> 12) * ((int32_t)Calibration.TEMP_3)) >> 14;
  temp_raw = temp_val_1 + temp_val_2; //цельночисленная температура
  uint16_t temp = (temp_raw * 5 + 128) >> 8; //записываем знаковую температуру в беззнаковую переменную
  tempSens.temp = (temp > 8500) ? 0 : temp;


  int64_t press_val_1 = ((int64_t)temp_raw) - 128000; //компенсация температуры
  int64_t press_val_2 = press_val_1 * press_val_1 * (int64_t)Calibration.PRESS_6;
  press_val_2 = press_val_2 + ((press_val_1 * (int64_t)Calibration.PRESS_5) << 17);
  press_val_2 = press_val_2 + (((int64_t)Calibration.PRESS_4) << 35);
  press_val_1 = ((press_val_1 * press_val_1 * (int64_t)Calibration.PRESS_3) >> 8) + ((press_val_1 * (int64_t)Calibration.PRESS_2) << 12);
  press_val_1 = (((((int64_t)1) << 47) + press_val_1)) * ((int64_t)Calibration.PRESS_1) >> 33;
  if (press_val_1) { //если значение не нулевое
    int64_t press_val_3 = 1048576 - press_raw;
    press_val_3 = (((press_val_3 << 31) - press_val_2) * 3125) / press_val_1;
    press_val_1 = (((int64_t)Calibration.PRESS_9) * (press_val_3 >> 13) * (press_val_3 >> 13)) >> 25;
    press_val_2 = (((int64_t)Calibration.PRESS_8) * press_val_3) >> 19;
    press_val_3 = ((press_val_3 + press_val_1 + press_val_2) >> 8) + (((int64_t)Calibration.PRESS_7) << 4);

    tempSens.press = ((float)press_val_3 / 256.0) * 0.00750062; //записываем давление в мм рт.ст.
  }
  else tempSens.press = 0; //иначе записываем 0


  int32_t hum_val  = (temp_raw - ((int32_t)76800)); //компенсация температуры
  hum_val = (((((hum_raw << 14) - (((int32_t)Calibration.HUM_4) << 20) - (((int32_t)Calibration.HUM_5) * hum_val)) +
               ((int32_t)16384)) >> 15) * (((((((hum_val * ((int32_t)Calibration.HUM_6)) >> 10) * (((hum_val * ((int32_t)Calibration.HUM_3)) >> 11) +
                   ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)Calibration.HUM_2) + 8192) >> 14));
  hum_val = (hum_val - (((((hum_val >> 15) * (hum_val >> 15)) >> 7) * ((int32_t)Calibration.HUM_1)) >> 4));
  hum_val = (hum_val < 0) ? 0 : hum_val;
  hum_val = (hum_val > 419430400) ? 419430400 : hum_val;

  tempSens.hum = (hum_val >> 12) / 1024.0; //записываем влажность в %
}
