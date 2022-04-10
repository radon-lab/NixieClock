boolean initDHT = 0; //флаг инициализации датчика

#define DHT11_RESET 25 //время сигнала сброса DHT11
#define DHT22_RESET 8  //время сигнала сброса DHT22
#define DHT_CHECK_TIMEOUT 1000 //таймаут ожидания

//-------------------------------------------Сигнал сброса шины----------------------------------------------
boolean wireReset(uint8_t time)
{
  SENS_LO;
  for (_timer_ms[TMR_SENS] = time; _timer_ms[TMR_SENS];) dataUpdate(); //ждем
  SENS_HI;
  _delay_us(2);
  for (uint8_t c = 80; c; c--) {
    if (!SENS_CHK) {
      for (uint8_t i = 150; !SENS_CHK && i; i--) _delay_us(1);
      for (uint8_t i = 150; SENS_CHK && i; i--) _delay_us(1);
      return 0;
    }
    _delay_us(1);
  }
  return 1;
}
//--------------------------------------Чтение температуры/влажности------------------------------------------
void readTempDHT22(void)
{
  if (!initDHT) {
    initDHT = 1;
    SENS_INIT; //инициализируем датчик
    _delay_ms(2);
  }

  if (wireReset(DHT22_RESET)) { //посылаем сигнал сброса
    readTempRTC(); //читаем температуру DS3231
    return; //выходим
  }

  uint8_t data[5];

  for (uint8_t i = 0; i < 40; i++) {
    data[i >> 3] <<= 1;

    uint16_t low = 0;
    while (!SENS_CHK) {
      if (++low > DHT_CHECK_TIMEOUT) return;
    }
    uint16_t high = 0;
    while (SENS_CHK) {
      if (++high > DHT_CHECK_TIMEOUT) return;
    }

    if (low < high) data[i >> 3] |= 0x01;
  }

  if (data[4] != (uint8_t)(data[0] + data[1] + data[2] + data[3])) {
    readTempRTC(); //читаем температуру DS3231
    return; //выходим
  }

  sens.temp = (((uint16_t)(data[2] & 0x7F)) << 8 | data[3]) * 10; //установили температуру
  sens.press = 0; //сбросили давление
  sens.hum = (((uint16_t)data[0]) << 8 | data[1]) / 10; //установили влажность
  sens.err = 0; //сбросили ошибку датчика температуры
}
//--------------------------------------Чтение температуры/влажности------------------------------------------
void readTempDHT11(void)
{
  if (!initDHT) {
    initDHT = 1;
    SENS_INIT; //инициализируем датчик
    _delay_ms(2);
  }

  if (wireReset(DHT11_RESET)) { //посылаем сигнал сброса
    readTempRTC(); //читаем температуру DS3231
    return; //выходим
  }

  uint8_t data[5];

  for (uint8_t i = 0; i < 40; i++) {
    data[i >> 3] <<= 1;

    uint16_t low = 0;
    while (!SENS_CHK) {
      if (++low > DHT_CHECK_TIMEOUT) return;
    }
    uint16_t high = 0;
    while (SENS_CHK) {
      if (++high > DHT_CHECK_TIMEOUT) return;
    }

    if (low < high) data[i >> 3] |= 0x01;
  }

  if (data[4] != (uint8_t)(data[0] + data[1] + data[2] + data[3])) {
    readTempRTC(); //читаем температуру DS3231
    return; //выходим
  }

  sens.temp = (((uint16_t)data[2] * 10) + data[3]) * 10; //установили температуру
  sens.press = 0; //сбросили давление
  sens.hum = data[0]; //установили влажность
  sens.err = 0; //сбросили ошибку датчика температуры
}
