#define DHT_RESET_TIME 25      //время сигнала сброса DHT(8..30)(мс)
#define DHT_RESET_WAIT_TIME 80 //длительность ожидания сигнала присутствия(50..150)(мкс)
#define DHT_HIGH_LVL_TIME 50   //длительность выского уровня сигнала(30..70)(мкс)
#define DHT_PRESENCE_TIME 150  //длительность сигнала присутствия(80..200)(мкс)
#define DHT_CHECK_TIMEOUT 150  //таймаут ожидания изменения сигнала(100..200)timer

#define DHT_HIGH 1 //высокий уровень шины(1)
#define DHT_LOW 0  //низкий уровень шины(0)
#define DHT_ERR -1 //ошибка шины(-1)

//-------------------------------------------Сигнал сброса шины----------------------------------------------
int8_t wireGetData(boolean lvl, uint32_t time)
{
  uint32_t timer = micros(); //замера длительности сигнала
  while (SDA_READ() == lvl) { //пока есть установленный уровень
    if ((micros() - timer) >= time) return DHT_ERR; //если таймаут то выходим
  }
  if ((micros() - timer) >= DHT_HIGH_LVL_TIME) return DHT_HIGH;
  return DHT_LOW;
}
//--------------------------------------Чтение температуры/влажности------------------------------------------
boolean readTempDHT(void)
{
  uint8_t data[5]; //буфер приема

  SDA_LOW(); //посылаем сигнал сброса
  delay(DHT_RESET_TIME); //ждем
  SDA_HIGH(); //отпускаем шину
  delayMicroseconds(2);

  if (wireGetData(DHT_HIGH, DHT_RESET_WAIT_TIME) == DHT_ERR) return 0; //если таймаут то выходим

  if (wireGetData(DHT_LOW, DHT_PRESENCE_TIME) == DHT_ERR) return 0; //если таймаут то выходим
  if (wireGetData(DHT_HIGH, DHT_PRESENCE_TIME) == DHT_ERR) return 0; //если таймаут то выходим

  for (uint8_t i = 0; i < 40; i++) { //читаем все биты
    data[i >> 3] <<= 1; //сдвинули байт

    if (wireGetData(DHT_LOW, DHT_CHECK_TIMEOUT) == DHT_ERR) return 0; //если таймаут то выходим

    switch (wireGetData(DHT_HIGH, DHT_CHECK_TIMEOUT)) {
      case DHT_ERR: return 0; //если таймаут то выходим
      case DHT_HIGH: data[i >> 3] |= 0x01; break; //установили бит
    }
  }

  if (data[4] != (uint8_t)(data[0] + data[1] + data[2] + data[3])) return 0; //если контрольная сумма не совпала то выходим
  
  if (data[0] > 3) { //если тип датчика DHT11
    sens.temp = ((uint16_t)data[2] * 10) + data[3]; //установили температуру
    sens.hum = data[0]; //установили влажность
  }
  else { //иначе тип датчика DHT22
    sens.temp = ((uint16_t)(data[2] & 0x7F) << 8) | data[3]; //установили температуру
    sens.hum = (((uint16_t)data[0] << 8) | data[1]) / 10; //установили влажность
  }
  if (sens.temp & 0x8000) sens.temp = -(sens.temp & 0x7FFF); //если температура отрицательная

  settings.sensor |= (0x01 << SENS_DHT); //установили флаг сенсора

  return 1;
}
