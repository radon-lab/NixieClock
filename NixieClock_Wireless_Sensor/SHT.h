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
void readTempSHT(void) //чтение температуры/влажности
{
  static uint8_t typeSHT; //тип датчика SHT
  uint16_t timer = (uint16_t)millis(); //установили таймер

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
    else return; //иначе выходим
  }

  switch (typeSHT) { //чтение данных
    case SHT20_ADDR: {
        if (twi_beginTransmission(SHT20_ADDR)) return; //начало передачи
        twi_write_byte(SHT20_READ_TEMP); //устанавливаем адрес записи
        twi_write_stop(); //остановка шины wire

        while (twi_beginTransmission(SHT20_ADDR, 1)) { //ждем окончания преобразования
          yield(); //обработка данных
          if (((uint16_t)millis() - timer) >= SHT20_TEMP_TIME) return; //выходим если таймаут
        }
        uint16_t temp_raw = ((uint16_t)twi_read_byte(TWI_ACK) << 8) | (twi_read_byte(TWI_ACK) & 0xFC);
        twi_read_byte(TWI_NACK); //пропускаем контрольную сумму
        if (sens.temp == 0x7FFF) sens.temp = (uint16_t)(temp_raw * 0.0268) - 468; //рассчитываем температуру

        if (twi_beginTransmission(SHT20_ADDR)) return; //начало передачи
        twi_write_byte(SHT20_READ_HUM); //устанавливаем адрес записи
        twi_write_stop(); //остановка шины wire

        timer = (uint16_t)millis(); //установили таймер
        while (twi_beginTransmission(SHT20_ADDR, 1)) { //ждем окончания преобразования
          yield(); //обработка данных
          if (((uint16_t)millis() - timer) >= SHT20_HUM_TIME) return; //выходим если таймаут
        }
        uint16_t hum_raw = ((uint16_t)twi_read_byte(TWI_ACK) << 8) | (twi_read_byte(TWI_ACK) & 0xFC);
        twi_read_byte(TWI_NACK); //пропускаем контрольную сумму
        if (!sens.hum) sens.hum = (uint16_t)(hum_raw * 0.0019) - 6; //рассчитываем влажность
      }
      break;
    case SHT30_ADDR: {
        if (twi_beginTransmission(SHT30_ADDR)) return; //начало передачи
        twi_write_byte(SHT30_READ_DATA); //устанавливаем адрес записи
        twi_write_byte(SHT30_RESOLUTION); //записываем настройку
        twi_write_stop(); //остановка шины wire

        while (twi_beginTransmission(SHT30_ADDR, 1)) { //ждем окончания преобразования
          yield(); //обработка данных
          if (((uint16_t)millis() - timer) >= SHT30_MEAS_TIME) return; //выходим если таймаут
        }
        uint16_t temp_raw = ((uint16_t)twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_ACK);
        twi_read_byte(TWI_ACK); //пропускаем контрольную сумму
        uint16_t hum_raw = ((uint16_t)twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_ACK);
        twi_read_byte(TWI_NACK); //пропускаем контрольную сумму
        if (sens.temp == 0x7FFF) sens.temp = (uint16_t)(temp_raw * 0.0267) - 450;  //рассчитываем температуру
        if (!sens.hum) sens.hum = hum_raw * 0.00152; //рассчитываем влажность
      }
      break;
  }

  settings.sensor |= (0x01 << SENS_SHT); //установили флаг сенсора
}
