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
  if (twi_beginTransmission(_addr)) return; //начало передачи
  twi_write_byte(_reg); //устанавливаем адрес записи
  twi_write_byte(_data); //записываем байт
  twi_write_stop(); //конец передачи
}
//--------------------------------------Чтение калибровок датчика------------------------------------------
boolean readCalibrationBMP180(void) //чтение калибровок датчика
{
  if (twi_requestFrom(BMP180_ADDR, 0xAA)) return 0; //запрашиваем чтение данных, если нет ответа выходим

  CalibrationBMP.AC_1 = ((twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_ACK)); //читаем
  CalibrationBMP.AC_2 = ((twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_ACK));
  CalibrationBMP.AC_3 = ((twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_ACK));
  CalibrationBMP.AC_4 = ((twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_ACK));
  CalibrationBMP.AC_5 = ((twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_ACK));
  CalibrationBMP.AC_6 = ((twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_ACK));
  CalibrationBMP.B_1 = ((twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_ACK));
  CalibrationBMP.B_2 = ((twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_ACK));
  CalibrationBMP.MB = ((twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_ACK));
  CalibrationBMP.MC = ((twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_ACK));
  CalibrationBMP.MD = ((twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_NACK));

  return 1;
}
//--------------------------------------Чтение калибровок датчика------------------------------------------
boolean readCalibrationBME280(void) //чтение калибровок датчика
{
  if (twi_requestFrom(BME280_ADDR, 0x88)) return 0; //запрашиваем чтение данных, если нет ответа выходим

  CalibrationBME.TEMP_1 = (twi_read_byte(TWI_ACK) | (twi_read_byte(TWI_ACK) << 8)); //читаем
  CalibrationBME.TEMP_2 = (twi_read_byte(TWI_ACK) | (twi_read_byte(TWI_ACK) << 8));
  CalibrationBME.TEMP_3 = (twi_read_byte(TWI_ACK) | (twi_read_byte(TWI_ACK) << 8));
  CalibrationBME.PRESS_1 = (twi_read_byte(TWI_ACK) | (twi_read_byte(TWI_ACK) << 8));
  CalibrationBME.PRESS_2 = (twi_read_byte(TWI_ACK) | (twi_read_byte(TWI_ACK) << 8));
  CalibrationBME.PRESS_3 = (twi_read_byte(TWI_ACK) | (twi_read_byte(TWI_ACK) << 8));
  CalibrationBME.PRESS_4 = (twi_read_byte(TWI_ACK) | (twi_read_byte(TWI_ACK) << 8));
  CalibrationBME.PRESS_5 = (twi_read_byte(TWI_ACK) | (twi_read_byte(TWI_ACK) << 8));
  CalibrationBME.PRESS_6 = (twi_read_byte(TWI_ACK) | (twi_read_byte(TWI_ACK) << 8));
  CalibrationBME.PRESS_7 = (twi_read_byte(TWI_ACK) | (twi_read_byte(TWI_ACK) << 8));
  CalibrationBME.PRESS_8 = (twi_read_byte(TWI_ACK) | (twi_read_byte(TWI_ACK) << 8));
  CalibrationBME.PRESS_9 = (twi_read_byte(TWI_ACK) | (twi_read_byte(TWI_ACK) << 8));
  CalibrationBME.HUM_1 = twi_read_byte(TWI_NACK);

  if (twi_requestFrom(BME280_ADDR, 0xE1)) return 0; //запрашиваем чтение данных, если нет ответа выходим

  CalibrationBME.HUM_2 = (twi_read_byte(TWI_ACK) | (twi_read_byte(TWI_ACK) << 8)); //читаем
  CalibrationBME.HUM_3 = twi_read_byte(TWI_ACK);
  CalibrationBME.HUM_4 = (twi_read_byte(TWI_ACK) << 4);
  uint8_t interVal = twi_read_byte(TWI_ACK);
  CalibrationBME.HUM_4 |= (interVal & 0x0F);
  CalibrationBME.HUM_5 = (((interVal & 0xF0) >> 4) | (twi_read_byte(TWI_ACK) << 4));
  CalibrationBME.HUM_6 = twi_read_byte(TWI_NACK);

  return 1;
}
//--------------------------------------Чтение температуры/давления/влажности------------------------------------------
uint8_t readTempBME(void) //чтение температуры/давления/влажности
{
  static uint8_t typeBME; //тип датчика BME
  static uint8_t attemptsBME; //попытки запроса датчика BME
  uint16_t timer = (uint16_t)millis(); //установили таймер

  if (sens.type == 3) return 2; //сенсор установлен на стороне часов

  if (attemptsBME < 5) attemptsBME++; //попытка запроса температуры
  else return 2; //иначе выходим

  if (!typeBME) { //если датчик не определен
    if (readCalibrationBMP180()) typeBME = BMP180_ADDR; //установлили тип датчика
    else if (readCalibrationBME280()) { //если BME280
      typeBME = BME280_ADDR; //установлили тип датчика
      writeREG(BME280_ADDR, BME280_CONTROL_HUM_REG, BME280_HUM_OVERSAMP); //устанавливаем разрешение датчика влажности
      writeREG(BME280_ADDR, BME280_CONTROL_MEAS_REG, ((BME280_TEMP_OVERSAMP << 5) | (BME280_PRESS_OVERSAMP << 2) | BME280_MODE)); //устанавливаем разрешение датчика температуры и датчика давления, устанавливаем режим работы
      writeREG(BME280_ADDR, BME280_CONFIG_REG, ((BME280_STANDBY << 5) | (BME280_FILTER_COEF << 2))); //устанавливаем частоту опроса и коэффициент фильтрации
    }
    else return 1; //иначе выходим
  }

  switch (typeBME) { //чтение данных
    case BMP180_ADDR: {
        int32_t temp_raw; //регистр температуры
        int32_t press_raw; //регистр давления

        writeREG(BMP180_ADDR, BMP180_CONTROL_REG, BMP180_READ_TEMP); //запустили замер температуры
        while (1) { //ожидаем окончания замера
          yield(); //обработка данных
          if (twi_requestFrom(BMP180_ADDR, BMP180_CONTROL_REG)) return 1; //запрашиваем чтение данных, если нет ответа выходим
          if (!(twi_read_byte(TWI_NACK) & 0x20)) break; //если замер завершён продолжаем
          if (((uint16_t)millis() - timer) >= BME_CHECK_TIMEOUT) return 1; //выходим если таймаут
        }

        if (twi_requestFrom(BMP180_ADDR, BMP180_DATA_OUT_REG)) return 1; //запрашиваем чтение данных, если нет ответа выходим
        temp_raw = (((uint16_t)twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_NACK));

        timer = (uint16_t)millis(); //установили таймер

        writeREG(BMP180_ADDR, BMP180_CONTROL_REG, BMP180_READ_PRESS | (BMP180_OVERSAMP << 6)); //запустили замер давления
        while (1) { //ожидаем окончания замера
          yield(); //обработка данных
          if (twi_requestFrom(BMP180_ADDR, BMP180_CONTROL_REG)) return 1; //запрашиваем чтение данных, если нет ответа выходим
          if (!(twi_read_byte(TWI_NACK) & 0x20)) break; //если замер завершён продолжаем
          if (((uint16_t)millis() - timer) >= BME_CHECK_TIMEOUT) return 1; //выходим если таймаут
        }

        if (twi_requestFrom(BMP180_ADDR, BMP180_DATA_OUT_REG)) return 1; //запрашиваем чтение данных, если нет ответа выходим
        press_raw = (((uint32_t)twi_read_byte(TWI_ACK) << 16) | ((uint32_t)twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_NACK));
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

        temp_raw = (temp_raw + 8) >> 4; //рассчитываем температуру
        
        if (!sens.status) sens.temp[SENS_MAIN] = temp_raw; //установили температуру
        else sens.temp[SENS_MAIN] = (sens.temp[SENS_MAIN] + temp_raw) / 2; //усредняем температуру
        
        sens.press[SENS_MAIN] = (press_raw + ((temp_val_1 + temp_val_2 + 3791) >> 4)) * 0.00750062; //записываем давление в мм рт.ст.
      }
      break;
    case BME280_ADDR: {
        writeREG(BME280_ADDR, BME280_CONTROL_MEAS_REG, ((BME280_TEMP_OVERSAMP << 5) | (BME280_PRESS_OVERSAMP << 2) | BME280_MODE)); //устанавливаем разрешение датчика температуры и датчика давления, устанавливаем режим работы
        while (1) { //ожидаем окончания замера
          yield(); //обработка данных
          if (twi_requestFrom(BME280_ADDR, BME280_STATUS_REG)) return 1; //запрашиваем чтение данных, если нет ответа выходим
          if (!(twi_read_byte(TWI_NACK) & 0x08)) break; //если замер завершён продолжаем
          if (((uint16_t)millis() - timer) >= BME_CHECK_TIMEOUT) return 1; //выходим если таймаут
        }
        if (twi_requestFrom(BME280_ADDR, BME280_DATA_OUT_REG)) return 1; //запрашиваем чтение данных, если нет ответа выходим

        uint32_t press_raw = (((uint32_t)twi_read_byte(TWI_ACK) << 16) | ((uint32_t)twi_read_byte(TWI_ACK) << 8) | (uint32_t)twi_read_byte(TWI_ACK)) >> 4; //читаем 24-х битное значение ацп давления
        int32_t temp_raw = (((uint32_t)twi_read_byte(TWI_ACK) << 16) | ((uint32_t)twi_read_byte(TWI_ACK) << 8) | (uint32_t)twi_read_byte(TWI_ACK)) >> 4; //читаем 24-х битное значение ацп температуры
        int32_t hum_raw = ((uint16_t)twi_read_byte(TWI_ACK) << 8) | (uint16_t)twi_read_byte(TWI_NACK); //читаем 16-и битное значение ацп влажности

        int32_t temp_val_1 = ((((temp_raw >> 3) - ((int32_t)CalibrationBME.TEMP_1 << 1))) * ((int32_t)CalibrationBME.TEMP_2)) >> 11;
        int32_t temp_val_2 = (((((temp_raw >> 4) - ((int32_t)CalibrationBME.TEMP_1)) * ((temp_raw >> 4) - ((int32_t)CalibrationBME.TEMP_1))) >> 12) * ((int32_t)CalibrationBME.TEMP_3)) >> 14;
        temp_raw = temp_val_1 + temp_val_2; //цельночисленная температура
        temp_raw = ((temp_raw * 5 + 128) >> 8) / 10; //рассчитываем температуру

        if (!sens.status) sens.temp[SENS_MAIN] = temp_raw; //установили температуру
        else sens.temp[SENS_MAIN] = (sens.temp[SENS_MAIN] + temp_raw) / 2; //усредняем температуру

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

          sens.press[SENS_MAIN] = (uint32_t)((int32_t)press_raw + ((press_val_1 + press_val_2 + CalibrationBME.PRESS_7) >> 4)) * 0.00750062; //записываем давление в мм рт.ст.
        }
        else sens.press[SENS_MAIN] = 0; //иначе записываем 0

        int32_t hum_val  = (temp_raw - ((int32_t)76800)); //компенсация температуры
        hum_val = (((((hum_raw << 14) - (((int32_t)CalibrationBME.HUM_4) << 20) - (((int32_t)CalibrationBME.HUM_5) * hum_val)) +
                     ((int32_t)16384)) >> 15) * (((((((hum_val * ((int32_t)CalibrationBME.HUM_6)) >> 10) * (((hum_val * ((int32_t)CalibrationBME.HUM_3)) >> 11) +
                         ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)CalibrationBME.HUM_2) + 8192) >> 14));
        hum_val = (hum_val - (((((hum_val >> 15) * (hum_val >> 15)) >> 7) * ((int32_t)CalibrationBME.HUM_1)) >> 4));
        hum_val = (hum_val < 0) ? 0 : hum_val;
        hum_val = (hum_val > 419430400) ? 419430400 : hum_val;

        hum_val = (hum_val >> 12) / 1024.0; //записываем влажность в %

        if (!sens.status) sens.hum[SENS_MAIN] = hum_val; //установили влажность
        else if (hum_val) sens.hum[SENS_MAIN] = (sens.hum[SENS_MAIN] + hum_val) / 2; //усредняем влажность
      }
      break;
  }

  if ((uint16_t)sens.temp[SENS_MAIN] > 850) sens.temp[SENS_MAIN] = 0; //если вышли за предел
  if (sens.hum[SENS_MAIN] > 99) sens.hum[SENS_MAIN] = 99; //если вышли за предел

  sens.status |= SENS_BME; //установили флаг
  attemptsBME = 0; //сбросили попытки запроса
  return 0; //выходим
}
