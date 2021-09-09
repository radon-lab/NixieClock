//--------------------------------------Инициализация Wire------------------------------------------
void WireInit(void) //инициализация Wire
{
  PORTC |= 0b00110000; //подтяжка SDA и SCL
  DDRC &= 0b11001111; //устанавливаем SDA и SCL как входы
  TWBR = 72; //устанавливаем скорость 100kHz
  TWSR = 0; //устанавливаем делитель 1, статус 0;
}
//--------------------------------------Запуск шины Wire------------------------------------------
void WireStart(void) //запуск шины Wire
{
  TWCR = (0x01 << TWSTA) | (0x01 << TWEN) | (0x01 << TWINT); //включаем Wire, отправляем команду старт и устанавливаем флаг выполнить задачу
  while (!(TWCR & (0x01 << TWINT))); //ожидание завершения
}
//--------------------------------------Остановка шины Wire------------------------------------------
void WireEnd(void) //остановка шины Wire
{
  TWCR = (0x01 << TWSTO) | (0x01 << TWEN) | (0x01 << TWINT); //включаем Wire, отправляем команду стоп и устанавливаем флаг выполнить задачу
}
//--------------------------------------Отправка байта------------------------------------------
void WireWrite(uint8_t data) //отправка байта
{
  TWDR = data; //записать данные в data регистр
  TWCR = (0x01 << TWEN) | (0x01 << TWINT); //запустить передачу
  while (!(TWCR & (0x01 << TWINT))); //дождаться окончания
}
//--------------------------------------Чтение запрошенного байта------------------------------------------
uint8_t WireRead(void) //чтение запрошенного байта
{
  TWCR = (0x01 << TWEN) | (0x01 << TWINT) | (0x01 << TWEA); //запустить чтение шины с подтверждением
  while (!(TWCR & (0x01 << TWINT))); //дождаться окончания приема данных
  return TWDR; //вернуть принятый байт
}
//--------------------------------------Чтение последнего байта------------------------------------------
uint8_t WireReadEndByte(void) //чтение последнего байта
{
  TWCR = (0x01 << TWEN) | (0x01 << TWINT); //запустить чтение шины без подтверждения
  while (!(TWCR & (0x01 << TWINT))); //дождаться окончания приема данных
  WireEnd(); //остановка шины Wire
  return TWDR; //вернуть принятый байт
}
//--------------------------------------Запуск передачи------------------------------------------
void WireBeginTransmission(uint8_t addr) //запуск передачи
{
  WireStart(); //запуск шины Wire
  WireWrite(addr << 0x01); //отправка устройству адреса с битом write
}
//--------------------------------------Проверка статуса шины------------------------------------------
boolean WireStatus(void) //проверка статуса шины
{
  return ((TWSR & 0xF8) == 0x20 || (TWSR & 0xF8) == 0x30) ? 1 : 0; //возвращаем статус
}
//--------------------------------------Запрос чтения данных------------------------------------------
boolean WireRequestFrom(uint8_t addr, uint8_t set) //запрос чтения данных
{
  boolean WireError = 0; //флаг ошибки передачи адреса или данных

  WireStart(); //запуск шины Wire
  WireWrite(addr << 0x01); //отправка устройству адреса с битом write
  WireError |= WireStatus(); //если нет ответа при передаче адреса или данных
  WireWrite(set); //отправка устройству начального адреса чтения
  WireError |= WireStatus(); //если нет ответа при передаче адреса или данных
  WireStart(); //перезапуск шины Wire
  WireWrite((addr << 0x01) | 0x01); //отправка устройству адреса с битом read
  WireError |= WireStatus(); //если нет ответа при передаче адреса или данных

  if (WireError) { //если была обнаружена ошибка
    WireEnd(); //остановка шины Wire
    return 1; //возвращаем ошибку
  }
  return 0; //возвращаем готовность к чтению данных
}
