/*
  Arduino IDE 1.8.13 версия прошивки бета 0.0.2 от 16.04.21
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

struct alarm1 {
  uint8_t hh = 15;
  uint8_t mm = 25;
  uint8_t mode = 0;
  uint8_t days = 3;
} dataAlarm1;

struct alarm2 {
  uint8_t hh = 15;
  uint8_t mm = 25;
  uint8_t mode = 2;
  uint8_t days = 3;
} dataAlarm2;

struct alarm3 {
  uint8_t hh = 15;
  uint8_t mm = 25;
  uint8_t mode = 1;
  uint8_t days = 3;
} dataAlarm3;

struct alarm4 {
  uint8_t hh = 15;
  uint8_t mm = 25;
  uint8_t mode = 1;
  uint8_t days = 3;
} dataAlarm4;

struct alarm5 {
  uint8_t hh = 15;
  uint8_t mm = 25;
  uint8_t mode = 2;
  uint8_t days = 3;
} dataAlarm5;

int atexit(void (* /*func*/ )()) { //инициализация функций
  return 0;
}

int main(void)  //инициализация
{
  OK_INIT;
  LEFT_INIT;
  RIGHT_INIT;
  CONV_INIT;
  SQW_INIT;
  DOT_INIT;
  LIGHT_INIT;
  BUZZ_INIT;

  WireInit(); //инициализация Wire
  dataChannelInit(9600); //инициализация UART
  indiInit(); //инициализация индикаторов

  OCR1A = 130; //устанавливаем первичное значение
  TCCR1A = (1 << COM1A1 | 1 << WGM10);  //подключаем D9
  TCCR1B = (1 << CS10);  //задаем частоту ШИМ на 9 и 10 выводах 31 кГц

  EICRA = (1 << ISC01); //настраиваем внешнее прерывание по спаду импульса на INT0
  EIMSK = (1 << INT0); //разрешаем внешнее прерывание INT0

  setSQW(); //установка SQW на 1Гц
  getTime(); //запрашиваем время из RTC

  if (eeprom_read_byte((uint8_t*)100) != 100) { //если первый запуск, восстанавливаем из переменных
    eeprom_update_byte((uint8_t*)100, 100); //делаем метку
    eeprom_update_block((void*)&RTC_time, 0, sizeof(RTC_time)); //записываем дату по умолчанию в память
    //eeprom_update_byte((uint8_t*)11, _flask_mode); //записываем в память режим колбы
  }
  else {
    //eeprom_read_block((void*)&timeBright, 7, sizeof(timeBright)); //считываем время из памяти
    //_flask_mode = eeprom_read_byte((uint8_t*)11); //считываем режим колбы из памяти
  }

  if (RTC_time.YY < 2021 || RTC_time.YY > 2050) { //если пропадало питание
    eeprom_read_block((void*)&RTC_time, 0, sizeof(RTC_time)); //считываем дату из памяти
    sendTime(); //отправить время в RTC
  }

  WDT_enable(); //включение WDT
  //----------------------------------Главная-------------------------------------------------------------
  for (;;) //главная
  {
    data_convert(); //преобразование данных
    sincData(); //синхронизация данных
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
//-------------------------------Синхронизация данных---------------------------------------------------
void sincData(void) //синхронизация данных
{
  if (availableData()) {
    uint8_t command = readData();
    switch (command) {
      case 0x50: comSendTime(); break;
      case 0x51: comGetTime(); break;
      case 0x52: comSendAlarm(); break;
      case 0x53: comGetAlarm(); break;
      default:
        sendCommand(0x00);
        clearBuffer(); //очистить буфер приёма
        break;
    }
  }
}
//--------------------------------------------------------------------------------------
void comGetAlarm(void)
{
  uint8_t dataBuf[20];
  uint16_t crc = 0;
  uint16_t crcData = 0;

  if (availableData() == 22) {
    for (uint8_t c = 0; c < 20; c += 4) {
      for (uint8_t i = c; i < c + 4; i++) {
        dataBuf[i] = readData();
        crcData += (uint16_t)dataBuf[i] * (i + 2);
      }
    }

    for (uint8_t i = 0; i < 2; i++) *((uint8_t*)&crc + i) = readData();

    if (crc == crcData) {
      for (uint8_t i = 0; i < 4; i++) {
        *((uint8_t*)&dataAlarm1 + i) = dataBuf[i];
        *((uint8_t*)&dataAlarm2 + i) = dataBuf[i + 4];
        *((uint8_t*)&dataAlarm3 + i) = dataBuf[i + 8];
        *((uint8_t*)&dataAlarm4 + i) = dataBuf[i + 12];
        *((uint8_t*)&dataAlarm5 + i) = dataBuf[i + 16];
      }
      sendCommand(0xFF);
    }
    else sendCommand(0x02);
  }
  else {
    sendCommand(0x01);
    clearBuffer(); //очистить буфер приёма
  }
}
//--------------------------------------------------------------------------------------
void comGetTime(void)
{
  uint8_t dataBuf[sizeof(RTC_time)];
  uint16_t crc = 0;
  uint16_t crcData = 0;

  if (availableData() == sizeof(RTC_time) + 2) {
    for (uint8_t i = 0; i < sizeof(RTC_time); i++) {
      dataBuf[i] = readData();
      crcData += (uint16_t)dataBuf[i] * (i + 2);
    }

    for (uint8_t i = 0; i < 2; i++) *((uint8_t*)&crc + i) = readData();

    if (crc == crcData) {
      for (uint8_t i = 0; i < sizeof(RTC_time); i++) *((uint8_t*)&RTC_time + i) = dataBuf[i];
      sendCommand(0xFF);
    }
    else sendCommand(0x02);
  }
  else {
    sendCommand(0x01);
    clearBuffer(); //очистить буфер приёма
  }
}
//--------------------------------------------------------------------------------------
void comSendAlarm(void)
{
  uint8_t buf[20];

  for (uint8_t i = 0; i < 4; i++) {
    buf[i] = *((uint8_t*)&dataAlarm1 + i);
    buf[i + 4] = *((uint8_t*)&dataAlarm2 + i);
    buf[i + 8] = *((uint8_t*)&dataAlarm3 + i);
    buf[i + 12] = *((uint8_t*)&dataAlarm4 + i);
    buf[i + 16] = *((uint8_t*)&dataAlarm5 + i);
  }
  sendData(0x11, buf, 20);
}
//--------------------------------------------------------------------------------------
void comSendTime(void)
{
  //  uint8_t buf[sizeof(dataTime)];
  //
  //  for (uint8_t i = 0; i < sizeof(dataTime); i++) buf[i] = *((uint8_t*)&dataTime + i);
  sendData(0x10, (uint8_t*)&RTC_time, sizeof(RTC_time));
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
      else if (!LEFT_OUT) { //если нажата левая кл.
        btn_switch = 2; //выбираем клавишу опроса
        btn_state = 0; //обновляем текущее состояние кнопки
      }
      else if (!RIGHT_OUT) { //если нажата правая кл.
        btn_switch = 3; //выбираем клавишу опроса
        btn_state = 0; //обновляем текущее состояние кнопки
      }
      else btn_state = 1; //обновляем текущее состояние кнопки
      break;
    case 1: btn_state = OK_OUT; break; //опрашиваем клавишу ок
    case 2: btn_state = LEFT_OUT; break; //опрашиваем левую клавишу
    case 3: btn_state = RIGHT_OUT; break; //опрашиваем правую клавишу
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
        case 2: return 2; //left press, возвращаем 2
        case 3: return 3; //right press, возвращаем 3
      }
      break;

    case 2:
      btn_set = 0; //сбрасываем признак нажатия
      switch (btn_switch) { //переключаемся в зависимости от состояния мультиопроса
        case 1: return 6; //ok hold, возвращаем 6
        case 2: return 1; //left hold, возвращаем 1
        case 3: return 4; //right hold, возвращаем 4
      }
      break;
  }
  return 0;
}
//----------------------------------------------------------------------------------
void settings_time(void)
{
  uint8_t cur_mode = 0; //текущий режим
  boolean blink_data = 0; //мигание сигментами

  indiClr(); //очищаем индикаторы
  DOT_ON;

  //настройки
  while (1) {
    data_convert(); //преобразование данных

    if (!_scr) {
      _scr = 1; //сбрасываем флаг
      indiClr(); //очистка индикаторов
      switch (cur_mode) {
        case 0:
        case 1:
          if (!blink_data || cur_mode == 1) indiPrintNum(RTC_time.h, 0, 2, 0); //вывод часов
          if (!blink_data || cur_mode == 0) indiPrintNum(RTC_time.m, 2, 2, 0); //вывод минут
          break;
        case 2:
        case 3:
          if (!blink_data || cur_mode == 3) indiPrintNum(RTC_time.DD, 0, 2, 0); //вывод даты
          if (!blink_data || cur_mode == 2) indiPrintNum(RTC_time.MM, 2, 2, 0); //вывод месяца
          break;
        case 4:
          if (!blink_data) indiPrintNum(RTC_time.YY, 0); //вывод года
          break;
      }
      blink_data = !blink_data; //мигание сигментами
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (check_keys()) {
      case 2: //left click
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

      case 3: //right click
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

      case 5: //ok click
        if (cur_mode < 4) cur_mode++; else cur_mode = 0;
        if (cur_mode != 4) DOT_ON;
        else DOT_OFF;
        _scr = blink_data = 0; //сбрасываем флаги
        break;

      case 6: //ok hold
        RTC_time.s = 0;
        eeprom_update_block((void*)&RTC_time, 0, sizeof(RTC_time)); //записываем дату по умолчанию в память
        sendTime(); //отправить время в RTC
        indiClr(); //очистка индикаторов
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
    DOT_INV; //инвертируем точки
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
        indiPrintNum(RTC_time.h, 0, 2, 0); //вывод часов
        indiPrintNum(RTC_time.m, 2, 2, 0); //вывод минут
        break;
      case 2:
        indiPrintNum(RTC_time.DD, 0, 2, 0); //вывод даты
        indiPrintNum(RTC_time.MM, 2, 2, 0); //вывод месяца
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
    case 1: //left key hold

      _scr = 0; //обновление экрана
      break;

    case 2: //left key press

      _scr = 0; //обновление экрана
      break;

    case 3: //right key press

      _scr = 0; //обновление экрана
      break;

    case 4: //right key hold

      _scr = 0; //обновление экрана
      break;

    case 5: //ok key press

      _scr = 0; //обновление экрана
      break;

    case 6: //ok key hold
      settings_time();
      _scr = 0; //обновление экрана
      break;
  }
}
