/*
  Arduino IDE 1.8.13 версия прошивки 1.5.1 релиз от 28.02.22
  Специльно для проекта "Часы на ГРИ и Arduino v2 | AlexGyver"
  Страница проекта - https://alexgyver.ru/nixieclock_v2

  Исходник - https://github.com/radon-lab/NixieClock
  Автор Radon-lab.
*/
//-----------------Таймеры------------------
enum {
  TMR_SENS, //таймер сенсоров температуры
  TMR_MS, //таймер общего назначения
  TMR_MELODY, //таймер мелодий
  TMR_BACKL, //таймер подсветки
  TMR_DOT, //таймер точек
  TMR_ANIM, //таймер анимаций
  TMR_GLITCH, //таймер глюков
  TIMERS_NUM //количество таймеров
};
uint16_t _timer_ms[TIMERS_NUM]; //таймер отсчета миллисекунд

//----------------Температура--------------
struct temp {
  uint16_t temp = 0; //температура
  uint16_t press = 0; //давление
  uint8_t hum = 0; //влажность
} sens;
void dataUpdate(void); //процедура обработки данных

//----------------Библиотеки----------------
#include <util/delay.h>

//---------------Конфигурации---------------
#include "config.h"
#include "connection.h"
#include "indiDisp.h"
#include "wire.h"
#include "EEPROM.h"
#include "RTC.h"
#include "BME.h"
#include "DHT.h"
#include "DS.h"
#include "WS.h"

//----------------Настройки----------------
struct Settings_1 {
  uint8_t indiBright[2] = {DEFAULT_INDI_BRIGHT_N, DEFAULT_INDI_BRIGHT}; //яркость индикаторов
  uint8_t backlBright[2] = {DEFAULT_BACKL_BRIGHT_N, DEFAULT_BACKL_BRIGHT}; //яркость подсветки
  uint8_t timeBright[2] = {DEFAULT_NIGHT_START, DEFAULT_NIGHT_END}; //время перехода яркости
  uint8_t timeHour[2] = {DEFAULT_HOUR_SOUND_START, DEFAULT_HOUR_SOUND_END}; //время звукового оповещения нового часа
  boolean timeFormat = DEFAULT_TIME_FORMAT; //формат времени
  boolean knock_sound = DEFAULT_KNOCK_SOUND; //звук кнопок
  uint8_t sensorSet = DEFAULT_TEMP_SENSOR; //сенсор температуры
  int8_t tempCorrect = DEFAULT_TEMP_CORRECT; //коррекция температуры
  boolean glitchMode = DEFAULT_GLITCH_MODE; //режим глюков
  uint16_t timePeriod = US_PERIOD; //коррекция хода внутреннего осцилятора
  uint8_t autoTempTime = DEFAULT_AUTO_TEMP_TIME; //интервал времени показа температуры
} mainSettings;

struct Settings_2 {
  uint8_t flipMode = DEFAULT_FLIP_ANIM; //режим анимации
  uint8_t backlMode = DEFAULT_BACKL_MODE; //режим подсветки
  uint8_t backlColor = DEFAULT_BACKL_COLOR * 10; //цвет подсветки
  uint8_t dotMode = DEFAULT_DOT_MODE; //режим точек
} fastSettings;

//переменные обработки кнопок
uint16_t btn_tmr; //таймер тиков обработки кнопок
boolean btn_check; //флаг разрешения опроса кнопки
boolean btn_state; //флаг текущего состояния кнопки

enum {
  KEY_NULL,        //кнопка не нажата
  LEFT_KEY_PRESS,  //клик левой кнопкой
  LEFT_KEY_HOLD,   //удержание левой кнопки
  RIGHT_KEY_PRESS, //клик правой кнопкой
  RIGHT_KEY_HOLD,  //удержание правой кнопки
  SET_KEY_PRESS,   //клик средней кнопкой
  SET_KEY_HOLD,    //удержание средней кнопки
  ADD_KEY_PRESS,   //клик дополнительной кнопкой
  ADD_KEY_HOLD     //удержание дополнительной кнопки
};

enum {
  SET_TIME_FORMAT,  //формат времени
  SET_GLITCH,       //глюки
  SET_BTN_SOUND,    //звук кнопок
  SET_HOUR_TIME,    //звук смены часа
  SET_BRIGHT_TIME,  //время смены яркости
  SET_INDI_BRIGHT,  //яркость индикаторов
  SET_BACKL_BRIGHT, //яркость подсветки
  SET_TEMP_SENS,    //настройка датчика температуры
  SET_AUTO_TEMP,    //автопоказ температуры
  SET_TIME_CORRECT, //корректировка хода времени
  SET_MAX_ITEMS     //максимум пунктов меню
};

enum {
  FAST_BACKL_MODE,  //режим подсветки
  FAST_FLIP_MODE,   //режим перелистывания
  FAST_DOT_MODE,    //режим точек
  FAST_BACKL_COLOR  //цвет подсветки
};

enum {
  FLIP_BRIGHT,            //плавное угасание и появление
  FLIP_ORDER_OF_NUMBERS,  //перемотка по порядку числа
  FLIP_ORDER_OF_CATHODES, //перемотка по порядку катодов в лампе
  FLIP_TRAIN,             //поезд
  FLIP_RUBBER_BAND,       //резинка
  FLIP_WAVE,              //волна
  FLIP_HIGHLIGHTS,        //блики
  FLIP_EVAPORATION,       //испарение
  FLIP_EFFECT_NUM         //максимум эффектов перелистывания
};

enum {
  BACKL_OFF,                 //выключена
  BACKL_STATIC,              //статичная
  BACKL_PULS,                //дыхание
  BACKL_PULS_COLOR,          //дыхание со сменой цвета при затухании
  BACKL_RUNNING_FIRE,        //бегущий огонь
  BACKL_RUNNING_FIRE_COLOR,  //бегущий огонь со сменой цвета
  BACKL_WAVE,                //волна
  BACKL_WAVE_COLOR,          //волна со сменой цвета
  BACKL_SMOOTH_COLOR_CHANGE, //плавная смена цвета
  BACKL_RAINBOW,             //радуга
  BACKL_CONFETTI,            //конфетти
  BACKL_EFFECT_NUM           //максимум эффектов подсветки
};

enum {
  DOT_OFF,          //выключена
  DOT_STATIC,       //статичная
  DOT_PULS,         //плавно мигает
  DOT_BLINK,        //одиночное мигание
  DOT_DOOBLE_BLINK  //двойное мигание
};

boolean _animShow; //флаг анимации
boolean _sec; //флаг обновления секунды
boolean _dot; //флаг обновления точек

uint8_t dotBrightStep; //шаг мигания точек
uint8_t dotBrightTime; //период шага мигания точек
uint8_t dotMaxBright; //максимальная яркость точек

uint16_t backlPulsBrightTime; //период шага "дыхания" подсветки
uint16_t backlWaveBrightTime; //период шага "волна" подсветки
uint8_t backlMaxBright; //максимальная яркость подсветки

uint8_t indiMaxBright; //максимальная яркость индикаторов

//alarmRead/Write - час | минута | режим(0 - выкл, 1 - одиночный, 2 - вкл, 3 - по будням, 4 - по дням недели) | день недели(вс,сб,пт,чт,ср,вт,пн,null) | мелодия будильника
uint8_t alarms_num; //текущее количество будильников

boolean alarmEnabled; //флаг включенного будильника
boolean alarmWaint; //флаг ожидания звука будильника
uint8_t alarm; //флаг активации будильника
uint8_t minsAlarm; //таймер полного отключения будильника
uint8_t minsAlarmSound; //таймер ожидания отключения звука будильника
uint8_t minsAlarmWaint; //таймер ожидания повторного включения будильника

uint8_t _tmrBurn; //таймер активации антиотравления
uint8_t _tmrTemp; //таймер автоматического отображения температуры
uint8_t _tmrGlitch; //таймер активации глюков

uint8_t timerMode; //режим таймера/секундомера
uint16_t timerCnt; //счетчик таймера/секундомера
uint16_t timerTime = TIMER_TIME; //время таймера сек

volatile uint16_t cnt_puls; //количество циклов для работы пищалки
volatile uint16_t cnt_freq; //частота для генерации звука пищалкой
uint16_t tmr_score; //частота для генерации звука пищалкой

uint8_t semp; //переключатель семплов мелодии
#define MELODY_PLAY(melody) _melody_chart(melody) //воспроизведение мелодии
#define MELODY_RESET semp = 0; _timer_ms[TMR_MELODY] = 0 //сброс мелодии

#define BTN_GIST_TICK (BTN_GIST_TIME / MS_PERIOD) //количество циклов для защиты от дребезга
#define BTN_HOLD_TICK (BTN_HOLD_TIME / MS_PERIOD) //количество циклов после которого считается что кнопка зажата

#define EEPROM_BLOCK_TIME EEPROM_BLOCK_NULL //блок памяти времени
#define EEPROM_BLOCK_SETTINGS_FAST (EEPROM_BLOCK_TIME + sizeof(RTC)) //блок памяти настроек свечения
#define EEPROM_BLOCK_SETTINGS_MAIN (EEPROM_BLOCK_SETTINGS_FAST + sizeof(fastSettings)) //блок памяти основных настроек
#define EEPROM_BLOCK_ALARM (EEPROM_BLOCK_SETTINGS_MAIN + sizeof(mainSettings)) //блок памяти количества будильников

#define EEPROM_BLOCK_CRC (EEPROM_BLOCK_ALARM + sizeof(alarms_num)) //блок памяти контрольной суммы настроек
#define EEPROM_BLOCK_CRC_TIME (EEPROM_BLOCK_CRC + 1) //блок памяти контрольной суммы времени
#define EEPROM_BLOCK_CRC_MAIN (EEPROM_BLOCK_CRC_TIME + 1) //блок памяти контрольной суммы основных настроек
#define EEPROM_BLOCK_CRC_FAST (EEPROM_BLOCK_CRC_MAIN + 1) //блок памяти контрольной суммы быстрых настроек
#define EEPROM_BLOCK_ALARM_DATA (EEPROM_BLOCK_CRC_FAST + 1) //первая ячейка памяти будильников

#define MAX_ALARMS ((1023 - EEPROM_BLOCK_ALARM_DATA) / 5) //максимальное количество будильников

#if BTN_TYPE
#define SET_CHK checkKeyADC(BTN_SET_MIN, BTN_SET_MAX) //чтение средней аналоговой кнопки
#define LEFT_CHK checkKeyADC(BTN_LEFT_MIN, BTN_LEFT_MAX) //чтение левой аналоговой кнопки
#define RIGHT_CHK checkKeyADC(BTN_RIGHT_MIN, BTN_RIGHT_MAX) //чтение правой аналоговой кнопки
#endif

