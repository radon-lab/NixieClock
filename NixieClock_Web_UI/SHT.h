#define SHT20_ADDR 0x40 //адрес датчика

#define SHT20_WRITE_REG 0xE6 //регистр настроек и статуса

#define SHT20_READ_TEMP 0xF3 //начать замер температуры
#define SHT20_READ_HUM 0xF5 //начать замер влажности

#define SHT20_TEMP_TIME 15 //таймаут ожидания замера температуры
#define SHT20_HUM_TIME 20 //таймаут ожидания замера влажности

#define SHT30_ADDR 0x44 //адрес датчика

#define SHT30_READ_DATA 0x24 //начать замер температуры и влажности
#define SHT30_RESOLUTION 0x00 //установка точности измерения

#define SHT30_MEAS_TIME 20 //таймаут ожидания замера

//--------------------------------------Чтение температуры/влажности------------------------------------------
uint8_t readTempSHT(void) //чтение температуры/влажности
{
  static uint8_t typeSHT; //тип датчика SHT
  static uint8_t attemptsSHT; //попытки запроса датчика SHT
  uint16_t timer = (uint16_t)millis(); //установили таймер

  if (sens.type == 2) return 2; //сенсор установлен на стороне часов

  if (attemptsSHT < 5) attemptsSHT++; //попытка запроса температуры
  else return 2; //иначе выходим

  if (!typeSHT) { //если датчик не определен
    if (!twi_beginTransmission(SHT20_ADDR)) { //если SHT20
      typeSHT = SHT20_ADDR; //установлили тип датчика
      twi_write_byte(SHT20_WRITE_REG); //устанавливаем адрес записи
      twi_write_byte(0xC1); //записываем настройку
      twi_write_stop(); //остановка шины wire
    }
    else if (!twi_beginTransmission(SHT30_ADDR)) { //если SHT30
      typeSHT = SHT30_ADDR; //установлили тип датчика
      twi_write_byte(0x30); //записываем настройку
      twi_write_byte(0x66); //записываем настройку
      twi_write_stop(); //остановка шины wire
    }
    else return 1; //иначе выходим
  }

  uint16_t temp_raw = 0; //буфер для рассчета температуры
  uint16_t hum_raw = 0; //буфер для рассчета влажности

  switch (typeSHT) { //чтение данных
    case SHT20_ADDR: {
        if (twi_beginTransmission(SHT20_ADDR)) return 1; //начало передачи
        twi_write_byte(SHT20_READ_TEMP); //устанавливаем адрес записи
        twi_write_stop(); //остановка шины wire

        while (twi_beginTransmission(SHT20_ADDR, 1)) { //ждем окончания преобразования
          yield(); //обработка данных
          if (((uint16_t)millis() - timer) >= SHT20_TEMP_TIME) return 1; //выходим если таймаут
        }
        temp_raw = ((uint16_t)twi_read_byte(TWI_ACK) << 8) | (twi_read_byte(TWI_ACK) & 0xFC);
        twi_read_byte(TWI_NACK); //пропускаем контрольную сумму
        temp_raw = (uint16_t)(temp_raw * 0.0268) - 468; //рассчитываем температуру

        if (twi_beginTransmission(SHT20_ADDR)) return 1; //начало передачи
        twi_write_byte(SHT20_READ_HUM); //устанавливаем адрес записи
        twi_write_stop(); //остановка шины wire

        timer = (uint16_t)millis(); //установили таймер
        while (twi_beginTransmission(SHT20_ADDR, 1)) { //ждем окончания преобразования
          yield(); //обработка данных
          if (((uint16_t)millis() - timer) >= SHT20_HUM_TIME) return 1; //выходим если таймаут
        }
        hum_raw = ((uint16_t)twi_read_byte(TWI_ACK) << 8) | (twi_read_byte(TWI_ACK) & 0xFC);
        twi_read_byte(TWI_NACK); //пропускаем контрольную сумму
        hum_raw = (uint16_t)(hum_raw * 0.0019) - 6; //рассчитываем влажность
      }
      break;
    case SHT30_ADDR: {
        if (twi_beginTransmission(SHT30_ADDR)) return 1; //начало передачи
        twi_write_byte(SHT30_READ_DATA); //устанавливаем адрес записи
        twi_write_byte(SHT30_RESOLUTION); //записываем настройку
        twi_write_stop(); //остановка шины wire

        while (twi_beginTransmission(SHT30_ADDR, 1)) { //ждем окончания преобразования
          yield(); //обработка данных
          if (((uint16_t)millis() - timer) >= SHT30_MEAS_TIME) return 1; //выходим если таймаут
        }
        temp_raw = ((uint16_t)twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_ACK);
        twi_read_byte(TWI_ACK); //пропускаем контрольную сумму
        hum_raw = ((uint16_t)twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_ACK);
        twi_read_byte(TWI_NACK); //пропускаем контрольную сумму
        
        temp_raw = (uint16_t)(temp_raw * 0.0267) - 450;  //рассчитываем температуру
        hum_raw = hum_raw * 0.00152; //рассчитываем влажность
      }
      break;
  }

  if (!sens.status) sens.temp[SENS_MAIN] = temp_raw; //записываем температуру
  else sens.temp[SENS_MAIN] = (sens.temp[SENS_MAIN] + temp_raw) / 2; //усредняем температуру
  
  if (!sens.status) sens.hum[SENS_MAIN] = hum_raw; //записываем влажность
  else if (hum_raw) sens.hum[SENS_MAIN] = (sens.hum[SENS_MAIN] + hum_raw) / 2; //усредняем влажность

  if ((uint16_t)sens.temp[SENS_MAIN] > 850) sens.temp[SENS_MAIN] = 0; //если вышли за предел
  if (sens.hum[SENS_MAIN] > 99) sens.hum[SENS_MAIN] = 99; //если вышли за предел

  sens.status |= SENS_SHT; //установили флаг
  attemptsSHT = 0; //сбросили попытки запроса
  return 0; //выходим
}
