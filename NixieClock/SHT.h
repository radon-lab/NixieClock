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

  if (!typeSHT) { //если датчик не определен
#if SENS_SHT_ENABLE == 1
    if (!wireBeginTransmission(SHT20_ADDR)) { //если SHT20
      typeSHT = SHT20_ADDR; //установлили тип датчика
      wireWrite(SHT20_WRITE_REG); //устанавливаем адрес записи
      wireWrite(0xC1); //записываем настройку
      wireEnd(); //остановка шины wire
    }
#elif SENS_SHT_ENABLE == 2
    if (!wireBeginTransmission(SHT30_ADDR)) { //если SHT30
      typeSHT = SHT30_ADDR; //установлили тип датчика
      wireWrite(0x30); //записываем настройку
      wireWrite(0x66); //записываем настройку
      wireEnd(); //остановка шины wire
    }
#else
    if (!wireBeginTransmission(SHT20_ADDR)) { //если SHT20
      typeSHT = SHT20_ADDR; //установлили тип датчика
      wireWrite(SHT20_WRITE_REG); //устанавливаем адрес записи
      wireWrite(0xC1); //записываем настройку
      wireEnd(); //остановка шины wire
    }
    else if (!wireBeginTransmission(SHT30_ADDR)) { //если SHT30
      typeSHT = SHT30_ADDR; //установлили тип датчика
      wireWrite(0x30); //записываем настройку
      wireWrite(0x66); //записываем настройку
      wireEnd(); //остановка шины wire
    }
#endif
    else return; //иначе выходим
  }

  uint16_t temp_raw = 0; //буфер расчета температуры
  uint16_t hum_raw = 0; //буфер расчета влажности

  switch (typeSHT) { //чтение данных
#if (SENS_SHT_ENABLE == 1) || (SENS_SHT_ENABLE != 2)
    case SHT20_ADDR: {
        if (wireBeginTransmission(SHT20_ADDR)) return; //начало передачи
        wireWrite(SHT20_READ_TEMP); //устанавливаем адрес записи
        wireEnd(); //остановка шины wire

        _timer_ms[TMR_SENS] = SHT20_TEMP_TIME; //установили таймер
        while (wireBeginTransmission(SHT20_ADDR, 1)) { //ждем окончания преобразования
          systemTask(); //обработка данных
          if (!_timer_ms[TMR_SENS]) return; //выходим если таймаут
        }
        temp_raw = ((uint16_t)wireRead() << 8) | (wireRead() & 0xFC);
        wireReadEndByte(); //пропускаем контрольную сумму
        
        temp_raw = (uint16_t)(temp_raw * 0.0268) - 468; //рассчитываем температуру

        if (wireBeginTransmission(SHT20_ADDR)) return; //начало передачи
        wireWrite(SHT20_READ_HUM); //устанавливаем адрес записи
        wireEnd(); //остановка шины wire

        _timer_ms[TMR_SENS] = SHT20_HUM_TIME; //установили таймер
        while (wireBeginTransmission(SHT20_ADDR, 1)) { //ждем окончания преобразования
          systemTask(); //обработка данных
          if (!_timer_ms[TMR_SENS]) return; //выходим если таймаут
        }
        hum_raw = ((uint16_t)wireRead() << 8) | (wireRead() & 0xFC);
        wireReadEndByte(); //пропускаем контрольную сумму
        
        hum_raw = (uint16_t)(hum_raw * 0.0019) - 6; //рассчитываем влажность
      }
      break;
#endif
#if (SENS_SHT_ENABLE == 2) || (SENS_SHT_ENABLE != 1)
    case SHT30_ADDR: {
        if (wireBeginTransmission(SHT30_ADDR)) return; //начало передачи
        wireWrite(SHT30_READ_DATA); //устанавливаем адрес записи
        wireWrite(SHT30_RESOLUTION); //записываем настройку
        wireEnd(); //остановка шины wire

        _timer_ms[TMR_SENS] = SHT30_MEAS_TIME; //установили таймер
        while (wireBeginTransmission(SHT30_ADDR, 1)) { //ждем окончания преобразования
          systemTask(); //обработка данных
          if (!_timer_ms[TMR_SENS]) return; //выходим если таймаут
        }
        temp_raw = ((uint16_t)wireRead() << 8) | wireRead();
        wireRead(); //пропускаем контрольную сумму
        hum_raw = ((uint16_t)wireRead() << 8) | wireRead();
        wireReadEndByte(); //пропускаем контрольную сумму
        
        temp_raw = (uint16_t)(temp_raw * 0.0267) - 450;  //рассчитываем температуру
        hum_raw = hum_raw * 0.00152; //рассчитываем влажность
      }
      break;
#endif
  }

  sens.err = 0; //сбросили ошибку датчика температуры

#if SENS_BME_ENABLE
  if (sens.type & (0x01 << SENS_BME)) {
    sens.temp = (sens.temp + temp_raw) >> 1; //усредняем температуру
    if (sens.hum) sens.hum = (sens.hum + hum_raw) >> 1; //усредняем влажность
    else sens.hum = hum_raw; //иначе копируем влажность
    return; //выходим
  }
  sens.press = 0; //сбрасываем давление
#endif

  sens.temp = temp_raw; //копируем температуру
  sens.hum = hum_raw; //копируем влажность
}
