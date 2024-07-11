#define BMP180_ADDR 0x77 //адрес датчика

#define BMP180_CONTROL_REG 0xF4 //регистр настройки замеров
#define BMP180_DATA_OUT_REG 0xF6 //регистр чтения данных

#define BMP180_READ_TEMP 0x2E //запрос температуры
#define BMP180_READ_PRESS 0x34 //запрос давления

#define BMP180_OVERSAMP 0x03  //разрешение датчика давления

#define BME280_ADDR 0x76 //адрес датчика

#define BME280_DATA_OUT_REG 0xF7 //регистр чтения данных
#define BME280_STATUS_REG 0xF3 //регистр статуса
#define BME280_CONFIG_REG 0xF5 //регистр основных настроек
#define BME280_CONTROL_HUM_REG 0xF2 //регистр настройки влажности
#define BME280_CONTROL_MEAS_REG 0xF4 //регистр настройки замеров

#define BME280_MODE 0x02 //режим замера по запросу
#define BME280_STANDBY 0x05 //замер каждую секунду
#define BME280_FILTER_COEF 0x04 //коэффициент фильтрации 16

#define BME280_PRESS_OVERSAMP 0x02 //разрешение датчика давления
#define BME280_TEMP_OVERSAMP 0x03 //разрешение датчика температуры
#define BME280_HUM_OVERSAMP 0x01 //разрешение датчика влажности

#define BME_CHECK_TIMEOUT 50 //таймаут ожидания(50..150)(мс)

//структура калибровок датчика BME280
struct CalibrationDataBME280 {
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
} CalibrationBME;

//структура калибровок датчика BMP180
struct CalibrationDataBMP180 {
  int16_t AC_1;
  int16_t AC_2;
  int16_t AC_3;
  uint16_t AC_4;
  uint16_t AC_5;
  uint16_t AC_6;
  int16_t B_1;
  int16_t B_2;
  int16_t MB;
  int16_t MC;
  int16_t MD;
} CalibrationBMP;

