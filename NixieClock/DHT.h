#define DHT11_RESET 25 //время сигнала сброса DHT11(18..30)(мс)
#define DHT22_RESET 8  //время сигнала сброса DHT22(1..10)(мс)
#define DHT_RESET_WAIT_TIME 80 //длительность ожидания сигнала присутствия(50..150)(мкс)
#define DHT_PRESENCE_TIME 150  //длительность сигнала присутствия(80..200)(мкс)
#define DHT_CHECK_TIMEOUT 1000 //таймаут ожидания(циклов)

#define SENS_DHT11 0x01 //идентификатор датчика DHT11
#define SENS_DHT22 0x02 //идентификатор датчика DHT22

//-------------------------------------------Сигнал сброса шины----------------------------------------------
boolean wireReset(uint8_t time)
{
  SENS_LO;
  for (_timer_ms[TMR_SENS] = time; _timer_ms[TMR_SENS];) dataUpdate(); //ждем
  SENS_HI;
  _delay_us(2);
  for (uint8_t c = DHT_RESET_WAIT_TIME; c; c--) {
    if (!SENS_CHK) {
      for (uint8_t i = DHT_PRESENCE_TIME; i; i--) {
        if (SENS_CHK) {
          for (uint8_t f = DHT_PRESENCE_TIME; f; f--) {
            if (!SENS_CHK) return 0;
            _delay_us(1);
          }
          return 1;
        }
        _delay_us(1);
      }
      return 1;
    }
    _delay_us(1);
  }
  return 1;
}
//--------------------------------------Чтение температуры/влажности------------------------------------------
void readTempDHT22(void)
{
  if (!sens.initPort) { //если порт не инициализирован
    sens.initPort = 1; //устанавливаем флаг инициализации
    SENS_INIT; //инициализируем порт
  }

  if (wireReset(DHT22_RESET)) return; //посылаем сигнал сброса, если нет ответа то выходим

  uint8_t data[5]; //буфер приема

  for (uint8_t i = 0; i < 40; i++) { //читаем все биты
    data[i >> 3] <<= 1; //сдвинули байт

    uint16_t low = 0; //буфер низкого уровня
    while (!SENS_CHK) { //пока низкий уровень
      if (++low > DHT_CHECK_TIMEOUT) return; //если таймаут то выходим
    }
    uint16_t high = 0; //буфер высокого уровня
    while (SENS_CHK) { //пока высокий уровень
      if (++high > DHT_CHECK_TIMEOUT) return; //если таймаут то выходим
    }

    if (low < high) data[i >> 3] |= 0x01; //установили бит
  }

  if (data[4] != (uint8_t)(data[0] + data[1] + data[2] + data[3])) return; //если контрольная сумма не совпала то выходим

  sens.temp = (((uint16_t)(data[2] & 0x7F)) << 8 | data[3]) * 10; //установили температуру
  if (sens.temp > 8500) sens.temp = 0; //если вышли за предел
  sens.press = 0; //сбросили давление
  sens.hum = (((uint16_t)data[0]) << 8 | data[1]) / 10; //установили влажность
  if (sens.hum > 99) sens.hum = 99; //если вышли за предел
  sens.err = 0; //сбросили ошибку датчика температуры
}
//--------------------------------------Чтение температуры/влажности------------------------------------------
void readTempDHT11(void)
{
  if (!sens.initPort) { //если порт не инициализирован
    sens.initPort = 1; //устанавливаем флаг инициализации
    SENS_INIT; //инициализируем порт
  }

  if (wireReset(DHT11_RESET)) return; //посылаем сигнал сброса, если нет ответа то выходим

  uint8_t data[5]; //буфер приема

  for (uint8_t i = 0; i < 40; i++) { //читаем все биты
    data[i >> 3] <<= 1; //сдвинули байт

    uint16_t low = 0; //буфер низкого уровня
    while (!SENS_CHK) { //пока низкий уровень
      if (++low > DHT_CHECK_TIMEOUT) return; //если таймаут то выходим
    }
    uint16_t high = 0; //буфер высокого уровня
    while (SENS_CHK) { //пока высокий уровень
      if (++high > DHT_CHECK_TIMEOUT) return; //если таймаут то выходим
    }

    if (low < high) data[i >> 3] |= 0x01; //установили бит
  }

  if (data[4] != (uint8_t)(data[0] + data[1] + data[2] + data[3])) return; //если контрольная сумма не совпала то выходим

  sens.temp = (((uint16_t)data[2] * 10) + data[3]) * 10; //установили температуру
  if (sens.temp > 8500) sens.temp = 0; //если вышли за предел
  sens.press = 0; //сбросили давление
  sens.hum = data[0]; //установили влажность
  if (sens.hum > 99) sens.hum = 99; //если вышли за предел
  sens.err = 0; //сбросили ошибку датчика температуры
}
