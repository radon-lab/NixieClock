#define RTC_ADDR 0x68 //адрес RTC DS3231

#define RTC_CHECK_OSF 0x00 //проверить флаг OSF
#define RTC_CLEAR_OSF 0x01 //проверить и очистить флаг OSF

#define RTC_TIME_REG 0x00 //регистр времени
#define RTC_CONTROL_REG 0x0E //регистр управления
#define RTC_STATUS_REG 0x0F //регистр состояния
#define RTC_AGING_REG 0x10 //регистр старения

enum {
  RTC_ONLINE, //модуль работает нормально
  RTC_BAT_LOW, //разряжена батарея
  RTC_NOT_FOUND //модуль не обнаружен
};

uint8_t rtc_status = RTC_NOT_FOUND; //состояние модуля RTC
int8_t rtc_aging = 0; //коррекция модуля RTC

const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //дней в месяце
//------------------------------Максимальное количество дней------------------------------
uint8_t maxDays(uint16_t YY, uint8_t MM) { //максимальное количество дней
  return (((MM == 2) && !(YY % 4)) ? 1 : 0) + daysInMonth[MM - 1]; //возвращаем количество дней в месяце
}
//---------------------------------Получить день недели-----------------------------------
uint8_t getWeekDay(uint16_t YY, uint8_t MM, uint8_t DD) { //получить день недели
  if (YY >= 2000) YY -= 2000; //если год больше 2000
  uint16_t days = DD; //записываем дату
  for (uint8_t i = 1; i < MM; i++) days += daysInMonth[i - 1]; //записываем сколько дней прошло до текущего месяца
  if ((MM > 2) && !(YY % 4)) days++; //если високосный год, прибавляем день
  return (days + 365 * YY + (YY + 3) / 4 - 2 + 6) % 7 + 1; //возвращаем день недели
}
//--------------------------------Получить летнее время-----------------------------------
boolean DST(uint8_t MM, uint8_t DD, uint8_t DW, uint8_t HH) { //получить летнее время
  if (MM < 3 || MM > 10) return 0; //зима
  switch (MM) {
    case 3:
      if (DD < 25) return 0; //зима
      else if (DW == 7) {
        if (HH >= 1) return 1; //лето
        else return 0; //зима
      }
      else if ((DD - 25) < DW) return 0; //зима
      else return 1; //лето
      break;
    case 10:
      if (DD < 25) return 1; //лето
      else if (DW == 7) {
        if (HH >= 2) return 0; //зима
        else return 1; //лето
      }
      else if ((DD - 25) < DW) return 1; //лето
      else return 0; //зима
      break;
  }
  return 1; //лето
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
//----------------------------------Онлайн статус rtc--------------------------------------
boolean rtcGetFoundStatus(void) {
  return (boolean)(rtc_status != RTC_NOT_FOUND);
}
//----------------------------Статус нормальной работы rtc---------------------------------
boolean rtcGetNormalStatus(void) {
  return (boolean)(rtc_status == RTC_ONLINE);
}
//-------------------------------Чтение коррекции хода-------------------------------------
boolean rtcReadAging(void) //чтение коррекции хода
{
  if (twi_requestFrom(RTC_ADDR, RTC_AGING_REG)) return 1; //модуль RTC не отвечает
  rtc_aging = twi_read_byte(TWI_NACK); //записываем результат
  twi_write_stop(); //конец передачи
  return 0; //выходим
}
//-------------------------------Запись коррекции хода-------------------------------------
boolean rtcWriteAging(void) //запись коррекции хода
{
  if (twi_beginTransmission(RTC_ADDR)) return 1; //модуль RTC не отвечает
  twi_write_byte(RTC_AGING_REG); //устанавливаем адрес записи
  twi_write_byte(rtc_aging); //записываем коррекцию хода
  twi_write_stop(); //остановка шины wire
  return 0; //выходим
}
//--------------------------------Проверка флага OSF--------------------------------------
uint8_t rtcGetOSF(boolean mode) //проверка флага OSF
{
  if (twi_requestFrom(RTC_ADDR, RTC_STATUS_REG)) return 1; //модуль RTC не отвечает

  uint8_t statReg = twi_read_byte(TWI_NACK); //прочитали регистр статуса

  if (statReg & 0x80) { //проверяем установлен ли флаг OSF
    rtc_status = RTC_BAT_LOW; //установили ошибку осцилятора модуля RTC
    if (mode) { //если нужно очистить флаг OSF
      if (twi_beginTransmission(RTC_ADDR)) return 1; //модуль RTC не отвечает
      twi_write_byte(RTC_STATUS_REG); //устанавливаем адрес записи
      twi_write_byte(statReg & 0x7F); //отправляем настройку OSF
    }
    return 2; //выходим
  }

  return 0; //выходим
}
//-----------------------------------Настройка SQW-----------------------------------------
boolean rtcSetSQW(void) //настройка SQW
{
  if (!twi_requestFrom(RTC_ADDR, RTC_CONTROL_REG)) { //запрашиваем чтение данных
    uint8_t ctrlReg = twi_read_byte(TWI_NACK); //выключаем INTCON и устанавливаем частоту 1Гц

    if (!twi_beginTransmission(RTC_ADDR)) { //отправляем данные
      twi_write_byte(RTC_CONTROL_REG); //устанавливаем адрес записи
      twi_write_byte(ctrlReg & 0x20); //отправляем настройку SQW
      return 0; //выходим
    }
  }
  return 1; //модуль RTC не отвечает
}
//-------------------------------Отключение вывода 32K-------------------------------------
boolean rtcDisable32K(void) //отключение вывода 32K
{
  if (!twi_requestFrom(RTC_ADDR, RTC_STATUS_REG)) { //запрашиваем чтение данных
    uint8_t ctrlReg = twi_read_byte(TWI_NACK); //выключаем 32K

    if (!twi_beginTransmission(RTC_ADDR)) { //отправляем данные
      twi_write_byte(RTC_STATUS_REG); //устанавливаем адрес записи
      twi_write_byte(ctrlReg & 0xF7); //отправляем настройку SQW
      return 0; //выходим
    }
  }
  return 1; //модуль RTC не отвечает
}
//--------------------------------------Отправить время в RTC------------------------------------------
boolean rtcSendTime(void) //отправить время в RTC
{
  if (!twi_beginTransmission(RTC_ADDR)) { //отправляем данные
    twi_write_byte(RTC_TIME_REG); //устанавливаем адрес записи
    twi_write_byte(packREG(mainTime.second)); //отправляем секунды
    twi_write_byte(packREG(mainTime.minute)); //отправляем минуты
    twi_write_byte(packREG(mainTime.hour)); //отправляем часы
    twi_write_byte(0); //пропускаем день недели
    twi_write_byte(packREG(mainDate.day)); //отправляем дату
    twi_write_byte(packREG(mainDate.month)); //отправляем месяц
    twi_write_byte(packREG(mainDate.year - 2000)); //отправляем год
    if (!twi_error()) { //если передача была успешной
      return 0; //выходим
    }
  }
  return 1; //модуль RTC не отвечает
}
//------------------------------------Запрашиваем время из RTC-----------------------------------------
uint8_t rtcGetTime(boolean mode) //запрашиваем время из RTC
{
  uint8_t status = rtcGetOSF(mode); //проверка флага OSF
  if (!status) { //если модуль на связи
    if (!twi_requestFrom(RTC_ADDR, RTC_TIME_REG)) { //запрашиваем чтение
      mainTime.second = unpackREG(twi_read_byte(TWI_ACK)); //получаем секунды
      mainTime.minute = unpackREG(twi_read_byte(TWI_ACK)); //получаем минуты
      mainTime.hour = unpackREG(twi_read_byte(TWI_ACK)); //получаем часы
      twi_read_byte(TWI_ACK); //пропускаем день недели
      mainDate.day = unpackREG(twi_read_byte(TWI_ACK)); //получаем дату
      mainDate.month = unpackREG(twi_read_byte(TWI_ACK)); //получаем месяц
      mainDate.year = unpackREG(twi_read_byte(TWI_NACK)) + 2000; //получаем год
      if (!twi_error()) { //если передача была успешной
        if (timeState != 0x03) climateTimer = 0; //обновляем состояние микроклимата
        timeState = 0x03; //установили флаги актуального времени
        return 0; //выходим
      }
    }
    return 1; //модуль RTC не отвечает
  }

  return status; //возвращаем статус
}
//-------------------------------------Инициализируем модуль RTC------------------------------------------
boolean rtcInitTime(void) //инициализируем модуль RTC
{
  static uint8_t attemptsRTC; //попытки инициализации модуля RTC

  if (deviceInformation[DS3231_ENABLE]) {
    rtc_status = RTC_NOT_FOUND;
    return 2; //выходим
  }

  if (attemptsRTC < 5) attemptsRTC++; //попытка запроса температуры
  else {
    rtc_status = RTC_NOT_FOUND;
    return 2; //выходим
  }

  if (rtcDisable32K()) { //отключение вывода 32K
    rtc_status = RTC_NOT_FOUND;
    return 1; //выходим
  }
  if (rtcSetSQW()) { //настройка SQW
    rtc_status = RTC_NOT_FOUND;
    return 1; //выходим
  }
  if (rtcReadAging()) { //чтение коррекции хода
    rtc_status = RTC_NOT_FOUND;
    return 1; //выходим
  }

  rtc_status = RTC_ONLINE; //модуль RTC работает нормально

  return rtcGetTime(RTC_CLEAR_OSF); //запрашиваем время из RTC
}
