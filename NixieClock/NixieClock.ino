/*
  Arduino IDE 1.8.13 версия прошивки 1.0.0 релиз от 29.07.21
  Специльно для проекта "Часы на ГРИ и Arduino v2 | AlexGyver"
  Страница проекта - https://alexgyver.ru/nixieclock_v2

  Исходник - https://github.com/radon-lab/NixieClock
  Автор Radon-lab.
*/
//--------------Версия прошивки-------------
#define VERSION_FW 0x71

//-----------------Таймеры------------------
#define TIMERS_NUM 7 //количество таймеров
uint32_t _timer_ms[TIMERS_NUM]; //таймер отсчета миллисекунд

#define TMR_SENS   0 //таймер сенсоров температуры
#define TMR_MS     1 //таймер общего назначения
#define TMR_MELODY 2 //таймер мелодий
#define TMR_BACKL  3 //таймер подсветки
#define TMR_DOT    4 //таймер точек
#define TMR_ANIM   5 //таймер анимаций
#define TMR_GLITCH 6 //таймер глюков

//----------------Библиотеки----------------
#include <avr/eeprom.h>
#include <util/delay.h>

//---------------Конфигурации---------------
#include "config.h"
#include "connection.h"
#include "indiDisp.h"
#include "wire.h"
#include "BME.h"
#include "RTC.h"
#include "DHT.h"
#include "DS.h"

//----------------Настройки----------------
struct Settings_1 {
  uint8_t indiBright[2] = {DEFAULT_INDI_BRIGHT_N, DEFAULT_INDI_BRIGHT};
  uint8_t timeBright[2] = {DEFAULT_NIGHT_START, DEFAULT_NIGHT_END};
  uint8_t timeHour[2] = {DEFAULT_HOUR_SOUND_START, DEFAULT_HOUR_SOUND_END};
  boolean timeFormat = DEFAULT_TIME_FORMAT;
  boolean knock_sound = DEFAULT_KNOCK_SOUND;
  uint8_t sensorSet = DEFAULT_TEMP_SENSOR;
  int8_t tempCorrect = DEFAULT_TEMP_CORRECT;
  boolean glitchMode = DEFAULT_GLITCH_MODE;
} mainSettings;

struct Settings_2 {
  uint8_t flipMode = DEFAULT_FLIP_ANIM;
  uint8_t backlMode = DEFAULT_BACKL_MODE;
  uint8_t dotMode = DEFAULT_DOT_MODE;
} fastSettings;


uint8_t semp = 0; //переключатель мелодии
#define MELODY_PLAY(melody) _melody_chart(melody)
#define MELODY_RESET semp = 0

//переменные обработки кнопок
uint8_t btn_tmr; //таймер тиков обработки
boolean btn_check; //флаг разрешения опроса кнопки
boolean btn_state; //флаг текущего состояния кнопки

boolean _animShow = 0; //флаг анимации
boolean _scr = 0; //флаг обновления экрана
boolean _dot = 0; //флаг обновления точек
uint8_t _mode = 0; //текущий основной режим

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
uint16_t FLIP_SPEED[] = {FLIP_MODE_2_TIME, FLIP_MODE_3_TIME, FLIP_MODE_4_TIME, FLIP_MODE_5_TIME, FLIP_MODE_6_TIME}; //скорость эффектов

uint8_t dotBrightStep;
uint8_t dotBrightTime;
uint8_t dotMaxBright;
uint8_t backlBrightTime;
uint8_t backlMaxBright;
uint8_t indiMaxBright;

//alarms - час | минута | режим(0 - выкл, 2 - одиночный, 1 - вкл) | день недели(вс,сб,пт,чт,ср,вт,пн,null)
//alarmsSettings - время до автоматического включения ожидания | время до полного отключения | время ожидания будильника для повторного включения | мелодия будильника
uint8_t alarms_num = 0; //текущее количество будильников

boolean alarmWaint = 0;
uint8_t alarm = 0;
uint8_t minsAlarm = 0;
uint8_t minsAlarmSound = 0;
uint8_t minsAlarmWaint = 0;

uint8_t _tmrBurn = 0;
uint8_t _tmrGlitch = 0;

#define EEPROM_BLOCK_TIME EEPROM_BLOCK_NULL //блок памяти времени
#define EEPROM_BLOCK_SETTINGS_FAST (EEPROM_BLOCK_TIME + sizeof(RTC_time)) //блок памяти настроек свечения
#define EEPROM_BLOCK_SETTINGS_MAIN (EEPROM_BLOCK_SETTINGS_FAST + sizeof(fastSettings)) //блок памяти основных настроек
#define EEPROM_BLOCK_ALARM (EEPROM_BLOCK_SETTINGS_MAIN + sizeof(mainSettings)) //блок памяти количества будильников
#define EEPROM_BLOCK_ALARM_DATA (EEPROM_BLOCK_ALARM + sizeof(alarms_num)) //первая ячейка памяти будильников

#define MAX_ALARMS ((255 - (sizeof(RTC_time) + sizeof(fastSettings) + sizeof(mainSettings) + sizeof(alarms_num))) >> 3)

