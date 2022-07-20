/*
  Arduino IDE 1.8.13 версия прошивки 1.6.8 релиз от 20.07.22
  Специльно для проекта "Часы на ГРИ и Arduino v2 | AlexGyver"
  Страница проекта - https://alexgyver.ru/nixieclock_v2

  Исходник - https://github.com/radon-lab/NixieClock
  Автор Radon-lab.
*/
//-----------------Ошибки-----------------
enum {
  VCC_ERROR,           //0001 - ошибка напряжения питания
  SQW_ERROR,           //0002 - ошибка сигнала SQW
  DS3231_ERROR,        //0003 - ошибка связи с модулем DS3231
  DS3231_OSF_ERROR,    //0004 - ошибка осцилятора модуля DS3231
  MEMORY_ERROR,        //0005 - ошибка памяти еепром
  SQW_LOW_TIME_ERROR,  //0006 - ошибка слишком короткий сигнал SQW
  SQW_HIGH_TIME_ERROR, //0007 - ошибка слишком длинный сигнал SQW
  RESET_ERROR          //0008 - ошибка аварийной перезагрузки
};
void dataUpdate(void); //процедура обработки данных
void SET_ERROR(uint8_t err); //процедура установка ошибки

//-----------------Таймеры------------------
enum {
  TMR_MS,        //таймер общего назначения
  TMR_IR,        //таймер инфракрасного приемника
  TMR_SENS,      //таймер сенсоров температуры
  TMR_PLAYER,    //таймер плеера/мелодий
  TMR_BACKL,     //таймер подсветки
  TMR_LIGHT,     //таймер сенсора яркости
  TMR_COLOR,     //таймер смены цвета подсветки
  TMR_DOT,       //таймер точек
  TMR_ANIM,      //таймер анимаций
  TIMERS_MS_NUM  //количество таймеров
};
uint16_t _timer_ms[TIMERS_MS_NUM]; //таймер отсчета миллисекунд

enum {
  TMR_ALM,       //таймер тайм-аута будильника
  TMR_ALM_WAINT, //таймер ожидания повторного включения будильника
  TMR_ALM_SOUND, //таймер отключения звука будильника
  TMR_SYNC,      //таймер синхронизации
  TMR_BURN,      //таймер антиотравления
  TMR_TEMP,      //таймер автопоказа температуры
  TMR_GLITCH,    //таймер глюков
  TIMERS_SEC_NUM //количество таймеров
};
uint16_t _timer_sec[TIMERS_SEC_NUM]; //таймер отсчета секунд

volatile uint8_t tick_ms;  //счетчик тиков миллисекунд
volatile uint8_t tick_sec; //счетчик тиков от RTC

//----------------Температура--------------
struct sensorData {
  uint16_t temp; //температура
  uint16_t press; //давление
  uint8_t hum; //влажность
  uint8_t type; //тип датчика температуры
  boolean init; //флаг инициализации порта
  boolean err; //ошибка сенсора
} sens;

//----------------Библиотеки----------------
#include <util/delay.h>

//---------------Конфигурации---------------
#include "userConfig.h"
#include "connection.h"
#include "config.h"

//----------------Периферия----------------
#include "WIRE.h"
#include "EEPROM.h"
#include "PLAYER.h"
#include "RDA.h"
#include "RTC.h"
#include "BME.h"
#include "DHT.h"
#include "DS.h"
#include "IR.h"
#include "WS.h"
#include "INDICATION.h"

//-----------------Настройки----------------
struct Settings_1 {
  uint8_t indiBright[2] = {DEFAULT_INDI_BRIGHT_N, DEFAULT_INDI_BRIGHT}; //яркость индикаторов
  uint8_t backlBright[2] = {DEFAULT_BACKL_BRIGHT_N, DEFAULT_BACKL_BRIGHT}; //яркость подсветки
  uint8_t dotBright[2] = {DEFAULT_DOT_BRIGHT_N, DEFAULT_DOT_BRIGHT}; //яркость точек
  uint8_t timeBright[2] = {DEFAULT_NIGHT_START, DEFAULT_NIGHT_END}; //время перехода яркости
  uint8_t timeHour[2] = {DEFAULT_HOUR_SOUND_START, DEFAULT_HOUR_SOUND_END}; //время звукового оповещения нового часа
  boolean timeFormat = DEFAULT_TIME_FORMAT; //формат времени
  boolean knockSound = DEFAULT_KNOCK_SOUND; //звук кнопок или озвучка
  uint8_t volumeSound = DEFAULT_PLAYER_VOLUME; //громкость озвучки
  int8_t tempCorrect = DEFAULT_TEMP_CORRECT; //коррекция температуры
  boolean glitchMode = DEFAULT_GLITCH_MODE; //режим глюков
  uint8_t autoTempTime = DEFAULT_AUTO_TEMP_TIME; //интервал времени показа температуры
  uint8_t burnMode = DEFAULT_BURN_MODE; //режим антиотравления индикаторов
} mainSettings;

struct Settings_2 {
  uint8_t flipMode = DEFAULT_FLIP_ANIM; //режим анимации
  uint8_t backlMode = DEFAULT_BACKL_MODE; //режим подсветки
  uint8_t backlColor = DEFAULT_BACKL_COLOR * 10; //цвет подсветки
  uint8_t dotMode = DEFAULT_DOT_MODE; //режим точек
} fastSettings;

struct Settings_3 { //настройки радио
  uint16_t stationsSave[9] = {DEFAULT_RADIO_STATIONS};
  uint16_t stationsFreq = RADIO_MIN_FREQ;
  uint8_t volume = DEFAULT_RADIO_VOLUME;
  uint8_t stationNum;
} radioSettings;


//переменные обработки кнопок
struct buttonData {
  uint8_t state; //текущее состояние кнопок
  uint8_t adc; //результат опроса аналоговых кнопок
  uint8_t leftMax; //максимальное значение левой клавиши
  uint8_t leftMin; //минимальное значение левой клавиши
  uint8_t rightMax; //максимальное значение правой клавиши
  uint8_t rightMin; //минимальное значение правой клавиши
  uint8_t setMax; //максимальное значение клавиши ок
  uint8_t setMin; //минимальное значение клавиши ок
#if (BTN_ADD_TYPE == 2)
  uint8_t addMax; //максимальное значение дополнитнльной клавиши
  uint8_t addMin; //минимальное значение дополнитнльной клавиши
#endif
} btn;
uint8_t analogState; //флаги обновления аналоговых портов

//переменные работы с подсветкой
struct backlightData {
  uint16_t mode_2_time; //время эффекта номер 2
  uint8_t mode_2_step; //шаг эффекта номер 2
  uint8_t mode_4_step; //шаг эффекта номер 4
  uint16_t mode_6_time; //время эффекта номер 6
  uint8_t mode_6_step; //шаг эффекта номер 6
} backl;
uint8_t backlMaxBright; //максимальная яркость подсветки
uint8_t backlMinBright; //минимальная яркость подсветки

uint8_t dotBrightStep; //шаг мигания точек
uint8_t dotBrightTime; //период шага мигания точек
uint8_t dotMaxBright;  //максимальная яркость точек

uint8_t indiMaxBright; //максимальная яркость индикаторов

//флаги анимаций
boolean animShow; //флаг анимации
boolean secUpd; //флаг обновления секунды
boolean dotUpd; //флаг обновления точек

//alarmRead/Write - час | минута | режим(0 - выкл, 1 - одиночный, 2 - вкл, 3 - по будням, 4 - по дням недели) | день недели(вс,сб,пт,чт,ср,вт,пн,null) | мелодия будильника
boolean alarmEnabled; //флаг включенного будильника
boolean alarmWaint; //флаг ожидания звука будильника
uint8_t alarmsNum; //текущее количество будильников
uint8_t alarm; //флаг активоного будильника

//переменные таймера/секундомера
uint8_t timerMode; //режим таймера/секундомера
uint16_t timerCnt; //счетчик таймера/секундомера
uint16_t timerTime = TIMER_TIME; //время таймера сек

//переменные работы со звуками
struct soundData {
  uint8_t replay; //флаг повтора мелодии
  uint16_t semp; //текущий семпл мелодии
  uint16_t link; //ссылка на мелодию
  uint16_t size; //количество семплов мелодии
} sound;
volatile uint16_t cnt_puls; //количество циклов для работы пищалки
volatile uint16_t cnt_freq; //частота для генерации звука пищалкой
uint16_t tmr_score; //циклы полуволны для работы пищалки

uint16_t vcc_adc; //напряжение питания
#define GET_VCC(ref, adc) (float)((ref * 1023.0) / (float)adc) //расчет напряжения питания
#define GET_ADC(vcc, coef) (int16_t)((255.0 / (float)vcc) * ((float)vcc / (float)coef)) //рассчет значения ацп кнопок

#define BTN_GIST_TICK (BTN_GIST_TIME / (US_PERIOD / 1000.0)) //количество циклов для защиты от дребезга
#define BTN_HOLD_TICK (BTN_HOLD_TIME / (US_PERIOD / 1000.0)) //количество циклов после которого считается что кнопка зажата

//перечисления кнопок
enum {
  KEY_NULL, //кнопка не нажата
  LEFT_KEY_PRESS, //клик левой кнопкой
  LEFT_KEY_HOLD, //удержание левой кнопки
  RIGHT_KEY_PRESS, //клик правой кнопкой
  RIGHT_KEY_HOLD, //удержание правой кнопки
  SET_KEY_PRESS, //клик средней кнопкой
  SET_KEY_HOLD, //удержание средней кнопки
  ADD_KEY_PRESS, //клик дополнительной кнопкой
  ADD_KEY_HOLD //удержание дополнительной кнопки
};

//перечисления меню настроек
enum {
  SET_TIME_FORMAT, //формат времени
  SET_GLITCH, //анимация глюков
  SET_BTN_SOUND, //звук кнопок(озвучки)
  SET_HOUR_TIME, //звук смены часа
  SET_BRIGHT_TIME, //время смены яркости
  SET_INDI_BRIGHT, //яркость индикаторов
  SET_BACKL_BRIGHT, //яркость подсветки
  SET_DOT_BRIGHT, //яркость точек
  SET_TEMP_SENS, //настройка датчика температуры
  SET_AUTO_TEMP, //автопоказ температуры
  SET_BURN_MODE, //анимация антиотравления индикаторов
  SET_MAX_ITEMS //максимум пунктов меню
};

//перечисления меню отладки
enum {
  DEB_TIME_CORRECT, //корректировка хода времени
  DEB_DEFAULT_MIN_PWM, //минимальное значение шим
  DEB_DEFAULT_MAX_PWM, //максимальное значение шим
  DEB_HV_ADC, //значение ацп преобразователя
  DEB_IR_BUTTONS, //програмирование кнопок
  DEB_LIGHT_SENS, //калибровка датчика освещения
  DEB_RESET, //максимальное значение шим
  DEB_MAX_ITEMS //максимум пунктов меню
};

//перечисления режимов антиотравления
enum {
  BURN_ALL, //перебор всех индикаторов
  BURN_SINGLE, //перебор одного индикатора
  BURN_SINGLE_TIME, //перебор одного индикатора с отображением времени
  BURN_EFFECT_NUM //максимум эффектов антиотравления
};

//перечисления быстрого меню
enum {
  FAST_BACKL_MODE, //режим подсветки
  FAST_FLIP_MODE, //режим перелистывания
  FAST_DOT_MODE, //режим точек
  FAST_BACKL_COLOR //цвет подсветки
};

//перечисления анимаций перебора цифр
enum {
  FLIP_BRIGHT, //плавное угасание и появление
  FLIP_ORDER_OF_NUMBERS, //перемотка по порядку числа
  FLIP_ORDER_OF_CATHODES, //перемотка по порядку катодов в лампе
  FLIP_TRAIN, //поезд
  FLIP_RUBBER_BAND, //резинка
  FLIP_GATES, //ворота
  FLIP_WAVE, //волна
  FLIP_HIGHLIGHTS, //блики
  FLIP_EVAPORATION, //испарение
  FLIP_EFFECT_NUM //максимум эффектов перелистывания
};

//перечисления режимов подсветки
enum {
  BACKL_OFF, //выключена
  BACKL_STATIC, //статичная
  BACKL_PULS, //дыхание
  BACKL_PULS_COLOR, //дыхание со сменой цвета при затухании
  BACKL_RUNNING_FIRE, //бегущий огонь
  BACKL_RUNNING_FIRE_COLOR, //бегущий огонь со сменой цвета
  BACKL_WAVE, //волна
  BACKL_WAVE_COLOR, //волна со сменой цвета
  BACKL_SMOOTH_COLOR_CHANGE, //плавная смена цвета
  BACKL_RAINBOW, //радуга
  BACKL_CONFETTI, //конфетти
  BACKL_EFFECT_NUM //максимум эффектов подсветки
};

//перечисления режимов точек
enum {
  DOT_OFF, //выключена
  DOT_STATIC, //статичная
  DOT_PULS, //плавно мигает
  DOT_BLINK, //одиночное мигание
  DOT_DOOBLE_BLINK //двойное мигание
};

//перечисления настроек будильника
enum {
  ALARM_HOURS, //час будильника
  ALARM_MINS, //минута будильника
  ALARM_MODE, //режим будильника
  ALARM_DAYS, //день недели будильника
  ALARM_SOUND, //мелодия будильника
  ALARM_MAX_ARR //максимальное количество данных
};

//перечисления датчиков температуры
enum {
  SENS_DS3231, //датчик DS3231
#if SENS_BME_ENABLE
  SENS_BME, //датчики BME/BMP
#endif
#if SENS_PORT_ENABLE
  SENS_DS18B20, //датчик DS18B20
  SENS_DHT, //датчик DHT
#endif
  SENS_ALL //датчиков всего
};

//перечисления системных звуков
enum {
  SOUND_PASS_ERROR, //звук ошибки ввода пароля
  SOUND_RESET_SETTINGS, //звук сброса настроек
  SOUND_ALARM_DISABLE, //звук отключения будильника
  SOUND_ALARM_WAINT, //звук ожидания будильника
  SOUND_TIMER_WARN, //звук окончания таймера
  SOUND_HOUR //звук смены часа
};

//перечисления режимов воспроизведения мелодий
enum {
  REPLAY_STOP, //остановить воспроизведение
  REPLAY_ONCE, //проиграть один раз
  REPLAY_CYCLE //проиграть по кругу
};

#define CONVERT_NUM(x) ((x[0] - 48) * 100 + (x[2] - 48) * 10 + (x[4] - 48)) //преобразовать строку в число
#define CONVERT_CHAR(x) (x - 48) //преобразовать символ в число

#define EEPROM_BLOCK_TIME EEPROM_BLOCK_NULL //блок памяти времени
#define EEPROM_BLOCK_SETTINGS_FAST (EEPROM_BLOCK_TIME + sizeof(RTC)) //блок памяти настроек свечения
#define EEPROM_BLOCK_SETTINGS_MAIN (EEPROM_BLOCK_SETTINGS_FAST + sizeof(fastSettings)) //блок памяти основных настроек
#define EEPROM_BLOCK_SETTINGS_RADIO (EEPROM_BLOCK_SETTINGS_MAIN + sizeof(mainSettings)) //блок памяти настроек радио
#define EEPROM_BLOCK_SETTINGS_DEBUG (EEPROM_BLOCK_SETTINGS_RADIO + sizeof(radioSettings)) //блок памяти настроек отладки
#define EEPROM_BLOCK_ERROR (EEPROM_BLOCK_SETTINGS_DEBUG + sizeof(debugSettings)) //блок памяти ошибок
#define EEPROM_BLOCK_ALARM (EEPROM_BLOCK_ERROR + 1) //блок памяти количества будильников

#define EEPROM_BLOCK_CRC_DEFAULT (EEPROM_BLOCK_ALARM + sizeof(alarmsNum)) //блок памяти контрольной суммы настроек
#define EEPROM_BLOCK_CRC_TIME (EEPROM_BLOCK_CRC_DEFAULT + 1) //блок памяти контрольной суммы времени
#define EEPROM_BLOCK_CRC_FAST (EEPROM_BLOCK_CRC_TIME + 1) //блок памяти контрольной суммы быстрых настроек
#define EEPROM_BLOCK_CRC_MAIN (EEPROM_BLOCK_CRC_FAST + 1) //блок памяти контрольной суммы основных настроек
#define EEPROM_BLOCK_CRC_RADIO (EEPROM_BLOCK_CRC_MAIN + 1) //блок памяти контрольной суммы настроек радио
#define EEPROM_BLOCK_CRC_DEBUG (EEPROM_BLOCK_CRC_RADIO + 1) //блок памяти контрольной суммы настроек отладки
#define EEPROM_BLOCK_CRC_DEBUG_DEFAULT (EEPROM_BLOCK_CRC_DEBUG + 1) //блок памяти контрольной суммы настроек отладки
#define EEPROM_BLOCK_CRC_ERROR (EEPROM_BLOCK_CRC_DEBUG_DEFAULT + 1) //блок контрольной суммы памяти ошибок
#define EEPROM_BLOCK_CRC_ALARM (EEPROM_BLOCK_CRC_ERROR + 1) //блок контрольной суммы количества будильников
#define EEPROM_BLOCK_ALARM_DATA (EEPROM_BLOCK_CRC_ALARM + 1) //первая ячейка памяти будильников

#define MAX_ALARMS ((EEPROM_BLOCK_MAX - EEPROM_BLOCK_ALARM_DATA) / ALARM_MAX_ARR) //максимальное количество будильников

#if BTN_TYPE
#define SET_CHK buttonCheckADC(btn.setMin, btn.setMax) //чтение средней аналоговой кнопки
#define LEFT_CHK buttonCheckADC(btn.leftMin, btn.leftMax) //чтение левой аналоговой кнопки
#define RIGHT_CHK buttonCheckADC(btn.rightMin, btn.rightMax) //чтение правой аналоговой кнопки

#if (BTN_ADD_TYPE == 2)
#define ADD_CHK buttonCheckADC(btn.addMin, btn.addMax) //чтение правой аналоговой кнопки
#endif
#endif