//----------------------------------Инициализация----------------------------------
int main(void) //инициализация
{
#if BTN_TYPE
  initADC(); //инициализация АЦП
#else
  SET_INIT; //инициализация средней кнопки
  LEFT_INIT; //инициализация левой кнопки
  RIGHT_INIT; //инициализация правой кнопки
#endif

#if !BTN_ADD_DISABLE
  ADD_INIT; //инициализация дополнительной кнопки
#endif

#if !GEN_DISABLE
  CONV_INIT; //инициализация преобразователя
#endif
  SQW_INIT; //инициализация счета секунд
  BACKL_INIT; //инициализация подсветки
  BUZZ_INIT; //инициализация бузера

  uartDisable(); //отключение uart

  if (checkSettingsCRC() || !SET_CHK) { //если контрольная сумма не совпала или зажата средняя кнопка, восстанавливаем из переменных
    updateData((uint8_t*)&RTC, sizeof(RTC), EEPROM_BLOCK_TIME, EEPROM_BLOCK_CRC_TIME); //записываем дату и время в память
    updateData((uint8_t*)&fastSettings, sizeof(fastSettings), EEPROM_BLOCK_SETTINGS_FAST, EEPROM_BLOCK_CRC_FAST); //записываем настройки яркости в память
    updateData((uint8_t*)&mainSettings, sizeof(mainSettings), EEPROM_BLOCK_SETTINGS_MAIN, EEPROM_BLOCK_CRC_MAIN); //записываем основные настройки в память
#if ALARM_TYPE
    EEPROM_UpdateByte(EEPROM_BLOCK_ALARM, alarms_num); //записываем количество будильников в память
#endif
  }
  else if (LEFT_CHK) { //иначе загружаем настройки из памяти
    if (checkData(sizeof(RTC), EEPROM_BLOCK_TIME, EEPROM_BLOCK_CRC_TIME)) updateData((uint8_t*)&RTC, sizeof(RTC), EEPROM_BLOCK_TIME, EEPROM_BLOCK_CRC_TIME); //записываем дату и время в память
    if (checkData(sizeof(fastSettings), EEPROM_BLOCK_SETTINGS_FAST, EEPROM_BLOCK_CRC_FAST)) updateData((uint8_t*)&fastSettings, sizeof(fastSettings), EEPROM_BLOCK_SETTINGS_FAST, EEPROM_BLOCK_CRC_FAST); //записываем настройки яркости в память
    else EEPROM_ReadBlock((uint16_t)&fastSettings, EEPROM_BLOCK_SETTINGS_FAST, sizeof(fastSettings)); //считываем настройки яркости из памяти
    if (checkData(sizeof(mainSettings), EEPROM_BLOCK_SETTINGS_MAIN, EEPROM_BLOCK_CRC_MAIN)) updateData((uint8_t*)&mainSettings, sizeof(mainSettings), EEPROM_BLOCK_SETTINGS_MAIN, EEPROM_BLOCK_CRC_MAIN); //записываем основные настройки в память
    else EEPROM_ReadBlock((uint16_t)&mainSettings, EEPROM_BLOCK_SETTINGS_MAIN, sizeof(mainSettings)); //считываем основные настройки из памяти
#if ALARM_TYPE
    alarms_num = EEPROM_ReadByte(EEPROM_BLOCK_ALARM); //считываем количество будильников из памяти
#endif
  }

#if ALARM_TYPE == 1
  initAlarm(); //инициализация будильника
#endif

  WireInit(); //инициализация шины Wire
  IndiInit(); //инициализация индикаторов

  if (testRTC()) buzz_pulse(RTC_ERROR_SOUND_FREQ, RTC_ERROR_SOUND_TIME); //сигнал ошибки модуля часов
  if (!RIGHT_CHK) testLamp(); //если правая кнопка зажата запускаем тест системы

  randomSeed(RTC.s * (RTC.m + RTC.h) + RTC.DD * RTC.MM); //радомный сид для глюков
  _tmrGlitch = random(GLITCH_MIN, GLITCH_MAX); //находим рандомное время появления глюка
  changeBright(); //установка яркости от времени суток
#if ALARM_TYPE
  checkAlarms(); //проверка будильников
#endif
  //----------------------------------Главная----------------------------------
  for (;;) //главная
  {
    dataUpdate(); //обработка данных
#if !BTN_ADD_DISABLE
    timerWarn(); //тревога таймера
#endif
#if ALARM_TYPE
    alarmWarn(); //тревога будильника
#endif
    dotFlash(); //мигаем точками
    mainScreen(); //главный экран
  }
  return 0; //конец
}
//---------------------------------------Прерывание от RTC----------------------------------------------
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
      TIMSK2 &= ~(0x01 << OCIE2B); //выключаем таймер
    }
  }
}
//---------------------------------Инициализация АЦП----------------------------------------------------
void initADC(void) //инициализация АЦП
{
  ADCSRA = (0x01 << ADEN) | (0x01 << ADPS0) | (0x01 << ADPS2); //настройка АЦП
  ADMUX = (0x01 << REFS0) | (0x01 << ADLAR) | ANALOG_BTN_PIN; //настройка мультиплексатора АЦП
}
//-----------------------------Инициализация будильника------------------------------------------------
void initAlarm(void) //инициализация будильника
{
  if (!alarms_num) newAlarm(); //создать новый будильник
  else if (alarms_num > 1) { //если будильников в памяти больше одного
    alarms_num = 1; //оставляем один будильник
    EEPROM_UpdateByte(EEPROM_BLOCK_ALARM, alarms_num); //записываем будильник в память
  }
}
//----------------------------------Отключение uart----------------------------------------------------
void uartDisable(void) //отключение uart
{
  UCSR0B = 0; //выключаем UART
  PRR |= (0x01 << PRUSART0); //выключаем питание UART
}
//------------------------------------Проверка системы-------------------------------------------------
void testLamp(void) //проверка системы
{
#if BACKL_WS2812B
  setLedBright(DEFAULT_BACKL_BRIGHT); //устанавливаем максимальную яркость
#else
  backlSetBright(DEFAULT_BACKL_BRIGHT); //если посветка статичная, устанавливаем яркость
#endif
  fastSettings.backlMode |= 0x80; //запретили эффекты подсветки
  dotSetBright(DOT_BRIGHT); //установка яркости точек
  while (1) {
    for (byte indi = 0; indi < LAMP_NUM; indi++) {
      indiClr(); //очистка индикаторов
      for (byte digit = 0; digit < 10; digit++) {
        indiPrintNum(digit, indi); //отрисовываем цифру
#if BACKL_WS2812B
        setLedHue(indi, digit * 25); //отправляем статичный цвет
#endif
        for (_timer_ms[TMR_MS] = TEST_LAMP_TIME; _timer_ms[TMR_MS];) { //ждем
          dataUpdate(); //обработка данных
          MELODY_PLAY(0); //воспроизводим мелодию
          if (check_keys()) return; //возврат если нажата кнопка
        }
      }
#if BACKL_WS2812B
      clrLed(indi); //очищаем светодиод
#endif
    }
  }
}
//------------------------Проверка модуля часов реального времени--------------------------------------
boolean testRTC(void) //проверка модуля часов реального времени
{
  if (disable32K()) return 1; //отключение вывода 32K
  if (setSQW()) return 1; //установка SQW на 1Гц

  EICRA = (0x01 << ISC01); //настраиваем внешнее прерывание по спаду импульса на INT0
  EIFR |= (0x01 << INTF0); //сбрасываем флаг прерывания INT0

  for (_timer_ms[TMR_MS] = TEST_SQW_TIME; !(EIFR & (0x01 << INTF0)) && _timer_ms[TMR_MS];) { //ждем сигнала от SQW
    for (; tick_ms; tick_ms--) { //если был тик, обрабатываем данные
      if (_timer_ms[TMR_MS] > MS_PERIOD) _timer_ms[TMR_MS] -= MS_PERIOD; //если таймер больше периода
      else if (_timer_ms[TMR_MS]) _timer_ms[TMR_MS] = 0; //иначе сбрасываем таймер
    }
  }

  if (getTime()) return 1; //считываем время из RTC
  if (getOSF()) { //если пропадало питание
    EEPROM_ReadBlock((uint16_t)&RTC, EEPROM_BLOCK_TIME, sizeof(RTC)); //считываем дату и время из памяти
    sendTime(); //отправить время в RTC
  }

  if (EIFR & (0x01 << INTF0)) { //если был сигнал с SQW
    EIFR |= (0x01 << INTF0); //сбрасываем флаг прерывания INT0
    EIMSK = (0x01 << INT0); //разрешаем внешнее прерывание INT0
  }
  else return 1; //иначе выдаем ошибку

  return 0; //возвращаем статус "ок"
}
//------------------------Проверка данных в памяти--------------------------------------------------
boolean checkData(uint8_t size, uint8_t cell, uint8_t cellCRC) //проверка данных в памяти
{
  uint8_t crc = 0;
  for (uint8_t n = 0; n < size; n++) checkCRC(&crc, EEPROM_ReadByte(cell + n));
  if (crc != EEPROM_ReadByte(cellCRC)) return 1;
  return 0;
}
//------------------------Обновление данных в памяти--------------------------------------------------
void updateData(uint8_t* str, uint8_t size, uint8_t cell, uint8_t cellCRC) //обновление данных в памяти
{
  uint8_t crc = 0;
  for (uint8_t n = 0; n < size; n++) checkCRC(&crc, str[n]);
  EEPROM_UpdateBlock((uint16_t)str, cell, size);
  EEPROM_UpdateByte(cellCRC, crc);
}
//------------------------Сверка контрольной суммы--------------------------------------------------
void checkCRC(uint8_t* crc, uint8_t data) //сверка контрольной суммы
{
  for (uint8_t i = 0; i < 8; i++) { //считаем для всех бит
    *crc = ((*crc ^ data) & 0x01) ? (*crc >> 0x01) ^ 0x8C : (*crc >> 0x01); //рассчитываем значение
    data >>= 0x01; //сдвигаем буфер
  }
}
//------------------------Проверка контрольной суммы настроек-----------------------------------------
boolean checkSettingsCRC(void) //проверка контрольной суммы настроек
{
  uint8_t CRC = 0;

  for (uint8_t i = 0; i < sizeof(RTC); i++) checkCRC(&CRC, *((uint8_t*)&RTC + i));
  for (uint8_t i = 0; i < sizeof(mainSettings); i++) checkCRC(&CRC, *((uint8_t*)&mainSettings + i));
  for (uint8_t i = 0; i < sizeof(fastSettings); i++) checkCRC(&CRC, *((uint8_t*)&fastSettings + i));

  if (EEPROM_ReadByte(EEPROM_BLOCK_CRC) == CRC) return 0;
  else EEPROM_UpdateByte(EEPROM_BLOCK_CRC, CRC);
  return 1;
}
//--------------------------------Генерация частот бузера-----------------------------------------------
void buzz_pulse(uint16_t freq, uint16_t time) //генерация частоты бузера (частота 10..10000, длительность мс.)
{
  cnt_puls = ((uint32_t)freq * (uint32_t)time) / 500; //пересчитываем частоту и время в циклы таймера
  cnt_freq = tmr_score = (1000000 / freq); //пересчитываем частоту в циклы полуволны
  OCR2B = 255; //устанавливаем COMB в начало
  TIMSK2 |= (0x01 << OCIE2B); //запускаем таймер
}
//---------------------------------Воспроизведение мелодии-----------------------------------------------
void _melody_chart(uint8_t melody) //воспроизведение мелодии
{
  if (!_timer_ms[TMR_MELODY]) { //если пришло время
    buzz_pulse(pgm_read_word((uint16_t*)(alarm_sound[melody][0] + (6 * semp))), pgm_read_word((uint16_t*)(alarm_sound[melody][0] + (6 * semp) + 2))); //запускаем звук с задоной частотой и временем
    _timer_ms[TMR_MELODY] = pgm_read_word((uint16_t*)(alarm_sound[melody][0] + (6 * semp) + 4)); //устанавливаем паузу перед воспроизведением нового звука
    if (++semp > alarm_sound[melody][1] - 1) semp = 0; //переключпем на следующий семпл
  }
}
//----------------------------------Проверка будильников----------------------------------------------------
void checkAlarms(void) //проверка будильников
{
  alarmEnabled = 0; //сбрасываем флаг включенного будильника
  for (uint8_t alm = 0; alm < alarms_num; alm++) { //опрашиваем все будильники
    if (alarmRead(alm, 2)) { //если будильник включен
      alarmEnabled = 1; //устанавливаем флаг включенного будильника
      if (RTC.h == alarmRead(alm, 0) && RTC.m == alarmRead(alm, 1) && (alarmRead(alm, 2) < 3 || (alarmRead(alm, 2) == 3 && RTC.DW < 6) || (alarmRead(alm, 3) & (0x01 << RTC.DW)))) {
        alarm = alm + 1; //устанавливаем флаг тревоги
        return; //выходим
      }
    }
  }
}
//----------------------------Обновление данных будильников--------------------------------------------------
void alarmDataUpdate(void) //обновление данных будильников
{
  if (alarm) { //если тревога активна
    if (++minsAlarm >= ALARM_TIMEOUT) { //если пришло время выключить будильник
      alarmReset(); //сброс будильника
      MELODY_RESET; //сброс позиции мелодии
      return; //выходим
    }

    if (ALARM_WAINT && alarmWaint) { //если будильник в режиме ожидания
      if (++minsAlarmWaint >= ALARM_WAINT) { //если пришло время повторно включить звук
        alarmWaint = 0; //сбрасываем флаг ожидания
        minsAlarmWaint = 0; //сбрасываем таймер ожидания
      }
    }
    else if (ALARM_TIMEOUT_SOUND) { //если таймаут тревоги включен
      if (++minsAlarmSound >= ALARM_TIMEOUT_SOUND) { //если пришло время выключить тревогу
        if (ALARM_WAINT) { //если время ожидания включено
          alarmWaint = 1; //устанавливаем флаг ожидания тревоги
          minsAlarmSound = 0; //сбрасываем таймер таймаута тревоги
        }
        else alarmReset(); //сброс будильника
        MELODY_RESET; //сброс позиции мелодии
      }
    }
  }
  else checkAlarms(); //иначе проверяем будильники на совподение
}
//----------------------------------Тревога будильника---------------------------------------------------------
void alarmWarn(void) //тревога будильника
{
  if (alarm && !alarmWaint) { //если флаг установлен флаг тревоги и флаг ожидания очещен
    boolean blink_data = 0; //флаг мигания индикаторами
    uint8_t soundNum = alarmRead(alarm - 1, 4); //считываем мелодию сигнала тревоги
    while (1) {
      dataUpdate(); //обработка данных

      if (!alarm || alarmWaint) { //если тревога сброшена
        _animShow = 0; //сбросить флаг анимации
        return; //выходим
      }

      MELODY_PLAY(soundNum); //воспроизводим мелодию

      if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
        _timer_ms[TMR_MS] = ALM_BLINK_TIME; //устанавливаем таймер

        switch (blink_data) {
          case 0:
            indiClr(); //очистка индикаторов
            dotSetBright(0); //выключаем точки
            break;
          case 1:
            indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
            indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
            indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
            dotSetBright(dotMaxBright); //включаем точки
            break;
        }
        blink_data = !blink_data; //мигаем временем
      }

      switch (check_keys()) {
        case LEFT_KEY_PRESS: //клик левой кнопкой
        case RIGHT_KEY_PRESS: //клик правой кнопкой
        case SET_KEY_PRESS: //клик средней кнопкой
        case ADD_KEY_PRESS: //клик дополнительной кнопкой
          if (ALARM_WAINT) { //если есть время ожидания
            alarmWaint = 1; //устанавливаем флаг ожидания
            minsAlarmSound = 0; //сбрасывем таймер ожидания
          }
          else {
            buzz_pulse(ALM_OFF_SOUND_FREQ, ALM_OFF_SOUND_TIME); //звук выключения будильника
            alarmReset(); //сброс будильника
          }
          MELODY_RESET; //сброс позиции мелодии
          _animShow = 0; //сбросить флаг анимации
          _sec = 0; //обновление экрана
          return; //выходим

        case LEFT_KEY_HOLD: //удержание левой кнопки
        case RIGHT_KEY_HOLD: //удержание правой кнопки
        case SET_KEY_HOLD: //удержание средней кнопки
        case ADD_KEY_HOLD: //удержание дополнительной кнопки
          buzz_pulse(ALM_OFF_SOUND_FREQ, ALM_OFF_SOUND_TIME); //звук выключения будильника
          alarmReset(); //сброс будильника
          MELODY_RESET; //сброс позиции мелодии
          _animShow = 0; //сбросить флаг анимации
          _sec = 0; //обновление экрана
          return; //выходим
      }
    }
  }
}
//----------------------------------Сброс будильника---------------------------------------------------------
void alarmReset(void) //сброс будильника
{
  if (alarmRead(alarm - 1, 2) == 1) EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + ((alarm - 1) * 5) + 2, 0); //если был установлен режим одиночный то выключаем будильник
  checkAlarms(); //проверка будильников
  alarmWaint = 0; //сбрасываем флаг ожидания
  minsAlarm = 0; //сбрасываем таймер отключения будильника
  minsAlarmWaint = 0; //сбрасываем таймер ожидания повторного включения тревоги
  minsAlarmSound = 0; //сбрасываем таймер отключения звука
  alarm = 0; //сбрасываем флаг тревоги
}
//----------------------------------Получить основные данные будильника---------------------------------------------------------
uint8_t alarmRead(uint8_t almNum, uint8_t almData) //получить основные данные будильника
{
  return EEPROM_ReadByte(EEPROM_BLOCK_ALARM_DATA + (almNum * 5) + almData); //возвращаем запрошеный байт
}
//----------------------------------Получить блок основных данных будильника---------------------------------------------------------
void alarmReadBlock(uint8_t almNum, uint8_t* data) //получить блок основных данных будильника
{
  for (uint8_t i = 0; i < 5; i++) data[i] = (almNum) ? EEPROM_ReadByte(EEPROM_BLOCK_ALARM_DATA + ((almNum - 1) * 5) + i) : 0; //считываем блок данных
}
//----------------------------------Записать блок основных данных будильника---------------------------------------------------------
void alarmWriteBlock(uint8_t almNum, uint8_t* data) //записать блок основных данных будильника
{
  if (!almNum) return; //если нет ни одного будильника - выходим
  for (uint8_t i = 0; i < 5; i++) EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + ((almNum - 1) * 5) + i, data[i]); //записываем блок данных
}
//---------------------Создать новый будильник---------------------------------------------------------
void newAlarm(void) //создать новый будильник
{
  if (alarms_num < MAX_ALARMS) { //если новый будильник меньше максимума
    EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + (alarms_num * 5), DEFAULT_ALARM_TIME_HH); //устанавливаем час по умолчанию
    EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + (alarms_num * 5) + 1, DEFAULT_ALARM_TIME_MM); //устанавливаем минуты по умолчанию
    EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + (alarms_num * 5) + 2, DEFAULT_ALARM_MODE); //устанавливаем режим по умолчанию
    EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + (alarms_num * 5) + 3, 0); //устанавливаем дни недели по умолчанию
    EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + (alarms_num * 5) + 4, DEFAULT_ALARM_SOUND); //устанавливаем мелодию по умолчанию
    EEPROM_UpdateByte(EEPROM_BLOCK_ALARM, ++alarms_num); //записываем количество будильников в память
  }
}
//---------------------Удалить будильник---------------------------------------------------------
void delAlarm(uint8_t alarm) //удалить будильник
{
  if (alarms_num) { //если будильник доступен
    for (uint8_t start = alarm; start < alarms_num; start++) { //перезаписываем массив будильников
      EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + (alarms_num * 5), EEPROM_ReadByte(EEPROM_BLOCK_ALARM_DATA + ((alarms_num + 1) * 5)));
      EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + (alarms_num * 5) + 1, EEPROM_ReadByte(EEPROM_BLOCK_ALARM_DATA + ((alarms_num + 1) * 5) + 1));
      EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + (alarms_num * 5) + 2, EEPROM_ReadByte(EEPROM_BLOCK_ALARM_DATA + ((alarms_num + 1) * 5) + 2));
      EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + (alarms_num * 5) + 3, EEPROM_ReadByte(EEPROM_BLOCK_ALARM_DATA + ((alarms_num + 1) * 5) + 3));
      EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + (alarms_num * 5) + 4, EEPROM_ReadByte(EEPROM_BLOCK_ALARM_DATA + ((alarms_num + 1) * 5) + 4));
    }
    EEPROM_UpdateByte(EEPROM_BLOCK_ALARM, --alarms_num); //записываем количество будильников в память
  }
}
//----------------------------------Обработка данных---------------------------------------------------------
void dataUpdate(void) //обработка данных
{
  static uint32_t timeNotRTC; //счетчик реального времени
  static uint16_t timerCorrect; //остаток для коррекции времени
#if BACKL_WS2812B
  backlEffect(); //анимация подсветки
#else
  backlFlash(); //"дыхание" подсветки
#endif

  for (; tick_ms > 0; tick_ms--) { //если был тик, обрабатываем данные
    switch (btn_state) { //таймер опроса кнопок
      case 0: if (btn_check) btn_tmr++; break; //считаем циклы
      case 1: if (btn_tmr) btn_tmr--; break; //убираем дребезг
    }

    if (!EIMSK) { //если внешние часы не обнаружены
      timeNotRTC += mainSettings.timePeriod; //добавляем ко времени период таймера
      if (timeNotRTC > 1000000UL) { //если прошла секунда
        timeNotRTC -= 1000000UL; //оставляем остаток
        tick_sec++; //прибавляем секунду
      }
    }

    timerCorrect += mainSettings.timePeriod % 1000; //остаток для коррекции
    uint16_t msDec = (mainSettings.timePeriod + timerCorrect) / 1000; //находим целые мс
    for (uint8_t tm = 0; tm < TIMERS_NUM; tm++) { //опрашиваем все таймеры
      if (_timer_ms[tm] > msDec) _timer_ms[tm] -= msDec; //если таймер больше периода
      else if (_timer_ms[tm]) _timer_ms[tm] = 0; //иначе сбрасываем таймер
    }
    if (timerCorrect >= 1000) timerCorrect -= 1000; //если коррекция больше либо равна 1 мс
  }

  for (; tick_sec > 0; tick_sec--) { //если был тик, обрабатываем данные
    //счет времени
    if (++RTC.s > 59) { //секунды
      RTC.s = 0;
      if (++RTC.m > 59) { //минуты
        RTC.m = 0;
        if (++RTC.h > 23) { //часы
          RTC.h = 0;
          if (++RTC.DW > 7) RTC.DW = 1; //день недели
          if (++RTC.DD > maxDays()) { //дата
            RTC.DD = 1;
            if (++RTC.MM > 12) { //месяц
              RTC.MM = 1;
              if (++RTC.YY > 2050) { //год
                RTC.YY = 2021;
              }
            }
          }
        }
        hourSound(); //звук смены часа
        changeBright(); //установка яркости от времени суток
      }
      _tmrBurn++; //прибавляем минуту к таймеру антиотравления
      _animShow = 1; //показать анимацию переключения цифр
#if ALARM_TYPE
      alarmDataUpdate(); //проверка будильников
#endif
    }
#if !BTN_ADD_DISABLE
    switch (timerMode) {
      case 1: if (timerCnt != 65535) timerCnt++; break;
      case 2: if (timerCnt) timerCnt--; break;
    }
#endif
    _sec = _dot = 0; //очищаем флаги секунды и точек
  }
}
//------------------------------------Звук смены часа------------------------------------
void hourSound(void) //звук смены часа
{
  if (!alarm || alarmWaint) { //если будильник не работает
    if ((mainSettings.timeHour[1] > mainSettings.timeHour[0] && RTC.h < mainSettings.timeHour[1] && RTC.h >= mainSettings.timeHour[0]) ||
        (mainSettings.timeHour[1] < mainSettings.timeHour[0] && (RTC.h < mainSettings.timeHour[1] || RTC.h >= mainSettings.timeHour[0]))) {
      buzz_pulse(HOUR_SOUND_FREQ, HOUR_SOUND_TIME); //звук смены часа
    }
  }
}
//------------------------------------Имитация глюков------------------------------------
void glitchMode(void) //имитация глюков
{
  if (mainSettings.glitchMode) { //если глюки включены
    if (!_tmrGlitch-- && RTC.s > 7 && RTC.s < 55) { //если пришло время
      boolean indiState = 0; //состояние индикатора
      uint8_t glitchCounter = random(GLITCH_NUM_MIN, GLITCH_NUM_MAX); //максимальное количество глюков
      uint8_t glitchIndic = random(0, LAMP_NUM); //номер индикатора
      uint8_t indiSave = indiGet(glitchIndic); //сохраняем текущую цифру в индикаторе
      while (!check_keys()) {
        dataUpdate(); //обработка данных
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
      indiSet(indiSave, glitchIndic); //восстанавливаем состояние индикатора
    }
  }
}
//-----------------------Проверка аналоговой кнопки-----------------------------------------------
boolean checkKeyADC(uint8_t minADC, uint8_t maxADC) //проверка аналоговой кнопки
{
  ADCSRA |= (1 << ADSC); //запускаем преобразование
  while (ADCSRA & (1 << ADSC)); //ждем окончания преобразования
  return !(minADC < ADCH && ADCH <= maxADC); //возвращаем результат опроса
}
//-----------------------------Проверка кнопок----------------------------------------------------
uint8_t check_keys(void) //проверка кнопок
{
  static uint8_t btn_switch; //флаг мультиплексатора кнопок

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
#if !BTN_ADD_DISABLE
      else if (!ADD_CHK) { //если нажата дополнительная кл.
        btn_switch = 4; //выбираем клавишу опроса
        btn_state = 0; //обновляем текущее состояние кнопки
      }
#endif
      else btn_state = 1; //обновляем текущее состояние кнопки
      break;
    case 1: btn_state = SET_CHK; break; //опрашиваем клавишу ок
    case 2: btn_state = LEFT_CHK; break; //опрашиваем левую клавишу
    case 3: btn_state = RIGHT_CHK; break; //опрашиваем правую клавишу
#if !BTN_ADD_DISABLE
    case 4: btn_state = ADD_CHK; break; //опрашиваем дополнительную клавишу
#endif
  }

  switch (btn_state) { //переключаемся в зависимости от состояния клавиши
    case 0:
      if (btn_check) { //если разрешена провекрка кнопки
        if (btn_tmr > BTN_HOLD_TICK) { //если таймер больше длительности удержания кнопки
          btn_tmr = BTN_GIST_TICK; //сбрасываем таймер на антидребезг
          btn_check = 0; //запрещем проврку кнопки
          switch (btn_switch) { //переключаемся в зависимости от состояния мультиопроса
            case 1: return SET_KEY_HOLD; //возвращаем удержание средней кнопки
            case 2: return LEFT_KEY_HOLD; //возвращаем удержание левой кнопки
            case 3: return RIGHT_KEY_HOLD; //возвращаем удержание правой кнопки
#if !BTN_ADD_DISABLE
            case 4: return ADD_KEY_HOLD; //возвращаем удержание дополнительной кнопки
#endif
          }
        }
      }
      break;

    case 1:
      if (btn_tmr > BTN_GIST_TICK) { //если таймер больше времени антидребезга
        btn_tmr = BTN_GIST_TICK; //сбрасываем таймер на антидребезг
        btn_check = 0; //запрещем проврку кнопки
        if (mainSettings.knock_sound) buzz_pulse(KNOCK_SOUND_FREQ, KNOCK_SOUND_TIME); //щелчок пищалкой
        switch (btn_switch) { //переключаемся в зависимости от состояния мультиопроса
          case 1: return SET_KEY_PRESS; //возвращаем клик средней кнопкой
          case 2: return LEFT_KEY_PRESS; //возвращаем клик левой кнопкой
          case 3: return RIGHT_KEY_PRESS; //возвращаем клик правой кнопкой
#if !BTN_ADD_DISABLE
          case 4: return ADD_KEY_PRESS; //возвращаем клик дополнительной кнопкой
#endif
        }
      }
      else if (!btn_tmr) {
        btn_check = 1; //разрешаем проврку кнопки
        btn_switch = 0; //сбрасываем мультиплексатор кнопок
      }
      break;
  }

  return KEY_NULL;
}
//----------------------------Настройки времени----------------------------------
void settings_time(void) //настройки времени
{
  boolean blink_data = 0; //мигание сигментами
  uint8_t cur_mode = 0; //текущий режим
  uint8_t time_out = 0; //таймаут автовыхода

  indiClr(); //очищаем индикаторы
  dotSetBright(dotMaxBright); //включаем точки

  //настройки
  while (1) {
    dataUpdate(); //обработка данных

    if (!_sec) {
      _sec = 1;
      if (++time_out >= SETTINGS_TIMEOUT) return;
    }

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

      indiClr(); //очистка индикаторов
      indiPrintNum(cur_mode + 1, 5); //режим
      switch (cur_mode) {
        case 0:
        case 1:
          if (!blink_data || cur_mode != 0) indiPrintNum(RTC.h, 0, 2, 0); //вывод часов
          if (!blink_data || cur_mode != 1) indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
          break;
        case 2:
        case 3:
          if (!blink_data || cur_mode != 2) indiPrintNum(RTC.DD, 0, 2, 0); //вывод даты
          if (!blink_data || cur_mode != 3) indiPrintNum(RTC.MM, 2, 2, 0); //вывод месяца
          break;
        case 4:
          if (!blink_data) indiPrintNum(RTC.YY, 0); //вывод года
          break;
      }
      blink_data = !blink_data; //мигание сигментами
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (check_keys()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (cur_mode) {
          //настройка времени
          case 0: if (RTC.h > 0) RTC.h--; else RTC.h = 23; RTC.s = 0; break; //часы
          case 1: if (RTC.m > 0) RTC.m--; else RTC.m = 59; RTC.s = 0; break; //минуты

          //настройка даты
          case 2: if (RTC.DD > 1) RTC.DD--; else RTC.DD = maxDays(); break; //день
          case 3: //месяц
            if (RTC.MM > 1) RTC.MM--;
            else RTC.MM = 12;
            if (RTC.DD > maxDays()) RTC.DD = maxDays();
            break;

          //настройка года
          case 4: if (RTC.YY > 2021) RTC.YY--; else RTC.YY = 2050; break; //год
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (cur_mode) {
          //настройка времени
          case 0: if (RTC.h < 23) RTC.h++; else RTC.h = 0; RTC.s = 0; break; //часы
          case 1: if (RTC.m < 59) RTC.m++; else RTC.m = 0; RTC.s = 0; break; //минуты

          //настройка даты
          case 2: if (RTC.DD < maxDays()) RTC.DD++; else RTC.DD = 1; break; //день
          case 3: //месяц
            if (RTC.MM < 12) RTC.MM++;
            else RTC.MM = 1;
            if (RTC.DD > maxDays()) RTC.DD = maxDays();
            break;

          //настройка года
          case 4: if (RTC.YY < 2050) RTC.YY++; else RTC.YY = 2021; break; //год
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        if (cur_mode < 4) cur_mode++; else cur_mode = 0;
        if (cur_mode != 4) dotSetBright(dotMaxBright); //включаем точки
        else dotSetBright(0); //выключаем точки
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        sendTime(); //отправить время в RTC
        changeBright(); //установка яркости от времени суток
        updateData((uint8_t*)&RTC, sizeof(RTC), EEPROM_BLOCK_TIME, EEPROM_BLOCK_CRC_TIME); //записываем дату и время в память
        return;
    }
  }
}
//-----------------------------Настройка будильника------------------------------------
void settings_singleAlarm(void) //настройка будильника
{
  boolean cur_indi = 0; //текущий индикатор
  boolean blink_data = 0; //мигание сигментами
  uint8_t alarm[5]; //массив данных о будильнике
  uint8_t cur_mode = 0; //текущий режим
  uint8_t cur_day = 1; //текущий день недели
  uint8_t time_out = 0; //таймаут автовыхода

  indiClr(); //очищаем индикаторы
  dotSetBright(dotMaxBright); //выключаем точки

  alarmReset(); //сброс будильника
  alarmReadBlock(1, alarm); //читаем блок данных

  while (1) {
    dataUpdate(); //обработка данных

    if (!_sec) {
      _sec = 1;
      if (++time_out >= SETTINGS_TIMEOUT) {
        MELODY_RESET; //сброс воспроизведения мелодии
        checkAlarms(); //проверка будильников
        return; //выходим по тайм-ауту
      }
    }

    if (cur_mode == 3) MELODY_PLAY(alarm[4]); //воспроизводим мелодию

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

      indiClr(); //очистка индикаторов
      indiPrintNum(cur_mode + 1, 5); //режим
      switch (cur_mode) {
        case 0:
          if (!blink_data || cur_indi) indiPrintNum(alarm[0], 0, 2, 0); //вывод часов
          if (!blink_data || !cur_indi) indiPrintNum(alarm[1], 2, 2, 0); //вывод минут
          break;
        case 1:
        case 2:
          if (!blink_data || cur_mode != 1) indiPrintNum(alarm[2], 0); //вывод режима
          if (alarm[2] > 3) {
            if (!blink_data || cur_mode != 2 || cur_indi) indiPrintNum(cur_day, 2); //вывод дня недели
            if (!blink_data || cur_mode != 2 || !cur_indi) indiPrintNum((alarm[3] >> cur_day) & 0x01, 3); //вывод установки
          }
          break;
        case 3:
          if (!blink_data) indiPrintNum(alarm[4] + 1, 3); //вывод номера мелодии
          break;
      }
      blink_data = !blink_data; //мигание сигментами
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (check_keys()) {
      case LEFT_KEY_HOLD: //удержание левой кнопки
        if (cur_mode > 0) cur_mode--; else cur_mode = 3; //переключение пунктов

        switch (cur_mode) {
          case 0: dotSetBright(dotMaxBright); break; //включаем точки
          case 2: if (alarm[2] < 4) cur_mode = 1; break; //если нет дней недели
          default: dotSetBright(0); break; //выключаем точки
        }

        MELODY_RESET; //сбрасываем мелодию
        cur_indi = 0; //сбрасываем текущий индикатор
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (cur_mode) {
          //настройка времени будильника
          case 0:
            switch (cur_indi) {
              case 0: if (alarm[0] > 0) alarm[0]--; else alarm[0] = 23; break; //часы
              case 1: if (alarm[1] > 0) alarm[1]--; else alarm[1] = 59; break; //минуты
            }
            break;
          //настройка режима будильника
          case 1: if (alarm[2] > 0) alarm[2]--; else alarm[2] = 4; break; //режим
          case 2:
            switch (cur_indi) {
              case 0: if (cur_day > 1) cur_day--; else cur_day = 7; break; //день недели
              case 1: alarm[3] &= ~(0x01 << cur_day); break; //установка
            }
            break;
          //настройка мелодии будильника
          case 3: if (alarm[4] > 0) alarm[4]--; else alarm[4] = (sizeof(alarm_sound) >> 2) - 1; MELODY_RESET; break; //мелодия
        }
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;
      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (cur_mode) {
          //настройка времени будильника
          case 0:
            switch (cur_indi) {
              case 0: if (alarm[0] < 23) alarm[0]++; else alarm[0] = 0; break; //часы
              case 1: if (alarm[1] < 59) alarm[1]++; else alarm[1] = 0; break; //минуты
            }
            break;
          //настройка режима будильника
          case 1: if (alarm[2] < 4) alarm[2]++; else alarm[2] = 0; break; //режим
          case 2:
            switch (cur_indi) {
              case 0: if (cur_day < 7) cur_day++; else cur_day = 1; break; //день недели
              case 1: alarm[3] |= (0x01 << cur_day); break; //установка
            }
            break;
          //настройка мелодии будильника
          case 3: if (alarm[4] < ((sizeof(alarm_sound) >> 2) - 1)) alarm[4]++; else alarm[4] = 0; MELODY_RESET; break; //мелодия
        }
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;
      case RIGHT_KEY_HOLD: //удержание правой кнопки
        if (cur_mode < 3) cur_mode++; else cur_mode = 0; //переключение пунктов

        switch (cur_mode) {
          case 0: dotSetBright(dotMaxBright); break; //включаем точки
          case 2: if (alarm[2] < 4) cur_mode = 3; break; //если нет дней недели
          default: dotSetBright(0); break; //выключаем точки
        }

        MELODY_RESET; //сбрасываем мелодию
        cur_indi = 0; //сбрасываем текущий индикатор
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;
      case SET_KEY_PRESS: //клик средней кнопкой
        cur_indi = !cur_indi;
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;
      case SET_KEY_HOLD: //удержание средней кнопки
        alarmWriteBlock(1, alarm); //записать блок основных данных будильника и выйти
        checkAlarms(); //проверка будильников
        return;
    }
  }
}
//-----------------------------Настройка будильников------------------------------------
void settings_multiAlarm(void) //настройка будильников
{
  boolean blink_data = 0; //мигание сигментами
  uint8_t alarm[5]; //массив данных о будильнике
  uint8_t cur_mode = 0; //текущий режим
  uint8_t cur_day = 1; //текущий день недели
  uint8_t time_out = 0; //таймаут автовыхода
  uint8_t curAlarm = alarms_num > 0;

  indiClr(); //очищаем индикаторы
  dotSetBright(0); //выключаем точки

  alarmReset(); //сброс будильника
  alarmReadBlock(curAlarm, alarm); //читаем блок данных

  while (1) {
    dataUpdate(); //обработка данных

    if (!_sec) {
      _sec = 1;
      if (++time_out >= SETTINGS_TIMEOUT) {
        MELODY_RESET; //сброс воспроизведения мелодии
        checkAlarms(); //проверка будильников
        return; //выходим по тайм-ауту
      }
    }

    if (cur_mode == 6) MELODY_PLAY(alarm[4]); //воспроизводим мелодию

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

      indiClr(); //очистка индикаторов
      indiPrintNum(cur_mode + 1, 5); //режим
      switch (cur_mode) {
        case 0:
        case 1:
          if (!blink_data || cur_mode != 0) indiPrintNum(curAlarm, 0, 2, 0); //вывод номера будильника
          if ((!blink_data || cur_mode != 1) && curAlarm) indiPrintNum(alarm[2], 3); //вывод режима
          break;
        case 2:
        case 3:
          if (!blink_data || cur_mode != 2) indiPrintNum(alarm[0], 0, 2, 0); //вывод часов
          if (!blink_data || cur_mode != 3) indiPrintNum(alarm[1], 2, 2, 0); //вывод минут
          break;
        case 4:
        case 5:
        case 6:
          if (alarm[2] > 3) {
            if (!blink_data || cur_mode != 4) indiPrintNum(cur_day, 0); //вывод дня недели
            if (!blink_data || cur_mode != 5) indiPrintNum((alarm[3] >> cur_day) & 0x01, 1); //вывод установки
          }
          if (!blink_data || cur_mode != 6) indiPrintNum(alarm[4] + 1, 2, 2, 0); //вывод номера мелодии
          break;
      }
      blink_data = !blink_data; //мигание сигментами
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (check_keys()) {
      case LEFT_KEY_HOLD: //удержание левой кнопки
        switch (cur_mode) {
          case 0:
            if (curAlarm) { //если есть будильники в памяти
              delAlarm(curAlarm - 1); //удалить текущий будильник
              dotSetBright(dotMaxBright); //включаем точки
              for (_timer_ms[TMR_MS] = 500; _timer_ms[TMR_MS];) dataUpdate(); //обработка данных
              dotSetBright(0); //выключаем точки
              if (curAlarm > (alarms_num > 0)) curAlarm--; //убавляем номер текущего будильника
              else curAlarm = (alarms_num > 0);
              alarmReadBlock(curAlarm, alarm); //читаем блок данных
            }
            break;
          default:
            cur_mode = 2; //режим настройки времени
            dotSetBright(dotMaxBright); //включаем точки
            break;
        }
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (cur_mode) {
          case 0: if (curAlarm > (alarms_num > 0)) curAlarm--; else curAlarm = alarms_num; alarmReadBlock(curAlarm, alarm); break; //будильник
          case 1: if (alarm[2] > 0) alarm[2]--; else alarm[2] = 4; break; //режим

          //настройка времени будильника
          case 2: if (alarm[0] > 0) alarm[0]--; else alarm[0] = 23; break; //часы
          case 3: if (alarm[1] > 0) alarm[1]--; else alarm[1] = 59; break; //минуты

          //настройка режима будильника
          case 4: if (cur_day > 1) cur_day--; else cur_day = 7; break; //день недели
          case 5: alarm[3] &= ~(0x01 << cur_day); break; //установка
          case 6: if (alarm[4] > 0) alarm[4]--; else alarm[4] = (sizeof(alarm_sound) >> 2) - 1; MELODY_RESET; break; //мелодия
        }
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;
      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (cur_mode) {
          case 0: if (curAlarm < alarms_num) curAlarm++; else curAlarm = (alarms_num > 0); alarmReadBlock(curAlarm, alarm); break; //будильник
          case 1: if (alarm[2] < 4) alarm[2]++; else alarm[2] = 0; break; //режим

          //настройка времени будильника
          case 2: if (alarm[0] < 23) alarm[0]++; else alarm[0] = 0; break; //часы
          case 3: if (alarm[1] < 59) alarm[1]++; else alarm[1] = 0; break; //минуты

          //настройка режима будильника
          case 4: if (cur_day < 7) cur_day++; else cur_day = 1; break; //день недели
          case 5: alarm[3] |= (0x01 << cur_day); break; //установка
          case 6: if (alarm[4] < ((sizeof(alarm_sound) >> 2) - 1)) alarm[4]++; else alarm[4] = 0; MELODY_RESET; break; //мелодия
        }
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;
      case RIGHT_KEY_HOLD: //удержание правой кнопки
        switch (cur_mode) {
          case 0:
            newAlarm(); //создать новый будильник
            dotSetBright(dotMaxBright); //включаем точки
            for (_timer_ms[TMR_MS] = 500; _timer_ms[TMR_MS];) dataUpdate(); //обработка данных
            dotSetBright(0); //выключаем точки
            curAlarm = alarms_num;
            alarmReadBlock(curAlarm, alarm); //читаем блок данных
            break;
          case 4:
          case 5: cur_mode = 6; break;
          default:
            cur_mode = (alarm[2] < 4) ? 6 : 4; //режим настройки функций
            dotSetBright(dotMaxBright); //включаем точки
            break;

        }
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;
      case SET_KEY_PRESS: //клик средней кнопкой
        switch (cur_mode) {
          case 0: if (curAlarm) cur_mode = 1; break; //если есть будильники в памяти, перейти к настройке режима
          case 1: cur_mode = 0; alarmWriteBlock(curAlarm, alarm); break; //записать блок основных данных будильника
          case 2: cur_mode = 3; break; //перейти к настройке минут
          case 3: cur_mode = 2; break; //перейти к настройке часов
          case 4: cur_mode = 5; break; //перейти к активации дня недели
          case 5:
          case 6: cur_mode = (alarm[2] < 4) ? 6 : 4; break; //перейти к выбору дня недели
            break;
        }
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;
      case SET_KEY_HOLD: //удержание средней кнопки
        switch (cur_mode) {
          case 0: checkAlarms(); return; //выход
          case 1: alarmWriteBlock(curAlarm, alarm); checkAlarms(); return; //записать блок основных данных будильника и выйти
          default:
            MELODY_RESET; //сброс воспроизведения мелодии
            dotSetBright(0); //выключаем точки
            cur_mode = 1; //выбор будильника
            blink_data = 0; //сбрасываем флаги
            _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
            break;
        }
        break;
    }
  }
}
//-----------------------------Настроки основные------------------------------------
void settings_main(void) //настроки основные
{
  boolean set = 0; //режим настройки
  boolean cur_indi = 0; //текущий индикатор
  boolean blink_data = 0; //мигание сигментами
  int8_t aging = 0; //буфер регистра старения
  uint8_t cur_mode = 0; //текущий режим
  uint8_t time_out = 0; //таймаут автовыхода

  indiClr(); //очищаем индикаторы
  dotSetBright(0); //выключаем точки

  //настройки
  while (1) {
    dataUpdate(); //обработка данных

    if (!_sec) {
      _sec = 1;
      if (++time_out >= SETTINGS_TIMEOUT) return;
    }

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

      indiClr(); //очистка индикаторов
      switch (set) {
        case 0:
          indiPrintNum(cur_mode + 1, (LAMP_NUM / 2 - 1), 2, 0); //вывод режима
          break;
        case 1:
          indiPrintNum(cur_mode + 1, 5); //режим
          switch (cur_mode) {
            case SET_TIME_FORMAT: if (!blink_data) indiPrintNum((mainSettings.timeFormat) ? 12 : 24, 2); break;
            case SET_GLITCH: if (!blink_data) indiPrintNum(mainSettings.glitchMode, 3); break;
            case SET_BTN_SOUND: if (!blink_data) indiPrintNum(mainSettings.knock_sound, 3); break;
            case SET_HOUR_TIME:
              if (!blink_data || cur_indi) indiPrintNum(mainSettings.timeHour[0], 0, 2, 0); //вывод часов
              if (!blink_data || !cur_indi) indiPrintNum(mainSettings.timeHour[1], 2, 2, 0); //вывод часов
              break;
            case SET_BRIGHT_TIME:
              if (!blink_data || cur_indi) indiPrintNum(mainSettings.timeBright[0], 0, 2, 0); //вывод часов
              if (!blink_data || !cur_indi) indiPrintNum(mainSettings.timeBright[1], 2, 2, 0); //вывод часов
              break;
            case SET_INDI_BRIGHT:
              if (!blink_data || cur_indi) indiPrintNum(mainSettings.indiBright[0], 0, 2, 0); //вывод часов
              if (!blink_data || !cur_indi) indiPrintNum(mainSettings.indiBright[1], 2, 2, 0); //вывод часов
              break;
            case SET_BACKL_BRIGHT:
              if (!blink_data || cur_indi) indiPrintNum(mainSettings.backlBright[0] / 10, 0, 2, 0); //вывод часов
              if (!blink_data || !cur_indi) indiPrintNum(mainSettings.backlBright[1] / 10, 2, 2, 0); //вывод часов
              break;
            case SET_TEMP_SENS:
              if (!blink_data || cur_indi) indiPrintNum(sens.temp / 10 + mainSettings.tempCorrect, 0, 3); //вывод часов
              if (!blink_data || !cur_indi) indiPrintNum(mainSettings.sensorSet, 3); //вывод часов
              break;
            case SET_AUTO_TEMP:
              if (!blink_data) indiPrintNum(mainSettings.autoTempTime, 1, 3);
              break;
            case SET_TIME_CORRECT:
              if (!blink_data) {
                if (EIMSK) indiPrintNum((aging < 0) ? (uint8_t)((aging ^ 0xFF) + 1) : (uint8_t)aging, 0, (aging > 0) ? 4 : 0);
                else indiPrintNum(mainSettings.timePeriod, 0);
              }
              break;
          }
          blink_data = !blink_data; //мигание сигментами
          break;
      }
    }
    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (check_keys()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (set) {
          case 0: if (cur_mode > 0) cur_mode--; else cur_mode = SET_MAX_ITEMS - 1; break;
          case 1:
            switch (cur_mode) {
              case SET_TIME_FORMAT: mainSettings.timeFormat = 0; break; //формат времени
              case SET_GLITCH: mainSettings.glitchMode = 0; break; //глюки
              case SET_BTN_SOUND: mainSettings.knock_sound = 0; break; //звук кнопок
              case SET_HOUR_TIME: //время звука смены часа
                switch (cur_indi) {
                  case 0: if (mainSettings.timeHour[0] > 0) mainSettings.timeHour[0]--; else mainSettings.timeHour[0] = 23; break;
                  case 1: if (mainSettings.timeHour[1] > 0) mainSettings.timeHour[1]--; else mainSettings.timeHour[1] = 23; break;
                }
                break;
              case SET_BRIGHT_TIME: //время смены подсветки
                switch (cur_indi) {
                  case 0: if (mainSettings.timeBright[0] > 0) mainSettings.timeBright[0]--; else mainSettings.timeBright[0] = 23; break;
                  case 1: if (mainSettings.timeBright[1] > 0) mainSettings.timeBright[1]--; else mainSettings.timeBright[1] = 23; break;
                }
                break;
              case SET_INDI_BRIGHT: //яркость индикаторов
                switch (cur_indi) {
                  case 0: if (mainSettings.indiBright[0] > 0) mainSettings.indiBright[0]--; else mainSettings.indiBright[0] = 30; break;
                  case 1: if (mainSettings.indiBright[1] > 0) mainSettings.indiBright[1]--; else mainSettings.indiBright[1] = 30; break;
                }
                indiSetBright(mainSettings.indiBright[cur_indi]); //установка общей яркости индикаторов
                break;
              case SET_BACKL_BRIGHT: //яркость подсветки
                switch (cur_indi) {
                  case 0: if (mainSettings.backlBright[0] > 0) mainSettings.backlBright[0] -= 10; else mainSettings.backlBright[0] = 250; break;
                  case 1: if (mainSettings.backlBright[1] > 10) mainSettings.backlBright[1] -= 10; else mainSettings.backlBright[1] = 250; break;
                }
#if BACKL_WS2812B
                setLedBright(mainSettings.backlBright[cur_indi]); //устанавливаем максимальную яркость
                setLedHue(fastSettings.backlColor); //отправляем статичный цвет
#else
                OCR2A = mainSettings.backlBright[cur_indi]; //если посветка статичная, устанавливаем яркость
#endif
                break;
              case SET_TEMP_SENS: //настройка сенсора температуры
                switch (cur_indi) {
                  case 0: if (mainSettings.tempCorrect > -127) mainSettings.tempCorrect--; else mainSettings.tempCorrect = 127; break;
                  case 1:
                    if (mainSettings.sensorSet > 0) mainSettings.sensorSet--;
                    updateTemp(); //обновить показания температуры
                    break;
                }
                break;
              case SET_AUTO_TEMP: //автопоказ температуры
                if (mainSettings.autoTempTime > 5) mainSettings.autoTempTime -= 5; else mainSettings.autoTempTime = 0;
                break;
              case SET_TIME_CORRECT: //коррекция хода
                switch (EIMSK) {
                  case 0: if (mainSettings.timePeriod > US_PERIOD_MIN) mainSettings.timePeriod--; else mainSettings.timePeriod = US_PERIOD_MAX; break;
                  case 1: if (aging > -127) aging--; else aging = 127; break;
                }
                break;
            }
            break;
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (set) {
          case 0: if (cur_mode < (SET_MAX_ITEMS - 1)) cur_mode++; else cur_mode = 0; break;
          case 1:
            switch (cur_mode) {
              case SET_TIME_FORMAT: mainSettings.timeFormat = 1; break; //формат времени
              case SET_GLITCH: mainSettings.glitchMode = 1; break; //глюки
              case SET_BTN_SOUND: mainSettings.knock_sound = 1; break; //звук кнопок
              case SET_HOUR_TIME: //время звука смены часа
                switch (cur_indi) {
                  case 0: if (mainSettings.timeHour[0] < 23) mainSettings.timeHour[0]++; else mainSettings.timeHour[0] = 0; break;
                  case 1: if (mainSettings.timeHour[1] < 23) mainSettings.timeHour[1]++; else mainSettings.timeHour[1] = 0; break;
                }
                break;
              case SET_BRIGHT_TIME: //время смены подсветки
                switch (cur_indi) {
                  case 0: if (mainSettings.timeBright[0] < 23) mainSettings.timeBright[0]++; else mainSettings.timeBright[0] = 0; break;
                  case 1: if (mainSettings.timeBright[1] < 23) mainSettings.timeBright[1]++; else mainSettings.timeBright[1] = 0; break;
                }
                break;
              case SET_INDI_BRIGHT: //яркость индикаторов
                switch (cur_indi) {
                  case 0: if (mainSettings.indiBright[0] < 30) mainSettings.indiBright[0]++; else mainSettings.indiBright[0] = 0; break;
                  case 1: if (mainSettings.indiBright[1] < 30) mainSettings.indiBright[1]++; else mainSettings.indiBright[1] = 0; break;
                }
                indiSetBright(mainSettings.indiBright[cur_indi]); //установка общей яркости индикаторов
                break;
              case SET_BACKL_BRIGHT: //яркость подсветки
                switch (cur_indi) {
                  case 0: if (mainSettings.backlBright[0] < 250) mainSettings.backlBright[0] += 10; else mainSettings.backlBright[0] = 0; break;
                  case 1: if (mainSettings.backlBright[1] < 250) mainSettings.backlBright[1] += 10; else mainSettings.backlBright[1] = 10; break;
                }
#if BACKL_WS2812B
                setLedBright(mainSettings.backlBright[cur_indi]); //устанавливаем максимальную яркость
                setLedHue(fastSettings.backlColor); //отправляем статичный цвет
#else
                OCR2A = mainSettings.backlBright[cur_indi]; //если посветка статичная, устанавливаем яркость
#endif
                break;
              case SET_TEMP_SENS: //настройка сенсора температуры
                switch (cur_indi) {
                  case 0: if (mainSettings.tempCorrect < 127) mainSettings.tempCorrect++; else mainSettings.tempCorrect = -127; break;
                  case 1:
#if !SENS_PORT_DISABLE
                    if (mainSettings.sensorSet < 4) mainSettings.sensorSet++;
#else
                    if (mainSettings.sensorSet < 1) mainSettings.sensorSet++;
#endif
                    updateTemp(); //обновить показания температуры
                    break;
                }
                break;
              case SET_AUTO_TEMP: //автопоказ температуры
                if (mainSettings.autoTempTime < 240) mainSettings.autoTempTime += 5; else mainSettings.autoTempTime = 240;
                break;
              case SET_TIME_CORRECT: //коррекция хода
                switch (EIMSK) {
                  case 0: if (mainSettings.timePeriod < US_PERIOD_MAX) mainSettings.timePeriod++; else mainSettings.timePeriod = US_PERIOD_MIN; break;
                  case 1: if (aging < 127) aging++; else aging = -127; break;
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
          switch (cur_mode) {
            case SET_INDI_BRIGHT: indiSetBright(mainSettings.indiBright[0]); break; //установка общей яркости индикаторов
            case SET_BACKL_BRIGHT: //яркость подсветки
#if BACKL_WS2812B
              setLedBright(mainSettings.backlBright[0]); //устанавливаем максимальную яркость
              setLedHue(fastSettings.backlColor); //отправляем статичный цвет
#else
              OCR2A = mainSettings.backlBright[0]; //если посветка статичная, устанавливаем яркость
#endif
              break;
          }
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_HOLD: //удержание правой кнопки
        if (set) {
          cur_indi = 1;
          switch (cur_mode) {
            case SET_INDI_BRIGHT: indiSetBright(mainSettings.indiBright[1]); break; //установка общей яркости индикаторов
            case SET_BACKL_BRIGHT: //яркость подсветки
#if BACKL_WS2812B
              setLedBright(mainSettings.backlBright[1]); //устанавливаем максимальную яркость
              setLedHue(fastSettings.backlColor); //отправляем статичный цвет
#else
              OCR2A = mainSettings.backlBright[1]; //если посветка статичная, устанавливаем яркость
#endif
              break;
          }
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        set = !set;
        if (set) {
          switch (cur_mode) {
            case SET_INDI_BRIGHT: indiSetBright(mainSettings.indiBright[0]); break; //установка общей яркости индикаторов
            case SET_BACKL_BRIGHT: //яркость подсветки
#if BACKL_WS2812B
              setLedBright(mainSettings.backlBright[0]); //устанавливаем максимальную яркость
              setLedHue(fastSettings.backlColor); //отправляем статичный цвет
#else
              OCR2A = mainSettings.backlBright[0]; //если посветка статичная, устанавливаем яркость
#endif
              fastSettings.backlMode |= 0x80; //запретили эффекты подсветки
              break;
            case SET_TEMP_SENS:
              updateTemp(); //обновить показания температуры
              break;
            case SET_TIME_CORRECT:
              if (EIMSK) aging = readAgingRTC(); //чтение коррекции хода
              break;
          }
          dotSetBright(dotMaxBright); //включаем точки
        }
        else {
          changeBright(); //установка яркости от времени суток
          dotSetBright(0); //выключаем точки
          if (EIMSK && cur_mode == SET_TIME_CORRECT) WriteAgingRTC((uint8_t)aging); //запись коррекции хода
        }
        cur_indi = 0;
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        updateData((uint8_t*)&mainSettings, sizeof(mainSettings), EEPROM_BLOCK_SETTINGS_MAIN, EEPROM_BLOCK_CRC_MAIN); //записываем основные настройки в память
        changeBright(); //установка яркости от времени суток
        _tmrTemp = 0; //сбрасываем таймер показа температуры
        return;
    }
  }
}
//---------------------Установка яркости от времени суток-----------------------------
void changeBright(void) //установка яркости от времени суток
{
  if ((mainSettings.timeBright[0] > mainSettings.timeBright[1] && (RTC.h >= mainSettings.timeBright[0] || RTC.h < mainSettings.timeBright[1])) ||
      (mainSettings.timeBright[0] < mainSettings.timeBright[1] && RTC.h >= mainSettings.timeBright[0] && RTC.h < mainSettings.timeBright[1])) {
    //ночной режим
    dotMaxBright = DOT_BRIGHT_N; //установка максимальной яркости точек
    backlMaxBright = mainSettings.backlBright[0]; //установка максимальной яркости подсветки
    indiMaxBright = mainSettings.indiBright[0]; //установка максимальной яркости индикаторов
  }
  else {
    //дневной режим
    dotMaxBright = DOT_BRIGHT; //установка максимальной яркости точек
    backlMaxBright = mainSettings.backlBright[1]; //установка максимальной яркости подсветки
    indiMaxBright = mainSettings.indiBright[1]; //установка максимальной яркости индикаторов
  }
  switch (fastSettings.dotMode) {
    case 0: dotSetBright(0); break; //если точки выключены
    case 1: dotSetBright(dotMaxBright); break; //если точки статичные, устанавливаем яркость
    case 2:
      dotBrightStep = ceil((float)(dotMaxBright * 2.00) / DOT_TIME * DOT_TIMER); //расчёт шага яркости точки
      if (!dotBrightStep) dotBrightStep = 1; //если шаг слишком мал, устанавливаем минимум
      dotBrightTime = ceil(DOT_TIME / (float)(dotMaxBright * 2.00)); //расчёт шага яркости точки
      if (dotBrightTime < DOT_TIMER) dotBrightTime = DOT_TIMER; //если шаг слишком мал, устанавливаем минимум
      break;
  }
  fastSettings.backlMode &= 0x7F; //разрешили эффекты подсветки
#if BACKL_WS2812B
  if (backlMaxBright) {
    switch (fastSettings.backlMode) {
      case BACKL_OFF: clrLeds(); break; //выключили светодиоды
      case BACKL_STATIC:
        setLedBright(backlMaxBright); //устанавливаем максимальную яркость
        setLedHue(fastSettings.backlColor); //отправляем статичный цвет
        break;
      case BACKL_SMOOTH_COLOR_CHANGE:
      case BACKL_RAINBOW:
      case BACKL_CONFETTI:
        setLedBright(backlMaxBright); //устанавливаем максимальную яркость
        break;
    }
  }
  else clrLeds(); //выключили светодиоды
#else
  switch (fastSettings.backlMode) {
    case BACKL_OFF: backlSetBright(0); break; //если посветка выключена
    case BACKL_STATIC: backlSetBright(backlMaxBright); break; //если посветка статичная, устанавливаем яркость
    case BACKL_PULS: if (!backlMaxBright) backlSetBright(0); break; //иначе посветка выключена
  }
#endif
  if (backlMaxBright) {
    backlPulsBrightTime = (float)BACKL_MODE_2_STEP / (backlMaxBright > BACKL_MIN_BRIGHT) ? (backlMaxBright - BACKL_MIN_BRIGHT) : backlMaxBright / 2 * BACKL_MODE_2_TIME; //расчёт шага подсветки дыхания
    backlWaveBrightTime = (float)BACKL_MODE_6_STEP / (backlMaxBright > BACKL_MIN_BRIGHT) ? (backlMaxBright - BACKL_MIN_BRIGHT) : backlMaxBright / (2 * LAMP_NUM) * BACKL_MODE_6_TIME; //расчёт шага подсветки волна
  }
  indiSetBright(indiMaxBright); //установка общей яркости индикаторов
}
//----------------------------------Анимация подсветки---------------------------------
void backlEffect(void) //анимация подсветки
{
  static boolean backl_drv; //направление огня
  static uint8_t backl_pos; //положение огня
  static uint8_t backl_steps; //шаги затухания
  static uint8_t color_steps; //номер цвета

  if (backlMaxBright && !_timer_ms[TMR_BACKL]) { //если время пришло
    switch (fastSettings.backlMode) {
      case BACKL_OFF: //подсветка выключена
      case BACKL_STATIC: //статичный режим
        return; //выходим
      case BACKL_PULS:
      case BACKL_PULS_COLOR: { //дыхание подсветки
          _timer_ms[TMR_BACKL] = backlPulsBrightTime; //установили таймер
          if (backl_drv) { //если светодиоды в режиме разгорания
            if (incLedBright(BACKL_MODE_2_STEP, backlMaxBright)) backl_drv = 0; //прибавили шаг яркости
          }
          else { //иначе светодиоды в режиме затухания
            if (decLedBright(BACKL_MODE_2_STEP, (backlMaxBright > BACKL_MIN_BRIGHT) ? BACKL_MIN_BRIGHT : 0)) { //уменьшаем яркость
              backl_drv = 1;
              if (fastSettings.backlMode == BACKL_PULS_COLOR) color_steps += BACKL_MODE_3_COLOR; //плавно меняем цвет
              else color_steps = fastSettings.backlColor; //иначе статичный цвет
              _timer_ms[TMR_BACKL] = BACKL_MODE_2_PAUSE; //установили таймер
              return; //выходим
            }
          }
          setLedHue(color_steps); //отправили цвет
        }
        break;
      case BACKL_RUNNING_FIRE:
      case BACKL_RUNNING_FIRE_COLOR: { //бегущий огонь
          _timer_ms[TMR_BACKL] = BACKL_MODE_4_TIME; //установили таймер
          if (backl_steps) { //если есть шаги затухания
            decLedsBright(backl_pos - 1, BACKL_MODE_4_STEP); //уменьшаем яркость
            if (fastSettings.backlMode == BACKL_RUNNING_FIRE_COLOR) color_steps += BACKL_MODE_5_COLOR; //плавно меняем цвет
            else color_steps = fastSettings.backlColor; //иначе статичный цвет
            backl_steps--; //уменьшаем шаги затухания
          }
          else { //иначе двигаем голову
            if (backl_drv) { //если направление вправо
              if (backl_pos < LAMP_NUM + 1) backl_pos++; else backl_drv = 0; //едем вправо
            }
            else { //иначе напрвление влево
              if (backl_pos > 0) backl_pos--; else backl_drv = 1; //едем влево
            }
            setLedBright(backl_pos - 1, backlMaxBright); //установили яркость
            backl_steps = BACKL_MODE_4_FADING; //установили шаги затухания
          }
          setLedHue(color_steps); //отправили цвет
        }
        break;
      case BACKL_WAVE:
      case BACKL_WAVE_COLOR: { //волна
          _timer_ms[TMR_BACKL] = backlWaveBrightTime; //установили таймер
          if (backl_drv) {
            if (incLedBright(backl_pos, BACKL_MODE_6_STEP, backlMaxBright)) { //прибавили шаг яркости
              if (backl_pos < (LAMP_NUM - 1)) backl_pos++; //сменили позицию
              else {
                backl_pos = 0; //сбросили позицию
                backl_drv = 0; //перешли в затухание
              }
            }
          }
          else {
            if (decLedBright(backl_pos, BACKL_MODE_6_STEP, (backlMaxBright > BACKL_MIN_BRIGHT) ? BACKL_MIN_BRIGHT : 0)) { //иначе убавляем яркость
              if (backl_pos < (LAMP_NUM - 1)) backl_pos++; //сменили позицию
              else {
                backl_pos = 0; //сбросили позицию
                backl_drv = 1; //перешли в разгорание
              }
            }
          }
          if (fastSettings.backlMode == BACKL_WAVE_COLOR) color_steps += BACKL_MODE_7_COLOR; //плавно меняем цвет
          else color_steps = fastSettings.backlColor; //иначе статичный цвет
          setLedHue(color_steps); //отправили цвет
        }
        break;
      case BACKL_SMOOTH_COLOR_CHANGE: { //плавная смена цвета
          setLedHue(color_steps++); //установили цвет
          _timer_ms[TMR_BACKL] = BACKL_MODE_8_TIME; //установили таймер
        }
        break;
      case BACKL_RAINBOW: { //радуга
          color_steps += BACKL_MODE_9_STEP; //прибавили шаг
          for (uint8_t f = 0; f < LAMP_NUM; f++) setLedHue(f, color_steps + (f * BACKL_MODE_9_STEP)); //установили цвет
          _timer_ms[TMR_BACKL] = BACKL_MODE_9_TIME; //установили таймер
        }
        break;
      case BACKL_CONFETTI: { //рандомный цвет
          setLedHue(random(0, LAMP_NUM), random(0, 256)); //установили цвет
          _timer_ms[TMR_BACKL] = BACKL_MODE_10_TIME; //установили таймер
        }
        break;
    }
  }
}
//----------------------------------Мигание подсветки---------------------------------
void backlFlash(void) //мигание подсветки
{
  static boolean backl_drv; //направление яркости

  if (fastSettings.backlMode == BACKL_PULS && backlMaxBright) {
    if (!_timer_ms[TMR_BACKL]) {
      _timer_ms[TMR_BACKL] = backlPulsBrightTime;
      switch (backl_drv) {
        case 0: if (backlIncBright(BACKL_MODE_2_STEP, backlMaxBright)) backl_drv = 1; break;
        case 1:
          if (backlDecBright(BACKL_MODE_2_STEP, (backlMaxBright > BACKL_MIN_BRIGHT) ? BACKL_MIN_BRIGHT : 0)) {
            _timer_ms[TMR_BACKL] = BACKL_MODE_2_PAUSE;
            backl_drv = 0;
          }
          break;
      }
    }
  }
}
//--------------------------------Режим мигания точек------------------------------------
void dotFlashMode(uint8_t mode) //режим мигания точек
{
  static boolean dot_drv; //направление яркости
  static uint8_t dot_cnt; //счетчик мигания точки

  if (!_dot && !_timer_ms[TMR_DOT]) {
    switch (mode) {
      case DOT_OFF: if (OCR1B) dotSetBright(0); break; //если точки включены, выключаем их
      case DOT_STATIC: if (OCR1B != dotMaxBright) dotSetBright(dotMaxBright); break; //если яркость не совпадает, устанавливаем яркость
      case DOT_PULS:
        _timer_ms[TMR_DOT] = dotBrightTime;
        switch (dot_drv) {
          case 0: if (dotIncBright(dotBrightStep, dotMaxBright)) dot_drv = 1; break;
          case 1:
            if (dotDecBright(dotBrightStep, 0)) {
              dot_drv = 0;
              _dot = 1;
            }
            break;
        }
        break;
      case DOT_BLINK:
        switch (dot_drv) {
          case 0: dotSetBright(dotMaxBright); dot_drv = 1; _timer_ms[TMR_DOT] = 500; break; //включаем точки
          case 1: dotSetBright(0); dot_drv = 0; _dot = 1; break; //выключаем точки
        }
        break;
      case DOT_DOOBLE_BLINK:
        _timer_ms[TMR_DOT] = 150;
        switch (dot_cnt % 2) {
          case 0: dotSetBright(dotMaxBright); break; //включаем точки
          case 1: dotSetBright(0); break; //выключаем точки
        }
        if (++dot_cnt > 3) {
          dot_cnt = 0;
          _dot = 1;
        }
        break;
    }
  }
}
//--------------------------------Мигание точек------------------------------------
void dotFlash(void) //мигание точек
{
  if (alarmWaint) dotFlashMode((ALM_WAINT_BLINK_DOT != DOT_PULS) ? ALM_WAINT_BLINK_DOT : fastSettings.dotMode); //мигание точек в режиме ожидания будильника
  else if (alarmEnabled) dotFlashMode((ALM_ON_BLINK_DOT != DOT_PULS) ? ALM_ON_BLINK_DOT : fastSettings.dotMode); //мигание точек при включенном будильнике
  else dotFlashMode(fastSettings.dotMode); //мигание точек по умолчанию
}
//--------------------------------Обновить показания температуры----------------------------------------
void updateTemp(void) //обновить показания температуры
{
  switch (mainSettings.sensorSet) { //выбор датчика температуры
    default: readTempRTC(); break; //чтение температуры с датчика DS3231
    case 1: readTempBME(); break; //чтение температуры/давления/влажности с датчика BME
#if !SENS_PORT_DISABLE
    case 2: readTempDHT11(); break; //чтение температуры/влажности с датчика DHT11
    case 3: readTempDHT22(); break; //чтение температуры/влажности с датчика DHT22
    case 4: readTempDS(); break; //чтение температуры с датчика DS18B20
#endif
  }
}
//--------------------------------Автоматический показ температуры----------------------------------------
void autoShowTemp(void) //автоматический показ температуры
{
  if (mainSettings.autoTempTime && _tmrTemp++ >= mainSettings.autoTempTime && RTC.s > 7 && RTC.s < 55) {
    _tmrTemp = 0; //сбрасываем таймер

    uint8_t pos = LAMP_NUM; //текущее положение анимации
    boolean drv = 0; //направление анимации

    updateTemp(); //обновить показания температуры
    dotSetBright(dotMaxBright); //включаем точки

    for (uint8_t mode = 0; mode < 3; mode++) {
      switch (mode) {
        case 1:
          if (!sens.hum) {
            if (!sens.press) return; //выходим
            else mode = 2; //отображаем давление
          }
          dotSetBright(0); //выключаем точки
          break;
        case 2: if (!sens.press) return; break; //выходим
      }

      while (1) { //анимация перехода
        dataUpdate(); //обработка данных
        if (check_keys()) return; //возврат если нажата кнопка
        if (!_timer_ms[TMR_ANIM]) { //если таймер истек
          _timer_ms[TMR_ANIM] = AUTO_TEMP_ANIM_TIME; //устанавливаем таймер

          indiClr(); //очистка индикаторов
          switch (mode) {
            case 0: indiPrintNum(sens.temp / 10 + mainSettings.tempCorrect, pos, 3, ' '); break; //вывод температуры
            case 1: indiPrintNum(sens.hum, pos, 4, ' '); break; //вывод влажности
            case 2: indiPrintNum(sens.press, pos, 4, ' '); break; //вывод давления
          }
          if (!drv) {
            if (pos > 0) pos--;
            else {
              drv = 1;
              _timer_ms[TMR_ANIM] = AUTO_TEMP_PAUSE_TIME; //устанавливаем таймер
            }
          }
          else {
            if (pos < LAMP_NUM) pos++;
            else {
              drv = 0;
              break;
            }
          }
        }
      }
    }
  }
}
//--------------------------------Показать температуру----------------------------------------
void showTemp(void) //показать температуру
{
  uint8_t mode = 0; //текущий режим

  _sec = 0; //обновление экрана

  updateTemp(); //обновить показания температуры
  dotSetBright(dotMaxBright); //включаем точки

  for (_timer_ms[TMR_MS] = SHOW_TIME; _timer_ms[TMR_MS];) {
    dataUpdate(); //обработка данных

    if (!_sec) {
      _sec = 1; //сбрасываем флаг
      indiClr(); //очистка индикаторов
      indiPrintNum(mode + 1, 5); //режим
      switch (mode) {
        case 0: indiPrintNum(sens.temp / 10 + mainSettings.tempCorrect, 0, 3, ' '); break;
        case 1: indiPrintNum(sens.hum, 0, 4, ' '); break;
        case 2: indiPrintNum(sens.press, 0, 4, ' '); break;
      }
    }

    switch (check_keys()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        if (++mode > 2) mode = 0;
        switch (mode) {
          case 1:
            if (!sens.hum) {
              if (!sens.press) mode = 0;
              else {
                mode = 2;
                dotSetBright(0); //выключаем точки
              }
            }
            else dotSetBright(0); break; //выключаем точки
          case 2: if (!sens.press) mode = 0; break;
        }
        if (!mode) dotSetBright(dotMaxBright); //включаем точки
        _timer_ms[TMR_MS] = SHOW_TIME;
        _sec = 0; //обновление экрана
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

  _sec = 0; //обновление экрана
  dotSetBright(dotMaxBright); //включаем точки

  for (_timer_ms[TMR_MS] = SHOW_TIME; _timer_ms[TMR_MS];) {
    dataUpdate(); //обработка данных

    if (!_sec) {
      _sec = 1; //сбрасываем флаг
      indiClr(); //очистка индикаторов
      indiPrintNum(mode + 1, 5); //режим
      switch (mode) {
        case 0:
          indiPrintNum(RTC.DD, 0, 2, 0); //вывод даты
          indiPrintNum(RTC.MM, 2, 2, 0); //вывод месяца
          break;
        case 1: indiPrintNum(RTC.YY, 0); break; //вывод года
      }
    }

    switch (check_keys()) {
      case RIGHT_KEY_PRESS: //клик правой кнопкой
        if (++mode > 1) mode = 0;
        switch (mode) {
          case 0: dotSetBright(dotMaxBright); break; //включаем точки
          case 1: dotSetBright(0); break; //выключаем точки
        }
        _timer_ms[TMR_MS] = SHOW_TIME;
        _sec = 0; //обновление экрана
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

  dotSetBright(0); //выключаем точки

  for (_timer_ms[TMR_MS] = SWITCH_TIME; _timer_ms[TMR_MS];) {
    dataUpdate(); //обработка данных

    if (anim < 4) {
      if (!_timer_ms[TMR_ANIM]) { //если таймер истек
        _timer_ms[TMR_ANIM] = ANIM_TIME; //устанавливаем таймер

        indiClr(); //очистка индикаторов
        indiPrintNum(mode + 1, 5); //режим
        switch (mode) {
          case FAST_BACKL_MODE: indiPrintNum(fastSettings.backlMode, anim - 1, 2); break; //вывод режима подсветки
          case FAST_FLIP_MODE: indiPrintNum(fastSettings.flipMode, anim); break; //вывод режима анимации
          case FAST_DOT_MODE: indiPrintNum(fastSettings.dotMode, anim); break; //вывод режима точек
          case FAST_BACKL_COLOR: indiPrintNum(fastSettings.backlColor / 10, anim - 1, 2); break; //вывод цвета подсветки
        }
        anim++; //сдвигаем анимацию
      }
    }

    switch (check_keys()) {
      case SET_KEY_PRESS: //клик средней кнопкой
        if (mode != FAST_BACKL_MODE) mode = FAST_BACKL_MODE; //демострация текущего режима работы
        else {
#if BACKL_WS2812B
          if (++fastSettings.backlMode < BACKL_EFFECT_NUM) {
            switch (fastSettings.backlMode) {
              case BACKL_STATIC:
                setLedBright(backlMaxBright); //устанавливаем максимальную яркость
                setLedHue(fastSettings.backlColor); //отправляем статичный цвет
                break;
              case BACKL_PULS:
                setLedBright(BACKL_MIN_BRIGHT); //устанавливаем минимальную яркость
                setLedHue(fastSettings.backlColor); //отправляем статичный цвет
                break;
              case BACKL_RUNNING_FIRE:
                setLedBright(0); //устанавливаем минимальную яркость
                setLedHue(fastSettings.backlColor); //отправляем статичный цвет
                break;
              case BACKL_WAVE:
                setLedBright(BACKL_MIN_BRIGHT); //устанавливаем минимальную яркость
                setLedHue(fastSettings.backlColor); //отправляем статичный цвет
                break;
              case BACKL_SMOOTH_COLOR_CHANGE:
                setLedBright(backlMaxBright); //устанавливаем максимальную яркость
                break;
            }
          }
          else {
            clrLeds(); //выключили светодиоды
            fastSettings.backlMode = 0; //переключили режим подсветки
          }
#else
          if (++fastSettings.backlMode > 2) fastSettings.backlMode = 0; //переключили режим подсветки
          switch (fastSettings.backlMode) {
            case BACKL_OFF: backlSetBright(0); break; //выключаем подсветку
            case BACKL_STATIC: backlSetBright(backlMaxBright); break; //включаем подсветку
            case BACKL_PULS: backlSetBright(backlMaxBright ? BACKL_MIN_BRIGHT : 0); break; //выключаем подсветку
          }
#endif
        }
        _timer_ms[TMR_MS] = SWITCH_TIME;
        anim = 0;
        break;
#if BACKL_WS2812B
      case SET_KEY_HOLD: //удержание средней кнопки
        if (!mode) {
          switch (fastSettings.backlMode) {
            case BACKL_STATIC:
            case BACKL_PULS:
            case BACKL_RUNNING_FIRE:
            case BACKL_WAVE:
              mode = FAST_BACKL_COLOR;
              break;
          }
        }
        _timer_ms[TMR_MS] = SWITCH_TIME;
        anim = 0;
        break;
#endif
      case RIGHT_KEY_PRESS: //клик правой кнопкой
        if (mode == FAST_BACKL_COLOR) {
          if (fastSettings.backlColor < 250) fastSettings.backlColor += 10; else fastSettings.backlColor = 0;
          setLedHue(fastSettings.backlColor); //отправляем статичный цвет
        }
        else if (mode != FAST_FLIP_MODE) mode = FAST_FLIP_MODE; //демострация текущего режима работы
        else if (++fastSettings.flipMode > (FLIP_EFFECT_NUM + 1)) fastSettings.flipMode = 0;
        _timer_ms[TMR_MS] = SWITCH_TIME;
        anim = 0;
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
        if (mode == FAST_BACKL_COLOR) {
          if (fastSettings.backlColor > 0) fastSettings.backlColor -= 10; else fastSettings.backlColor = 250;
          setLedHue(fastSettings.backlColor); //отправляем статичный цвет
        }
        else if (mode != FAST_DOT_MODE) mode = FAST_DOT_MODE; //демострация текущего режима работы
        else if (++fastSettings.dotMode > DOT_PULS) fastSettings.dotMode = 0;
        _timer_ms[TMR_MS] = SWITCH_TIME;
        anim = 0;
        break;
    }
  }
  if (mode == 1) flipIndi(fastSettings.flipMode, 1); //демонстрация анимации цифр
  updateData((uint8_t*)&fastSettings, sizeof(fastSettings), EEPROM_BLOCK_SETTINGS_FAST, EEPROM_BLOCK_CRC_FAST); //записываем настройки яркости в память
}
//--------------------------------Тревога таймера----------------------------------------
void timerWarn(void) //тревога таймера
{
  boolean blink_data = 0; //флаг мигания индикаторами
  if (timerMode == 2 && !timerCnt) {
    while (!check_keys()) { //ждем
      dataUpdate(); //обработка данных
      MELODY_PLAY(0); //воспроизводим мелодию
      if (!_timer_ms[TMR_ANIM]) {
        _timer_ms[TMR_ANIM] = TIMER_BLINK_TIME;
        switch (blink_data) {
          case 0: indiClr(); dotSetBright(0); break; //очищаем индикаторы и выключаем точки
          case 1:
            indiPrintNum((timerTime < 3600) ? ((timerTime / 60) % 60) : (timerTime / 3600), 0, 2, 0); //вывод минут/часов
            indiPrintNum((timerTime < 3600) ? (timerTime % 60) : ((timerTime / 60) % 60), 2, 2, 0); //вывод секунд/минут
            indiPrintNum((timerTime < 3600) ? 0 : (timerTime % 60), 4, 2, 0); //вывод секунд
            dotSetBright(dotMaxBright); //включаем точки
            break;
        }
        blink_data = !blink_data; //мигаем временем
      }
    }
    timerMode = 0; //деактивируем таймер
    timerCnt = timerTime; //сбрасываем таймер
  }
}
//----------------------------Настройки таймера----------------------------------
void timerSettings(void) //настройки таймера
{
  boolean mode = 0; //текущий режим
  boolean blink_data = 0; //флаг мигания индикаторами

  dotSetBright(0); //выключаем точки
  while (1) {
    dataUpdate(); //обработка данных

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

      indiClr(); //очистка индикаторов
      indiPrintNum(2, 5); //вывод режима

      if (!blink_data || mode) indiPrintNum(timerTime / 60, 0, 2, 0); //вывод минут
      if (!blink_data || !mode) indiPrintNum(timerTime % 60, 2, 2, 0); //вывод секунд
      blink_data = !blink_data;
    }

    switch (check_keys()) {
      case SET_KEY_PRESS: //клик средней кнопкой
        mode = !mode; //переключаем режим
        _timer_ms[TMR_MS] = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (mode) {
          case 0: if (timerTime / 60 < 99) timerTime += 60; else timerTime -= 5940; break; //сбрасываем секундомер
          case 1: if (timerTime % 60 < 59) timerTime++; else timerTime -= 59; break; //сбрасываем таймер
        }
        _timer_ms[TMR_MS] = blink_data = 0; //сбрасываем флаги
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (mode) {
          case 0: if (timerTime / 60 > 0) timerTime -= 60; else timerTime += 5940; break; //сбрасываем секундомер
          case 1: if (timerTime % 60 > 0) timerTime--; else timerTime += 59; break; //сбрасываем таймер
        }
        _timer_ms[TMR_MS] = blink_data = 0; //сбрасываем флаги
        break;

      case ADD_KEY_HOLD: //удержание дополнительной кнопки
      case SET_KEY_HOLD: //удержание средней кнопки
        if (!timerTime) timerTime = TIMER_TIME; //устанавливаем значение по умолчанию
        timerCnt = timerTime; //сбрасываем таймер
        return; //выходим
    }
  }
}
//--------------------------------Таймер-секундомер----------------------------------------
void timerStopwatch(void) //таймер-секундомер
{
  uint8_t mode = 0; //текущий режим
  uint8_t time_out = 0; //таймаут автовыхода
  uint16_t msTmr = 10000UL / mainSettings.timePeriod * 2; //расчет хода миллисекунд
  static uint16_t millisCnt; //счетчик миллисекун

  if (timerMode & 0x7F) mode = (timerMode & 0x7F) - 1; //если таймер был запущен
  else timerCnt = 0; //иначе сбрасываем таймер

  _sec = 0; //обновление экрана

  while (1) {
    dataUpdate(); //обработка данных
    timerWarn(); //тревога таймера
#if LAMP_NUM > 4
    dotFlashMode((!timerCnt) ? 0 : ((timerMode > 2 || !timerMode ) ? 1 : 3)); //мигание точек по умолчанию
#else
    dotFlashMode((!timerMode) ? 0 : ((timerMode > 2) ? 1 : 3)); //мигание точек по умолчанию
#endif

    if (!_sec) {
      _sec = 1; //сбрасываем флаг
      if (!timerMode && ++time_out >= SETTINGS_TIMEOUT) return;

      indiClr(); //очистка индикаторов
      switch (timerMode) {
        case 0: indiPrintNum(mode + 1, 5); break; //вывод режима
        default:
          if (!(timerMode & 0x80)) millisCnt = 0; //сбрасываем счетчик миллисекунд
          indiPrintNum((timerCnt < 3600) ? ((mode) ? (100 - millisCnt / 10) : (millisCnt / 10)) : (timerCnt % 60), 4, 2, 0); //вывод милиекунд/секунд
          break;
      }

      indiPrintNum((timerCnt < 3600) ? ((timerCnt / 60) % 60) : (timerCnt / 3600), 0, 2, 0); //вывод минут/часов
      indiPrintNum((timerCnt < 3600) ? (timerCnt % 60) : ((timerCnt / 60) % 60), 2, 2, 0); //вывод секунд/минут
    }

    switch (timerMode) {
      case 1: case 2:
        if (!_timer_ms[TMR_MS]) {
          _timer_ms[TMR_MS] = 10;
          if (timerCnt < 3600) {
            millisCnt += msTmr;
            indiPrintNum((mode) ? (100 - millisCnt / 10) : (millisCnt / 10), 4, 2, 0); //вывод милиекунд
          }
        }
        break;
    }

    switch (check_keys()) {
      case SET_KEY_PRESS: //клик средней кнопкой
        if (mode && !timerMode) {
          timerSettings(); //настройки таймера
          time_out = 0; //сбрасываем таймер автовыхода
          _sec = 0; //обновление экрана
        }
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        if (timerMode == 1) timerMode |= 0x80; //приостановка секундомера
        return; //выходим

      case RIGHT_KEY_PRESS: //клик правой кнопкой
      case RIGHT_KEY_HOLD: //удержание правой кнопки
        mode = 1; //переключаем режим
        timerMode = 0; //деактивируем таймер
        timerCnt = timerTime; //сбрасываем таймер
        time_out = 0; //сбрасываем таймер автовыхода
        _sec = 0; //обновление экрана
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
      case LEFT_KEY_HOLD: //удержание левой кнопки
        mode = 0; //переключаем режим
        timerMode = 0; //деактивируем таймер
        timerCnt = 0; //сбрасываем секундомер
        time_out = 0; //сбрасываем таймер автовыхода
        _sec = 0; //обновление экрана
        break;

      case ADD_KEY_PRESS: //клик дополнительной кнопкой
        if (!timerMode) {
          millisCnt = 0; //сбрасываем счетчик миллисекунд
          timerMode = mode + 1;
        }
        else timerMode ^= 0x80; //приостановка таймера/секундомера
        time_out = 0; //сбрасываем таймер автовыхода
        _sec = 0; //обновление экрана
        break;

      case ADD_KEY_HOLD: //удержание дополнительной кнопки
        timerMode = 0; //деактивируем таймер
        switch (mode) {
          case 0: timerCnt = 0; break; //сбрасываем секундомер
          case 1: timerCnt = timerTime; break; //сбрасываем таймер
        }
        time_out = 0; //сбрасываем таймер автовыхода
        _sec = 0; //обновление экрана
        break;
    }
  }
}
//----------------------------Антиотравление индикаторов-------------------------------
void burnIndi(void) //антиотравление индикаторов
{
#if BURN_INDI_TYPE
  if (_tmrBurn >= BURN_PERIOD && RTC.s >= BURN_PHASE) {
    _tmrBurn = 0; //сбрасываем таймер
    dotSetBright(0); //выключаем точки
    for (byte indi = 0; indi < LAMP_NUM; indi++) {
      indiClr(); //очистка индикаторов
      for (byte loops = 0; loops < BURN_LOOPS; loops++) {
        for (byte digit = 0; digit < 10; digit++) {
          indiPrintNum(cathodeMask[digit], indi); //отрисовываем цифру
          for (_timer_ms[TMR_MS] = BURN_TIME; _timer_ms[TMR_MS];) { //ждем
            if (check_keys()) { //если нажата кнопка
              indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
              indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
              return; //выходим
            }
            dataUpdate(); //обработка данных
          }
        }
      }
    }
    indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
    indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
  }
#else
  if (_tmrBurn >= BURN_PERIOD && RTC.s >= BURN_PHASE) {
    _tmrBurn = 0; //сбрасываем таймер
    for (byte indi = 0; indi < LAMP_NUM; indi++) {
      for (byte loops = 0; loops < BURN_LOOPS; loops++) {
        for (byte digit = 0; digit < 10; digit++) {
          indiPrintNum(cathodeMask[digit], indi); //отрисовываем цифру
          for (_timer_ms[TMR_MS] = BURN_TIME; _timer_ms[TMR_MS];) { //ждем
            if (check_keys()) { //если нажата кнопка
              indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
              indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
              return; //выходим
            }
            dataUpdate(); //обработка данных
            dotFlash(); //мигаем точками
          }
        }
      }
      indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
      indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
      indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
    }
  }
#endif
}
//----------------------------------Анимация цифр-----------------------------------
void flipIndi(uint8_t flipMode, boolean demo) //анимация цифр
{
  _animShow = 0; //сбрасываем флаг

  switch (flipMode) {
    case 0: return;
    case 1: if (demo) return; else flipMode = random(0, FLIP_EFFECT_NUM); break;
    default: flipMode -= 2; break;
  }

  uint8_t anim_buf[12];
  uint8_t changeIndi = 0;
  uint8_t HH = (RTC.m) ? RTC.h : ((RTC.h) ? (RTC.h - 1) : 23);
  uint8_t MM = (RTC.m) ? (RTC.m - 1) : 59;

  if (!demo) {
    if (RTC.h / 10 != HH / 10) changeIndi |= (0x01 << 0);
    if (RTC.h % 10 != HH % 10) changeIndi |= (0x01 << 1);
    if (RTC.m / 10 != MM / 10) changeIndi |= (0x01 << 2);
    if (RTC.m % 10 != MM % 10) changeIndi |= (0x01 << 3);
#if LAMP_NUM > 4
    changeIndi |= (0x01 << 4);
    changeIndi |= (0x01 << 5);
#endif
  }
  else {
    indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
    indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
#if LAMP_NUM > 4
    indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
    changeIndi = 0x3F;
#else
    changeIndi = 0x0F;
#endif
  }

  switch (flipMode) {
    case FLIP_BRIGHT: { //плавное угасание и появление
        anim_buf[0] = indiMaxBright;
        anim_buf[1] = 0;
        *((uint16_t*)anim_buf + 2) = FLIP_MODE_2_TIME / indiMaxBright; //расчёт шага яркости режима 2

        while (!check_keys()) {
          dataUpdate(); //обработка данных
          dotFlash(); //мигаем точками
#if LAMP_NUM > 4 && SECONDS_ANIM
          tickSecs(); //анимация секунд
#endif

          if (!_timer_ms[TMR_ANIM]) { //если таймер истек
            if (!anim_buf[1]) {
              if (anim_buf[0] > 0) {
                anim_buf[0]--;
                for (uint8_t i = 0; i < LAMP_NUM; i++) if (changeIndi & (0x01 << i)) indiSetBright(anim_buf[0], i);
              }
              else {
                anim_buf[1] = 1;
                indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
                indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
              }
            }
            else {
              if (anim_buf[0] < indiMaxBright) {
                anim_buf[0]++;
                for (uint8_t i = 0; i < LAMP_NUM; i++) if (changeIndi & (0x01 << i)) indiSetBright(anim_buf[0], i);
              }
              else break;
            }
            _timer_ms[TMR_ANIM] = *((uint16_t*)anim_buf + 2); //устанавливаем таймер
          }
        }
        indiSetBright(indiMaxBright); //возвращаем максимальную яркость
      }
      break;
    case FLIP_ORDER_OF_NUMBERS: //перемотка по порядку числа
      //старое время
      anim_buf[0] = HH / 10;
      anim_buf[1] = HH % 10;
      anim_buf[2] = MM / 10;
      anim_buf[3] = MM % 10;
      anim_buf[4] = 5;
      anim_buf[5] = 9;
      //новое время
      anim_buf[6] = RTC.h / 10;
      anim_buf[7] = RTC.h % 10;
      anim_buf[8] = RTC.m / 10;
      anim_buf[9] = RTC.m % 10;
      anim_buf[10] = 0;
      anim_buf[11] = 0;

      while (!check_keys()) {
        dataUpdate(); //обработка данных
        dotFlash(); //мигаем точками

        if (!_timer_ms[TMR_ANIM]) { //если таймер истек
          for (uint8_t i = 0; i < LAMP_NUM; i++) {
            if (changeIndi & (0x01 << i)) {
              if (anim_buf[i]) anim_buf[i]--;
              else anim_buf[i] = 9;
              if (anim_buf[i] == anim_buf[i + 6]) changeIndi &= ~(0x01 << i);
              indiPrintNum(anim_buf[i], i);
            }
          }
          if (!changeIndi) break;
          _timer_ms[TMR_ANIM] = FLIP_MODE_3_TIME; //устанавливаем таймер
        }
      }
      break;
    case FLIP_ORDER_OF_CATHODES: //перемотка по порядку катодов в лампе
      if (!demo) {
        //старое время
        anim_buf[0] = HH / 10;
        anim_buf[1] = HH % 10;
        anim_buf[2] = MM / 10;
        anim_buf[3] = MM % 10;
        anim_buf[4] = 5;
        anim_buf[5] = 9;
        //новое время
        anim_buf[6] = RTC.h / 10;
        anim_buf[7] = RTC.h % 10;
        anim_buf[8] = RTC.m / 10;
        anim_buf[9] = RTC.m % 10;
        anim_buf[10] = 0;
        anim_buf[11] = 0;

        for (uint8_t i = 0; i < LAMP_NUM; i++) {
          for (uint8_t f = 0; f < 2; f++) {
            for (uint8_t c = 0; c < 10; c++) {
              if (cathodeMask[c] == anim_buf[i + ((f) ? 0 : 6)]) {
                anim_buf[i + ((f) ? 6 : 0)] = c;
                break;
              }
            }
          }
        }
      }
      else {
        for (uint8_t i = 0; i < LAMP_NUM; i++) {
          anim_buf[i] = 9;
          anim_buf[i + 6] = 0;
        }
      }

      while (!check_keys()) {
        dataUpdate(); //обработка данных
        dotFlash(); //мигаем точками

        if (!_timer_ms[TMR_ANIM]) { //если таймер истек
          for (uint8_t i = 0; i < LAMP_NUM; i++) {
            if (changeIndi & (0x01 << i)) {
              if (anim_buf[i]) anim_buf[i]--;
              else anim_buf[i] = 9;
              if (anim_buf[i] == anim_buf[i + 6]) changeIndi &= ~(0x01 << i);
              indiPrintNum(cathodeMask[anim_buf[i]], i);
            }
          }
          if (!changeIndi) break;
          _timer_ms[TMR_ANIM] = FLIP_MODE_4_TIME; //устанавливаем таймер
        }
      }
      break;
    case FLIP_TRAIN: { //поезд
        //старое время
        *(uint16_t*)anim_buf = (uint16_t)(HH * 100) + MM;
        //новое время
        *((uint16_t*)anim_buf + 2) = (uint16_t)(RTC.h * 100) + RTC.m;

        for (uint8_t c = 0; c < 2; c++) {
          for (uint8_t i = 0; i < LAMP_NUM;) {
            dataUpdate(); //обработка данных
            dotFlash(); //мигаем точками

            if (check_keys()) return; //возврат если нажата кнопка
            if (!_timer_ms[TMR_ANIM]) { //если таймер истек
              indiClr(); //очистка индикатора
              switch (c) {
                case 0:
                  indiPrintNum(*(uint16_t*)anim_buf, i + 1); //вывод часов
#if LAMP_NUM > 4
                  indiPrintNum(59, i + 5); //вывод секунд
#endif
                  break;
                case 1:
                  indiPrintNum(*((uint16_t*)anim_buf + 2), i - (LAMP_NUM - 1)); //вывод часов
#if LAMP_NUM > 4
                  indiPrintNum(RTC.s, i - 1); //вывод секунд
#endif
                  break;
              }
              i++; //прибавляем цикл
              _timer_ms[TMR_ANIM] = FLIP_MODE_5_TIME; //устанавливаем таймер
            }
          }
        }
      }
      break;
    case FLIP_RUBBER_BAND: //резинка
#if LAMP_NUM > 4
      //старое время
      anim_buf[5] = HH / 10; //часы
      anim_buf[4] = HH % 10; //часы
      anim_buf[3] = MM / 10; //минуты
      anim_buf[2] = MM % 10; //минуты
      anim_buf[1] = 5; //секунды
      anim_buf[0] = 9; //секунды
      //новое время
      anim_buf[6] = RTC.h / 10; //часы
      anim_buf[7] = RTC.h % 10; //часы
      anim_buf[8] = RTC.m / 10; //минуты
      anim_buf[9] = RTC.m % 10; //минуты
#else
      //старое время
      anim_buf[3] = HH / 10; //часы
      anim_buf[2] = HH % 10; //часы
      anim_buf[1] = MM / 10; //минуты
      anim_buf[0] = MM % 10; //минуты
      //новое время
      anim_buf[4] = RTC.h / 10; //часы
      anim_buf[5] = RTC.h % 10; //часы
      anim_buf[6] = RTC.m / 10; //минуты
      anim_buf[7] = RTC.m % 10; //минуты
#endif

      changeIndi = 0;

      for (uint8_t c = 0; c < 2; c++) {
        for (uint8_t i = 0; i < LAMP_NUM;) {
          dataUpdate(); //обработка данных
          dotFlash(); //мигаем точками

          if (check_keys()) return; //возврат если нажата кнопка
          if (!_timer_ms[TMR_ANIM]) { //если таймер истек
#if LAMP_NUM > 4
            anim_buf[10] = RTC.s / 10; //секунды
            anim_buf[11] = RTC.s % 10; //секунды
#endif
            switch (c) {
              case 0:
                for (uint8_t b = i + 1; b > 0; b--) {
                  if ((b - 1) == (i - changeIndi)) indiPrintNum(anim_buf[i], LAMP_NUM - b); //вывод часов
                  else indiClr(LAMP_NUM - b); //очистка индикатора
                }
                if (changeIndi++ >= i) {
                  changeIndi = 0; //сбрасываем позицию индикатора
                  i++; //прибавляем цикл
                }
                break;
              case 1:
                for (uint8_t b = 0; b < LAMP_NUM - i; b++) {
                  if (b == changeIndi) indiPrintNum(anim_buf[((LAMP_NUM * 2) - 1) - i], b); //вывод часов
                  else indiClr(b); //очистка индикатора
                }
                if (changeIndi++ >= (LAMP_NUM - 1) - i) {
                  changeIndi = 0; //сбрасываем позицию индикатора
                  i++; //прибавляем цикл
                }
                break;
            }
            _timer_ms[TMR_ANIM] = FLIP_MODE_6_TIME; //устанавливаем таймер
          }
        }
      }
      break;
    case FLIP_WAVE: //волна
      //новое время
      anim_buf[0] = RTC.h / 10; //часы
      anim_buf[1] = RTC.h % 10; //часы
      anim_buf[2] = RTC.m / 10; //минуты
      anim_buf[3] = RTC.m % 10; //минуты

      for (uint8_t c = 0; c < 2; c++) {
        for (uint8_t i = 0; i < LAMP_NUM;) {
          dataUpdate(); //обработка данных
          dotFlash(); //мигаем точками

          if (check_keys()) return; //возврат если нажата кнопка
          if (!_timer_ms[TMR_ANIM]) { //если таймер истек
#if LAMP_NUM > 4
            anim_buf[4] = RTC.s / 10; //секунды
            anim_buf[5] = RTC.s % 10; //секунды
#endif
            switch (c) {
              case 0: indiClr(i); break; //очистка индикатора
              case 1: indiPrintNum(anim_buf[i], i); break; //вывод часов
            }
            i++; //прибавляем цикл
            _timer_ms[TMR_ANIM] = FLIP_MODE_7_TIME; //устанавливаем таймер
          }
        }
      }
      break;
    case FLIP_HIGHLIGHTS: //блики
      //новое время
      anim_buf[0] = RTC.h / 10; //часы
      anim_buf[1] = RTC.h % 10; //часы
      anim_buf[2] = RTC.m / 10; //минуты
      anim_buf[3] = RTC.m % 10; //минуты

      for (uint8_t i = 0; i < LAMP_NUM;) {
        changeIndi = random(0, LAMP_NUM);
        for (uint8_t c = 0; c < 2;) {
          dataUpdate(); //обработка данных
          dotFlash(); //мигаем точками

          if (check_keys()) return; //возврат если нажата кнопка
          if (!_timer_ms[TMR_ANIM]) { //если таймер истек
#if LAMP_NUM > 4
            anim_buf[4] = RTC.s / 10; //секунды
            anim_buf[5] = RTC.s % 10; //секунды
#endif
            for (uint8_t b = 0; b < i; b++) {
              while (anim_buf[LAMP_NUM + b] == changeIndi) {
                changeIndi = random(0, LAMP_NUM);
                b = 0;
              }
            }
            anim_buf[LAMP_NUM + i] = changeIndi;
            switch (c) {
              case 0: indiClr(changeIndi); break; //очистка индикатора
              case 1:
                indiPrintNum(anim_buf[changeIndi], changeIndi);
                i++; //прибавляем цикл
                break; //вывод часов
            }
            c++; //прибавляем цикл
            _timer_ms[TMR_ANIM] = FLIP_MODE_8_TIME; //устанавливаем таймер
          }
        }
      }
      break;
    case FLIP_EVAPORATION: //испарение
      //новое время
      anim_buf[0] = RTC.h / 10; //часы
      anim_buf[1] = RTC.h % 10; //часы
      anim_buf[2] = RTC.m / 10; //минуты
      anim_buf[3] = RTC.m % 10; //минуты

      for (uint8_t c = 0; c < 2; c++) {
        changeIndi = random(0, LAMP_NUM);
        for (uint8_t i = 0; i < LAMP_NUM;) {
          dataUpdate(); //обработка данных
          dotFlash(); //мигаем точками

          if (check_keys()) return; //возврат если нажата кнопка
          if (!_timer_ms[TMR_ANIM]) { //если таймер истек
#if LAMP_NUM > 4
            anim_buf[4] = RTC.s / 10; //секунды
            anim_buf[5] = RTC.s % 10; //секунды
#endif
            for (uint8_t b = 0; b < i; b++) {
              while (anim_buf[LAMP_NUM + b] == changeIndi) {
                changeIndi = random(0, LAMP_NUM);
                b = 0;
              }
            }
            anim_buf[LAMP_NUM + i] = changeIndi;
            switch (c) {
              case 0: indiClr(changeIndi); break; //очистка индикатора
              case 1: indiPrintNum(anim_buf[changeIndi], changeIndi); break; //вывод часов
            }
            i++; //прибавляем цикл
            _timer_ms[TMR_ANIM] = FLIP_MODE_9_TIME; //устанавливаем таймер
          }
        }
      }
      break;
  }
}
//-----------------------------Анимация секунд------------------------------------------------
void tickSecs(void) //анимация секунд
{
  static uint8_t time_tick;
  static uint8_t time_old;
  static uint8_t time_anim[4];

  if (time_old != RTC.s) {
    time_old = RTC.s;
    uint8_t temp = (RTC.s) ? (RTC.s - 1) : 59;
    time_anim[0] = temp % 10;
    time_anim[1] = temp / 10;
    time_anim[2] = RTC.s % 10;
    time_anim[3] = RTC.s / 10;
    time_tick = 0x03;
  }
  if (time_tick && !_timer_ms[TMR_MS]) {
    _timer_ms[TMR_MS] = SECONDS_ANIM_TIME;
    for (uint8_t i = 0; i < 2; i++) {
      if (time_anim[i] != time_anim[i + 2]) {
        if (--time_anim[i] > 9) time_anim[i] = 9;
      }
      else time_tick &= ~(0x01 << i);
      indiPrintNum(time_anim[i], (LAMP_NUM - 1) - i); //вывод секунд
    }
  }
}
//-----------------------------Главный экран------------------------------------------------
void mainScreen(void) //главный экран
{
  if (_animShow) flipIndi(fastSettings.flipMode, 0); //анимация цифр основная
#if LAMP_NUM > 4 && SECONDS_ANIM
  tickSecs(); //анимация секунд
#endif

  if (!_sec) {
    _sec = 1; //сбрасываем флаг

    burnIndi(); //антиотравление индикаторов
    glitchMode(); //имитация глюков
    autoShowTemp(); //автоматический показ температуры

    indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
    indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
#if LAMP_NUM > 4 && !SECONDS_ANIM
    indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
#endif
  }

  switch (check_keys()) {
    case LEFT_KEY_HOLD: //удержание левой кнопки
      settings_time(); //иначе настройки времени
      _sec = _animShow = 0; //обновление экрана
      break;
    case LEFT_KEY_PRESS: //клик левой кнопкой
      showTemp(); //показать температуру
      _sec = _animShow = 0; //обновление экрана
      break;
    case RIGHT_KEY_PRESS: //клик правой кнопкой
      showDate(); //показать дату
      _sec = _animShow = 0; //обновление экрана
      break;
#if ALARM_TYPE
    case RIGHT_KEY_HOLD: //удержание правой кнопки
#if ALARM_TYPE == 1
      settings_singleAlarm(); //настройка будильника
#else
      settings_multiAlarm(); //настройка будильников
#endif
      _sec = _animShow = 0; //обновление экрана
      break;
#endif
    case SET_KEY_PRESS: //клик средней кнопкой
      fastSetSwitch(); //переключение настроек
      _sec = _animShow = 0; //обновление экрана
      break;
    case SET_KEY_HOLD: //удержание средней кнопки
      if (alarmWaint) {
        buzz_pulse(ALM_OFF_SOUND_FREQ, ALM_OFF_SOUND_TIME); //звук выключения будильника
        alarmReset(); //сброс будильника
      }
      else settings_main(); //настроки основные
      _sec = _animShow = 0; //обновление экрана
      break;
#if !BTN_ADD_DISABLE
    case ADD_KEY_PRESS: //клик дополнительной кнопкой
      timerStopwatch(); //таймер-секундомер
      _sec = _animShow = 0; //обновление экрана
      break;
#endif
  }
}
//-------------------------------Получить 12-ти часовой формат---------------------------------------------------
uint8_t get_12h(uint8_t timeH) //получить 12-ти часовой формат
{
  return (timeH > 12) ? (timeH - 12) : (timeH) ? timeH : 12;
}