int atexit(void (* /*func*/ )()) { //инициализация функций
  return 0;
}
//----------------------------------Инициализация-------------------------------------------------------------
int main(void) //инициализация
{
  SET_INIT; //инициализация средней кнопки
  LEFT_INIT; //инициализация левой кнопки
  RIGHT_INIT; //инициализация правой кнопки
  CONV_INIT; //инициализация преобразователя
  SQW_INIT; //инициализация счета секунд
  DOT_INIT; //инициализация точки
  BACKL_INIT; //инициализация подсветки
  BUZZ_INIT; //инициализация бузера

  WireInit(); //инициализация Wire
  IndiInit(); //инициализация индикаторов
  UartDisable(); //отключение uart

  if (eeprom_read_byte((uint8_t*)EEPROM_BLOCK_VERSION_FW) != VERSION_FW || !SET_CHK) { //если первый запуск или зажата средняя кнопка, восстанавливаем из переменных
    eeprom_update_byte((uint8_t*)EEPROM_BLOCK_VERSION_FW, VERSION_FW); //делаем метку версии прошивки
    eeprom_update_block((void*)&RTC_time, (void*)EEPROM_BLOCK_TIME, sizeof(RTC_time)); //записываем дату и время в память
    eeprom_update_block((void*)&fastSettings, (void*)EEPROM_BLOCK_SETTINGS_FAST, sizeof(fastSettings)); //записываем настройки яркости в память
    eeprom_update_block((void*)&mainSettings, (void*)EEPROM_BLOCK_SETTINGS_MAIN, sizeof(mainSettings)); //записываем основные настройки в память
    eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM, alarms_num); //записываем количество будильников в память
  }
  else if (LEFT_CHK) { //если левая кнопка не зажата, загружаем настройки из памяти
    eeprom_read_block((void*)&fastSettings, (void*)EEPROM_BLOCK_SETTINGS_FAST, sizeof(fastSettings)); //считываем настройки яркости из памяти
    eeprom_read_block((void*)&mainSettings, (void*)EEPROM_BLOCK_SETTINGS_MAIN, sizeof(mainSettings)); //считываем основные настройки из памяти
    alarms_num = eeprom_read_byte((uint8_t*)EEPROM_BLOCK_ALARM); //считываем количество будильников из памяти
  }

  if (getTime()) { //запрашиваем время из RTC
#if RTC_MODUL
    EICRA = (1 << ISC01); //настраиваем внешнее прерывание по спаду импульса на INT0
    EIMSK = (1 << INT0); //разрешаем внешнее прерывание INT0
    setSQW(); //установка SQW на 1Гц
#endif
    if (RTC_time.YY < 2021 || RTC_time.YY > 2050) { //если пропадало питание
      eeprom_read_block((void*)&RTC_time, 0, sizeof(RTC_time)); //считываем дату и время из памяти
      sendTime(); //отправить время в RTC
    }
  }
  else buzz_pulse(RTC_ERROR_SOUND_FREQ, RTC_ERROR_SOUND_TIME); //сигнал ошибки модуля часов

  randomSeed(RTC_time.s * (RTC_time.m + RTC_time.h) + RTC_time.DD * RTC_time.MM); //радомный сид для глюков
  _tmrGlitch = random(GLITCH_MIN, GLITCH_MAX); //находим рандомное время появления глюка
  changeBright(); //установка яркости от времени суток
  //----------------------------------Главная-------------------------------------------------------------
  for (;;) //главная
  {
    data_convert(); //обработка данных
    alarmWarn(); //тревога будильника
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
//----------------------------------Отключение uart---------------------------------------------------------
void UartDisable(void) //отключение uart
{
  UCSR0B = 0; //выключаем UART
  PRR |= (0x01 << PRUSART0); //выключаем питание UART
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
    buzz_pulse(pgm_read_word((uint16_t*)(alarm_sound[melody][0] + (6 * semp))), pgm_read_word((uint16_t*)(alarm_sound[melody][0] + (6 * semp) + 2))); //запускаем звук с задоной частотой и временем
    _timer_ms[TMR_MELODY] = pgm_read_word((uint16_t*)(alarm_sound[melody][0] + (6 * semp) + 4)); //устанавливаем паузу перед воспроизведением нового звука
    if (++semp > alarm_sound[melody][1] - 1) semp = 0; //переключпем на следующий семпл
  }
}
//----------------------------------Проверка будильников---------------------------------------------------------
void checkAlarms(void) //проверка будильников
{
  if (alarm) { //если тревога активна
    if (++minsAlarm >= alarmSettings(alarm, 1)) {
      alarmReset(); //сброс будильника
      MELODY_RESET; //сброс позиции мелодии
      return; //выходим
    }

    if (alarmSettings(alarm, 2) && alarmWaint) {
      if (++minsAlarmWaint >= alarmSettings(alarm, 2)) {
        alarmWaint = 0;
        minsAlarmWaint = 0;
      }
    }
    else if (alarmSettings(alarm, 0)) {
      if (++minsAlarmSound >= alarmSettings(alarm, 0)) {
        if (alarmSettings(alarm, 2)) {
          alarmWaint = 1;
          minsAlarmSound = 0;
        }
        else alarmReset(); //сброс будильника
        MELODY_RESET; //сброс позиции мелодии
      }
    }
  }
  else { //иначе проверяем будильники на совподение
    for (uint8_t alm = 0; alm < alarms_num; alm++) {
      if (alarms(alm, 2)) {
        if (RTC_time.h == alarms(alm, 0) && RTC_time.m == alarms(alm, 1) && (alarms(alm, 2) == 2 || (alarms(alm, 3) & (0x01 << RTC_time.DW)))) {
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

      MELODY_PLAY(alarmSettings(alarm, 3)); //воспроизводим мелодию

      if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
        _timer_ms[TMR_MS] = ALM_BLINK_TIME; //устанавливаем таймер

        switch (blink_data) {
          case 0:
            indiClr(); //очистка индикаторов
            OCR1B = 0; //выключаем точки
            break;
          case 1:
            indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC_time.h) : RTC_time.h, 0, 2, 0); //вывод часов
            indiPrintNum(RTC_time.m, 2, 2, 0); //вывод минут
            OCR1B = dotMaxBright; //включаем точки
            break;
        }
        blink_data = !blink_data; //мигаем временем
      }

      switch (check_keys()) {
        case LEFT_KEY_PRESS: //клик левой кнопкой
        case RIGHT_KEY_PRESS: //клик правой кнопкой
        case SET_KEY_PRESS: //клик средней кнопкой
          if (alarmSettings(alarm, 2)) {
            alarmWaint = 1;
            minsAlarmSound = 0;
          }
          else {
            buzz_pulse(ALM_OFF_SOUND_FREQ, ALM_OFF_SOUND_TIME); //звук выключения будильника
            alarmReset(); //сброс будильника
          }
          MELODY_RESET; //сброс позиции мелодии
          _animShow = 0; //сбросить флаг анимации
          _scr = 0; //обновление экрана
          return;

        case LEFT_KEY_HOLD: //удержание левой кнопки
        case RIGHT_KEY_HOLD: //удержание правой кнопки
        case SET_KEY_HOLD: //удержание средней кнопки
          buzz_pulse(ALM_OFF_SOUND_FREQ, ALM_OFF_SOUND_TIME); //звук выключения будильника
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
  if (alarms(alarm - 1, 2) == 2) eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + ((alarm - 1) << 3) + 2, 0);
  alarmWaint = 0;
  minsAlarm = 0;
  minsAlarmWaint = 0;
  minsAlarmSound = 0;
  alarm = 0;
}
//----------------------------------Получить основные данные будильника---------------------------------------------------------
uint8_t alarms(uint8_t num, uint8_t data) //получить основные данные будильника
{
  return eeprom_read_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (num << 3) + data);
}
//----------------------------------Получить настройку будильника---------------------------------------------------------
uint8_t alarmSettings(uint8_t num, uint8_t data) //получить настройку будильника
{
  return eeprom_read_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (num << 3) + data + 4);
}
//---------------------Создать новый будильник---------------------------------------------------------
void newAlarm(void) //создать новый будильник
{
  if (alarms_num < MAX_ALARMS) {
    eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (alarms_num << 3), DEFAULT_ALARM_TIME_HH);
    eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (alarms_num << 3) + 1, DEFAULT_ALARM_TIME_MM);
    eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (alarms_num << 3) + 2, DEFAULT_ALARM_MODE);
    eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (alarms_num << 3) + 3, 0);
    eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (alarms_num << 3) + 4, DEFAULT_ALARM_TIMEOUT_S);
    eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (alarms_num << 3) + 5, DEFAULT_ALARM_TIMEOUT);
    eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (alarms_num << 3) + 6, DEFAULT_ALARM_WAINT);
    eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (alarms_num << 3) + 7, DEFAULT_ALARM_SOUND);
    eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM, ++alarms_num); //записываем количество будильников в память
  }
}
//---------------------Удалить будильник---------------------------------------------------------
void delAlarm(uint8_t alarm) //удалить будильник
{
  if (alarms_num) {
    for (uint8_t start = alarm; start < alarms_num; start++) {
      eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (alarms_num << 3), eeprom_read_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + ((alarms_num + 1) << 3)));
      eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (alarms_num << 3) + 1, eeprom_read_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + ((alarms_num + 1) << 3) + 1));
      eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (alarms_num << 3) + 2, eeprom_read_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + ((alarms_num + 1) << 3) + 2));
      eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (alarms_num << 3) + 3, eeprom_read_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + ((alarms_num + 1) << 3) + 3));
      eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (alarms_num << 3) + 4, eeprom_read_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + ((alarms_num + 1) << 3) + 4));
      eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (alarms_num << 3) + 5, eeprom_read_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + ((alarms_num + 1) << 3) + 5));
      eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (alarms_num << 3) + 6, eeprom_read_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + ((alarms_num + 1) << 3) + 6));
      eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (alarms_num << 3) + 7, eeprom_read_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + ((alarms_num + 1) << 3) + 7));
    }
    eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM, --alarms_num); //записываем количество будильников в память
  }
}
//----------------------------------Преобразование данных---------------------------------------------------------
void data_convert(void) //преобразование данных
{
  static uint32_t timeNotRTC; //счетчик реального времени
  backlFlash(); //"дыхание" подсветки

  for (; tick_ms > 0; tick_ms--) { //если был тик, обрабатываем данные
    switch (btn_state) { //таймер опроса кнопок
      case 0: if (btn_check) btn_tmr++; break; //считаем циклы
      case 1: if (btn_tmr > 0) btn_tmr--; break; //убираем дребезг
    }

    if (!EIMSK) { //если внешние часы не обнаружены
      timeNotRTC += TIME_PERIOD; //добавляем ко времени период таймера
      if (timeNotRTC > 1000000UL) { //если прошла секунда
        timeNotRTC -= 1000000UL; //оставляем остаток
        tick_sec++; //прибавляем секунду
      }
    }

    for (uint8_t tm = 0; tm < TIMERS_NUM; tm++) { //опрашиваем все таймеры
      if (_timer_ms[tm] > 4) _timer_ms[tm] -= 4; //если таймер больше 4мс
      else if (_timer_ms[tm]) _timer_ms[tm] = 0; //иначе сбрасываем таймер
    }
  }

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
    _scr = _dot = 0; //разрешаем обновить индикаторы
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
      while (!check_keys()) {
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
      _tmrGlitch = random(GLITCH_MIN, GLITCH_MAX); //находим рандомное время появления глюка
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
      if (!SET_CHK) { //если нажата кл. ок
        btn_switch = 1; //выбираем клавишу опроса
        btn_state = 0; //обновляем текущее состояние кнопки
      }
      else if (!LEFT_CHK) { //если нажата левая кл.
        btn_switch = 2; //выбираем клавишу опроса
        btn_state = 0; //обновляем текущее состояние кнопки
      }
      else if (!RIGHT_CHK) { //если нажата правая кл.
        btn_switch = 3; //выбираем клавишу опроса
        btn_state = 0; //обновляем текущее состояние кнопки
      }
      else btn_state = 1; //обновляем текущее состояние кнопки
      break;
    case 1: btn_state = SET_CHK; break; //опрашиваем клавишу ок
    case 2: btn_state = LEFT_CHK; break; //опрашиваем левую клавишу
    case 3: btn_state = RIGHT_CHK; break; //опрашиваем правую клавишу
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
//----------------------------Настройки времени----------------------------------
void settings_time(void) //настройки времени
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
          case 2: if (RTC_time.DD > 1 ) RTC_time.DD--; else RTC_time.DD = maxDays(); break; //день
          case 3: //месяц
            if (RTC_time.MM > 1) RTC_time.MM--;
            else RTC_time.MM = 12;
            if (RTC_time.DD > maxDays()) RTC_time.DD = maxDays();
            break;

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
          case 2: if (RTC_time.DD < maxDays()) RTC_time.DD++; else RTC_time.DD = 1; break; //день
          case 3: //месяц
            if (RTC_time.MM < 12) RTC_time.MM++;
            else RTC_time.MM = 1;
            if (RTC_time.DD > maxDays()) RTC_time.DD = maxDays();
            break;

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
//-----------------------------выбор будильника------------------------------------
void choice_alarm(void) //выбор будильника
{
  uint8_t time_out = 0; //таймаут автовыхода
  uint8_t curAlarm = alarms_num > 0;

  indiClr(); //очищаем индикаторы
  OCR1B = 0; //выключаем точки

  while (1) {
    data_convert(); //обработка данных

    if (!_scr) {
      _scr = 1;
      if (++time_out >= SETTINGS_TIMEOUT) return;
      indiClr(); //очищаем индикаторы
      indiPrintNum(curAlarm, 1, 2, 0); //вывод номера будильника
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (check_keys()) {
      case LEFT_KEY_HOLD: //удержание левой кнопки
        if (curAlarm) {
          delAlarm(curAlarm - 1); //удалить текущий будильник
          OCR1B = dotMaxBright; //включаем точки
          for (_timer_ms[TMR_MS] = 500; _timer_ms[TMR_MS];) data_convert(); //обработка данных
          OCR1B = 0; //выключаем точки
        }
        curAlarm = (alarms_num > 0);
        _scr = 0;
        time_out = 0;
        break;
      case LEFT_KEY_PRESS: //клик левой кнопкой
        if (curAlarm > (alarms_num > 0)) curAlarm--; else curAlarm = alarms_num;
        _scr = 0;
        time_out = 0;
        break;
      case RIGHT_KEY_PRESS: //клик правой кнопкой
        if (curAlarm < alarms_num) curAlarm++; else curAlarm = (alarms_num > 0);
        _scr = 0;
        time_out = 0;
        break;
      case RIGHT_KEY_HOLD: //удержание правой кнопки
        newAlarm(); //создать новый будильник
        OCR1B = dotMaxBright; //включаем точки
        for (_timer_ms[TMR_MS] = 500; _timer_ms[TMR_MS];) data_convert(); //обработка данных
        OCR1B = 0; //выключаем точки
        curAlarm = alarms_num;
        _scr = 0;
        time_out = 0;
        break;
      case SET_KEY_PRESS: //клик средней кнопкой
        if (curAlarm) settings_alarm(curAlarm - 1); //настроки будильника
        _scr = 0;
        time_out = 0;
        break;
      case SET_KEY_HOLD: //удержание средней кнопки
        return; //выход
    }
  }
}
//-----------------------------Настроки будильника------------------------------------
void settings_alarm(uint8_t Alarm) //настроки будильника
{
  uint8_t alarms[4]; //массив данных о будильнике
  uint8_t cur_mode = 0; //текущий режим
  uint8_t cur_day = 1; //текущий день недели
  uint8_t time_out = 0; //таймаут автовыхода
  boolean blink_data = 0; //мигание сигментами

  indiClr(); //очищаем индикаторы
  OCR1B = dotMaxBright; //включаем точки

  for (uint8_t i = 0; i < 4; i++) alarms[i] = eeprom_read_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (Alarm << 3) + i);

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
          if (!blink_data || cur_mode == 1) indiPrintNum(alarms[0], 0, 2, 0); //вывод часов
          if (!blink_data || cur_mode == 0) indiPrintNum(alarms[1], 2, 2, 0); //вывод минут
          break;
        case 2:
        case 3:
        case 4:
          indiPrintNum(alarms[2], 0); //вывод режима
          indiPrintNum(cur_day, 2); //вывод дня недели
          indiPrintNum((alarms[3] >> cur_day) & 0x01, 3); //вывод установки
          if (blink_data) indiClr((cur_mode == 2) ? 0 : (cur_mode - 1)); //очистка индикатора
          break;
      }
      blink_data = !blink_data; //мигание сигментами
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (check_keys()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (cur_mode) {
          //настройка времени будильника
          case 0: if (alarms[0] > 0) alarms[0]--; else alarms[0] = 23; break; //часы
          case 1: if (alarms[1] > 0) alarms[1]--; else alarms[1] = 59; break; //минуты

          //настройка режима будильника
          case 2: if (alarms[2] < 2) alarms[2]++; else alarms[2] = 0; break; //режим
          case 3: if (cur_day < 7) cur_day++; else cur_day = 1; break; //день недели
          case 4: alarms[3] &= ~(0x01 << cur_day); break; //установка
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (cur_mode) {
          //настройка времени будильника
          case 0: if (alarms[0] < 23) alarms[0]++; else alarms[0] = 0; break; //часы
          case 1: if (alarms[1] < 59) alarms[1]++; else alarms[1] = 0; break; //минуты

          //настройка режима будильника
          case 2: if (alarms[2] < 2) alarms[2]++; else alarms[2] = 0; break; //режим
          case 3: if (cur_day < 7) cur_day++; else cur_day = 1; break; //день недели
          case 4: alarms[3] |= (0x01 << cur_day); break; //установка
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        if (cur_mode < 4) cur_mode++; else cur_mode = 0;
        if (cur_mode < 2) OCR1B = dotMaxBright; //включаем точки
        else OCR1B = 0; //выключаем точки
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        for (uint8_t i = 0; i < 4; i++) eeprom_update_byte((uint8_t*)EEPROM_BLOCK_ALARM_DATA + (Alarm << 3) + i, alarms[i]);
        return;
    }
  }
}
//-----------------------------Настроки основные------------------------------------
void settings_main(void) //настроки основные
{
  uint8_t cur_mode = 0; //текущий режим
  boolean cur_indi = 0; //текущий индикатор
  uint8_t anim = 0; //анимация переключения
  boolean drv = 0; //направление анимации
  boolean set = 0; //режим настройки
  uint8_t time_out = 0; //таймаут автовыхода
  boolean blink_data = 0; //мигание сигментами

  indiClr(); //очищаем индикаторы
  OCR1B = 0; //выключаем точки

  //настройки
  while (1) {
    data_convert(); //обработка данных

    if (!_scr) {
      _scr = 1;
      if (++time_out >= SETTINGS_TIMEOUT) return;
    }

    switch (set) {
      case 0:
        if (!_timer_ms[TMR_ANIM]) { //если таймер истек
          _timer_ms[TMR_ANIM] = ANIM_TIME; //устанавливаем таймер
          switch (drv) {
            case 0: if (anim < 3) anim++; else drv = 1; break;
            case 1: if (anim > 0) anim--; else drv = 0; break;
          }
          indiClr(); //очистка индикаторов
          indiPrintNum(cur_mode + 1, anim); //вывод режима
        }
        break;
      case 1:
        if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
          _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

          indiClr(); //очистка индикаторов
          switch (cur_mode) {
            case 0: if (!blink_data) indiPrintNum((mainSettings.timeFormat) ? 12 : 24, 2); break;
            case 1: if (!blink_data) indiPrintNum(mainSettings.glitchMode, 3); break;
            case 2: if (!blink_data) indiPrintNum(mainSettings.knock_sound, 3); break;
            case 3:
              if (!blink_data || cur_indi) indiPrintNum(mainSettings.timeHour[0], 0, 2, 0); //вывод часов
              if (!blink_data || !cur_indi) indiPrintNum(mainSettings.timeHour[1], 2, 2, 0); //вывод часов
              break;
            case 4:
              if (!blink_data || cur_indi) indiPrintNum(mainSettings.timeBright[0], 0, 2, 0); //вывод часов
              if (!blink_data || !cur_indi) indiPrintNum(mainSettings.timeBright[1], 2, 2, 0); //вывод часов
              break;
            case 5:
              if (!blink_data || cur_indi) indiPrintNum(mainSettings.indiBright[0], 0, 2, 0); //вывод часов
              if (!blink_data || !cur_indi) indiPrintNum(mainSettings.indiBright[1], 2, 2, 0); //вывод часов
              break;
            case 6:
              if (!blink_data || cur_indi) indiPrintNum(tempSens.temp / 10 + mainSettings.tempCorrect, 0, 3); //вывод часов
              if (!blink_data || !cur_indi) indiPrintNum(mainSettings.sensorSet, 3); //вывод часов
              break;
          }
          blink_data = !blink_data; //мигание сигментами
        }
        break;
    }
    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (check_keys()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (set) {
          case 0: if (cur_mode > 0) cur_mode--; else cur_mode = 6; break;
          case 1:
            switch (cur_mode) {
              case 0: mainSettings.timeFormat = 0; break; //формат времени
              case 1: mainSettings.glitchMode = 0; break; //глюки
              case 2: mainSettings.knock_sound = 0; break; //звук кнопок
              case 3: //время звука смены часа
                switch (cur_indi) {
                  case 0: if (mainSettings.timeHour[0] > 0) mainSettings.timeHour[0]--; else mainSettings.timeHour[0] = 23; break;
                  case 1: if (mainSettings.timeHour[1] > 0) mainSettings.timeHour[1]--; else mainSettings.timeHour[1] = 23; break;
                }
                break;
              case 4: //время смены подсветки
                switch (cur_indi) {
                  case 0: if (mainSettings.timeBright[0] > 0) mainSettings.timeBright[0]--; else mainSettings.timeBright[0] = 23; break;
                  case 1: if (mainSettings.timeBright[1] > 0) mainSettings.timeBright[1]--; else mainSettings.timeBright[1] = 23; break;
                }
                break;
              case 5: //яркость индикаторов
                switch (cur_indi) {
                  case 0: if (mainSettings.indiBright[0] > 0) mainSettings.indiBright[0]--; else mainSettings.indiBright[0] = 30; break;
                  case 1: if (mainSettings.indiBright[1] > 0) mainSettings.indiBright[1]--; else mainSettings.indiBright[1] = 30; break;
                }
                indiSetBright(mainSettings.indiBright[cur_indi]); //установка общей яркости индикаторов
                break;
              case 6: //настройка сенсора температуры
                switch (cur_indi) {
                  case 0: if (mainSettings.tempCorrect > -127) mainSettings.tempCorrect--; else mainSettings.tempCorrect = 127; break;
                  case 1:
                    if (mainSettings.sensorSet > 0) mainSettings.sensorSet--;
                    switch (mainSettings.sensorSet) { //выбор датчика температуры
                      case 0: readTempRTC(); break; //чтение температуры с датчика DS3231
                      case 1: readTempBME(); break; //чтение температуры/давления/влажности с датчика BME
                      case 2: readTempDHT11(); break; //чтение температуры/влажности с датчика DHT11
                      case 3: readTempDHT22(); break; //чтение температуры/влажности с датчика DHT22
                      case 4: readTempDS(); break; //чтение температуры с датчика DS18B20
                    }
                    break;
                }
                break;
            }
            break;
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (set) {
          case 0: if (cur_mode < 6) cur_mode++; else cur_mode = 0; break;
          case 1:
            switch (cur_mode) {
              case 0: mainSettings.timeFormat = 1; break; //формат времени
              case 1: mainSettings.glitchMode = 1; break; //глюки
              case 2: mainSettings.knock_sound = 1; break; //звук кнопок
              case 3: //время звука смены часа
                switch (cur_indi) {
                  case 0: if (mainSettings.timeHour[0] < 23) mainSettings.timeHour[0]++; else mainSettings.timeHour[0] = 0; break;
                  case 1: if (mainSettings.timeHour[1] < 23) mainSettings.timeHour[1]++; else mainSettings.timeHour[1] = 0; break;
                }
                break;
              case 4: //время смены подсветки
                switch (cur_indi) {
                  case 0: if (mainSettings.timeBright[0] < 23) mainSettings.timeBright[0]++; else mainSettings.timeBright[0] = 0; break;
                  case 1: if (mainSettings.timeBright[1] < 23) mainSettings.timeBright[1]++; else mainSettings.timeBright[1] = 0; break;
                }
                break;
              case 5: //яркость индикаторов
                switch (cur_indi) {
                  case 0: if (mainSettings.indiBright[0] < 30) mainSettings.indiBright[0]++; else mainSettings.indiBright[0] = 0; break;
                  case 1: if (mainSettings.indiBright[1] < 30) mainSettings.indiBright[1]++; else mainSettings.indiBright[1] = 0; break;
                }
                indiSetBright(mainSettings.indiBright[cur_indi]); //установка общей яркости индикаторов
                break;
              case 6: //настройка сенсора температуры
                switch (cur_indi) {
                  case 0: if (mainSettings.tempCorrect < 127) mainSettings.tempCorrect++; else mainSettings.tempCorrect = -127; break;
                  case 1:
                    if (mainSettings.sensorSet < 4) mainSettings.sensorSet++;
                    switch (mainSettings.sensorSet) { //выбор датчика температуры
                      case 0: readTempRTC(); break; //чтение температуры с датчика DS3231
                      case 1: readTempBME(); break; //чтение температуры/давления/влажности с датчика BME
                      case 2: readTempDHT11(); break; //чтение температуры/влажности с датчика DHT11
                      case 3: readTempDHT22(); break; //чтение температуры/влажности с датчика DHT22
                      case 4: readTempDS(); break; //чтение температуры с датчика DS18B20
                    }
                    break;
                }
                break;
            }
            break;
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case LEFT_KEY_HOLD: //удержание левой кнопки
        if (set) {
          cur_indi = 0;
          if (cur_mode == 5) indiSetBright(mainSettings.indiBright[0]); //установка общей яркости индикаторов
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_HOLD: //удержание правой кнопки
        if (set) {
          cur_indi = 1;
          if (cur_mode == 5) indiSetBright(mainSettings.indiBright[1]); //установка общей яркости индикаторов
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        set = !set;
        if (set) {
          switch (cur_mode) {
            case 5: indiSetBright(mainSettings.indiBright[0]); break; //установка общей яркости индикаторов
            case 6:
              switch (mainSettings.sensorSet) { //выбор датчика температуры
                case 0: readTempRTC(); break; //чтение температуры с датчика DS3231
                case 1: readTempBME(); break; //чтение температуры/давления/влажности с датчика BME
                case 2: readTempDHT11(); break; //чтение температуры/влажности с датчика DHT11
                case 3: readTempDHT22(); break; //чтение температуры/влажности с датчика DHT22
                case 4: readTempDS(); break; //чтение температуры с датчика DS18B20
              }
              break;
          }
          OCR1B = dotMaxBright; //включаем точки
        }
        else {
          changeBright(); //установка яркости от времени суток
          OCR1B = 0; //выключаем точки
        }
        cur_indi = 0;
        anim = 0;
        drv = 0;
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        eeprom_update_block((void*)&mainSettings, (void*)EEPROM_BLOCK_SETTINGS_MAIN, sizeof(mainSettings)); //записываем основные настройки в память
        changeBright(); //установка яркости от времени суток
        return;
    }
  }
}
//---------------------Установка яркости от времени суток-----------------------------
void changeBright(void) //установка яркости от времени суток
{
  if ((mainSettings.timeBright[0] > mainSettings.timeBright[1] && (RTC_time.h >= mainSettings.timeBright[0] || RTC_time.h < mainSettings.timeBright[1])) ||
      (mainSettings.timeBright[0] < mainSettings.timeBright[1] && RTC_time.h >= mainSettings.timeBright[0] && RTC_time.h < mainSettings.timeBright[1])) {
    //ночной режим
    dotMaxBright = DOT_BRIGHT_N; //установка максимальной яркости точек
    backlMaxBright = BACKL_BRIGHT_N; //установка максимальной яркости подсветки
    indiMaxBright = mainSettings.indiBright[0]; //установка максимальной яркости индикаторов
    FLIP_SPEED[0] = (uint16_t)FLIP_MODE_2_TIME * mainSettings.indiBright[1] / mainSettings.indiBright[0]; //расчёт шага яркости режима 2
  }
  else {
    //дневной режим
    dotMaxBright = DOT_BRIGHT; //установка максимальной яркости точек
    backlMaxBright = BACKL_BRIGHT; //установка максимальной яркости подсветки
    indiMaxBright = mainSettings.indiBright[1]; //установка максимальной яркости индикаторов
    FLIP_SPEED[0] = (uint16_t)FLIP_MODE_2_TIME; //расчёт шага яркости режима 2
  }
  switch (fastSettings.dotMode) {
    case 0: OCR1B = 0; break; //если точки выключены
    case 1: OCR1B = dotMaxBright; break; //если точки статичные, устанавливаем яркость
    case 2:
      dotBrightStep = ceil((float)dotMaxBright * 2 / DOT_TIME * DOT_TIMER); //расчёт шага яркости точки
      if (!dotBrightStep) dotBrightStep = 1; //если шаг слишком мал, устанавливаем минимум
      dotBrightTime = ceil(DOT_TIME / (float)dotMaxBright * 2); //расчёт шага яркости точки
      if (!dotBrightTime) dotBrightTime = DOT_TIMER; //если шаг слишком мал, устанавливаем минимум
      break;
  }
  switch (fastSettings.backlMode) {
    case 0: OCR2A = 0; break; //если посветка выключена
    case 1: OCR2A = backlMaxBright; break; //если посветка статичная, устанавливаем яркость
    case 2:
      if (backlMaxBright) backlBrightTime = (float)BACKL_STEP / backlMaxBright / 2 * BACKL_TIME; //если подсветка динамичная, расчёт шага дыхания подсветки
      else OCR2A = 0; //иначе посветка выключена
      break;
  }
  indiSetBright(indiMaxBright); //установка общей яркости индикаторов
}
//----------------------------------Мигание подсветки---------------------------------
void backlFlash(void) //мигание подсветки
{
  static boolean backl_drv; //направление яркости
  if (fastSettings.backlMode == 2 && backlMaxBright) {
    if (!_timer_ms[TMR_BACKL]) {
      _timer_ms[TMR_BACKL] = backlBrightTime;
      switch (backl_drv) {
        case 0: if (OCR2A < backlMaxBright) OCR2A += BACKL_STEP; else backl_drv = 1; break;
        case 1:
          if (OCR2A > BACKL_MIN_BRIGHT) OCR2A -= BACKL_STEP;
          else {
            backl_drv = 0;
            _timer_ms[TMR_BACKL] = BACKL_PAUSE;
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
  if (!alarmWaint) {
    if (fastSettings.dotMode == 2) {
      if (!_dot && !_timer_ms[TMR_DOT]) {
        _timer_ms[TMR_DOT] = dotBrightTime;
        switch (dot_drv) {
          case 0: if (OCR1B < dotMaxBright) OCR1B += dotBrightStep; else dot_drv = 1; break;
          case 1:
            if (OCR1B > dotBrightStep) OCR1B -= dotBrightStep;
            else {
              OCR1B = 0;
              _dot = 1;
              dot_drv = 0;
            }
            break;
        }
      }
    }
    else {
      switch (fastSettings.dotMode) {
        case 0: if (OCR1B) OCR1B = 0; break; //если точки включены, выключаем их
        case 1: if (OCR1B != dotMaxBright) OCR1B = dotMaxBright; break; //если яркость не совпадает, устанавливаем яркость
      }
    }
  }
  else {
    if (!_timer_ms[TMR_DOT]) {
      _timer_ms[TMR_DOT] = ALM_BLINK_DOT;
      OCR1B = (!OCR1B) ? dotMaxBright : 0;
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
    case 0: readTempRTC(); break; //чтение температуры с датчика DS3231
    case 1: readTempBME(); break; //чтение температуры/давления/влажности с датчика BME
    case 2: readTempDHT11(); break; //чтение температуры/влажности с датчика DHT11
    case 3: readTempDHT22(); break; //чтение температуры/влажности с датчика DHT22
    case 4: readTempDS(); break; //чтение температуры с датчика DS18B20
  }
  for (_timer_ms[TMR_MS] = SHOW_TIME; _timer_ms[TMR_MS];) {
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
          case 1:
            if (!tempSens.press) {
              if (!tempSens.hum) mode = 0;
              else {
                mode = 2;
                OCR1B = 0;
              }
            }
            else OCR1B = 0; break;
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
  for (_timer_ms[TMR_MS] = SHOW_TIME; _timer_ms[TMR_MS];) {
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
//----------------------------------Переключение быстрых настроек-----------------------------------
void fastSetSwitch(void) //переключение быстрых настроек
{
  uint8_t anim = 0; //анимация переключения
  uint8_t mode = 0; //режим быстрой настройки

  OCR1B = 0; //выключаем точки
  for (_timer_ms[TMR_MS] = SWITCH_TIME; _timer_ms[TMR_MS];) {
    data_convert(); //обработка данных

    if (anim < 4) {
      if (!_timer_ms[TMR_ANIM]) { //если таймер истек
        _timer_ms[TMR_ANIM] = ANIM_TIME; //устанавливаем таймер

        indiClr(); //очистка индикаторов
        switch (mode) {
          case 0: indiPrintNum(fastSettings.backlMode, anim); break; //вывод режима подсветки
          case 1: indiPrintNum(fastSettings.flipMode, anim); break; //вывод режима анимации
          case 2: indiPrintNum(fastSettings.dotMode, anim); break; //вывод режима точек
        }
        anim++;
      }
    }

    switch (check_keys()) {
      case SET_KEY_PRESS: //клик средней кнопкой
        if (++fastSettings.backlMode > 2) fastSettings.backlMode = 0;
        switch (fastSettings.backlMode) {
          case 0: OCR2A = 0; break; //выключаем подсветку
          case 1: OCR2A = backlMaxBright; break; //включаем подсветку
          case 2: OCR2A = BACKL_MIN_BRIGHT; break; //выключаем подсветку
        }
        _timer_ms[TMR_MS] = SWITCH_TIME;
        anim = 0;
        mode = 0;
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        if (++fastSettings.flipMode > FLIP_EFFECT_NUM) fastSettings.flipMode = 0;
        _timer_ms[TMR_MS] = SWITCH_TIME;
        anim = 0;
        mode = 1;
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
        if (++fastSettings.dotMode > 2) fastSettings.dotMode = 0;
        _timer_ms[TMR_MS] = SWITCH_TIME;
        anim = 0;
        mode = 2;
        break;
    }
  }
  if (mode == 1) flipIndi(fastSettings.flipMode, 1); //демонстрация анимации цифр
  eeprom_update_block((void*)&fastSettings, (void*)EEPROM_BLOCK_SETTINGS_FAST, sizeof(fastSettings)); //записываем настройки в память
}
//----------------------------Антиотравление индикаторов-------------------------------
void burnIndi(void) //антиотравление индикаторов
{
  if (_tmrBurn >= BURN_PERIOD && RTC_time.s >= BURN_PHASE) {
    _tmrBurn = 0; //сбрасываем таймер
    for (byte indi = 0; indi < 4; indi++) {
      for (byte loops = 0; loops < BURN_LOOPS; loops++) {
        for (byte digit = 0; digit < 10; digit++) {
          indiPrintNum(cathodeMask[digit], indi); //отрисовываем цифру
          for (_timer_ms[TMR_MS] = BURN_TIME; _timer_ms[TMR_MS];) { //ждем
            if (check_keys()) { //если нажата кнопка
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

      while (!check_keys()) {
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
      indiSetBright(indiMaxBright); //возвращаем максимальную яркость
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

      while (!check_keys()) {
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

      while (!check_keys()) {
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

      for (uint8_t i = 0; i < 4 && !check_keys();) {
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

      for (uint8_t i = 0; i < 4 && !check_keys();) {
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

      for (uint8_t i = 0; i < 4 && !check_keys();) {
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

      for (uint8_t i = 0; i < 4 && !check_keys();) {
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
  if (_animShow) flipIndi(fastSettings.flipMode, 0); //анимация цифр основная

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
      settings_main(); //настроки основные
      _scr = _animShow = 0; //обновление экрана
      break;
    case LEFT_KEY_PRESS: //клик левой кнопкой
      showTemp(); //показать температуру
      _scr = _animShow = 0; //обновление экрана
      break;
    case RIGHT_KEY_PRESS: //клик правой кнопкой
      showDate(); //показать дату
      _scr = _animShow = 0; //обновление экрана
      break;
    case RIGHT_KEY_HOLD: //удержание правой кнопки
      choice_alarm(); //выбор будильника
      _scr = _animShow = 0; //обновление экрана
      break;
    case SET_KEY_PRESS: //клик средней кнопкой
      fastSetSwitch(); //переключение настроек
      _scr = _animShow = 0; //обновление экрана
      break;
    case SET_KEY_HOLD: //удержание средней кнопки
      if (alarmWaint) {
        buzz_pulse(ALM_OFF_SOUND_FREQ, ALM_OFF_SOUND_TIME); //звук выключения будильника
        alarmReset(); //сброс будильника
      }
      else settings_time(); //иначе настройки времени
      _scr = _animShow = 0; //обновление экрана
      break;
  }
}
//-------------------------------Получить 12-ти часовой формат---------------------------------------------------
uint8_t get_12h(uint8_t timeH) //получить 12-ти часовой формат
{
  return (timeH > 12) ? (timeH - 12) : (timeH) ? timeH : 12;
}
