boolean initDHT = 0; //флаг инициализации датчика

//--------------------------------------Чтение температуры/влажности------------------------------------------
void readTempDHT22(void)
{
  if (!initDHT) {
    initDHT = 1;
    SENS_INIT;
    _delay_ms(2);
  }
  if (_timer_ms[TMR_SENS]) return;

  SENS_OUT;
  SENS_LO; //сигнал начала чтения

  _delay_us(1100);
  SENS_HI; //посылаем сигнал сброса
  SENS_INP; //переходим в режим чтения
  _delay_us(200);

  uint8_t data[5];

  for (uint8_t i = 0; i < 40; i++) {
    data[i >> 3] <<= 1;

    uint16_t low = 0;
    while (!SENS_CHK) {
      if (++low > 1000) return;
    }
    uint16_t high = 0;
    while (SENS_CHK) {
      if (++high > 1000) return;
    }

    if (low < high) data[i >> 3] |= 1;
  }

  if (data[4] != (uint8_t)(data[0] + data[1] + data[2] + data[3])) return;

  tempSens.temp = (((uint16_t)(data[2] & 0x7F)) << 8 | data[3]) * 10;
  tempSens.press = 0;
  tempSens.hum = (((uint16_t)data[0]) << 8 | data[1]) / 10;

  _timer_ms[TMR_SENS] = 2000;
}
//--------------------------------------Чтение температуры/влажности------------------------------------------
void readTempDHT11(void)
{
  if (!initDHT) {
    initDHT = 1;
    SENS_INIT;
    _delay_ms(2);
  }
  if (_timer_ms[TMR_SENS]) return;

  SENS_OUT;
  SENS_LO; //сигнал начала чтения

  _delay_ms(20);
  SENS_HI; //посылаем сигнал сброса
  SENS_INP; //переходим в режим чтения
  _delay_us(200);

  uint8_t data[5];

  for (uint8_t i = 0; i < 40; i++) {
    data[i >> 3] <<= 1;

    uint16_t low = 0;
    while (!SENS_CHK) {
      if (++low > 1000) return;
    }
    uint16_t high = 0;
    while (SENS_CHK) {
      if (++high > 1000) return;
    }

    if (low < high) data[i >> 3] |= 1;
  }

  if (data[4] != (uint8_t)(data[0] + data[1] + data[2] + data[3])) return;

  tempSens.temp = (((uint16_t)data[2] * 10) + data[3]) * 10;
  tempSens.press = 0;
  tempSens.hum = data[0];

  _timer_ms[TMR_SENS] = 2000;
}
