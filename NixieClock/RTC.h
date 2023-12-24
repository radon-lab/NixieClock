#define RTC_ADDR 0x68 //адрес RTC DS3231

#define RTC_CHECK_OSF 0x00 //проверить флаг OSF
#define RTC_CLEAR_OSF 0x01 //проверить и очистить флаг OSF

//структура времени
struct time {
  uint8_t s = 0; //секунды
  uint8_t m = 0; //минуты
  uint8_t h = 8; //часы
  uint8_t DD = 1; //день
  uint8_t MM = 1; //месяц
  uint16_t YY = 2021; //год
  uint8_t DW = 5; //день недели
} RTC;

const uint8_t daysInMonth[] PROGMEM = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //дней в месяце

//---------------------------------Получить день недели-----------------------------------
uint8_t getWeekDay(uint16_t YY, uint8_t MM, uint8_t DD) //получить день недели
{
  if (YY >= 2000) YY -= 2000; //если год больше 2000
  uint16_t days = DD; //записываем дату
  for (uint8_t i = 1; i < MM; i++) days += pgm_read_byte(&daysInMonth[i - 1]); //записываем сколько дней прошло до текущего месяца
  if ((MM > 2) && !(YY % 4)) days++; //если високосный год, прибавляем день
  return (days + 365 * YY + (YY + 3) / 4 - 2 + 6) % 7 + 1; //возвращаем день недели
}
//------------------------------Максимальное количество дней------------------------------
uint8_t maxDays(void) //максимальное количество дней
{
  return (((RTC.MM == 2) && !(RTC.YY % 4)) ? 1 : 0) + pgm_read_byte(&daysInMonth[RTC.MM - 1]); //возвращаем количество дней в месяце
}
//---------------------------------Распаковка регистра-------------------------------------
uint8_t unpackREG(uint8_t data) //распаковка регистра
{
  return ((data >> 4) * 10 + (data & 0x0F)); //возвращаем результат
}
//---------------------------------Запаковка регистра--------------------------------------
uint8_t packREG(uint8_t data) //запаковка регистра
{
  return (((data / 10) << 4) | (data % 10)); //возвращаем результат
}
//-------------------------------Чтение коррекции хода-------------------------------------
boolean readAgingRTC(int8_t* data) //чтение коррекции хода
{
  if (wireRequestFrom(RTC_ADDR, 0x10)) return 0; //запрашиваем чтение данных, если нет ответа выходим
  *data = wireReadEndByte(); //записываем результат
  return 1; //выходим
}
//-------------------------------Запись коррекции хода-------------------------------------
void writeAgingRTC(int8_t data) //запись коррекции хода
{
  if (wireBeginTransmission(RTC_ADDR)) return; //начало передачи
  wireWrite(0x10); //устанавливаем адрес записи
  wireWrite(data); //записываем коррекцию хода
  wireEnd(); //остановка шины wire
}
//--------------------------------Проверка флага OSF--------------------------------------
boolean getOSF(boolean mode) //проверка флага OSF
{
  if (wireRequestFrom(RTC_ADDR, 0x0F)) { //запрашиваем чтение данных, если нет ответа то
    SET_ERROR(DS3231_ERROR); //устанавливаем ошибку модуля RTC
    return 0; //выходим
  }
  uint8_t ctrlReg = wireReadEndByte(); //прочитали регистр статуса

  if (ctrlReg & 0x80) { //проверяем установлен ли флаг OSF
    if (mode) { //если нужно очистить флаг OSF
      if (wireBeginTransmission(RTC_ADDR)) SET_ERROR(DS3231_ERROR); //устанавливаем ошибку модуля RTC
      else { //иначе отправляем данные
        ctrlReg &= 0x7F; //очистили флаг OSF
        wireWrite(0x0F); //устанавливаем адрес записи
        wireWrite(ctrlReg); //отправляем настройку OSF
        wireEnd(); //конец передачи
      }
    }
    SET_ERROR(DS3231_OSF_ERROR); //установили ошибку осцилятора модуля RTC
    return 0; //выходим
  }
  return 1; //выходим
}
//-----------------------------------Настройка SQW-----------------------------------------
boolean setSQW(void) //настройка SQW
{
  if (wireRequestFrom(RTC_ADDR, 0x0E)) { //запрашиваем чтение данных, если нет ответа то
    SET_ERROR(DS3231_ERROR); //устанавливаем ошибку модуля RTC
    return 0; //выходим
  }
  uint8_t ctrlReg = wireReadEndByte() & 0x20; //выключаем INTCON и устанавливаем частоту 1Гц

  if (wireBeginTransmission(RTC_ADDR)) SET_ERROR(DS3231_ERROR); //устанавливаем ошибку модуля RTC
  else { //иначе отправляем данные
    wireWrite(0x0E); //устанавливаем адрес записи
    wireWrite(ctrlReg); //отправляем настройку SQW
    wireEnd(); //конец передачи
    return 1; //выходим
  }
  return 0; //выходим
}
//-------------------------------Отключение вывода 32K-------------------------------------
boolean disable32K(void) //отключение вывода 32K
{
  if (wireRequestFrom(RTC_ADDR, 0x0F)) { //запрашиваем чтение данных, если нет ответа то
    SET_ERROR(DS3231_ERROR); //устанавливаем ошибку модуля RTC
    return 0; //выходим
  }
  uint8_t ctrlReg = wireReadEndByte() & 0xF7; //выключаем 32K

  if (wireBeginTransmission(RTC_ADDR)) SET_ERROR(DS3231_ERROR); //устанавливаем ошибку модуля RTC
  else { //иначе отправляем данные
    wireWrite(0x0F); //устанавливаем адрес записи
    wireWrite(ctrlReg); //отправляем настройку SQW
    wireEnd(); //конец передачи
    return 1; //выходим
  }
  return 0; //выходим
}
//-------------------------------Чтение температуры-------------------------------------
boolean readTempRTC(void) //чтение температуры
{
  if (wireRequestFrom(RTC_ADDR, 0x11)) return 0; //запрашиваем чтение данных, если нет ответа выходим
  uint16_t temp = (((uint16_t)wireRead() << 2 | wireReadEndByte() >> 6) * 10) >> 2;
  sens.temp = (temp > 850) ? 0 : temp;
  sens.press = 0; //сбросили давление
  sens.hum = 0; //сбросили влажность
  return 1; //выходим
}
//--------------------------------------Отправить время в RTC------------------------------------------
void sendTime(void) //отправить время в RTC
{
  RTC.DW = getWeekDay(RTC.YY, RTC.MM, RTC.DD); //получаем день недели
  if (wireBeginTransmission(RTC_ADDR)) SET_ERROR(DS3231_ERROR); //устанавливаем ошибку модуля RTC
  else { //иначе отправляем данные
    wireWrite(0x00); //устанавливаем адрес записи
    wireWrite(packREG(RTC.s)); //отправляем секунды
    wireWrite(packREG(RTC.m)); //отправляем минуты
    wireWrite(packREG(RTC.h)); //отправляем часы
    wireWrite(0); //пропускаем день недели
    wireWrite(packREG(RTC.DD)); //отправляем дату
    wireWrite(packREG(RTC.MM)); //отправляем месяц
    wireWrite(packREG(RTC.YY - 2000)); //отправляем год
    wireEnd(); //конец передачи
  }
}
//--------------------------------------Запрашиваем время из RTC------------------------------------------
boolean getTime(boolean mode) //запрашиваем время из RTC
{
  if (getOSF(mode)) { //проверка флага OSF
    if (wireRequestFrom(RTC_ADDR, 0x00)) { //запрашиваем чтение данных, если нет ответа выходим
      SET_ERROR(DS3231_ERROR); //устанавливаем ошибку модуля RTC
      return 0; //выходим
    }
    RTC.s = unpackREG(wireRead()); //получаем секунды
    RTC.m = unpackREG(wireRead()); //получаем минуты
    RTC.h = unpackREG(wireRead()); //получаем часы
    wireRead(); //пропускаем день недели
    RTC.DD = unpackREG(wireRead()); //получаем дату
    RTC.MM = unpackREG(wireRead()); //получаем месяц
    RTC.YY = unpackREG(wireReadEndByte()) + 2000; //получаем год
    RTC.DW = getWeekDay(RTC.YY, RTC.MM, RTC.DD); //получаем день недели
    return 1; //выходим
  }
  return 0; //выходим
}
