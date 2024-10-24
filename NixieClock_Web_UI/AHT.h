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
uint8_t readTempAHT(void) //чтение температуры/влажности
{
  static uint8_t typeAHT; //тип датчика AHT
  static uint8_t attemptsAHT; //попытки запроса датчика AHT
  uint16_t timer = (uint16_t)millis(); //установили таймер

  if (sens.type == 1) return 2; //сенсор установлен на стороне часов

  if (attemptsAHT < 5) attemptsAHT++; //попытка запроса температуры
  else return 2; //иначе выходим

  if (!typeAHT) { //если датчик не определен
    if (!twi_beginTransmission(AHT10_ADDR)) { //выходим если нет ответа
      twi_write_byte(AHT10_CALIBRATE_START); //начинаем калибровку для AHT10
      twi_write_byte(AHT10_CALIBRATE_DATA); //записываем настройку
      twi_write_byte(AHT10_NOP_DATA); //записываем настройку
      twi_write_stop(); //остановка шины wire

      while (1) { //ждем окончания калибровки
        yield(); //обработка данных
        if (twi_beginTransmission(AHT10_ADDR, 1)) return 1; //запрашиваем чтение данных
        if ((twi_read_byte(TWI_NACK) & 0x88) == 0x08) break;
        if (((uint16_t)millis() - timer) >= AHT10_CALIBRATE_TIME) return 1; //выходим если таймаут
      }
    }
    else return 1;
    typeAHT = AHT10_ADDR; //установлили тип датчика
  }

  if (twi_beginTransmission(AHT10_ADDR)) return 1; //начало передачи
  twi_write_byte(AHT10_MEASURE_START); //записываем настройку
  twi_write_byte(AHT10_MEASURE_DATA); //записываем настройку
  twi_write_byte(AHT10_NOP_DATA); //записываем настройку
  twi_write_stop(); //остановка шины wire

  timer = (uint16_t)millis(); //установили таймер
  while (1) { //ждем окончания замера
    yield(); //обработка данных
    if (twi_beginTransmission(AHT10_ADDR, 1)) return 1; //запрашиваем чтение данных
    if (!(twi_read_byte(TWI_NACK) & 0x80)) break;
    if (((uint16_t)millis() - timer) >= AHT10_MEASURE_TIME) return 1; //выходим если таймаут
  }

  if (twi_beginTransmission(AHT10_ADDR, 1)) return 1; //запрашиваем чтение данных
  twi_read_byte(TWI_ACK); //пропускаем статус

  uint32_t hum_raw = ((uint32_t)twi_read_byte(TWI_ACK) << 16) | ((uint32_t)twi_read_byte(TWI_ACK) << 8);
  uint8_t data_raw = twi_read_byte(TWI_ACK);
  uint32_t temp_raw = ((uint32_t)(data_raw & 0x0F) << 16) | ((uint16_t)twi_read_byte(TWI_ACK) << 8) | twi_read_byte(TWI_NACK);
  temp_raw = ((temp_raw * 2000) >> 20) - 500; //рассчитываем температуру
  hum_raw = ((hum_raw | data_raw) >> 4);
  hum_raw = (hum_raw * 100) >> 20; //рассчитываем влажность

  if (!sens.status) sens.temp[SENS_MAIN] = temp_raw; //записываем температуру
  else sens.temp[SENS_MAIN] = (sens.temp[SENS_MAIN] + temp_raw) / 2; //усредняем температуру

  if (!sens.status) sens.hum[SENS_MAIN] = hum_raw; //записываем влажность
  else if (hum_raw) sens.hum[SENS_MAIN] = (sens.hum[SENS_MAIN] + hum_raw) / 2; //усредняем влажность

  if ((uint16_t)sens.temp[SENS_MAIN] > 850) sens.temp[SENS_MAIN] = 0; //если вышли за предел
  if (sens.hum[SENS_MAIN] > 99) sens.hum[SENS_MAIN] = 99; //если вышли за предел

  sens.status |= SENS_AHT; //установили флаг
  attemptsAHT = 0; //сбросили попытки запроса
  return 0; //выходим
}
