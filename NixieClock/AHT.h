#define AHT10_ADDR 0x38 //адрес датчика

#define AHT10_MEASURE_START 0xAC
#define AHT10_MEASURE_DATA 0x33

#define AHT10_CALIBRATE_START 0xE1
#define AHT10_CALIBRATE_DATA 0x08

#define AHT10_NORMAL_MODE 0xA8
#define AHT10_NOP_DATA 0x00

#define AHT10_CALIBRATE_TIME 350 //таймаут ожидания завершения калибровки
#define AHT10_MEASURE_TIME 100 //таймаут ожидания завершения замера

//--------------------------------------Чтение температуры/влажности------------------------------------------
void readTempAHT(void) //чтение температуры/влажности
{
  static uint8_t typeAHT; //тип датчика AHT

  if (!typeAHT) { //если датчик не определен
    if (!wireBeginTransmission(AHT10_ADDR)) { //если AHT10
      wireWrite(AHT10_CALIBRATE_START); //начинаем калибровку для AHT10
      wireWrite(AHT10_CALIBRATE_DATA); //записываем настройку
      wireWrite(AHT10_NOP_DATA); //записываем настройку
      wireEnd(); //остановка шины wire

      _timer_ms[TMR_SENS] = AHT10_CALIBRATE_TIME; //установили таймер
      while (1) { //ждем окончания калибровки
        systemTask(); //обработка данных
        if (wireBeginTransmission(AHT10_ADDR, 1)) return; //запрашиваем чтение данных
        if ((wireReadEndByte() & 0x88) == 0x08) break;
        if (!_timer_ms[TMR_SENS]) return; //выходим если таймаут
      }
      typeAHT = AHT10_ADDR; //установлили тип датчика
    }
    else return; //иначе выходим
  }

  if (wireBeginTransmission(AHT10_ADDR)) return; //начало передачи
  wireWrite(AHT10_MEASURE_START); //записываем настройку
  wireWrite(AHT10_MEASURE_DATA); //записываем настройку
  wireWrite(AHT10_NOP_DATA); //записываем настройку
  wireEnd(); //остановка шины wire

  _timer_ms[TMR_SENS] = AHT10_MEASURE_TIME; //установили таймер
  while (1) { //ждем окончания калибровки
    systemTask(); //обработка данных
    if (wireBeginTransmission(AHT10_ADDR, 1)) return; //запрашиваем чтение данных
    if (!(wireReadEndByte() & 0x80)) break;
    if (!_timer_ms[TMR_SENS]) return; //выходим если таймаут
  }

  if (wireBeginTransmission(AHT10_ADDR, 1)) return; //запрашиваем чтение данных
  wireRead(); //пропускаем статус

  uint32_t hum_raw = ((uint32_t)wireRead() << 16) | ((uint32_t)wireRead() << 8);
  uint8_t data_raw = wireRead();
  uint32_t temp_raw = ((uint32_t)(data_raw & 0x0F) << 16) | ((uint16_t)wireRead() << 8) | wireReadEndByte();
  hum_raw = ((hum_raw | data_raw) >> 4);

  temp_raw = ((temp_raw * 2000) >> 20) - 500; //рассчитываем температуру
  hum_raw = (hum_raw * 100) >> 20; //рассчитываем влажность

  sens.err = 0; //сбросили ошибку датчика температуры

#if SENS_BME_ENABLE || SENS_SHT_ENABLE
  if (sens.type & ((0x01 << SENS_BME) | (0x01 << SENS_SHT))) {
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
