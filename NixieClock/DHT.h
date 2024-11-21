#define DHT_RESET_TIME 25      //время сигнала сброса DHT(8..30)(мс)
#define DHT_RESET_WAIT_TIME 80 //длительность ожидания сигнала присутствия(50..150)(мкс)
#define DHT_PRESENCE_TIME 150  //длительность сигнала присутствия(80..200)(мкс)
#define DHT_CHECK_TIMEOUT 1000 //таймаут ожидания(циклов)

//-------------------------------------------Сигнал сброса шины----------------------------------------------
boolean wireReset(uint8_t time)
{
  SENS_LO;
  for (_timer_ms[TMR_SENS] = time; _timer_ms[TMR_SENS];) systemTask(); //ждем
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
//-------------------------------------------Сигнал сброса шины----------------------------------------------
boolean wireGetData(uint8_t* buff, uint8_t time)
{
  if (wireReset(time)) return 1; //посылаем сигнал сброса, если нет ответа то выходим

  uint16_t low; //буфер низкого уровня
  uint16_t high; //буфер высокого уровня

  for (uint8_t i = 0; i < 40; i++) { //читаем все биты
    buff[i >> 3] <<= 1; //сдвинули байт

    low = 0; //сбросили буфер низкого уровня
    while (!SENS_CHK) { //пока низкий уровень
      if (++low > DHT_CHECK_TIMEOUT) return 1; //если таймаут то выходим
    }
    high = 0; //сбросили буфер высокого уровня
    while (SENS_CHK) { //пока высокий уровень
      if (++high > DHT_CHECK_TIMEOUT) return 1; //если таймаут то выходим
    }

    if (low < high) buff[i >> 3] |= 0x01; //установили бит
  }

  if (buff[4] != (uint8_t)(buff[0] + buff[1] + buff[2] + buff[3])) return 1; //если контрольная сумма не совпала то выходим

  return 0;
}
//--------------------------------------Чтение температуры/влажности------------------------------------------
void readTempDHT(void)
{
  uint8_t data[5]; //буфер приема

  if (wireGetData(data, DHT_RESET_TIME)) return; //посылаем сигнал сброса, если нет ответа то выходим
  if (data[0] > 3) { //если тип датчика DHT11
    sens.temp = ((uint16_t)data[2] * 10) + data[3]; //установили температуру
    sens.hum = data[0]; //установили влажность
  }
  else { //иначе тип датчика DHT22
    sens.temp = ((uint16_t)(data[2] & 0x7F) << 8) | data[3]; //установили температуру
    sens.hum = (((uint16_t)data[0] << 8) | data[1]) / 10; //установили влажность
  }
  if (sens.hum > 99) sens.hum = 99; //если вышли за предел
  if (sens.temp & 0x8000) sens.temp = -(sens.temp & 0x7FFF); //если температура отрицательная
  sens.press = 0; //сбросили давление
  sens.update = 1; //установили флаг обновления сенсора
}
