//--------------------------------------Инициализация wire------------------------------------------
void wireInit(void) //инициализация wire
{
  PORTC |= 0b00110000; //подтяжка SDA и SCL
  DDRC &= 0b11001111; //устанавливаем SDA и SCL как входы
  TWBR = 72; //устанавливаем скорость 100kHz
  TWSR = 0; //устанавливаем делитель 1 и статус 0
}
//--------------------------------------Запуск шины wire------------------------------------------
void wireStart(void) //запуск шины wire
{
  TWCR = (0x01 << TWSTA) | (0x01 << TWEN) | (0x01 << TWINT); //отправляем команду старт и устанавливаем флаг выполнить задачу
  while (!(TWCR & (0x01 << TWINT))); //ожидание завершения
}
//--------------------------------------Остановка шины wire------------------------------------------
void wireEnd(void) //остановка шины wire
{
  TWCR = (0x01 << TWSTO) | (0x01 << TWEN) | (0x01 << TWINT); //отправляем команду стоп и устанавливаем флаг выполнить задачу
}
//--------------------------------------Отправка байта------------------------------------------
void wireWrite(uint8_t data) //отправка байта
{
  TWDR = data; //записываем данные в регистр
  TWCR = (0x01 << TWEN) | (0x01 << TWINT); //запустить передачу
  while (!(TWCR & (0x01 << TWINT))); //ждем окончания передачи
}
//--------------------------------------Чтение запрошенного байта------------------------------------------
uint8_t wireRead(void) //чтение запрошенного байта
{
  TWCR = (0x01 << TWEN) | (0x01 << TWINT) | (0x01 << TWEA); //запускаем чтение шины с подтверждением
  while (!(TWCR & (0x01 << TWINT))); //ждем окончания приема данных
  return TWDR; //вернуть принятый байт
}
//--------------------------------------Чтение последнего байта------------------------------------------
uint8_t wireReadEndByte(void) //чтение последнего байта
{
  TWCR = (0x01 << TWEN) | (0x01 << TWINT); //запускаем чтение шины без подтверждения
  while (!(TWCR & (0x01 << TWINT))); //ждем окончания приема данных
  wireEnd(); //остановка шины wire
  return TWDR; //вернуть принятый байт
}
//--------------------------------------Запуск передачи------------------------------------------
void wireBeginTransmission(uint8_t addr) //запуск передачи
{
  wireStart(); //запуск шины wire
  wireWrite(addr << 0x01); //отправка устройству адреса с битом write
}
//--------------------------------------Проверка статуса шины------------------------------------------
boolean wireStatus(void) //проверка статуса шины
{
  return (boolean)((TWSR & 0xF8) == 0x20 || (TWSR & 0xF8) == 0x30); //возвращаем статус
}
//--------------------------------------Запрос чтения данных------------------------------------------
boolean wireRequestFrom(uint8_t addr, uint8_t reg) //запрос чтения данных
{
  boolean wireError = 0; //флаг ошибки передачи адреса или данных

  wireStart(); //запуск шины wire
  wireWrite(addr << 0x01); //отправка устройству адреса с битом write
  wireError |= wireStatus(); //если нет ответа при передаче адреса или данных
  wireWrite(reg); //отправка устройству начального адреса чтения
  wireError |= wireStatus(); //если нет ответа при передаче адреса или данных
  wireStart(); //перезапуск шины wire
  wireWrite((addr << 0x01) | 0x01); //отправка устройству адреса с битом read
  wireError |= wireStatus(); //если нет ответа при передаче адреса или данных

  if (wireError) { //если была обнаружена ошибка
    wireEnd(); //остановка шины wire
    return 1; //возвращаем ошибку
  }
  return 0; //возвращаем готовность к чтению данных
}