//----------------------------------Инициализация--------------------------------------------
int main(void) //инициализация
{
#if AMP_PORT_ENABLE
  AMP_INIT; //инициализация питания усилителя
#endif

#if (PLAYER_TYPE != 1) || PLAYER_UART_MODE
  uartDisable(); //отключение uart
#endif

#if !PLAYER_TYPE
  BUZZ_INIT; //инициализация бузера
#endif

#if !BTN_TYPE
  SET_INIT; //инициализация средней кнопки
  LEFT_INIT; //инициализация левой кнопки
  RIGHT_INIT; //инициализация правой кнопки
#endif

#if (BTN_ADD_TYPE == 1)
  ADD_INIT; //инициализация дополнительной кнопки
#endif

#if GEN_ENABLE
  CONV_INIT; //инициализация преобразователя
#endif
#if SQW_PORT_ENABLE
  SQW_INIT; //инициализация счета секунд
#endif
  BACKL_INIT; //инициализация подсветки

#if IR_PORT_ENABLE
  irInit();
#endif

  if (checkByte(EEPROM_BLOCK_ERROR, EEPROM_BLOCK_CRC_ERROR)) updateByte(0x00, EEPROM_BLOCK_ERROR, EEPROM_BLOCK_CRC_ERROR); //если контрольная сумма ошибок не совпала

  checkVCC(); //чтение напряжения питания

  if (checkSettingsCRC() || !SET_CHK) { //если контрольная сумма не совпала или зажата средняя кнопка то восстанавливаем настройеи по умолчанию
    updateData((uint8_t*)&RTC, sizeof(RTC), EEPROM_BLOCK_TIME, EEPROM_BLOCK_CRC_TIME); //записываем дату и время в память
    updateData((uint8_t*)&fastSettings, sizeof(fastSettings), EEPROM_BLOCK_SETTINGS_FAST, EEPROM_BLOCK_CRC_FAST); //записываем настройки яркости в память
    updateData((uint8_t*)&mainSettings, sizeof(mainSettings), EEPROM_BLOCK_SETTINGS_MAIN, EEPROM_BLOCK_CRC_MAIN); //записываем основные настройки в память
    updateData((uint8_t*)&radioSettings, sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO); //записываем настройки радио в память
#if ALARM_TYPE
    updateByte(alarmsNum, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
#endif
#if PLAYER_TYPE
    playerSetTrack(PLAYER_RESET_SOUND, PLAYER_GENERAL_FOLDER); //звук сброса настроек
#else
    melodyPlay(SOUND_RESET_SETTINGS, SOUND_LINK(general_sound), REPLAY_ONCE); //сигнал сброса настроек
#endif
  }
  else { //иначе загружаем настройки из памяти
    if (checkData(sizeof(RTC), EEPROM_BLOCK_TIME, EEPROM_BLOCK_CRC_TIME)) { //проверяем дату и время в памяти
      updateData((uint8_t*)&RTC, sizeof(RTC), EEPROM_BLOCK_TIME, EEPROM_BLOCK_CRC_TIME); //записываем дату и время в память
      SET_ERROR(MEMORY_ERROR); //устанавливаем ошибку памяти
    }
    if (checkData(sizeof(fastSettings), EEPROM_BLOCK_SETTINGS_FAST, EEPROM_BLOCK_CRC_FAST)) { //проверяем быстрые настройки
      updateData((uint8_t*)&fastSettings, sizeof(fastSettings), EEPROM_BLOCK_SETTINGS_FAST, EEPROM_BLOCK_CRC_FAST); //записываем быстрые настройки в память
      SET_ERROR(MEMORY_ERROR); //устанавливаем ошибку памяти
    }
    else EEPROM_ReadBlock((uint16_t)&fastSettings, EEPROM_BLOCK_SETTINGS_FAST, sizeof(fastSettings)); //считываем быстрые настройки из памяти
    if (checkData(sizeof(mainSettings), EEPROM_BLOCK_SETTINGS_MAIN, EEPROM_BLOCK_CRC_MAIN)) { //проверяем основные настройки
      updateData((uint8_t*)&mainSettings, sizeof(mainSettings), EEPROM_BLOCK_SETTINGS_MAIN, EEPROM_BLOCK_CRC_MAIN); //записываем основные настройки в память
      SET_ERROR(MEMORY_ERROR); //устанавливаем ошибку памяти
    }
    else EEPROM_ReadBlock((uint16_t)&mainSettings, EEPROM_BLOCK_SETTINGS_MAIN, sizeof(mainSettings)); //считываем основные настройки из памяти
    if (checkData(sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO)) { //проверяем настройки радио
      updateData((uint8_t*)&radioSettings, sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO); //записываем настройки радио в память
      SET_ERROR(MEMORY_ERROR); //устанавливаем ошибку памяти
    }
    else EEPROM_ReadBlock((uint16_t)&radioSettings, EEPROM_BLOCK_SETTINGS_RADIO, sizeof(radioSettings)); //считываем настройки радио из памяти
#if ALARM_TYPE
    if (checkByte(EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM)) { //проверяем количетво будильников
      updateByte(alarmsNum, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
      SET_ERROR(MEMORY_ERROR); //устанавливаем ошибку памяти
    }
    else alarmsNum = EEPROM_ReadByte(EEPROM_BLOCK_ALARM); //считываем количество будильников из памяти
#endif
  }

  if (checkDebugSettingsCRC()) updateData((uint8_t*)&debugSettings, sizeof(debugSettings), EEPROM_BLOCK_SETTINGS_DEBUG, EEPROM_BLOCK_CRC_DEBUG); //записываем настройки отладки в память
  if (checkData(sizeof(debugSettings), EEPROM_BLOCK_SETTINGS_DEBUG, EEPROM_BLOCK_CRC_DEBUG)) { //проверяем настройки отладки
    updateData((uint8_t*)&debugSettings, sizeof(debugSettings), EEPROM_BLOCK_SETTINGS_DEBUG, EEPROM_BLOCK_CRC_DEBUG); //записываем настройки отладки в память
    SET_ERROR(MEMORY_ERROR); //устанавливаем ошибку памяти
  }
  else EEPROM_ReadBlock((uint16_t)&debugSettings, EEPROM_BLOCK_SETTINGS_DEBUG, sizeof(debugSettings)); //считываем настройки отладки из памяти

#if GEN_ENABLE && GEN_FEEDBACK
  updateTresholdADC(); //обновление предела удержания напряжения
#endif

  indiChangeCoef(); //обновление коэффициента линейного регулирования

#if PLAYER_TYPE == 2
  sdPlayerInit(mainSettings.volumeSound); //инициализация плеера
#endif

  wireInit(); //инициализация шины wire
  indiInit(); //инициализация индикаторов

#if PLAYER_TYPE == 1
  dfPlayerInit(mainSettings.volumeSound); //инициализация плеера
#endif

  backlAnimDisable(); //запретили эффекты подсветки

  checkRTC(); //проверка модуля часов
  checkTempSens(); //проверка установленного датчика температуры

  if (!LEFT_CHK && check_pass()) debug_menu(); //если правая кнопка зажата запускаем отладку
  if (!RIGHT_CHK) test_system(); //если правая кнопка зажата запускаем тест системы

  checkErrors(); //проверка на наличие ошибок

#if ALARM_TYPE == 1
  alarmInit(); //инициализация будильника
#endif

  randomSeed(RTC.s * (RTC.m + RTC.h) + RTC.DD * RTC.MM); //радомный сид для глюков
  _timer_sec[TMR_BURN] = (uint16_t)(BURN_PERIOD * 60); //устанавливаем таймер антиотравления
  _timer_sec[TMR_SYNC] = (uint16_t)(RTC_SYNC_TIME * 60); //устанавливаем таймер синхронизации

#if LIGHT_SENS_ENABLE
  _timer_ms[TMR_LIGHT] = LIGHT_SENS_TIME;
  analogState |= 0x01; //установили флаг обновления АЦП сенсора яркости
#endif

  animsReset(); //сброс анимаций
  changeBright(); //установка яркости от времени суток

#if ALARM_TYPE
  checkAlarms(); //проверка будильников
#endif

  mainScreen(); //главный экран
  return 0; //конец
}
//-----------------------------Прерывание от RTC--------------------------------
ISR(INT0_vect) //внешнее прерывание на пине INT0 - считаем секунды с RTC
{
  tick_sec++; //прибавляем секунду
}
//-----------------------Прерывание сигнала для пищалки-------------------------
#if !PLAYER_TYPE
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
#endif
//-----------------------------Установка ошибки---------------------------------
void SET_ERROR(uint8_t err) //установка ошибки
{
  uint8_t _error_bit = (0x01 << err); //выбрали флаг ошибки
  EEPROM_UpdateByte(EEPROM_BLOCK_ERROR, EEPROM_ReadByte(EEPROM_BLOCK_ERROR) | _error_bit); //обновили ячейку ошибки
  EEPROM_UpdateByte(EEPROM_BLOCK_CRC_ERROR, EEPROM_ReadByte(EEPROM_BLOCK_CRC_ERROR) & (_error_bit ^ 0xFF)); //обновили ячейку контрольной суммы ошибки
}
//-----------------------------Расчет шага яркости-----------------------------
uint8_t setBrightStep(uint16_t _brt, uint16_t _step, uint16_t _time) //расчет шага яркости
{
  uint8_t temp = ceil((float)_brt / (float)_time * (float)_step); //расчёт шага яркости точки
  if (!temp) temp = 1; //если шаг слишком мал, устанавливаем минимум
  return temp;
}
//-------------------------Расчет периода шага яркости--------------------------
uint16_t setBrightTime(uint16_t _brt, uint16_t _step, uint16_t _time) //расчет периода шага яркости
{
  uint16_t temp = ceil((float)_time / (float)_brt); //расчёт шага яркости точки
  if (temp < _step) temp = _step; //если шаг слишком мал, устанавливаем минимум
  return temp;
}
//-------------------------Разрешить анимации подсветки-------------------------
void backlAnimEnable(void) //разрешить анимации подсветки
{
  fastSettings.backlMode &= 0x7F; //разрешили эффекты подсветки
}
//-------------------------Запретить анимации подсветки-------------------------
void backlAnimDisable(void) //запретить анимации подсветки
{
  fastSettings.backlMode |= 0x80; //запретили эффекты подсветки
}
//---------------------Установка яркости от времени суток-----------------------------
boolean checkHourStrart(uint8_t _start, uint8_t _end) //установка яркости от времени суток
{
  return ((_start > _end && (RTC.h >= _start || RTC.h < _end)) || (_start < _end && RTC.h >= _start && RTC.h < _end));
}
//-------------------------Получить 12-ти часовой формат------------------------
uint8_t get_12h(uint8_t timeH) //получить 12-ти часовой формат
{
  return (timeH > 12) ? (timeH - 12) : (timeH) ? timeH : 12; //возвращаем результат
}
//------------------------Сверка контрольной суммы---------------------------------
void checkCRC(uint8_t* crc, uint8_t data) //сверка контрольной суммы
{
  for (uint8_t i = 0; i < 8; i++) { //считаем для всех бит
    *crc = ((*crc ^ data) & 0x01) ? (*crc >> 0x01) ^ 0x8C : (*crc >> 0x01); //рассчитываем значение
    data >>= 0x01; //сдвигаем буфер
  }
}
//------------------------Проверка байта в памяти-------------------------------
boolean checkByte(uint8_t cell, uint8_t cellCRC) //проверка байта в памяти
{
  return (boolean)((EEPROM_ReadByte(cell) ^ 0xFF) != EEPROM_ReadByte(cellCRC));
}
//-----------------------Обновление байта в памяти-------------------------------
void updateByte(uint8_t data, uint8_t cell, uint8_t cellCRC) //обновление байта в памяти
{
  EEPROM_UpdateByte(cell, data);
  EEPROM_UpdateByte(cellCRC, data ^ 0xFF);
}
//------------------------Проверка данных в памяти-------------------------------
boolean checkData(uint8_t size, uint8_t cell, uint8_t cellCRC) //проверка данных в памяти
{
  uint8_t crc = 0;
  for (uint8_t n = 0; n < size; n++) checkCRC(&crc, EEPROM_ReadByte(cell + n));
  return (boolean)(crc != EEPROM_ReadByte(cellCRC));
}
//-----------------------Обновление данных в памяти-------------------------------
void updateData(uint8_t* str, uint8_t size, uint8_t cell, uint8_t cellCRC) //обновление данных в памяти
{
  uint8_t crc = 0;
  for (uint8_t n = 0; n < size; n++) checkCRC(&crc, str[n]);
  EEPROM_UpdateBlock((uint16_t)str, cell, size);
  EEPROM_UpdateByte(cellCRC, crc);
}
//--------------------Проверка контрольной суммы настроек--------------------------
boolean checkSettingsCRC(void) //проверка контрольной суммы настроек
{
  uint8_t CRC = 0; //буфер контрольной суммы

  for (uint8_t i = 0; i < sizeof(RTC); i++) checkCRC(&CRC, *((uint8_t*)&RTC + i));
  for (uint8_t i = 0; i < sizeof(fastSettings); i++) checkCRC(&CRC, *((uint8_t*)&fastSettings + i));
  for (uint8_t i = 0; i < sizeof(mainSettings); i++) checkCRC(&CRC, *((uint8_t*)&mainSettings + i));
  for (uint8_t i = 0; i < sizeof(radioSettings); i++) checkCRC(&CRC, *((uint8_t*)&radioSettings + i));


  if (EEPROM_ReadByte(EEPROM_BLOCK_CRC_DEFAULT) == CRC) return 0;
  else EEPROM_UpdateByte(EEPROM_BLOCK_CRC_DEFAULT, CRC);
  return 1;
}
//----------------Проверка контрольной суммы настроек отладки-----------------------
boolean checkDebugSettingsCRC(void) //проверка контрольной суммы настроек отладки
{
  uint8_t CRC = 0; //буфер контрольной суммы

  for (uint8_t i = 0; i < sizeof(debugSettings); i++) checkCRC(&CRC, *((uint8_t*)&debugSettings + i));

  if (EEPROM_ReadByte(EEPROM_BLOCK_CRC_DEBUG_DEFAULT) == CRC) return 0;
  else EEPROM_UpdateByte(EEPROM_BLOCK_CRC_DEBUG_DEFAULT, CRC);
  return 1;
}
//---------------Проверка установленного датчика температуры-----------------------
void checkTempSens(void) //проверка установленного датчика температуры
{
#if SENS_BME_ENABLE || SENS_PORT_ENABLE
  for (_timer_ms[TMR_SENS] = TEMP_START_TIME; _timer_ms[TMR_SENS];) dataUpdate(); //ждем
#endif
  for (sens.type = (SENS_ALL - 1); sens.type; sens.type--) { //перебираем все датчики температуры
    updateTemp(); //обновить показания температуры
    if (!sens.err) { //если найден датчик температуры
      _timer_ms[TMR_SENS] = TEMP_UPDATE_TIME; //установили таймаут
      return; //выходим
    }
  }
}
//-----------------Обновление предела удержания напряжения-------------------------
void updateTresholdADC(void) //обновление предела удержания напряжения
{
  hv_treshold = HV_ADC(GET_VCC(REFERENCE, vcc_adc)) + constrain(debugSettings.hvCorrect, -25, 25);
}
//------------------------------Обработка аналоговых входов-----------------------------------------
void analogUpdate(void) //обработка аналоговых входов
{
  if (!(ADCSRA & (1 << ADSC))) { //ждем окончания преобразования
    switch (ADMUX & 0x0F) {
#if GEN_ENABLE && GEN_FEEDBACK
      case ANALOG_DET_PIN: {
          static uint8_t adc_cycle; //циклы буфера усреднения
          static uint16_t adc_temp; //буфер усреднения

          adc_temp += ADCL | ((uint16_t)ADCH << 8); //добавляем значение в буфер
          if (++adc_cycle >= CYCLE_HV_CHECK) { //если буфер заполнен
            adc_temp /= CYCLE_HV_CHECK; //находим среднее значение
            if (adc_temp < hv_treshold) TCCR1A |= (0x01 << COM1A1); //включаем шим преобразователя
            else {
              TCCR1A &= ~(0x01 << COM1A1); //выключаем шим преобразователя
              CONV_OFF; //выключаем пин преобразователя
            }
            adc_temp = 0; //сбрасываем буфер усреднения
            adc_cycle = 0; //сбрасываем циклы буфера усреднения

            analogState |= 0x04; //установили флаг обновления АЦП обратной связи преобразователя
            ADMUX = 0; //сбросли признак чтения АЦП
          }
          else ADCSRA |= (1 << ADSC); //перезапускаем преобразование
        }
        break;
#endif
#if BTN_TYPE
      case ANALOG_BTN_PIN:
        btn.adc = ADCH; //записываем результат опроса
        ADMUX = 0; //сбросли признак чтения АЦП
        break;
#endif
#if LIGHT_SENS_ENABLE
      case ANALOG_LIGHT_PIN:
        adc_light = ADCH; //записываем результат опроса
        ADMUX = 0; //сбросли признак чтения АЦП
        break;
#endif
      default:
#if LIGHT_SENS_ENABLE
        if (analogState & 0x01) { //сенсор яркости
          analogState &= ~0x01; //сбросили флаг обновления АЦП сенсора яркости
          ADMUX = (0x01 << REFS0) | (0x01 << ADLAR) | ANALOG_LIGHT_PIN; //настройка мультиплексатора АЦП
          ADCSRA |= (1 << ADSC); //запускаем преобразование
          return; //выходим
        }
#endif
#if BTN_TYPE
        if (analogState & 0x02) { //аналоговые кнопки
          analogState &= ~0x02; //сбросили флаг обновления АЦП кнопок
          ADMUX = (0x01 << REFS0) | (0x01 << ADLAR) | ANALOG_BTN_PIN; //настройка мультиплексатора АЦП
          ADCSRA |= (1 << ADSC); //запускаем преобразование
          return; //выходим
        }
#endif
#if GEN_ENABLE && GEN_FEEDBACK
        if (analogState & 0x04) { //обратная связь
          analogState &= ~0x04; //сбросили флаг обновления АЦП обратной связи преобразователя
          ADMUX = (0x01 << REFS0) | ANALOG_DET_PIN; //настройка мультиплексатора АЦП
          ADCSRA |= (1 << ADSC); //запускаем преобразование
          return; //выходим
        }
#endif
        break;
    }
  }
}
//----------------------Чтение напряжения питания----------------------------------
void checkVCC(void) //чтение напряжения питания
{
  uint16_t temp = 0; //буфер замеров
  ADCSRA = (0x01 << ADEN) | (0x01 << ADPS0) | (0x01 << ADPS1) | (0x01 << ADPS2); //настройка АЦП пределитель 128
  ADMUX = (0x01 << REFS0) | (0x01 << MUX3) | (0x01 << MUX2) | (0x01 << MUX1); //выбор внешнего опорного + 1.1в
  _delay_ms(1000); //ждём пока напряжение успокоится
  for (uint8_t i = 0; i < CYCLE_VCC_CHECK; i++) {
    _delay_ms(5); //ждём пока опорное успокоится
    ADCSRA |= (1 << ADSC); //запускаем преобразование
    while (ADCSRA & (1 << ADSC)); //ждем окончания преобразования
    temp += ADCL | ((uint16_t)ADCH << 8); //записали результат
  }
  vcc_adc = temp / CYCLE_VCC_CHECK; //получаем напряжение питания

  if (GET_VCC(REFERENCE, vcc_adc) < MIN_VCC || GET_VCC(REFERENCE, vcc_adc) > MAX_VCC) SET_ERROR(VCC_ERROR); //устанвливаем ошибку по питанию

#if BTN_TYPE
  buttonUpdateADC(); //обновление пределов аналоговых кнопок

  ADMUX = (0x01 << REFS0) | (0x01 << ADLAR) | ANALOG_BTN_PIN; //настройка мультиплексатора АЦП
  ADCSRA |= (1 << ADSC); //запускаем преобразование
  while (ADCSRA & (1 << ADSC)); //ждем окончания преобразования
  btn.adc = ADCH; //записываем результат опроса
#endif
#if GEN_ENABLE && GEN_FEEDBACK
  ADMUX = (0x01 << REFS0) | ANALOG_DET_PIN; //настройка мультиплексатора АЦП
#endif

#if (GEN_ENABLE && GEN_FEEDBACK) || BTN_TYPE || LIGHT_SENS_ENABLE
  ADCSRA = (0x01 << ADEN) | (0x01 << ADPS0) | (0x01 << ADPS2); //настройка АЦП пределитель 32
  ADCSRA |= (0x01 << ADSC); //запускаем преобразование
#endif
}
//-----------------Обновление пределов аналоговых кнопок----------------------------
void buttonUpdateADC(void) //обновление пределов аналоговых кнопок
{
  int16_t temp = GET_ADC(GET_VCC(REFERENCE, vcc_adc), R_COEF(BTN_R_LOW, BTN_SET_R_HIGH));
  btn.setMin = constrain(temp - BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE);
  btn.setMax = constrain(temp + BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE);

  temp = GET_ADC(GET_VCC(REFERENCE, vcc_adc), R_COEF(BTN_R_LOW, BTN_LEFT_R_HIGH));
  btn.leftMin = constrain(temp - BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE);
  btn.leftMax = constrain(temp + BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE);

  temp = GET_ADC(GET_VCC(REFERENCE, vcc_adc), R_COEF(BTN_R_LOW, BTN_RIGHT_R_HIGH));
  btn.rightMin = constrain(temp - BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE);
  btn.rightMax = constrain(temp + BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE);

#if (BTN_ADD_TYPE == 2)
  temp = GET_ADC(GET_VCC(REFERENCE, vcc_adc), R_COEF(BTN_R_LOW, BTN_ADD_R_HIGH));
  btn.addMin = constrain(temp - BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE);
  btn.addMax = constrain(temp + BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE);
#endif
}
//-----------------------Проверка аналоговой кнопки--------------------------------
inline boolean buttonCheckADC(uint8_t minADC, uint8_t maxADC) //проверка аналоговой кнопки
{
  return !(minADC < btn.adc && btn.adc <= maxADC); //возвращаем результат опроса
}
//---------------------------Проверка кнопок---------------------------------------
inline uint8_t buttonState(void) //проверка кнопок
{
  uint8_t button = btn.state; //запоминаем текущее состояние кнопки
  btn.state = 0; //сбрасываем текущее состояние кнопки
  return button; //возвращаем состояние кнопки
}
//--------------------------Обновление кнопок--------------------------------------
inline uint8_t buttonStateUpdate(void) //обновление кнопок
{
  static boolean btn_check; //флаг разрешения опроса кнопки
  static boolean btn_state; //флаг текущего состояния кнопки
  static uint8_t btn_switch; //флаг мультиплексатора кнопок
  static uint16_t btn_tmr; //таймер тиков обработки кнопок

#if BTN_TYPE
  analogState |= 0x02; //устанавливаем флаг обновления АЦП кнопок
#endif

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
#if BTN_ADD_TYPE
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
#if BTN_ADD_TYPE
    case 4: btn_state = ADD_CHK; break; //опрашиваем дополнительную клавишу
#endif
  }

  switch (btn_state) { //переключаемся в зависимости от состояния клавиши
    case 0:
      if (btn_check) { //если разрешена провекрка кнопки
        if (++btn_tmr > BTN_HOLD_TICK) { //если таймер больше длительности удержания кнопки
          btn_tmr = BTN_GIST_TICK; //сбрасываем таймер на антидребезг
          btn_check = 0; //запрещем проврку кнопки
#if PLAYER_TYPE
          playerStop(); //сброс воспроизведения плеера
#else
          melodyStop(); //сброс воспроизведения мелодии
#endif
          switch (btn_switch) { //переключаемся в зависимости от состояния мультиопроса
            case 1: return SET_KEY_HOLD; //возвращаем удержание средней кнопки
            case 2: return LEFT_KEY_HOLD; //возвращаем удержание левой кнопки
            case 3: return RIGHT_KEY_HOLD; //возвращаем удержание правой кнопки
#if BTN_ADD_TYPE
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
#if PLAYER_TYPE
        playerStop(); //сброс воспроизведения плеера
#else
        if (mainSettings.knockSound) buzz_pulse(KNOCK_SOUND_FREQ, KNOCK_SOUND_TIME); //щелчок пищалкой
        melodyStop(); //сброс воспроизведения мелодии
#endif
        switch (btn_switch) { //переключаемся в зависимости от состояния мультиопроса
          case 1: return SET_KEY_PRESS; //возвращаем клик средней кнопкой
          case 2: return LEFT_KEY_PRESS; //возвращаем клик левой кнопкой
          case 3: return RIGHT_KEY_PRESS; //возвращаем клик правой кнопкой
#if BTN_ADD_TYPE
          case 4: return ADD_KEY_PRESS; //возвращаем клик дополнительной кнопкой
#endif
        }
      }
      else if (!btn_tmr) {
        btn_check = 1; //разрешаем проврку кнопки
        btn_switch = 0; //сбрасываем мультиплексатор кнопок
      }
      else btn_tmr--; //убираем дребезг
      break;
  }

#if IR_PORT_ENABLE
  if (irCommand && !irDisable) { //если пришла команда и управление ИК не заблокировано
    uint8_t command = irCommand; //копируем команду
    irCommand = 0; //сбрасываем команду
    for (uint8_t button = 0; button < sizeof(debugSettings.irButtons); button++) { //ищем номер кнопки
      if (command == debugSettings.irButtons[button]) { //если команда совпала
#if PLAYER_TYPE
        playerStop(); //сброс воспроизведения плеера
#else
        melodyStop(); //сброс воспроизведения мелодии
#endif
        return button + 1; //возвращаем номер кнопки
      }
    }
  }
#endif

  return KEY_NULL; //кнопка не нажата
}
//------------------Проверка модуля часов реального времени-------------------------
void checkRTC(void) //проверка модуля часов реального времени
{
  disable32K(); //отключение вывода 32K

#if SQW_PORT_ENABLE
  setSQW(); //установка SQW на 1Гц

  EICRA = (0x01 << ISC01); //настраиваем внешнее прерывание по спаду импульса на INT0
  EIFR |= (0x01 << INTF0); //сбрасываем флаг прерывания INT0

  for (_timer_ms[TMR_MS] = SQW_TEST_TIME; !(EIFR & (0x01 << INTF0)) && _timer_ms[TMR_MS];) { //ждем сигнала от SQW
    for (; tick_ms; tick_ms--) { //если был тик, обрабатываем данные
      if (_timer_ms[TMR_MS] > MS_PERIOD) _timer_ms[TMR_MS] -= MS_PERIOD; //если таймер больше периода
      else if (_timer_ms[TMR_MS]) _timer_ms[TMR_MS] = 0; //иначе сбрасываем таймер
    }
  }
#endif

  if (getTime()) { //считываем время из RTC
    EEPROM_ReadBlock((uint16_t)&RTC, EEPROM_BLOCK_TIME, sizeof(RTC)); //считываем дату и время из памяти
    sendTime(); //отправить время в RTC
  }

#if SQW_PORT_ENABLE
  if (EIFR & (0x01 << INTF0)) { //если был сигнал с SQW
    EIFR |= (0x01 << INTF0); //сбрасываем флаг прерывания INT0
    EIMSK = (0x01 << INT0); //разрешаем внешнее прерывание INT0
  }
  else SET_ERROR(SQW_ERROR); //иначе выдаем ошибку
#endif
}
//-----------------------------Проверка ошибок-------------------------------------
void checkErrors(void) //проверка ошибок
{
  uint8_t _error_reg = EEPROM_ReadByte(EEPROM_BLOCK_ERROR); //прочитали регистр ошибок
  for (uint8_t i = 0; i < 8; i++) { //проверяем весь регистр
    if (_error_reg & (0x01 << i)) { //если стоит флаг ошибки
      indiPrintNum(i + 1, 0, 4, 0); //вывод ошибки
#if PLAYER_TYPE
      playerSetTrack(PLAYER_ERROR_SOUND, PLAYER_GENERAL_FOLDER); //воспроизводим трек ошибки
      playerSpeakNumber(i + 1); //воспроизводим номер ошибки
#else
      melodyPlay(i, SOUND_LINK(error_sound), REPLAY_ONCE); //воспроизводим мелодию
#endif
      for (_timer_ms[TMR_MS] = ERROR_SHOW_TIME; !buttonState() && _timer_ms[TMR_MS];) dataUpdate(); //обработка данных
    }
  }
  updateByte(0x00, EEPROM_BLOCK_ERROR, EEPROM_BLOCK_CRC_ERROR); //сбросили ошибки
}
//---------------------------Проверка системы---------------------------------------
void test_system(void) //проверка системы
{
  indiPrintNum(CONVERT_NUM(FIRMWARE_VERSION), 0); //отрисовываем версию прошивки
#if PLAYER_TYPE
  playerSetTrackNow(PLAYER_FIRMWARE_SOUND, PLAYER_GENERAL_FOLDER);
  playerSpeakNumber(CONVERT_CHAR(FIRMWARE_VERSION[0]));
  playerSpeakNumber(CONVERT_CHAR(FIRMWARE_VERSION[2]));
  playerSpeakNumber(CONVERT_CHAR(FIRMWARE_VERSION[4]));
#endif
  for (_timer_ms[TMR_MS] = TEST_FIRMWARE_TIME; _timer_ms[TMR_MS] && !buttonState();) dataUpdate(); //ждем
#if PLAYER_TYPE
  playerSetTrackNow(PLAYER_TEST_SOUND, PLAYER_GENERAL_FOLDER);
#endif

#if BACKL_TYPE != 2
  backlSetBright(TEST_BACKL_BRIGHT); //устанавливаем максимальную яркость
#endif
  indiSetBright(TEST_INDI_BRIGHT); //установка яркости индикаторов
  dotSetBright(TEST_DOT_BRIGHT); //установка яркости точек
  while (1) {
    for (uint8_t indi = 0; indi < LAMP_NUM; indi++) {
      indiClr(); //очистка индикаторов
#if DOTS_PORT_ENABLE
      indiClrDots(); //выключаем разделительные точки
      indiSetDots(indi); //включаем разделителную точку
#endif
#if BACKL_TYPE == 2
#if TEST_BACKL_REVERSE
      setLedBright((LAMP_NUM - 1) - indi, TEST_BACKL_BRIGHT); //включаем светодиод
#else
      setLedBright(indi, TEST_BACKL_BRIGHT); //включаем светодиод
#endif
#endif
      for (uint8_t digit = 0; digit < 10; digit++) {
        indiPrintNum(digit, indi); //отрисовываем цифру
#if BACKL_TYPE == 2
#if TEST_BACKL_REVERSE
        setLedHue((LAMP_NUM - 1) - indi, digit * 25); //устанавливаем статичный цвет
#else
        setLedHue(indi, digit * 25); //устанавливаем статичный цвет
#endif
        showLeds(); //отрисовка светодиодов
#endif
#if !PLAYER_TYPE
        buzz_pulse(TEST_FREQ_STEP + (digit * TEST_FREQ_STEP), TEST_LAMP_TIME); //перебор частот
#endif
        for (_timer_ms[TMR_MS] = TEST_LAMP_TIME; _timer_ms[TMR_MS];) { //ждем
          dataUpdate(); //обработка данных
          if (buttonState()) return; //возврат если нажата кнопка
        }
      }
#if BACKL_TYPE == 2
      setLedBright(0); //выключаем светодиоды
#endif
    }
  }
}
//-----------------------------Проверка пароля------------------------------------
boolean check_pass(void) //проверка пароля
{
  boolean blink_data = 0; //мигание сигментами
  uint8_t cur_indi = 0; //текущий индикатор
  uint8_t time_out = 0; //таймер авто выхода
  uint8_t attempts_pass = 0; //попытки ввода пароля
  uint16_t entry_pass = 0; //введеный пароль

  dotSetBright(0); //выключаем точки
  indiSetBright(30); //устанавливаем максимальную яркость индикаторов

#if PLAYER_TYPE
  playerSetTrack(PLAYER_DEBUG_SOUND, PLAYER_GENERAL_FOLDER);
#endif

  while (1) {
    dataUpdate(); //обработка данных

    if (!secUpd) {
      secUpd = 1; //сбросили флаг
      if (++time_out >= DEBUG_TIMEOUT) return 0; //если время вышло то выходим
    }

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = DEBUG_PASS_BLINK_TIME; //устанавливаем таймер

      indiPrintNum(entry_pass, (LAMP_NUM / 2 - 2), 4, 0); //вывод пароля
      if (blink_data) indiClr(cur_indi + (LAMP_NUM / 2 - 2)); //очистка индикатора

      blink_data = !blink_data; //мигаем индикатором
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (cur_indi) {
          case 0: if (((entry_pass % 10000) / 1000) > 0) entry_pass -= 1000; else entry_pass += 9000; break; //первый разряд
          case 1: if (((entry_pass % 1000) / 100) > 0) entry_pass -= 100; else entry_pass += 900; break; //второй разряд
          case 2: if (((entry_pass % 100) / 10) > 0) entry_pass -= 10; else entry_pass += 90; break; //третий разряд
          case 3: if ((entry_pass % 10) > 0) entry_pass -= 1; else entry_pass += 9; break; //четвертый разряд
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (cur_indi) {
          case 0: if (((entry_pass % 10000) / 1000) < 9) entry_pass += 1000; else entry_pass -= 9000; break; //первый разряд
          case 1: if (((entry_pass % 1000) / 100) < 9) entry_pass += 100; else entry_pass -= 900; break; //второй разряд
          case 2: if (((entry_pass % 100) / 10) < 9) entry_pass += 10; else entry_pass -= 90; break; //третий разряд
          case 3: if ((entry_pass % 10) < 9) entry_pass += 1; else entry_pass -= 9; break; //четвертый разряд
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        if (cur_indi < 3) cur_indi++; else cur_indi = 0; //переключаем разряды
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        if (entry_pass == DEBUG_PASS) return 1; //если пароль совпал
        cur_indi = 0; //сбросили текущий индикатор
        entry_pass = 0; //сбросили введеный пароль
#if PLAYER_TYPE
        playerSetTrack(PLAYER_PASS_SOUND, PLAYER_GENERAL_FOLDER);
#else
        melodyPlay(SOUND_PASS_ERROR, SOUND_LINK(general_sound), REPLAY_ONCE); //сигнал ошибки ввода пароля
#endif
        if (++attempts_pass >= DEBUG_PASS_ATTEMPTS) return 0; //превышено количество попыток ввода пароля
        break; //пароль не совпал
    }
  }
  return 0;
}
//---------------------Воспроизвести пункт отладки-------------------------
void debugPlayItem(uint8_t _menu) //воспроизвести пункт отладки
{
  playerSetTrackNow(PLAYER_DEBUG_MENU_START + ((!_menu && EIMSK) ? 0 : _menu + 1), PLAYER_MENU_FOLDER); //воспроизводим название пункта отладки
}
//-----------------------------Отладка------------------------------------
void debug_menu(void) //отладка
{
  boolean set = 0; //режим настройки
  int8_t aging = 0; //буфер регистра старения
  uint8_t cur_mode = 0; //текущий режим
#if IR_PORT_ENABLE
  uint8_t cur_button = 0; //текущая кнопка пульта
#endif
#if LIGHT_SENS_ENABLE
  uint8_t temp_min = 255;
  uint8_t temp_max = 0;
#endif

#if PLAYER_TYPE
  debugPlayItem(0);
#endif

  dotSetBright(0); //выключаем точки
  indiSetBright(30); //устанавливаем максимальную яркость индикаторов

  //настройки
  while (1) {
    dataUpdate(); //обработка данных

    if (!secUpd) {
      secUpd = 1; //сбрасываем флаг

      indiClr(); //очистка индикаторов
      switch (set) {
        case 0:
          indiPrintNum(cur_mode + 1, (LAMP_NUM / 2 - 1), 2, 0); //вывод режима
          break;
        case 1:
          indiPrintNum(cur_mode + 1, 5); //режим
          switch (cur_mode) {
            case DEB_TIME_CORRECT:
              if (EIMSK) indiPrintNum((aging < 0) ? (uint8_t) - aging : (uint8_t)aging, 0, (aging > 0) ? 4 : 0); //выводим коррекцию DS3231
              else indiPrintNum(debugSettings.timePeriod, 0); //выводим коррекцию внутреннего таймера
              break;
            case DEB_DEFAULT_MIN_PWM: indiPrintNum(debugSettings.min_pwm, 0); break; //выводим минимальный шим
            case DEB_DEFAULT_MAX_PWM: indiPrintNum(debugSettings.max_pwm, 0); break; //выводим максимальный шим
#if GEN_ENABLE && GEN_FEEDBACK
            case DEB_HV_ADC: indiPrintNum(hv_treshold, 0); break; //выводим корекцию напряжения
#endif
#if IR_PORT_ENABLE
            case DEB_IR_BUTTONS: //програмирование кнопок
              indiPrintNum(cur_button + 1, 0); //выводим номер кнопки пульта
              indiPrintNum(debugSettings.irButtons[cur_button], 1, 3); //выводим код кнопки пульта
              break;
#endif
#if LIGHT_SENS_ENABLE
            case DEB_LIGHT_SENS: //калибровка датчика освещения
              indiPrintNum(adc_light, 1, 3); //выводим значение АЦП датчика освещения
              break;
#endif
          }
          break;
      }
    }

#if LIGHT_SENS_ENABLE || IR_PORT_ENABLE
    if (set) {
      switch (cur_mode) {
#if IR_PORT_ENABLE
        case DEB_IR_BUTTONS: //програмирование кнопок
          if (irDisable && irCommand) { //если управление ИК заблокировано и пришла новая команда
            debugSettings.irButtons[cur_button] = irCommand; //записываем команду в массив
            irCommand = 0; //сбрасываем команду
            secUpd = 0; //обновление экрана
          }
          break;
#endif
#if LIGHT_SENS_ENABLE
        case DEB_LIGHT_SENS: //калибровка датчика освещения
          if (!_timer_ms[TMR_LIGHT]) {
            if (temp_min > adc_light) temp_min = adc_light;
            if (temp_max < adc_light) temp_max = adc_light;
            analogState |= 0x01; //установили флаг обновления АЦП сенсора яркости
            _timer_ms[TMR_LIGHT] = LIGHT_SENS_TIME;
            secUpd = 0; //обновление экрана
          }
          break;
#endif
      }
    }
#endif
    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (set) {
          case 0:
            if (cur_mode > 0) cur_mode--;
            else cur_mode = DEB_MAX_ITEMS - 1;
#if PLAYER_TYPE
            debugPlayItem(cur_mode);
#endif
            break;
          case 1:
            switch (cur_mode) {
              case DEB_TIME_CORRECT: //коррекция хода
                switch (EIMSK) {
                  case 0: if (debugSettings.timePeriod > US_PERIOD_MIN) debugSettings.timePeriod--; else debugSettings.timePeriod = US_PERIOD_MAX; break;
                  case 1: if (aging > -127) aging--; else aging = 127; break;
                }
                break;
              case DEB_DEFAULT_MIN_PWM: //коррекция минимального значения шим
                if (debugSettings.min_pwm > 100) debugSettings.min_pwm -= 5; //минимальное значение шим
                indiChangeCoef(); //обновление коэффициента Linear Advance
                break;
              case DEB_DEFAULT_MAX_PWM: //коррекция максимального значения шим
                if (debugSettings.max_pwm > 150) debugSettings.max_pwm -= 5; //максимальное значение шим
                indiChangeCoef(); //обновление коэффициента Linear Advance
                break;
#if GEN_ENABLE && GEN_FEEDBACK
              case DEB_HV_ADC: //коррекция значения ацп преобразователя
                if (debugSettings.hvCorrect > -30) debugSettings.hvCorrect--; //значение ацп преобразователя
                updateTresholdADC(); //обновление предела удержания напряжения
                break;
#endif
#if IR_PORT_ENABLE
              case DEB_IR_BUTTONS: //програмирование кнопок
                if (cur_button) cur_button--;
                break;
#endif
            }
            break;
        }
        secUpd = 0; //обновление экрана
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (set) {
          case 0:
            if (cur_mode < (DEB_MAX_ITEMS - 1)) cur_mode++;
            else cur_mode = 0;
#if PLAYER_TYPE
            debugPlayItem(cur_mode);
#endif
            break;
          case 1:
            switch (cur_mode) {
              case DEB_TIME_CORRECT: //коррекция хода
                switch (EIMSK) {
                  case 0: if (debugSettings.timePeriod < US_PERIOD_MAX) debugSettings.timePeriod++; else debugSettings.timePeriod = US_PERIOD_MIN; break;
                  case 1: if (aging < 127) aging++; else aging = -127; break;
                }
                break;
              case DEB_DEFAULT_MIN_PWM: //коррекция минимального значения шим
                if (debugSettings.min_pwm < 150) debugSettings.min_pwm += 5; //минимальное значение шим
                indiChangeCoef(); //обновление коэффициента Linear Advance
                break;
              case DEB_DEFAULT_MAX_PWM: //коррекция максимального значения шим
                if (debugSettings.max_pwm < 200) debugSettings.max_pwm += 5; //максимальное значение шим
                indiChangeCoef(); //обновление коэффициента Linear Advance
                break;
#if GEN_ENABLE && GEN_FEEDBACK
              case DEB_HV_ADC: //коррекция значения ацп преобразователя
                if (debugSettings.hvCorrect < 30) debugSettings.hvCorrect++; //значение ацп преобразователя
                updateTresholdADC(); //обновление предела удержания напряжения
                break;
#endif
#if IR_PORT_ENABLE
              case DEB_IR_BUTTONS: //програмирование кнопок
                if (cur_button < (sizeof(debugSettings.irButtons) - 1)) cur_button++;
                break;
#endif
            }
            break;
        }
        secUpd = 0; //обновление экрана
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        switch (cur_mode) {
#if !GEN_ENABLE || !GEN_FEEDBACK
          case DEB_HV_ADC: break; //коррекция значения ацп преобразователя
#endif
#if !IR_PORT_ENABLE
          case DEB_IR_BUTTONS: break; //програмирование кнопок
#endif
#if !LIGHT_SENS_ENABLE
          case DEB_LIGHT_SENS: break; //калибровка датчика освещения
#endif
          default: set = !set; break;
        }
        if (set) {
          dotSetBright(DEFAULT_DOT_BRIGHT); //включаем точки
          switch (cur_mode) {
            case DEB_TIME_CORRECT:
              if (EIMSK) aging = readAgingRTC(); //чтение коррекции хода
              break;
            case DEB_DEFAULT_MIN_PWM: indiSetBright(1); break; //минимальное значение шим
            case DEB_DEFAULT_MAX_PWM: indiSetBright(30); break; //максимальное значение шим
#if IR_PORT_ENABLE
            case DEB_IR_BUTTONS: //програмирование кнопок
              irDisable = 1;
              cur_button = 0;
              break;
#endif
#if LIGHT_SENS_ENABLE
            case DEB_LIGHT_SENS: //калибровка датчика освещения
              temp_min = 255;
              temp_max = 0;
              break;
#endif
            case DEB_RESET:
              set = 0; //сбросили на начальный уровень меню
              cur_mode = 0; //перешли на первый пункт меню
              if (check_pass()) { //подтверждение паролем
                debugSettings.timePeriod = US_PERIOD; //коррекция хода внутреннего осцилятора
                debugSettings.min_pwm = DEFAULT_MIN_PWM; //минимальное значение шим
                debugSettings.max_pwm = DEFAULT_MAX_PWM; //максимальное значение шим
                indiChangeCoef(); //обновление коэффициента Linear Advance
#if GEN_ENABLE && GEN_FEEDBACK
                debugSettings.hvCorrect = 0; //коррекция напряжения преобразователя
                updateTresholdADC(); //обновление предела удержания напряжения
#endif
#if PLAYER_TYPE
                playerSetTrack(PLAYER_RESET_SOUND, PLAYER_GENERAL_FOLDER);
#else
                melodyPlay(SOUND_RESET_SETTINGS, SOUND_LINK(general_sound), REPLAY_ONCE); //сигнал сброса настроек отладки
#endif
              }
              break;
          }
        }
        else {
#if LIGHT_SENS_ENABLE || IR_PORT_ENABLE
          switch (cur_mode) {
#if IR_PORT_ENABLE
            case DEB_IR_BUTTONS: //програмирование кнопок
              irDisable = 0;
              break;
#endif
#if LIGHT_SENS_ENABLE
            case DEB_LIGHT_SENS: { //калибровка датчика освещения
                uint8_t temp_mid = ((temp_max - temp_min) / 2) + temp_min;
                debugSettings.min_light = temp_mid - LIGHT_SENS_GIST;
                debugSettings.max_light = temp_mid + LIGHT_SENS_GIST;
              }
              break;
#endif
          }
#endif

          dotSetBright(0); //выключаем точки
          indiSetBright(30); //устанавливаем максимальную яркость индикаторов
        }
        secUpd = 0; //обновление экрана
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
#if IR_PORT_ENABLE
        irDisable = 0;
#endif
        if (EIMSK) writeAgingRTC((uint8_t)aging); //запись коррекции хода
        updateData((uint8_t*)&debugSettings, sizeof(debugSettings), EEPROM_BLOCK_SETTINGS_DEBUG, EEPROM_BLOCK_CRC_DEBUG); //записываем настройки отладки в память
        return;
    }
  }
}
//------------------------------Генерация частот бузера------------------------------------------------
void buzz_pulse(uint16_t freq, uint16_t time) //генерация частоты бузера (частота 10..10000, длительность мс.)
{
  cnt_puls = ((uint32_t)freq * (uint32_t)time) / 500; //пересчитываем частоту и время в циклы таймера
  cnt_freq = tmr_score = (1000000 / freq); //пересчитываем частоту в циклы полуволны
  OCR2B = 255; //устанавливаем COMB в начало
  TIMSK2 |= (0x01 << OCIE2B); //запускаем таймер
}
//---------------------------------Воспроизведение мелодии-----------------------------------------------
void melodyUpdate(void) //воспроизведение мелодии
{
  if (sound.replay && !_timer_ms[TMR_PLAYER]) { //если пришло время
    buzz_pulse(pgm_read_word(sound.link + sound.semp), pgm_read_word(sound.link + sound.semp + 2)); //запускаем звук с задоной частотой и временем
    _timer_ms[TMR_PLAYER] = pgm_read_word(sound.link + sound.semp + 4); //устанавливаем паузу перед воспроизведением нового звука
    if ((sound.semp += 6) >= sound.size) { //переключаем на следующий семпл
      if (sound.replay == REPLAY_ONCE) melodyStop(); //если повтор выключен то остановка воспроизведения мелодии
      sound.semp = 0; //сбросили семпл
    }
  }
}
//-----------------------------Запуск воспроизведения мелодии--------------------------------------------
void melodyPlay(uint8_t melody, uint16_t link, uint8_t replay) //запуск воспроизведения мелодии
{
  sound.semp = 0; //сбросили позицию семпла
  sound.replay = replay; //установили повтор
  sound.link = pgm_read_word(link + (melody << 2)); //установили ссылку
  sound.size = pgm_read_word(link + (melody << 2) + 2); //установили размер
  _timer_ms[TMR_PLAYER] = 0; //сбросили таймер
}
//---------------------------Остановка воспроизведения мелодии-------------------------------------------
void melodyStop(void) //остановка воспроизведения мелодии
{
  sound.replay = REPLAY_STOP; //сбросили воспроизведение
  _timer_ms[TMR_PLAYER] = 0; //сбросили таймер
}
//---------------------------------Инициализация будильника----------------------------------------------
void alarmInit(void) //инициализация будильника
{
  if (!alarmsNum) newAlarm(); //создать новый будильник
  else if (alarmsNum > 1) { //если будильников в памяти больше одного
    alarmsNum = 1; //оставляем один будильник
    updateByte(alarmsNum, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
  }
}
//----------------------------------Сброс будильника---------------------------------------------------------
void alarmReset(void) //сброс будильника
{
  if (alarmRead(alarm - 1, ALARM_MODE) == 1) EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + ((alarm - 1) * ALARM_MAX_ARR) + ALARM_MODE, 0); //если был установлен режим одиночный то выключаем будильник
  checkAlarms(); //проверка будильников
  _timer_sec[TMR_ALM] = 0; //сбрасываем таймер отключения будильника
  _timer_sec[TMR_ALM_WAINT] = 0; //сбрасываем таймер ожидания повторного включения тревоги
  _timer_sec[TMR_ALM_SOUND] = 0; //сбрасываем таймер отключения звука
  alarmWaint = 0; //сбрасываем флаг ожидания
  alarm = 0; //сбрасываем флаг тревоги
}
//----------------------------------Получить основные данные будильника---------------------------------------------------------
uint8_t alarmRead(uint8_t almNum, uint8_t almData) //получить основные данные будильника
{
  return EEPROM_ReadByte(EEPROM_BLOCK_ALARM_DATA + ((uint16_t)almNum * ALARM_MAX_ARR) + almData); //возвращаем запрошеный байт
}
//----------------------------------Получить блок основных данных будильника---------------------------------------------------------
void alarmReadBlock(uint8_t almNum, uint8_t* data) //получить блок основных данных будильника
{
  uint16_t curCell = (uint16_t)(almNum - 1) * ALARM_MAX_ARR;
  for (uint8_t i = 0; i < ALARM_MAX_ARR; i++) data[i] = (almNum) ? EEPROM_ReadByte(EEPROM_BLOCK_ALARM_DATA + curCell + i) : 0; //считываем блок данных
}
//----------------------------------Записать блок основных данных будильника---------------------------------------------------------
void alarmWriteBlock(uint8_t almNum, uint8_t* data) //записать блок основных данных будильника
{
  if (!almNum) return; //если нет ни одного будильника то выходим
  uint16_t curCell = (uint16_t)(almNum - 1) * ALARM_MAX_ARR;
  for (uint8_t i = 0; i < ALARM_MAX_ARR; i++) EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + curCell + i, data[i]); //записываем блок данных
}
//---------------------Создать новый будильник---------------------------------------------------------
void newAlarm(void) //создать новый будильник
{
  if (alarmsNum < MAX_ALARMS) { //если новый будильник меньше максимума
    uint16_t newCell = EEPROM_BLOCK_ALARM_DATA + ((uint16_t)alarmsNum * ALARM_MAX_ARR);
    EEPROM_UpdateByte(newCell + ALARM_HOURS, DEFAULT_ALARM_TIME_HH); //устанавливаем час по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_MINS, DEFAULT_ALARM_TIME_MM); //устанавливаем минуты по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_MODE, DEFAULT_ALARM_MODE); //устанавливаем режим по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_DAYS, 0); //устанавливаем дни недели по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_SOUND, DEFAULT_ALARM_SOUND); //устанавливаем мелодию по умолчанию
    updateByte(++alarmsNum, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
  }
}
//---------------------Удалить будильник---------------------------------------------------------
void delAlarm(uint8_t alarm) //удалить будильник
{
  if (alarmsNum) { //если будильник доступен
    for (uint8_t start = alarm; start < alarmsNum; start++) { //перезаписываем массив будильников
      uint16_t oldCell = EEPROM_BLOCK_ALARM_DATA + ((uint16_t)start * ALARM_MAX_ARR);
      uint16_t newCell = EEPROM_BLOCK_ALARM_DATA + ((uint16_t)(start - 1) * ALARM_MAX_ARR);
      for (uint8_t block = 0; block < ALARM_MAX_ARR; block++) EEPROM_UpdateByte(newCell + block, EEPROM_ReadByte(oldCell + block));
    }
    updateByte(--alarmsNum, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
  }
}
//-----------------------------------Проверка будильников-------------------------------------------------
void checkAlarms(void) //проверка будильников
{
  alarmEnabled = 0; //сбрасываем флаг включенного будильника
  for (uint8_t alm = 0; alm < alarmsNum; alm++) { //опрашиваем все будильники
    if (alarmRead(alm, ALARM_MODE)) { //если будильник включен
      alarmEnabled = 1; //устанавливаем флаг включенного будильника
      if (RTC.h == alarmRead(alm, ALARM_HOURS) && RTC.m == alarmRead(alm, ALARM_MINS) && (alarmRead(alm, ALARM_MODE) < 3 || (alarmRead(alm, ALARM_MODE) == 3 && RTC.DW < 6) || (alarmRead(alm, ALARM_DAYS) & (0x01 << RTC.DW)))) {
        if (!alarm) { //если тревога не активна
          alarm = alm + 1; //устанавливаем флаг тревоги
          _timer_sec[TMR_ALM] = (uint16_t)(ALARM_TIMEOUT * 60); //установили таймер таймаута будильника
          _timer_sec[TMR_ALM_SOUND] = (uint16_t)(ALARM_TIMEOUT_SOUND * 60); //установили таймер таймаута звука будильника
          return; //выходим
        }
      }
    }
  }
}
//-------------------------------Обновление данных будильников---------------------------------------------
void alarmDataUpdate(void) //обновление данных будильников
{
  if (alarm) { //если тревога активна
    if (!_timer_sec[TMR_ALM]) { //если пришло время выключить будильник
      alarmReset(); //сброс будильника
      return; //выходим
    }

    if (ALARM_WAINT && alarmWaint) { //если будильник в режиме ожидания
      if (!_timer_sec[TMR_ALM_WAINT]) { //если пришло время повторно включить звук
        _timer_sec[TMR_ALM_SOUND] = (uint16_t)(ALARM_TIMEOUT_SOUND * 60);
        alarmWaint = 0; //сбрасываем флаг ожидания
      }
    }
    else if (ALARM_TIMEOUT_SOUND) { //если таймаут тревоги включен
      if (!_timer_sec[TMR_ALM_SOUND]) { //если пришло время выключить тревогу
        if (ALARM_WAINT) { //если время ожидания включено
          _timer_sec[TMR_ALM_WAINT] = (uint16_t)(ALARM_WAINT * 60);
          alarmWaint = 1; //устанавливаем флаг ожидания тревоги
        }
        else alarmReset(); //сброс будильника
      }
    }
  }
}
//----------------------------------Тревога будильника---------------------------------------------------------
void alarmWarn(void) //тревога будильника
{
  if (alarm && !alarmWaint) { //если флаг установлен флаг тревоги и флаг ожидания очещен
    boolean blink_data = 0; //флаг мигания индикаторами

#if PLAYER_TYPE
    playerStop(); //сброс позиции мелодии
#else
    melodyPlay(alarmRead(alarm - 1, ALARM_SOUND), SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif

    while (1) {
      dataUpdate(); //обработка данных

#if PLAYER_TYPE
      if (!playerWriteStatus()) playerSetTrack(PLAYER_ALARM_START + alarmRead(alarm - 1, ALARM_SOUND), PLAYER_ALARM_FOLDER); //воспроизводим мелодию
#endif

      if (!alarm || alarmWaint) { //если тревога сброшена
#if PLAYER_TYPE
        playerStop(); //сброс позиции мелодии
#else
        melodyStop(); //сброс позиции мелодии
#endif
        animsReset(); //сброс анимаций
        return; //выходим
      }

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

      switch (buttonState()) {
        case LEFT_KEY_PRESS: //клик левой кнопкой
        case RIGHT_KEY_PRESS: //клик правой кнопкой
        case SET_KEY_PRESS: //клик средней кнопкой
        case ADD_KEY_PRESS: //клик дополнительной кнопкой
          if (ALARM_WAINT) { //если есть время ожидания
            alarmWaint = 1; //устанавливаем флаг ожидания
            _timer_sec[TMR_ALM_WAINT] = (uint16_t)(ALARM_WAINT * 60);
            _timer_sec[TMR_ALM_SOUND] = 0;
#if PLAYER_TYPE
            playerSetTrackNow(PLAYER_ALARM_WAIT_SOUND, PLAYER_GENERAL_FOLDER); //звук ожидания будильника
#else
            melodyPlay(SOUND_ALARM_WAINT, SOUND_LINK(general_sound), REPLAY_ONCE); //звук ожидания будильника
#endif
          }
          else {
#if PLAYER_TYPE
            playerSetTrackNow(PLAYER_ALARM_DISABLE_SOUND, PLAYER_GENERAL_FOLDER); //звук выключения будильника
#else
            melodyPlay(SOUND_ALARM_DISABLE, SOUND_LINK(general_sound), REPLAY_ONCE); //звук выключения будильника
#endif
            alarmReset(); //сброс будильника
          }
          animsReset(); //сброс анимаций
          return; //выходим

        case LEFT_KEY_HOLD: //удержание левой кнопки
        case RIGHT_KEY_HOLD: //удержание правой кнопки
        case SET_KEY_HOLD: //удержание средней кнопки
        case ADD_KEY_HOLD: //удержание дополнительной кнопки
#if PLAYER_TYPE
          playerSetTrackNow(PLAYER_ALARM_DISABLE_SOUND, PLAYER_GENERAL_FOLDER); //звук выключения будильника
#else
          melodyPlay(SOUND_ALARM_DISABLE, SOUND_LINK(general_sound), REPLAY_ONCE); //звук выключения будильника
#endif
          alarmReset(); //сброс будильника
          animsReset(); //сброс анимаций
          return; //выходим
      }
    }
  }
}
//----------------------------------Обработка данных------------------------------------------------
void dataUpdate(void) //обработка данных
{
  static uint16_t timeClock; //счетчик реального времени
  static uint16_t timerCorrect; //остаток для коррекции времени
  static uint16_t timerSQW = SQW_MIN_TIME; //таймер контроля сигнала SQW
#if BACKL_TYPE == 2
  backlEffect(); //анимация подсветки
#else
  backlFlash(); //"дыхание" подсветки
#endif

#if PLAYER_TYPE
  playerUpdate(); //обработка плеера
#if PLAYER_TYPE == 2
#if AMP_PORT_ENABLE
  if (!_timer_ms[TMR_PLAYER]) //если таймер истек
#endif
    readerUpdate(); //обработка SD плеера
#endif
#else
  melodyUpdate(); //обработка мелодий
#endif

#if (GEN_ENABLE && GEN_FEEDBACK) || BTN_TYPE || LIGHT_SENS_ENABLE
  analogUpdate(); //обработка аналоговых входов
#endif

  for (; tick_ms > 0; tick_ms--) { //если был тик, обрабатываем данные
    btn.state = buttonStateUpdate(); //обновление состояния кнопок

    timerCorrect += debugSettings.timePeriod; //прибавляем период для коррекции
    uint8_t msDec = timerCorrect / 1000; //находим целые мс
    for (uint8_t tm = 0; tm < TIMERS_MS_NUM; tm++) { //опрашиваем все таймеры
      if (_timer_ms[tm]) { //если таймер активен
        if (_timer_ms[tm] > msDec) _timer_ms[tm] -= msDec; //если таймер больше периода
        else _timer_ms[tm] = 0; //иначе сбрасываем таймер
      }
    }
    timerCorrect %= 1000; //оставляем коррекцию

    if (EIMSK) { //если работаем от внешнего тактирования
      timerSQW += msDec; //прибавили время
      if (timerSQW > SQW_MAX_TIME) { //если сигнал слишком длинный
        EIMSK = 0; //перешли на внутреннее тактирование
        SET_ERROR(SQW_HIGH_TIME_ERROR); //устанавливаем ошибку длинного сигнала
      }
    }
    else { //если внешние тактирование не обнаружено
      timeClock += msDec; //добавляем ко времени период таймера
      if (timeClock >= 1000) { //если прошла секунда
        timeClock -= 1000; //оставляем остаток
        tick_sec++; //прибавляем секунду
      }
    }
  }

  if (tick_sec) { //если был тик, обрабатываем данные
    tick_sec--; //убавили счетчик секунд
    secUpd = dotUpd = 0; //очищаем флаги секунды и точек

    for (uint8_t tm = 0; tm < TIMERS_SEC_NUM; tm++) { //опрашиваем все таймеры
      if (_timer_sec[tm]) _timer_sec[tm]--; //если таймер активен
    }

#if ALARM_TYPE
    alarmDataUpdate(); //проверка таймеров будильников
#endif
#if BTN_ADD_TYPE || IR_PORT_ENABLE
    switch (timerMode) {
      case 1: if (timerCnt != 65535) timerCnt++; break;
      case 2: if (timerCnt) timerCnt--; break;
    }
#endif

    if (EIMSK) { //если работаем от внешнего тактирования
      if (timerSQW < SQW_MIN_TIME) { //если сигнал слишком короткий
        EIMSK = 0; //перешли на внутреннее тактирование
        tick_sec = 0; //сбросили тики
        SET_ERROR(SQW_LOW_TIME_ERROR); //устанавливаем ошибку короткого сигнала
        return; //выходим
      }
      timerSQW = 0; //сбросили таймер
    }
    else if (!_timer_sec[TMR_SYNC] && RTC.s == RTC_SYNC_PHASE) { //если работаем от внутреннего тактирования
      _timer_sec[TMR_SYNC] = (uint16_t)(RTC_SYNC_TIME * 60); //установили таймер
      getTime(); //получили новое время
      return; //выходим
    }

    //счет времени
    if (++RTC.s > 59) { //секунды
      RTC.s = 0; //сбросили секунды
      if (++RTC.m > 59) { //минуты
        RTC.m = 0; //сбросили минуты
        if (++RTC.h > 23) { //часы
          RTC.h = 0; //сбросили часы
          if (++RTC.DW > 7) RTC.DW = 1; //день недели
          if (++RTC.DD > maxDays()) { //дата
            RTC.DD = 1; //сбросили день
            if (++RTC.MM > 12) { //месяц
              RTC.MM = 1; //сбросили месяц
              if (++RTC.YY > 2099) { //год
                RTC.YY = 2021; //сбросили год
              }
            }
          }
        }
        hourSound(); //звук смены часа
        changeBright(); //установка яркости от времени суток
      }
      animShow = 1; //показать анимацию переключения цифр
#if ALARM_TYPE
      checkAlarms(); //проверяем будильники на совпадение
#endif
    }
  }
}
//----------------------------Настройки времени----------------------------------
void settings_time(void) //настройки времени
{
  boolean blink_data = 0; //мигание сигментами
  uint8_t cur_mode = 0; //текущий режим
  uint8_t time_out = 0; //таймаут автовыхода

  indiClr(); //очищаем индикаторы
  dotSetBright(dotMaxBright); //включаем точки

  _timer_ms[TMR_MS] = 0; //сбросили таймер

#if PLAYER_TYPE
  if (mainSettings.knockSound) playerSetTrackNow(PLAYER_TIME_SET_SOUND, PLAYER_GENERAL_FOLDER); //воспроизводим название меню
#endif

  //настройки
  while (1) {
    dataUpdate(); //обработка данных

    if (!secUpd) {
      secUpd = 1;
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
    switch (buttonState()) {
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
  //boolean cur_sound = 0; //текущий режим мелодии
  boolean blink_data = 0; //мигание сигментами
  uint8_t alarm[5]; //массив данных о будильнике
  uint8_t cur_mode = 0; //текущий режим
  uint8_t cur_day = 1; //текущий день недели
  uint8_t time_out = 0; //таймаут автовыхода

  indiClr(); //очищаем индикаторы
  dotSetBright(dotMaxBright); //выключаем точки

  alarmReset(); //сброс будильника
  alarmReadBlock(1, alarm); //читаем блок данных

  _timer_ms[TMR_MS] = 0; //сбросили таймер

#if PLAYER_TYPE
  if (mainSettings.knockSound) playerSetTrackNow(PLAYER_ALARM_SET_SOUND, PLAYER_GENERAL_FOLDER); //воспроизводим название меню
#endif

  while (1) {
    dataUpdate(); //обработка данных

#if PLAYER_TYPE
    if (cur_mode == 3 && !playerWriteStatus()) playerSetTrack(PLAYER_ALARM_START + alarm[ALARM_SOUND], PLAYER_ALARM_FOLDER); //воспроизводим мелодию будильника
#endif

    if (!secUpd) {
      secUpd = 1;
      if (++time_out >= SETTINGS_TIMEOUT) {
#if PLAYER_TYPE
        playerStop(); //сброс воспроизведения мелодии
#else
        melodyStop(); //сброс воспроизведения мелодии
#endif
        checkAlarms(); //проверка будильников
        return; //выходим по тайм-ауту
      }
    }

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

      indiClr(); //очистка индикаторов
      indiPrintNum(cur_mode + 1, 5); //режим
      switch (cur_mode) {
        case 0:
          if (!blink_data || cur_indi) indiPrintNum(alarm[ALARM_HOURS], 0, 2, 0); //вывод часов
          if (!blink_data || !cur_indi) indiPrintNum(alarm[ALARM_MINS], 2, 2, 0); //вывод минут
          break;
        case 1:
        case 2:
          if (!blink_data || cur_mode != 1) indiPrintNum(alarm[ALARM_MODE], 0); //вывод режима
          if (alarm[ALARM_MODE] > 3) {
            if (!blink_data || cur_mode != 2 || cur_indi) indiPrintNum(cur_day, 2); //вывод дня недели
            if (!blink_data || cur_mode != 2 || !cur_indi) indiPrintNum((alarm[ALARM_DAYS] >> cur_day) & 0x01, 3); //вывод установки
          }
          break;
        case 3:
          if (!blink_data) indiPrintNum(alarm[ALARM_SOUND] + 1, 2, 2, 0); //вывод номера мелодии
          break;
      }
      blink_data = !blink_data; //мигание сигментами
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (cur_mode) {
          //настройка времени будильника
          case 0:
            switch (cur_indi) {
              case 0: if (alarm[ALARM_HOURS] > 0) alarm[ALARM_HOURS]--; else alarm[ALARM_HOURS] = 23; break; //часы
              case 1: if (alarm[ALARM_MINS] > 0) alarm[ALARM_MINS]--; else alarm[ALARM_MINS] = 59; break; //минуты
            }
            break;
          //настройка режима будильника
          case 1: if (alarm[ALARM_MODE] > 0) alarm[ALARM_MODE]--; else alarm[ALARM_MODE] = 4; break; //режим
          case 2:
            switch (cur_indi) {
              case 0: if (cur_day > 1) cur_day--; else cur_day = 7; break; //день недели
              case 1: alarm[ALARM_DAYS] &= ~(0x01 << cur_day); break; //установка
            }
            break;
          //настройка мелодии будильника
          case 3:
#if PLAYER_TYPE
            if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = PLAYER_ALARM_MAX - 1; //мелодия
#else
            if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = SOUND_MAX(alarm_sound) - 1; //мелодия
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
            break;
        }
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (cur_mode) {
          //настройка времени будильника
          case 0:
            switch (cur_indi) {
              case 0: if (alarm[ALARM_HOURS] < 23) alarm[ALARM_HOURS]++; else alarm[ALARM_HOURS] = 0; break; //часы
              case 1: if (alarm[ALARM_MINS] < 59) alarm[ALARM_MINS]++; else alarm[ALARM_MINS] = 0; break; //минуты
            }
            break;
          //настройка режима будильника
          case 1: if (alarm[ALARM_MODE] < 4) alarm[ALARM_MODE]++; else alarm[ALARM_MODE] = 0; break; //режим
          case 2:
            switch (cur_indi) {
              case 0: if (cur_day < 7) cur_day++; else cur_day = 1; break; //день недели
              case 1: alarm[ALARM_DAYS] |= (0x01 << cur_day); break; //установка
            }
            break;
          //настройка мелодии будильника
          case 3:
#if PLAYER_TYPE
            if (alarm[ALARM_SOUND] < (PLAYER_ALARM_MAX - 1)) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
#else
            if (alarm[ALARM_SOUND] < (SOUND_MAX(alarm_sound) - 1)) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
            break;
        }
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        cur_indi = !cur_indi;
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case LEFT_KEY_HOLD: //удержание левой кнопки
        if (cur_mode > 0) cur_mode--; else cur_mode = 3; //переключение пунктов

        dotSetBright((cur_mode) ? 0 : dotMaxBright); //включаем точки
        if (cur_mode == 2 && alarm[ALARM_MODE] < 4) cur_mode = 3; //если нет дней недели
#if !PLAYER_TYPE
        if (cur_mode == 3) melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif

        cur_indi = 0; //сбрасываем текущий индикатор
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case RIGHT_KEY_HOLD: //удержание правой кнопки
        if (cur_mode < 3) cur_mode++; else cur_mode = 0; //переключение пунктов

        dotSetBright((cur_mode) ? 0 : dotMaxBright); //включаем точки
        if (cur_mode == 2 && alarm[ALARM_MODE] < 4) cur_mode = 3; //если нет дней недели
#if !PLAYER_TYPE
        if (cur_mode == 3) melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif

        cur_indi = 0; //сбрасываем текущий индикатор
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
  uint8_t curAlarm = alarmsNum > 0;

  indiClr(); //очищаем индикаторы
  dotSetBright(0); //выключаем точки

  alarmReset(); //сброс будильника
  alarmReadBlock(curAlarm, alarm); //читаем блок данных

  _timer_ms[TMR_MS] = 0; //сбросили таймер

#if PLAYER_TYPE
  if (mainSettings.knockSound) playerSetTrackNow(PLAYER_ALARM_SET_SOUND, PLAYER_GENERAL_FOLDER); //воспроизводим название меню
#endif

  while (1) {
    dataUpdate(); //обработка данных

#if PLAYER_TYPE
    if (cur_mode == 6 && !playerWriteStatus()) playerSetTrackNow(PLAYER_ALARM_START + alarm[ALARM_SOUND], PLAYER_ALARM_FOLDER); //воспроизводим мелодию будильника
#endif

    if (!secUpd) {
      secUpd = 1;
      if (++time_out >= SETTINGS_TIMEOUT) {
#if PLAYER_TYPE
        playerStop(); //сброс воспроизведения мелодии
#else
        melodyStop(); //сброс воспроизведения мелодии
#endif
        checkAlarms(); //проверка будильников
        return; //выходим по тайм-ауту
      }
    }

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

      indiClr(); //очистка индикаторов
      indiPrintNum(cur_mode + 1, 5); //режим
      switch (cur_mode) {
        case 0:
        case 1:
          if (!blink_data || cur_mode != 0) indiPrintNum(curAlarm, 0, 2, 0); //вывод номера будильника
          if ((!blink_data || cur_mode != 1) && curAlarm) indiPrintNum(alarm[ALARM_MODE], 3); //вывод режима
          break;
        case 2:
        case 3:
          if (!blink_data || cur_mode != 2) indiPrintNum(alarm[ALARM_HOURS], 0, 2, 0); //вывод часов
          if (!blink_data || cur_mode != 3) indiPrintNum(alarm[ALARM_MINS], 2, 2, 0); //вывод минут
          break;
        case 4:
        case 5:
        case 6:
          if (alarm[ALARM_MODE] > 3) {
            if (!blink_data || cur_mode != 4) indiPrintNum(cur_day, 0); //вывод дня недели
            if (!blink_data || cur_mode != 5) indiPrintNum((alarm[ALARM_DAYS] >> cur_day) & 0x01, 1); //вывод установки
          }
          if (!blink_data || cur_mode != 6) indiPrintNum(alarm[ALARM_SOUND] + 1, 2, 2, 0); //вывод номера мелодии
          break;
      }
      blink_data = !blink_data; //мигание сигментами
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (cur_mode) {
          case 0: if (curAlarm > (alarmsNum > 0)) curAlarm--; else curAlarm = alarmsNum; alarmReadBlock(curAlarm, alarm); break; //будильник
          case 1: if (alarm[ALARM_MODE] > 0) alarm[ALARM_MODE]--; else alarm[ALARM_MODE] = 4; break; //режим

          //настройка времени будильника
          case 2: if (alarm[ALARM_HOURS] > 0) alarm[ALARM_HOURS]--; else alarm[ALARM_HOURS] = 23; break; //часы
          case 3: if (alarm[ALARM_MINS] > 0) alarm[ALARM_MINS]--; else alarm[ALARM_MINS] = 59; break; //минуты

          //настройка режима будильника
          case 4: if (cur_day > 1) cur_day--; else cur_day = 7; break; //день недели
          case 5: alarm[ALARM_DAYS] &= ~(0x01 << cur_day); break; //установка
          case 6:
#if PLAYER_TYPE
            if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = PLAYER_ALARM_MAX - 1; //мелодия
#else
            if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = SOUND_MAX(alarm_sound) - 1; //мелодия
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
            break;
        }
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (cur_mode) {
          case 0: if (curAlarm < alarmsNum) curAlarm++; else curAlarm = (alarmsNum > 0); alarmReadBlock(curAlarm, alarm); break; //будильник
          case 1: if (alarm[ALARM_MODE] < 4) alarm[ALARM_MODE]++; else alarm[ALARM_MODE] = 0; break; //режим

          //настройка времени будильника
          case 2: if (alarm[ALARM_HOURS] < 23) alarm[ALARM_HOURS]++; else alarm[ALARM_HOURS] = 0; break; //часы
          case 3: if (alarm[ALARM_MINS] < 59) alarm[ALARM_MINS]++; else alarm[ALARM_MINS] = 0; break; //минуты

          //настройка режима будильника
          case 4: if (cur_day < 7) cur_day++; else cur_day = 1; break; //день недели
          case 5: alarm[ALARM_DAYS] |= (0x01 << cur_day); break; //установка
          case 6:
#if PLAYER_TYPE
            if (alarm[ALARM_SOUND] < (PLAYER_ALARM_MAX - 1)) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
#else
            if (alarm[ALARM_SOUND] < (SOUND_MAX(alarm_sound) - 1)) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
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
          default:
            cur_mode = (alarm[ALARM_MODE] < 4) ? 6 : 4; //перейти к выбору дня недели
#if !PLAYER_TYPE
            if (cur_mode == 6) melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
            break;
        }
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case LEFT_KEY_HOLD: //удержание левой кнопки
        switch (cur_mode) {
          case 0:
            if (curAlarm) { //если есть будильники в памяти
              delAlarm(curAlarm - 1); //удалить текущий будильник
              dotSetBright(dotMaxBright); //включаем точки
              for (_timer_ms[TMR_MS] = 500; _timer_ms[TMR_MS];) dataUpdate(); //обработка данных
              dotSetBright(0); //выключаем точки
              if (curAlarm > (alarmsNum > 0)) curAlarm--; //убавляем номер текущего будильника
              else curAlarm = (alarmsNum > 0);
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

      case RIGHT_KEY_HOLD: //удержание правой кнопки
        switch (cur_mode) {
          case 0:
            newAlarm(); //создать новый будильник
            dotSetBright(dotMaxBright); //включаем точки
            for (_timer_ms[TMR_MS] = 500; _timer_ms[TMR_MS];) dataUpdate(); //обработка данных
            dotSetBright(0); //выключаем точки
            curAlarm = alarmsNum;
            alarmReadBlock(curAlarm, alarm); //читаем блок данных
            break;
          case 4:
          case 5:
            cur_mode = 6;
#if !PLAYER_TYPE
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
            break;
          default:
            cur_mode = (alarm[ALARM_MODE] < 4) ? 6 : 4; //режим настройки функций
#if !PLAYER_TYPE
            if (cur_mode == 6) melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
            dotSetBright(dotMaxBright); //включаем точки
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
  uint8_t cur_mode = 0; //текущий режим
  uint8_t time_out = 0; //таймаут автовыхода

  indiClr(); //очищаем индикаторы
  dotSetBright(0); //выключаем точки

  _timer_ms[TMR_MS] = 0; //сбросили таймер

#if PLAYER_TYPE
  if (mainSettings.knockSound) playerSetTrackNow(PLAYER_MAIN_MENU_START, PLAYER_MENU_FOLDER); //воспроизводим название меню
#endif

  //настройки
  while (1) {
    dataUpdate(); //обработка данных

    if (!secUpd) {
      secUpd = 1;
      if (++time_out >= SETTINGS_TIMEOUT) return;
    }

    if (cur_mode == SET_TEMP_SENS && !_timer_ms[TMR_SENS]) { //если таймаут нового запроса вышел
      updateTemp(); //обновить показания температуры
      _timer_ms[TMR_SENS] = TEMP_UPDATE_TIME; //установили таймаут
    }

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

      indiClr(); //очистка индикаторов
      switch (set) {
        case 0:
          indiPrintNum(cur_mode + 1, (LAMP_NUM / 2 - 1), 2, 0); //вывод режима
          break;
        case 1:
          indiPrintNum(cur_mode + 1, 4, 2); //режим
          switch (cur_mode) {
            case SET_TIME_FORMAT: if (!blink_data) indiPrintNum((mainSettings.timeFormat) ? 12 : 24, 2); break; //вывод формата времени
            case SET_GLITCH: if (!blink_data) indiPrintNum(mainSettings.glitchMode, 3); break; //вывод режима глюков
            case SET_BTN_SOUND: //вывод звука кнопок
#if PLAYER_TYPE
              if (!blink_data || cur_indi) indiPrintNum(mainSettings.volumeSound, 2, 2, 0); //громкость озвучки
#endif
              if (!blink_data || !cur_indi) indiPrintNum(mainSettings.knockSound, 0); //звук кнопок или озвучка
              break;
            case SET_HOUR_TIME:
              if (!blink_data || cur_indi) indiPrintNum(mainSettings.timeHour[0], 0, 2, 0); //вывод часа начала звукового оповещения нового часа
              if (!blink_data || !cur_indi) indiPrintNum(mainSettings.timeHour[1], 2, 2, 0); //вывод часа окончания звукового оповещения нового часа
              break;
            case SET_BRIGHT_TIME:
              if (!blink_data || cur_indi) indiPrintNum(mainSettings.timeBright[0], 0, 2, 0); //вывод часа начала ночной посветки
              if (!blink_data || !cur_indi) indiPrintNum(mainSettings.timeBright[1], 2, 2, 0); //вывод часа окончания ночной посветки
              break;
            case SET_INDI_BRIGHT:
              if (!blink_data || cur_indi) indiPrintNum(mainSettings.indiBright[0], 0, 2, 0); //яркости ночь
              if (!blink_data || !cur_indi) indiPrintNum(mainSettings.indiBright[1], 2, 2, 0); //вывод яркости день
              break;
            case SET_BACKL_BRIGHT:
              if (!blink_data || cur_indi) indiPrintNum(mainSettings.backlBright[0] / 10, 0, 2, 0); //яркости ночь
              if (!blink_data || !cur_indi) indiPrintNum(mainSettings.backlBright[1] / 10, 2, 2, 0); //вывод яркости день
              break;
            case SET_DOT_BRIGHT:
              if (!blink_data || cur_indi) indiPrintNum(mainSettings.dotBright[0] / 10, 0, 2, 0); //вывод яркости ночь
              if (!blink_data || !cur_indi) indiPrintNum(mainSettings.dotBright[1] / 10, 2, 2, 0); //вывод яркости день
              break;
            case SET_TEMP_SENS:
              if (!blink_data) {
                if (sens.err) indiPrintNum(0, 0); //вывод ошибки
                else indiPrintNum(sens.temp / 10 + mainSettings.tempCorrect, 0, 3); //вывод температуры
              }
              indiPrintNum(sens.type, 3); //вывод сенсора температуры
              break;
            case SET_AUTO_TEMP:
              if (!blink_data) indiPrintNum(mainSettings.autoTempTime, 1, 3); //вывод времени автопоказа температуры
              break;
            case SET_BURN_MODE:
              if (!blink_data) indiPrintNum(mainSettings.burnMode, 3); //вывод анимации антиотравления индикаторов
              break;
          }
          blink_data = !blink_data; //мигание сигментами
          break;
      }
    }
    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (set) {
          case 0:
            if (cur_mode > 0) cur_mode--;
            else cur_mode = SET_MAX_ITEMS - 1;
#if PLAYER_TYPE
            if (mainSettings.knockSound) playerSetTrackNow(PLAYER_MAIN_MENU_START + cur_mode, PLAYER_MENU_FOLDER);
#endif
            break;
          case 1:
            switch (cur_mode) {
              case SET_TIME_FORMAT: mainSettings.timeFormat = 0; break; //формат времени
              case SET_GLITCH: mainSettings.glitchMode = 0; break; //глюки
              case SET_BTN_SOUND: //звук кнопок
                switch (cur_indi) {
                  case 0: mainSettings.knockSound = 0; break;
#if PLAYER_TYPE
                  case 1: if (mainSettings.volumeSound > PLAYER_MIN_VOL) playerSetVol(--mainSettings.volumeSound); break;
#endif
                }
                break;
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
#if BACKL_TYPE == 2
                setLedBright(mainSettings.backlBright[cur_indi]); //устанавливаем максимальную яркость
                setLedHue(fastSettings.backlColor); //устанавливаем статичный цвет
                showLeds(); //отрисовка светодиодов
#else
                backlSetBright(mainSettings.backlBright[cur_indi]); //если посветка статичная, устанавливаем яркость
#endif
                break;
              case SET_DOT_BRIGHT: //яркость точек
                switch (cur_indi) {
                  case 0: if (mainSettings.dotBright[0] > 0) mainSettings.dotBright[0] -= 10; else mainSettings.dotBright[0] = 250; break;
                  case 1: if (mainSettings.dotBright[1] > 10) mainSettings.dotBright[1] -= 10; else mainSettings.dotBright[1] = 250; break;
                }
                dotSetBright(mainSettings.dotBright[cur_indi]); //включаем точки
                break;
              case SET_TEMP_SENS: //настройка коррекции температуры
                if (mainSettings.tempCorrect > -127) mainSettings.tempCorrect--; else mainSettings.tempCorrect = 127;
                break;
              case SET_AUTO_TEMP: //автопоказ температуры
                if (mainSettings.autoTempTime > 5) mainSettings.autoTempTime -= 5; else mainSettings.autoTempTime = 0;
                break;
              case SET_BURN_MODE: //анимация антиотравления индикаторов
                if (mainSettings.burnMode) mainSettings.burnMode--; else mainSettings.burnMode = (BURN_EFFECT_NUM - 1);
                break;
            }
            break;
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (set) {
          case 0:
            if (cur_mode < (SET_MAX_ITEMS - 1)) cur_mode++;
            else cur_mode = 0;
#if PLAYER_TYPE
            if (mainSettings.knockSound) playerSetTrackNow(PLAYER_MAIN_MENU_START + cur_mode, PLAYER_MENU_FOLDER);
#endif
            break;
          case 1:
            switch (cur_mode) {
              case SET_TIME_FORMAT: mainSettings.timeFormat = 1; break; //формат времени
              case SET_GLITCH: mainSettings.glitchMode = 1; break; //глюки
              case SET_BTN_SOUND: //звук кнопок
                switch (cur_indi) {
                  case 0: mainSettings.knockSound = 1; break;
#if PLAYER_TYPE
                  case 1: if (mainSettings.volumeSound < PLAYER_MAX_VOL) playerSetVol(++mainSettings.volumeSound); break;
#endif
                }
                break;
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
#if BACKL_TYPE == 2
                setLedBright(mainSettings.backlBright[cur_indi]); //устанавливаем максимальную яркость
                setLedHue(fastSettings.backlColor); //устанавливаем статичный цвет
                showLeds(); //отрисовка светодиодов
#else
                backlSetBright(mainSettings.backlBright[cur_indi]); //если посветка статичная, устанавливаем яркость
#endif
                break;
              case SET_DOT_BRIGHT: //яркость точек
                switch (cur_indi) {
                  case 0: if (mainSettings.dotBright[0] < 250) mainSettings.dotBright[0] += 10; else mainSettings.dotBright[0] = 0; break;
                  case 1: if (mainSettings.dotBright[1] < 250) mainSettings.dotBright[1] += 10; else mainSettings.dotBright[1] = 10; break;
                }
                dotSetBright(mainSettings.dotBright[cur_indi]); //включаем точки
                break;
              case SET_TEMP_SENS: //настройка коррекции температуры
                if (mainSettings.tempCorrect < 127) mainSettings.tempCorrect++; else mainSettings.tempCorrect = -127;
                break;
              case SET_AUTO_TEMP: //автопоказ температуры
                if (mainSettings.autoTempTime < 240) mainSettings.autoTempTime += 5; else mainSettings.autoTempTime = 240;
                break;
              case SET_BURN_MODE: //анимация антиотравления индикаторов
                if (mainSettings.burnMode < (BURN_EFFECT_NUM - 1)) mainSettings.burnMode++; else mainSettings.burnMode = 0;
                break;
            }
            break;
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        set = !set;
        if (set) {
          switch (cur_mode) {
            case SET_INDI_BRIGHT: indiSetBright(mainSettings.indiBright[0]); break; //установка общей яркости индикаторов
            case SET_BACKL_BRIGHT: //яркость подсветки
#if BACKL_TYPE == 2
              setLedBright(mainSettings.backlBright[0]); //устанавливаем максимальную яркость
              setLedHue(fastSettings.backlColor); //устанавливаем статичный цвет
              showLeds(); //отрисовка светодиодов
#else
              backlSetBright(mainSettings.backlBright[0]); //если посветка статичная, устанавливаем яркость
#endif
              backlAnimDisable(); //запретили эффекты подсветки
              break;
            case SET_TEMP_SENS: //настройка коррекции температуры
#if DOTS_PORT_ENABLE
              indiSetDots(2); //включаем разделителную точку
#endif
              break;
          }
          dotSetBright((cur_mode != SET_DOT_BRIGHT) ? dotMaxBright : mainSettings.dotBright[0]); //включаем точки
        }
        else {
          changeBright(); //установка яркости от времени суток
          dotSetBright(0); //выключаем точки
#if DOTS_PORT_ENABLE
          indiClrDots(); //выключаем разделительные точки
#endif
        }
        cur_indi = 0;
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case LEFT_KEY_HOLD: //удержание левой кнопки
        if (set) {
          cur_indi = 0;
          switch (cur_mode) {
            case SET_INDI_BRIGHT: indiSetBright(mainSettings.indiBright[0]); break; //установка общей яркости индикаторов
            case SET_BACKL_BRIGHT: //яркость подсветки
#if BACKL_TYPE == 2
              setLedBright(mainSettings.backlBright[0]); //устанавливаем максимальную яркость
              setLedHue(fastSettings.backlColor); //устанавливаем статичный цвет
              showLeds(); //отрисовка светодиодов
#else
              backlSetBright(mainSettings.backlBright[0]); //если посветка статичная, устанавливаем яркость
#endif
              break;
            case SET_DOT_BRIGHT: dotSetBright(mainSettings.dotBright[0]); break;//яркость точек
            case SET_TEMP_SENS: mainSettings.tempCorrect = 0; break; //сброс коррекции температуры
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
#if BACKL_TYPE == 2
              setLedBright(mainSettings.backlBright[1]); //устанавливаем максимальную яркость
              setLedHue(fastSettings.backlColor); //устанавливаем статичный цвет
              showLeds(); //отрисовка светодиодов
#else
              backlSetBright(mainSettings.backlBright[1]); //если посветка статичная, устанавливаем яркость
#endif
              break;
            case SET_DOT_BRIGHT: dotSetBright(mainSettings.dotBright[1]); break;//яркость точек
            case SET_TEMP_SENS: mainSettings.tempCorrect = 0; break; //сброс коррекции температуры
          }
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        updateData((uint8_t*)&mainSettings, sizeof(mainSettings), EEPROM_BLOCK_SETTINGS_MAIN, EEPROM_BLOCK_CRC_MAIN); //записываем основные настройки в память
        changeBright(); //установка яркости от времени суток
        return;
    }
  }
}
//---------------------Обработка сенсора яркости освещения-----------------------------
void lightSensUpdate(void) //обработка сенсора яркости освещения
{
  static boolean now_state_light;
  if ((mainSettings.timeBright[0] == mainSettings.timeBright[1]) && !_timer_ms[TMR_LIGHT]) {
    _timer_ms[TMR_LIGHT] = LIGHT_SENS_TIME;
    analogState |= 0x01; //установили флаг обновления АЦП сенсора яркости
#if LIGHT_SENS_PULL
    now_state_light = (adc_light < debugSettings.min_light) ? 0 : ((adc_light > debugSettings.max_light) ? 1 : now_state_light);
#else
    now_state_light = (adc_light < debugSettings.min_light) ? 1 : ((adc_light > debugSettings.max_light) ? 0 : now_state_light);
#endif
    if (now_state_light != state_light) {
      state_light = now_state_light;
      changeBright(); //установка яркости
    }
  }
}
//---------------------Установка яркости от времени суток-----------------------------
void changeBright(void) //установка яркости от времени суток
{
  if ((mainSettings.timeBright[0] != mainSettings.timeBright[1]) ? (checkHourStrart(mainSettings.timeBright[0], mainSettings.timeBright[1])) : state_light) {
    //ночной режим
    dotMaxBright = mainSettings.dotBright[0]; //установка максимальной яркости точек
    backlMaxBright = mainSettings.backlBright[0]; //установка максимальной яркости подсветки
    indiMaxBright = mainSettings.indiBright[0]; //установка максимальной яркости индикаторов
  }
  else {
    //дневной режим
    dotMaxBright = mainSettings.dotBright[1]; //установка максимальной яркости точек
    backlMaxBright = mainSettings.backlBright[1]; //установка максимальной яркости подсветки
    indiMaxBright = mainSettings.indiBright[1]; //установка максимальной яркости индикаторов
  }
  switch (fastSettings.dotMode) {
    case 0: dotSetBright(0); break; //если точки выключены
    case 1: dotSetBright(dotMaxBright); break; //если точки статичные, устанавливаем яркость
    case 2:
      if (dotMaxBright) {
        dotBrightStep = setBrightStep(dotMaxBright * 2, DOT_STEP_TIME, DOT_TIME); //расчёт шага яркости точки
        dotBrightTime = setBrightTime(dotMaxBright * 2, DOT_STEP_TIME, DOT_TIME); //расчёт шага яркости точки
      }
      else dotSetBright(0); //если точки выключены
      break;
  }
  backlAnimEnable(); //разрешили эффекты подсветки
#if BACKL_TYPE == 2
  if (backlMaxBright) {
    switch (fastSettings.backlMode) {
      case BACKL_OFF: clrLeds(); break; //выключили светодиоды
      case BACKL_STATIC:
        setLedBright(backlMaxBright); //устанавливаем максимальную яркость
        setLedHue(fastSettings.backlColor); //устанавливаем статичный цвет
        showLeds(); //отрисовка светодиодов
        break;
      case BACKL_SMOOTH_COLOR_CHANGE:
      case BACKL_RAINBOW:
      case BACKL_CONFETTI:
        setLedBright(backlMaxBright); //устанавливаем максимальную яркость
        showLeds(); //отрисовка светодиодов
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
    backlMinBright = (backlMaxBright > (BACKL_MIN_BRIGHT + 10)) ? BACKL_MIN_BRIGHT : 0;
    uint8_t backlNowBright = (backlMaxBright > BACKL_MIN_BRIGHT) ? (backlMaxBright - BACKL_MIN_BRIGHT) : backlMaxBright;

    backl.mode_2_time = setBrightTime((uint16_t)backlNowBright * 2, BACKL_MODE_2_STEP_TIME, BACKL_MODE_2_TIME); //расчёт шага яркости
    backl.mode_2_step = setBrightStep((uint16_t)backlNowBright * 2, BACKL_MODE_2_STEP_TIME, BACKL_MODE_2_TIME); //расчёт шага яркости

#if BACKL_TYPE == 2
    backl.mode_4_step = ceil((float)backlMaxBright / (float)BACKL_MODE_4_TAIL / (float)BACKL_MODE_4_FADING); //расчёт шага яркости
    if (!backl.mode_4_step) backl.mode_4_step = 1; //если шаг слишком мал, устанавливаем минимум
    backl.mode_6_time = setBrightTime((uint16_t)backlNowBright * LAMP_NUM, BACKL_MODE_6_STEP_TIME, BACKL_MODE_6_TIME); //расчёт шага яркости
    backl.mode_6_step = setBrightStep((uint16_t)backlNowBright * LAMP_NUM, BACKL_MODE_6_STEP_TIME, BACKL_MODE_6_TIME); //расчёт шага яркости
#endif
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

  if (backlMaxBright) { //если подсветка не выключена
    if (!_timer_ms[TMR_BACKL]) { //если время пришло
      switch (fastSettings.backlMode) {
        case BACKL_OFF: //подсветка выключена
        case BACKL_STATIC: //статичный режим
          return; //выходим
        case BACKL_PULS:
        case BACKL_PULS_COLOR: { //дыхание подсветки
            _timer_ms[TMR_BACKL] = backl.mode_2_time; //установили таймер
            if (backl_drv) { //если светодиоды в режиме разгорания
              if (incLedBright(backl.mode_2_step, backlMaxBright)) backl_drv = 0; //прибавили шаг яркости
            }
            else { //иначе светодиоды в режиме затухания
              if (decLedBright(backl.mode_2_step, backlMinBright)) { //уменьшаем яркость
                backl_drv = 1;
                if (fastSettings.backlMode == BACKL_PULS_COLOR) color_steps += BACKL_MODE_3_COLOR; //меняем цвет
                else color_steps = fastSettings.backlColor; //иначе статичный цвет
                setLedHue(color_steps); //установили цвет
                _timer_ms[TMR_BACKL] = BACKL_MODE_2_PAUSE; //установили таймер
              }
            }
          }
          break;
        case BACKL_RUNNING_FIRE:
        case BACKL_RUNNING_FIRE_COLOR: { //бегущий огонь
            _timer_ms[TMR_BACKL] = BACKL_MODE_4_TIME / LAMP_NUM / BACKL_MODE_4_FADING; //установили таймер
            if (backl_steps) { //если есть шаги затухания
              decLedsBright(backl_pos - 1, backl.mode_4_step); //уменьшаем яркость
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
            if (fastSettings.backlMode != BACKL_RUNNING_FIRE_COLOR) {
              color_steps = fastSettings.backlColor; //статичный цвет
              setLedHue(color_steps); //установили цвет
            }
          }
          break;
        case BACKL_WAVE:
        case BACKL_WAVE_COLOR: { //волна
            _timer_ms[TMR_BACKL] = backl.mode_6_time; //установили таймер
            if (backl_drv) {
              if (incLedBright(backl_pos, backl.mode_6_step, backlMaxBright)) { //прибавили шаг яркости
                if (backl_pos < (LAMP_NUM - 1)) backl_pos++; //сменили позицию
                else {
                  backl_pos = 0; //сбросили позицию
                  backl_drv = 0; //перешли в затухание
                }
              }
            }
            else {
              if (decLedBright(backl_pos, backl.mode_6_step, backlMinBright)) { //иначе убавляем яркость
                if (backl_pos < (LAMP_NUM - 1)) backl_pos++; //сменили позицию
                else {
                  backl_pos = 0; //сбросили позицию
                  backl_drv = 1; //перешли в разгорание
                }
              }
            }
            if (fastSettings.backlMode != BACKL_WAVE_COLOR) {
              color_steps = fastSettings.backlColor; //статичный цвет
              setLedHue(color_steps); //установили цвет
            }
          }
          break;
        case BACKL_RAINBOW: { //радуга
            _timer_ms[TMR_BACKL] = BACKL_MODE_9_TIME; //установили таймер
            color_steps += BACKL_MODE_9_STEP; //прибавили шаг
            for (uint8_t f = 0; f < LAMP_NUM; f++) setLedHue(f, color_steps + (f * BACKL_MODE_9_STEP)); //установили цвет
          }
          break;
        case BACKL_CONFETTI: { //рандомный цвет
            _timer_ms[TMR_BACKL] = BACKL_MODE_10_TIME; //установили таймер
            setLedHue(random(0, LAMP_NUM), random(0, 256)); //установили цвет
          }
          break;
      }
      showLeds(); //отрисовка светодиодов
    }
    if (!_timer_ms[TMR_COLOR]) { //если время пришло
      switch (fastSettings.backlMode) {
        case BACKL_RUNNING_FIRE_COLOR:
        case BACKL_WAVE_COLOR:
        case BACKL_SMOOTH_COLOR_CHANGE: { //плавная смена цвета
            _timer_ms[TMR_COLOR] = BACKL_MODE_8_TIME; //установили таймер
            color_steps += BACKL_MODE_8_COLOR;
            setLedHue(color_steps); //установили цвет
            showLeds(); //отрисовка светодиодов
          }
          break;
      }
    }
  }
}
//----------------------------------Мигание подсветки---------------------------------
void backlFlash(void) //мигание подсветки
{
  static boolean backl_drv; //направление яркости

  if (fastSettings.backlMode == BACKL_PULS && backlMaxBright) {
    if (!_timer_ms[TMR_BACKL]) {
      _timer_ms[TMR_BACKL] = backl.mode_2_time;
      switch (backl_drv) {
        case 0: if (backlIncBright(backl.mode_2_step, backlMaxBright)) backl_drv = 1; break;
        case 1:
          if (backlDecBright(backl.mode_2_step, backlMinBright)) {
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

  if (dotMaxBright && !dotUpd && !_timer_ms[TMR_DOT]) {
    switch (mode) {
      case DOT_OFF: if (dotGetBright()) dotSetBright(0); break; //если точки включены, выключаем их
      case DOT_STATIC: if (dotGetBright() != dotMaxBright) dotSetBright(dotMaxBright); break; //если яркость не совпадает, устанавливаем яркость
      case DOT_PULS:
        _timer_ms[TMR_DOT] = dotBrightTime;
        switch (dot_drv) {
          case 0: if (dotIncBright(dotBrightStep, dotMaxBright)) dot_drv = 1; break;
          case 1:
            if (dotDecBright(dotBrightStep, 0)) {
              dot_drv = 0;
              dotUpd = 1;
            }
            break;
        }
        break;
      case DOT_BLINK:
        switch (dot_drv) {
          case 0: dotSetBright(dotMaxBright); dot_drv = 1; _timer_ms[TMR_DOT] = 500; break; //включаем точки
          case 1: dotSetBright(0); dot_drv = 0; dotUpd = 1; break; //выключаем точки
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
          dotUpd = 1;
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
  sens.err = 1; //подняли флаг проверки датчика температуры на ошибку связи
  switch (sens.type) { //выбор датчика температуры
    default: if (readTempRTC()) sens.err = 0; return; //чтение температуры с датчика DS3231
#if SENS_BME_ENABLE
    case SENS_BME: readTempBME(); break; //чтение температуры/давления/влажности с датчика BME/BMP
#endif
#if SENS_PORT_ENABLE
    case SENS_DS18B20: readTempDS(); break; //чтение температуры с датчика DS18x20
    case SENS_DHT: readTempDHT(); break; //чтение температуры/влажности с датчика DHT/MW/AM
#endif
  }
  if (sens.err) readTempRTC(); //чтение температуры с датчика DS3231
}
//----------------------------Воспроизвести температуру--------------------------------------
void speakTemp(void) //воспроизвести температуру
{
  uint16_t _ceil = (sens.temp / 10 + mainSettings.tempCorrect) / 10;
  uint16_t _dec = (sens.temp / 10 + mainSettings.tempCorrect) % 10;

  playerSetTrackNow(PLAYER_TEMP_SOUND, PLAYER_GENERAL_FOLDER);
  if (_dec) {
    playerSpeakNumber(_ceil, OTHER_NUM);
    playerSetTrack(PLAYER_SENS_CEIL_START + (boolean)playerGetSpeak(_ceil), PLAYER_END_NUMBERS_FOLDER);
    playerSpeakNumber(_dec, OTHER_NUM);
    playerSetTrack(PLAYER_SENS_DEC_START + (boolean)playerGetSpeak(_dec), PLAYER_END_NUMBERS_FOLDER);
    playerSetTrack(PLAYER_SENS_TEMP_START + 1, PLAYER_END_NUMBERS_FOLDER);
  }
  else {
    playerSpeakNumber(_ceil);
    playerSetTrack(PLAYER_SENS_TEMP_START + playerGetSpeak(_ceil), PLAYER_END_NUMBERS_FOLDER);
  }
}
//------------------------------Воспроизвести влажность---------------------------------------
void speakHum(void) //воспроизвести влажность
{
  playerSetTrackNow(PLAYER_HUM_SOUND, PLAYER_GENERAL_FOLDER);
  playerSpeakNumber(sens.hum);
  playerSetTrack(PLAYER_SENS_HUM_START + playerGetSpeak(sens.hum), PLAYER_END_NUMBERS_FOLDER);
}
//-------------------------------Воспроизвести давление---------------------------------------
void speakPress(void) //воспроизвести давление
{
  playerSetTrackNow(PLAYER_PRESS_SOUND, PLAYER_GENERAL_FOLDER);
  playerSpeakNumber(sens.press);
  playerSetTrack(PLAYER_SENS_PRESS_START + playerGetSpeak(sens.press), PLAYER_END_NUMBERS_FOLDER);
  playerSetTrack(PLAYER_SENS_PRESS_OTHER, PLAYER_END_NUMBERS_FOLDER);
}
//--------------------------------Автоматический показ температуры----------------------------------------
void autoShowTemp(void) //автоматический показ температуры
{
  if (mainSettings.autoTempTime && !_timer_sec[TMR_TEMP] && RTC.s > 7 && RTC.s < 55) {
    _timer_sec[TMR_TEMP] = mainSettings.autoTempTime; //устанавливаем таймер

    uint8_t pos = LAMP_NUM; //текущее положение анимации
    boolean drv = 0; //направление анимации

    if (!_timer_ms[TMR_SENS]) { //если таймаут нового запроса вышел
      updateTemp(); //обновить показания температуры
      _timer_ms[TMR_SENS] = TEMP_UPDATE_TIME; //установили таймаут
    }

#if DOTS_PORT_ENABLE
    dotSetBright(0); //выключаем точки
#else
    dotSetBright(dotMaxBright); //включаем точки
#endif

    for (uint8_t mode = 0; mode < 3; mode++) {
      switch (mode) {
        case 1:
          if (!sens.hum) {
            if (!sens.press) return; //выходим
            else mode = 2; //отображаем давление
          }
#if DOTS_PORT_ENABLE
          indiClrDots(); //выключаем разделительные точки
#else
          dotSetBright(0); //выключаем точки
#endif
          break;
        case 2: if (!sens.press) return; break; //выходим
      }

      while (1) { //анимация перехода
        dataUpdate(); //обработка данных
        if (buttonState()) return; //возврат если нажата кнопка
        if (!_timer_ms[TMR_ANIM]) { //если таймер истек
          _timer_ms[TMR_ANIM] = AUTO_TEMP_ANIM_TIME; //устанавливаем таймер

          indiClr(); //очистка индикаторов
          switch (mode) {
            case 0:
              indiPrintNum(sens.temp / 10 + mainSettings.tempCorrect, pos, 3, ' '); //вывод температуры
#if DOTS_PORT_ENABLE
              indiClrDots(); //выключаем разделительные точки
              indiSetDots(pos + 2); //включаем разделителную точку
#endif
              break;
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
  secUpd = 0; //обновление экрана

  if (!_timer_ms[TMR_SENS]) { //если таймаут нового запроса вышел
    updateTemp(); //обновить показания температуры
    _timer_ms[TMR_SENS] = TEMP_UPDATE_TIME; //установили таймаут
  }

#if DOTS_PORT_ENABLE
  indiSetDots(2); //включаем разделителную точку
  dotSetBright(0); //выключаем точки
#else
  dotSetBright(dotMaxBright); //включаем точки
#endif

#if PLAYER_TYPE
  if (mainSettings.knockSound) speakTemp(); //воспроизвести температуру
#endif

  for (_timer_ms[TMR_MS] = SHOW_TEMP_TIME; _timer_ms[TMR_MS];) {
    dataUpdate(); //обработка данных

    if (!secUpd) {
      secUpd = 1; //сбрасываем флаг
      indiClr(); //очистка индикаторов
      indiPrintNum(mode + 1, 5); //режим
      switch (mode) {
        case 0: indiPrintNum(sens.temp / 10 + mainSettings.tempCorrect, 0, 3, ' '); break;
        case 1: indiPrintNum(sens.hum, 0, 4, ' '); break;
        case 2: indiPrintNum(sens.press, 0, 4, ' '); break;
      }
    }

    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        if (++mode > 2) mode = 0;
        switch (mode) {
          case 1:
            if (!sens.hum) {
              if (!sens.press) mode = 0;
              else {
                mode = 2;
#if DOTS_PORT_ENABLE
                indiClrDots(); //выключаем разделительные точки
#else
                dotSetBright(0); //выключаем точки
#endif
              }
            }
            else {
#if DOTS_PORT_ENABLE
              indiClrDots(); //выключаем разделительные точки
#else
              dotSetBright(0); //выключаем точки
#endif
            }
            break;
          case 2: if (!sens.press) mode = 0; break;
        }
        if (!mode) { //если режим отображения температуры
#if DOTS_PORT_ENABLE
          indiSetDots(2); //включаем разделителную точку
#else
          dotSetBright(dotMaxBright); //включаем точки
#endif
        }
#if PLAYER_TYPE
        if (mainSettings.knockSound) {
          switch (mode) {
            case 0: speakTemp(); break; //воспроизвести температуру
            case 1: speakHum(); break; //воспроизвести влажность
            case 2: speakPress(); break; //воспроизвести давление
          }
        }
#endif
        _timer_ms[TMR_MS] = SHOW_TEMP_TIME;
        secUpd = 0; //обновление экрана
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
      case SET_KEY_PRESS: //клик средней кнопкой
        return; //выходим
    }
  }
}
//-------------------------------Воспроизвести время--------------------------------
void speakTime(void) //воспроизвести время
{
  playerSetTrackNow(PLAYER_TIME_NOW_SOUND, PLAYER_GENERAL_FOLDER);
  playerSpeakNumber(RTC.h);
  playerSetTrack(PLAYER_TIME_HOUR_START + playerGetSpeak(RTC.h), PLAYER_END_NUMBERS_FOLDER);
  if (RTC.m) {
    playerSpeakNumber(RTC.m, OTHER_NUM);
    playerSetTrack(PLAYER_TIME_MINS_START + playerGetSpeak(RTC.m), PLAYER_END_NUMBERS_FOLDER);
  }
}
//----------------------------------Показать дату-----------------------------------
void showDate(void) //показать дату
{
  uint8_t mode = 0; //текущий режим
  secUpd = 0; //обновление экрана

#if DOTS_PORT_ENABLE
  indiSetDots(2); //включаем разделителную точку
  dotSetBright(0); //выключаем точки
#else
  dotSetBright(dotMaxBright); //включаем точки
#endif

#if PLAYER_TYPE
  if (mainSettings.knockSound) speakTime(); //воспроизвести время
#endif

  for (_timer_ms[TMR_MS] = SHOW_DATE_TIME; _timer_ms[TMR_MS];) {
    dataUpdate(); //обработка данных

    if (!secUpd) {
      secUpd = 1; //сбрасываем флаг
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

    switch (buttonState()) {
      case RIGHT_KEY_PRESS: //клик правой кнопкой
        if (++mode > 1) mode = 0;
        switch (mode) {
          case 0: //дата
#if DOTS_PORT_ENABLE
            indiSetDots(2); //включаем разделителную точку
#else
            dotSetBright(dotMaxBright); //включаем точки
#endif
            break;
          case 1: //год
#if DOTS_PORT_ENABLE
            indiClrDots(); //выключаем разделительные точки
#else
            dotSetBright(0); //выключаем точки
#endif
            break;
        }
        _timer_ms[TMR_MS] = SHOW_DATE_TIME;
        secUpd = 0; //обновление экрана
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

#if PLAYER_TYPE
  if (mainSettings.knockSound) playerSetTrackNow(PLAYER_FAST_MENU_START, PLAYER_MENU_FOLDER);
#endif

  for (_timer_ms[TMR_MS] = FAST_BACKL_TIME; _timer_ms[TMR_MS];) {
    dataUpdate(); //обработка данных

    if (anim < 4) {
      if (!_timer_ms[TMR_ANIM]) { //если таймер истек
        _timer_ms[TMR_ANIM] = FAST_ANIM_TIME; //устанавливаем таймер

        indiClr(); //очистка индикаторов
        indiPrintNum(mode + 1, 5); //режим
        switch (mode) {
          case FAST_BACKL_MODE: indiPrintNum(fastSettings.backlMode, anim - 1, 2); break; //вывод режима подсветки
          case FAST_FLIP_MODE: indiPrintNum(fastSettings.flipMode, anim - 1, 2); break; //вывод режима анимации
          case FAST_DOT_MODE: indiPrintNum(fastSettings.dotMode, anim); break; //вывод режима точек
          case FAST_BACKL_COLOR: indiPrintNum(fastSettings.backlColor / 10, anim - 1, 2); break; //вывод цвета подсветки
        }
        anim++; //сдвигаем анимацию
      }
    }

    switch (buttonState()) {
      case SET_KEY_PRESS: //клик средней кнопкой
        if (mode != FAST_BACKL_MODE) {
#if PLAYER_TYPE
          if (mainSettings.knockSound) playerSetTrackNow(PLAYER_FAST_MENU_START + FAST_BACKL_MODE, PLAYER_MENU_FOLDER);
#endif
          mode = FAST_BACKL_MODE; //демострация текущего режима работы
        }
        else {
#if BACKL_TYPE == 2
          if (++fastSettings.backlMode < BACKL_EFFECT_NUM) {
            switch (fastSettings.backlMode) {
              case BACKL_STATIC:
                setLedBright(backlMaxBright); //устанавливаем максимальную яркость
                setLedHue(fastSettings.backlColor); //устанавливаем статичный цвет
                break;
              case BACKL_PULS:
                setLedBright(BACKL_MIN_BRIGHT); //устанавливаем минимальную яркость
                setLedHue(fastSettings.backlColor); //устанавливаем статичный цвет
                break;
              case BACKL_RUNNING_FIRE:
                setLedBright(0); //устанавливаем минимальную яркость
                setLedHue(fastSettings.backlColor); //устанавливаем статичный цвет
                break;
              case BACKL_WAVE:
                setLedBright(BACKL_MIN_BRIGHT); //устанавливаем минимальную яркость
                setLedHue(fastSettings.backlColor); //устанавливаем статичный цвет
                break;
              case BACKL_SMOOTH_COLOR_CHANGE:
                setLedBright(backlMaxBright); //устанавливаем максимальную яркость
                break;
            }
            showLeds(); //отрисовка светодиодов
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
        _timer_ms[TMR_MS] = FAST_BACKL_TIME;
        anim = 0;
        break;
#if BACKL_TYPE == 2
      case SET_KEY_HOLD: //удержание средней кнопки
        if (!mode) {
          switch (fastSettings.backlMode) {
            case BACKL_STATIC:
            case BACKL_PULS:
            case BACKL_RUNNING_FIRE:
            case BACKL_WAVE:
#if PLAYER_TYPE
              if (mainSettings.knockSound) playerSetTrackNow(PLAYER_FAST_MENU_START + FAST_BACKL_COLOR, PLAYER_MENU_FOLDER);
#endif
              mode = FAST_BACKL_COLOR;
              break;
          }
        }
        _timer_ms[TMR_MS] = FAST_BACKL_TIME;
        anim = 0;
        break;
#endif
      case RIGHT_KEY_PRESS: //клик правой кнопкой
        if (mode == FAST_BACKL_COLOR) {
          if (fastSettings.backlColor < 250) fastSettings.backlColor += 10; else fastSettings.backlColor = 0;
          setLedHue(fastSettings.backlColor); //устанавливаем статичный цвет
          showLeds(); //отрисовка светодиодов
          _timer_ms[TMR_MS] = FAST_BACKL_TIME;
        }
        else {
          if (mode != FAST_FLIP_MODE) {
#if PLAYER_TYPE
            if (mainSettings.knockSound) playerSetTrackNow(PLAYER_FAST_MENU_START + FAST_FLIP_MODE, PLAYER_MENU_FOLDER);
#endif
            mode = FAST_FLIP_MODE; //демострация текущего режима работы
          }
          else if (++fastSettings.flipMode > (FLIP_EFFECT_NUM + 1)) fastSettings.flipMode = 0;
          _timer_ms[TMR_MS] = FAST_FLIP_TIME;
        }
        anim = 0;
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
        if (mode == FAST_BACKL_COLOR) {
          if (fastSettings.backlColor > 0) fastSettings.backlColor -= 10; else fastSettings.backlColor = 250;
          setLedHue(fastSettings.backlColor); //устанавливаем статичный цвет
          showLeds(); //отрисовка светодиодов
          _timer_ms[TMR_MS] = FAST_BACKL_TIME;
        }
        else {
          if (mode != FAST_DOT_MODE) {
#if PLAYER_TYPE
            if (mainSettings.knockSound) playerSetTrackNow(PLAYER_FAST_MENU_START + FAST_DOT_MODE, PLAYER_MENU_FOLDER);
#endif
            mode = FAST_DOT_MODE; //демострация текущего режима работы
          }
          else if (++fastSettings.dotMode > DOT_PULS) fastSettings.dotMode = 0;
          _timer_ms[TMR_MS] = FAST_DOT_TIME;
        }
        anim = 0;
        break;
    }
  }
  if (mode == 1) flipIndi(fastSettings.flipMode, 1); //демонстрация анимации цифр
  updateData((uint8_t*)&fastSettings, sizeof(fastSettings), EEPROM_BLOCK_SETTINGS_FAST, EEPROM_BLOCK_CRC_FAST); //записываем настройки яркости в память
}
//---------------------------Настройка громкости радио----------------------------------
boolean radioVolSettings(void) //настройка громкости радио
{
  boolean _state = 0;
  _timer_ms[TMR_MS] = 0; //сбросили таймер

  while (1) {
    dataUpdate(); //обработка данных

    if (!_timer_ms[TMR_MS]) { //если таймер истек
      _timer_ms[TMR_MS] = RADIO_VOL_TIME; //устанавливаем таймер
      if (_state) return 0;
      indiClr(); //очистка индикаторов
      indiPrintNum(radioSettings.volume, ((LAMP_NUM / 2) - 1), 2, 0); //номер станции
      _state = 1;
    }

    switch (buttonState()) {
      case RIGHT_KEY_PRESS: //клик правой кнопкой
        if (radioSettings.volume < RADIO_MAX_VOL) radioSettings.volume++;
        _state = 0;
        _timer_ms[TMR_MS] = 0; //сбросили таймер
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
        if (radioSettings.volume > RADIO_MIN_VOL) radioSettings.volume--;
        _state = 0;
        _timer_ms[TMR_MS] = 0; //сбросили таймер
        break;

      case ADD_KEY_PRESS: //клик дополнительной кнопкой
        return 0; //выходим

      case SET_KEY_PRESS: //клик средней кнопкой
        return 1; //выходим
    }
  }
}
//------------------------Остановка автопоиска радиостанции-----------------------------
void radioSeekStop(void) //остановка автопоиска радиостанции
{
  stopSeekRDA(); //остановили поиск радио
  clrSeekCompleteStatusRDA(); //очищаем флаг окончания поиска
  setFreqRDA(radioSettings.stationsFreq); //устанавливаем частоту
  setMuteRDA(RDA_MUTE_OFF); //выключаем приглушение звука
}
//-------------------------Включить питание радиоприемника------------------------------
void radioPowerOn(void) //включить питание радиоприемника
{
  setPowerRDA(RDA_ON); //включаем радио
  setVolumeRDA(radioSettings.volume); //устанавливаем громкость
  setFreqRDA(radioSettings.stationsFreq); //устанавливаем частоту
}
//---------------------------------Радиоприемник----------------------------------------
void radioMenu(void) //радиоприемник
{
  if (getPowerStatusRDA() != RDA_ERROR) { //если радиоприемник доступен
#if PLAYER_TYPE
    boolean power_state = 0; //состояние питания радио
#endif
    uint8_t time_out = 0; //таймаут автовыхода
    uint8_t seek_run = 0; //флаг поиска
    uint16_t seek_freq = 0; //частота поиска
    boolean station_show = 0; //флаг анимации номера станции

    if (getPowerStatusRDA() == RDA_OFF) { //если радио выключено
#if PLAYER_TYPE
      if (mainSettings.knockSound) playerSetTrackNow(PLAYER_RADIO_SOUND, PLAYER_GENERAL_FOLDER);
      playerSetMute(PLAYER_MUTE_ON); //включаем приглушение звука плеера
      power_state = 1;
#else
      radioPowerOn(); //включить питание радиоприемника
#endif
    }

    _timer_ms[TMR_MS] = 0; //сбросили таймер

    while (1) {
      dataUpdate(); //обработка данных

#if PLAYER_TYPE
      if (power_state) {
        if (!playerWriteStatus()) {
          radioPowerOn(); //включить питание радиоприемника
          power_state = 0;
        }
      }
#endif

      if (!secUpd) { //если прошла секунда
        secUpd = 1; //сбросили флаг секунды
        if (++time_out >= RADIO_TIMEOUT) { //если время вышло
          if (seek_run) radioSeekStop(); //остановка автопоиска радиостанции
          updateData((uint8_t*)&radioSettings, sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO); //записываем настройки радио в память
          return; //выходим по тайм-ауту
        }
      }

      if (!_timer_ms[TMR_MS]) { //если таймер истек
        _timer_ms[TMR_MS] = RADIO_UPDATE_TIME; //устанавливаем таймер

        if (!seek_run) dotSetBright((getStationStatusRDA()) ? dotMaxBright : 0); //управление точками в зависимости от устойчивости сигнала
        else { //иначе идет автопоиск
          if (getSeekCompleteStatusRDA()) { //если поиск завершился
            clrSeekCompleteStatusRDA(); //очищаем флаг окончания поиска
            seek_freq = getFreqRDA(); //прочитали частоту
          }
          switch (seek_run) {
            case 1: if (radioSettings.stationsFreq > seek_freq) radioSettings.stationsFreq--; else seek_run = 0; break;
            case 2: if (radioSettings.stationsFreq < seek_freq) radioSettings.stationsFreq++; else seek_run = 0; break;
          }
          if (!seek_run) {
            setMuteRDA(RDA_MUTE_OFF); //выключаем приглушение звука
            radioSettings.stationsFreq = seek_freq; //прочитали частоту
          }
          else _timer_ms[TMR_MS] = RADIO_ANIM_TIME; //устанавливаем таймер
        }

        indiClr(); //очистка индикаторов
        if (station_show) { //если нужно показать номер станции
          station_show = 0; //сбросили флага показа номера станции
          _timer_ms[TMR_MS] = RADIO_SHOW_TIME; //устанавливаем таймер
          indiPrintNum(radioSettings.stationNum + 1, ((LAMP_NUM / 2) - 1), 2, 0); //номер станции
        }
        else {
#if DOTS_PORT_ENABLE
          indiSetDots(3); //включаем разделителную точку
#endif
          indiPrintNum(radioSettings.stationsFreq, 0, 4); //текущаяя частота
          indiPrintNum(radioSettings.stationNum + 1, 5); //номер станции
        }
      }

      switch (buttonState()) {
        case RIGHT_KEY_PRESS: //клик правой кнопкой
          if (seek_run) { //если идет поиск
            seek_run = 0; //выключили поиск
            radioSeekStop(); //остановка автопоиска радиостанции
          }
          if (radioSettings.stationsFreq < RADIO_MAX_FREQ) radioSettings.stationsFreq++; else radioSettings.stationsFreq = RADIO_MIN_FREQ; //переключаем частоту
          setFreqRDA(radioSettings.stationsFreq); //установили частоту
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;

        case LEFT_KEY_PRESS: //клик левой кнопкой
          if (seek_run) { //если идет поиск
            seek_run = 0; //выключили поиск
            radioSeekStop(); //остановка автопоиска радиостанции
          }
          if (radioSettings.stationsFreq > RADIO_MIN_FREQ) radioSettings.stationsFreq--; else radioSettings.stationsFreq = RADIO_MAX_FREQ; //переключаем частоту
          setFreqRDA(radioSettings.stationsFreq); //установили частоту
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;

        case ADD_KEY_PRESS: //клик дополнительной кнопкой
          if (seek_run) { //если идет поиск
            seek_run = 0; //выключили поиск
            radioSeekStop(); //остановка автопоиска радиостанции
          }
          if (radioSettings.stationNum < 8) radioSettings.stationNum++; else radioSettings.stationNum = 0; //переключаем станцию
          if (radioSettings.stationsSave[radioSettings.stationNum]) { //если в памяти записана частота
            radioSettings.stationsFreq = radioSettings.stationsSave[radioSettings.stationNum]; //прочитали частоту
            setFreqRDA(radioSettings.stationsFreq); //установили частоту
          }
          station_show = 1; //подняли флаг отображения номера станции
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;

        case SET_KEY_PRESS: //клик средней кнопкой
          if (seek_run) { //если идет поиск
            seek_run = 0; //выключили поиск
            radioSeekStop(); //остановка автопоиска радиостанции
          }
#if PLAYER_TYPE
          if (power_state) {
            playerSetMute(PLAYER_MUTE_ON); //включаем приглушение звука плеера
            radioPowerOn(); //включить питание радиоприемника
          }
#endif
          if (radioVolSettings()) { //настройка громкости радио
            if (seek_run) radioSeekStop(); //остановка автопоиска радиостанции
            updateData((uint8_t*)&radioSettings, sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO); //записываем настройки радио в память
            return; //выходим
          }
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;

        case RIGHT_KEY_HOLD: //удержание правой кнопки
          if (!seek_run) { //если не идет поиск
            seek_run = 2; //включили поиск
            seek_freq = RADIO_MAX_FREQ; //установили максимальную частоту
            setMuteRDA(RDA_MUTE_ON); //включаем приглушение звука
            startSeekRDA(RDA_SEEK_UP); //начинаем поиск вверх
            dotSetBright(0); //выключаем точки
          }
          else {
            seek_run = 0; //выключили поиск
            radioSeekStop(); //остановка автопоиска радиостанции
          }
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = RADIO_ANIM_TIME; //устанавливаем таймер
          break;

        case LEFT_KEY_HOLD: //удержание левой кнопки
          if (!seek_run) { //если не идет поиск
            seek_run = 1; //включили поиск
            seek_freq = RADIO_MIN_FREQ; //установили минимальную частоту
            setMuteRDA(RDA_MUTE_ON); //включаем приглушение звука
            startSeekRDA(RDA_SEEK_DOWN); //начинаем поиск вниз
            dotSetBright(0); //выключаем точки
          }
          else {
            seek_run = 0; //выключили поиск
            radioSeekStop(); //остановка автопоиска радиостанции
          }
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = RADIO_ANIM_TIME; //устанавливаем таймер
          break;

        case ADD_KEY_HOLD: //удержание дополнительной кнопки
          if (!seek_run) { //если не идет поиск
#if !PLAYER_TYPE
            buzz_pulse(RADIO_SAVE_SOUND_FREQ, RADIO_SAVE_SOUND_TIME); //сигнал успешной записи радиостанции в память
#endif
            radioSettings.stationsSave[radioSettings.stationNum] = radioSettings.stationsFreq; //записали частоту в память
            station_show = 1; //подняли флаг отображения номера станции
          }
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;

        case SET_KEY_HOLD: //удержание средней кнопк
#if PLAYER_TYPE
          playerSetMute(PLAYER_MUTE_OFF); //выключаем приглушение звука плеера
#endif
          setPowerRDA(RDA_OFF); //выключили радио
          updateData((uint8_t*)&radioSettings, sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO); //записываем настройки радио в память
          return; //выходим
      }
    }
  }
}
//--------------------------------Тревога таймера----------------------------------------
void timerWarn(void) //тревога таймера
{
  boolean blink_data = 0; //флаг мигания индикаторами
  if (timerMode == 2 && !timerCnt) {
#if PLAYER_TYPE
    playerStop(); //сброс позиции мелодии
#else
    melodyPlay(SOUND_TIMER_WARN, SOUND_LINK(general_sound), REPLAY_CYCLE); //звук окончания таймера
#endif
    while (!buttonState()) { //ждем
      dataUpdate(); //обработка данных
#if PLAYER_TYPE
      if (!playerWriteStatus()) playerSetTrack(PLAYER_TIMER_WARN_SOUND, PLAYER_GENERAL_FOLDER);
#endif
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
    animsReset(); //сброс анимаций
  }
}
//----------------------------Настройки таймера----------------------------------
void timerSettings(void) //настройки таймера
{
  boolean mode = 0; //текущий режим
  boolean blink_data = 0; //флаг мигания индикаторами

#if PLAYER_TYPE
  if (mainSettings.knockSound) playerSetTrackNow(PLAYER_TIMER_SET_SOUND, PLAYER_GENERAL_FOLDER);
#endif

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

    switch (buttonState()) {
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
  uint16_t msTmr = 10000UL / debugSettings.timePeriod * 2; //расчет хода миллисекунд
  static uint16_t millisCnt; //счетчик миллисекун

  if (timerMode & 0x7F) mode = (timerMode & 0x7F) - 1; //если таймер был запущен
  else timerCnt = 0; //иначе сбрасываем таймер

  secUpd = 0; //обновление экрана

#if PLAYER_TYPE
  if (mainSettings.knockSound) playerSetTrackNow((mode) ? PLAYER_TIMER_SOUND : PLAYER_STOPWATCH_SOUND, PLAYER_GENERAL_FOLDER);
#endif

  while (1) {
    dataUpdate(); //обработка данных
    timerWarn(); //тревога таймера
#if LAMP_NUM > 4
    dotFlashMode((!timerCnt) ? 0 : ((timerMode > 2 || !timerMode ) ? 1 : 3)); //мигание точек по умолчанию
#else
    dotFlashMode((!timerMode) ? 0 : ((timerMode > 2) ? 1 : 3)); //мигание точек по умолчанию
#endif

    if (!secUpd) {
      secUpd = 1; //сбрасываем флаг
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

    switch (buttonState()) {
      case SET_KEY_PRESS: //клик средней кнопкой
        if (mode && !timerMode) {
          timerSettings(); //настройки таймера
          time_out = 0; //сбрасываем таймер автовыхода
          secUpd = 0; //обновление экрана
        }
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        if (timerMode == 1) timerMode |= 0x80; //приостановка секундомера
        return; //выходим

      case RIGHT_KEY_PRESS: //клик правой кнопкой
      case RIGHT_KEY_HOLD: //удержание правой кнопки
#if PLAYER_TYPE
        if (mainSettings.knockSound) playerSetTrackNow(PLAYER_TIMER_SOUND, PLAYER_GENERAL_FOLDER);
#endif
        mode = 1; //переключаем режим
        timerMode = 0; //деактивируем таймер
        timerCnt = timerTime; //сбрасываем таймер
        time_out = 0; //сбрасываем таймер автовыхода
        secUpd = 0; //обновление экрана
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
      case LEFT_KEY_HOLD: //удержание левой кнопки
#if PLAYER_TYPE
        if (mainSettings.knockSound) playerSetTrackNow(PLAYER_STOPWATCH_SOUND, PLAYER_GENERAL_FOLDER);
#endif
        mode = 0; //переключаем режим
        timerMode = 0; //деактивируем таймер
        timerCnt = 0; //сбрасываем секундомер
        time_out = 0; //сбрасываем таймер автовыхода
        secUpd = 0; //обновление экрана
        break;

      case ADD_KEY_PRESS: //клик дополнительной кнопкой
        if (!timerMode) {
          millisCnt = 0; //сбрасываем счетчик миллисекунд
          timerMode = mode + 1;
        }
        else timerMode ^= 0x80; //приостановка таймера/секундомера
        time_out = 0; //сбрасываем таймер автовыхода
        secUpd = 0; //обновление экрана
        break;

      case ADD_KEY_HOLD: //удержание дополнительной кнопки
        timerMode = 0; //деактивируем таймер
        switch (mode & 0x01) {
          case 0: timerCnt = 0; break; //сбрасываем секундомер
          case 1: timerCnt = timerTime; break; //сбрасываем таймер
        }
        time_out = 0; //сбрасываем таймер автовыхода
        secUpd = 0; //обновление экрана
        break;
    }
  }
}
//------------------------------------Сброс анимаций-------------------------------------
void animsReset(void) //сброс анимаций
{
#if DOTS_PORT_ENABLE
  indiClrDots(); //выключаем разделительные точки
#endif
  _timer_sec[TMR_GLITCH] = random(GLITCH_MIN_TIME, GLITCH_MAX_TIME); //находим рандомное время появления глюка
  _timer_sec[TMR_TEMP] = mainSettings.autoTempTime; //устанавливаем таймер автопоказа температуры
  animShow = 0; //сбрасываем флаг анимации цифр
  secUpd = 0; //обновление экрана
}
//------------------------------------Звук смены часа------------------------------------
void hourSound(void) //звук смены часа
{
  if (!alarm || alarmWaint) { //если будильник не работает
    if (checkHourStrart(mainSettings.timeHour[0], mainSettings.timeHour[1])) {
#if PLAYER_TYPE
      if (mainSettings.knockSound) speakTime(); //воспроизвести время
      else playerSetTrackNow(PLAYER_HOUR_SOUND, PLAYER_GENERAL_FOLDER); //звук смены часа
#else
      melodyPlay(SOUND_HOUR, SOUND_LINK(general_sound), REPLAY_ONCE); //звук смены часа
#endif
    }
  }
}
//------------------------------------Имитация глюков------------------------------------
void glitchMode(void) //имитация глюков
{
  if (mainSettings.glitchMode) { //если глюки включены
    if (!_timer_sec[TMR_GLITCH] && RTC.s > 7 && RTC.s < 55) { //если пришло время
      boolean indiState = 0; //состояние индикатора
      uint8_t glitchCounter = random(GLITCH_NUM_MIN, GLITCH_NUM_MAX); //максимальное количество глюков
      uint8_t glitchIndic = random(0, LAMP_NUM); //номер индикатора
      uint8_t indiSave = indiGet(glitchIndic); //сохраняем текущую цифру в индикаторе
      while (!buttonState()) {
        dataUpdate(); //обработка данных
        dotFlash(); //мигаем точками
#if LAMP_NUM > 4 && SECONDS_ANIM
        if (!indiState) tickSecs(); //анимация секунд
#endif

        if (!_timer_ms[TMR_ANIM]) { //если таймер истек
          if (!indiState) indiClr(glitchIndic); //выключаем индикатор
          else indiSet(indiSave, glitchIndic); //включаем индикатор
          indiState = !indiState; //меняем состояние глюка лампы
          _timer_ms[TMR_ANIM] = random(1, 6) * GLITCH_TIME; //перезапускаем таймер глюка
          if (!glitchCounter--) break; //выходим если закончились глюки
        }
      }
      _timer_sec[TMR_GLITCH] = random(GLITCH_MIN_TIME, GLITCH_MAX_TIME); //находим рандомное время появления глюка
      indiSet(indiSave, glitchIndic); //восстанавливаем состояние индикатора
    }
  }
}
//----------------------------Антиотравление индикаторов-------------------------------
void burnIndi(void) //антиотравление индикаторов
{
  if (!_timer_sec[TMR_BURN] && RTC.s >= BURN_PHASE) {
    _timer_sec[TMR_BURN] = (uint16_t)(BURN_PERIOD * 60); //устанавливаем таймер

    uint8_t indi = 0;
    switch (mainSettings.burnMode) {
      case BURN_ALL:
      case BURN_SINGLE:
        dotSetBright(0); //выключаем точки
        break;
    }

    while (1) {
      if (mainSettings.burnMode == BURN_SINGLE) indiClr(); //очистка индикаторов
      for (uint8_t loops = 0; loops < BURN_LOOPS; loops++) {
        for (uint8_t digit = 0; digit < 10; digit++) {
          switch (mainSettings.burnMode) {
            case BURN_ALL:
              for (indi = 0; indi < LAMP_NUM; indi++) indiPrintNum(cathodeMask[digit], indi); //отрисовываем цифру
              break;
            case BURN_SINGLE:
            case BURN_SINGLE_TIME:
              indiPrintNum(cathodeMask[digit], indi); //отрисовываем цифру
              break;
          }
          for (_timer_ms[TMR_MS] = BURN_TIME; _timer_ms[TMR_MS];) { //ждем
            if (buttonState()) return; //если нажата кнопка выходим
            dataUpdate(); //обработка данных
          }
        }
        if (mainSettings.burnMode == BURN_SINGLE_TIME) {
          indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
          indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
#if LAMP_NUM > 4
          indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
#endif
        }
      }
      if (mainSettings.burnMode == BURN_ALL || (++indi >= LAMP_NUM)) return;
    }
  }
}
//----------------------------------Анимация цифр-----------------------------------
void flipIndi(uint8_t flipMode, boolean demo) //анимация цифр
{
  switch (flipMode) {
    case 0: return; //без анимации
    case 1: if (demo) return; else flipMode = random(0, FLIP_EFFECT_NUM); break; //случайный режим
    default: flipMode -= 2; break; //выбранный режим
  }

  uint8_t anim_buf[12]; //буфер разрядов
  uint8_t changeIndi = 0; //флаги разрядов
  uint8_t HH = (RTC.m) ? RTC.h : ((RTC.h) ? (RTC.h - 1) : 23); //предыдущий час
  uint8_t MM = (RTC.m) ? (RTC.m - 1) : 59; //предыдущая минута

  if (!demo) { //если не демонстрация
    if (RTC.h / 10 != HH / 10) changeIndi |= (0x01 << 0); //установили флаг разряда
    if (RTC.h % 10 != HH % 10) changeIndi |= (0x01 << 1); //установили флаг разряда
    if (RTC.m / 10 != MM / 10) changeIndi |= (0x01 << 2); //установили флаг разряда
    if (RTC.m % 10 != MM % 10) changeIndi |= (0x01 << 3); //установили флаг разряда
#if LAMP_NUM > 4
    changeIndi |= (0x01 << 4); //установили флаг разряда
    changeIndi |= (0x01 << 5); //установили флаг разряда
#endif
  }
  else { //иначе режим демонстрации
    indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
    indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
#if LAMP_NUM > 4
    indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
    changeIndi = 0x3F; //установили флаги разрядов
#else
    changeIndi = 0x0F; //установили флаги разрядов
#endif
  }

  switch (flipMode) { //режим анимации перелистывания
    case FLIP_BRIGHT: { //плавное угасание и появление
        uint16_t timer_anim = FLIP_MODE_2_TIME / indiMaxBright; //расчёт шага яркости режима 2

        anim_buf[0] = indiMaxBright;
        anim_buf[1] = 0;

        while (!buttonState()) {
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
            _timer_ms[TMR_ANIM] = timer_anim; //устанавливаем таймер
          }
        }
        indiSetBright(indiMaxBright); //возвращаем максимальную яркость
      }
      break;
    case FLIP_ORDER_OF_NUMBERS: { //перемотка по порядку числа
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

        while (!buttonState()) {
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
      }
      break;
    case FLIP_ORDER_OF_CATHODES: { //перемотка по порядку катодов в лампе
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

        while (!buttonState()) {
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
      }
      break;
    case FLIP_TRAIN: { //поезд
        //старое время
        uint16_t old_time = (uint16_t)(HH * 100) + MM;
        //новое время
        uint16_t new_time = (uint16_t)(RTC.h * 100) + RTC.m;

        for (uint8_t c = 0; c < 2; c++) {
          for (uint8_t i = 0; i < LAMP_NUM;) {
            dataUpdate(); //обработка данных
            dotFlash(); //мигаем точками

            if (buttonState()) return; //возврат если нажата кнопка
            if (!_timer_ms[TMR_ANIM]) { //если таймер истек
              indiClr(); //очистка индикатора
              switch (c) {
                case 0:
                  indiPrintNum(old_time, i + 1, 4, 0); //вывод часов
#if LAMP_NUM > 4
                  indiPrintNum(59, i + 5, 2, 0); //вывод секунд
#endif
                  break;
                case 1:
                  indiPrintNum(new_time, i - (LAMP_NUM - 1), 4, 0); //вывод часов
#if LAMP_NUM > 4
                  indiPrintNum(RTC.s, i - 1, 2, 0); //вывод секунд
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
    case FLIP_RUBBER_BAND: { //резинка
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

            if (buttonState()) return; //возврат если нажата кнопка
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
      }
      break;
    case FLIP_GATES: { //ворота
#if LAMP_NUM > 4
        //старое время
        uint16_t old_time_left = (uint16_t)(HH * 10) + (MM / 10);
        uint16_t old_time_right = (uint16_t)((MM % 10) * 100) + 59;
        //новое время
        uint16_t new_time_left = (uint16_t)(RTC.h * 10) + (RTC.m / 10);
        uint16_t new_time_right = (uint16_t)((RTC.m % 10) * 100);
#else
        //старое время
        uint16_t old_time_left = (uint16_t)HH;
        uint16_t old_time_right = (uint16_t)MM;
        //новое время
        uint16_t new_time_left = (uint16_t)RTC.h;
        uint16_t new_time_right = (uint16_t)RTC.m;
#endif

        for (uint8_t c = 0; c < 2; c++) {
          for (uint8_t i = 0; i < ((LAMP_NUM / 2) + 1);) {
            dataUpdate(); //обработка данных
            dotFlash(); //мигаем точками

            if (buttonState()) return; //возврат если нажата кнопка
            if (!_timer_ms[TMR_ANIM]) { //если таймер истек
              indiClr(); //очистка индикатора
              switch (c) {
                case 0:
                  indiPrintNum(old_time_left, -i, (LAMP_NUM / 2), 0); //вывод часов
                  indiPrintNum(old_time_right, i + (LAMP_NUM / 2), (LAMP_NUM / 2), 0); //вывод часов
                  break;
                case 1:
                  indiPrintNum(new_time_left, i - (LAMP_NUM / 2), (LAMP_NUM / 2), 0); //вывод часов
#if LAMP_NUM > 4
                  indiPrintNum(new_time_right + RTC.s, LAMP_NUM - i, (LAMP_NUM / 2), 0); //вывод часов
#else
                  indiPrintNum(new_time_right, LAMP_NUM - i, (LAMP_NUM / 2), 0); //вывод часов
#endif
                  break;
              }
              i++; //прибавляем цикл
              _timer_ms[TMR_ANIM] = FLIP_MODE_7_TIME; //устанавливаем таймер
            }
          }
        }
      }
      break;
    case FLIP_WAVE: { //волна
        //новое время
        anim_buf[0] = RTC.h / 10; //часы
        anim_buf[1] = RTC.h % 10; //часы
        anim_buf[2] = RTC.m / 10; //минуты
        anim_buf[3] = RTC.m % 10; //минуты

        for (uint8_t c = 0; c < 2; c++) {
          for (uint8_t i = 0; i < LAMP_NUM;) {
            dataUpdate(); //обработка данных
            dotFlash(); //мигаем точками

            if (buttonState()) return; //возврат если нажата кнопка
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
              _timer_ms[TMR_ANIM] = FLIP_MODE_8_TIME; //устанавливаем таймер
            }
          }
        }
      }
      break;
    case FLIP_HIGHLIGHTS: { //блики
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

            if (buttonState()) return; //возврат если нажата кнопка
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
              _timer_ms[TMR_ANIM] = FLIP_MODE_9_TIME; //устанавливаем таймер
            }
          }
        }
      }
      break;
    case FLIP_EVAPORATION: { //испарение
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

            if (buttonState()) return; //возврат если нажата кнопка
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
              _timer_ms[TMR_ANIM] = FLIP_MODE_10_TIME; //устанавливаем таймер
            }
          }
        }
      }
      break;
  }
}
//-----------------------------Анимация секунд------------------------------------------------
void tickSecs(void) //анимация секунд
{
  static uint8_t time_flip; //флаги анимации секунд
  static uint8_t time_old; //предыдущее время
  static uint8_t time_anim[4]; //буфер анимации

  if (time_old != RTC.s) { //если сменились секунды
    time_anim[0] = time_old % 10; //старые секунды
    time_anim[1] = time_old / 10; //старые секунды
    time_anim[2] = RTC.s % 10; //новые секунды
    time_anim[3] = RTC.s / 10; //новые секунды
    time_old = RTC.s; //запоминаем текущее время
    time_flip = 0x03; //устанавливаем флаги анимации
  }
  if (time_flip && !_timer_ms[TMR_MS]) { //если анимация активна и пришло время
    _timer_ms[TMR_MS] = SECONDS_ANIM_TIME; //установили таймер
    for (uint8_t i = 0; i < 2; i++) { //перебираем все цифры
      if (time_anim[i] != time_anim[i + 2]) { //если не достигли конца анимации разряда
        if (--time_anim[i] > 9) time_anim[i] = 9; //меняем цифру разряда
      }
      else time_flip &= ~(0x01 << i); //иначе завершаем анимацию для разряда
      indiPrintNum(time_anim[i], (LAMP_NUM - 1) - i); //вывод секунд
    }
  }
}
//-----------------------------Главный экран------------------------------------------------
void mainScreen(void) //главный экран
{
  for (;;) { //основной цикл
    dataUpdate(); //обработка данных
    dotFlash(); //мигаем точками

#if LIGHT_SENS_ENABLE
    lightSensUpdate(); //обработка сенсора яркости освещения
#endif

    if (!secUpd) { //если пришло время обновить индикаторы
#if BTN_ADD_TYPE || IR_PORT_ENABLE
      timerWarn(); //тревога таймера
#endif
#if ALARM_TYPE
      alarmWarn(); //тревога будильника
#endif
      if (animShow) flipIndi(fastSettings.flipMode, 0); //анимация цифр основная

      burnIndi(); //антиотравление индикаторов
      glitchMode(); //имитация глюков
      autoShowTemp(); //автоматический показ температуры

      indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
      indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
#if LAMP_NUM > 4 && !SECONDS_ANIM
      indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
#endif
      secUpd = 1; //сбрасываем флаг
      animShow = 0; //сбрасываем флаг анимации
    }

#if LAMP_NUM > 4 && SECONDS_ANIM
    tickSecs(); //анимация секунд
#endif

    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        showTemp(); //показать температуру
        animsReset(); //сброс анимаций
        break;

#if ALARM_TYPE
      case LEFT_KEY_HOLD: //удержание левой кнопки
#if ALARM_TYPE == 1
        settings_singleAlarm(); //настройка будильника
#else
        settings_multiAlarm(); //настройка будильников
#endif
        animsReset(); //сброс анимаций
        break;
#endif

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        showDate(); //показать дату
        animsReset(); //сброс анимаций
        break;

      case RIGHT_KEY_HOLD: //удержание правой кнопки
        settings_time(); //иначе настройки времени
        animsReset(); //сброс анимаций
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        fastSetSwitch(); //переключение настроек
        animsReset(); //сброс анимаций
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        if (alarmWaint) {
#if PLAYER_TYPE
          playerSetTrackNow(PLAYER_ALARM_DISABLE_SOUND, PLAYER_GENERAL_FOLDER);
#else
          melodyPlay(SOUND_ALARM_DISABLE, SOUND_LINK(general_sound), REPLAY_ONCE); //звук выключения будильника
#endif
          alarmReset(); //сброс будильника
        }
        else settings_main(); //настроки основные
        animsReset(); //сброс анимаций
        break;

#if BTN_ADD_TYPE || IR_PORT_ENABLE
      case ADD_KEY_PRESS: //клик дополнительной кнопкой
        timerStopwatch(); //таймер-секундомер
        animsReset(); //сброс анимаций
        break;

      case ADD_KEY_HOLD: //удержание дополнительной кнопки
        if (alarmWaint) {
#if PLAYER_TYPE
          playerSetTrackNow(PLAYER_ALARM_DISABLE_SOUND, PLAYER_GENERAL_FOLDER);
#else
          melodyPlay(SOUND_ALARM_DISABLE, SOUND_LINK(general_sound), REPLAY_ONCE); //звук выключения будильника
#endif
          alarmReset(); //сброс будильника
        }
        else radioMenu(); //радиоприемник
        animsReset(); //сброс анимаций
        break;
#endif
    }
  }
}
