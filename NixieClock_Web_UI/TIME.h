#include <time.h>

GPdate mainDate; //основная дата
GPtime mainTime; //основное время

uint8_t time_state = 0; //флаг состояния актуальности времени
uint32_t time_timer = 0; //таймер счета секундных интервалов

enum {
  TIME_EVENT_START, //событие первого запуска
  TIME_EVENT_SECOND, //событие раз в секунду
  TIME_EVENT_MINUTE, //событие раз в минуту
  TIME_EVENT_HOUR, //событие раз в час
  TIME_EVENT_DAY //событие раз в сутки
};

void timeEvent(uint8_t event); //процедура обработки событий

const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //дней в месяце
//------------------------------Максимальное количество дней------------------------------
uint8_t timeGetMaxDays(uint16_t YY, uint8_t MM) { //максимальное количество дней
  return (((MM == 2) && !(YY % 4)) ? 1 : 0) + daysInMonth[MM - 1]; //возвращаем количество дней в месяце
}
//---------------------------------Получить день недели-----------------------------------
uint8_t timeGetWeekDay(uint16_t YY, uint8_t MM, uint8_t DD) { //получить день недели
  if (YY >= 2000) YY -= 2000; //если год больше 2000
  uint16_t days = DD; //записываем дату
  for (uint8_t i = 1; i < MM; i++) days += daysInMonth[i - 1]; //записываем сколько дней прошло до текущего месяца
  if ((MM > 2) && !(YY % 4)) days++; //если високосный год, прибавляем день
  return (days + 365 * YY + (YY + 3) / 4 - 2 + 6) % 7 + 1; //возвращаем день недели
}
//--------------------------------Получить летнее время-----------------------------------
boolean timeGetDST(uint8_t MM, uint8_t DD, uint8_t DW, uint8_t HH) { //получить летнее время
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
//--------------------------------Установить летнее время-----------------------------------
void timeSetDST(void) {
  if (mainTime.hour != 23) mainTime.hour += 1; //прибавили час
  else {
    mainTime.hour = 0; //сбросили час
    if (++mainDate.day > timeGetMaxDays(mainDate.year, mainDate.month)) { //день
      mainDate.day = 1; //сбросили день
      if (++mainDate.month > 12) { //месяц
        mainDate.month = 1; //сбросили месяц
        if (++mainDate.year > 2099) { //год
          mainDate.year = 2000; //сбросили год
        }
      }
    }
  }
}
//--------------------------------------------------------------------
boolean timeGetValidState(void) {
  return (boolean)(time_state != 0x03);
}
//--------------------------------------------------------------------
uint8_t timeGetState(void) {
  return time_state;
}
//--------------------------------------------------------------------
void timeSetState(uint8_t state) {
  time_state |= state;
}
//--------------------------------------------------------------------
void timeSetMillis(uint32_t ms) {
  time_timer = ms;
}
//--------------------------------------------------------------------
void timeSetUnix(time_t unix) {
  tm time; //буфер времени
  gmtime_r(&unix, &time);

  mainTime.second = time.tm_sec;
  mainTime.minute = time.tm_min;
  mainTime.hour = time.tm_hour;

  mainDate.day = time.tm_mday;
  mainDate.month = time.tm_mon + 1;
  mainDate.year = time.tm_year + 1900;
}
//--------------------------------------------------------------------
void timeUpdate(void) {
  if ((millis() - time_timer) >= 1000) { //если прошла секунда
    if (!time_timer) time_timer = millis(); //инициализировали таймер
    else { //счет времени
      if (++mainTime.second > 59) { //секунды
        mainTime.second = 0; //сбросили секунды
        if (++mainTime.minute > 59) { //минуты
          mainTime.minute = 0; //сбросили минуты
          if (++mainTime.hour > 23) { //часы
            mainTime.hour = 0; //сбросили часы
            if (++mainDate.day > timeGetMaxDays(mainDate.year, mainDate.month)) { //дата
              mainDate.day = 1; //сбросили день
              if (++mainDate.month > 12) { //месяц
                mainDate.month = 1; //сбросили месяц
                if (++mainDate.year > 2099) { //год
                  mainDate.year = 2000; //сбросили год
                }
              }
            }
            timeEvent(TIME_EVENT_DAY); //событие раз в сутки
          }
          timeEvent(TIME_EVENT_HOUR); //событие раз в час
        }
        timeEvent(TIME_EVENT_MINUTE); //событие раз в минуту
      }
      timeEvent(TIME_EVENT_SECOND); //событие раз в секунду

      time_timer += 1000; //прибавили секунду
      return; //выходим
    }
    timeEvent(TIME_EVENT_START); //событие первого запуска
  }
}
