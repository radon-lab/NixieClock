/*
  Arduino IDE 1.8.13 версия прошивки бета 0.1.6 от 28.04.21
  Специльно для проекта "Часы на ГРИ и Arduino v2 | AlexGyver"
  Страница проекта - https://alexgyver.ru/nixieclock_v2

  Исходник - https://github.com/radon-lab/NixieClock
  Автор Radon-lab.
*/
//--------------Версия прошивки-------------
#define VERSION_FW 0x67

//----------------Библиотеки----------------
#include <avr/eeprom.h>

//---------------Конфигурации---------------
#include "config.h"
#include "connection.h"
#include "indiDisp.h"
#include "wire.h"
#include "UART.h"
#include "BME.h"
#include "RTC.h"

//----------------Настройки----------------
struct Settings_1 {
  uint8_t timeBright[2] = {DEFAULT_NIGHT_START, DEFAULT_NIGHT_END};
  uint8_t indiBright[2] = {DEFAULT_INDI_BRIGHT_N, DEFAULT_INDI_BRIGHT};
  uint8_t backlBright[2] = {DEFAULT_BACKL_BRIGHT_N, DEFAULT_BACKL_BRIGHT};
  uint8_t backlMinBright = DEFAULT_BACKL_MIN_BRIGHT;
  uint8_t backlMode = DEFAULT_BACKL_MODE;
  uint16_t backlTime = DEFAULT_BACKL_TIME;
  uint8_t backlStep = DEFAULT_BACKL_STEP;
  uint8_t backlPause = DEFAULT_BACKL_PAUSE;
  uint8_t dotBright[2] = {DEFAULT_DOT_BRIGHT_N, DEFAULT_DOT_BRIGHT};
  uint8_t dotMode = DEFAULT_DOT_MODE;
  uint8_t dotTimer = DEFAULT_DOT_TIMER;
  uint16_t dotTime = DEFAULT_DOT_TIME;
} brightSettings;

struct Settings_2 {
  uint8_t timeHour[2] = {DEFAULT_HOUR_SOUND_START, DEFAULT_HOUR_SOUND_END};
  uint8_t flipMode = DEFAULT_FLIP_ANIM;
  boolean timeFormat = DEFAULT_TIME_FORMAT;
  boolean knock_sound = DEFAULT_KNOCK_SOUND;
  uint8_t sensorSet = DEFAULT_TEMP_SENSOR;
  int8_t tempCorrect = DEFAULT_TEMP_CORRECT;
  uint8_t almMelody = DEFAULT_ALM_MELODY;
  uint8_t almTimeOutSound = DEFAULT_ALM_TIMEOUT_SOUND;
  uint8_t almTimeOut = DEFAULT_ALM_TIMEOUT;
  uint8_t almWaint = DEFAULT_ALM_WAINT;
  boolean glitchMode = DEFAULT_GLITCH_MODE;
  uint8_t glitchMin = DEFAULT_GLITCH_MIN;
  uint8_t glitchMax = DEFAULT_GLITCH_MAX;
  uint8_t burnTime = DEFAULT_BURN_TIME;
  uint8_t burnLoops = DEFAULT_BURN_LOOPS;
  uint8_t burnPeriod = DEFAULT_BURN_PERIOD;
} mainSettings;


uint8_t semp = 0; //переключатель мелодии
#define MELODY_PLAY(melody) _melody_chart(melody)
#define MELODY_RESET semp = 0

//переменные обработки кнопок
uint8_t btn_tmr; //таймер тиков обработки
boolean btn_check; //флаг разрешения опроса кнопки
boolean btn_state; //флаг текущего состояния кнопки

boolean _animShow = 0; //флаг анимации
boolean _scr = 0; //флаг обновления экрана
uint8_t _mode = 0; //текущий основной режим

#define TIMERS_NUM 6 //количество таймеров
uint32_t _timer_ms[TIMERS_NUM]; //таймер отсчета миллисекунд

#define TMR_MS     0 //таймер общего назначения
#define TMR_MELODY 1 //таймер мелодий
#define TMR_BACKL  2 //таймер подсветки
#define TMR_DOT    3 //таймер точек
#define TMR_ANIM   4 //таймер анимаций
#define TMR_GLITCH 5 //таймер глюков

#define LEFT_KEY_PRESS  2 //клик левой кнопкой
#define LEFT_KEY_HOLD   1 //удержание левой кнопки
#define RIGHT_KEY_PRESS 3 //клик правой кнопкой
#define RIGHT_KEY_HOLD  4 //удержание правой кнопки
#define SET_KEY_PRESS   5 //клик средней кнопкой
#define SET_KEY_HOLD    6 //удержание средней кнопки

volatile uint16_t cnt_puls; //количество циклов для работы пищалки
volatile uint16_t cnt_freq; //частота для генерации звука пищалкой
uint16_t tmr_score; //частота для генерации звука пищалкой

#define FLIP_EFFECT_NUM 6 //количество эффектов
uint16_t FLIP_SPEED[] = {DEFAULT_FLIP_TIME, 80, 80, 80, 80}; //скорость эффектов,(мс)

uint8_t dotBrightStep;
uint8_t dotMaxBright;
uint8_t backlBrightStep;
uint8_t backlMaxBright;
uint8_t indiMaxBright;

#define ALARMS_NUM 5 //количество будильников
uint8_t alarms[ALARMS_NUM][4] = {
  {15, 25, 0, 3}, //час | минута | режим(0 - выкл, 2 - одиночный, 1 - вкл) | день недели(вс,сб,пт,чт,ср,вт,пн,null)
  {15, 25, 0, 3}, //час | минута | режим(0 - выкл, 2 - одиночный, 1 - вкл) | день недели(вс,сб,пт,чт,ср,вт,пн,null)
  {15, 25, 0, 3}, //час | минута | режим(0 - выкл, 2 - одиночный, 1 - вкл) | день недели(вс,сб,пт,чт,ср,вт,пн,null)
  {15, 25, 0, 3}, //час | минута | режим(0 - выкл, 2 - одиночный, 1 - вкл) | день недели(вс,сб,пт,чт,ср,вт,пн,null)
  {15, 25, 0, 3}  //час | минута | режим(0 - выкл, 2 - одиночный, 1 - вкл) | день недели(вс,сб,пт,чт,ср,вт,пн,null)
};
boolean alarmWaint = 0;
uint8_t alarm = 0;
uint8_t minsAlarm = 0;
uint8_t minsAlarmSound = 0;
uint8_t minsAlarmWaint = 0;

