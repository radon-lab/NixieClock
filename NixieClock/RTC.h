#define RTC_ADDR 0x68 //адрес RTC DS3231

struct time { //структура времени
  uint8_t s = 0;
  uint8_t m = 0;
  uint8_t h = 8;
  uint8_t DD = 1;
  uint8_t MM = 1;
  uint16_t YY = 2021;
  uint8_t DW = 5;
} RTC;

const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //дней в месяце

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
  return ((RTC.MM == 2 && !(RTC.YY % 4)) ? 1 : 0) + daysInMonth[RTC.MM - 1]; //возвращаем количество дней в месяце
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
//-------------------------------Чтение коррекции хода-------------------------------------
uint8_t readAgingRTC(void) //чтение коррекции хода
{
  if (WireRequestFrom(RTC_ADDR, 0x10)) return 0; //запрашиваем чтение данных, если нет ответа выходим
  return WireReadEndByte(); //возвращаем результат
}
//-------------------------------Запись коррекции хода-------------------------------------
void writeAgingRTC(uint8_t data) //запись коррекции хода
{
  WireBeginTransmission(RTC_ADDR); //начало передачи
  WireWrite(0x10); //устанавливаем адрес записи
  WireWrite(data); //записываем коррекцию хода
  WireEnd(); //остановка шины Wire
}
//--------------------------------Проверка флага OSF--------------------------------------
boolean getOSF(void) //проверка флага OSF
{
  if (WireRequestFrom(RTC_ADDR, 0x0F)) { //запрашиваем чтение данных, если нет ответа то
    SET_ERROR(DS3231_ERROR); //устанавливаем ошибку модуля RTC
    return 0; //выходим
  }
  uint8_t ctrlReg = WireReadEndByte(); //прочитали регистр статуса

  if (ctrlReg & 0x80) { //проверяем установлен ли флаг OSF
    ctrlReg &= 0x7F; //очистили флаг OSF
    WireBeginTransmission(RTC_ADDR); //начало передачи
    WireWrite(0x0F); //устанавливаем адрес записи
    WireWrite(ctrlReg); //отправляем настройку OSF
    WireEnd(); //конец передачи
    SET_ERROR(DS3231_OSF_ERROR); //установили ошибку осцилятора модуля RTC
    return 0;
  }
  return 1;
}
//-----------------------------------Настройка SQW-----------------------------------------
void setSQW(void) //настройка SQW
{
  if (WireRequestFrom(RTC_ADDR, 0x0E)) { //запрашиваем чтение данных, если нет ответа то
    SET_ERROR(DS3231_ERROR); //устанавливаем ошибку модуля RTC
    return; //выходим
  }
  uint8_t ctrlReg = WireReadEndByte() & 0x20; //выключаем INTCON и устанавливаем частоту 1Гц

  WireBeginTransmission(RTC_ADDR); //начало передачи
  WireWrite(0x0E); //устанавливаем адрес записи
  WireWrite(ctrlReg); //отправляем настройку SQW
  WireEnd(); //конец передачи
}
//-------------------------------Отключение вывода 32K-------------------------------------
void disable32K(void) //отключение вывода 32K
{
  if (WireRequestFrom(RTC_ADDR, 0x0F)) { //запрашиваем чтение данных, если нет ответа то
    SET_ERROR(DS3231_ERROR); //устанавливаем ошибку модуля RTC
    return; //выходим
  }
  uint8_t ctrlReg = WireReadEndByte() & 0xF7; //выключаем 32K

  WireBeginTransmission(RTC_ADDR); //начало передачи
  WireWrite(0x0F); //устанавливаем адрес записи
  WireWrite(ctrlReg); //отправляем настройку SQW
  WireEnd(); //конец передачи
}
//-------------------------------Чтение температуры-------------------------------------
boolean readTempRTC(void) //чтение температуры
{
  if (WireRequestFrom(RTC_ADDR, 0x11)) return 0; //запрашиваем чтение данных, если нет ответа выходим
  uint16_t temp = (((uint16_t)WireRead() << 2 | WireReadEndByte() >> 6) * 100) >> 2;
  sens.temp = (temp > 8500) ? 0 : temp;
  sens.press = 0; //сбросили давление
  sens.hum = 0; //сбросили влажность
  return 1; //выходим
}
//--------------------------------------Отправить время в RTC------------------------------------------
void sendTime(void) //отправить время в RTC
{
  RTC.DW = getWeekDay(RTC.YY, RTC.MM, RTC.DD); //получаем день недели
  WireBeginTransmission(RTC_ADDR); //начало передачи
  WireWrite(0x00); //устанавливаем адрес записи
  WireWrite((((RTC.s / 10) << 4) | (RTC.s % 10))); //отправляем секунды
  WireWrite((((RTC.m / 10) << 4) | (RTC.m % 10))); //отправляем минуты
  if (RTC.h > 19) WireWrite((0x02 << 4) | (RTC.h % 20)); //отправляем часы
  else if (RTC.h > 9) WireWrite((0x01 << 4) | (RTC.h % 10));
  else WireWrite(RTC.h);
  WireWrite(0); //пропускаем день недели
  WireWrite(((RTC.DD / 10) << 4) | (RTC.DD % 10)); //отправляем дату
  WireWrite(((RTC.MM / 10) << 4) | (RTC.MM % 10)); //отправляем месяц
  WireWrite((((RTC.YY - 2000) / 10) << 4) | ((RTC.YY - 2000) % 10)); //отправляем год
  WireEnd(); //конец передачи
}
//--------------------------------------Запрашиваем время из RTC------------------------------------------
boolean getTime(void) //запрашиваем время из RTC
{
  if (getOSF()) { //проверка флага OSF
    if (WireRequestFrom(RTC_ADDR, 0x00)) { //запрашиваем чтение данных, если нет ответа выходим
      SET_ERROR(DS3231_ERROR); //устанавливаем ошибку модуля RTC
      return 1; //выходим
    }
    RTC.s = unpackREG(WireRead()); //получаем секунды
    RTC.m = unpackREG(WireRead()); //получаем минуты
    RTC.h = unpackHours(WireRead()); //получаем часы
    WireRead(); //пропускаем день недели
    RTC.DD = unpackREG(WireRead()); //получаем дату
    RTC.MM = unpackREG(WireRead()); //получаем месяц
    RTC.YY = unpackREG(WireReadEndByte()) + 2000; //получаем год
    RTC.DW = getWeekDay(RTC.YY, RTC.MM, RTC.DD); //получаем день недели
    return 0;
  }
  return 1; //выходим
}
