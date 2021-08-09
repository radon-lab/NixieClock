#define RTC_ADDR 0x68 //адрес RTC

struct time { //структура времени
  uint8_t s = 0;
  uint8_t m = 0;
  uint8_t h = 8;
  uint8_t DD = 1;
  uint8_t MM = 1;
  uint16_t YY = 2021;
  uint8_t DW = 5;
} TimeRTC;

const uint8_t daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; //дней в месяце

//--------------------------------------Получить день недели------------------------------------------
uint8_t getWeekDay(uint16_t YY, uint8_t MM, uint8_t DD) //получить день недели
{
  if (YY >= 2000) YY -= 2000; //если год больше 2000
  uint16_t days = DD; //записываем дату
  for (uint8_t i = 1; i < MM; i++) days += daysInMonth[i - 1]; //записываем сколько дней прошло до текущего месяца
  if (MM > 2 && YY % 4 == 0) days++; //если високосный год, прибавляем день
  return (days + 365 * YY + (YY + 3) / 4 - 2 + 6) % 7 + 1; //возвращаем день недели
}
//-------------------------------Максимальное количество дней--------------------------------------------------
uint8_t maxDays(void) //максимальное количество дней
{
  return ((TimeRTC.MM == 2 && !(TimeRTC.YY % 4)) ? 1 : 0) + daysInMonth[TimeRTC.MM - 1]; //возвращаем количество дней в месяце
}
//--------------------------------------Распаковка short------------------------------------------
uint8_t unpackREG(uint8_t data) //распаковка short
{
  return ((data >> 4) * 10 + (data & 0xF)); //возвращаем результат
}
//--------------------------------------Распаковка часов------------------------------------------
uint8_t unpackHours(uint8_t data) //распаковка часов
{
  if (data & 0x20) return ((data & 0x0F) + 20); //возвращаем результат
  else if (data & 0x10) return ((data & 0x0F) + 10);
  else return (data & 0x0F);
}
//--------------------------------------Отправить время в RTC------------------------------------------
void sendTime(void) //отправить время в RTC
{
  TimeRTC.DW = getWeekDay(TimeRTC.YY, TimeRTC.MM, TimeRTC.DD); //получаем день недели
  WireBeginTransmission(RTC_ADDR); //начало передачи
  WireWrite(0x00); //устанавливаем адрес записи
  WireWrite((((TimeRTC.s / 10) << 4) | (TimeRTC.s % 10))); //отправляем секунды
  WireWrite((((TimeRTC.m / 10) << 4) | (TimeRTC.m % 10))); //отправляем минуты
  if (TimeRTC.h > 19) WireWrite((0x02 << 4) | (TimeRTC.h % 20)); //отправляем часы
  else if (TimeRTC.h > 9) WireWrite((0x01 << 4) | (TimeRTC.h % 10));
  else WireWrite(TimeRTC.h);
  WireWrite(0); //пропускаем день недели
  WireWrite(((TimeRTC.DD / 10) << 4) | (TimeRTC.DD % 10)); //отправляем дату
  WireWrite(((TimeRTC.MM / 10) << 4) | (TimeRTC.MM % 10)); //отправляем месяц
  WireWrite((((TimeRTC.YY - 2000) / 10) << 4) | ((TimeRTC.YY - 2000) % 10)); //отправляем год
  WireEnd(); //конец передачи
}
//--------------------------------------Запрашиваем время из RTC------------------------------------------
boolean getTime(void) //запрашиваем время из RTC
{
  if (WireRequestFrom(RTC_ADDR, 0x00)) return 0; //запрашиваем чтение данных, если нет ответа выходим
  TimeRTC.s = unpackREG(WireRead()); //получаем секунды
  TimeRTC.m = unpackREG(WireRead()); //получаем минуты
  TimeRTC.h = unpackHours(WireRead()); //получаем часы
  WireRead(); //пропускаем день недели
  TimeRTC.DD = unpackREG(WireRead()); //получаем дату
  TimeRTC.MM = unpackREG(WireRead()); //получаем месяц
  TimeRTC.YY = unpackREG(WireReadEndByte()) + 2000; //получаем год
  TimeRTC.DW = getWeekDay(TimeRTC.YY, TimeRTC.MM, TimeRTC.DD); //получаем день недели
  return 1;
}
//-------------------------------Настройка SQW-------------------------------------
void setSQW(void) //настройка SQW
{
  if (WireRequestFrom(RTC_ADDR, 0x0E)) return; //запрашиваем чтение данных, если нет ответа выходим
  uint8_t ctrlReg = WireReadEndByte() & 0xE3; //выключаем INTCON и устанавливаем частоту 1Гц

  WireBeginTransmission(RTC_ADDR); //начало передачи
  WireWrite(0x0E); //устанавливаем адрес записи
  WireWrite(ctrlReg); //отправляем настройку SQW
  WireEnd(); //конец передачи
}
//-------------------------------Цтение температура-------------------------------------
void readTempRTC(void) //чтение температуры
{
  if (WireRequestFrom(RTC_ADDR, 0x11)) return; //запрашиваем чтение данных, если нет ответа выходим
  uint16_t temp = ((float)(WireRead() << 2 | WireReadEndByte() >> 6) * 0.25) * 100.0;
  tempSens.temp = (temp > 8500) ? 0 : temp;
  tempSens.press = 0;
  tempSens.hum = 0;
}
//-------------------------------Чтение коррекции хода-------------------------------------
uint8_t readAgingRTC(void) //чтение коррекции хода
{
  if (WireRequestFrom(RTC_ADDR, 0x10)) return 0; //запрашиваем чтение данных, если нет ответа выходим
  return WireRead();
}
//-------------------------------Запись коррекции хода-------------------------------------
void WriteAgingRTC(uint8_t data) //запись коррекции хода
{
  WireBeginTransmission(RTC_ADDR); //начало передачи
  WireWrite(0x10); //устанавливаем адрес записи
  WireWrite(data); //записываем коррекцию хода
}