uint8_t _tmrBurn = 0;
uint8_t _tmrGlitch = 0;

int atexit(void (* /*func*/ )()) { //инициализация функций
  return 0;
}
//----------------------------------Инициализация-------------------------------------------------------------
int main(void)  //инициализация
{
  OK_INIT;
  LEFT_INIT;
  RIGHT_INIT;
  CONV_INIT;
  SQW_INIT;
  DOT_INIT;
  BACKL_INIT;
  BUZZ_INIT;

  WireInit(); //инициализация Wire
  dataChannelInit(BT_UART_BAUND); //инициализация UART
  indiInit(); //инициализация индикаторов

  if (eeprom_read_byte((uint8_t*)EEPROM_BLOCK_VERSION_FW) != VERSION_FW) { //если первый запуск, восстанавливаем из переменных
    eeprom_update_byte((uint8_t*)EEPROM_BLOCK_VERSION_FW, VERSION_FW); //делаем метку версии прошивки
    eeprom_update_block((void*)&RTC_time, (void*)EEPROM_BLOCK_TIME, sizeof(RTC_time)); //записываем дату и время в память
    eeprom_update_block((void*)&alarms, (void*)EEPROM_BLOCK_ALARM, sizeof(alarms)); //записываем будильники в память
    eeprom_update_block((void*)&brightSettings, (void*)EEPROM_BLOCK_SETTINGS_BRIGHT, sizeof(brightSettings)); //записываем настройки яркости в память
    eeprom_update_block((void*)&mainSettings, (void*)EEPROM_BLOCK_SETTINGS_MAIN, sizeof(mainSettings)); //записываем основные настройки в память
  }
  else {
    eeprom_read_block((void*)&alarms, (void*)EEPROM_BLOCK_ALARM, sizeof(alarms)); //считываем будильники из памяти
    eeprom_read_block((void*)&brightSettings, (void*)EEPROM_BLOCK_SETTINGS_BRIGHT, sizeof(brightSettings)); //считываем настройки яркости из памяти
    eeprom_read_block((void*)&mainSettings, (void*)EEPROM_BLOCK_SETTINGS_MAIN, sizeof(mainSettings)); //считываем основные настройки из памяти
  }

  EICRA = (1 << ISC01); //настраиваем внешнее прерывание по спаду импульса на INT0
  EIMSK = (1 << INT0); //разрешаем внешнее прерывание INT0

  setSQW(); //установка SQW на 1Гц
  getTime(); //запрашиваем время из RTC

  if (RTC_time.YY < 2021 || RTC_time.YY > 2050) { //если пропадало питание
    eeprom_read_block((void*)&RTC_time, 0, sizeof(RTC_time)); //считываем дату и время из памяти
    sendTime(); //отправить время в RTC
  }

  randomSeed(RTC_time.s * (RTC_time.m + RTC_time.h) + RTC_time.DD * RTC_time.MM); //радомный сид для глюков
  _tmrGlitch = random(mainSettings.glitchMin, mainSettings.glitchMax); //находим рандомное время появления глюка
  changeBright(); //установка яркости от времени суток
  //----------------------------------Главная-------------------------------------------------------------
  for (;;) //главная
  {
    data_convert(); //обработка данных
    alarmWarn(); //тревога будильника
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
//---------------------------------Прерывание сигнала для пищалки---------------------------------------
ISR(TIMER2_COMPB_vect) //прерывание сигнала для пищалки
{
  if (cnt_freq > 255) cnt_freq -= 255; //считаем циклы полуволны
  else if (cnt_freq) { //если остался хвост
    OCR2B = cnt_freq; //устанавливаем хвост
    cnt_freq = 0; //сбрасываем счетчик циклов полуволны
  }
  else { //если циклы полуволны кончились
    OCR2B = 255; //устанавливаем COMB в начало
    cnt_freq = tmr_score; //устанавливаем циклов полуволны
    BUZZ_INV; //инвертируем бузер
    if (!--cnt_puls) { //считаем циклы времени работы бузера
      BUZZ_OFF; //если циклы кончились, выключаем бузер
      TIMSK2 = 0; //выключаем таймер
    }
  }
}
//--------------------------------Генерация частот бузера----------------------------------
void buzz_pulse(uint16_t freq, uint16_t time) //генерация частоты бузера (частота 10..10000, длительность мс.)
{
  cnt_puls = ((uint32_t)freq * (uint32_t)time) / 500; //пересчитываем частоту и время в циклы таймера
  cnt_freq = tmr_score = (1000000 / freq); //пересчитываем частоту в циклы полуволны
  OCR2B = 255; //устанавливаем COMB в начало
  TIMSK2 = (0x01 << OCIE2B); //запускаем таймер
}
//---------------------------------Воспроизведение мелодии---------------------------------------
void _melody_chart(uint8_t melody) //воспроизведение мелодии
{
  if (!_timer_ms[TMR_MELODY]) { //если пришло время
    buzz_pulse(pgm_read_word((uint16_t*)(alarm_sound[melody][0] + (6 * semp) + 0)), pgm_read_word((uint16_t*)(alarm_sound[melody][0] + (6 * semp) + 2))); //запускаем звук с задоной частотой и временем
    _timer_ms[TMR_MELODY] = pgm_read_word((uint16_t*)(alarm_sound[melody][0] + (6 * semp) + 4)); //устанавливаем паузу перед воспроизведением нового звука
    if (++semp > alarm_sound[melody][1] - 1) semp = 0; //переключпем на следующий семпл
  }
}
//----------------------------------Проверка будильников---------------------------------------------------------
void checkAlarms(void) //проверка будильников
{
  if (alarm) { //если тревога активна
    if (++minsAlarm >= mainSettings.almTimeOut) {
      alarmReset(); //сброс будильника
      MELODY_RESET; //сброс позиции мелодии
      return; //выходим
    }

    if (mainSettings.almWaint && alarmWaint) {
      if (++minsAlarmWaint >= mainSettings.almWaint) {
        alarmWaint = 0;
        minsAlarmWaint = 0;
      }
    }
    else if (mainSettings.almTimeOutSound) {
      if (++minsAlarmSound >= mainSettings.almTimeOutSound) {
        if (mainSettings.almWaint) {
          alarmWaint = 1;
          minsAlarmSound = 0;
        }
        else alarmReset(); //сброс будильника
        MELODY_RESET; //сброс позиции мелодии
      }
    }
  }
  else { //иначе проверяем будильники на совподение
    for (uint8_t alm = 0; alm < ALARMS_NUM; alm++) {
      if (alarms[alm][2]) {
        if (RTC_time.h == alarms[alm][0] && RTC_time.m == alarms[alm][1] && (alarms[alm][2] == 2 || (alarms[alm][3] & (0x01 << RTC_time.DW)))) {
          alarm = alm + 1;
          return;
        }
      }
    }
  }
}
//----------------------------------Тревога будильника---------------------------------------------------------
void alarmWarn(void) //тревога будильника
{
  if (alarm && !alarmWaint) {
    boolean blink_data = 1;
    while (1) {
      data_convert(); //обработка данных
      if (!alarm || alarmWaint) {
        _animShow = 0; //сбросить флаг анимации
        return;
      }

      MELODY_PLAY(mainSettings.almMelody); //воспроизводим мелодию

      if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
        _timer_ms[TMR_MS] = ALM_BLINK_TIME; //устанавливаем таймер

        if (blink_data) { //если нужно отрисовать время
          indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC_time.h) : RTC_time.h, 0, 2, 0); //вывод часов
          indiPrintNum(RTC_time.m, 2, 2, 0); //вывод минут
          OCR1B = dotMaxBright; //включаем точки
        }
        else {
          indiClr(); //очистка индикаторов
          OCR1B = 0; //выключаем точки
        }

        blink_data = !blink_data; //мигаем временем
      }

      switch (check_keys()) {
        case LEFT_KEY_PRESS: //клик левой кнопкой
        case RIGHT_KEY_PRESS: //клик правой кнопкой
        case SET_KEY_PRESS: //клик средней кнопкой
          if (mainSettings.almWaint) {
            alarmWaint = 1;
            minsAlarmSound = 0;
          }
          else alarmReset(); //сброс будильника
          MELODY_RESET; //сброс позиции мелодии
          _animShow = 0; //сбросить флаг анимации
          _scr = 0; //обновление экрана
          return;

        case LEFT_KEY_HOLD: //удержание левой кнопки
        case RIGHT_KEY_HOLD: //удержание правой кнопки
        case SET_KEY_HOLD: //удержание средней кнопки
          alarmReset(); //сброс будильника
          MELODY_RESET; //сброс позиции мелодии
          _animShow = 0; //сбросить флаг анимации
          _scr = 0; //обновление экрана
          return;
      }
    }
  }
}
//----------------------------------Сброс будильника---------------------------------------------------------
void alarmReset(void) //сброс будильника
{
  if (alarms[alarm - 1][2] == 2) {
    alarms[alarm - 1][2] = 0;
    eeprom_update_block((void*)&alarms, (void*)EEPROM_BLOCK_ALARM, sizeof(alarms)); //записываем будильники в память
  }
  alarmWaint = 0;
  minsAlarm = 0;
  minsAlarmWaint = 0;
  minsAlarmSound = 0;
  alarm = 0;
}
//----------------------------------Преобразование данных---------------------------------------------------------
void data_convert(void) //преобразование данных
{
  backlFlash(); //"дыхание" подсветки

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
        changeBright(); //установка яркости от времени суток
        hourSound(); //звук смены часа
      }
      checkAlarms(); //проверка будильников
      _animShow = 1; //показать анимацию переключения цифр
      _tmrBurn++; //прибавляем минуту к таймеру антиотравления
    }
    if (_tmrGlitch) _tmrGlitch--; //убавляем секунду от таймера глюков
    _scr = 0; //разрешаем обновить индикаторы
  }

  for (; tick_ms > 0; tick_ms--) { //если был тик, обрабатываем данные
    switch (btn_state) { //таймер опроса кнопок
      case 0: if (btn_check) btn_tmr++; break; //считаем циклы
      case 1: if (btn_tmr > 0) btn_tmr--; break; //убираем дребезг
    }

    for (uint8_t tm = 0; tm < TIMERS_NUM; tm++) {
      if (_timer_ms[tm] > 4) _timer_ms[tm] -= 4; //если таймер больше 4мс
      else if (_timer_ms[tm]) _timer_ms[tm] = 0; //иначе сбрасываем таймер
    }
  }
}
//------------------------------------Звук смены часа------------------------------------
void hourSound(void) //звук смены часа
{
  if (!alarm || alarmWaint) { //если будильник не работает
    if ((mainSettings.timeHour[1] > mainSettings.timeHour[0] && RTC_time.h < mainSettings.timeHour[1] && RTC_time.h >= mainSettings.timeHour[0]) ||
        (mainSettings.timeHour[1] < mainSettings.timeHour[0] && (RTC_time.h < mainSettings.timeHour[1] || RTC_time.h >= mainSettings.timeHour[0]))) {
      buzz_pulse(HOUR_SOUND_FREQ, HOUR_SOUND_TIME); //звук смены часа
    }
  }
}
//------------------------------------Имитация глюков------------------------------------
void glitchTick(void) //имитация глюков
{
  if (mainSettings.glitchMode) {
    if (!_tmrGlitch && RTC_time.s > 7 && RTC_time.s < 55) {
      boolean indiState = 0; //состояние индикатора
      uint8_t glitchCounter = random(2, 6); //максимальное количество глюков
      uint8_t glitchIndic = random(0, 4); //номер индикатора
      uint8_t indiSave = indi_buf[glitchIndic]; //сохраняем текущую цифру в индикаторе
      while (!availableData() && !check_keys()) {
        data_convert(); //обработка данных
        dotFlash(); //мигаем точками

        if (!_timer_ms[TMR_GLITCH]) { //если таймер истек
          if (!indiState) indiClr(glitchIndic); //выключаем индикатор
          else indiSet(indiSave, glitchIndic); //установка индикатора
          indiState = !indiState; //меняем состояние глюка лампы
          _timer_ms[TMR_GLITCH] = random(1, 6) * 20; //перезапускаем таймер глюка
          if (!glitchCounter--) break; //выходим если закончились глюки
        }
      }
      _tmrGlitch = random(mainSettings.glitchMin, mainSettings.glitchMax); //находим рандомное время появления глюка
      indiSet(indiSave, glitchIndic);
    }
  }
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
        if (mainSettings.knock_sound) buzz_pulse(KNOCK_SOUND_FREQ, KNOCK_SOUND_TIME); //щелчок пищалкой
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
        case 1: return SET_KEY_PRESS; //возвращаем клик средней кнопкой
        case 2: return LEFT_KEY_PRESS; //возвращаем клик левой кнопкой
        case 3: return RIGHT_KEY_PRESS; //возвращаем клик правой кнопкой
      }
      break;

    case 2:
      btn_set = 0; //сбрасываем признак нажатия
      switch (btn_switch) { //переключаемся в зависимости от состояния мультиопроса
        case 1: return SET_KEY_HOLD; //возвращаем удержание средней кнопки
        case 2: return LEFT_KEY_HOLD; //возвращаем удержание левой кнопки
        case 3: return RIGHT_KEY_HOLD; //возвращаем удержание правой кнопки
      }
      break;
  }
  return 0;
}
//----------------------------------------------------------------------------------
void settings_time(void)
{
  uint8_t cur_mode = 0; //текущий режим
  uint8_t time_out = 0; //таймаут автовыхода
  boolean blink_data = 0; //мигание сигментами

  indiClr(); //очищаем индикаторы
  OCR1B = dotMaxBright; //включаем точки

  //настройки
  while (1) {
    data_convert(); //обработка данных

    if (!_scr) {
      _scr = 1;
      if (++time_out >= SETTINGS_TIMEOUT) return;
    }

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

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
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (cur_mode) {
          //настройка времени
          case 0: if (RTC_time.h > 0) RTC_time.h--; else RTC_time.h = 23; RTC_time.s = 0; break; //часы
          case 1: if (RTC_time.m > 0) RTC_time.m--; else RTC_time.m = 59; RTC_time.s = 0; break; //минуты

          //настройка даты
          case 2: if (RTC_time.DD > 1 ) RTC_time.DD--; else RTC_time.DD = (RTC_time.MM == 2 && !(RTC_time.YY % 4)) ? 1 : 0 + daysInMonth[RTC_time.MM - 1]; break; //день
          case 3: if (RTC_time.MM > 1) RTC_time.MM--; else RTC_time.MM = 12; RTC_time.DD = 1; break; //месяц

          //настройка года
          case 4: if (RTC_time.YY > 2021) RTC_time.YY--; else RTC_time.YY = 2050; break; //год
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (cur_mode) {
          //настройка времени
          case 0: if (RTC_time.h < 23) RTC_time.h++; else RTC_time.h = 0; RTC_time.s = 0; break; //часы
          case 1: if (RTC_time.m < 59) RTC_time.m++; else RTC_time.m = 0; RTC_time.s = 0; break; //минуты

          //настройка даты
          case 2: if (RTC_time.DD < daysInMonth[RTC_time.MM - 1] + (RTC_time.MM == 2 && !(RTC_time.YY % 4)) ? 1 : 0) RTC_time.DD++; else RTC_time.DD = 1; break; //день
          case 3: if (RTC_time.MM < 12) RTC_time.MM++; else RTC_time.MM = 1; RTC_time.DD = 1; break; //месяц

          //настройка года
          case 4: if (RTC_time.YY < 2050) RTC_time.YY++; else RTC_time.YY = 2021; break; //год
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        if (cur_mode < 4) cur_mode++; else cur_mode = 0;
        if (cur_mode != 4) OCR1B = dotMaxBright; //включаем точки
        else OCR1B = 0; //выключаем точки
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        sendTime(); //отправить время в RTC
        changeBright(); //установка яркости от времени суток
        eeprom_update_block((void*)&RTC_time, (void*)EEPROM_BLOCK_TIME, sizeof(RTC_time)); //записываем дату по умолчанию в память
        return;
    }
  }
}
//----------------------------------------------------------------------------------
void changeBright(void) //установка яркости от времени суток
{
  if ((brightSettings.timeBright[0] > brightSettings.timeBright[1] && (RTC_time.h >= brightSettings.timeBright[0] || RTC_time.h < brightSettings.timeBright[1])) ||
      (brightSettings.timeBright[0] < brightSettings.timeBright[1] && RTC_time.h >= brightSettings.timeBright[0] && RTC_time.h < brightSettings.timeBright[1])) {
    //ночной режим
    dotMaxBright = brightSettings.dotBright[0]; //установка максимальной яркости точек
    backlMaxBright = brightSettings.backlBright[0]; //установка максимальной яркости подсветки
    indiMaxBright = brightSettings.indiBright[0]; //установка максимальной яркости индикаторов
    FLIP_SPEED[0] = (uint16_t)DEFAULT_FLIP_TIME * brightSettings.indiBright[1] / brightSettings.indiBright[0]; //расчёт шага яркости режима 2
  }
  else {
    //дневной режим
    dotMaxBright = brightSettings.dotBright[1]; //установка максимальной яркости точек
    backlMaxBright = brightSettings.backlBright[1]; //установка максимальной яркости подсветки
    indiMaxBright = brightSettings.indiBright[1]; //установка максимальной яркости индикаторов
    FLIP_SPEED[0] = (uint16_t)DEFAULT_FLIP_TIME; //расчёт шага яркости режима 2
  }
  switch (brightSettings.dotMode) {
    case 0: OCR1B = 0; break; //если точки выключены
    case 1: OCR1B = dotMaxBright; break; //если точки статичные, устанавливаем яркость
    case 2:
      dotBrightStep = ceil((float)dotMaxBright * 2 / brightSettings.dotTime * brightSettings.dotTimer); //расчёт шага яркости точки
      if (!dotBrightStep) dotBrightStep = 1; //если шаг слишком мал, устанавливаем минимум
      break;
  }
  switch (brightSettings.backlMode) {
    case 0: OCR2A = 0; break; //если посветка выключена
    case 1: OCR2A = backlMaxBright; break; //если посветка статичная, устанавливаем яркость
    case 2:
      if (backlMaxBright) backlBrightStep = (float)brightSettings.backlStep / backlMaxBright / 2 * brightSettings.backlTime; //если подсветка динамичная, расчёт шага дыхания подсветки
      else OCR2A = 0; //иначе посветка выключена
      break;
  }
  indiSetBright(indiMaxBright); //установка общей яркости индикаторов
}
//----------------------------------Мигание подсветки---------------------------------
void backlFlash(void) //мигание подсветки
{
  static boolean backl_drv; //направление яркости
  if (brightSettings.backlMode == 2 && backlMaxBright) {
    if (!_timer_ms[TMR_BACKL]) {
      _timer_ms[TMR_BACKL] = backlBrightStep;
      switch (backl_drv) {
        case 0: if (OCR2A < backlMaxBright) OCR2A += brightSettings.backlStep; else backl_drv = 1; break;
        case 1:
          if (OCR2A > brightSettings.backlMinBright) OCR2A -= brightSettings.backlStep;
          else {
            backl_drv = 0;
            _timer_ms[TMR_BACKL] = brightSettings.backlPause;
          }
          break;
      }
    }
  }
}
//--------------------------------Мигание точек------------------------------------
void dotFlash(void) //мигание точек
{
  static boolean dot_drv; //направление яркости
  if (brightSettings.dotMode == 2) {
    if (!_timer_ms[TMR_DOT]) {
      _timer_ms[TMR_DOT] = brightSettings.dotTimer;
      switch (dot_drv) {
        case 0: if (OCR1B < dotMaxBright) OCR1B += dotBrightStep; else dot_drv = 1; break;
        case 1:
          if (OCR1B > 0) OCR1B -= dotBrightStep;
          else {
            dot_drv = 0;
            _timer_ms[TMR_DOT] = 1000 - brightSettings.dotTime;
          }
          break;
      }
    }
  }
}
//--------------------------------Показать температуру----------------------------------------
void showTemp(void) //показать температуру
{
  uint8_t mode = 0; //текущий режим
  OCR1B = dotMaxBright; //включаем точки
  _scr = 0; //обновление экрана
  switch (mainSettings.sensorSet) { //выбор датчика температуры
    case 0: readTempDS(); break; //чтение температуры с датчика DS3231
    case 1: readTempBME(); break; //чтение температуры/давления/влажности с датчика BME
  }
  for (_timer_ms[TMR_MS] = SHOW_TIME; _timer_ms[TMR_MS] && !availableData();) {
    data_convert(); //обработка данных

    if (!_scr) {
      _scr = 1; //сбрасываем флаг
      indiClr(); //очистка индикаторов
      switch (mode) {
        case 0: indiPrintNum(tempSens.temp / 10 + mainSettings.tempCorrect, 0, 3, ' '); break;
        case 1: indiPrintNum(tempSens.press, 0, 4, ' '); break;
        case 2: indiPrintNum(tempSens.hum, 1, 3, ' '); break;
      }
    }

    switch (check_keys()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        if (++mode > 2) mode = 0;
        switch (mode) {
          case 1: if (!tempSens.press) mode = 0; else OCR1B = 0; break;
          case 2: if (!tempSens.hum) mode = 0; break;
        }
        if (!mode) OCR1B = dotMaxBright; //включаем точки
        _timer_ms[TMR_MS] = SHOW_TIME;
        _scr = 0; //обновление экрана
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
      case SET_KEY_PRESS: //клик средней кнопкой
        return; //выходим
    }
  }
}
//----------------------------------Показать дату-----------------------------------
void showDate(void) //показать дату
{
  uint8_t mode = 0; //текущий режим
  OCR1B = dotMaxBright; //включаем точки
  _scr = 0; //обновление экрана
  for (_timer_ms[TMR_MS] = SHOW_TIME; _timer_ms[TMR_MS] && !availableData();) {
    data_convert(); //обработка данных

    if (!_scr) {
      _scr = 1; //сбрасываем флаг
      indiClr(); //очистка индикаторов
      switch (mode) {
        case 0:
          indiPrintNum(RTC_time.DD, 0, 2, 0); //вывод даты
          indiPrintNum(RTC_time.MM, 2, 2, 0); //вывод месяца
          break;
        case 1: indiPrintNum(RTC_time.YY, 0); break; //вывод года
      }
    }

    switch (check_keys()) {
      case RIGHT_KEY_PRESS: //клик правой кнопкой
        if (++mode > 1) mode = 0;
        switch (mode) {
          case 0: OCR1B = dotMaxBright; break; //включаем точки
          case 1: OCR1B = 0; break; //выключаем точки
        }
        _timer_ms[TMR_MS] = SHOW_TIME;
        _scr = 0; //обновление экрана
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
      case SET_KEY_PRESS: //клик средней кнопкой
        return; //выходим
    }
  }
}
//----------------------------------Переключение подсветки-----------------------------------
void backlSwitch(void) //переключение подсветки
{
  uint8_t anim = 0; //анимация переключения
  OCR1B = 0; //выключаем точки
  for (_timer_ms[TMR_MS] = SWITCH_TIME; _timer_ms[TMR_MS] && !availableData();) {
    data_convert(); //обработка данных

    if (anim < 4) {
      if (!_timer_ms[TMR_ANIM]) { //если таймер истек
        _timer_ms[TMR_ANIM] = ANIM_TIME; //устанавливаем таймер

        indiClr(); //очистка индикаторов
        indiPrintNum(brightSettings.backlMode, anim); //вывод даты
        anim++;
      }
    }

    switch (check_keys()) {
      case SET_KEY_PRESS: //клик средней кнопкой
        if (++brightSettings.backlMode > 2) brightSettings.backlMode = 0;
        switch (brightSettings.backlMode) {
          case 0: OCR2A = 0; break; //выключаем подсветку
          case 1: OCR2A = backlMaxBright; break; //включаем подсветку
          case 2: OCR2A = brightSettings.backlMinBright; break; //выключаем подсветку
        }
        eeprom_update_block((void*)&brightSettings, (void*)EEPROM_BLOCK_SETTINGS_BRIGHT, sizeof(brightSettings)); //записываем основные настройки в память
        _timer_ms[TMR_MS] = SWITCH_TIME;
        anim = 0;
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
      case RIGHT_KEY_PRESS: //клик правой кнопкой
        return; //выходим
    }
  }
}
//----------------------------------Переключение анимации цифр-----------------------------------
void flipSwitch(void) //переключение анимации цифр
{
  uint8_t anim = 0; //анимация переключения
  OCR1B = 0; //выключаем точки
  for (_timer_ms[TMR_MS] = SWITCH_TIME; _timer_ms[TMR_MS] && !availableData();) {
    data_convert(); //обработка данных

    if (anim < 4) {
      if (!_timer_ms[TMR_ANIM]) { //если таймер истек
        _timer_ms[TMR_ANIM] = ANIM_TIME; //устанавливаем таймер

        indiClr(); //очистка индикаторов
        indiPrintNum(mainSettings.flipMode, anim); //вывод даты
        anim++;
      }
    }

    switch (check_keys()) {
      case RIGHT_KEY_PRESS: //клик правой кнопкой
        if (++mainSettings.flipMode > FLIP_EFFECT_NUM) mainSettings.flipMode = 0;
        eeprom_update_block((void*)&mainSettings, (void*)EEPROM_BLOCK_SETTINGS_MAIN, sizeof(mainSettings)); //записываем основные настройки в память
        _timer_ms[TMR_MS] = SWITCH_TIME;
        anim = 0;
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
      case SET_KEY_PRESS: //клик средней кнопкой
        return; //выходим
    }
  }
}
//----------------------------------Переключение глюков-----------------------------------
void glitchSwitch(void) //переключение глюков
{
  uint8_t anim = 0; //анимация переключения
  OCR1B = 0; //выключаем точки
  for (_timer_ms[TMR_MS] = SWITCH_TIME; _timer_ms[TMR_MS] && !availableData();) {
    data_convert(); //обработка данных

    if (anim < 4) {
      if (!_timer_ms[TMR_ANIM]) { //если таймер истек
        _timer_ms[TMR_ANIM] = ANIM_TIME; //устанавливаем таймер

        indiClr(); //очистка индикаторов
        indiPrintNum(mainSettings.glitchMode, anim); //вывод даты
        anim++;
      }
    }

    switch (check_keys()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        mainSettings.glitchMode = !mainSettings.glitchMode;
        eeprom_update_block((void*)&mainSettings, (void*)EEPROM_BLOCK_SETTINGS_MAIN, sizeof(mainSettings)); //записываем основные настройки в память
        _timer_ms[TMR_MS] = SWITCH_TIME;
        anim = 0;
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
      case SET_KEY_PRESS: //клик средней кнопкой
        return; //выходим
    }
  }
}
//----------------------------Антиотравление индикаторов-------------------------------
void burnIndi(void) //антиотравление индикаторов
{
  if (_tmrBurn >= mainSettings.burnPeriod && RTC_time.s >= BURN_PHASE) {
    _tmrBurn = 0; //сбрасываем таймер
    for (byte indi = 0; indi < 4; indi++) {
      for (byte loops = 0; loops < mainSettings.burnLoops; loops++) {
        for (byte digit = 0; digit < 10; digit++) {
          indiPrintNum(cathodeMask[digit], indi); //отрисовываем цифру
          for (_timer_ms[TMR_MS] = mainSettings.burnTime; _timer_ms[TMR_MS];) { //ждем
            if (availableData() || check_keys()) { //если доступны данные или нажата кнопка
              indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC_time.h) : RTC_time.h, 0, 2, 0); //вывод часов
              indiPrintNum(RTC_time.m, 2, 2, 0); //вывод минут
              return; //выходим
            }
            data_convert(); //обработка данных
            dotFlash(); //мигаем точками
          }
        }
      }
      indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC_time.h) : RTC_time.h, 0, 2, 0); //вывод часов
      indiPrintNum(RTC_time.m, 2, 2, 0); //вывод минут
    }
  }
}
//----------------------------------Анимация цифр-----------------------------------
void flipIndi(uint8_t flipMode, boolean demo) //анимация цифр
{
  uint8_t mode;
  _animShow = 0; //сбрасываем флаг
  switch (flipMode) {
    case 0: return;
    case 1: if (demo) return; else mode = random(0, FLIP_EFFECT_NUM - 1); break;
    default: mode = flipMode - 2; break;
  }

  uint8_t drvIndi = 1;
  uint8_t flipIndi[4] = {0, 0, 0, 0};
  uint8_t anim_buf[8];
  uint8_t HH;
  uint8_t MM;

  if (RTC_time.m) {
    MM = RTC_time.m - 1;
    HH = RTC_time.h;
  }
  else {
    MM = 59;
    if (RTC_time.h) HH = RTC_time.h - 1;
    else HH = 23;
  }

  if (!demo) {
    if (RTC_time.h / 10 != HH / 10) flipIndi[0] = 1;
    if (RTC_time.h % 10 != HH % 10) flipIndi[1] = 1;
    if (RTC_time.m / 10 != MM / 10) flipIndi[2] = 1;
    if (RTC_time.m % 10 != MM % 10) flipIndi[3] = 1;
  }
  else {
    indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC_time.h) : RTC_time.h, 0, 2, 0); //вывод часов
    indiPrintNum(RTC_time.m, 2, 2, 0); //вывод минут
    for (uint8_t i = 0; i < 4; i++) flipIndi[i] = 1;
  }

  switch (mode) {
    case 0: //плавное угасание и появление
      anim_buf[0] = indiMaxBright;

      while (1 && !availableData()) {
        data_convert(); //обработка данных
        dotFlash(); //мигаем точками

        if (!_timer_ms[TMR_ANIM]) { //если таймер истек
          if (drvIndi) {
            if (anim_buf[0] > 0) {
              anim_buf[0]--;
              for (uint8_t i = 0; i < 4; i++) if (flipIndi[i]) indiSetBright(anim_buf[0], i);
            }
            else {
              drvIndi = 0;
              indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC_time.h) : RTC_time.h, 0, 2, 0); //вывод часов
              indiPrintNum(RTC_time.m, 2, 2, 0); //вывод минут
            }
          }
          else {
            if (anim_buf[0] < indiMaxBright) {
              anim_buf[0]++;
              for (uint8_t i = 0; i < 4; i++) if (flipIndi[i]) indiSetBright(anim_buf[0], i);
            }
            else break;
          }
          _timer_ms[TMR_ANIM] = FLIP_SPEED[mode]; //устанавливаем таймер
        }
      }
      break;
    case 1: //перемотка по порядку числа
      anim_buf[0] = HH / 10;
      anim_buf[1] = HH % 10;
      anim_buf[2] = MM / 10;
      anim_buf[3] = MM % 10;
      anim_buf[4] = RTC_time.h / 10;
      anim_buf[5] = RTC_time.h % 10;
      anim_buf[6] = RTC_time.m / 10;
      anim_buf[7] = RTC_time.m % 10;

      while (1 && !availableData()) {
        data_convert(); //обработка данных
        dotFlash(); //мигаем точками

        if (!_timer_ms[TMR_ANIM]) { //если таймер истек
          drvIndi = 0;
          for (uint8_t i = 0; i < 4; i++) {
            if (flipIndi[i]) {
              if (anim_buf[i]) anim_buf[i]--;
              else anim_buf[i] = 9;
              if (anim_buf[i] == anim_buf[i + 4]) flipIndi[i] = 0;
              else drvIndi = 1;
              indiPrintNum(anim_buf[i], i);
            }
          }
          if (!drvIndi) break;
          _timer_ms[TMR_ANIM] = FLIP_SPEED[mode]; //устанавливаем таймер
        }
      }
      break;
    case 2: //перемотка по порядку катодов в лампе
      if (!demo) {
        for (byte c = 0; c < 10; c++) {
          if (cathodeMask[c] == RTC_time.h / 10) anim_buf[0] = c;
          if (cathodeMask[c] == HH / 10) anim_buf[4] = c;
          if (cathodeMask[c] == RTC_time.h % 10) anim_buf[1] = c;
          if (cathodeMask[c] == HH % 10) anim_buf[5] = c;
          if (cathodeMask[c] == RTC_time.m / 10) anim_buf[2] = c;
          if (cathodeMask[c] == MM / 10) anim_buf[6] = c;
          if (cathodeMask[c] == RTC_time.m % 10) anim_buf[3] = c;
          if (cathodeMask[c] == MM % 10) anim_buf[7] = c;
        }
      }
      else {
        for (uint8_t i = 0; i < 4; i++) {
          for (byte c = 0; c < 10; c++) {
            anim_buf[i] = 0;
            anim_buf[i + 4] = 9;
          }
        }
      }

      while (1 && !availableData()) {
        data_convert(); //обработка данных
        dotFlash(); //мигаем точками

        if (!_timer_ms[TMR_ANIM]) { //если таймер истек
          drvIndi = 0;
          for (byte i = 0; i < 4; i++) {
            if (flipIndi[i]) {
              drvIndi = 1;
              if (anim_buf[i] > anim_buf[i + 4]) anim_buf[i]--;
              else if (anim_buf[i] < anim_buf[i + 4]) anim_buf[i]++;
              else flipIndi[i] = 0;
              indiPrintNum(cathodeMask[anim_buf[i]], i);
            }
          }
          if (!drvIndi) break;
          _timer_ms[TMR_ANIM] = FLIP_SPEED[mode]; //устанавливаем таймер
        }
      }
      break;
    case 3: //поезд
      //старое время
      anim_buf[0] = HH / 10; //часы
      anim_buf[1] = HH % 10; //часы
      anim_buf[2] = MM / 10; //минуты
      anim_buf[3] = MM % 10; //минуты

      for (uint8_t i = 0; i < 4 && !availableData();) {
        data_convert(); //обработка данных
        dotFlash(); //мигаем точками
        if (!_timer_ms[TMR_ANIM]) { //если таймер истек
          for (uint8_t b = 0; b < 4; b++) {
            if (b >= i) indiPrintNum(anim_buf[b - i], b); //вывод часов
            else indiClr(b); //очистка индикатора
          }
          i++; //прибавляем цикл
          _timer_ms[TMR_ANIM] = FLIP_SPEED[mode]; //устанавливаем таймер
        }
      }
      //новое время
      anim_buf[3] = RTC_time.h / 10; //часы
      anim_buf[2] = RTC_time.h % 10; //часы
      anim_buf[1] = RTC_time.m / 10; //минуты
      anim_buf[0] = RTC_time.m % 10; //минуты

      for (uint8_t i = 0; i < 4 && !availableData();) {
        data_convert(); //обработка данных
        dotFlash(); //мигаем точками
        if (!_timer_ms[TMR_ANIM]) { //если таймер истек
          for (uint8_t b = 0; b < 4; b++) {
            if (b <= i) indiPrintNum(anim_buf[i - b], b); //вывод часов
            else indiClr(b); //очистка индикатора
          }
          i++; //прибавляем цикл
          _timer_ms[TMR_ANIM] = FLIP_SPEED[mode]; //устанавливаем таймер
        }
      }
      break;
    case 4: //резинка
      //старое время
      anim_buf[3] = HH / 10; //часы
      anim_buf[2] = HH % 10; //часы
      anim_buf[1] = MM / 10; //минуты
      anim_buf[0] = MM % 10; //минуты

      drvIndi = 0;

      for (uint8_t i = 0; i < 4 && !availableData();) {
        data_convert(); //обработка данных
        dotFlash(); //мигаем точками
        if (!_timer_ms[TMR_ANIM]) { //если таймер истек
          for (uint8_t b = i + 1; b > 0; b--) {
            if (b - 1 == i - drvIndi) indiPrintNum(anim_buf[i], 4 - b); //вывод часов
            else indiClr(4 - b); //очистка индикатора
          }
          if (drvIndi++ >= i) {
            drvIndi = 0; //сбрасываем позицию индикатора
            i++; //прибавляем цикл
          }
          _timer_ms[TMR_ANIM] = FLIP_SPEED[mode]; //устанавливаем таймер
        }
      }
      //новое время
      anim_buf[0] = RTC_time.h / 10; //часы
      anim_buf[1] = RTC_time.h % 10; //часы
      anim_buf[2] = RTC_time.m / 10; //минуты
      anim_buf[3] = RTC_time.m % 10; //минуты

      drvIndi = 0;

      for (uint8_t i = 0; i < 4 && !availableData();) {
        data_convert(); //обработка данных
        dotFlash(); //мигаем точками
        if (!_timer_ms[TMR_ANIM]) { //если таймер истек
          for (uint8_t b = 0; b < 4 - i; b++) {
            if (b == drvIndi) indiPrintNum(anim_buf[3 - i], b); //вывод часов
            else indiClr(b); //очистка индикатора
          }
          if (drvIndi++ >= 3 - i) {
            drvIndi = 0; //сбрасываем позицию индикатора
            i++; //прибавляем цикл
          }
          _timer_ms[TMR_ANIM] = FLIP_SPEED[mode]; //устанавливаем таймер
        }
      }
      break;
  }
}
//-----------------------------Главный экран------------------------------------------------
void main_screen(void) //главный экран
{
  if (_animShow) flipIndi(mainSettings.flipMode, 0); //анимация цифр основная

  if (!_scr) {
    _scr = 1; //сбрасываем флаг
    indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC_time.h) : RTC_time.h, 0, 2, 0); //вывод часов
    indiPrintNum(RTC_time.m, 2, 2, 0); //вывод минут
  }

  dotFlash(); //мигаем точками
  glitchTick(); //имитация глюков
  burnIndi(); //антиотравление индикаторов

  switch (check_keys()) {
    case LEFT_KEY_HOLD: //удержание левой кнопки
      glitchSwitch(); //переключение глюков
      _scr = 0; //обновление экрана
      break;
    case LEFT_KEY_PRESS: //клик левой кнопкой
      showTemp(); //показать температуру
      _scr = 0; //обновление экрана
      break;
    case RIGHT_KEY_PRESS: //клик правой кнопкой
      showDate(); //показать дату
      _scr = 0; //обновление экрана
      break;
    case RIGHT_KEY_HOLD: //удержание правой кнопки
      flipSwitch(); //переключение анимации цифр
      flipIndi(mainSettings.flipMode, 1); //демонстрация анимации цифр
      _scr = 0; //обновление экрана
      break;
    case SET_KEY_PRESS: //клик средней кнопкой
      backlSwitch(); //переключение подсветки
      _scr = 0; //обновление экрана
      break;
    case SET_KEY_HOLD: //удержание средней кнопки
      if (alarmWaint) alarmReset(); //сброс будильника
      else settings_time(); //иначе настройки времени
      _scr = 0; //обновление экрана
      break;
  }
}
//-------------------------------Получить 12-ти часовой формат---------------------------------------------------
uint8_t get_12h(uint8_t timeH) //получить 12-ти часовой формат
{
  return (timeH > 12) ? (timeH - 12) : (timeH) ? timeH : 12;
}
//-------------------------------Сброс до заводских настроек---------------------------------------------------
void mainReset(void) //сброс до заводских настроек
{
  indiClr(); //очистка индикаторов
  eeprom_update_byte((uint8_t*)EEPROM_BLOCK_VERSION_FW, 0); //сбрасываем метку
  sendCommand(ANSWER_RESET_OK); //отправляем ответ
  for (_timer_ms[TMR_MS] = WAINT_BEFORE_REBOOT; _timer_ms[TMR_MS];) data_convert(); //ждем отправки ответа
  indiDisable(); //выключение индикаторов
  asm volatile("jmp 0x0000"); //прыгаем в начало
}
//-------------------------------Синхронизация данных---------------------------------------------------
void sincData(void) //синхронизация данных
{
  if (availableData()) {
    uint8_t command = readData();
    switch (command) {
      case COMMAND_SEND_VERSION: sendCommand(VERSION_FW); break;
      case COMMAND_SEND_TIME: sendData(ANSWER_SEND_TIME, (uint8_t*)&RTC_time, sizeof(RTC_time)); break;
      case COMMAND_GET_TIME: getData((uint8_t*)&RTC_time, sizeof(RTC_time)); sendTime(); changeBright(); eeprom_update_block((void*)&RTC_time, (void*)EEPROM_BLOCK_TIME, sizeof(RTC_time)); break;
      case COMMAND_SEND_ALARM: sendData(ANSWER_SEND_ALARM, (uint8_t*)&alarms, sizeof(alarms)); break;
      case COMMAND_GET_ALARM: getData((uint8_t*)&alarms, sizeof(alarms)); eeprom_update_block((void*)&alarms, (void*)EEPROM_BLOCK_ALARM, sizeof(alarms)); break;
      case COMMAND_SEND_SET_BRIGHT: sendData(ANSWER_SEND_SET_BRIGHT, (uint8_t*)&brightSettings, sizeof(brightSettings)); break;
      case COMMAND_GET_SET_BRIGHT: getData((uint8_t*)&brightSettings, sizeof(brightSettings)); changeBright(); eeprom_update_block((void*)&brightSettings, (void*)EEPROM_BLOCK_SETTINGS_BRIGHT, sizeof(brightSettings)); break;
      case COMMAND_SEND_SET_MAIN: sendData(ANSWER_SEND_SET_MAIN, (uint8_t*)&mainSettings, sizeof(mainSettings)); break;
      case COMMAND_GET_SET_MAIN: getData((uint8_t*)&mainSettings, sizeof(mainSettings)); eeprom_update_block((void*)&mainSettings, (void*)EEPROM_BLOCK_SETTINGS_MAIN, sizeof(mainSettings)); break;
      case COMMAND_RESET_SETTINGS: mainReset(); break;
      default:
        sendCommand(ANSWER_UNKNOWN_COMMAND);
        clearBuffer(); //очистить буфер приёма
        break;
    }
  }
}
