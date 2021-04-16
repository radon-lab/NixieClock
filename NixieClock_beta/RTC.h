#define RTC_ADDR 0x68 //адрес RTC

struct time { //структура времени
  uint8_t s = 0;
  uint8_t m = 0;
  uint8_t h = 0;
  uint8_t DD = 0;
  uint8_t MM = 0;
  uint16_t YY = 0;
  uint8_t DW = 0;
} RTC_time;

const uint8_t daysInMonth[] PROGMEM = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; //дней в месяце

//--------------------------------------Получить день недели------------------------------------------
uint16_t getWeekDay(uint16_t YY, uint8_t MM, uint8_t DD) //получить день недели
{
  if (YY >= 2000) YY -= 2000; //если год больше 2000
  uint16_t days = DD; //записываем дату
  for (uint8_t i = 1; i < MM; i++) days += pgm_read_byte(daysInMonth + i - 1); //записываем сколько дней прошло до текущего месяца
  if (MM > 2 && YY % 4 == 0) days++; //если високосный год, прибавляем день
  return (days + 365 * YY + (YY + 3) / 4 - 1 + 6) % 7; //возвращаем день недели
}
//--------------------------------------Распаковка short------------------------------------------
uint8_t unpackREG(uint8_t data) //распаковка short
{
  return ((data >> 4) * 10 + (data & 0xF)); //возвращаем результат
}
//--------------------------------------Распаковка часов------------------------------------------
uint8_t unpackHours(uint8_t data) //распаковка часов
{
  if (data & 0x20) return ((data & 0xF) + 20); //возвращаем результат
  else if (data & 0x10) return ((data & 0xF) + 10);
  else return (data & 0xF);
}
//--------------------------------------Отправить время в RTC------------------------------------------
void sendTime(void) //отправить время в RTC
{
  uint8_t day = getWeekDay(RTC_time.YY, RTC_time.MM, RTC_time.DD); //получаем день недели
  WireBeginTransmission(RTC_ADDR); //начало передачи
  WireWrite(0x00); //устанавливаем адрес записи
  WireWrite((((RTC_time.s / 10) << 4) | (RTC_time.s % 10))); //отправляем секунды
  WireWrite((((RTC_time.m / 10) << 4) | (RTC_time.m % 10))); //отправляем минуты
  if (RTC_time.h > 19) WireWrite((0x2 << 4) | (RTC_time.h % 20)); //отправляем часы
  else if (RTC_time.h > 9) WireWrite((0x1 << 4) | (RTC_time.h % 10));
  else WireWrite(RTC_time.h);
  WireWrite(day); //отправляем день недели
  WireWrite(((RTC_time.DD / 10) << 4) | (RTC_time.DD % 10)); //отправляем дату
  WireWrite(((RTC_time.MM / 10) << 4) | (RTC_time.MM % 10)); //отправляем месяц
  WireWrite((((RTC_time.YY - 2000) / 10) << 4) | ((RTC_time.YY - 2000) % 10)); //отправляем год
  WireEndTransmission(); //конец передачи
}
//--------------------------------------Запрашиваем время из RTC------------------------------------------
void getTime(void) //запрашиваем время из RTC
{
  WireBeginTransmission(RTC_ADDR); //начало передачи
  WireWrite(0x00); //устанавливаем адрес чтения
  if (WireEndTransmission() != 0) return; //если нет ответа выходим
  WireRequestFrom(RTC_ADDR, 7); //запрашиваем данные
  RTC_time.s = unpackREG(WireRead()); //получаем секунды
  RTC_time.m = unpackREG(WireRead()); //получаем минуты
  RTC_time.h = unpackHours(WireRead()); //получаем часы
  RTC_time.DW = WireRead(); //получаем день недели
  RTC_time.DD = unpackREG(WireRead()); //получаем дату
  RTC_time.MM = unpackREG(WireRead()); //получаем месяц
  RTC_time.YY = unpackREG(WireRead()) + 2000; //получаем год
}
//-------------------------------Температура-------------------------------------
uint16_t getTemp()
{
  WireBeginTransmission(RTC_ADDR); //начало передачи
  WireWrite(0x11); //устанавливаем адрес чтения
  if (WireEndTransmission() != 0) return; //если нет ответа выходим
  WireRequestFrom(0x68, 2); //запрашиваем данные
  uint16_t temp = ((float)(WireRead() << 2 | WireRead() >> 6) * 0.25) * 100.0;
  return (temp > 8500) ? 0 : temp;
}
