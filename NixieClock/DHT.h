boolean initDHT = 0; //флаг инициализации датчика

//--------------------------------------Чтение температуры/влажности------------------------------------------
void readTempDHT22(void)
{
  if (!initDHT) {
    initDHT = 1;
    SENS_INIT; //инициализируем датчик
    _delay_ms(2);
  }

  if (_timer_ms[TMR_SENS]) return;
  _timer_ms[TMR_SENS] = 2000;

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

    if (low < high) data[i >> 3] |= 0x01;
  }

  if (data[4] != (uint8_t)(data[0] + data[1] + data[2] + data[3])) {
    readTempRTC(); //читаем температуру DS3231
    return; //выходим
  }

  sens.temp = (((uint16_t)(data[2] & 0x7F)) << 8 | data[3]) * 10;
  sens.press = 0;
  sens.hum = (((uint16_t)data[0]) << 8 | data[1]) / 10;
}
//--------------------------------------Чтение температуры/влажности------------------------------------------
void readTempDHT11(void)
{
  if (!initDHT) {
    initDHT = 1;
    SENS_INIT; //инициализируем датчик
    _delay_ms(2);
  }

  if (_timer_ms[TMR_SENS]) return;

  SENS_OUT;
  SENS_LO; //сигнал начала чтения

  for (_timer_ms[TMR_SENS] = 25; _timer_ms[TMR_SENS];) dataUpdate(); //ждем
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

    if (low < high) data[i >> 3] |= 0x01;
  }
  
  _timer_ms[TMR_SENS] = 2000;
  
  if (data[4] != (uint8_t)(data[0] + data[1] + data[2] + data[3])) {
    readTempRTC(); //читаем температуру DS3231
    return; //выходим
  }

  sens.temp = (((uint16_t)data[2] * 10) + data[3]) * 10;
  sens.press = 0;
  sens.hum = data[0];
}
