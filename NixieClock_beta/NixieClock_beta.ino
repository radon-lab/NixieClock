/*
  Arduino IDE 1.8.13 версия прошивки бета 0.0.1 от 16.04.21
  Специльно для проекта "Часы на ГРИ v2"
  Исходник -
  Автор Radon-lab.
*/

//----------------Библиотеки----------------
#include <avr/eeprom.h>
#include <util/delay.h>

//---------------Конфигурации---------------
#include "config.h"
#include "connection.h"
#include "indiDisp.h"
#include "wire.h"
#include "UART.h"
#include "RTC.h"
#include "BME.h"

//переменные обработки кнопок
uint8_t btn_tmr; //таймер тиков обработки
boolean btn_check; //флаг разрешения опроса кнопки
boolean btn_state; //флаг текущего состояния кнопки

boolean _scr = 0; //флаг обновления экрана
uint8_t _mode = 0; //текущий основной режим

volatile uint8_t tick_wdt; //счетчик тиков для обработки данных
volatile uint8_t tick_sec; //счетчик тиков от RTC
uint32_t timer_millis; //таймер отсчета миллисекунд
uint32_t timer_dot; //таймер отсчета миллисекунд для точек

int atexit(void (* /*func*/ )()) { //инициализация функций
  return 0;
}