//--------------------------------------Запись одного байта------------------------------------------
void writeREG(uint8_t _addr, uint8_t _reg, uint8_t _data) //Запись одного байта
{
  if (wireBeginTransmission(_addr)) return; //начало передачи
  wireWrite(_reg); //устанавливаем адрес записи
  wireWrite(_data); //записываем байт
  wireEnd(); //конец передачи
}
//--------------------------------------Чтение калибровок датчика------------------------------------------
boolean readCalibrationBMP180(void) //чтение калибровок датчика
{
  if (wireRequestFrom(BMP180_ADDR, 0xAA)) return 0; //запрашиваем чтение данных, если нет ответа выходим

  CalibrationBMP.AC_1 = ((wireRead() << 8) | wireRead()); //читаем
  CalibrationBMP.AC_2 = ((wireRead() << 8) | wireRead());
  CalibrationBMP.AC_3 = ((wireRead() << 8) | wireRead());
  CalibrationBMP.AC_4 = ((wireRead() << 8) | wireRead());
  CalibrationBMP.AC_5 = ((wireRead() << 8) | wireRead());
  CalibrationBMP.AC_6 = ((wireRead() << 8) | wireRead());
  CalibrationBMP.B_1 = ((wireRead() << 8) | wireRead());
  CalibrationBMP.B_2 = ((wireRead() << 8) | wireRead());
  CalibrationBMP.MB = ((wireRead() << 8) | wireRead());
  CalibrationBMP.MC = ((wireRead() << 8) | wireRead());
  CalibrationBMP.MD = ((wireRead() << 8) | wireReadEndByte());

  return 1;
}
//--------------------------------------Чтение калибровок датчика------------------------------------------
boolean readCalibrationBME280(void) //чтение калибровок датчика
{
  if (wireRequestFrom(BME280_ADDR, 0x88)) return 0; //запрашиваем чтение данных, если нет ответа выходим

  CalibrationBME.TEMP_1 = (wireRead() | (wireRead() << 8)); //читаем
  CalibrationBME.TEMP_2 = (wireRead() | (wireRead() << 8));
  CalibrationBME.TEMP_3 = (wireRead() | (wireRead() << 8));
  CalibrationBME.PRESS_1 = (wireRead() | (wireRead() << 8));
  CalibrationBME.PRESS_2 = (wireRead() | (wireRead() << 8));
  CalibrationBME.PRESS_3 = (wireRead() | (wireRead() << 8));
  CalibrationBME.PRESS_4 = (wireRead() | (wireRead() << 8));
  CalibrationBME.PRESS_5 = (wireRead() | (wireRead() << 8));
  CalibrationBME.PRESS_6 = (wireRead() | (wireRead() << 8));
  CalibrationBME.PRESS_7 = (wireRead() | (wireRead() << 8));
  CalibrationBME.PRESS_8 = (wireRead() | (wireRead() << 8));
  CalibrationBME.PRESS_9 = (wireRead() | (wireRead() << 8));
  CalibrationBME.HUM_1 = wireReadEndByte();

  if (wireRequestFrom(BME280_ADDR, 0xE1)) return 0; //запрашиваем чтение данных, если нет ответа выходим

  CalibrationBME.HUM_2 = (wireRead() | (wireRead() << 8)); //читаем
  CalibrationBME.HUM_3 = wireRead();
  CalibrationBME.HUM_4 = (wireRead() << 4);
  uint8_t interVal = wireRead();
  CalibrationBME.HUM_4 |= (interVal & 0x0F);
  CalibrationBME.HUM_5 = (((interVal & 0xF0) >> 4) | (wireRead() << 4));
  CalibrationBME.HUM_6 = wireReadEndByte();

  return 1;
}
//--------------------------------------Чтение температуры/давления/влажности------------------------------------------
void readTempBME(void) //чтение температуры/давления/влажности
{
  static uint8_t typeBME; //тип датчика BME

  if (!typeBME) { //если датчик не определен
#if SENS_BME_ENABLE == 1
    if (readCalibrationBMP180()) typeBME = BMP180_ADDR; //установлили тип датчика
#elif SENS_BME_ENABLE == 2
    if (readCalibrationBME280()) { //если BME280
      typeBME = BME280_ADDR; //установлили тип датчика
      writeREG(BME280_ADDR, BME280_CONTROL_HUM_REG, BME280_HUM_OVERSAMP); //устанавливаем разрешение датчика влажности
      writeREG(BME280_ADDR, BME280_CONTROL_MEAS_REG, ((BME280_TEMP_OVERSAMP << 5) | (BME280_PRESS_OVERSAMP << 2) | BME280_MODE)); //устанавливаем разрешение датчика температуры и датчика давления, устанавливаем режим работы
      writeREG(BME280_ADDR, BME280_CONFIG_REG, ((BME280_STANDBY << 5) | (BME280_FILTER_COEF << 2))); //устанавливаем частоту опроса и коэффициент фильтрации
    }
#else
    if (readCalibrationBMP180()) typeBME = BMP180_ADDR; //установлили тип датчика
    else if (readCalibrationBME280()) { //если BME280
      typeBME = BME280_ADDR; //установлили тип датчика
      writeREG(BME280_ADDR, BME280_CONTROL_HUM_REG, BME280_HUM_OVERSAMP); //устанавливаем разрешение датчика влажности
      writeREG(BME280_ADDR, BME280_CONTROL_MEAS_REG, ((BME280_TEMP_OVERSAMP << 5) | (BME280_PRESS_OVERSAMP << 2) | BME280_MODE)); //устанавливаем разрешение датчика температуры и датчика давления, устанавливаем режим работы
      writeREG(BME280_ADDR, BME280_CONFIG_REG, ((BME280_STANDBY << 5) | (BME280_FILTER_COEF << 2))); //устанавливаем частоту опроса и коэффициент фильтрации
    }
#endif
    else return; //иначе выходим
  }

  switch (typeBME) { //чтение данных
#if (SENS_BME_ENABLE == 1) || (SENS_BME_ENABLE != 2)
    case BMP180_ADDR: {
        int32_t temp_raw; //регистр температуры
        int32_t press_raw; //регистр давления

        _timer_ms[TMR_SENS] = BME_CHECK_TIMEOUT; //установили таймаут

        writeREG(BMP180_ADDR, BMP180_CONTROL_REG, BMP180_READ_TEMP); //запустили замер температуры
        while (1) { //ожидаем окончания замера
          systemTask(); //обработка данных
          if (wireRequestFrom(BMP180_ADDR, BMP180_CONTROL_REG)) return; //запрашиваем чтение данных, если нет ответа выходим
          if (!(wireReadEndByte() & 0x20)) break; //если замер завершён продолжаем
          if (!_timer_ms[TMR_SENS]) return; //выходим если таймаут
        }

        if (wireRequestFrom(BMP180_ADDR, BMP180_DATA_OUT_REG)) return; //запрашиваем чтение данных, если нет ответа выходим
        temp_raw = (((uint16_t)wireRead() << 8) | wireReadEndByte());

        _timer_ms[TMR_SENS] = BME_CHECK_TIMEOUT; //установили таймаут

        writeREG(BMP180_ADDR, BMP180_CONTROL_REG, BMP180_READ_PRESS | (BMP180_OVERSAMP << 6)); //запустили замер давления
        while (1) { //ожидаем окончания замера
          systemTask(); //обработка данных
          if (wireRequestFrom(BMP180_ADDR, BMP180_CONTROL_REG)) return; //запрашиваем чтение данных, если нет ответа выходим
          if (!(wireReadEndByte() & 0x20)) break; //если замер завершён продолжаем
          if (!_timer_ms[TMR_SENS]) return; //выходим если таймаут
        }

        if (wireRequestFrom(BMP180_ADDR, BMP180_DATA_OUT_REG)) return; //запрашиваем чтение данных, если нет ответа выходим
        press_raw = (((uint32_t)wireRead() << 16) | ((uint32_t)wireRead() << 8) | wireReadEndByte());
        press_raw >>= (8 - BMP180_OVERSAMP);

        int32_t temp_val_1 = ((int32_t)(temp_raw - CalibrationBMP.AC_6) * CalibrationBMP.AC_5) >> 15;
        int32_t temp_val_2 = ((int32_t)CalibrationBMP.MC << 11) / (temp_val_1 + CalibrationBMP.MD);
        temp_raw = temp_val_1 + temp_val_2;

        int32_t press_val_4 = temp_raw - 4000;
        temp_val_1 = ((int32_t)CalibrationBMP.B_2 * ((press_val_4 * press_val_4) >> 12)) >> 11;
        temp_val_2 = ((int32_t)CalibrationBMP.AC_2 * press_val_4) >> 11;
        int32_t press_val_1 = temp_val_1 + temp_val_2;
        int32_t press_val_2 = ((((int32_t)CalibrationBMP.AC_1 * 4 + press_val_1) << BMP180_OVERSAMP) + 2) >> 2;
        temp_val_1 = ((int32_t)CalibrationBMP.AC_3 * press_val_4) >> 13;
        temp_val_2 = ((int32_t)CalibrationBMP.B_1 * ((press_val_4 * press_val_4) >> 12)) >> 16;
        press_val_1 = ((temp_val_1 + temp_val_2) + 2) >> 2;
        uint32_t press_val_3 = ((uint32_t)CalibrationBMP.AC_4 * (uint32_t)(press_val_1 + 32768)) >> 15;
        uint32_t press_val_5 = ((uint32_t)press_raw - press_val_2) * (50000 >> BMP180_OVERSAMP);
        if (press_val_5 < 0x80000000) press_raw = (press_val_5 * 2) / press_val_3;
        else press_raw = (press_val_5 / press_val_3) * 2;
        temp_val_1 = (press_raw >> 8) * (press_raw >> 8);
        temp_val_1 = (temp_val_1 * 3038) >> 16;
        temp_val_2 = (-7357 * (press_raw)) >> 16;

        sens.temp = (temp_raw + 8) >> 4; //установили температуру
        sens.press = (press_raw + ((temp_val_1 + temp_val_2 + 3791) >> 4)) * 0.00750062; //записываем давление в мм рт.ст.
        sens.hum = 0; //сбросили влажность
      }
      break;
#endif
#if (SENS_BME_ENABLE == 2) || (SENS_BME_ENABLE != 1)
    case BME280_ADDR: {
        _timer_ms[TMR_SENS] = BME_CHECK_TIMEOUT; //установили таймаут

        writeREG(BME280_ADDR, BME280_CONTROL_MEAS_REG, ((BME280_TEMP_OVERSAMP << 5) | (BME280_PRESS_OVERSAMP << 2) | BME280_MODE)); //устанавливаем разрешение датчика температуры и датчика давления, устанавливаем режим работы
        while (1) { //ожидаем окончания замера
          systemTask(); //обработка данных
          if (wireRequestFrom(BME280_ADDR, BME280_STATUS_REG)) return; //запрашиваем чтение данных, если нет ответа выходим
          if (!(wireReadEndByte() & 0x08)) break; //если замер завершён продолжаем
          if (!_timer_ms[TMR_SENS]) return; //выходим если таймаут
        }
        if (wireRequestFrom(BME280_ADDR, BME280_DATA_OUT_REG)) return; //запрашиваем чтение данных, если нет ответа выходим

        uint32_t press_raw = (((uint32_t)wireRead() << 16) | ((uint32_t)wireRead() << 8) | (uint32_t)wireRead()) >> 4; //читаем 24-х битное значение ацп давления
        int32_t temp_raw = (((uint32_t)wireRead() << 16) | ((uint32_t)wireRead() << 8) | (uint32_t)wireRead()) >> 4; //читаем 24-х битное значение ацп температуры
        int32_t hum_raw = ((uint16_t)wireRead() << 8) | (uint16_t)wireReadEndByte(); //читаем 16-и битное значение ацп влажности

        int32_t temp_val_1 = ((((temp_raw >> 3) - ((int32_t)CalibrationBME.TEMP_1 << 1))) * ((int32_t)CalibrationBME.TEMP_2)) >> 11;
        int32_t temp_val_2 = (((((temp_raw >> 4) - ((int32_t)CalibrationBME.TEMP_1)) * ((temp_raw >> 4) - ((int32_t)CalibrationBME.TEMP_1))) >> 12) * ((int32_t)CalibrationBME.TEMP_3)) >> 14;
        temp_raw = temp_val_1 + temp_val_2; //цельночисленная температура

        sens.temp = ((temp_raw * 5 + 128) >> 8) / 10; //установили температуру

        int32_t press_val_1 = (temp_raw >> 1) - 64000L; //компенсация температуры
        int32_t press_val_2 = ((press_val_1 >> 2) * (press_val_1 >> 2) >> 11) * (int32_t)CalibrationBME.PRESS_6;
        press_val_2 = press_val_2 + ((press_val_1 * (int32_t)CalibrationBME.PRESS_5) << 1);
        press_val_2 = (press_val_2 >> 2) + (((int32_t)CalibrationBME.PRESS_4) << 16);
        press_val_1 = (((CalibrationBME.PRESS_3 * (((press_val_1 >> 2) * (press_val_1 >> 2)) >> 13 )) >> 3) + ((((int32_t)CalibrationBME.PRESS_2) * press_val_1) >> 1)) >> 18;
        press_val_1 = ((((32768 + press_val_1)) * ((int32_t)CalibrationBME.PRESS_1)) >> 15);
        if (press_val_1) { //если значение не нулевое
          press_raw = (((uint32_t)(((int32_t)1048576) - press_raw) - (press_val_2 >> 12))) * 3125;
          if (press_raw < 0x80000000) press_raw = (press_raw << 1) / ((uint32_t)press_val_1);
          else press_raw = (press_raw / (uint32_t)press_val_1) * 2;
          press_val_1 = (((int32_t)CalibrationBME.PRESS_9) * ((int32_t)(((press_raw >> 3) * (press_raw >> 3)) >> 13))) >> 12;
          press_val_2 = (((int32_t)(press_raw >> 2)) * ((int32_t)CalibrationBME.PRESS_8)) >> 13;

          sens.press = (uint32_t)((int32_t)press_raw + ((press_val_1 + press_val_2 + CalibrationBME.PRESS_7) >> 4)) * 0.00750062; //записываем давление в мм рт.ст.
        }
        else sens.press = 0; //иначе записываем 0

        int32_t hum_val  = (temp_raw - ((int32_t)76800)); //компенсация температуры
        hum_val = (((((hum_raw << 14) - (((int32_t)CalibrationBME.HUM_4) << 20) - (((int32_t)CalibrationBME.HUM_5) * hum_val)) +
                     ((int32_t)16384)) >> 15) * (((((((hum_val * ((int32_t)CalibrationBME.HUM_6)) >> 10) * (((hum_val * ((int32_t)CalibrationBME.HUM_3)) >> 11) +
                         ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)CalibrationBME.HUM_2) + 8192) >> 14));
        hum_val = (hum_val - (((((hum_val >> 15) * (hum_val >> 15)) >> 7) * ((int32_t)CalibrationBME.HUM_1)) >> 4));
        hum_val = (hum_val < 0) ? 0 : hum_val;
        hum_val = (hum_val > 419430400) ? 419430400 : hum_val;

        sens.hum = (hum_val >> 12) / 1024.0; //записываем влажность в %
      }
      break;
#endif
  }
  if ((uint16_t)sens.temp > 850) sens.temp = 0; //если вышли за предел
  sens.err = 0; //сбросили ошибку датчика температуры
}