int main(void)  //инициализация
{
  WireInit(); //инициализация Wire
  dataChannelInit(9600); //инициализация UART
  indiInit(); //инициализация индикаторов

  DDRB = (1 << 1); //устанавливываем D9 как выход

  OCR1A = 130; //устанавливаем первичное значение
  TCCR1A = (1 << COM1A1 | 1 << WGM10);  //подключаем D9
  TCCR1B = (1 << CS10);  //задаем частоту ШИМ на 9 и 10 выводах 31 кГц

  if (eeprom_read_byte((uint8_t*)100) != 100) { //если первый запуск, восстанавливаем из переменных
    eeprom_update_byte((uint8_t*)100, 100); //делаем метку
    //eeprom_update_block((void*)&timeDefault, 0, sizeof(timeDefault)); //записываем дату по умолчанию в память
    //eeprom_update_byte((uint8_t*)11, _flask_mode); //записываем в память режим колбы
  }
  else {
    //eeprom_read_block((void*)&timeBright, 7, sizeof(timeBright)); //считываем время из памяти
    //_flask_mode = eeprom_read_byte((uint8_t*)11); //считываем режим колбы из памяти
  }

  getTime(); //запрашиваем время из RTC

  if (RTC_time.YY < 21 || RTC_time.YY > 50) { //если пропадало питание
    eeprom_read_block((void*)&RTC_time, 0, sizeof(RTC_time)); //считываем дату из памяти
    sendTime(); //отправить время в RTC
  }

  EICRA = (1 << ISC01); //настраиваем внешнее прерывание по спаду импульса на INT0
  EIMSK = (1 << INT0); //разрешаем внешнее прерывание INT0

  void WDT_enable(); //включение WDT

  for (timer_millis = 2000; timer_millis && !check_keys();) data_convert(); // ждем, преобразование данных
  //----------------------------------Главная-------------------------------------------------------------
  for (;;) //главная
  {
    data_convert(); //преобразование данных
    main_screen(); //главный экран
  }
  return 0; //конец
}
//-------------------------------Прерывание от RTC------------------------------------------------
ISR(INT0_vect) //внешнее прерывание на пине INT0 - считаем секунды с RTC
{
  tick_sec++; //прибавляем секунду
}
//-------------------------Прерывание по переполнению wdt - 17.5мс------------------------------------
ISR(WDT_vect) //прерывание по переполнению wdt - 17.5мс
{
  tick_wdt++; //прибавляем тик
}
//----------------------------------Преобразование данных---------------------------------------------------------
void data_convert(void) //преобразование данных
{
  for (; tick_sec > 0; tick_sec--) { //если был тик, обрабатываем данные
    //счет времени
    if (++RTC_time.s > 59) { //секунды
      RTC_time.s = 0;
      if (++RTC_time.m > 59) { //минуты
        RTC_time.m = 0;
        if (++RTC_time.h > 23) { //часы
          RTC_time.h = 0;
          if (++RTC_time.DW > 7) RTC_time.DW = 1;
          if (++RTC_time.DD > pgm_read_byte(&daysInMonth[RTC_time.MM - 1]) + (RTC_time.MM == 2 && !(RTC_time.YY % 4)) ? 1 : 0) {
            RTC_time.DD = 1;
            if (++RTC_time.MM > 12) {
              RTC_time.MM = 1;
              if (++RTC_time.YY > 2050) {
                RTC_time.YY = 2021;
              }
            }
          }
        }
      }
    }
    _scr = 0; //разрешаем обновить индикаторы
  }

  for (; tick_wdt > 0; tick_wdt--) { //если был тик, обрабатываем данные

    switch (btn_state) { //таймер опроса кнопок
      case 0: if (btn_check) btn_tmr++; break; //считаем циклы
      case 1: if (btn_tmr > 0) btn_tmr--; break; //убираем дребезг
    }

    if (timer_millis > 17) timer_millis -= 17; //если таймер больше 17мс
    else if (timer_millis) timer_millis = 0; //иначе сбрасываем таймер

    if (timer_dot > 17) timer_dot -= 17; //если таймер больше 17мс
    else if (timer_dot) timer_dot = 0; //иначе сбрасываем таймер
  }
}
//-------------------------------Включение WDT----------------------------------------------------
void WDT_enable(void) //включение WDT
{
  uint8_t sregCopy = SREG; //Сохраняем глобальные прерывания
  cli(); //Запрещаем глобальные прерывания
  WDTCSR = ((1 << WDCE) | (1 << WDE)); //Сбрасываем собаку
  WDTCSR = 0x40; //Устанавливаем пределитель 2(режим прерываний)
  SREG = sregCopy; //Восстанавливаем глобальные прерывания
}
//-------------------------------Выключение WDT---------------------------------------------------
void WDT_disable(void) //выключение WDT
{
  uint8_t sregCopy = SREG; //Сохраняем глобальные прерывания
  cli(); //Запрещаем глобальные прерывания
  WDTCSR = ((1 << WDCE) | (1 << WDE)); //Сбрасываем собаку
  WDTCSR = 0x00; //Выключаем собаку
  SREG = sregCopy; //Восстанавливаем глобальные прерывания
}
//-----------------------------Проверка кнопок----------------------------------------------------
uint8_t check_keys(void) //проверка кнопок
{
  static uint8_t btn_set; //флаг признака действия
  static uint8_t btn_switch; //флаг мультиопроса кнопок

  switch (btn_switch) { //переключаемся в зависимости от состояния мультиопроса
    case 0:
      if (!OK_OUT) { //если нажата кл. ок
        btn_switch = 1; //выбираем клавишу опроса
        btn_state = 0; //обновляем текущее состояние кнопки
      }
      else if (!DOWN_OUT) { //если нажата кл. вниз
        btn_switch = 2; //выбираем клавишу опроса
        btn_state = 0; //обновляем текущее состояние кнопки
      }
      else if (!UP_OUT) { //если нажата кл. вверх
        btn_switch = 3; //выбираем клавишу опроса
        btn_state = 0; //обновляем текущее состояние кнопки
      }
      else btn_state = 1; //обновляем текущее состояние кнопки
      break;
    case 1: btn_state = OK_OUT; break; //опрашиваем клавишу ок
    case 2: btn_state = DOWN_OUT; break; //опрашиваем клавишу вниз
    case 3: btn_state = UP_OUT; break; //опрашиваем клавишу вверх
  }

  switch (btn_state) { //переключаемся в зависимости от состояния клавиши
    case 0:
      if (btn_check) { //если разрешена провекрка кнопки
        if (btn_tmr > BTN_HOLD_TICK) { //если таймер больше длительности удержания кнопки
          btn_tmr = BTN_GIST_TICK; //сбрасываем таймер на антидребезг
          btn_set = 2; //поднимаем признак удержания
          btn_check = 0; //запрещем проврку кнопки
        }
      }
      break;

    case 1:
      if (btn_tmr > BTN_GIST_TICK) { //если таймер больше времени антидребезга
        btn_tmr = BTN_GIST_TICK; //сбрасываем таймер на антидребезг
        btn_set = 1; //если не спим и если подсветка включена, поднимаем признак нажатия
        btn_check = 0; //запрещем проврку кнопки
        //if (!knock_disable) buzz_pulse(FREQ_BEEP, TIME_BEEP); //щелчок пищалкой
      }
      else if (!btn_tmr) {
        btn_check = 1; //разрешаем проврку кнопки
        btn_switch = 0; //сбрасываем мультиопрос кнопок
      }
      break;
  }

  switch (btn_set) { //переключаемся в зависимости от признака нажатия
    case 0: return 0; //клавиша не нажата, возвращаем 0
    case 1:
      btn_set = 0; //сбрасываем признак нажатия
      switch (btn_switch) { //переключаемся в зависимости от состояния мультиопроса
        case 1: return 5; //ok press, возвращаем 5
        case 2: return 2; //down press, возвращаем 2
        case 3: return 3; //up press, возвращаем 3
      }
      break;

    case 2:
      btn_set = 0; //сбрасываем признак нажатия
      switch (btn_switch) { //переключаемся в зависимости от состояния мультиопроса
        case 1: return 6; //ok hold, возвращаем 6
        case 2: return 1; //down hold, возвращаем 1
        case 3: return 4; //up hold, возвращаем 4
      }
      break;
  }
}
//----------------------------------------------------------------------------------
void settings_time(void)
{
  uint8_t cur_mode = 0; //текущий режим
  boolean blink_data = 0; //мигание сигментами

  indiClr(); //очищаем индикаторы
  for (timer_millis = TIME_MSG; timer_millis && !check_keys();) data_convert(); // ждем, преобразование данных

  //настройки
  while (1) {
    data_convert(); //преобразование данных

    if (!_scr) {
      _scr = 1; //сбрасываем флаг
      indiClr(); //очистка индикаторов
      switch (cur_mode) {
        case 0:
        case 1:
          if (!blink_data || cur_mode == 1) indiPrintNum(RTC_time.h, 0, 2, '0'); //вывод часов
          if (!blink_data || cur_mode == 0) indiPrintNum(RTC_time.m, 2, 2, '0'); //вывод минут
          break;
        case 2:
        case 3:
          if (!blink_data || cur_mode == 3) indiPrintNum(RTC_time.DD, 0, 2, '0'); //вывод даты
          if (!blink_data || cur_mode == 2) indiPrintNum(RTC_time.MM, 2, 2, '0'); //вывод месяца
          break;
        case 4:
          if (!blink_data) indiPrintNum(RTC_time.YY, 0); //вывод года
          break;
      }
      blink_data = !blink_data; //мигание сигментами
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (check_keys()) {
      case 1: //left click
        switch (cur_mode) {
          //настройка времени
          case 0: if (RTC_time.h > 0) RTC_time.h--; else RTC_time.h = 23; break; //часы
          case 1: if (RTC_time.m > 0) RTC_time.m--; else RTC_time.m = 59; break; //минуты

          //настройка даты
          case 2: if (RTC_time.DD > 1 ) RTC_time.DD--; else RTC_time.DD = (RTC_time.MM == 2 && !(RTC_time.YY % 4)) ? 1 : 0 + daysInMonth[RTC_time.MM - 1]; break; //день
          case 3: if (RTC_time.MM > 1) RTC_time.MM--; else RTC_time.MM = 12; RTC_time.DD = 1; break; //месяц

          //настройка года
          case 4: if (RTC_time.YY > 2021) RTC_time.YY--; else RTC_time.YY = 2050; break; //год
        }
        _scr = blink_data = 0; //сбрасываем флаги
        break;

      case 2: //right click
        switch (cur_mode) {
          //настройка времени
          case 0: if (RTC_time.h < 23) RTC_time.h++; else RTC_time.h = 0; break; //часы
          case 1: if (RTC_time.m < 59) RTC_time.m++; else RTC_time.m = 0; break; //минуты

          //настройка даты
          case 2: if (RTC_time.DD < daysInMonth[RTC_time.MM - 1] + (RTC_time.MM == 2 && !(RTC_time.YY % 4)) ? 1 : 0) RTC_time.DD++; else RTC_time.DD = 1; break; //день
          case 3: if (RTC_time.MM < 12) RTC_time.MM++; else RTC_time.MM = 1; RTC_time.DD = 1; break; //месяц

          //настройка года
          case 4: if (RTC_time.YY < 2050) RTC_time.YY++; else RTC_time.YY = 2021; break; //год
        }
        _scr = blink_data = 0; //сбрасываем флаги
        break;

      case 3: //left hold
        if (cur_mode < 4) cur_mode++; else cur_mode = 0;
        _scr = blink_data = 0; //сбрасываем флаги
        break;

      case 4: //right hold
        RTC_time.s = 0;
        eeprom_read_block((void*)&RTC_time, 0, sizeof(RTC_time)); //считываем дату из памяти
        sendTime(); //отправить время в RTC
        indiClr(); //очистка индикаторов
        for (timer_millis = TIME_MSG; timer_millis && !check_keys();) data_convert(); // ждем, преобразование данных
        _scr = 0; //обновляем экран
        return;
    }
  }
}
//----------------------------------------------------------------------------------
boolean changeBright(void) { // установка яркости от времени суток
  if ((timeBright[0] > timeBright[1] && (RTC_time.h >= timeBright[0] || RTC_time.h < timeBright[1])) ||
      (timeBright[0] < timeBright[1] && RTC_time.h >= timeBright[0] && RTC_time.h < timeBright[1])) {
    return 0;
  } else {
    return 1;
  }
}
//----------------------------------------------------------------------------------
void dotFlash(void) {
  if (!timer_dot) {
    //dot_state = !dot_state; //инвертируем точки
    timer_dot = DOT_TIME;
  }
}
//-----------------------------Главный экран------------------------------------------------
void main_screen(void) //главный экран
{
  if (!_scr) {
    _scr = 1; //сбрасываем флаг
    switch (_mode) {
      case 0:
        indiPrintNum(RTC_time.h, 0, 2, '0'); //вывод часов
        indiPrintNum(RTC_time.m, 2, 2, '0'); //вывод минут
        break;
      case 2:
        indiPrintNum(RTC_time.DD, 0, 2, '0'); //вывод даты
        indiPrintNum(RTC_time.MM, 2, 2, '0'); //вывод месяца
        break;
    }
  }

  switch (_mode) {
    case 0: //режим часов
      dotFlash(); //мигаем точками
      break;

    case 2:

      break;
  }

  switch (check_keys()) {
    case 1: //left key press

      _scr = 0; //обновление экрана
      break;

    case 2: //right key press

      _scr = 0; //обновление экрана
      break;

    case 3: //left key hold

      _scr = 0; //обновление экрана
      break;

    case 4: //right key hold

      _scr = 0; //обновление экрана
      break;
  }
}
