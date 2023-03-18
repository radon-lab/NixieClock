/*
  Arduino IDE 1.8.13 версия прошивки 1.8.9 релиз от 18.03.23
  Специльно для проекта "Часы на ГРИ и Arduino v2 | AlexGyver"
  Страница проекта - https://alexgyver.ru/nixieclock_v2

  Исходник - https://github.com/radon-lab/NixieClock
  Автор Radon-lab.
*/
//-----------------Ошибки-----------------
enum {
  DS3231_ERROR,        //0001 - ошибка связи с модулем DS3231
  DS3231_OSF_ERROR,    //0002 - ошибка осцилятора модуля DS3231
  SQW_SHORT_ERROR,     //0003 - ошибка слишком короткий сигнал SQW
  SQW_LONG_ERROR,      //0004 - ошибка сигнал SQW отсутсвует или слишком длинный сигнал SQW
  TEMP_SENS_ERROR,     //0005 - ошибка выбранный датчик температуры не обнаружен
  VCC_ERROR,           //0006 - ошибка напряжения питания
  MEMORY_ERROR,        //0007 - ошибка памяти еепром
  RESET_ERROR,         //0008 - ошибка софтовой перезагрузки
  CONVERTER_ERROR,     //0009 - ошибка сбоя работы преобразователя
  PWM_OVF_ERROR,       //0010 - ошибка переполнения заполнения шим преобразователя
  STACK_OVF_ERROR,     //0011 - ошибка переполнения стека
  TICK_OVF_ERROR,      //0012 - ошибка переполнения тиков времени
  INDI_ERROR           //0013 - ошибка сбоя работы динамической индикации
};
void dataUpdate(void); //процедура обработки данных
void SET_ERROR(uint8_t err); //процедура установка ошибки

//-----------------Таймеры------------------
enum {
  TMR_MS,        //таймер общего назначения
  TMR_IR,        //таймер инфракрасного приемника
  TMR_SENS,      //таймер сенсоров температуры
  TMR_PLAYER,    //таймер плеера/мелодий
  TMR_LIGHT,     //таймер сенсора яркости
  TMR_BACKL,     //таймер подсветки
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
  TMR_SLEEP,     //таймер ухода в сон
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
#include "SHT.h"
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
  uint8_t timeSleep[2] = {DEFAULT_SLEEP_WAKE_TIME_N, DEFAULT_SLEEP_WAKE_TIME}; //время перехода яркости
  boolean timeFormat = DEFAULT_TIME_FORMAT; //формат времени
  boolean knockSound = DEFAULT_KNOCK_SOUND; //звук кнопок или озвучка
  uint8_t volumeSound = CONSTRAIN((uint8_t)(PLAYER_MAX_VOL * (DEFAULT_PLAYER_VOLUME / 100.0)), PLAYER_MIN_VOL, PLAYER_MAX_VOL); //громкость озвучки
  int8_t tempCorrect = DEFAULT_TEMP_CORRECT; //коррекция температуры
  boolean glitchMode = DEFAULT_GLITCH_MODE; //режим глюков
  uint8_t autoTempTime = DEFAULT_AUTO_TEMP_TIME; //интервал времени показа температуры
  uint8_t autoTempFlip = DEFAULT_AUTO_TEMP_ANIM; //режим анимации показа температуры
  uint8_t burnMode = DEFAULT_BURN_MODE; //режим антиотравления индикаторов
  uint8_t secsMode = DEFAULT_SECONDS_ANIM; //режим анимации секунд индикаторов
} mainSettings;

struct Settings_2 {
  uint8_t flipMode = DEFAULT_FLIP_ANIM; //режим анимации
  uint8_t backlMode = DEFAULT_BACKL_MODE; //режим подсветки
  uint8_t backlColor = (DEFAULT_BACKL_COLOR) ? ((DEFAULT_BACKL_COLOR - 1) * 10) : 255; //цвет подсветки
  uint8_t dotMode = DEFAULT_DOT_MODE; //режим точек
} fastSettings;

struct Settings_3 { //настройки радио
  uint16_t stationsSave[RADIO_MAX_STATIONS] = {DEFAULT_RADIO_STATIONS};
  uint16_t stationsFreq = RADIO_MIN_FREQ;
  uint8_t volume = CONSTRAIN((uint8_t)(RADIO_MAX_VOL * (DEFAULT_RADIO_VOLUME / 100.0)), RADIO_MIN_VOL, RADIO_MAX_VOL);
  uint8_t stationNum;

} radioSettings;


//переменные обработки кнопок
struct radioData {
  boolean powerState; //текущее состояние радио
  uint8_t seekRun; //флаг автопоиска радио
} radio;

//переменные обработки кнопок
struct buttonData {
  uint8_t state; //текущее состояние кнопок
  uint8_t adc; //результат опроса аналоговых кнопок
} btn;
uint8_t analogState; //флаги обновления аналоговых портов
uint16_t vcc_adc; //напряжение питания

//переменные работы с точками
struct dotData {
  boolean update; //флаг обновления точек
  boolean drive; //направление яркости
  uint8_t count; //счетчик мигания точки
#if DOTS_PORT_ENABLE
  uint8_t steps; //шаги точек
#endif
  uint8_t brightStep; //шаг мигания точек
  uint8_t brightTime; //период шага мигания точек
  uint8_t maxBright; //максимальная яркость точек
  uint8_t menuBright; //максимальная яркость точек в меню
} dot;

//переменные работы с подсветкой
struct backlightData {
  boolean drive; //направление огня
  uint8_t steps; //шаги затухания
  uint8_t color; //номер цвета
  uint8_t position; //положение огня
  uint8_t maxBright; //максимальная яркость подсветки
  uint8_t minBright; //минимальная яркость подсветки
  uint8_t menuBright; //максимальная яркость подсветки в меню
  uint16_t mode_2_time; //время эффекта номер 2
  uint8_t mode_2_step; //шаг эффекта номер 2
  uint8_t mode_4_step; //шаг эффекта номер 4
  uint16_t mode_8_time; //время эффекта номер 6
  uint8_t mode_8_step; //шаг эффекта номер 6
} backl;

//переменные работы с индикаторами
struct indiData {
  uint8_t sleepMode; //флаг режима сна индикаторов
  uint8_t maxBright; //максимальная яркость индикаторов
} indi;

//флаги анимаций
uint8_t changeBrightState; //флаг состояния смены яркости подсветки
uint8_t changeAnimState; //флаг состояния анимаций
uint8_t animShow; //флаг анимации смены времени
boolean secUpd; //флаг обновления секунды

//alarmRead/Write - час | минута | режим(0 - выкл, 1 - одиночный, 2 - вкл, 3 - по будням, 4 - по дням недели) | день недели(вс,сб,пт,чт,ср,вт,пн,null) | мелодия будильника | громкость мелодии
struct alarmData {
  boolean wait; //флаг ожидания звука будильника
  uint8_t num; //текущее количество будильников
  uint8_t dot; //флаг активных точек будильника
  uint8_t now; //флаг активоного будильника
} alarms;

//переменные таймера/секундомера
struct timerData {
  uint8_t mode; //режим таймера/секундомера
  uint16_t count; //счетчик таймера/секундомера
  uint16_t time = TIMER_TIME; //время таймера сек
} timer;

//переменные работы со звуками
struct soundData {
  uint8_t replay; //флаг повтора мелодии
  uint16_t semp; //текущий семпл мелодии
  uint16_t link; //ссылка на мелодию
  uint16_t size; //количество семплов мелодии
} sound;
volatile uint16_t buzz_cnt_puls; //счетчик циклов длительности
volatile uint16_t buzz_cnt_time; //счетчик циклов полуволны
uint16_t buzz_time; //циклы полуволны для работы пищалки

#define ALARM_PLAYER_VOL_MIN (uint8_t)(PLAYER_MAX_VOL * (ALARM_AUTO_VOL_MIN / 100.0))
#define ALARM_PLAYER_VOL_MAX (uint8_t)(PLAYER_MAX_VOL * (ALARM_AUTO_VOL_MAX / 100.0))
#define ALARM_PLAYER_VOL_TIME (uint16_t)(((uint16_t)ALARM_AUTO_VOL_TIME * 1000) / (ALARM_PLAYER_VOL_MAX - ALARM_PLAYER_VOL_MIN))
#define ALARM_RADIO_VOL_MIN (uint8_t)(RADIO_MAX_VOL * (ALARM_AUTO_VOL_MIN / 100.0))
#define ALARM_RADIO_VOL_MAX (uint8_t)(RADIO_MAX_VOL * (ALARM_AUTO_VOL_MAX / 100.0))
#define ALARM_RADIO_VOL_TIME (uint16_t)(((uint16_t)ALARM_AUTO_VOL_TIME * 1000) / (ALARM_RADIO_VOL_MAX - ALARM_RADIO_VOL_MIN))

#define BTN_GIST_TICK (BTN_GIST_TIME / (US_PERIOD / 1000.0)) //количество циклов для защиты от дребезга
#define BTN_HOLD_TICK (BTN_HOLD_TIME / (US_PERIOD / 1000.0)) //количество циклов после которого считается что кнопка зажата

#if BTN_TYPE
#define GET_ADC(low, high) (int16_t)(255.0 / (float)R_COEF(low, high)) //рассчет значения ацп кнопок

#define SET_MIN_ADC (uint8_t)(CONSTRAIN(GET_ADC(BTN_R_LOW, BTN_SET_R_HIGH) - BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE))
#define SET_MAX_ADC (uint8_t)(CONSTRAIN(GET_ADC(BTN_R_LOW, BTN_SET_R_HIGH) + BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE))

#define LEFT_MIN_ADC (uint8_t)(CONSTRAIN(GET_ADC(BTN_R_LOW, BTN_LEFT_R_HIGH) - BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE))
#define LEFT_MAX_ADC (uint8_t)(CONSTRAIN(GET_ADC(BTN_R_LOW, BTN_LEFT_R_HIGH) + BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE))

#define RIGHT_MIN_ADC (uint8_t)(CONSTRAIN(GET_ADC(BTN_R_LOW, BTN_RIGHT_R_HIGH) - BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE))
#define RIGHT_MAX_ADC (uint8_t)(CONSTRAIN(GET_ADC(BTN_R_LOW, BTN_RIGHT_R_HIGH) + BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE))

#define SET_CHK buttonCheckADC(SET_MIN_ADC, SET_MAX_ADC) //чтение средней аналоговой кнопки
#define LEFT_CHK buttonCheckADC(LEFT_MIN_ADC, LEFT_MAX_ADC) //чтение левой аналоговой кнопки
#define RIGHT_CHK buttonCheckADC(RIGHT_MIN_ADC, RIGHT_MAX_ADC) //чтение правой аналоговой кнопки

#if (BTN_ADD_TYPE == 2)
#define ADD_MIN_ADC (uint8_t)(CONSTRAIN(GET_ADC(BTN_R_LOW, BTN_ADD_R_HIGH) - BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE))
#define ADD_MAX_ADC (uint8_t)(CONSTRAIN(GET_ADC(BTN_R_LOW, BTN_ADD_R_HIGH) + BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE))

#define ADD_CHK buttonCheckADC(ADD_MIN_ADC, ADD_MAX_ADC) //чтение правой аналоговой кнопки
#endif
#endif

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
  SET_SLEEP_TIME, //время до ухода в сон
  SET_MAX_ITEMS //максимум пунктов меню
};

//перечисления меню отладки
enum {
  DEB_AGING_CORRECT, //корректировка регистра стариния часов
  DEB_TIME_CORRECT, //корректировка хода внутреннего таймера
  DEB_DEFAULT_MIN_PWM, //минимальное значение шим
  DEB_DEFAULT_MAX_PWM, //максимальное значение шим
  DEB_HV_ADC, //значение ацп преобразователя
  DEB_IR_BUTTONS, //програмирование кнопок
  DEB_LIGHT_SENS, //калибровка датчика освещения
  DEB_RESET, //сброс настроек отладки
  DEB_MAX_ITEMS //максимум пунктов меню
};

//перечисления режимов антиотравления
enum {
  BURN_ALL, //перебор всех индикаторов
  BURN_SINGLE, //перебор одного индикатора
  BURN_SINGLE_TIME, //перебор одного индикатора с отображением времени
  BURN_EFFECT_NUM //максимум эффектов антиотравления
};
enum {
  BURN_NORMAL, //нормальный режим
  BURN_DEMO //демонстрация
};

//перечисления анимаций перебора цифр секунд
enum {
  SECS_STATIC, //без анимации
  SECS_BRIGHT, //плавное угасание и появление
  SECS_ORDER_OF_NUMBERS, //перемотка по порядку числа
  SECS_ORDER_OF_CATHODES, //перемотка по порядку катодов в лампе
  SECS_EFFECT_NUM //максимум эффектов перелистывания
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
  FLIP_SLOT_MACHINE, //игровой автомат
  FLIP_EFFECT_NUM //максимум эффектов перелистывания
};
enum {
  FLIP_NORMAL, //нормальный режим
  FLIP_TIME, //режим анимации времени
  FLIP_DEMO //демонстрация
};

//перечисления режимов подсветки
enum {
  BACKL_OFF, //выключена
  BACKL_STATIC, //статичная
  BACKL_PULS, //дыхание
  BACKL_PULS_COLOR, //дыхание со сменой цвета при затухании
  BACKL_RUNNING_FIRE, //бегущий огонь
  BACKL_RUNNING_FIRE_COLOR, //бегущий огонь со сменой цвета
  BACKL_RUNNING_FIRE_RAINBOW, //бегущий огонь с радугой
  BACKL_RUNNING_FIRE_CONFETTI, //бегущий огонь с конфетти
  BACKL_WAVE, //волна
  BACKL_WAVE_COLOR, //волна со сменой цвета
  BACKL_WAVE_RAINBOW, //волна с радугой
  BACKL_WAVE_CONFETTI, //волна с конфетти
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
#if NEON_DOT == 2
  DOT_TURN_BLINK_NEON, //мигание по очереди неоновых ламп
#endif
#if DOTS_PORT_ENABLE
  DOT_RUNNING, //бегущая
  DOT_SNAKE, //змейка
  DOT_RUBBER_BAND, //резинка
#if (DOTS_NUM > 4) || (DOTS_TYPE == 2)
  DOT_TURN_BLINK, //мигание по очереди
#endif
#endif
  DOT_BLINK, //одиночное мигание
  DOT_DOOBLE_BLINK, //двойное мигание
  DOT_EFFECT_NUM //максимум эффектов подсветки
};

//перечисления настроек будильника
enum {
  ALARM_HOURS, //час будильника
  ALARM_MINS, //минута будильника
  ALARM_MODE, //режим будильника
  ALARM_DAYS, //день недели будильника
  ALARM_SOUND, //мелодия будильника
  ALARM_VOLUME, //громкость будильника
  ALARM_RADIO, //радиобудильник
  ALARM_MAX_ARR //максимальное количество данных
};

//перечисления датчиков температуры
enum {
  SENS_DS3231, //датчик DS3231
#if SENS_SHT_ENABLE
  SENS_SHT,
#endif
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

//перечисления режимов анимации времени
enum {
  ANIM_NULL, //нет анимации
  ANIM_SECS, //запуск анимации секунд
  ANIM_MINS, //запуск анимации минут
  ANIM_MAIN, //запуск основной анимации
  ANIM_OTHER //запуск иной анимации
};

//перечисления режимов времени
enum {
  TIME_NIGHT, //ночной режим
  TIME_DAY //дневной режим
};

//перечисления режимов сна
enum {
  SLEEP_DISABLE, //режим сна выключен
  SLEEP_NIGHT, //ночной режим сна
  SLEEP_DAY //дневной режим сна
};

//перечисления режимов смены яркости
enum {
  CHANGE_DISABLE, //смена яркости запрещена
  CHANGE_STATIC_BACKL, //разрешено управления яркостью статичной подсветки
  CHANGE_DYNAMIC_BACKL, //разрешено управления яркостью динамичной подсветки
#if BURN_BRIGHT
  CHANGE_INDI_BLOCK, //запрещена смена яркости индикаторов
#endif
  CHANGE_ENABLE //смена яркости разрешена
};

//перечисления основных программ
enum {
  INIT_PROGRAM,      //инициализация
  MAIN_PROGRAM,      //главный экран
  TEMP_PROGRAM,      //температура
  DATE_PROGRAM,      //текущая дата
  WARN_PROGRAM,      //предупреждение таймера
  ALARM_PROGRAM,     //тревога будильника
  RADIO_PROGRAM,     //радиоприемник
  TIMER_PROGRAM,     //таймер-секундомер
  SLEEP_PROGRAM,     //режим сна индикаторов
  FAST_SET_PROGRAM,  //быстрые настройки
  MAIN_SET_PROGRAM,  //основные настройки
  CLOCK_SET_PROGRAM, //настройки времени
  ALARM_SET_PROGRAM  //настройки будильника
};
uint8_t mainTask = INIT_PROGRAM; //переключатель подпрограмм

#define CONVERT_NUM(x) ((x[0] - 48) * 100 + (x[2] - 48) * 10 + (x[4] - 48)) //преобразовать строку в число
#define CONVERT_CHAR(x) (x - 48) //преобразовать символ в число

#define EEPROM_BLOCK_TIME EEPROM_BLOCK_NULL //блок памяти времени
#define EEPROM_BLOCK_SETTINGS_FAST (EEPROM_BLOCK_TIME + sizeof(RTC)) //блок памяти настроек свечения
#define EEPROM_BLOCK_SETTINGS_MAIN (EEPROM_BLOCK_SETTINGS_FAST + sizeof(fastSettings)) //блок памяти основных настроек
#define EEPROM_BLOCK_SETTINGS_RADIO (EEPROM_BLOCK_SETTINGS_MAIN + sizeof(mainSettings)) //блок памяти настроек радио
#define EEPROM_BLOCK_SETTINGS_DEBUG (EEPROM_BLOCK_SETTINGS_RADIO + sizeof(radioSettings)) //блок памяти настроек отладки
#define EEPROM_BLOCK_ERROR (EEPROM_BLOCK_SETTINGS_DEBUG + sizeof(debugSettings)) //блок памяти ошибок
#define EEPROM_BLOCK_EXT_ERROR (EEPROM_BLOCK_ERROR + 1) //блок памяти расширеных ошибок
#define EEPROM_BLOCK_ALARM (EEPROM_BLOCK_EXT_ERROR + 1) //блок памяти количества будильников

#define EEPROM_BLOCK_CRC_DEFAULT (EEPROM_BLOCK_ALARM + 1) //блок памяти контрольной суммы настроек
#define EEPROM_BLOCK_CRC_TIME (EEPROM_BLOCK_CRC_DEFAULT + 1) //блок памяти контрольной суммы времени
#define EEPROM_BLOCK_CRC_FAST (EEPROM_BLOCK_CRC_TIME + 1) //блок памяти контрольной суммы быстрых настроек
#define EEPROM_BLOCK_CRC_MAIN (EEPROM_BLOCK_CRC_FAST + 1) //блок памяти контрольной суммы основных настроек
#define EEPROM_BLOCK_CRC_RADIO (EEPROM_BLOCK_CRC_MAIN + 1) //блок памяти контрольной суммы настроек радио
#define EEPROM_BLOCK_CRC_DEBUG (EEPROM_BLOCK_CRC_RADIO + 1) //блок памяти контрольной суммы настроек отладки
#define EEPROM_BLOCK_CRC_DEBUG_DEFAULT (EEPROM_BLOCK_CRC_DEBUG + 1) //блок памяти контрольной суммы настроек отладки
#define EEPROM_BLOCK_CRC_ERROR (EEPROM_BLOCK_CRC_DEBUG_DEFAULT + 1) //блок контрольной суммы памяти ошибок
#define EEPROM_BLOCK_CRC_EXT_ERROR (EEPROM_BLOCK_CRC_ERROR + 1) //блок контрольной суммы памяти расширеных ошибок
#define EEPROM_BLOCK_CRC_ALARM (EEPROM_BLOCK_CRC_EXT_ERROR + 1) //блок контрольной суммы количества будильников
#define EEPROM_BLOCK_ALARM_DATA (EEPROM_BLOCK_CRC_ALARM + 1) //первая ячейка памяти будильников

#define MAX_ALARMS ((EEPROM_BLOCK_MAX - EEPROM_BLOCK_ALARM_DATA) / ALARM_MAX_ARR) //максимальное количество будильников

//--------------------------------------Главный цикл программ---------------------------------------------------
int main(void) //главный цикл программ
{
  startEnableWDT(); //первичный запуск WDT
  INIT_SYSTEM(); //инициализация

  for (;;) {
    dotReset(); //сброс анимации точек
#if BACKL_TYPE
    backlAnimEnable(); //разрешили эффекты подсветки
#endif
    changeBrightEnable(); //разрешить смену яркости
    changeBright(); //установка яркости от времени суток
    secUpd = 0; //обновление экрана
    switch (mainTask) {
      default: RESET_SYSTEM; break; //перезагрузка
      case MAIN_PROGRAM: mainTask = mainScreen(); break; //главный экран
      case TEMP_PROGRAM: mainTask = showTemp(); break; //показать температуру
      case DATE_PROGRAM: mainTask = showDate(); break; //показать дату
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
      case WARN_PROGRAM: mainTask = timerWarn(); break; //предупреждение таймера
#endif
#if ALARM_TYPE
      case ALARM_PROGRAM: //тревога будильника
        mainTask = alarmWarn(); //переход в программу
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
        radioPowerOff(); //выключить питание радиоприемника
#endif
#if PLAYER_TYPE
        playerSetVolNow(mainSettings.volumeSound); //установили громкость
#endif
        break;
#endif
#if BTN_ADD_TYPE || IR_PORT_ENABLE
#if RADIO_ENABLE
      case RADIO_PROGRAM:  //радиоприемник
        mainTask = radioMenu(); //переход в программу
        updateData((uint8_t*)&radioSettings, sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO); //записываем настройки радио в память
        break;
#endif
#if TIMER_ENABLE
      case TIMER_PROGRAM: mainTask = timerStopwatch(); break; //таймер-секундомер
#endif
#endif
      case SLEEP_PROGRAM: //режим сна индикаторов
        mainTask = sleepIndi(); //переход в программу
        setAnimTimers(); //установка таймеров анимаций
        break;
      case FAST_SET_PROGRAM: mainTask = fastSetSwitch(); break; //переключение настроек
      case MAIN_SET_PROGRAM: //основные настроки
        mainTask = settings_main(); //переход в программу
        updateData((uint8_t*)&mainSettings, sizeof(mainSettings), EEPROM_BLOCK_SETTINGS_MAIN, EEPROM_BLOCK_CRC_MAIN); //записываем основные настройки в память
        break;
      case CLOCK_SET_PROGRAM: //настройки времени
        mainTask = settings_time(); //переход в программу
        sendTime(); //отправить время в RTC
        updateData((uint8_t*)&RTC, sizeof(RTC), EEPROM_BLOCK_TIME, EEPROM_BLOCK_CRC_TIME); //записываем дату и время в память
        break;
#if ALARM_TYPE
      case ALARM_SET_PROGRAM: //настройка будильника
#if ALARM_TYPE == 1
        mainTask = settings_singleAlarm(); //переход в программу
#elif ALARM_TYPE == 2
        mainTask = settings_multiAlarm(); //переход в программу
#endif
#if PLAYER_TYPE
        playerSetVolNow(mainSettings.volumeSound); //установили громкость
#endif
        checkAlarms(); //проверка будильников
        break;
#endif
    }
  }
  return INIT_PROGRAM;
}
//--------------------------------------Инициализация---------------------------------------------------
void INIT_SYSTEM(void) //инициализация
{
#if GEN_ENABLE
  CONV_INIT; //инициализация преобразователя
#endif

#if (PLAYER_TYPE != 1) || PLAYER_UART_MODE
  uartDisable(); //отключение uart
#endif

#if AMP_PORT_ENABLE
  AMP_INIT; //инициализация питания усилителя
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

#if SQW_PORT_ENABLE
  SQW_INIT; //инициализация счета секунд
#endif

#if BACKL_TYPE
  BACKL_INIT; //инициализация подсветки
#endif

#if MOV_PORT_ENABLE
  MOV_INIT; //инициализация датчика движения
#endif

#if IR_PORT_ENABLE
  irInit();
#endif

  if (checkByte(EEPROM_BLOCK_ERROR, EEPROM_BLOCK_CRC_ERROR)) updateByte(0x00, EEPROM_BLOCK_ERROR, EEPROM_BLOCK_CRC_ERROR); //если контрольная сумма ошибок не совпала
  if (checkByte(EEPROM_BLOCK_EXT_ERROR, EEPROM_BLOCK_CRC_EXT_ERROR)) updateByte(0x00, EEPROM_BLOCK_EXT_ERROR, EEPROM_BLOCK_CRC_EXT_ERROR); //если контрольная сумма расширеных ошибок не совпала

  checkVCC(); //чтение напряжения питания

  if (checkSettingsCRC() || !SET_CHK) { //если контрольная сумма не совпала или зажата средняя кнопка то восстанавливаем настройеи по умолчанию
    updateData((uint8_t*)&RTC, sizeof(RTC), EEPROM_BLOCK_TIME, EEPROM_BLOCK_CRC_TIME); //записываем дату и время в память
    updateData((uint8_t*)&fastSettings, sizeof(fastSettings), EEPROM_BLOCK_SETTINGS_FAST, EEPROM_BLOCK_CRC_FAST); //записываем настройки яркости в память
    updateData((uint8_t*)&mainSettings, sizeof(mainSettings), EEPROM_BLOCK_SETTINGS_MAIN, EEPROM_BLOCK_CRC_MAIN); //записываем основные настройки в память
    updateData((uint8_t*)&radioSettings, sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO); //записываем настройки радио в память
#if ALARM_TYPE
    updateByte(alarms.num, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
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
    else EEPROM_ReadBlock((uint16_t)&RTC, EEPROM_BLOCK_TIME, sizeof(RTC)); //считываем дату и время из памяти
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
      updateByte(alarms.num, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
      SET_ERROR(MEMORY_ERROR); //устанавливаем ошибку памяти
    }
    else alarms.num = EEPROM_ReadByte(EEPROM_BLOCK_ALARM); //считываем количество будильников из памяти
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

#if GEN_ENABLE
  indiChangeCoef(); //обновление коэффициента линейного регулирования
#endif

#if PLAYER_TYPE == 1
  dfPlayerInit(); //инициализация DF плеера
#elif PLAYER_TYPE == 2
  sdPlayerInit(); //инициализация SD плеера
#endif

  wireInit(); //инициализация шины wire
  indiInit(); //инициализация индикаторов

  backlAnimDisable(); //запретили эффекты подсветки
  changeBrightDisable(CHANGE_DISABLE); //запретить смену яркости

  checkRTC(); //проверка модуля часов
#if SENS_BME_ENABLE || SENS_SHT_ENABLE || SENS_PORT_ENABLE
  checkTempSens(); //проверка установленного датчика температуры
#endif

  if (!LEFT_CHK) { //если левая кнопка зажата
#if DEBUG_PASS_ENABLE
    if (check_pass()) //если пароль верный
#endif
      debug_menu(); //запускаем отладку
  }
  else if (!RIGHT_CHK) test_system(); //если правая кнопка зажата запускаем тест системы
#if FLIP_ANIM_START == 1
  else animShow = ANIM_MAIN; //установили флаг анимации
#elif FLIP_ANIM_START > 1
  else animShow = (ANIM_OTHER + FLIP_ANIM_START); //установили флаг анимации
#endif

  checkErrors(); //проверка на наличие ошибок

#if PLAYER_TYPE
  playerSetVolNow(mainSettings.volumeSound); //установили громкость
#endif

#if ALARM_TYPE == 1
  alarmInit(); //инициализация будильника
#endif

  randomSeed(RTC.s * (RTC.m + RTC.h) + RTC.DD * RTC.MM); //радомный сид для глюков
  setAnimTimers(); //установка таймеров анимаций
  _timer_sec[TMR_SYNC] = ((uint16_t)RTC_SYNC_TIME * 60); //устанавливаем таймер синхронизации

#if ALARM_TYPE
  checkAlarms(); //проверка будильников
#endif

  mainTask = MAIN_PROGRAM; //установили основную программу
}
//-----------------------------Прерывание от RTC--------------------------------
#if SQW_PORT_ENABLE
ISR(INT0_vect) //внешнее прерывание на пине INT0 - считаем секунды с RTC
{
  tick_sec++; //прибавляем секунду
}
#endif
//-----------------------Прерывание сигнала для пищалки-------------------------
#if !PLAYER_TYPE
ISR(TIMER2_COMPB_vect) //прерывание сигнала для пищалки
{
  if (!buzz_cnt_time) { //если циклы полуволны кончились
    BUZZ_INV; //инвертируем бузер
    buzz_cnt_time = buzz_time; //устанавливаем циклы полуволны
    if (!--buzz_cnt_puls) { //считаем циклы времени работы бузера
      BUZZ_OFF; //если циклы кончились, выключаем бузер
      TIMSK2 &= ~(0x01 << OCIE2B); //выключаем таймер
    }
  }
  if (buzz_cnt_time > 255) buzz_cnt_time -= 255; //считаем циклы полуволны
  else if (buzz_cnt_time) { //если остался хвост
    OCR2B += buzz_cnt_time; //устанавливаем хвост
    buzz_cnt_time = 0; //сбрасываем счетчик циклов полуволны
  }
}
#endif
//-----------------------------Установка ошибки---------------------------------
void SET_ERROR(uint8_t err) //установка ошибки
{
  uint8_t _error_bit = 0; //флаг ошибки
  if (err < 8) { //если номер ошибки не привышает первый блок
    _error_bit = (0x01 << err); //выбрали флаг ошибки
    EEPROM_UpdateByte(EEPROM_BLOCK_ERROR, EEPROM_ReadByte(EEPROM_BLOCK_ERROR) | _error_bit); //обновили ячейку ошибки
    EEPROM_UpdateByte(EEPROM_BLOCK_CRC_ERROR, EEPROM_ReadByte(EEPROM_BLOCK_CRC_ERROR) & (_error_bit ^ 0xFF)); //обновили ячейку контрольной суммы ошибки
  }
  else { //иначе расширеная ошибка
    _error_bit = (0x01 << (err - 8)); //выбрали флаг ошибки
    EEPROM_UpdateByte(EEPROM_BLOCK_EXT_ERROR, EEPROM_ReadByte(EEPROM_BLOCK_EXT_ERROR) | _error_bit); //обновили ячейку расширеной ошибки
    EEPROM_UpdateByte(EEPROM_BLOCK_CRC_EXT_ERROR, EEPROM_ReadByte(EEPROM_BLOCK_CRC_EXT_ERROR) & (_error_bit ^ 0xFF)); //обновили ячейку контрольной суммы расширеной ошибки
  }
}
//--------------------------Установка таймеров анимаций-------------------------
void setAnimTimers(void) //установка таймеров анимаций
{
  _timer_sec[TMR_TEMP] = getPhaseTime(mainSettings.autoTempTime, AUTO_TEMP_PHASE); //установка таймера показа температуры
  _timer_sec[TMR_BURN] = getPhaseTime(BURN_PERIOD, BURN_PHASE); //установка таймера антиотравления
  _timer_sec[TMR_GLITCH] = random(GLITCH_MIN_TIME, GLITCH_MAX_TIME); //находим рандомное время появления глюка
}
//-------------------------Разрешить анимации подсветки-------------------------
void backlAnimEnable(void) //разрешить анимации подсветки
{
#if BACKL_TYPE == 3
  if (fastSettings.backlMode & 0x80) { //если эффекты подсветки были запрещены
    fastSettings.backlMode &= 0x7F; //разрешили эффекты подсветки
    backl.steps = 0; //сбросили шаги
    backl.drive = 0; //сбросили направление
    backl.position = 0; //сбросили позицию
    _timer_ms[TMR_COLOR] = 0; //сбросили таймер смены цвета
    _timer_ms[TMR_BACKL] = 0; //сбросили таймер анимации подсветки
    if (fastSettings.backlMode) setLedBright(backl.maxBright); //установили максимальную яркость
  }
#else
  fastSettings.backlMode &= 0x7F; //разрешили эффекты подсветки
#endif
}
//-------------------------Запретить анимации подсветки-------------------------
void backlAnimDisable(void) //запретить анимации подсветки
{
  fastSettings.backlMode |= 0x80; //запретили эффекты подсветки
}
//----------------------Разрешить анимацию секундных точек----------------------
void dotAnimEnable(void) //разрешить анимацию секундных точек
{
  fastSettings.dotMode &= 0x7F; //разрешили эффекты точек
}
//----------------------Запретить анимацию секундных точеки---------------------
void dotAnimDisable(void) //запретить анимацию секундных точек
{
  fastSettings.dotMode |= 0x80; //запретили эффекты точек
}
//----------------------------Разрешить смену яркости---------------------------
void changeBrightEnable(void) //разрешить смену яркости
{
  changeBrightState = CHANGE_ENABLE; //разрешили смену яркости
}
//----------------------------Запретить смену яркости---------------------------
void changeBrightDisable(uint8_t _state) //запретить смену яркости
{
  changeBrightState = _state; //запретили смену яркости
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
//---------------------------------Получить время со сдвигом фазы-----------------------------------------
uint16_t getPhaseTime(uint8_t time, int8_t phase) //получить время со сдвигом фазы
{
  return ((uint16_t)time * 60) + (phase - RTC.s) - ((RTC.s >= phase) ? 0 : 60);  //возвращаем результат
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
//------------------Проверка контрольной суммы настроек отладки---------------------
boolean checkDebugSettingsCRC(void) //проверка контрольной суммы настроек отладки
{
  uint8_t CRC = 0; //буфер контрольной суммы

  for (uint8_t i = 0; i < sizeof(debugSettings); i++) checkCRC(&CRC, *((uint8_t*)&debugSettings + i));

  if (EEPROM_ReadByte(EEPROM_BLOCK_CRC_DEBUG_DEFAULT) == CRC) return 0;
  else EEPROM_UpdateByte(EEPROM_BLOCK_CRC_DEBUG_DEFAULT, CRC);
  return 1;
}
//-----------------Проверка установленного датчика температуры----------------------
void checkTempSens(void) //проверка установленного датчика температуры
{
  for (sens.type = (SENS_ALL - 1); sens.type; sens.type--) { //перебираем все датчики температуры
    updateTemp(); //обновить показания температуры
    if (!sens.err) { //если найден датчик температуры
      _timer_ms[TMR_SENS] = TEMP_UPDATE_TIME; //установили таймаут
      return; //выходим
    }
  }
  SET_ERROR(TEMP_SENS_ERROR); //иначе выдаем ошибку
}
//-------------------------Обновить показания температуры---------------------------
void updateTemp(void) //обновить показания температуры
{
  sens.err = 1; //подняли флаг проверки датчика температуры на ошибку связи
  switch (sens.type) { //выбор датчика температуры
    default: if (readTempRTC()) sens.err = 0; return; //чтение температуры с датчика DS3231
#if SENS_SHT_ENABLE
    case SENS_SHT: readTempSHT(); break; //чтение температуры/влажности с датчика SHT
#endif
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
//-----------------Обновление предела удержания напряжения-------------------------
void updateTresholdADC(void) //обновление предела удержания напряжения
{
  hv_treshold = HV_ADC(GET_VCC(REFERENCE, vcc_adc)) + CONSTRAIN(debugSettings.hvCorrect, -25, 25);
}
//------------------------Обработка аналоговых входов------------------------------
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
#if CONV_PIN == 9
            if (adc_temp < hv_treshold) TCCR1A |= (0x01 << COM1A1); //включаем шим преобразователя
            else {
              TCCR1A &= ~(0x01 << COM1A1); //выключаем шим преобразователя
              CONV_OFF; //выключаем пин преобразователя
            }
#elif CONV_PIN == 10
            if (adc_temp < hv_treshold) TCCR1A |= (0x01 << COM1B1); //включаем шим преобразователя
            else {
              TCCR1A &= ~(0x01 << COM1B1); //выключаем шим преобразователя
              CONV_OFF; //выключаем пин преобразователя
            }
#endif

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
  _delay_ms(START_DELAY); //ждём пока напряжение успокоится
  for (uint8_t i = 0; i < CYCLE_VCC_CHECK; i++) {
    _delay_ms(5); //ждём пока опорное успокоится
    ADCSRA |= (1 << ADSC); //запускаем преобразование
    while (ADCSRA & (1 << ADSC)); //ждем окончания преобразования
    temp += ADCL | ((uint16_t)ADCH << 8); //записали результат
  }
  vcc_adc = temp / CYCLE_VCC_CHECK; //получаем напряжение питания

  if (GET_VCC(REFERENCE, vcc_adc) < MIN_VCC || GET_VCC(REFERENCE, vcc_adc) > MAX_VCC) SET_ERROR(VCC_ERROR); //устанвливаем ошибку по питанию

#if BTN_TYPE
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
//-------------------Обработка сенсора яркости освещения---------------------------
void lightSensUpdate(void) //обработка сенсора яркости освещения
{
  static boolean now_state_light;
  if (mainSettings.timeBright[0] == mainSettings.timeBright[1]) { //если разрешена робота сенсора
    _timer_ms[TMR_LIGHT] = (1000 - LIGHT_SENS_TIME); //установили таймер
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
//-------------------Проверка сенсора яркости освещения----------------------------
void lightSensCheck(void) //проверка сенсора яркости освещения
{
  if (!_timer_ms[TMR_LIGHT]) { //если пришло время
    _timer_ms[TMR_LIGHT] = 1000; //установили таймер
    analogState |= 0x01; //установили флаг обновления АЦП сенсора яркости
  }
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
  if (irState == IR_READY) { //если пришла команда и управление ИК не заблокировано
    irState = 0; //сбрасываем флаг готовности
    for (uint8_t button = 0; button < (KEY_MAX_ITEMS - 1); button++) { //ищем номер кнопки
      if (irCommand == debugSettings.irButtons[button]) { //если команда совпала
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
  if (!disable32K()) return; //отключение вывода 32K

#if SQW_PORT_ENABLE
  if (!setSQW()) return; //установка SQW на 1Гц

  EIMSK = 0; //запретили прерывание INT0
  EICRA = (0x01 << ISC01); //настраиваем внешнее прерывание по спаду импульса на INT0
  EIFR |= (0x01 << INTF0); //сбрасываем флаг прерывания INT0

  for (_timer_ms[TMR_MS] = SQW_TEST_TIME; !(EIFR & (0x01 << INTF0)) && _timer_ms[TMR_MS];) { //ждем сигнала от SQW
    for (uint8_t _tick = tick_ms; _tick > 0; _tick--) { //если был тик то обрабатываем данные
      tick_ms--; //убавили счетчик миллисекунд
      if (_timer_ms[TMR_MS] > MS_PERIOD) _timer_ms[TMR_MS] -= MS_PERIOD; //если таймер больше периода
      else if (_timer_ms[TMR_MS]) _timer_ms[TMR_MS] = 0; //иначе сбрасываем таймер
    }
  }
#endif

  if (!getTime(RTC_CLEAR_OSF)) { //считываем время из RTC
    writeAgingRTC(debugSettings.aging); //восстанавливаем коррекцию хода
    sendTime(); //отправляем последнее сохраненное время в RTC
  }

#if SQW_PORT_ENABLE
  if (EIFR & (0x01 << INTF0)) { //если был сигнал с SQW
    EIFR |= (0x01 << INTF0); //сбрасываем флаг прерывания INT0
    EIMSK = (0x01 << INT0); //разрешаем внешнее прерывание INT0
  }
  else SET_ERROR(SQW_LONG_ERROR); //иначе выдаем ошибку
#endif
}
//-----------------------------Проверка ошибок-------------------------------------
void checkErrors(void) //проверка ошибок
{
  uint16_t _error_reg = EEPROM_ReadByte(EEPROM_BLOCK_ERROR) | ((uint16_t)EEPROM_ReadByte(EEPROM_BLOCK_EXT_ERROR) << 8); //прочитали регистр ошибок
  if (_error_reg) { //если есть ошибка
#if FLIP_ANIM_START == 1
    animShow = ANIM_MAIN; //установили флаг анимации
#elif FLIP_ANIM_START > 1
    animShow = (ANIM_OTHER + FLIP_ANIM_START); //установили флаг анимации
#endif
    for (uint8_t i = 0; i < 13; i++) { //проверяем весь регистр
      if (_error_reg & (0x01 << i)) { //если стоит флаг ошибки
        indiPrintNum(i + 1, 0, 4, 0); //вывод ошибки
#if PLAYER_TYPE
        playerSetTrack(PLAYER_ERROR_SOUND, PLAYER_GENERAL_FOLDER); //воспроизводим трек ошибки
        playerSpeakNumber(i + 1); //воспроизводим номер ошибки
#else
        uint8_t _sound_bit = ((i + 1) & 0x0F) | 0xF0; //указатель на бит ошибки
        _timer_ms[TMR_PLAYER] = 0; //сбросили таймер
#endif
        for (_timer_ms[TMR_MS] = ERROR_SHOW_TIME; _timer_ms[TMR_MS];) {
          dataUpdate(); //обработка данных
          if (buttonState()) { //если нажата кнопка
#if FLIP_ANIM_START
            animShow = ANIM_NULL; //сбросили флаг анимации
#endif
            break;
          }
#if !PLAYER_TYPE
          if (!_timer_ms[TMR_PLAYER] && (_sound_bit != 0x0F)) { //если звук не играет
            melodyPlay((boolean)(_sound_bit & 0x01), SOUND_LINK(error_sound), REPLAY_ONCE); //воспроизводим звук
            _sound_bit >>= 1; //сместили указатель
          }
#endif
        }
      }
    }
    updateByte(0x00, EEPROM_BLOCK_ERROR, EEPROM_BLOCK_CRC_ERROR); //сбросили ошибки
    updateByte(0x00, EEPROM_BLOCK_EXT_ERROR, EEPROM_BLOCK_CRC_EXT_ERROR); //сбросили ошибки
  }
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

#if (BACKL_TYPE != 3) && BACKL_TYPE
  backlSetBright(TEST_BACKL_BRIGHT); //устанавливаем максимальную яркость
#endif
  indiSetBright(TEST_INDI_BRIGHT); //установка яркости индикаторов
#if (NEON_DOT != 3) || !DOTS_PORT_ENABLE
  dotSetBright(TEST_DOT_BRIGHT); //установка яркости точек
#endif
  while (1) {
    for (uint8_t indi = 0; indi < LAMP_NUM; indi++) {
      indiClr(); //очистка индикаторов
#if DOTS_PORT_ENABLE
      indiClrDots(); //выключаем разделительные точки
      indiSetDotL(indi); //включаем разделительную точку
      indiSetDotR(indi); //включаем разделительную точку
#endif
#if BACKL_TYPE == 3
      setLedBright(indi, TEST_BACKL_BRIGHT); //включаем светодиод
#endif
      for (uint8_t digit = 0; digit < 10; digit++) {
        indiPrintNum(digit, indi); //отрисовываем цифру
#if BACKL_TYPE == 3
        setLedHue(indi, digit * 25, WHITE_OFF); //устанавливаем статичный цвет
#endif
#if !PLAYER_TYPE
        buzz_pulse(TEST_FREQ_STEP + (digit * TEST_FREQ_STEP), TEST_LAMP_TIME); //перебор частот
#endif
        for (_timer_ms[TMR_MS] = TEST_LAMP_TIME; _timer_ms[TMR_MS];) { //ждем
          dataUpdate(); //обработка данных
          if (buttonState()) return; //выходим если нажата кнопка
        }
      }
#if BACKL_TYPE == 3
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
      if (++time_out >= DEBUG_TIMEOUT) { //если время вышло
#if FLIP_ANIM_START == 1
        animShow = ANIM_MAIN; //установили флаг анимации
#elif FLIP_ANIM_START > 1
        animShow = (ANIM_OTHER + FLIP_ANIM_START); //установили флаг анимации
#endif
        return 0; //выходим
      }
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
        playerSetTrack(PLAYER_PASS_SOUND, PLAYER_GENERAL_FOLDER); //сигнал ошибки ввода пароля
#else
        melodyPlay(SOUND_PASS_ERROR, SOUND_LINK(general_sound), REPLAY_ONCE); //сигнал ошибки ввода пароля
#endif
        if (++attempts_pass >= DEBUG_PASS_ATTEMPTS) return 0; //выходим если превышено количество попыток ввода пароля
        break;
    }
  }
  return 0;
}
//-----------------------------Отладка------------------------------------
void debug_menu(void) //отладка
{
  boolean cur_set = 0; //режим настройки
  boolean cur_reset = 0; //сброс настройки
  boolean cur_update = 0; //обновление индикаторов
  uint8_t cur_mode = 0; //текущий режим
#if IR_PORT_ENABLE
  uint8_t cur_button = 0; //текущая кнопка пульта
#endif
#if LIGHT_SENS_ENABLE
  uint8_t temp_min = 255;
  uint8_t temp_max = 0;
#endif

#if PLAYER_TYPE
  playerSetTrackNow(PLAYER_DEBUG_MENU_START, PLAYER_MENU_FOLDER); //воспроизводим название пункта отладки
#endif

  dotSetBright(0); //выключаем точки
  indiSetBright(30); //устанавливаем максимальную яркость индикаторов

  //настройки
  while (1) {
    dataUpdate(); //обработка данных


#if LIGHT_SENS_ENABLE || IR_PORT_ENABLE
    if (cur_set) {
      switch (cur_mode) {
#if IR_PORT_ENABLE
        case DEB_IR_BUTTONS: //програмирование кнопок
          if (irState == (IR_READY | IR_DISABLE)) { //если управление ИК заблокировано и пришла новая команда
            indiClr(3); //очистка индикатора
            indiPrintNum((uint8_t)irCommand, 0, 3, 0); //выводим код кнопки пульта
            debugSettings.irButtons[cur_button] = irCommand; //записываем команду в массив
            irState = IR_DISABLE; //сбросили флаг готовности
            _timer_ms[TMR_MS] = DEBUG_IR_BUTTONS_TIME; //установили таймер
          }
          else if (!_timer_ms[TMR_MS]) {
            cur_update = 0; //обновление экрана
            _timer_ms[TMR_MS] = DEBUG_IR_BUTTONS_TIME; //установили таймер
          }
          break;
#endif
#if LIGHT_SENS_ENABLE
        case DEB_LIGHT_SENS: //калибровка датчика освещения
          if (!_timer_ms[TMR_MS]) {
            if (temp_min > adc_light) temp_min = adc_light;
            if (temp_max < adc_light) temp_max = adc_light;
            analogState |= 0x01; //установили флаг обновления АЦП сенсора яркости
            _timer_ms[TMR_MS] = DEBUG_LIGHT_SENS_TIME; //установили таймер
            cur_update = 0; //обновление экрана
          }
          break;
#endif
      }
    }
#endif

    if (!cur_update) {
      cur_update = 1; //сбрасываем флаг

      indiClr(); //очистка индикаторов
      switch (cur_set) {
        case 0:
          indiPrintNum(cur_mode + 1, (LAMP_NUM / 2 - 1), 2, 0); //вывод режима
          break;
        case 1:
          indiPrintNum(cur_mode + 1, 5); //режим
          switch (cur_mode) {
            case DEB_AGING_CORRECT: indiPrintNum(debugSettings.aging + 128, 0); break; //выводим коррекцию DS3231
            case DEB_TIME_CORRECT: indiPrintNum(debugSettings.timePeriod, 0); break; //выводим коррекцию внутреннего таймера
#if GEN_ENABLE
            case DEB_DEFAULT_MIN_PWM: indiPrintNum(debugSettings.min_pwm, 0); break; //выводим минимальный шим
            case DEB_DEFAULT_MAX_PWM: indiPrintNum(debugSettings.max_pwm, 0); break; //выводим максимальный шим
#if GEN_FEEDBACK
            case DEB_HV_ADC: indiPrintNum(hv_treshold, 0); break; //выводим корекцию напряжения
#endif
#endif
#if IR_PORT_ENABLE
            case DEB_IR_BUTTONS: //програмирование кнопок
              indiPrintNum((debugSettings.irButtons[cur_button]) ? 1 : 0, 0); //выводим значение записи в ячейке кнопки пульта
              indiPrintNum(cur_button + 1, 2, 2, 0); //выводим номер кнопки пульта
              break;
#endif
#if LIGHT_SENS_ENABLE
            case DEB_LIGHT_SENS: //калибровка датчика освещения
              indiPrintNum(adc_light, 1, 3); //выводим значение АЦП датчика освещения
              break;
#endif
            case DEB_RESET: indiPrintNum(cur_reset, 0, 2, 0); break; //сброс настроек отладки
          }
          break;
      }
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (cur_set) {
          case 0:
            if (cur_mode > 0) cur_mode--;
            else cur_mode = DEB_MAX_ITEMS - 1;
#if PLAYER_TYPE
            playerSetTrackNow(PLAYER_DEBUG_MENU_START + cur_mode, PLAYER_MENU_FOLDER); //воспроизводим название пункта отладки
#endif
            break;
          case 1:
            switch (cur_mode) {
              case DEB_AGING_CORRECT: if (debugSettings.aging > -127) debugSettings.aging--; else debugSettings.aging = 127; break; //коррекция хода
              case DEB_TIME_CORRECT: if (debugSettings.timePeriod > US_PERIOD_MIN) debugSettings.timePeriod--; else debugSettings.timePeriod = US_PERIOD_MAX; break; //коррекция хода
#if GEN_ENABLE
              case DEB_DEFAULT_MIN_PWM: //коррекция минимального значения шим
                if (debugSettings.min_pwm > 100) debugSettings.min_pwm -= 5; //минимальное значение шим
                indiChangeCoef(); //обновление коэффициента линейного регулирования
                break;
              case DEB_DEFAULT_MAX_PWM: //коррекция максимального значения шим
                if (debugSettings.max_pwm > 150) debugSettings.max_pwm -= 5; //максимальное значение шим
                indiChangeCoef(); //обновление коэффициента линейного регулирования
                break;
#if GEN_FEEDBACK
              case DEB_HV_ADC: //коррекция значения ацп преобразователя
                if (debugSettings.hvCorrect > -30) debugSettings.hvCorrect--; //значение ацп преобразователя
                updateTresholdADC(); //обновление предела удержания напряжения
                break;
#endif
#endif
#if IR_PORT_ENABLE
              case DEB_IR_BUTTONS: //програмирование кнопок
                if (cur_button) cur_button--;
                break;
#endif
              case DEB_RESET: cur_reset = 0; break; //сброс настроек отладки
            }
            break;
        }
        cur_update = 0; //обновление экрана
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (cur_set) {
          case 0:
            if (cur_mode < (DEB_MAX_ITEMS - 1)) cur_mode++;
            else cur_mode = 0;
#if PLAYER_TYPE
            playerSetTrackNow(PLAYER_DEBUG_MENU_START + cur_mode, PLAYER_MENU_FOLDER); //воспроизводим название пункта отладки
#endif
            break;
          case 1:
            switch (cur_mode) {
              case DEB_AGING_CORRECT: if (debugSettings.aging < 127) debugSettings.aging++; else debugSettings.aging = -127; break; //коррекция хода
              case DEB_TIME_CORRECT: if (debugSettings.timePeriod < US_PERIOD_MAX) debugSettings.timePeriod++; else debugSettings.timePeriod = US_PERIOD_MIN; break; //коррекция хода
#if GEN_ENABLE
              case DEB_DEFAULT_MIN_PWM: //коррекция минимального значения шим
                if (debugSettings.min_pwm < 190) debugSettings.min_pwm += 5; //минимальное значение шим
                indiChangeCoef(); //обновление коэффициента линейного регулирования
                break;
              case DEB_DEFAULT_MAX_PWM: //коррекция максимального значения шим
                if (debugSettings.max_pwm < 200) debugSettings.max_pwm += 5; //максимальное значение шим
                indiChangeCoef(); //обновление коэффициента линейного регулирования
                break;
#if GEN_FEEDBACK
              case DEB_HV_ADC: //коррекция значения ацп преобразователя
                if (debugSettings.hvCorrect < 30) debugSettings.hvCorrect++; //значение ацп преобразователя
                updateTresholdADC(); //обновление предела удержания напряжения
                break;
#endif
#endif
#if IR_PORT_ENABLE
              case DEB_IR_BUTTONS: //програмирование кнопок
                if (cur_button < (KEY_MAX_ITEMS - 2)) cur_button++;
                break;
#endif
              case DEB_RESET: cur_reset = 1; break; //сброс настроек отладки
            }
            break;
        }
        cur_update = 0; //обновление экрана
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        cur_set = !cur_set; //сменили сотояние подрежима меню

        if (cur_set) { //если в режиме настройки
          switch (cur_mode) {
            case DEB_AGING_CORRECT: if (!readAgingRTC(&debugSettings.aging)) cur_set = 0; break; //чтение коррекции хода
            case DEB_TIME_CORRECT: break; //коррекция хода
#if GEN_ENABLE
            case DEB_DEFAULT_MIN_PWM: indiSetBright(1); break; //минимальное значение шим
            case DEB_DEFAULT_MAX_PWM: break; //максимальное значение шим
#if GEN_FEEDBACK
            case DEB_HV_ADC: break; //коррекция значения ацп преобразователя
#endif
#endif
#if IR_PORT_ENABLE
            case DEB_IR_BUTTONS: //програмирование кнопок
              _timer_ms[TMR_MS] = 0; //сбросили таймер
              irState = IR_DISABLE; //установили флаг запрета
              cur_button = 0; //сбросили номер текущей кнопки
              break;
#endif
#if LIGHT_SENS_ENABLE
            case DEB_LIGHT_SENS: //калибровка датчика освещения
              _timer_ms[TMR_MS] = 0; //сбросили таймер
              temp_min = 255; //установили минимальное значение
              temp_max = 0; //установили максимальное значение
              break;
#endif
            case DEB_RESET: cur_reset = 0; break; //сброс настроек отладки
            default: cur_set = 0; break; //запретили войти в пункт меню
          }
        }
        else { //иначе режим выбора пункта меню
          switch (cur_mode) {
            case DEB_AGING_CORRECT: writeAgingRTC(debugSettings.aging); break; //запись коррекции хода
#if IR_PORT_ENABLE
            case DEB_IR_BUTTONS: //програмирование кнопок
              irState = 0; //сбросили состояние
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
            case DEB_RESET: //сброс настроек отладки
              if (cur_reset) { //подтверждение
                cur_mode = 0; //перешли на первый пункт меню
                debugSettings.aging = 0; //коррекции хода модуля часов
                debugSettings.timePeriod = US_PERIOD; //коррекция хода внутреннего осцилятора
#if GEN_ENABLE
                debugSettings.min_pwm = DEFAULT_MIN_PWM; //минимальное значение шим
                debugSettings.max_pwm = DEFAULT_MAX_PWM; //максимальное значение шим
                indiChangeCoef(); //обновление коэффициента линейного регулирования
#if GEN_FEEDBACK
                debugSettings.hvCorrect = 0; //коррекция напряжения преобразователя
                updateTresholdADC(); //обновление предела удержания напряжения
#endif
#endif
#if IR_PORT_ENABLE
                for (uint8_t i = 0; i < (KEY_MAX_ITEMS - 1); i++) debugSettings.irButtons[i] = 0; //сбрасываем значение ячеек кнопок пульта
#endif
                writeAgingRTC(debugSettings.aging); //запись коррекции хода
#if PLAYER_TYPE
                playerSetTrack(PLAYER_RESET_SOUND, PLAYER_GENERAL_FOLDER);
#else
                melodyPlay(SOUND_RESET_SETTINGS, SOUND_LINK(general_sound), REPLAY_ONCE); //сигнал сброса настроек отладки
#endif
              }
              break;
          }
          indiSetBright(30); //устанавливаем максимальную яркость индикаторов
        }
        dotSetBright((cur_set) ? 250 : 0); //включаем точки
        cur_update = 0; //обновление экрана
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        if (!cur_set) { //если не в режиме настройки
          updateData((uint8_t*)&debugSettings, sizeof(debugSettings), EEPROM_BLOCK_SETTINGS_DEBUG, EEPROM_BLOCK_CRC_DEBUG); //записываем настройки отладки в память
          return; //выходим
        }
        break;
    }
  }
}
//--------------------------------Генерация частот бузера----------------------------------------------
void buzz_pulse(uint16_t freq, uint16_t time) //генерация частоты бузера (частота 10..10000, длительность мс.)
{
  TIMSK2 &= ~(0x01 << OCIE2B); //выключаем таймер
  BUZZ_OFF; //выключаем бузер
  buzz_cnt_time = 0; //сбросили счетчик
  buzz_cnt_puls = ((uint32_t)freq * (uint32_t)time) / 500; //пересчитываем частоту и время в циклы таймера
  buzz_time = (1000000 / freq); //пересчитываем частоту в циклы полуволны
  OCR2B = 255; //устанавливаем COMB в начало
  TIFR2 |= (0x01 << OCF2B); //сбрасываем флаг прерывания
  TIMSK2 |= (0x01 << OCIE2B); //запускаем таймер
}
//--------------------------------Воспроизведение мелодии-----------------------------------------------
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
//----------------------------Запуск воспроизведения мелодии---------------------------------------------
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
  if (!alarms.num) newAlarm(); //создать новый будильник
  else if (alarms.num > 1) { //если будильников в памяти больше одного
    alarms.num = 1; //оставляем один будильник
    updateByte(alarms.num, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
  }
}
//-----------------------------------Отключение будильника------------------------------------------------
void alarmDisable(void) //отключение будильника
{
#if PLAYER_TYPE
  playerSetTrackNow(PLAYER_ALARM_DISABLE_SOUND, PLAYER_GENERAL_FOLDER); //звук выключения будильника
#else
  melodyPlay(SOUND_ALARM_DISABLE, SOUND_LINK(general_sound), REPLAY_ONCE); //звук выключения будильника
#endif
  alarmReset(); //сброс будильника
}
//--------------------------------------Сброс будильника--------------------------------------------------
void alarmReset(void) //сброс будильника
{
  if (alarmRead(alarms.now - 1, ALARM_MODE) == 1) EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + ((alarms.now - 1) * ALARM_MAX_ARR) + ALARM_MODE, 0); //если был установлен режим одиночный то выключаем будильник
  checkAlarms(); //проверка будильников
  _timer_sec[TMR_ALM] = 0; //сбрасываем таймер отключения будильника
  _timer_sec[TMR_ALM_WAINT] = 0; //сбрасываем таймер ожидания повторного включения тревоги
  _timer_sec[TMR_ALM_SOUND] = 0; //сбрасываем таймер отключения звука
  alarms.wait = 0; //сбрасываем флаг ожидания
  alarms.now = 0; //сбрасываем флаг тревоги
}
//-----------------------------Получить основные данные будильника-----------------------------------------
uint8_t alarmRead(uint8_t almNum, uint8_t almData) //получить основные данные будильника
{
  return EEPROM_ReadByte(EEPROM_BLOCK_ALARM_DATA + ((uint16_t)almNum * ALARM_MAX_ARR) + almData); //возвращаем запрошеный байт
}
//--------------------------Получить блок основных данных будильника---------------------------------------
void alarmReadBlock(uint8_t almNum, uint8_t* data) //получить блок основных данных будильника
{
  uint16_t curCell = (uint16_t)(almNum - 1) * ALARM_MAX_ARR;
  for (uint8_t i = 0; i < ALARM_MAX_ARR; i++) data[i] = (almNum) ? EEPROM_ReadByte(EEPROM_BLOCK_ALARM_DATA + curCell + i) : 0; //считываем блок данных
}
//---------------------------Записать блок основных данных будильника--------------------------------------
void alarmWriteBlock(uint8_t almNum, uint8_t* data) //записать блок основных данных будильника
{
  if (!almNum) return; //если нет ни одного будильника то выходим
  uint16_t curCell = (uint16_t)(almNum - 1) * ALARM_MAX_ARR;
  for (uint8_t i = 0; i < ALARM_MAX_ARR; i++) EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + curCell + i, data[i]); //записываем блок данных
}
//---------------------------------Создать новый будильник-------------------------------------------------
void newAlarm(void) //создать новый будильник
{
  if (alarms.num < MAX_ALARMS) { //если новый будильник меньше максимума
    uint16_t newCell = EEPROM_BLOCK_ALARM_DATA + ((uint16_t)alarms.num * ALARM_MAX_ARR);
    EEPROM_UpdateByte(newCell + ALARM_HOURS, DEFAULT_ALARM_TIME_HH); //устанавливаем час по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_MINS, DEFAULT_ALARM_TIME_MM); //устанавливаем минуты по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_MODE, DEFAULT_ALARM_MODE); //устанавливаем режим по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_DAYS, 0); //устанавливаем дни недели по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_SOUND, 0); //устанавливаем мелодию по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_VOLUME, 0); //устанавливаем громкость по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_RADIO, 0); //устанавливаем радиобудильник по умолчанию
    updateByte(++alarms.num, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
  }
}
//-----------------------------------Удалить будильник-----------------------------------------------------
void delAlarm(uint8_t alarm) //удалить будильник
{
  if (alarms.num) { //если будильник доступен
    for (uint8_t start = alarm; start < alarms.num; start++) { //перезаписываем массив будильников
      uint16_t oldCell = EEPROM_BLOCK_ALARM_DATA + ((uint16_t)start * ALARM_MAX_ARR);
      uint16_t newCell = EEPROM_BLOCK_ALARM_DATA + ((uint16_t)(start - 1) * ALARM_MAX_ARR);
      for (uint8_t block = 0; block < ALARM_MAX_ARR; block++) EEPROM_UpdateByte(newCell + block, EEPROM_ReadByte(oldCell + block));
    }
    updateByte(--alarms.num, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
  }
}
//----------------------------------Проверка будильников----------------------------------------------------
void checkAlarms(void) //проверка будильников
{
  alarms.dot = 0; //сбрасываем флаг включенных точек будильника
  for (uint8_t alm = 0; alm < alarms.num; alm++) { //опрашиваем все будильники
    if (alarmRead(alm, ALARM_MODE)) { //если будильник включен
#if ALARM_ON_BLINK_DOT != 2
      alarms.dot = ALARM_ON_BLINK_DOT + (DOT_EFFECT_NUM - 5) + 1; //мигание точек при включенном будильнике
#endif
      if (RTC.h == alarmRead(alm, ALARM_HOURS) && RTC.m == alarmRead(alm, ALARM_MINS) && (alarmRead(alm, ALARM_MODE) < 3 || (alarmRead(alm, ALARM_MODE) == 3 && RTC.DW < 6) || (alarmRead(alm, ALARM_DAYS) & (0x01 << RTC.DW)))) {
        if (!alarms.now) { //если тревога не активна
          alarms.now = alm + 1; //устанавливаем флаг тревоги
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
  if (alarms.now) { //если тревога активна
    if (!_timer_sec[TMR_ALM]) { //если пришло время выключить будильник
      alarmReset(); //сброс будильника
      return; //выходим
    }

    if (ALARM_WAINT && alarms.wait) { //если будильник в режиме ожидания
      if (!_timer_sec[TMR_ALM_WAINT]) { //если пришло время повторно включить звук
        _timer_sec[TMR_ALM_SOUND] = (uint16_t)(ALARM_TIMEOUT_SOUND * 60);
        alarms.wait = 0; //сбрасываем флаг ожидания
      }
    }
    else if (ALARM_TIMEOUT_SOUND) { //если таймаут тревоги включен
      if (!_timer_sec[TMR_ALM_SOUND]) { //если пришло время выключить тревогу
        if (ALARM_WAINT) { //если время ожидания включено
          _timer_sec[TMR_ALM_WAINT] = (uint16_t)(ALARM_WAINT * 60);
          alarms.wait = 1; //устанавливаем флаг ожидания тревоги
#if ALARM_WAINT_BLINK_DOT != 2
          alarms.dot = ALARM_WAINT_BLINK_DOT + (DOT_EFFECT_NUM - 5) + 1; //мигание точек при включенном будильнике
#endif
        }
        else alarmReset(); //сброс будильника
      }
    }
  }
}
//----------------------------------Тревога будильника---------------------------------------------------------
uint8_t alarmWarn(void) //тревога будильника
{
  boolean blink_data = 0; //флаг мигания индикаторами
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
  uint8_t radio_mode = alarmRead(alarms.now - 1, ALARM_RADIO); //текущий режим звука
#endif
#if PLAYER_TYPE || (RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE))
  boolean auto_vol = 0; //флаг автогромкости
  uint8_t cur_vol = alarmRead(alarms.now - 1, ALARM_VOLUME); //текущая громкость
#endif

#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
  if (radio_mode) { //если режим радио
    if (getPowerStatusRDA() != RDA_ERROR) { //если радиоприемник доступен
      if (!cur_vol) { //если автогромкость
        auto_vol = 1; //установили флаг автогромкости
        cur_vol = ALARM_RADIO_VOL_MIN; //установили минимальную громкость
      }
      setPowerRDA(RDA_ON); //включаем радио
      setVolumeRDA(cur_vol); //устанавливаем громкость
      setFreqRDA(radioSettings.stationsSave[alarmRead(alarms.now - 1, ALARM_SOUND)]); //устанавливаем частоту
      _timer_ms[TMR_ANIM] = ALARM_RADIO_VOL_TIME; //устанавливаем таймер
    }
    else radio_mode = 0; //отключили режим радио
  }
  else radioPowerOff(); //выключить питание радиоприемника
#endif

#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
  if (!radio_mode) {
#endif
#if PLAYER_TYPE
    if (!cur_vol) { //если автогромкость
      auto_vol = 1; //установили флаг автогромкости
      cur_vol = ALARM_RADIO_VOL_MIN; //установили минимальную громкость
    }
    playerStop(); //сброс позиции мелодии
    playerSetVolNow(cur_vol); //установка громкости
    _timer_ms[TMR_ANIM] = ALARM_PLAYER_VOL_TIME; //устанавливаем таймер
#else
    melodyPlay(alarmRead(alarms.now - 1, ALARM_SOUND), SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
  }
#endif

#if (BACKL_TYPE == 3) && ALARM_BACKL_TYPE
  backlAnimDisable(); //запретили эффекты подсветки
#if ALARM_BACKL_TYPE == 1
  changeBrightDisable(CHANGE_DYNAMIC_BACKL); //разрешить смену яркости динамичной подсветки
#endif
  setLedHue(ALARM_BACKL_COLOR, WHITE_ON); //установили цвет будильника
#endif

  _timer_ms[TMR_MS] = 0; //сбросили таймер

  while (1) {
    dataUpdate(); //обработка данных

    if (!alarms.now || alarms.wait) { //если тревога сброшена
#if PLAYER_TYPE
      playerStop(); //сброс позиции мелодии
#else
      melodyStop(); //сброс позиции мелодии
#endif
      return MAIN_PROGRAM; //выходим
    }

#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
#if PLAYER_TYPE
    if (!radio_mode && !playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarmRead(alarms.now - 1, ALARM_SOUND), PLAYER_ALARM_FOLDER); //воспроизводим мелодию
    if (auto_vol && !_timer_ms[TMR_ANIM]) { //если пришло время
      _timer_ms[TMR_ANIM] = (radio_mode) ? ALARM_RADIO_VOL_TIME : ALARM_PLAYER_VOL_TIME; //устанавливаем таймер
      if (cur_vol < ((radio_mode) ? ALARM_RADIO_VOL_MAX : ALARM_PLAYER_VOL_MAX)) cur_vol++;
      else auto_vol = 0; //сбросили флаг автогромкости
      if (radio_mode) setVolumeRDA(cur_vol); //устанавливаем громкость
      else playerSetVolNow(cur_vol); //установка громкости
    }
#else
    if (auto_vol && !_timer_ms[TMR_ANIM]) { //если пришло время
      _timer_ms[TMR_ANIM] = ALARM_RADIO_VOL_TIME; //устанавливаем таймер
      if (cur_vol < ALARM_RADIO_VOL_MAX) cur_vol++;
      else auto_vol = 0; //сбросили флаг автогромкости
      setVolumeRDA(cur_vol); //устанавливаем громкость
    }
#endif
#elif PLAYER_TYPE
    if (!playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarmRead(alarms.now - 1, ALARM_SOUND), PLAYER_ALARM_FOLDER); //воспроизводим мелодию
    if (auto_vol && !_timer_ms[TMR_ANIM]) { //если пришло время
      _timer_ms[TMR_ANIM] = ALARM_PLAYER_VOL_TIME; //устанавливаем таймер
      if (cur_vol < ALARM_PLAYER_VOL_MAX) cur_vol++;
      else auto_vol = 0; //сбросили флаг автогромкости
      playerSetVolNow(cur_vol); //установка громкости
    }
#endif

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = ALARM_BLINK_TIME; //устанавливаем таймер

      switch (blink_data) {
        case 0: indiClr(); break; //очистка индикаторов
        case 1:
          indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
          indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
          indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
          break;
      }
      dotSetBright((blink_data) ? dot.menuBright : 0); //установили точки
#if (BACKL_TYPE == 3) && ALARM_BACKL_TYPE
#if ALARM_BACKL_TYPE == 1
      setLedBright((blink_data) ? backl.maxBright : 0); //установили яркость
#else
      setLedBright((blink_data) ? backl.menuBright : 0); //установили яркость
#endif
#endif
      blink_data = !blink_data; //мигаем временем
    }

    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
      case RIGHT_KEY_PRESS: //клик правой кнопкой
      case SET_KEY_PRESS: //клик средней кнопкой
      case ADD_KEY_PRESS: //клик дополнительной кнопкой
        if (ALARM_WAINT) { //если есть время ожидания
          alarms.wait = 1; //устанавливаем флаг ожидания
#if ALARM_WAINT_BLINK_DOT != 2
          alarms.dot = ALARM_WAINT_BLINK_DOT + (DOT_EFFECT_NUM - 5) + 1; //мигание точек при включенном будильнике
#endif
          _timer_sec[TMR_ALM_WAINT] = (uint16_t)(ALARM_WAINT * 60);
          _timer_sec[TMR_ALM_SOUND] = 0;
#if PLAYER_TYPE
          playerSetTrackNow(PLAYER_ALARM_WAIT_SOUND, PLAYER_GENERAL_FOLDER); //звук ожидания будильника
#else
          melodyPlay(SOUND_ALARM_WAINT, SOUND_LINK(general_sound), REPLAY_ONCE); //звук ожидания будильника
#endif
        }
        else alarmDisable(); //отключение будильника
        return MAIN_PROGRAM; //выходим

      case LEFT_KEY_HOLD: //удержание левой кнопки
      case RIGHT_KEY_HOLD: //удержание правой кнопки
      case SET_KEY_HOLD: //удержание средней кнопки
      case ADD_KEY_HOLD: //удержание дополнительной кнопки
        alarmDisable(); //отключение будильника
        return MAIN_PROGRAM; //выходим
    }
  }
  return INIT_PROGRAM;
}
//----------------------------------Обработка данных------------------------------------------------
void dataUpdate(void) //обработка данных
{
  static uint16_t timeClock; //счетчик реального времени
  static uint16_t timerCorrect; //остаток для коррекции времени
  static uint16_t timerSQW = SQW_MIN_TIME; //таймер контроля сигнала SQW
#if BACKL_TYPE == 3
  backlEffect(); //анимация подсветки
  showLeds(); //отрисовка светодиодов
#elif BACKL_TYPE
  backlFlash(); //"дыхание" подсветки
#endif

  dotFlash(); //мигаем точками

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

#if LIGHT_SENS_ENABLE
  lightSensCheck(); //проверка сенсора яркости освещения
#endif

  for (uint8_t _tick = tick_ms; _tick > 0; _tick--) { //если был тик то обрабатываем данные
    tick_ms--; //убавили счетчик миллисекунд

    indiStateCheck(); //проверка состояния динамической индикации
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
        tick_sec = 1; //установили тики
        SET_ERROR(SQW_LONG_ERROR); //устанавливаем ошибку длинного сигнала
      }
    }
    else { //если внешние тактирование не обнаружено
      timeClock += msDec; //добавляем ко времени период таймера
      if (timeClock >= 1000) { //если прошла секунда
        timeClock -= 1000; //оставляем остаток
        tick_sec++; //прибавляем секунду
      }
    }
    RESET_WDT; //сбрасываем таймер WDT
  }

  if (tick_sec) { //если был тик, обрабатываем данные
    tick_sec--; //убавили счетчик секунд

#if GEN_ENABLE
    converterCheck(); //проверка состояния преобразователя
#endif
    indiCheck(); //проверка состояния динамической индикации

    for (uint8_t tm = 0; tm < TIMERS_SEC_NUM; tm++) { //опрашиваем все таймеры
      if (_timer_sec[tm]) _timer_sec[tm]--; //если таймер активен
    }

#if ALARM_TYPE
    alarmDataUpdate(); //проверка таймеров будильников
#endif

    if (EIMSK) { //если работаем от внешнего тактирования
      if (timerSQW < SQW_MIN_TIME) { //если сигнал слишком короткий
        EIMSK = 0; //перешли на внутреннее тактирование
        tick_sec = 0; //сбросили тики
        timeClock = timerSQW; //установили таймер секунды
        SET_ERROR(SQW_SHORT_ERROR); //устанавливаем ошибку короткого сигнала
        return; //выходим
      }
      timerSQW = 0; //сбросили таймер
    }
    else if (!_timer_sec[TMR_SYNC] && RTC.s == RTC_SYNC_PHASE) { //если работаем от внутреннего тактирования
      _timer_sec[TMR_SYNC] = ((uint16_t)RTC_SYNC_TIME * 60); //установили таймер
      if (getTime(RTC_CHECK_OSF)) RTC.s--; //синхронизируем время
    }

    secUpd = dot.update = 0; //очищаем флаги секунды и точек

#if LAMP_NUM > 4
    if (mainSettings.secsMode && (animShow == ANIM_NULL)) animShow = ANIM_SECS; //показать анимацию переключения цифр
#endif

#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
    switch (timer.mode) {
      case 1: if (timer.count != 65535) timer.count++; break;
      case 2: if (timer.count) timer.count--; break;
    }
#endif

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
        changeBright(); //установка яркости от времени суток
        hourSound(); //звук смены часа
      }
      if (fastSettings.flipMode && (animShow < ANIM_MAIN)) animShow = ANIM_MINS; //показать анимацию переключения цифр
#if ALARM_TYPE
      checkAlarms(); //проверяем будильники на совпадение
#endif
    }
#if LIGHT_SENS_ENABLE
    lightSensUpdate(); //обработка сенсора яркости освещения
#endif
#if MOV_PORT_ENABLE
    if (indi.sleepMode && MOV_CHK) {
      if (mainTask == SLEEP_PROGRAM) indi.sleepMode = SLEEP_DISABLE; //отключаем сон если в режиме сна
      _timer_sec[TMR_SLEEP] = mainSettings.timeSleep[indi.sleepMode - 1]; //установли время ожидания режима пробуждения
    }
#endif
  }
}
//----------------------------Настройки времени----------------------------------
uint8_t settings_time(void) //настройки времени
{
  boolean time_update = 0; //флаг изменения времени
  boolean blink_data = 0; //мигание сигментами
  uint8_t cur_mode = 0; //текущий режим
  uint8_t time_out = 0; //таймаут автовыхода

  dotSetBright(dot.menuBright); //включаем точки

  _timer_ms[TMR_MS] = 0; //сбросили таймер

#if BACKL_TYPE == 3
  backlAnimDisable(); //запретили эффекты подсветки
  setLedBright(backl.menuBright); //установили максимальную яркость
#endif

#if PLAYER_TYPE
  if (mainSettings.knockSound) playerSetTrackNow(PLAYER_TIME_SET_SOUND, PLAYER_GENERAL_FOLDER); //воспроизводим название меню
#endif

  //настройки
  while (1) {
    dataUpdate(); //обработка данных

    if (!secUpd) {
      secUpd = 1;
      if (++time_out >= SETTINGS_TIMEOUT) {
        animShow = ANIM_MAIN; //установили флаг анимации
        return MAIN_PROGRAM;
      }
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
#if BACKL_TYPE == 3
      setBacklHue((cur_mode % 2) * 2, (cur_mode != 4) ? 2 : 4, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); //подсветка активных разрядов
#endif
      blink_data = !blink_data; //мигание сигментами
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (cur_mode) {
          //настройка времени
          case 0: if (RTC.h > 0) RTC.h--; else RTC.h = 23; RTC.s = 0; time_update = 1; break; //часы
          case 1: if (RTC.m > 0) RTC.m--; else RTC.m = 59; RTC.s = 0; time_update = 1; break; //минуты

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
          case 0: if (RTC.h < 23) RTC.h++; else RTC.h = 0; RTC.s = 0; time_update = 1; break; //часы
          case 1: if (RTC.m < 59) RTC.m++; else RTC.m = 0; RTC.s = 0; time_update = 1; break; //минуты

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
        if (cur_mode != 4) dotSetBright(dot.menuBright); //включаем точки
        else dotSetBright(0); //выключаем точки
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        if (cur_mode < 2 && time_update) RTC.s = 0; //сбрасываем секунды
        return MAIN_PROGRAM;
    }
  }
  return INIT_PROGRAM;
}
//-----------------------------Настройка будильника------------------------------------
uint8_t settings_singleAlarm(void) //настройка будильника
{
  boolean cur_indi = 0; //текущий индикатор
  boolean blink_data = 0; //мигание сигментами
  uint8_t alarm[ALARM_MAX_ARR]; //массив данных о будильнике
  uint8_t cur_mode = 0; //текущий режим
  uint8_t cur_day = 1; //текущий день недели
  uint8_t time_out = 0; //таймаут автовыхода

  dotSetBright(dot.menuBright); //включаем точки

  alarmReset(); //сброс будильника
  alarmReadBlock(1, alarm); //читаем блок данных

  _timer_ms[TMR_MS] = 0; //сбросили таймер

#if BACKL_TYPE == 3
  backlAnimDisable(); //запретили эффекты подсветки
  setLedBright(backl.menuBright); //установили максимальную яркость
#endif

#if PLAYER_TYPE
  if (mainSettings.knockSound) playerSetTrackNow(PLAYER_ALARM_SET_SOUND, PLAYER_GENERAL_FOLDER); //воспроизводим название меню
#endif

  while (1) {
    dataUpdate(); //обработка данных

#if PLAYER_TYPE
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
    if (cur_mode == 3 && !alarm[ALARM_RADIO] && !playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarm[ALARM_SOUND], PLAYER_ALARM_FOLDER); //воспроизводим мелодию будильника
#else
    if (cur_mode == 3 && !playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarm[ALARM_SOUND], PLAYER_ALARM_FOLDER); //воспроизводим мелодию будильника
#endif
#endif

    if (!secUpd) {
      secUpd = 1;
      if (++time_out >= SETTINGS_TIMEOUT) {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
        if ((cur_mode == 3) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
#if PLAYER_TYPE
        playerStop(); //сброс воспроизведения мелодии
#else
        melodyStop(); //сброс воспроизведения мелодии
#endif
        animShow = ANIM_MAIN; //установили флаг анимации
        return MAIN_PROGRAM; //выходим по тайм-ауту
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
#if PLAYER_TYPE
          if (!blink_data || cur_indi) indiPrintNum(alarm[ALARM_VOLUME], 0, 2, 0); //вывод громкости мелодии
          if (!blink_data || !cur_indi) indiPrintNum(alarm[ALARM_SOUND] + !alarm[ALARM_RADIO], 2, 2, 0); //вывод номера мелодии
#else
          if (alarm[ALARM_RADIO]) {
            if (!blink_data || cur_indi) indiPrintNum(alarm[ALARM_VOLUME], 0, 2, 0); //вывод громкости мелодии
            if (!blink_data || !cur_indi) indiPrintNum(alarm[ALARM_SOUND], 2, 2, 0); //вывод номера мелодии
          }
          else if (!blink_data) indiPrintNum(alarm[ALARM_SOUND] + 1, 2, 2, 0); //вывод номера мелодии
#endif
#else
#if PLAYER_TYPE
          if (!blink_data || cur_indi) indiPrintNum(alarm[ALARM_VOLUME], 0, 2, 0); //вывод громкости мелодии
          if (!blink_data || !cur_indi) indiPrintNum(alarm[ALARM_SOUND] + 1, 2, 2, 0); //вывод номера мелодии
#else
          if (!blink_data) indiPrintNum(alarm[ALARM_SOUND] + 1, 2, 2, 0); //вывод номера мелодии
#endif
#endif
          break;
      }
#if BACKL_TYPE == 3
      switch (cur_mode) {
        case 1: setBacklHue(0, 1, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
        case 2: setBacklHue((cur_indi) ? 3 : 2, 1, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
#if !PLAYER_TYPE
        case 3:
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
          if (alarm[ALARM_RADIO]) setBacklHue(cur_indi * 2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); //подсветка активных разрядов
          else setBacklHue(2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2);  //подсветка активных разрядов
#else
          setBacklHue(2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2);  //подсветка активных разрядов
#endif
          break;
#endif
        default: setBacklHue(cur_indi * 2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
      }
#endif
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] > 0) alarm[ALARM_VOLUME]--; else alarm[ALARM_VOLUME] = (alarm[ALARM_RADIO]) ? RADIO_MAX_VOL : PLAYER_MAX_VOL; //громкость
                if (alarm[ALARM_RADIO]) setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_RADIO_VOL_MAX); //устанавливаем громкость
                else playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_PLAYER_VOL_MAX); //установка громкости
                break;
              case 1:
                if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = (alarm[ALARM_RADIO]) ? 9 : (PLAYER_ALARM_MAX - 1); //мелодия
                if (alarm[ALARM_RADIO]) setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
                break;
            }
#else
            if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = (alarm[ALARM_RADIO]) ? 9 : (SOUND_MAX(alarm_sound) - 1); //мелодия
            if (alarm[ALARM_RADIO]) setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
            else melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#else
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] > 0) alarm[ALARM_VOLUME]--; else alarm[ALARM_VOLUME] = PLAYER_MAX_VOL; //громкость
                playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
                break;
              case 1:
                if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = PLAYER_ALARM_MAX - 1; //мелодия
                break;
            }
#else
            if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = SOUND_MAX(alarm_sound) - 1; //мелодия
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] < ((alarm[ALARM_RADIO]) ? RADIO_MAX_VOL : PLAYER_MAX_VOL)) alarm[ALARM_VOLUME]++; else alarm[ALARM_VOLUME] = 0; //громкость
                if (alarm[ALARM_RADIO]) setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_RADIO_VOL_MAX); //устанавливаем громкость
                else playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_PLAYER_VOL_MAX); //установка громкости
                break;
              case 1:
                if (alarm[ALARM_SOUND] < ((alarm[ALARM_RADIO]) ? 9 : (PLAYER_ALARM_MAX - 1))) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
                if (alarm[ALARM_RADIO]) setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
                break;
            }
#else
            if (alarm[ALARM_SOUND] < ((alarm[ALARM_RADIO]) ? 9 : (SOUND_MAX(alarm_sound) - 1))) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
            if (alarm[ALARM_RADIO]) setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
            else melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#else
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] < PLAYER_MAX_VOL) alarm[ALARM_VOLUME]++; else alarm[ALARM_VOLUME] = 0; //громкость
                playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_PLAYER_VOL_MAX); //установка громкости
                break;
              case 1:
                if (alarm[ALARM_SOUND] < (PLAYER_ALARM_MAX - 1)) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
                break;
            }
#else
            if (alarm[ALARM_SOUND] < (SOUND_MAX(alarm_sound) - 1)) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
        if ((cur_mode == 3) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
        if (cur_mode > 0) cur_mode--; else cur_mode = 3; //переключение пунктов

        if ((cur_mode == 2) && (alarm[ALARM_MODE] < 4)) cur_mode = 1; //если нет дней недели
        if (cur_mode == 3) { //если режим настройки мелодии
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
          if (alarm[ALARM_RADIO] && (getPowerStatusRDA() != RDA_ERROR)) { //если режим радиобудильника
            setPowerRDA(RDA_ON); //включаем радио
            setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_RADIO_VOL_MAX); //устанавливаем громкость
            setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
          }
          else { //иначе обычный режим
            alarm[ALARM_RADIO] = 0; //обычный режим
            setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
            playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_PLAYER_VOL_MAX); //установка громкости
#else
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
          }
#else
#if PLAYER_TYPE
          playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
#else
          melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#endif
        }
        dotSetBright((cur_mode) ? 0 : dot.menuBright); //включаем точки

        cur_indi = 0; //сбрасываем текущий индикатор
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case RIGHT_KEY_HOLD: //удержание правой кнопки
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
        if ((cur_mode == 3) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
        if (cur_mode < 3) cur_mode++; else cur_mode = 0; //переключение пунктов

        if ((cur_mode == 2) && (alarm[ALARM_MODE] < 4)) cur_mode = 3; //если нет дней недели
        if (cur_mode == 3) { //если режим настройки мелодии
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
          if (alarm[ALARM_RADIO] && (getPowerStatusRDA() != RDA_ERROR)) { //если режим радиобудильника
            setPowerRDA(RDA_ON); //включаем радио
            setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_RADIO_VOL_MAX); //устанавливаем громкость
            setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
          }
          else { //иначе обычный режим
            alarm[ALARM_RADIO] = 0; //обычный режим
            setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
            playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_PLAYER_VOL_MAX); //установка громкости
#else
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
          }
#else
#if PLAYER_TYPE
          playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
#else
          melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#endif
        }
        dotSetBright((cur_mode) ? 0 : dot.menuBright); //включаем точки

        cur_indi = 0; //сбрасываем текущий индикатор
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
        if ((cur_mode == 3) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
        alarmWriteBlock(1, alarm); //записать блок основных данных будильника и выйти
        return MAIN_PROGRAM;

#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
      case ADD_KEY_HOLD: //удержание дополнительной кнопки
        if (cur_mode == 3) {
          if (alarm[ALARM_RADIO]) { //если режим радиобудильника
            alarm[ALARM_RADIO] = 0;
            alarm[ALARM_SOUND] = 0;
            setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
            alarm[ALARM_VOLUME] = (PLAYER_MAX_VOL / 2);
            playerSetVolNow(alarm[ALARM_VOLUME]); //установка громкости
#endif
          }
          else if (getPowerStatusRDA() != RDA_ERROR) { //иначе если радиоприемник доступен
            alarm[ALARM_RADIO] = 1;
            alarm[ALARM_SOUND] = 0;
            alarm[ALARM_VOLUME] = (RADIO_MAX_VOL / 2);
            setPowerRDA(RDA_ON); //включаем радио
            setVolumeRDA(alarm[ALARM_VOLUME]); //устанавливаем громкость
            setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
          }
          blink_data = 0; //сбрасываем флаги
          _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        }
        break;
#endif
    }
  }
  return INIT_PROGRAM;
}
//-----------------------------Настройка будильников------------------------------------
uint8_t settings_multiAlarm(void) //настройка будильников
{
  boolean cur_indi = 0; //текущий индикатор
  boolean blink_data = 0; //мигание сигментами
  uint8_t alarm[ALARM_MAX_ARR]; //массив данных о будильнике
  uint8_t cur_mode = 0; //текущий режим
  uint8_t cur_day = 1; //текущий день недели
  uint8_t time_out = 0; //таймаут автовыхода
  uint8_t curAlarm = alarms.num > 0;

  alarmReset(); //сброс будильника
  alarmReadBlock(curAlarm, alarm); //читаем блок данных

  _timer_ms[TMR_MS] = 0; //сбросили таймер

#if BACKL_TYPE == 3
  backlAnimDisable(); //запретили эффекты подсветки
  setLedBright(backl.menuBright); //установили максимальную яркость
#endif

#if PLAYER_TYPE
  if (mainSettings.knockSound) playerSetTrackNow(PLAYER_ALARM_SET_SOUND, PLAYER_GENERAL_FOLDER); //воспроизводим название меню
#endif

  while (1) {
    dataUpdate(); //обработка данных

#if PLAYER_TYPE
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
    if (cur_mode == 4 && !alarm[ALARM_RADIO] && !playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarm[ALARM_SOUND], PLAYER_ALARM_FOLDER); //воспроизводим мелодию будильника
#else
    if (cur_mode == 4 && !playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarm[ALARM_SOUND], PLAYER_ALARM_FOLDER); //воспроизводим мелодию будильника
#endif
#endif

    if (!secUpd) {
      secUpd = 1;
      if (++time_out >= SETTINGS_TIMEOUT) {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
        if ((cur_mode == 4) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
#if PLAYER_TYPE
        playerStop(); //сброс воспроизведения мелодии
#else
        melodyStop(); //сброс воспроизведения мелодии
#endif
        animShow = ANIM_MAIN; //установили флаг анимации
        return MAIN_PROGRAM; //выходим по тайм-ауту
      }
    }

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

      indiClr(); //очистка индикаторов
      indiPrintNum(cur_mode + 1, 5); //режим
      switch (cur_mode) {
        case 0:
          if (!blink_data) indiPrintNum(curAlarm, 0, 2, 0); //вывод номера будильника
          if (curAlarm) indiPrintNum(alarm[ALARM_MODE], 3); //вывод режима
          break;
        case 1:
          if (!blink_data || cur_indi) indiPrintNum(alarm[ALARM_HOURS], 0, 2, 0); //вывод часов
          if (!blink_data || !cur_indi) indiPrintNum(alarm[ALARM_MINS], 2, 2, 0); //вывод минут
          break;
        case 2:
        case 3:
          if (!blink_data || cur_mode != 1) indiPrintNum(alarm[ALARM_MODE], 0); //вывод режима
          if (alarm[ALARM_MODE] > 3) {
            if (!blink_data || cur_mode != 2 || cur_indi) indiPrintNum(cur_day, 2); //вывод дня недели
            if (!blink_data || cur_mode != 2 || !cur_indi) indiPrintNum((alarm[ALARM_DAYS] >> cur_day) & 0x01, 3); //вывод установки
          }
          break;
        case 4:
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
#if PLAYER_TYPE
          if (!blink_data || cur_indi) indiPrintNum(alarm[ALARM_VOLUME], 0, 2, 0); //вывод громкости мелодии
          if (!blink_data || !cur_indi) indiPrintNum(alarm[ALARM_SOUND] + !alarm[ALARM_RADIO], 2, 2, 0); //вывод номера мелодии
#else
          if (alarm[ALARM_RADIO]) {
            if (!blink_data || cur_indi) indiPrintNum(alarm[ALARM_VOLUME], 0, 2, 0); //вывод громкости мелодии
            if (!blink_data || !cur_indi) indiPrintNum(alarm[ALARM_SOUND], 2, 2, 0); //вывод номера мелодии
          }
          else if (!blink_data) indiPrintNum(alarm[ALARM_SOUND] + 1, 2, 2, 0); //вывод номера мелодии
#endif
#else
#if PLAYER_TYPE
          if (!blink_data || cur_indi) indiPrintNum(alarm[ALARM_VOLUME], 0, 2, 0); //вывод громкости мелодии
          if (!blink_data || !cur_indi) indiPrintNum(alarm[ALARM_SOUND] + 1, 2, 2, 0); //вывод номера мелодии
#else
          if (!blink_data) indiPrintNum(alarm[ALARM_SOUND] + 1, 2, 2, 0); //вывод номера мелодии
#endif
#endif
          break;
      }
#if BACKL_TYPE == 3
      switch (cur_mode) {
        case 0: setBacklHue(0, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
        case 2: setBacklHue(0, 1, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
        case 3: setBacklHue((cur_indi) ? 3 : 2, 1, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
#if !PLAYER_TYPE
        case 4:
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
          if (alarm[ALARM_RADIO]) setBacklHue(cur_indi * 2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); //подсветка активных разрядов
          else setBacklHue(2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2);  //подсветка активных разрядов
#else
          setBacklHue(2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2);  //подсветка активных разрядов
#endif
          break;
#endif
        default: setBacklHue(cur_indi * 2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
      }
#endif
      blink_data = !blink_data; //мигание сигментами
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (cur_mode) {
          case 0: if (curAlarm > (alarms.num > 0)) curAlarm--; else curAlarm = alarms.num; alarmReadBlock(curAlarm, alarm); break; //будильник

          //настройка времени будильника
          case 1:
            switch (cur_indi) {
              case 0: if (alarm[ALARM_HOURS] > 0) alarm[ALARM_HOURS]--; else alarm[ALARM_HOURS] = 23; break; //часы
              case 1: if (alarm[ALARM_MINS] > 0) alarm[ALARM_MINS]--; else alarm[ALARM_MINS] = 59; break; //минуты
            }
            break;

          //настройка режима будильника
          case 2: if (alarm[ALARM_MODE] > 0) alarm[ALARM_MODE]--; else alarm[ALARM_MODE] = 4; break; //режим
          case 3:
            switch (cur_indi) {
              case 0: if (cur_day > 1) cur_day--; else cur_day = 7; break; //день недели
              case 1: alarm[ALARM_DAYS] &= ~(0x01 << cur_day); break; //установка
            }
            break;

          //настройка мелодии будильника
          case 4:
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] > 0) alarm[ALARM_VOLUME]--; else alarm[ALARM_VOLUME] = (alarm[ALARM_RADIO]) ? RADIO_MAX_VOL : PLAYER_MAX_VOL; //громкость
                if (alarm[ALARM_RADIO]) setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_RADIO_VOL_MAX); //устанавливаем громкость
                else playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_PLAYER_VOL_MAX); //установка громкости
                break;
              case 1:
                if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = (alarm[ALARM_RADIO]) ? 9 : (PLAYER_ALARM_MAX - 1); //мелодия
                if (alarm[ALARM_RADIO]) setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
                break;
            }
#else
            if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = (alarm[ALARM_RADIO]) ? 9 : (SOUND_MAX(alarm_sound) - 1); //мелодия
            if (alarm[ALARM_RADIO]) setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
            else melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#else
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] > 0) alarm[ALARM_VOLUME]--; else alarm[ALARM_VOLUME] = PLAYER_MAX_VOL; //громкость
                playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
                break;
              case 1:
                if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = PLAYER_ALARM_MAX - 1; //мелодия
                break;
            }
#else
            if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = SOUND_MAX(alarm_sound) - 1; //мелодия
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#endif
            break;
        }
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (cur_mode) {
          case 0: if (curAlarm < alarms.num) curAlarm++; else curAlarm = (alarms.num > 0); alarmReadBlock(curAlarm, alarm); break; //будильник

          //настройка времени будильника
          case 1:
            switch (cur_indi) {
              case 0: if (alarm[ALARM_HOURS] < 23) alarm[ALARM_HOURS]++; else alarm[ALARM_HOURS] = 0; break; //часы
              case 1: if (alarm[ALARM_MINS] < 59) alarm[ALARM_MINS]++; else alarm[ALARM_MINS] = 0; break; //минуты
            }
            break;

          //настройка режима будильника
          case 2: if (alarm[ALARM_MODE] < 4) alarm[ALARM_MODE]++; else alarm[ALARM_MODE] = 0; break; //режим
          case 3:
            switch (cur_indi) {
              case 0: if (cur_day < 7) cur_day++; else cur_day = 1; break; //день недели
              case 1: alarm[ALARM_DAYS] |= (0x01 << cur_day); break; //установка
            }
            break;

          //настройка мелодии будильника
          case 4:
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] < ((alarm[ALARM_RADIO]) ? RADIO_MAX_VOL : PLAYER_MAX_VOL)) alarm[ALARM_VOLUME]++; else alarm[ALARM_VOLUME] = 0; //громкость
                if (alarm[ALARM_RADIO]) setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_RADIO_VOL_MAX); //устанавливаем громкость
                else playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_PLAYER_VOL_MAX); //установка громкости
                break;
              case 1:
                if (alarm[ALARM_SOUND] < ((alarm[ALARM_RADIO]) ? 9 : (PLAYER_ALARM_MAX - 1))) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
                if (alarm[ALARM_RADIO]) setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
                break;
            }
#else
            if (alarm[ALARM_SOUND] < ((alarm[ALARM_RADIO]) ? 9 : (SOUND_MAX(alarm_sound) - 1))) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
            if (alarm[ALARM_RADIO]) setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
            else melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#else
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] < PLAYER_MAX_VOL) alarm[ALARM_VOLUME]++; else alarm[ALARM_VOLUME] = 0; //громкость
                playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_PLAYER_VOL_MAX); //установка громкости
                break;
              case 1:
                if (alarm[ALARM_SOUND] < (PLAYER_ALARM_MAX - 1)) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
                break;
            }
#else
            if (alarm[ALARM_SOUND] < (SOUND_MAX(alarm_sound) - 1)) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#endif
            break;
        }
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        if (!cur_mode && curAlarm) {
          cur_mode = 1;
          cur_indi = 0;
          dotSetBright(dot.menuBright); //включаем точки
        }
        else cur_indi = !cur_indi;
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case LEFT_KEY_HOLD: //удержание левой кнопки
        if (!cur_mode) {
          if (curAlarm) { //если есть будильники в памяти
            delAlarm(curAlarm - 1); //удалить текущий будильник
            dotSetBright(dot.menuBright); //включаем точки
            for (_timer_ms[TMR_MS] = 500; _timer_ms[TMR_MS];) dataUpdate(); //обработка данных
            dotSetBright(0); //выключаем точки
            if (curAlarm > (alarms.num > 0)) curAlarm--; //убавляем номер текущего будильника
            else curAlarm = (alarms.num > 0);
            alarmReadBlock(curAlarm, alarm); //читаем блок данных
          }
        }
        else {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
          if ((cur_mode == 4) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
          if (cur_mode > 1) cur_mode--; else cur_mode = 4; //переключение пунктов

          if ((cur_mode == 3) && (alarm[ALARM_MODE] < 4)) cur_mode = 2; //если нет дней недели
          if (cur_mode == 4) { //если режим настройки мелодии
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
            if (alarm[ALARM_RADIO] && (getPowerStatusRDA() != RDA_ERROR)) { //если режим радиобудильника
              setPowerRDA(RDA_ON); //включаем радио
              setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_RADIO_VOL_MAX); //устанавливаем громкость
              setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
            }
            else { //иначе обычный режим
              alarm[ALARM_RADIO] = 0; //обычный режим
              setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
              playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_PLAYER_VOL_MAX); //установка громкости
#else
              melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
            }
#else
#if PLAYER_TYPE
            playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
#else
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#endif
          }
          dotSetBright((cur_mode == 1) ? 0 : dot.menuBright); //включаем точки
        }

        cur_indi = 0;
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case RIGHT_KEY_HOLD: //удержание правой кнопки
        if (!cur_mode) {
          newAlarm(); //создать новый будильник
          dotSetBright(dot.menuBright); //включаем точки
          for (_timer_ms[TMR_MS] = 500; _timer_ms[TMR_MS];) dataUpdate(); //обработка данных
          dotSetBright(0); //выключаем точки
          curAlarm = alarms.num;
          alarmReadBlock(curAlarm, alarm); //читаем блок данных
        }
        else {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
          if ((cur_mode == 4) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
          if (cur_mode < 4) cur_mode++; else cur_mode = 1; //переключение пунктов

          if ((cur_mode == 3) && (alarm[ALARM_MODE] < 4)) cur_mode = 4; //если нет дней недели
          if (cur_mode == 4) { //если режим настройки мелодии
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
            if (alarm[ALARM_RADIO] && (getPowerStatusRDA() != RDA_ERROR)) { //если режим радиобудильника
              setPowerRDA(RDA_ON); //включаем радио
              setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_RADIO_VOL_MAX); //устанавливаем громкость
              setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
            }
            else { //иначе обычный режим
              alarm[ALARM_RADIO] = 0; //обычный режим
              setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
              playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_PLAYER_VOL_MAX); //установка громкости
#else
              melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
            }
#else
#if PLAYER_TYPE
            playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
#else
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#endif
          }
          dotSetBright((cur_mode == 1) ? 0 : dot.menuBright); //включаем точки
        }


        cur_indi = 0;
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        if (!cur_mode) return MAIN_PROGRAM; //выход
        else {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
          if ((cur_mode == 4) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
          alarmWriteBlock(curAlarm, alarm); //записать блок основных данных будильника
          dotSetBright(0); //выключаем точки
          cur_mode = 0; //выбор будильника
          blink_data = 0; //сбрасываем флаги
          _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        }
        break;

#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
      case ADD_KEY_HOLD: //удержание дополнительной кнопки
        if (cur_mode == 4) {
          if (alarm[ALARM_RADIO]) { //если режим радиобудильника
            alarm[ALARM_RADIO] = 0;
            alarm[ALARM_SOUND] = 0;
            setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
            alarm[ALARM_VOLUME] = (PLAYER_MAX_VOL / 2);
            playerSetVolNow(alarm[ALARM_VOLUME]); //установка громкости
#endif
          }
          else if (getPowerStatusRDA() != RDA_ERROR) { //иначе если радиоприемник доступен
            alarm[ALARM_RADIO] = 1;
            alarm[ALARM_SOUND] = 0;
            alarm[ALARM_VOLUME] = (RADIO_MAX_VOL / 2);
            setPowerRDA(RDA_ON); //включаем радио
            setVolumeRDA(alarm[ALARM_VOLUME]); //устанавливаем громкость
            setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
          }
          blink_data = 0; //сбрасываем флаги
          _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        }
        break;
#endif
    }
  }
  return INIT_PROGRAM;
}
//-----------------------------Настроки основные------------------------------------
uint8_t settings_main(void) //настроки основные
{
  boolean set = 0; //режим настройки
  boolean cur_indi = 0; //текущий индикатор
  boolean blink_data = 0; //мигание сигментами
  uint8_t cur_mode = 0; //текущий режим
  uint8_t animDemo = 0; //флаг демонстрации анимации
  uint8_t time_out = 0; //таймаут автовыхода

  _timer_ms[TMR_MS] = 0; //сбросили таймер

#if BACKL_TYPE == 3
  backlAnimDisable(); //запретили эффекты подсветки
  setLedBright(backl.menuBright); //установили максимальную яркость
#endif

#if PLAYER_TYPE
  if (mainSettings.knockSound) playerSetTrackNow(PLAYER_MAIN_MENU_START, PLAYER_MENU_FOLDER); //воспроизводим название меню
#endif

  //настройки
  while (1) {
    dataUpdate(); //обработка данных

    if (!secUpd) {
      secUpd = 1;
      if (++time_out >= SETTINGS_TIMEOUT) {
        animShow = ANIM_MAIN; //установили флаг анимации
        return MAIN_PROGRAM;
      }
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
#if BACKL_TYPE == 3
          setBacklHue((LAMP_NUM / 2 - 1), 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); //подсветка активных разрядов
#endif
          break;
        case 1:
          if (animDemo == 1) { //если нужно отобразить демонстрацию эффекта
            animDemo = 0; //сбросили флаг демонстрации
#if BACKL_TYPE == 3
            setLedHue(BACKL_MENU_COLOR_1, WHITE_ON); //подсветка активных разрядов
#endif
            switch (cur_mode) {
              case SET_AUTO_TEMP: animIndi(mainSettings.autoTempFlip, FLIP_DEMO); break; //демонстрация анимации показа температуры
              case SET_BURN_MODE:
#if LAMP_NUM > 4
                if (!cur_indi) {
                  burnIndi(mainSettings.burnMode, BURN_DEMO); //демонстрация антиотравления индикаторов
                  dotSetBright(dot.menuBright); //включаем точки
                }
                else animIndi(mainSettings.secsMode + 1, FLIP_DEMO); //демонстрация анимации секунд
#else
                burnIndi(mainSettings.burnMode, BURN_DEMO); //демонстрация антиотравления индикаторов
                dotSetBright(dot.menuBright); //включаем точки
#endif
                break;
            }
            _timer_ms[TMR_MS] = blink_data = 0; //сбрасываем флаги
          }
          else {
            if (animDemo > 1) { //если нужно отобразить анимацию
              animDemo = 1; //перешли в режим анимации
              _timer_ms[TMR_MS] = SETTINGS_WAIT_TIME; //устанавливаем таймер
            }
            indiPrintNum(cur_mode + 1, 4, 2); //режим
            switch (cur_mode) {
              case SET_TIME_FORMAT: if (!blink_data) indiPrintNum((mainSettings.timeFormat) ? 12 : 24, 2); break; //вывод формата времени
              case SET_GLITCH: if (!blink_data) indiPrintNum(mainSettings.glitchMode, 3); break; //вывод режима глюков
              case SET_BTN_SOUND: //вывод звука кнопок
#if PLAYER_TYPE
                if (!blink_data || cur_indi) indiPrintNum(mainSettings.volumeSound, 0, 2, 0); //громкость озвучки
                if (!blink_data || !cur_indi) indiPrintNum(mainSettings.knockSound, 3); //звук кнопок или озвучка
#else
                if (!blink_data) indiPrintNum(mainSettings.knockSound, 3); //звук кнопок или озвучка
#endif
                break;
              case SET_HOUR_TIME:
                if (!blink_data || cur_indi) indiPrintNum(mainSettings.timeHour[TIME_NIGHT], 0, 2, 0); //вывод часа начала звукового оповещения нового часа
                if (!blink_data || !cur_indi) indiPrintNum(mainSettings.timeHour[TIME_DAY], 2, 2, 0); //вывод часа окончания звукового оповещения нового часа
                break;
              case SET_BRIGHT_TIME:
                if (!blink_data || cur_indi) indiPrintNum(mainSettings.timeBright[TIME_NIGHT], 0, 2, 0); //вывод часа начала ночной посветки
                if (!blink_data || !cur_indi) indiPrintNum(mainSettings.timeBright[TIME_DAY], 2, 2, 0); //вывод часа окончания ночной посветки
                break;
              case SET_INDI_BRIGHT:
                if (!blink_data || cur_indi) indiPrintNum(mainSettings.indiBright[TIME_NIGHT], 0, 2, 0); //яркости ночь
                if (!blink_data || !cur_indi) indiPrintNum(mainSettings.indiBright[TIME_DAY], 2, 2, 0); //вывод яркости день
                break;
#if BACKL_TYPE
              case SET_BACKL_BRIGHT:
                if (!blink_data || cur_indi) indiPrintNum(mainSettings.backlBright[TIME_NIGHT] / 10, 0, 2, 0); //яркости ночь
                if (!blink_data || !cur_indi) indiPrintNum(mainSettings.backlBright[TIME_DAY] / 10, 2, 2, 0); //вывод яркости день
                break;
#endif
              case SET_DOT_BRIGHT:
#if (NEON_DOT != 3) || !DOTS_PORT_ENABLE
                if (!blink_data || cur_indi) indiPrintNum(mainSettings.dotBright[TIME_NIGHT] / 10, 0, 2, 0); //вывод яркости ночь
                if (!blink_data || !cur_indi) indiPrintNum(mainSettings.dotBright[TIME_DAY] / 10, 2, 2, 0); //вывод яркости день
#else
                if (!blink_data) indiPrintNum((boolean)mainSettings.dotBright[TIME_NIGHT], 3); //вывод яркости ночь
#endif
                break;
              case SET_TEMP_SENS:
                if (!blink_data) {
                  if (sens.err) indiPrintNum(0, 0); //вывод ошибки
                  else indiPrintNum(sens.temp + mainSettings.tempCorrect, 0, 3); //вывод температуры
                }
                indiPrintNum(sens.type, 3); //вывод сенсора температуры
                break;
              case SET_AUTO_TEMP:
                if (!blink_data || cur_indi) indiPrintNum(mainSettings.autoTempTime, 0, 2, 0); //вывод времени автопоказа температуры
                if (!blink_data || !cur_indi) indiPrintNum(mainSettings.autoTempFlip, 2, 2, 0); //вывод анимации автопоказа температуры
                break;
              case SET_BURN_MODE:
#if LAMP_NUM > 4
                if (!blink_data || cur_indi) indiPrintNum(mainSettings.burnMode, 0, 2, 0); //вывод анимации антиотравления индикаторов
                if (!blink_data || !cur_indi) indiPrintNum(mainSettings.secsMode, 2, 2, 0); //вывод анимации антиотравления индикаторов
#else
                if (!blink_data) indiPrintNum(mainSettings.burnMode, 0, 2, 0); //вывод анимации антиотравления индикаторов
#endif
                break;
              case SET_SLEEP_TIME:
                if (!blink_data || cur_indi) indiPrintNum(mainSettings.timeSleep[TIME_NIGHT], 0, 2, 0); //вывод времени ночь
                if (!blink_data || !cur_indi) indiPrintNum(mainSettings.timeSleep[TIME_DAY], 2, 2, 0); //вывод времени день
                break;
            }
#if BACKL_TYPE == 3
            switch (cur_mode) {
              case SET_TIME_FORMAT: setBacklHue(2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
#if (NEON_DOT == 3) && DOTS_PORT_ENABLE
              case SET_DOT_BRIGHT:
#endif
#if PLAYER_TYPE
              case SET_BTN_SOUND:
#endif
              case SET_GLITCH: setBacklHue(3, 1, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
              case SET_TEMP_SENS: setBacklHue(0, 3, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
#if LAMP_NUM < 6
              case SET_BURN_MODE: setBacklHue(0, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
#endif
              default: setBacklHue(cur_indi * 2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
            }
#endif
            blink_data = !blink_data; //мигание сигментами
          }
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
#if PLAYER_TYPE
                switch (cur_indi) {
                  case 0: if (mainSettings.volumeSound > PLAYER_MIN_VOL) mainSettings.volumeSound--; playerSetVolNow(mainSettings.volumeSound); break; //установили громкость
                  case 1: mainSettings.knockSound = 0; break; //озвучка
                }
#else
                mainSettings.knockSound = 0; //звук кнопок
#endif
                break;
              case SET_HOUR_TIME: //время звука смены часа
                switch (cur_indi) {
                  case 0: if (mainSettings.timeHour[TIME_NIGHT] > 0) mainSettings.timeHour[TIME_NIGHT]--; else mainSettings.timeHour[TIME_NIGHT] = 23; break;
                  case 1: if (mainSettings.timeHour[TIME_DAY] > 0) mainSettings.timeHour[TIME_DAY]--; else mainSettings.timeHour[TIME_DAY] = 23; break;
                }
                break;
              case SET_BRIGHT_TIME: //время смены подсветки
                switch (cur_indi) {
                  case 0: if (mainSettings.timeBright[TIME_NIGHT] > 0) mainSettings.timeBright[TIME_NIGHT]--; else mainSettings.timeBright[TIME_NIGHT] = 23; break;
                  case 1: if (mainSettings.timeBright[TIME_DAY] > 0) mainSettings.timeBright[TIME_DAY]--; else mainSettings.timeBright[TIME_DAY] = 23; break;
                }
                break;
              case SET_INDI_BRIGHT: //яркость индикаторов
                switch (cur_indi) {
                  case 0: if (mainSettings.indiBright[TIME_NIGHT] > INDI_MIN_BRIGHT) mainSettings.indiBright[TIME_NIGHT]--; break;
                  case 1: if (mainSettings.indiBright[TIME_DAY] > INDI_MIN_BRIGHT) mainSettings.indiBright[TIME_DAY]--; break;
                }
                indiSetBright(mainSettings.indiBright[cur_indi]); //установка общей яркости индикаторо
                break;
#if BACKL_TYPE
              case SET_BACKL_BRIGHT: //яркость подсветки
                switch (cur_indi) {
                  case 0: if (mainSettings.backlBright[TIME_NIGHT] > 0) mainSettings.backlBright[TIME_NIGHT] -= 10; break;
                  case 1: if (mainSettings.backlBright[TIME_DAY] > 10) mainSettings.backlBright[TIME_DAY] -= 10; break;
                }
#if BACKL_TYPE == 3
                setLedBright(mainSettings.backlBright[cur_indi]); //устанавливаем максимальную яркость
#else
                backlSetBright(mainSettings.backlBright[cur_indi]); //если посветка статичная, устанавливаем яркость
#endif
                break;
#endif
              case SET_DOT_BRIGHT: //яркость точек
#if (NEON_DOT != 3) || !DOTS_PORT_ENABLE
                switch (cur_indi) {
                  case 0: if (mainSettings.dotBright[TIME_NIGHT] > 0) mainSettings.dotBright[TIME_NIGHT] -= 10; break;
                  case 1: if (mainSettings.dotBright[TIME_DAY] > 10) mainSettings.dotBright[TIME_DAY] -= 10; break;
                }
                dotSetBright(mainSettings.dotBright[cur_indi]); //включаем точки
#else
                mainSettings.dotBright[TIME_NIGHT] = 0;
#endif
                break;
              case SET_TEMP_SENS: //настройка коррекции температуры
                if (mainSettings.tempCorrect > -127) mainSettings.tempCorrect--; else mainSettings.tempCorrect = 127;
                break;
              case SET_AUTO_TEMP: //автопоказ температуры
                switch (cur_indi) {
                  case 0:
                    if (mainSettings.autoTempTime > 0) mainSettings.autoTempTime--; else mainSettings.autoTempTime = 15;
                    _timer_sec[TMR_TEMP] = getPhaseTime(mainSettings.autoTempTime, AUTO_TEMP_PHASE); //установка таймера показа температуры
                    break;
                  case 1:
                    if (mainSettings.autoTempFlip > 0) mainSettings.autoTempFlip--; else mainSettings.autoTempFlip = (FLIP_EFFECT_NUM + 1); //устанавливаем анимацию автопоказа температуры
                    if (mainSettings.autoTempFlip > 1) animDemo = 2; //установили флаг демонстрации анимации
                    break;
                }
                break;
              case SET_BURN_MODE: //анимация антиотравления индикаторов
#if LAMP_NUM > 4
                switch (cur_indi) {
                  case 0:
                    if (mainSettings.burnMode) mainSettings.burnMode--; else mainSettings.burnMode = (BURN_EFFECT_NUM - 1);
                    animDemo = 2; //установили флаг демонстрации анимации
                    break;
                  case 1:
                    if (mainSettings.secsMode) mainSettings.secsMode--; else mainSettings.secsMode = (SECS_EFFECT_NUM - 1);
                    if (mainSettings.secsMode) animDemo = 2; //установили флаг демонстрации анимации
                    break;
                }
#else
                if (mainSettings.burnMode) mainSettings.burnMode--; else mainSettings.burnMode = (BURN_EFFECT_NUM - 1);
                burnIndi(mainSettings.burnMode, BURN_DEMO); //демонстрация антиотравления индикаторов
                dotSetBright(dot.menuBright); //включаем точки
#endif
                break;
              case SET_SLEEP_TIME: //время ухода в сон
                switch (cur_indi) {
                  case 0: if (mainSettings.timeSleep[TIME_NIGHT] > 0) mainSettings.timeSleep[TIME_NIGHT] -= 5; else mainSettings.timeSleep[TIME_NIGHT] = 30; break;
                  case 1: if (mainSettings.timeSleep[TIME_DAY] > 0) mainSettings.timeSleep[TIME_DAY] -= 15; else mainSettings.timeSleep[TIME_DAY] = 90; break;
                }
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
#if PLAYER_TYPE
                switch (cur_indi) {
                  case 0: if (mainSettings.volumeSound < PLAYER_MAX_VOL) mainSettings.volumeSound++; playerSetVolNow(mainSettings.volumeSound); break; //установили громкость
                  case 1: mainSettings.knockSound = 1; break; //озвучка
                }
#else
                mainSettings.knockSound = 1; //звук кнопок
#endif
                break;
              case SET_HOUR_TIME: //время звука смены часа
                switch (cur_indi) {
                  case 0: if (mainSettings.timeHour[TIME_NIGHT] < 23) mainSettings.timeHour[TIME_NIGHT]++; else mainSettings.timeHour[TIME_NIGHT] = 0; break;
                  case 1: if (mainSettings.timeHour[TIME_DAY] < 23) mainSettings.timeHour[TIME_DAY]++; else mainSettings.timeHour[TIME_DAY] = 0; break;
                }
                break;
              case SET_BRIGHT_TIME: //время смены подсветки
                switch (cur_indi) {
                  case 0: if (mainSettings.timeBright[TIME_NIGHT] < 23) mainSettings.timeBright[TIME_NIGHT]++; else mainSettings.timeBright[TIME_NIGHT] = 0; break;
                  case 1: if (mainSettings.timeBright[TIME_DAY] < 23) mainSettings.timeBright[TIME_DAY]++; else mainSettings.timeBright[TIME_DAY] = 0; break;
                }
                break;
              case SET_INDI_BRIGHT: //яркость индикаторов
                switch (cur_indi) {
                  case 0: if (mainSettings.indiBright[TIME_NIGHT] < 30) mainSettings.indiBright[TIME_NIGHT]++; break;
                  case 1: if (mainSettings.indiBright[TIME_DAY] < 30) mainSettings.indiBright[TIME_DAY]++; break;
                }
                indiSetBright(mainSettings.indiBright[cur_indi]); //установка общей яркости индикаторо
                break;
#if BACKL_TYPE
              case SET_BACKL_BRIGHT: //яркость подсветки
                switch (cur_indi) {
                  case 0: if (mainSettings.backlBright[TIME_NIGHT] < 250) mainSettings.backlBright[TIME_NIGHT] += 10; break;
                  case 1: if (mainSettings.backlBright[TIME_DAY] < 250) mainSettings.backlBright[TIME_DAY] += 10; break;
                }
#if BACKL_TYPE == 3
                setLedBright(mainSettings.backlBright[cur_indi]); //устанавливаем максимальную яркость
#else
                backlSetBright(mainSettings.backlBright[cur_indi]); //если посветка статичная, устанавливаем яркость
#endif
                break;
#endif
              case SET_DOT_BRIGHT: //яркость точек
#if (NEON_DOT != 3) || !DOTS_PORT_ENABLE
                switch (cur_indi) {
                  case 0: if (mainSettings.dotBright[TIME_NIGHT] < 250) mainSettings.dotBright[TIME_NIGHT] += 10; break;
                  case 1: if (mainSettings.dotBright[TIME_DAY] < 250) mainSettings.dotBright[TIME_DAY] += 10; break;
                }
                dotSetBright(mainSettings.dotBright[cur_indi]); //включаем точки
#else
                mainSettings.dotBright[TIME_NIGHT] = 1;
#endif
                break;
              case SET_TEMP_SENS: //настройка коррекции температуры
                if (mainSettings.tempCorrect < 127) mainSettings.tempCorrect++; else mainSettings.tempCorrect = -127;
                break;
              case SET_AUTO_TEMP: //автопоказ температуры
                switch (cur_indi) {
                  case 0:
                    if (mainSettings.autoTempTime < 15) mainSettings.autoTempTime++; else mainSettings.autoTempTime = 0;
                    _timer_sec[TMR_TEMP] = getPhaseTime(mainSettings.autoTempTime, AUTO_TEMP_PHASE); //установка таймера показа температуры
                    break;
                  case 1:
                    if (mainSettings.autoTempFlip < (FLIP_EFFECT_NUM + 1)) mainSettings.autoTempFlip++; else mainSettings.autoTempFlip = 0; //устанавливаем анимацию автопоказа температуры
                    if (mainSettings.autoTempFlip > 1) animDemo = 2; //установили флаг демонстрации анимации
                    break;
                }
                break;
              case SET_BURN_MODE: //анимация антиотравления индикаторов
#if LAMP_NUM > 4
                switch (cur_indi) {
                  case 0:
                    if (mainSettings.burnMode < (BURN_EFFECT_NUM - 1)) mainSettings.burnMode++; else mainSettings.burnMode = 0;
                    animDemo = 2; //установили флаг демонстрации анимации
                    break;
                  case 1:
                    if (mainSettings.secsMode < (SECS_EFFECT_NUM - 1)) {
                      mainSettings.secsMode++;
                      animDemo = 2; //установили флаг демонстрации анимации
                    }
                    else mainSettings.secsMode = 0;
                    break;
                }
#else
                if (mainSettings.burnMode < (BURN_EFFECT_NUM - 1)) mainSettings.burnMode++; else mainSettings.burnMode = 0;
                burnIndi(mainSettings.burnMode, BURN_DEMO); //демонстрация антиотравления индикаторов
                dotSetBright(dot.menuBright); //включаем точки
#endif
                break;
              case SET_SLEEP_TIME: //время ухода в сон
                switch (cur_indi) {
                  case 0: if (mainSettings.timeSleep[TIME_NIGHT] < 30) mainSettings.timeSleep[TIME_NIGHT] += 5; else mainSettings.timeSleep[TIME_NIGHT] = 0; break;
                  case 1: if (mainSettings.timeSleep[TIME_DAY] < 90) mainSettings.timeSleep[TIME_DAY] += 15; else mainSettings.timeSleep[TIME_DAY] = 0; break;
                }
                break;
            }
            break;
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        set = !set;
        if (set) {
          changeBrightDisable(CHANGE_DISABLE); //запретить смену яркости
          dotSetBright((cur_mode != SET_DOT_BRIGHT) ? dot.menuBright : mainSettings.dotBright[TIME_NIGHT]); //включаем точки
          switch (cur_mode) {
            case SET_INDI_BRIGHT: indiSetBright(mainSettings.indiBright[TIME_NIGHT]); break; //установка общей яркости индикаторов
            case SET_BACKL_BRIGHT: //яркость подсветки
#if BACKL_TYPE == 3
              setLedBright(mainSettings.backlBright[TIME_NIGHT]); //устанавливаем максимальную яркость
#elif BACKL_TYPE
              backlAnimDisable(); //запретили эффекты подсветки
              backlSetBright(mainSettings.backlBright[TIME_NIGHT]); //если посветка статичная, устанавливаем яркость
#else
              set = 0; //заблокировали пункт меню
              dotSetBright(0); //выключаем точки
#endif
              break;
#if (NEON_DOT != 3) && DOTS_PORT_ENABLE
            case SET_TEMP_SENS: //настройка коррекции температуры
#if DOTS_TYPE == 1
              indiSetDotR(1); //включаем разделительную точку
#else
              indiSetDotL(2); //включаем разделительную точку
#endif
              break;
#endif
          }
        }
        else {
#if BACKL_TYPE == 3
          setLedBright(backl.menuBright); //устанавливаем максимальную яркость
#elif BACKL_TYPE
          backlAnimEnable(); //разрешили эффекты подсветки
#endif
          changeBrightEnable(); //разрешить смену яркости
          changeBright(); //установка яркости от времени суток
          dotSetBright(0); //выключаем точки
#if (NEON_DOT != 3) && DOTS_PORT_ENABLE
          indiClrDots(); //выключаем разделительные точки
#endif
        }
        cur_indi = 0;
        _timer_ms[TMR_MS] = time_out = animDemo = blink_data = 0; //сбрасываем флаги
        break;

      case LEFT_KEY_HOLD: //удержание левой кнопки
        if (set) {
          cur_indi = TIME_NIGHT;
          switch (cur_mode) {
            case SET_INDI_BRIGHT: indiSetBright(mainSettings.indiBright[TIME_NIGHT]); break; //установка общей яркости индикаторов
            case SET_BACKL_BRIGHT: //яркость подсветки
#if BACKL_TYPE == 3
              setLedBright(mainSettings.backlBright[TIME_NIGHT]); //устанавливаем максимальную яркость
#elif BACKL_TYPE
              backlSetBright(mainSettings.backlBright[TIME_NIGHT]); //если посветка статичная, устанавливаем яркость
#endif
              break;
            case SET_DOT_BRIGHT: dotSetBright(mainSettings.dotBright[TIME_NIGHT]); break;//яркость точек
            case SET_TEMP_SENS: mainSettings.tempCorrect = 0; break; //сброс коррекции температуры
          }
        }
        _timer_ms[TMR_MS] = time_out = animDemo = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_HOLD: //удержание правой кнопки
        if (set) {
          cur_indi = TIME_DAY;
          switch (cur_mode) {
            case SET_INDI_BRIGHT: indiSetBright(mainSettings.indiBright[TIME_DAY]); break; //установка общей яркости индикаторов
            case SET_BACKL_BRIGHT: //яркость подсветки
#if BACKL_TYPE == 3
              setLedBright(mainSettings.backlBright[TIME_DAY]); //устанавливаем максимальную яркость
#elif BACKL_TYPE
              backlSetBright(mainSettings.backlBright[TIME_DAY]); //если посветка статичная, устанавливаем яркость
#endif
              break;
            case SET_DOT_BRIGHT: dotSetBright(mainSettings.dotBright[TIME_DAY]); break;//яркость точек
            case SET_TEMP_SENS: mainSettings.tempCorrect = 0; break; //сброс коррекции температуры
          }
        }
        _timer_ms[TMR_MS] = time_out = animDemo = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        return MAIN_PROGRAM;
    }
  }
  return INIT_PROGRAM;
}
//----------------------------Воспроизвести температуру--------------------------------------
void speakTemp(void) //воспроизвести температуру
{
  uint16_t _ceil = (sens.temp + mainSettings.tempCorrect) / 10;
  uint16_t _dec = (sens.temp + mainSettings.tempCorrect) % 10;

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
//------------------------Воспроизвести целую температуру------------------------------------
void speakTempCeil(void) //воспроизвести целую температуру
{
  uint16_t _ceil = (sens.temp + mainSettings.tempCorrect) / 10;

  playerSetTrack(PLAYER_TEMP_SOUND, PLAYER_GENERAL_FOLDER);
  playerSpeakNumber(_ceil);
  playerSetTrack(PLAYER_SENS_TEMP_START + playerGetSpeak(_ceil), PLAYER_END_NUMBERS_FOLDER);
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
  if (!_timer_ms[TMR_SENS]) { //если таймаут нового запроса вышел
    updateTemp(); //обновить показания температуры
    _timer_ms[TMR_SENS] = TEMP_UPDATE_TIME; //установили таймаут
  }

  for (uint8_t mode = 0; mode < AUTO_TEMP_SHOW_TYPE; mode++) {
#if DOTS_PORT_ENABLE
    indiClrDots(); //выключаем разделительные точки
#endif
#if (NEON_DOT != 3) || !DOTS_PORT_ENABLE
    dotSetBright(0); //выключаем секундные точки
#endif
    animClearBuff(); //очистка буфера анимации
    switch (mode) {
      case 0:
#if AUTO_TEMP_SHOW_HUM && (LAMP_NUM > 4) && (AUTO_TEMP_SHOW_TYPE > 1) //режим отображения температуры и влажности
      case 1:
        mode = 1; //установили режим
        animPrintNum(sens.temp + mainSettings.tempCorrect, 0, 3, ' '); //вывод температуры
        if (sens.hum) animPrintNum(sens.hum, 4, 2, ' '); //вывод влажности
        animIndi((mainSettings.autoTempFlip) ? mainSettings.autoTempFlip : fastSettings.flipMode, FLIP_NORMAL); //анимация цифр
#if DOTS_PORT_ENABLE
#if DOTS_TYPE == 1
        indiSetDotR(1); //включаем разделительную точку
#else
        indiSetDotL(2); //включаем разделительную точку
#endif
#elif NEON_DOT == 2
        neonDotSetBright(dot.menuBright); //установка яркости неоновых точек
        neonDotSet(DOT_LEFT); //установка разделительной точки
#else
        dotSetBright(dot.menuBright); //включаем точки
#endif
#if (BACKL_TYPE == 3) && AUTO_TEMP_BACKL_TYPE
        if (mode) { //если режим отображения температуры и влажности
          setBacklHue(4, 2, SHOW_TEMP_COLOR_H, SHOW_TEMP_COLOR_T);  //установили цвет температуры и влажности
          setLedHue(3, SHOW_TEMP_COLOR_P, WHITE_ON); //установили цвет пустого сегмента
        }
        else setLedHue(SHOW_TEMP_COLOR_T, WHITE_ON); //установили цвет температуры
        backlAnimDisable(); //запретили эффекты подсветки
#if AUTO_TEMP_BACKL_TYPE == 1
        changeBrightDisable(CHANGE_STATIC_BACKL); //разрешить смену яркости статичной подсветки
        setLedBright((fastSettings.backlMode & 0x7F) ? backl.maxBright : 0); //установили яркость в зависимости от режима подсветки
#else
        setLedBright(backl.menuBright); //установили максимальную яркость
#endif
#endif
#else //иначе режим отображения температуры
        animPrintNum(sens.temp + mainSettings.tempCorrect, 0, 3, ' '); //вывод температуры
        animIndi((mainSettings.autoTempFlip) ? mainSettings.autoTempFlip : fastSettings.flipMode, FLIP_NORMAL); //анимация цифр
#if DOTS_PORT_ENABLE
#if DOTS_TYPE == 1
        indiSetDotR(1); //включаем разделительную точку
#else
        indiSetDotL(2); //включаем разделительную точку
#endif
#elif NEON_DOT == 2
        neonDotSetBright(dot.menuBright); //установка яркости неоновых точек
        neonDotSet(DOT_LEFT); //установка разделительной точки
#else
        dotSetBright(dot.menuBright); //включаем точки
#endif
#if (BACKL_TYPE == 3) && AUTO_TEMP_BACKL_TYPE
        setLedHue(SHOW_TEMP_COLOR_T, WHITE_ON); //установили цвет температуры
        backlAnimDisable(); //запретили эффекты подсветки
#if AUTO_TEMP_BACKL_TYPE == 1
        changeBrightDisable(CHANGE_STATIC_BACKL); //разрешить смену яркости статичной подсветки
        setLedBright((fastSettings.backlMode & 0x7F) ? backl.maxBright : 0); //установили яркость в зависимости от режима подсветки
#else
        setLedBright(backl.menuBright); //установили максимальную яркость
#endif
#endif
        break;
      case 1:
        if (!sens.hum) continue; //возвращаемся назад
#if (BACKL_TYPE == 3) && AUTO_TEMP_BACKL_TYPE
        setLedHue(SHOW_TEMP_COLOR_H, WHITE_ON); //установили цвет влажности
#endif
        animPrintNum(sens.hum, 0, 4, ' '); //вывод влажности
        animIndi((mainSettings.autoTempFlip) ? mainSettings.autoTempFlip : fastSettings.flipMode, FLIP_NORMAL); //анимация цифр
#endif
        break;
      case 2:
        if (!sens.press) continue; //возвращаемся назад
#if (BACKL_TYPE == 3) && AUTO_TEMP_BACKL_TYPE
        setLedHue(SHOW_TEMP_COLOR_P, WHITE_ON); //установили цвет давления
#endif
        animPrintNum(sens.press, 0, 4, ' '); //вывод давления
        animIndi((mainSettings.autoTempFlip) ? mainSettings.autoTempFlip : fastSettings.flipMode, FLIP_NORMAL); //анимация цифр
        break;
    }

    _timer_ms[TMR_MS] = AUTO_TEMP_PAUSE_TIME; //устанавливаем таймер
    while (_timer_ms[TMR_MS]) { //если таймер истек
      dataUpdate(); //обработка данных
      if (buttonState()) return; //возврат если нажата кнопка
    }
  }
  animShow = (mainSettings.autoTempFlip) ? (ANIM_OTHER + mainSettings.autoTempFlip) : ANIM_MAIN; //установили флаг анимации
}
//--------------------------------Показать температуру----------------------------------------
uint8_t showTemp(void) //показать температуру
{
  uint8_t mode = 0; //текущий режим

  if (!_timer_ms[TMR_SENS]) { //если таймаут нового запроса вышел
    updateTemp(); //обновить показания температуры
    _timer_ms[TMR_SENS] = TEMP_UPDATE_TIME; //установили таймаут
  }

#if (BACKL_TYPE == 3) && SHOW_TEMP_BACKL_TYPE
  backlAnimDisable(); //запретили эффекты подсветки
#if SHOW_TEMP_BACKL_TYPE == 1
  changeBrightDisable(CHANGE_STATIC_BACKL); //разрешить смену яркости статичной подсветки
  setLedBright((fastSettings.backlMode & 0x7F) ? backl.maxBright : 0); //установили яркость в зависимости от режима подсветки
#else
  setLedBright(backl.menuBright); //установили максимальную яркость
#endif
#endif

#if DOTS_PORT_ENABLE
#if DOTS_TYPE == 1
  indiSetDotR(1); //включаем разделительную точку
#else
  indiSetDotL(2); //включаем разделительную точку
#endif
#elif NEON_DOT == 2
  neonDotSetBright(dot.menuBright); //установка яркости неоновых точек
  neonDotSet(DOT_LEFT); //установка разделительной точки
#else
  dotSetBright(dot.menuBright); //включаем точки
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
        case 0:
          indiPrintNum(sens.temp + mainSettings.tempCorrect, 0, 3, ' ');
#if (BACKL_TYPE == 3) && SHOW_TEMP_BACKL_TYPE
          setLedHue(SHOW_TEMP_COLOR_T, WHITE_ON); //установили цвет температуры
#endif
          break;
        case 1:
          indiPrintNum(sens.hum, 0, 4, ' ');
#if (BACKL_TYPE == 3) && SHOW_TEMP_BACKL_TYPE
          setLedHue(SHOW_TEMP_COLOR_H, WHITE_ON); //установили цвет влажности
#endif
          break;
        case 2:
          indiPrintNum(sens.press, 0, 4, ' ');
#if (BACKL_TYPE == 3) && SHOW_TEMP_BACKL_TYPE
          setLedHue(SHOW_TEMP_COLOR_P, WHITE_ON); //установили цвет давления
#endif
          break;
      }
    }

    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        if (++mode > 2) mode = 0;
        switch (mode) {
          case 1:
            if (!sens.hum) {
              if (!sens.press) mode = 0;
              else mode = 2;
            }
            break;
          case 2: if (!sens.press) mode = 0; break;
        }
        if (!mode) { //если режим отображения температуры
#if DOTS_PORT_ENABLE
#if DOTS_TYPE == 1
          indiSetDotR(1); //включаем разделительную точку
#else
          indiSetDotL(2); //включаем разделительную точку
#endif
#elif NEON_DOT == 2
          neonDotSetBright(dot.menuBright); //установка яркости неоновых точек
          neonDotSet(DOT_LEFT); //установка разделительной точки
#else
          dotSetBright(dot.menuBright); //включаем точки
#endif
        }
        else { //иначе давление или влажность
#if DOTS_PORT_ENABLE
          indiClrDots(); //выключаем разделительные точки
#else
          dotSetBright(0); //выключаем точки

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
        return MAIN_PROGRAM; //выходим
    }
  }
  animShow = ANIM_MAIN; //установили флаг анимации
  return MAIN_PROGRAM; //выходим
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
uint8_t showDate(void) //показать дату
{
#if (SHOW_DATE_TYPE < 2) || (LAMP_NUM < 6)
  uint8_t mode = 0; //текущий режим
#endif

#if DOTS_PORT_ENABLE
#if SHOW_DATE_TYPE > 1
#if DOTS_TYPE == 2
  indiSetDotL(2); //включаем разделительную точку
  indiSetDotR(3); //включаем разделительную точку
#elif DOTS_NUM > 4
#if DOTS_TYPE == 1
  indiSetDotR(1); //включаем разделительную точку
  indiSetDotR(3); //включаем разделительную точку
#else
  indiSetDotL(2); //включаем разделительную точку
  indiSetDotL(4); //включаем разделительную точку
#endif
#elif NEON_DOT != 3
  dotSetBright(dot.menuBright); //включаем точки
#endif
#else
#if DOTS_TYPE == 1
  indiSetDotR(1); //включаем разделительную точку
#else
  indiSetDotL(2); //включаем разделительную точку
#endif
#endif
#elif NEON_DOT == 2
#if (SHOW_DATE_TYPE > 1) && (LAMP_NUM > 4)
  dotSetBright(dot.menuBright); //включаем точки
#else
  neonDotSetBright(dot.menuBright); //установка яркости неоновых точек
  neonDotSet(DOT_LEFT); //установка разделительной точки
#endif
#else
  dotSetBright(dot.menuBright); //включаем точки
#endif

#if (BACKL_TYPE == 3) && SHOW_DATE_BACKL_TYPE
  backlAnimDisable(); //запретили эффекты подсветки
#if SHOW_DATE_BACKL_TYPE == 1
  changeBrightDisable(CHANGE_STATIC_BACKL); //разрешить смену яркости статичной подсветки
  setLedBright((fastSettings.backlMode & 0x7F) ? backl.maxBright : 0); //установили яркость в зависимости от режима подсветки
#else
  setLedBright(backl.menuBright); //установили максимальную яркость
#endif
#endif

#if PLAYER_TYPE
  if (mainSettings.knockSound) speakTime(); //воспроизвести время
#endif

  for (_timer_ms[TMR_MS] = SHOW_DATE_TIME; _timer_ms[TMR_MS];) {
    dataUpdate(); //обработка данных

    if (!secUpd) {
      secUpd = 1; //сбрасываем флаг
      indiClr(); //очистка индикаторов
#if (SHOW_DATE_TYPE > 1) && (LAMP_NUM > 4)
#if SHOW_DATE_TYPE == 3
      indiPrintNum(RTC.MM, 0, 2, 0); //вывод месяца
      indiPrintNum(RTC.DD, 2, 2, 0); //вывод даты
#else
      indiPrintNum(RTC.DD, 0, 2, 0); //вывод даты
      indiPrintNum(RTC.MM, 2, 2, 0); //вывод месяца
#endif
      indiPrintNum(RTC.YY - 2000, 4, 2, 0); //вывод года
#if (BACKL_TYPE == 3) && SHOW_DATE_BACKL_TYPE
      setBacklHue(0, 4, SHOW_DATE_BACKL_DM, SHOW_DATE_BACKL_YY);
#endif
#else
      indiPrintNum(mode + 1, 5); //режим
      switch (mode) {
        case 0:
#if SHOW_DATE_TYPE == 1
          indiPrintNum(RTC.MM, 0, 2, 0); //вывод месяца
          indiPrintNum(RTC.DD, 2, 2, 0); //вывод даты
#else
          indiPrintNum(RTC.DD, 0, 2, 0); //вывод даты
          indiPrintNum(RTC.MM, 2, 2, 0); //вывод месяца
#endif
#if (BACKL_TYPE == 3) && SHOW_DATE_BACKL_TYPE
          setBacklHue(0, 4, SHOW_DATE_BACKL_DM, SHOW_DATE_BACKL_NN);
#endif
          break;
        case 1:
          indiPrintNum(RTC.YY, 0); //вывод года
#if (BACKL_TYPE == 3) && SHOW_DATE_BACKL_TYPE
          setBacklHue(0, 4, SHOW_DATE_BACKL_YY, SHOW_DATE_BACKL_NN);
#endif
          break;
      }
#endif
    }

    switch (buttonState()) {
      case RIGHT_KEY_PRESS: //клик правой кнопкой
#if (SHOW_DATE_TYPE < 2) || (LAMP_NUM < 6)
        if (++mode > 1) mode = 0;
        switch (mode) {
          case 0: //дата
#if DOTS_PORT_ENABLE
#if DOTS_TYPE == 1
            indiSetDotR(1); //включаем разделительную точку
#else
            indiSetDotL(2); //включаем разделительную точку
#endif
#elif NEON_DOT == 2
            neonDotSetBright(dot.menuBright); //установка яркости неоновых точек
            neonDotSet(DOT_LEFT); //установка разделительной точки
#else
            dotSetBright(dot.menuBright); //включаем точки
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
#endif

      case LEFT_KEY_PRESS: //клик левой кнопкой
      case SET_KEY_PRESS: //клик средней кнопкой
        return MAIN_PROGRAM; //выходим
    }
  }
  animShow = ANIM_MAIN; //установили флаг анимации
  return MAIN_PROGRAM; //выходим
}
//----------------------------------Переключение быстрых настроек-----------------------------------
uint8_t fastSetSwitch(void) //переключение быстрых настроек
{
  uint8_t anim = 0; //анимация переключения
#if BACKL_TYPE
  uint8_t mode = FAST_BACKL_MODE; //режим быстрой настройки
#else
  uint8_t mode = FAST_FLIP_MODE; //режим быстрой настройки
#endif

#if PLAYER_TYPE
#if BACKL_TYPE
  if (mainSettings.knockSound) playerSetTrackNow(PLAYER_FAST_MENU_START + FAST_BACKL_MODE, PLAYER_MENU_FOLDER);
#else
  if (mainSettings.knockSound) playerSetTrackNow(PLAYER_FAST_MENU_START + FAST_FLIP_MODE, PLAYER_MENU_FOLDER);
#endif
#endif

  for (_timer_ms[TMR_MS] = FAST_BACKL_TIME; _timer_ms[TMR_MS];) {
    dataUpdate(); //обработка данных

    if (anim < 4) {
      if (!_timer_ms[TMR_ANIM]) { //если таймер истек
        _timer_ms[TMR_ANIM] = FAST_ANIM_TIME; //устанавливаем таймер

        indiClr(); //очистка индикаторов
        indiPrintNum(mode + 1, 5); //режим
        switch (mode) {
#if BACKL_TYPE
          case FAST_BACKL_MODE: indiPrintNum(fastSettings.backlMode, anim - 1, 2); break; //вывод режима подсветки
#endif
          case FAST_FLIP_MODE: indiPrintNum(fastSettings.flipMode, anim - 1, 2); break; //вывод режима анимации
          case FAST_DOT_MODE: indiPrintNum(fastSettings.dotMode, anim); break; //вывод режима точек
#if BACKL_TYPE
          case FAST_BACKL_COLOR: indiPrintNum((fastSettings.backlColor == 255) ? 0 : ((fastSettings.backlColor / 10) + 1), anim - 1, 2); break; //вывод цвета подсветки
#endif
        }
        anim++; //сдвигаем анимацию
      }
    }

    switch (buttonState()) {
      case SET_KEY_PRESS: //клик средней кнопкой
#if BACKL_TYPE
        if (mode != FAST_BACKL_MODE) {
#if PLAYER_TYPE
          if (mainSettings.knockSound) playerSetTrackNow(PLAYER_FAST_MENU_START + FAST_BACKL_MODE, PLAYER_MENU_FOLDER);
#endif
          mode = FAST_BACKL_MODE; //демострация текущего режима работы
        }
        else {
#if BACKL_TYPE == 3
          if (++fastSettings.backlMode < BACKL_EFFECT_NUM) {
            switch (fastSettings.backlMode) {
              case BACKL_STATIC:
                setLedBright(backl.maxBright); //устанавливаем максимальную яркость
                setLedHue(fastSettings.backlColor, WHITE_ON); //устанавливаем статичный цвет
                break;
              case BACKL_PULS:
                setLedBright(backl.maxBright ? backl.minBright : 0); //устанавливаем минимальную яркость
                setLedHue(fastSettings.backlColor, WHITE_ON); //устанавливаем статичный цвет
                break;
              case BACKL_RUNNING_FIRE:
                setLedBright(0); //устанавливаем минимальную яркость
                setLedHue(fastSettings.backlColor, WHITE_ON); //устанавливаем статичный цвет
                break;
              case BACKL_WAVE:
                setLedBright(backl.maxBright ? backl.minBright : 0); //устанавливаем минимальную яркость
                setLedHue(fastSettings.backlColor, WHITE_ON); //устанавливаем статичный цвет
                break;
              case BACKL_SMOOTH_COLOR_CHANGE:
                setLedBright(backl.maxBright); //устанавливаем максимальную яркость
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
            case BACKL_STATIC: backlSetBright(backl.maxBright); break; //включаем подсветку
            case BACKL_PULS: backlSetBright(backl.maxBright ? backl.minBright : 0); break; //выключаем подсветку
          }
#endif
        }
        _timer_ms[TMR_MS] = FAST_BACKL_TIME;
        anim = 0;
#else
        _timer_ms[TMR_MS] = 0;
#endif
        break;
#if BACKL_TYPE == 3
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
#if BACKL_TYPE == 3
        if (mode == FAST_BACKL_COLOR) {
          if (fastSettings.backlColor < 250) fastSettings.backlColor += 10; else if (fastSettings.backlColor == 250) fastSettings.backlColor = 255; else fastSettings.backlColor = 0;
          setLedHue(fastSettings.backlColor, WHITE_ON); //устанавливаем статичный цвет
          _timer_ms[TMR_MS] = FAST_BACKL_TIME;
        }
        else {
#endif
          if (mode != FAST_FLIP_MODE) {
#if PLAYER_TYPE
            if (mainSettings.knockSound) playerSetTrackNow(PLAYER_FAST_MENU_START + FAST_FLIP_MODE, PLAYER_MENU_FOLDER);
#endif
            mode = FAST_FLIP_MODE; //демострация текущего режима работы
          }
          else if (++fastSettings.flipMode > (FLIP_EFFECT_NUM + 1)) fastSettings.flipMode = 0;
          _timer_ms[TMR_MS] = FAST_FLIP_TIME;
#if BACKL_TYPE == 3
        }
#endif
        anim = 0;
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
#if BACKL_TYPE == 3
        if (mode == FAST_BACKL_COLOR) {
          if (fastSettings.backlColor == 255) fastSettings.backlColor = 250; else if (fastSettings.backlColor > 0) fastSettings.backlColor -= 10; else fastSettings.backlColor = 255;
          setLedHue(fastSettings.backlColor, WHITE_ON); //устанавливаем статичный цвет
          _timer_ms[TMR_MS] = FAST_BACKL_TIME;
        }
        else {
#endif
          if (mode != FAST_DOT_MODE) {
#if PLAYER_TYPE
            if (mainSettings.knockSound) playerSetTrackNow(PLAYER_FAST_MENU_START + FAST_DOT_MODE, PLAYER_MENU_FOLDER);
#endif
            mode = FAST_DOT_MODE; //демострация текущего режима работы
          }
          else if (++fastSettings.dotMode > (DOT_EFFECT_NUM - 3)) fastSettings.dotMode = 0;
          _timer_ms[TMR_MS] = FAST_DOT_TIME;
#if BACKL_TYPE == 3
        }
#endif
        anim = 0;
        break;
    }
  }
  if (mode == 1) animIndi(fastSettings.flipMode, FLIP_DEMO); //демонстрация анимации цифр
  updateData((uint8_t*)&fastSettings, sizeof(fastSettings), EEPROM_BLOCK_SETTINGS_FAST, EEPROM_BLOCK_CRC_FAST); //записываем настройки яркости в память
  return MAIN_PROGRAM; //выходим
}
//-------------------------Включить питание радиоприемника------------------------------
void radioPowerOn(void) //включить питание радиоприемника
{
  setPowerRDA(RDA_ON); //включаем радио
  setVolumeRDA(radioSettings.volume); //устанавливаем громкость
  setFreqRDA(radioSettings.stationsFreq); //устанавливаем частоту
}
//------------------------Выключить питание радиоприемника------------------------------
void radioPowerOff(void) //выключить питание радиоприемника
{
  if (getPowerStatusRDA() == RDA_ON) { //если радио включено
    setPowerRDA(RDA_OFF); //выключаем радио
  }
}
//--------------------------Вернуть питание радиоприемника------------------------------
void radioPowerRet(void) //вернуть питание радиоприемника
{
  if (radio.powerState == RDA_ON) { //если радио было включено
    radioPowerOn(); //включить питание радиоприемника
  }
  else radioPowerOff(); // иначе выключить питание радиоприемника
}
//--------------------------Поиск радиостанции в памяти---------------------------------
void radioSearchStation(void) //поиск радиостанции в памяти
{
  for (uint8_t i = 0; i < RADIO_MAX_STATIONS; i++) { //ищем среди всех ячеек
    if (radioSettings.stationsSave[i] == radioSettings.stationsFreq) { //если частота совпадает с радиостанцией
      radioSettings.stationNum = i; //установили номер радиостанции
      return; //выходим
    }
  }
  radioSettings.stationNum |= 0x80; //установили номер ячейки за пределом видимости
}
//-----------------------Переключить радиостанцию в памяти------------------------------
void radioSwitchStation(boolean _drv) //переключить радиостанцию в памяти
{
  if (radioSettings.stationNum & 0x80) { //если установлен флаг ячейки
    radioSettings.stationNum &= 0x7F; //сбросили флаг
    radioSettings.stationsFreq = radioSettings.stationsSave[radioSettings.stationNum]; //прочитали частоту
    setFreqRDA(radioSettings.stationsFreq); //установили частоту
    return; //выходим
  }
  for (uint8_t i = 0; i < RADIO_MAX_STATIONS; i++) { //ищем среди всех ячеек
    if (_drv) { //ищем вперед
      if (radioSettings.stationNum < (RADIO_MAX_STATIONS - 1)) radioSettings.stationNum++; else radioSettings.stationNum = 0; //переключаем станцию
    }
    else { //ищем назад
      if (radioSettings.stationNum > 0) radioSettings.stationNum--; else radioSettings.stationNum = (RADIO_MAX_STATIONS - 1); //переключаем станцию
    }
    if (radioSettings.stationsSave[radioSettings.stationNum]) { //если в памяти записана частота
      radioSettings.stationsFreq = radioSettings.stationsSave[radioSettings.stationNum]; //прочитали частоту
      setFreqRDA(radioSettings.stationsFreq); //установили частоту
      return; //выходим
    }
  }
}
//------------------------Остановка автопоиска радиостанции-----------------------------
void radioSeekStop(void) //остановка автопоиска радиостанции
{
  if (radio.seekRun) { //если идет поиск
    radio.seekRun = 0; //выключили поиск
    stopSeekRDA(); //остановили поиск радио
    clrSeekCompleteStatusRDA(); //очищаем флаг окончания поиска
    setFreqRDA(radioSettings.stationsFreq); //устанавливаем частоту
    setMuteRDA(RDA_MUTE_OFF); //выключаем приглушение звука
    radioSearchStation(); //поиск радиостанции в памяти
  }
}
//-----------------------------Быстрые настройки радио-----------------------------------
#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
boolean radioFastSettings(uint8_t _state) //быстрые настройки радио
{
  if (radio.powerState) { //если радио включено
    if (mainTask != RADIO_PROGRAM) {
      mainTask = RADIO_PROGRAM; //подмена текущей программы
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
      backlAnimDisable(); //запретили эффекты подсветки
#if RADIO_BACKL_TYPE == 1
      changeBrightDisable(CHANGE_STATIC_BACKL); //разрешить смену яркости статичной подсветки
      setLedBright((fastSettings.backlMode & 0x7F) ? backl.maxBright : 0); //установили яркость в зависимости от режима подсветки
#else
      setLedBright(backl.menuBright); //установили максимальную яркость
#endif
#endif
    }

#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
    setBacklHue(((LAMP_NUM / 2) - 1), 2, RADIO_BACKL_COLOR_1, RADIO_BACKL_COLOR_2);
#endif

    dotSetBright(0); //выключаем точки
#if (NEON_DOT != 3) && DOTS_PORT_ENABLE
    indiClrDots(); //очистка разделителных точек
#endif

    while (1) {
      dataUpdate(); //обработка данных

      switch (_state) {
        case KEY_NULL: break;

        case VOL_UP_KEY_PRESS: //прибавить громкость
          if (radioSettings.volume < RADIO_MAX_VOL) setVolumeRDA(++radioSettings.volume); //прибавитиь громкость
          break;
        case VOL_DOWN_KEY_PRESS: //убавить громкость
          if (radioSettings.volume > RADIO_MIN_VOL) setVolumeRDA(--radioSettings.volume); //убавить громкость
          break;
        case STATION_UP_KEY_PRESS: //следующая станция
          radioSwitchStation(1); //переключить радиостанцию в памяти
          break;
        case STATION_DOWN_KEY_PRESS: //предыдущая станция
          radioSwitchStation(0); //переключить радиостанцию в памяти
          break;

        default: return 1; //выходим
      }

      switch (_state) {
        case VOL_UP_KEY_PRESS:
        case VOL_DOWN_KEY_PRESS:
          _timer_ms[TMR_MS] = RADIO_FAST_TIME; //устанавливаем таймер
          indiClr(); //очистка индикаторов
          indiPrintNum(radioSettings.volume, ((LAMP_NUM / 2) - 1), 2, 0); //вывод настройки
          break;
        case STATION_UP_KEY_PRESS:
        case STATION_DOWN_KEY_PRESS:
          _timer_ms[TMR_MS] = RADIO_FAST_TIME; //устанавливаем таймер
          indiClr(); //очистка индикаторов
          indiPrintNum(radioSettings.stationNum, ((LAMP_NUM / 2) - 1), 2, 0); //вывод настройки
          break;
      }

      _state = buttonState();

      if (!_timer_ms[TMR_MS]) return 1; //выходим
    }
  }
  return 0;
}
#endif
//------------------------------Меню настроек радио-------------------------------------
boolean radioMenuSettings(boolean mode) //меню настроек радио
{
  boolean _state = 0; //флаг бездействия
  uint8_t _station = radioSettings.stationNum & 0x7F; //текущий номер радиостанции
  _timer_ms[TMR_MS] = 0; //сбросили таймер

  dotSetBright(0); //выключаем точки
#if (NEON_DOT != 3) && DOTS_PORT_ENABLE
  indiClrDots(); //очистка разделителных точек
#endif

  while (1) {
    dataUpdate(); //обработка данных

    if (!_timer_ms[TMR_MS]) { //если таймер истек
      indiClr(); //очистка индикаторов
      switch (mode) {
        case 0:
          _timer_ms[TMR_MS] = RADIO_VOL_TIME; //устанавливаем таймер
          if (_state) return 0; //выходим по бездействию
          indiPrintNum(radioSettings.volume, ((LAMP_NUM / 2) - 1), 2, 0); //вывод настройки
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
          setBacklHue(((LAMP_NUM / 2) - 1), 2, RADIO_BACKL_COLOR_1, RADIO_BACKL_COLOR_2);
#endif
          break;
        case 1:
          _timer_ms[TMR_MS] = RADIO_STATION_TIME; //устанавливаем таймер
          if (_state) return 1; //выходим по бездействию
          indiPrintNum((boolean)radioSettings.stationsSave[_station], ((LAMP_NUM / 2) - 2)); //вывод настройки
          indiPrintNum(_station, (LAMP_NUM / 2), 2, 0); //вывод настройки
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
          setBacklHue((LAMP_NUM / 2), 2, RADIO_BACKL_COLOR_1, RADIO_BACKL_COLOR_2);
          setLedHue(((LAMP_NUM / 2) - 2), RADIO_BACKL_COLOR_1, WHITE_ON);
#endif
          break;
      }
      _state = 1; //установили флаг бездействия
    }

    switch (buttonState()) {
      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (mode) {
          case 0: if (radioSettings.volume < RADIO_MAX_VOL) setVolumeRDA(++radioSettings.volume); break;
          case 1: if (_station < (RADIO_MAX_STATIONS - 1)) _station++; else _station = 0; break;
        }
        _state = 0;
        _timer_ms[TMR_MS] = 0; //сбросили таймер
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (mode) {
          case 0: if (radioSettings.volume > RADIO_MIN_VOL) setVolumeRDA(--radioSettings.volume); break;
          case 1: if (_station > 0) _station--; else _station = (RADIO_MAX_STATIONS - 1); break;
        }
        _state = 0;
        _timer_ms[TMR_MS] = 0; //сбросили таймер
        break;

      case ADD_KEY_PRESS: //клик дополнительной кнопкой
        if (mode) { //если режим настройки радиостанций
          radioSettings.stationsSave[_station] = radioSettings.stationsFreq; //сохранили радиостанцию
          radioSettings.stationNum = _station; //установили номер радиостанции
        }
        return 0; //выходим
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        return 1; //выходим

      case ADD_KEY_HOLD: //удержание дополнительной кнопкой
        if (mode) radioSettings.stationsSave[_station] = 0; //сбросили радиостанцию
        _state = 0;
        _timer_ms[TMR_MS] = 0; //сбросили таймер
        break;
    }
  }
}
//---------------------------------Радиоприемник----------------------------------------
uint8_t radioMenu(void) //радиоприемник
{
  if (getPowerStatusRDA() != RDA_ERROR) { //если радиоприемник доступен
    boolean station_show = 0; //флаг анимации номера станции
    uint8_t time_out = 0; //таймаут автовыхода
    uint16_t seek_freq = 0; //частота поиска

#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
    uint8_t seek_anim = 0; //анимация поиска
    backlAnimDisable(); //запретили эффекты подсветки
#if RADIO_BACKL_TYPE == 1
    changeBrightDisable(CHANGE_STATIC_BACKL); //разрешить смену яркости статичной подсветки
    setLedBright((fastSettings.backlMode & 0x7F) ? backl.maxBright : 0); //установили яркость в зависимости от режима подсветки
#else
    setLedBright(backl.menuBright); //установили максимальную яркость
#endif
#endif

    if (getPowerStatusRDA() == RDA_OFF) { //если радио выключено
#if PLAYER_TYPE
      if (mainSettings.knockSound) playerSetTrackNow(PLAYER_RADIO_SOUND, PLAYER_GENERAL_FOLDER);
      playerSetMute(PLAYER_MUTE_ON); //включаем приглушение звука плеера
      radio.powerState = RDA_OFF; //сбросили флаг питания радио
#else
      radioPowerOn(); //включить питание радиоприемника
      radio.powerState = RDA_ON; //установили флаг питания радио
#endif
    }

    radioSearchStation(); //поиск радиостанции в памяти

    radio.seekRun = 0; //сбросили флаг автопоиска
    _timer_ms[TMR_MS] = 0; //сбросили таймер

    while (1) {
      dataUpdate(); //обработка данных

      if (!secUpd) { //если прошла секунда
        secUpd = 1; //сбросили флаг секунды
#if ALARM_TYPE
        if (alarms.now && !alarms.wait) {  //тревога таймера
          radioSeekStop(); //остановка автопоиска радиостанции
          return ALARM_PROGRAM;
        }
#endif
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
        if (timer.mode == 2 && !timer.count) { //тревога таймера
          radioSeekStop(); //остановка автопоиска радиостанции
          return WARN_PROGRAM;
        }
#endif
        if (++time_out >= RADIO_TIMEOUT) { //если время вышло
          radioSeekStop(); //остановка автопоиска радиостанции
          animShow = ANIM_MAIN; //установили флаг анимации
          return MAIN_PROGRAM; //выходим по тайм-ауту
        }
      }

      if (!_timer_ms[TMR_MS]) { //если таймер истек
        _timer_ms[TMR_MS] = RADIO_UPDATE_TIME; //устанавливаем таймер

        if (!radio.seekRun) { //если не идет поиск
#if NEON_DOT == 2
          neonDotSetBright(dot.menuBright); //установка яркости неоновых точек
          if (getStationStatusRDA()) neonDotSet(DOT_RIGHT); //включаем разделительную точку
          else neonDotSet(DOT_NULL); //выключаем разделительную точку
#elif NEON_DOT < 2
          dotSetBright((getStationStatusRDA()) ? dot.menuBright : 0); //управление точками в зависимости от устойчивости сигнала
#elif DOTS_PORT_ENABLE
#if DOTS_TYPE == 1
          if (getStationStatusRDA()) indiSetDotR(3); //установка разделительной точки
          else indiClrDotR(3); //очистка разделительных точек
#else
          if (getStationStatusRDA()) indiSetDotL(0); //установка разделительной точки
          else indiClrDotL(0); //очистка разделительных точек
#endif
#endif
        }
        else { //иначе идет автопоиск
          if (getSeekCompleteStatusRDA()) { //если поиск завершился
            clrSeekCompleteStatusRDA(); //очищаем флаг окончания поиска
            seek_freq = getFreqRDA(); //прочитали частоту
          }
          switch (radio.seekRun) {
            case 1:
              if (radioSettings.stationsFreq > seek_freq) radioSettings.stationsFreq--; else radio.seekRun = 0;
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
              if (seek_anim > 0) seek_anim--; else seek_anim = ((LAMP_NUM + 1) * 2);
#endif
              break;
            case 2:
              if (radioSettings.stationsFreq < seek_freq) radioSettings.stationsFreq++; else radio.seekRun = 0;
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
              if (seek_anim < ((LAMP_NUM + 1) * 2)) seek_anim++; else seek_anim = 0;
#endif
              break;
          }
          if (!radio.seekRun) {
            setMuteRDA(RDA_MUTE_OFF); //выключаем приглушение звука
            radioSettings.stationsFreq = seek_freq; //прочитали частоту
            radioSearchStation(); //поиск радиостанции в памяти
          }
          else _timer_ms[TMR_MS] = RADIO_ANIM_TIME; //устанавливаем таймер
        }

        indiClr(); //очистка индикаторов
        if (station_show) { //если нужно показать номер станции
          station_show = 0; //сбросили флага показа номера станции
          _timer_ms[TMR_MS] = RADIO_SHOW_TIME; //устанавливаем таймер
          indiPrintNum(radioSettings.stationNum, ((LAMP_NUM / 2) - 1), 2, 0); //номер станции
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
          setBacklHue(((LAMP_NUM / 2) - 1), 2, RADIO_BACKL_COLOR_1, RADIO_BACKL_COLOR_2);
#endif
        }
        else { //иначе отображаем частоту
#if DOTS_PORT_ENABLE
#if DOTS_TYPE == 1
          indiSetDotR(2); //включаем разделительную точку
#else
          indiSetDotL(3); //включаем разделительную точку
#endif
#endif
          indiPrintNum(radioSettings.stationsFreq, 0, 4); //текущаяя частота
          if (radioSettings.stationNum < RADIO_MAX_STATIONS) indiPrintNum(radioSettings.stationNum, 5); //номер станции
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
          if (!radio.seekRun) { //если не идет поиск
            setBacklHue(0, 3, RADIO_BACKL_COLOR_1, RADIO_BACKL_COLOR_2);
            setLedHue(3, RADIO_BACKL_COLOR_3, WHITE_ON);
#if LAMP_NUM > 4
            setLedHue(5, RADIO_BACKL_COLOR_3, WHITE_ON);
#endif
          }
          else setBacklHue((seek_anim >> 1) - 1, 1, RADIO_BACKL_COLOR_1, RADIO_BACKL_COLOR_2); //иначе анимация
#endif
        }
      }

#if PLAYER_TYPE
      if (!radio.powerState) { //если питание выключено
        if (!playerPlaybackStatus()) { //если все команды отправлены
          radio.powerState = RDA_ON; //установили флаг питания радио
          radioPowerOn(); //включить питание радиоприемника
        }
      }
#endif

      switch (buttonState()) {
        case RIGHT_KEY_PRESS: //клик правой кнопкой
          radioSeekStop(); //остановка автопоиска радиостанции
          if (radioSettings.stationsFreq < RADIO_MAX_FREQ) radioSettings.stationsFreq++; else radioSettings.stationsFreq = RADIO_MIN_FREQ; //переключаем частоту
          setFreqRDA(radioSettings.stationsFreq); //установили частоту
          radioSearchStation(); //поиск радиостанции в памяти
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;

        case LEFT_KEY_PRESS: //клик левой кнопкой
          radioSeekStop(); //остановка автопоиска радиостанции
          if (radioSettings.stationsFreq > RADIO_MIN_FREQ) radioSettings.stationsFreq--; else radioSettings.stationsFreq = RADIO_MAX_FREQ; //переключаем частоту
          setFreqRDA(radioSettings.stationsFreq); //установили частоту
          radioSearchStation(); //поиск радиостанции в памяти
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;

        case ADD_KEY_PRESS: //клик дополнительной кнопкой
          radioSeekStop(); //остановка автопоиска радиостанции
          radioSwitchStation(1); //переключить радиостанцию в памяти
          station_show = 1; //подняли флаг отображения номера станции
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;

        case SET_KEY_PRESS: //клик средней кнопкой
          radioSeekStop(); //остановка автопоиска радиостанции
          if (radioMenuSettings(0)) return MAIN_PROGRAM; //настройки радио
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;


        case RIGHT_KEY_HOLD: //удержание правой кнопки
          if (!radio.seekRun) { //если не идет поиск
            if (radioSettings.stationsFreq < RADIO_MAX_FREQ) { //если не достигли предела поиска
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
              seek_anim = 0; //сбросили анимацию поиска
#endif
              radio.seekRun = 2; //включили поиск
              seek_freq = RADIO_MAX_FREQ; //установили максимальную частоту
              radioSettings.stationNum |= 0x80; //установили номер ячейки за пределом видимости
              setMuteRDA(RDA_MUTE_ON); //включаем приглушение звука
              startSeekRDA(RDA_SEEK_UP); //начинаем поиск вверх
              dotSetBright(0); //выключаем точки
#if (NEON_DOT != 3) && DOTS_PORT_ENABLE
              indiClrDots(); //очистка разделителных точек
#endif
            }
          }
          else radioSeekStop(); //остановка автопоиска радиостанции
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = RADIO_ANIM_TIME; //устанавливаем таймер
          break;

        case LEFT_KEY_HOLD: //удержание левой кнопки
          if (!radio.seekRun) { //если не идет поиск
            if (radioSettings.stationsFreq > RADIO_MIN_FREQ) { //если не достигли предела поиска
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
              seek_anim = ((LAMP_NUM + 1) * 2); //сбросили анимацию поиска
#endif
              radio.seekRun = 1; //включили поиск
              seek_freq = RADIO_MIN_FREQ; //установили минимальную частоту
              radioSettings.stationNum |= 0x80; //установили номер ячейки за пределом видимости
              setMuteRDA(RDA_MUTE_ON); //включаем приглушение звука
              startSeekRDA(RDA_SEEK_DOWN); //начинаем поиск вниз
              dotSetBright(0); //выключаем точки
#if (NEON_DOT != 3) && DOTS_PORT_ENABLE
              indiClrDots(); //очистка разделителных точек
#endif
            }
          }
          else radioSeekStop(); //остановка автопоиска радиостанции
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = RADIO_ANIM_TIME; //устанавливаем таймер
          break;

        case ADD_KEY_HOLD: //удержание дополнительной кнопки
          if (!radio.seekRun) { //если не идет поиск
            if (!radioMenuSettings(1)) { //настройки радио
#if !PLAYER_TYPE
              buzz_pulse(RADIO_SAVE_SOUND_FREQ, RADIO_SAVE_SOUND_TIME); //сигнал успешной записи радиостанции в память
#endif
              station_show = 1; //подняли флаг отображения номера станции
            }
          }
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;

        case SET_KEY_HOLD: //удержание средней кнопк
          radio.powerState = RDA_OFF; //сбросили флаг питания радио
          setPowerRDA(RDA_OFF); //выключаем радио
          return MAIN_PROGRAM; //выходим

#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
        case PWR_KEY_PRESS: //управление питанием
          radioSeekStop(); //остановка автопоиска радиостанции
          return MAIN_PROGRAM; //выход в режим часов
        case VOL_UP_KEY_PRESS: //прибавить громкость
          radioSeekStop(); //остановка автопоиска радиостанции
          radioFastSettings(VOL_UP_KEY_PRESS); //прибавить громкость
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;
        case VOL_DOWN_KEY_PRESS: //убавить громкость
          radioSeekStop(); //остановка автопоиска радиостанции
          radioFastSettings(VOL_DOWN_KEY_PRESS); //убавить громкость
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;
        case STATION_UP_KEY_PRESS: //следующая станция
          radioSeekStop(); //остановка автопоиска радиостанции
          radioFastSettings(STATION_UP_KEY_PRESS); //следующая станция
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;
        case STATION_DOWN_KEY_PRESS: //предыдущая станция
          radioSeekStop(); //остановка автопоиска радиостанции
          radioFastSettings(STATION_DOWN_KEY_PRESS); //предыдущая станция
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;
#endif
      }
    }
    return INIT_PROGRAM;
  }
  return MAIN_PROGRAM;
}
//--------------------------------Тревога таймера----------------------------------------
uint8_t timerWarn(void) //тревога таймера
{
  boolean blink_data = 0; //флаг мигания индикаторами

#if PLAYER_TYPE
  playerStop(); //сброс позиции мелодии
#else
  melodyPlay(SOUND_TIMER_WARN, SOUND_LINK(general_sound), REPLAY_CYCLE); //звук окончания таймера
#endif
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
  radioPowerOff(); //выключить питание радиоприемника
#endif
#if (BACKL_TYPE == 3) && TIMER_WARN_BACKL_TYPE
  backlAnimDisable(); //запретили эффекты подсветки
#if TIMER_WARN_BACKL_TYPE == 1
  changeBrightDisable(CHANGE_DYNAMIC_BACKL); //разрешить смену яркости динамичной подсветки
#endif
  setLedHue(TIMER_WARN_COLOR, WHITE_ON); //установили цвет
#endif
  while (!buttonState()) { //ждем
    dataUpdate(); //обработка данных
#if PLAYER_TYPE
    if (!playerPlaybackStatus()) playerSetTrack(PLAYER_TIMER_WARN_SOUND, PLAYER_GENERAL_FOLDER);
#endif
    if (!_timer_ms[TMR_ANIM]) {
      _timer_ms[TMR_ANIM] = TIMER_BLINK_TIME;
      switch (blink_data) {
        case 0: indiClr(); break; //очищаем индикаторы
        case 1: indiPrintNum(0, 0, 6, 0); break; //вывод минут/часов/секунд
      }
      dotSetBright((blink_data) ? dot.menuBright : 0); //установили точки
#if (BACKL_TYPE == 3) && TIMER_WARN_BACKL_TYPE
#if TIMER_WARN_BACKL_TYPE == 1
      setLedBright((blink_data) ? backl.maxBright : 0); //установили яркость
#else
      setLedBright((blink_data) ? backl.menuBright : 0); //установили яркость
#endif
#endif
      blink_data = !blink_data; //мигаем временем
    }
  }
  timer.mode = 0; //деактивируем таймер
  timer.count = timer.time; //сбрасываем таймер
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
  radioPowerRet(); //вернуть питание радиоприемника
#endif
  return TIMER_PROGRAM;
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

      if (!blink_data || mode) indiPrintNum(timer.time / 60, 0, 2, 0); //вывод минут
      if (!blink_data || !mode) indiPrintNum(timer.time % 60, 2, 2, 0); //вывод секунд

#if (BACKL_TYPE == 3) && TIMER_BACKL_TYPE
      setBacklHue(mode * 2, 2, TIMER_MENU_COLOR_1, TIMER_MENU_COLOR_2);
#endif
      blink_data = !blink_data;
    }

    switch (buttonState()) {
      case SET_KEY_PRESS: //клик средней кнопкой
        mode = !mode; //переключаем режим
        _timer_ms[TMR_MS] = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (mode) {
          case 0: if (timer.time / 60 < 99) timer.time += 60; else timer.time -= 5940; break; //сбрасываем секундомер
          case 1: if (timer.time % 60 < 59) timer.time++; else timer.time -= 59; break; //сбрасываем таймер
        }
        _timer_ms[TMR_MS] = blink_data = 0; //сбрасываем флаги
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (mode) {
          case 0: if (timer.time / 60 > 0) timer.time -= 60; else timer.time += 5940; break; //сбрасываем секундомер
          case 1: if (timer.time % 60 > 0) timer.time--; else timer.time += 59; break; //сбрасываем таймер
        }
        _timer_ms[TMR_MS] = blink_data = 0; //сбрасываем флаги
        break;

      case ADD_KEY_HOLD: //удержание дополнительной кнопки
      case SET_KEY_HOLD: //удержание средней кнопки
        if (!timer.time) timer.time = TIMER_TIME; //устанавливаем значение по умолчанию
        timer.count = timer.time; //сбрасываем таймер
        return; //выходим
    }
  }
}
//--------------------------------Таймер-секундомер----------------------------------------
uint8_t timerStopwatch(void) //таймер-секундомер
{
  uint8_t mode = 0; //текущий режим
  uint8_t time_out = 0; //таймаут автовыхода
  static uint8_t millisCnt; //счетчик миллисекун

  if (timer.mode & 0x7F) mode = (timer.mode & 0x7F) - 1; //если таймер был запущен
  else timer.count = 0; //иначе сбрасываем таймер

#if PLAYER_TYPE
  if (mainSettings.knockSound) playerSetTrackNow((mode) ? PLAYER_TIMER_SOUND : PLAYER_STOPWATCH_SOUND, PLAYER_GENERAL_FOLDER);
#endif

#if (BACKL_TYPE == 3) && TIMER_BACKL_TYPE
  backlAnimDisable(); //запретили эффекты подсветки
#if TIMER_BACKL_TYPE == 1
  changeBrightDisable(CHANGE_STATIC_BACKL); //разрешить смену яркости статичной подсветки
  setLedBright((fastSettings.backlMode & 0x7F) ? backl.maxBright : 0); //установили яркость в зависимости от режима подсветки
#else
  setLedBright(backl.menuBright); //установили максимальную яркость
#endif
#endif

  while (1) {
    dataUpdate(); //обработка данных

    if (timer.mode == 2 && !timer.count) return WARN_PROGRAM; //тревога таймера

    if (!secUpd) {
      secUpd = 1; //сбрасываем флаг
      if ((!timer.mode || timer.mode & 0x80) && ++time_out >= TIMER_TIMEOUT) {
        animShow = ANIM_MAIN; //установили флаг анимации
        return MAIN_PROGRAM;
      }

#if LAMP_NUM > 4
      if (!timer.count) dotSetBright(0); //выключили точки
      else if (timer.mode > 2 || !timer.mode) dotSetBright(dot.menuBright); //установили точки
      else dotSetBright((dotGetBright()) ? 0 : dot.menuBright); //установили точки
#else
      if (!timer.count) dotSetBright(0); //выключили точки
      else if (timer.mode > 2) dotSetBright(dot.menuBright); //установили точки
      else dotSetBright((dotGetBright()) ? 0 : dot.menuBright); //установили точки
#endif

      indiClr(); //очистка индикаторов
      switch (timer.mode) {
        case 0:
          indiPrintNum(mode + 1, 5); //вывод режима
          break;
        default:
          if (!(timer.mode & 0x80)) millisCnt = 0; //сбрасываем счетчик миллисекунд
          indiPrintNum((timer.count < 3600) ? ((mode) ? (100 - millisCnt) : millisCnt) : (timer.count % 60), 4, 2, 0); //вывод милиекунд/секунд
          break;
      }

#if (BACKL_TYPE == 3) && TIMER_BACKL_TYPE
      if (timer.mode & 0x80) setLedHue(TIMER_PAUSE_COLOR, WHITE_ON); //установили цвет паузы
      else {
        switch (timer.mode) {
          case 0: setLedHue(TIMER_STOP_COLOR, WHITE_ON); break; //установили цвет остановки
          case 1: setLedHue(TIMER_RUN_COLOR_1, WHITE_ON); break; //установили цвет секундомера
          case 2: setLedHue(TIMER_RUN_COLOR_2, WHITE_ON); break; //установили цвет таймера
        }
      }
#endif

      indiPrintNum((timer.count < 3600) ? ((timer.count / 60) % 60) : (timer.count / 3600), 0, 2, 0); //вывод минут/часов
      indiPrintNum((timer.count < 3600) ? (timer.count % 60) : ((timer.count / 60) % 60), 2, 2, 0); //вывод секунд/минут
    }

    switch (timer.mode) {
      case 1: case 2:
        if (!_timer_ms[TMR_MS]) {
          _timer_ms[TMR_MS] = 10;
          if (timer.count < 3600) {
            millisCnt += 1;
            indiPrintNum((mode) ? (100 - millisCnt) : millisCnt, 4, 2, 0); //вывод милиекунд
          }
        }
        break;
    }

    switch (buttonState()) {
      case SET_KEY_PRESS: //клик средней кнопкой
        if (mode && !timer.mode) { //если режим таймера и таймер/секундомер не запущен
          timerSettings(); //настройки таймера
          time_out = 0; //сбрасываем таймер автовыхода
          secUpd = 0; //обновление экрана
        }
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        if (timer.mode == 1) timer.mode |= 0x80; //приостановка секундомера
        return MAIN_PROGRAM; //выходим

      case RIGHT_KEY_PRESS: //клик правой кнопкой
      case RIGHT_KEY_HOLD: //удержание правой кнопки
#if PLAYER_TYPE
        if (mainSettings.knockSound) playerSetTrackNow(PLAYER_TIMER_SOUND, PLAYER_GENERAL_FOLDER);
#endif
        mode = 1; //переключаем режим
        timer.mode = 0; //деактивируем таймер
        timer.count = timer.time; //сбрасываем таймер
        time_out = 0; //сбрасываем таймер автовыхода
        secUpd = 0; //обновление экрана
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
      case LEFT_KEY_HOLD: //удержание левой кнопки
#if PLAYER_TYPE
        if (mainSettings.knockSound) playerSetTrackNow(PLAYER_STOPWATCH_SOUND, PLAYER_GENERAL_FOLDER);
#endif
        mode = 0; //переключаем режим
        timer.mode = 0; //деактивируем таймер
        timer.count = 0; //сбрасываем секундомер
        time_out = 0; //сбрасываем таймер автовыхода
        secUpd = 0; //обновление экрана
        break;

      case ADD_KEY_PRESS: //клик дополнительной кнопкой
        if (!timer.mode) {
          millisCnt = 0; //сбрасываем счетчик миллисекунд
          timer.mode = mode + 1;
        }
        else timer.mode ^= 0x80; //приостановка таймера/секундомера
        time_out = 0; //сбрасываем таймер автовыхода
        secUpd = 0; //обновление экрана
        break;

      case ADD_KEY_HOLD: //удержание дополнительной кнопки
        timer.mode = 0; //деактивируем таймер
        switch (mode & 0x01) {
          case 0: timer.count = 0; break; //сбрасываем секундомер
          case 1: timer.count = timer.time; break; //сбрасываем таймер
        }
        time_out = 0; //сбрасываем таймер автовыхода
        secUpd = 0; //обновление экрана
        break;
    }
  }
  return INIT_PROGRAM;
}
//------------------------------------Звук смены часа------------------------------------
void hourSound(void) //звук смены часа
{
  if ((mainTask == MAIN_PROGRAM) || (mainTask == SLEEP_PROGRAM)) { //если в режиме часов или спим
    if (checkHourStrart(mainSettings.timeHour[0], mainSettings.timeHour[1])) {
#if PLAYER_TYPE
#if HOUR_SOUND_SPEAK_TYPE == 2
      if (mainSettings.knockSound) {
#endif
#if (HOUR_SOUND_SPEAK_TYPE == 1) || (HOUR_SOUND_SPEAK_TYPE == 2)
        speakTime(); //воспроизвести время
#if HOUR_SOUND_SPEAK_TEMP
        if (!_timer_ms[TMR_SENS]) { //если таймаут нового запроса вышел
          updateTemp(); //обновить показания температуры
          _timer_ms[TMR_SENS] = TEMP_UPDATE_TIME; //установили таймаут
        }
        speakTempCeil(); //воспроизвести целую температуру
#endif
#endif
#if HOUR_SOUND_SPEAK_TYPE == 2
      }
      else
#endif
#if (HOUR_SOUND_SPEAK_TYPE == 0) || (HOUR_SOUND_SPEAK_TYPE == 2)
        playerSetTrackNow(PLAYER_HOUR_SOUND, PLAYER_GENERAL_FOLDER); //звук смены часа
#endif
#else
      melodyPlay(SOUND_HOUR, SOUND_LINK(general_sound), REPLAY_ONCE); //звук смены часа
#endif
    }
  }
}
//---------------------Установка яркости от времени суток-----------------------------
void changeBright(void) //установка яркости от времени суток
{
  indi.sleepMode = SLEEP_DISABLE; //сбросили флаг режима сна индикаторов

#if LIGHT_SENS_ENABLE
  if ((mainSettings.timeBright[TIME_NIGHT] != mainSettings.timeBright[TIME_DAY]) ? (checkHourStrart(mainSettings.timeBright[TIME_NIGHT], mainSettings.timeBright[TIME_DAY])) : state_light)
#else
  if (checkHourStrart(mainSettings.timeBright[TIME_NIGHT], mainSettings.timeBright[TIME_DAY]))
#endif
  { //ночной режим
    dot.maxBright = mainSettings.dotBright[TIME_NIGHT]; //установка максимальной яркости точек
    dot.menuBright = (dot.maxBright) ? dot.maxBright : 10; //установка максимальной яркости точек в меню
#if BACKL_TYPE
    backl.maxBright = mainSettings.backlBright[TIME_NIGHT]; //установка максимальной яркости подсветки
    backl.menuBright = (backl.maxBright) ? backl.maxBright : 10; //установка максимальной яркости подсветки в меню
#endif
    indi.maxBright = mainSettings.indiBright[TIME_NIGHT]; //установка максимальной яркости индикаторов
    if (mainSettings.timeSleep[TIME_NIGHT]) indi.sleepMode = SLEEP_NIGHT; //установили флаг режима сна индикаторов
  }
  else { //дневной режим
#if (NEON_DOT != 3) || !DOTS_PORT_ENABLE
    dot.menuBright = dot.maxBright = mainSettings.dotBright[TIME_DAY]; //установка максимальной яркости точек
#else
    dot.menuBright = dot.maxBright = 1; //установка максимальной яркости точек
#endif
#if BACKL_TYPE
    backl.menuBright = backl.maxBright = mainSettings.backlBright[TIME_DAY]; //установка максимальной яркости подсветки
#endif
    indi.maxBright = mainSettings.indiBright[TIME_DAY]; //установка максимальной яркости индикаторов
    if (mainSettings.timeSleep[TIME_DAY]) indi.sleepMode = SLEEP_DAY; //установили флаг режима сна индикаторов
  }

  if (changeBrightState) { //если разрешено менять яркость
    if (mainTask == MAIN_PROGRAM) { //если основной режим
#if ALARM_TYPE
      switch ((alarms.dot) ? (alarms.dot - 1) : fastSettings.dotMode) { //мигание точек
#else
      switch (fastSettings.dotMode) { //мигание точек
#endif
        case DOT_OFF: dotSetBright(0); break; //точки выключены
        case DOT_STATIC: dotSetBright(dot.maxBright); break; //точки включены
#if (NEON_DOT != 3) || !DOTS_PORT_ENABLE
        case DOT_PULS: //плавное мигание
#if NEON_DOT == 2
        case DOT_TURN_BLINK_NEON:
#endif
          if (dot.maxBright) { //если яркость установлена
            dot.brightStep = setBrightStep(dot.maxBright * 2, DOT_PULS_STEP_TIME, DOT_PULS_TIME); //расчёт шага яркости точки
            dot.brightTime = setBrightTime(dot.maxBright * 2, DOT_PULS_STEP_TIME, DOT_PULS_TIME); //расчёт шага яркости точки
          }
          else dotSetBright(0); //иначе выключаем точки
          break;
#endif
        default:
#if NEON_DOT != 3
          if (dot.maxBright) { //если яркость установлена
            if (dotGetBright()) dotSetBright(dot.maxBright); //установка яркости точек
          }
          else dotSetBright(0); //иначе выключаем точки
#endif
#if DOTS_PORT_ENABLE
          if (!dot.maxBright) indiClrDots(); //очистка разделителных точек
#endif
          break;
      }
    }
#if NEON_DOT < 2
    else if (dotGetBright()) dotSetBright(dot.menuBright); //установка яркости точек в меню
#elif NEON_DOT == 2
    else if (dotGetBright()) neonDotSetBright(dot.menuBright); //установка яркости точек в меню
#endif

#if BACKL_TYPE
    if (fastSettings.backlMode & 0x80) { //если подсветка заблокирована
#if BACKL_TYPE == 3
      switch (changeBrightState) { //режим управления яркостью
        case CHANGE_STATIC_BACKL: if (fastSettings.backlMode & 0x7F) setLedBright(backl.maxBright); break; //устанавливаем максимальную яркость
        case CHANGE_DYNAMIC_BACKL: setOnLedBright(backl.maxBright); break; //устанавливаем максимальную яркость
        default: setOnLedBright(backl.menuBright); break; //установка яркости подсветки в меню
      }
#else
      if (backGetBright()) backlSetBright(backl.menuBright); //установили яркость если она была включена
#endif
    }
    else { //иначе устанавливаем яркость
#if BACKL_TYPE == 3
      if (backl.maxBright) {
        switch (fastSettings.backlMode) {
          case BACKL_OFF: clrLeds(); break; //выключили светодиоды
          case BACKL_STATIC:
            setLedBright(backl.maxBright); //устанавливаем максимальную яркость
            setLedHue(fastSettings.backlColor, WHITE_ON); //устанавливаем статичный цвет
            break;
          case BACKL_SMOOTH_COLOR_CHANGE:
          case BACKL_RAINBOW:
          case BACKL_CONFETTI:
            setLedBright(backl.maxBright); //устанавливаем максимальную яркость
            break;
        }
      }
      else clrLeds(); //выключили светодиоды
#else
      switch (fastSettings.backlMode) {
        case BACKL_OFF: backlSetBright(0); break; //если посветка выключена
        case BACKL_STATIC: backlSetBright(backl.maxBright); break; //если посветка статичная, устанавливаем яркость
        case BACKL_PULS: if (!backl.maxBright) backlSetBright(0); break; //иначе посветка выключена
      }
#endif
      if (backl.maxBright) {
        backl.minBright = (backl.maxBright > (BACKL_MIN_BRIGHT + 10)) ? BACKL_MIN_BRIGHT : 0;
        uint8_t backlNowBright = (backl.maxBright > BACKL_MIN_BRIGHT) ? (backl.maxBright - BACKL_MIN_BRIGHT) : backl.maxBright;

        backl.mode_2_time = setBrightTime((uint16_t)backlNowBright * 2, BACKL_MODE_2_STEP_TIME, BACKL_MODE_2_TIME); //расчёт шага яркости
        backl.mode_2_step = setBrightStep((uint16_t)backlNowBright * 2, BACKL_MODE_2_STEP_TIME, BACKL_MODE_2_TIME); //расчёт шага яркости

#if BACKL_TYPE == 3
        backl.mode_4_step = ceil((float)backl.maxBright / (float)BACKL_MODE_4_TAIL / (float)BACKL_MODE_4_FADING); //расчёт шага яркости
        if (!backl.mode_4_step) backl.mode_4_step = 1; //если шаг слишком мал
        backl.mode_8_time = setBrightTime((uint16_t)backlNowBright * LAMP_NUM, BACKL_MODE_8_STEP_TIME, BACKL_MODE_8_TIME); //расчёт шага яркости
        backl.mode_8_step = setBrightStep((uint16_t)backlNowBright * LAMP_NUM, BACKL_MODE_8_STEP_TIME, BACKL_MODE_8_TIME); //расчёт шага яркости
#endif
      }
    }
#endif
#if BURN_BRIGHT
    if (changeBrightState != CHANGE_INDI_BLOCK) indiSetBright(indi.maxBright); //установка общей яркости индикаторов
#else
    indiSetBright(indi.maxBright); //установка общей яркости индикаторов
#endif
  }
}
//----------------------------------Анимация подсветки---------------------------------
void backlEffect(void) //анимация подсветки
{
  if (backl.maxBright) { //если подсветка не выключена
    if (!_timer_ms[TMR_BACKL]) { //если время пришло
      switch (fastSettings.backlMode) {
        case BACKL_OFF: //подсветка выключена
        case BACKL_STATIC: //статичный режим
          return; //выходим
        case BACKL_PULS:
        case BACKL_PULS_COLOR: { //дыхание подсветки
            _timer_ms[TMR_BACKL] = backl.mode_2_time; //установили таймер
            if (backl.drive) { //если светодиоды в режиме разгорания
              if (incLedBright(backl.mode_2_step, backl.maxBright)) backl.drive = 0; //прибавили шаг яркости
            }
            else { //иначе светодиоды в режиме затухания
              if (decLedBright(backl.mode_2_step, backl.minBright)) { //уменьшаем яркость
                backl.drive = 1;
                if (fastSettings.backlMode == BACKL_PULS_COLOR) backl.color += BACKL_MODE_3_COLOR; //меняем цвет
                else backl.color = fastSettings.backlColor; //иначе статичный цвет
                setLedHue(backl.color, WHITE_ON); //установили цвет
                _timer_ms[TMR_BACKL] = BACKL_MODE_2_PAUSE; //установили таймер
              }
            }
          }
          break;
        case BACKL_RUNNING_FIRE:
        case BACKL_RUNNING_FIRE_COLOR:
        case BACKL_RUNNING_FIRE_RAINBOW:
        case BACKL_RUNNING_FIRE_CONFETTI: { //бегущий огонь
            _timer_ms[TMR_BACKL] = BACKL_MODE_4_TIME / LAMP_NUM / BACKL_MODE_4_FADING; //установили таймер
            if (backl.steps) { //если есть шаги затухания
              decLedsBright(backl.position - 1, backl.mode_4_step); //уменьшаем яркость
              backl.steps--; //уменьшаем шаги затухания
            }
            else { //иначе двигаем голову
              if (backl.drive) { //если направление вправо
                if (backl.position > 0) backl.position--; else backl.drive = 0; //едем влево
              }
              else { //иначе напрвление влево
                if (backl.position < (LAMP_NUM + 1)) backl.position++; else backl.drive = 1; //едем вправо
              }
              setLedBright(backl.position - 1, backl.maxBright); //установили яркость
              backl.steps = BACKL_MODE_4_FADING; //установили шаги затухания
            }
            if (fastSettings.backlMode == BACKL_RUNNING_FIRE) {
              backl.color = fastSettings.backlColor; //статичный цвет
              setLedHue(backl.color, WHITE_ON); //установили цвет
            }
          }
          break;
        case BACKL_WAVE:
        case BACKL_WAVE_COLOR:
        case BACKL_WAVE_RAINBOW:
        case BACKL_WAVE_CONFETTI: { //волна
            _timer_ms[TMR_BACKL] = backl.mode_8_time; //установили таймер
            if (backl.drive) {
              if (incLedBright(backl.position, backl.mode_8_step, backl.maxBright)) { //прибавили шаг яркости
                if (backl.position < (LAMP_NUM - 1)) backl.position++; //сменили позицию
                else {
                  backl.position = 0; //сбросили позицию
                  backl.drive = 0; //перешли в затухание
                }
              }
            }
            else {
              if (decLedBright(backl.position, backl.mode_8_step, backl.minBright)) { //иначе убавляем яркость
                if (backl.position < (LAMP_NUM - 1)) backl.position++; //сменили позицию
                else {
                  backl.position = 0; //сбросили позицию
                  backl.drive = 1; //перешли в разгорание
                }
              }
            }
            if (fastSettings.backlMode == BACKL_WAVE) {
              backl.color = fastSettings.backlColor; //статичный цвет
              setLedHue(backl.color, WHITE_ON); //установили цвет
            }
          }
          break;
      }
    }
    if (!_timer_ms[TMR_COLOR]) { //если время пришло
      switch (fastSettings.backlMode) {
        case BACKL_RUNNING_FIRE_RAINBOW:
        case BACKL_WAVE_RAINBOW:
        case BACKL_RAINBOW: { //радуга
            _timer_ms[TMR_COLOR] = BACKL_MODE_13_TIME; //установили таймер
            backl.color += BACKL_MODE_13_STEP; //прибавили шаг
            for (uint8_t f = 0; f < LAMP_NUM; f++) setLedHue(f, backl.color + (f * BACKL_MODE_13_STEP), WHITE_OFF); //установили цвет
          }
          break;
        case BACKL_RUNNING_FIRE_CONFETTI:
        case BACKL_WAVE_CONFETTI:
        case BACKL_CONFETTI: { //рандомный цвет
            _timer_ms[TMR_COLOR] = BACKL_MODE_14_TIME; //установили таймер
            setLedHue(random(0, LAMP_NUM), random(0, 256), WHITE_ON); //установили цвет
          }
          break;
        case BACKL_RUNNING_FIRE_COLOR:
        case BACKL_WAVE_COLOR:
        case BACKL_SMOOTH_COLOR_CHANGE: { //плавная смена цвета
            _timer_ms[TMR_COLOR] = BACKL_MODE_12_TIME; //установили таймер
            backl.color += BACKL_MODE_12_COLOR;
            setLedHue(backl.color, WHITE_OFF); //установили цвет
          }
          break;
      }
    }
  }
}
//----------------------------------Мигание подсветки---------------------------------
void backlFlash(void) //мигание подсветки
{
  if (backl.maxBright && fastSettings.backlMode == BACKL_PULS) {
    if (!_timer_ms[TMR_BACKL]) {
      _timer_ms[TMR_BACKL] = backl.mode_2_time;
      if (backl.drive) {
        if (backlDecBright(backl.mode_2_step, backl.minBright)) {
          _timer_ms[TMR_BACKL] = BACKL_MODE_2_PAUSE;
          backl.drive = 0;
        }
      }
      else if (backlIncBright(backl.mode_2_step, backl.maxBright)) backl.drive = 1;
    }
  }
}
//--------------------------------Мигание точек------------------------------------
void dotFlash(void) //мигание точек
{
  if (mainTask == MAIN_PROGRAM) { //если основная программа
    if (!dot.update && !_timer_ms[TMR_DOT]) { //если пришло время
      if (!dot.maxBright && (animShow < ANIM_MAIN)) { //если яркость не выключена и не запущена сторонняя анимация
        dot.update = 1; //сбросили флаг секунд
        return; //выходим
      }
#if ALARM_TYPE
      switch ((alarms.dot) ? (alarms.dot - 1) : fastSettings.dotMode) //режим точек
#else
      switch (fastSettings.dotMode) //режим точек
#endif
      { //мигание точек
        case DOT_PULS:
#if (NEON_DOT == 3) && DOTS_PORT_ENABLE
          if (!dot.drive) {
            dotSetBright(dot.maxBright); dot.drive = 1; //включаем точки
#if DOT_PULS_TIME != 1000
            _timer_ms[TMR_DOT] = DOT_PULS_TIME; //установили таймер
#else
            dot.update = 1; //сбросили флаг секунд
#endif
          }
          else {
            dotSetBright(0); //выключаем точки
            dot.drive = 0; //сменили направление
            dot.update = 1; //сбросили флаг секунд
          }
#else
          if (!dot.drive) {
            if (dotIncBright(dot.brightStep, dot.maxBright)) dot.drive = 1; //сменили направление
          }
          else {
            if (dotDecBright(dot.brightStep, 0)) {
              dot.drive = 0; //сменили направление
              dot.update = 1; //сбросили флаг обновления точек
              return; //выходим
            }
          }
          _timer_ms[TMR_DOT] = dot.brightTime; //установили таймер
#endif
          break;
#if NEON_DOT == 2
        case DOT_TURN_BLINK_NEON:
          switch (dot.count) {
            case 0: if (dotIncBright(dot.brightStep, dot.maxBright, DOT_LEFT)) dot.count = 1; break; //сменили направление
            case 1: if (dotDecBright(dot.brightStep, 0, DOT_LEFT)) dot.count = 2; break; //сменили направление
            case 2: if (dotIncBright(dot.brightStep, dot.maxBright, DOT_RIGHT)) dot.count = 3; break; //сменили направление
            default:
              if (dotDecBright(dot.brightStep, 0, DOT_RIGHT)) {
                dot.count = 0; //сменили направление
                dot.update = 1; //сбросили флаг обновления точек
                return; //выходим
              }
              break;
          }
          _timer_ms[TMR_DOT] = dot.brightTime; //установили таймер
          break;
#endif
#if DOTS_PORT_ENABLE
        case DOT_RUNNING: //бегущая
          indiClrDots(); //очистка разделителных точек
          indiSetDots(dot.count, 1); //установка разделительных точек
          if (dot.drive) {
            if (dot.count > 0) dot.count--; //сместили точку
            else {
              dot.drive = 0; //сменили направление
              dot.update = 1; //сбросили флаг обновления точек
              return; //выходим
            }
          }
          else {
            if (dot.count < (DOTS_ALL - 1)) dot.count++; //сместили точку
            else {
              dot.drive = 1; //сменили направление
              dot.update = 1; //сбросили флаг обновления точек
              return; //выходим
            }
          }
          _timer_ms[TMR_DOT] = (DOT_RUNNING_TIME / DOTS_ALL); //установили таймер
          break;
        case DOT_SNAKE: //змейка
          indiClrDots(); //очистка разделителных точек
          indiSetDots(dot.count - (DOTS_ALL - 1), DOTS_ALL); //установка разделительных точек
          if (dot.drive) {
            if (dot.count > 0) dot.count--; //убавили шаг
            else {
              dot.drive = 0; //сменили направление
              dot.update = 1; //сбросили флаг обновления точек
              return; //выходим
            }
          }
          else {
            if (dot.count < ((DOTS_ALL * 2) - 2)) dot.count++; //прибавили шаг
            else {
              dot.drive = 1; //сменили направление
              dot.update = 1; //сбросили флаг обновления точек
              return; //выходим
            }
          }
          _timer_ms[TMR_DOT] = (DOT_SNAKE_TIME / (DOTS_ALL * 2)); //установили таймер
          break;
        case DOT_RUBBER_BAND: //резинка
          indiClrDots(); //очистка разделителных точек
          if (dot.drive) { //если режим убывания
            if (dot.steps < (DOTS_ALL - 1)) dot.steps++; //сместили точку
            else {
              if (dot.count) { //если еще есть точки
                dot.count--; //убавили шаг
                dot.steps = dot.count; //установили точку
              }
              else {
                dot.steps = 0; //сбросили точку
                dot.drive = 0; //сменили направление
                dot.update = 1; //сбросили флаг обновления точек
                return; //выходим
              }
            }
            indiSetDots(dot.steps, 1); //установка разделительных точек
            indiSetDots(0, dot.count); //установка разделительных точек
          }
          else { //иначе режим заполнения
            indiSetDots(dot.steps, 1); //установка разделительных точек
            indiSetDots(DOTS_ALL - dot.count, dot.count); //установка разделительных точек
            if (dot.steps < ((DOTS_ALL - 1) - dot.count)) dot.steps++; //сместили точку
            else {
              if (dot.count < (DOTS_ALL - 1)) { //если еще есть точки
                dot.count++; //прибавили шаг
                dot.steps = 0; //сбросили точку
              }
              else {
                dot.steps = dot.count; //установили точку
                dot.drive = 1; //сменили направление
                dot.update = 1; //сбросили флаг обновления точек
                return; //выходим
              }
            }
          }
          _timer_ms[TMR_DOT] = (DOT_RUBBER_BAND_TIME / (DOTS_ALL * ((DOTS_ALL / 2) + 0.5))); //установили таймер
          break;
#if (DOTS_NUM > 4) || (DOTS_TYPE == 2)
        case DOT_TURN_BLINK: //мигание по очереди
#if DOT_TURN_TIME
          if (dot.count < ((1000 / DOT_TURN_TIME) - 1)) {
            dot.count++; //прибавили шаг
            _timer_ms[TMR_DOT] = DOT_TURN_TIME; //установили таймер
          }
          else {
            dot.count = 0; //сбросили счетчик
            dot.update = 1; //сбросили флаг секунд
          }
#else
          dot.update = 1; //сбросили флаг секунд
#endif
          switch (dot.drive) {
#if DOTS_TYPE == 2
#if LAMP_NUM > 4
            case 0: indiSetDotL(2); indiClrDotR(3); dot.drive = 1; break; //включаем левую точку
            case 1: indiSetDotR(3); indiClrDotL(2); dot.drive = 0; break; //включаем правую точку
#else
            case 0: indiSetDotR(1); indiClrDotL(2); dot.drive = 1; break; //включаем левую точку
            case 1: indiSetDotL(2); indiClrDotR(1); dot.drive = 0; break; //включаем правую точку
#endif
#else
#if DOTS_TYPE == 1
            case 0: indiSetDotR(1); indiClrDotR(3); dot.drive = 1; break; //включаем левую точку
            case 1: indiSetDotR(3); indiClrDotR(1); dot.drive = 0; break; //включаем правую точку
#else
            case 0: indiSetDotL(2); indiClrDotL(4); dot.drive = 1; break; //включаем левую точку
            case 1: indiSetDotL(4); indiClrDotL(2); dot.drive = 0; break; //включаем правую точку
#endif
#endif
          }
          break;
#endif
#endif
        case DOT_BLINK:
          switch (dot.drive) {
            case 0: dotSetBright(dot.maxBright); dot.drive = 1; //включаем точки
#if DOT_BLINK_TIME != 1000
              _timer_ms[TMR_DOT] = DOT_BLINK_TIME; //установили таймер
#else
              dot.update = 1; //сбросили флаг секунд
#endif
              break;
            case 1: dotSetBright(0); dot.drive = 0; dot.update = 1; break; //выключаем точки
          }
          break;
        case DOT_DOOBLE_BLINK:
          if (dot.count & 0x01) dotSetBright(0); //выключаем точки
          else dotSetBright(dot.maxBright); //включаем точки

          if (++dot.count > 3) {
            dot.count = 0;  //сбрасываем счетчик
            dot.update = 1; //сбросили флаг обновления точек
          }
          else _timer_ms[TMR_DOT] = DOT_DOOBLE_TIME; //установили таймер
          break;
        default: dot.update = 1; break; //сбросили флаг обновления точек
      }
    }
  }
}
//-----------------------------Сброс анимации точек--------------------------------
void dotReset(void) //сброс анимации точек
{
  if (changeAnimState != 1) { //если нужно сбросить анимацию точек
#if DOTS_PORT_ENABLE
    indiClrDots(); //выключаем разделительные точки
#endif
#if (NEON_DOT != 3) || !DOTS_PORT_ENABLE
    dotSetBright(0); //выключаем секундные точки
#endif
    _timer_ms[TMR_DOT] = 0; //сбросили таймер
#if ALARM_TYPE
    if (fastSettings.dotMode > 1 || alarms.dot > 2) //если анимация точек включена
#else
    if (fastSettings.dotMode > 1) //если анимация точек включена
#endif
    {
      dot.update = 1; //установли флаг обновления точек
      dot.drive = 0; //сбросили флаг направления яркости точек
      dot.count = 0; //сбросили счетчик вспышек точек
#if DOTS_PORT_ENABLE
      dot.steps = 0; //сбросили шаги точек
#endif
    }
  }
}
//---------------------------------Режим сна индикаторов---------------------------------
uint8_t sleepIndi(void) //режим сна индикаторов
{
  indiClr(); //очистка индикаторов
  backlAnimDisable(); //запретили эффекты подсветки
  changeBrightDisable(CHANGE_DISABLE); //запретить смену яркости
#if BACKL_TYPE == 3
  clrLeds(); //выключили светодиоды
#elif BACKL_TYPE
  backlSetBright(0); //выключили светодиоды
#endif
#if (RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)) && !RADIO_SLEEP_ENABLE
#if RADIO_SLEEP_ENABLE == 1
  if (indi.sleepMode != SLEEP_DAY) radioPowerOff(); //выключить питание радиоприемника
#else
  radioPowerOff(); //выключить питание радиоприемника
#endif
#endif
  while (!buttonState()) { //если не нажата кнопка
    dataUpdate(); //обработка данных

    if (!secUpd) { //если пришло время обновить индикаторы
      secUpd = 1; //сбрасываем флаг
#if ALARM_TYPE
      if (alarms.now && !alarms.wait) return ALARM_PROGRAM; //тревога будильника
#endif
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
      if (timer.mode == 2 && !timer.count) return WARN_PROGRAM; //тревога таймера
#endif
      if (!indi.sleepMode) return MAIN_PROGRAM; //выход в режим часов
    }
  }
  return MAIN_PROGRAM; //выход в режим часов
}
//------------------------------------Имитация глюков------------------------------------
void glitchIndi(void) //имитация глюков
{
  if (mainSettings.glitchMode) { //если глюки включены
    if (!_timer_sec[TMR_GLITCH] && RTC.s >= GLITCH_PHASE_MIN && RTC.s < GLITCH_PHASE_MAX) { //если пришло время
      boolean indiState = 0; //состояние индикатора
      uint8_t glitchCounter = random(GLITCH_NUM_MIN, GLITCH_NUM_MAX); //максимальное количество глюков
      uint8_t glitchIndic = random(0, LAMP_NUM); //номер индикатора
      uint8_t indiSave = indiGet(glitchIndic); //сохраняем текущую цифру в индикаторе
      _timer_ms[TMR_ANIM] = 0; //сбрасываем таймер
      while (!buttonState()) { //если не нажата кнопка
        dataUpdate(); //обработка данных
#if LAMP_NUM > 4
        if (!indiState || mainSettings.secsMode == SECS_BRIGHT) flipSecs(); //анимация секунд
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
void burnIndi(uint8_t mode, boolean demo) //антиотравление индикаторов
{
  uint8_t indi = 0; //номер индикатора

  if (mode != BURN_SINGLE_TIME) { //если режим без отображения времени
#if DOTS_PORT_ENABLE
    indiClrDots(); //выключаем разделительные точки
#endif
#if (NEON_DOT != 3) || !DOTS_PORT_ENABLE
    dotSetBright(0); //выключаем секундные точки
#endif
  }

  _timer_ms[TMR_MS] = 0; //сбрасываем таймер

  while (1) {
    if (mode == BURN_SINGLE) indiClr(); //очистка индикаторов
    for (uint8_t loops = (demo) ? 1 : BURN_LOOPS; loops; loops--) {
      if (mode == BURN_SINGLE_TIME) {
        indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
        indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
#if LAMP_NUM > 4
        indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
#endif
      }
      for (uint8_t digit = 0; digit < 10; digit++) {
        switch (mode) {
          case BURN_ALL:
            for (indi = 0; indi < LAMP_NUM; indi++) indiPrintNum(cathodeMask[digit], indi); //отрисовываем цифру
            break;
          case BURN_SINGLE:
          case BURN_SINGLE_TIME:
            indiPrintNum(cathodeMask[digit], indi); //отрисовываем цифру
            break;
        }
        for (_timer_ms[TMR_MS] = BURN_TIME; _timer_ms[TMR_MS] && !buttonState();) dataUpdate(); //ждем
      }
    }
    if (mode == BURN_ALL || (++indi >= LAMP_NUM)) return;
  }
}
//-------------------------------Анимация секунд-----------------------------------
#if LAMP_NUM > 4
void flipSecs(void) //анимация секунд
{
  switch (mainSettings.secsMode) {
    case SECS_BRIGHT: //плавное угасание и появление
      if (animShow == ANIM_SECS) { //если сменились секунды
        animShow = 0; //сбрасываем флаг анимации цифр
        _timer_ms[TMR_MS] = 0; //сбрасываем таймер
        anim.timeBright = SECONDS_ANIM_1_TIME / indi.maxBright; //расчёт шага яркости режима 2
        anim.flipSeconds = (RTC.s) ? (RTC.s - 1) : 59; //предыдущая секунда
        anim.flipBuffer[0] = 0; //сбросили флаг
        anim.flipBuffer[1] = indi.maxBright; //установили максимальную яркость
        anim.flipBuffer[2] = anim.flipSeconds % 10; //старые секунды
        anim.flipBuffer[3] = anim.flipSeconds / 10; //старые секунды
        anim.flipSeconds = 0; //сбросили разряды анимации
        if (anim.flipBuffer[2] != (RTC.s % 10)) anim.flipSeconds = 5;
        if (anim.flipBuffer[3] != (RTC.s / 10)) anim.flipSeconds = 4;
      }
      if (anim.flipSeconds && !_timer_ms[TMR_MS]) { //если анимация активна и пришло время
        _timer_ms[TMR_MS] = anim.timeBright; //установили таймер
        if (!anim.flipBuffer[0]) { //если режим уменьшения яркости
          if (anim.flipBuffer[1]) {
            anim.flipBuffer[1]--;
            indiSetBright(anim.flipBuffer[1], anim.flipSeconds, 6); //уменьшение яркости
          }
          else {
            anim.flipBuffer[0] = 1; //перешли к разгоранию
            indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
          }
        }
        else { //иначе режим увеличения яркости
          if (anim.flipBuffer[1] < indi.maxBright) {
            anim.flipBuffer[1]++;
            indiSetBright(anim.flipBuffer[1], anim.flipSeconds, 6); //увеличение яркости
          }
          else anim.flipSeconds = 0; //сбросили разряды анимации
        }
      }
      break;
    case SECS_ORDER_OF_NUMBERS: //перемотка по порядку числа
    case SECS_ORDER_OF_CATHODES: //перемотка по порядку катодов в лампе
      if (animShow == ANIM_SECS) { //если сменились секунды
        animShow = 0; //сбрасываем флаг анимации цифр
        _timer_ms[TMR_MS] = 0; //сбрасываем таймер
        anim.flipSeconds = (RTC.s) ? (RTC.s - 1) : 59; //предыдущая секунда
        anim.flipBuffer[0] = anim.flipSeconds % 10; //старые секунды
        anim.flipBuffer[1] = anim.flipSeconds / 10; //старые секунды
        anim.flipBuffer[2] = RTC.s % 10; //новые секунды
        anim.flipBuffer[3] = RTC.s / 10; //новые секунды
        anim.flipSeconds = 0x03; //устанавливаем флаги анимации

        if (mainSettings.secsMode == SECS_ORDER_OF_CATHODES) {
          for (uint8_t i = 0; i < 4; i++) {
            for (uint8_t c = 0; c < 10; c++) {
              if (cathodeMask[c] == anim.flipBuffer[i]) {
                anim.flipBuffer[i] = c;
                break;
              }
            }
          }
        }
      }
      if (anim.flipSeconds && !_timer_ms[TMR_MS]) { //если анимация активна и пришло время
        _timer_ms[TMR_MS] = (mainSettings.secsMode == SECS_ORDER_OF_NUMBERS) ? SECONDS_ANIM_2_TIME : SECONDS_ANIM_3_TIME; //установили таймер
        for (uint8_t i = 0; i < 2; i++) { //перебираем все цифры
          if (anim.flipBuffer[i] != anim.flipBuffer[i + 2]) { //если не достигли конца анимации разряда
            if (--anim.flipBuffer[i] > 9) anim.flipBuffer[i] = 9; //меняем цифру разряда
          }
          else anim.flipSeconds &= ~(0x01 << i); //иначе завершаем анимацию для разряда
          indiPrintNum((mainSettings.secsMode == SECS_ORDER_OF_NUMBERS) ? anim.flipBuffer[i] : cathodeMask[anim.flipBuffer[i]], (LAMP_NUM - 1) - i); //вывод секунд
        }
      }
      break;
  }
}
#endif
//----------------------Обновить буфер анимации текущего времени--------------------
void animUpdateTime(void) //обновить буфер анимации текущего времени
{
  animPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
  animPrintNum(RTC.m, 2, 2, 0); //вывод минут
#if LAMP_NUM > 4
  animPrintNum(RTC.s, 4, 2, 0); //вывод секунд
#endif
}
//----------------------------------Анимация цифр-----------------------------------
void animIndi(uint8_t mode, uint8_t type) //анимация цифр
{
  switch (mode) {
    case 0: if (type == FLIP_NORMAL) animPrintBuff(0, 6, LAMP_NUM); animShow = 0; return;  //без анимации
    case 1: if (type == FLIP_DEMO) return; else mode = pgm_read_byte(&_anim_set[random(0, sizeof(_anim_set))]); break; //случайный режим
  }

  mode -= 2; //установили режим
  flipIndi(mode, type); //перешли в отрисовку анимации
  if (mode == FLIP_BRIGHT) indiSetBright(indi.maxBright); //возвращаем максимальную яркость

  animPrintBuff(0, 6, LAMP_NUM); //отрисовали буфер
  animShow = 0; //сбрасываем флаг анимации
}
//----------------------------------Анимация цифр-----------------------------------
void flipIndi(uint8_t mode, uint8_t type) //анимация цифр
{
  uint8_t changeBuffer[6]; //буфер анимаций
  uint8_t changeIndi = 0; //флаги разрядов
  uint8_t changeCnt = 0; //счетчик шагов
  uint8_t changeNum = 0; //счетчик разрядов

  if (type != FLIP_NORMAL) animUpdateTime(); //обновили буфер анимации текущего времени
  if (type == FLIP_DEMO) { //если режим демонстрации
    indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
    indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
#if LAMP_NUM > 4
    indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
#endif
  }

  for (uint8_t i = 0; i < LAMP_NUM; i++) anim.flipBuffer[i] = indiGet(i);

  switch (mode) {
    case FLIP_BRIGHT:
    case FLIP_TRAIN:
    case FLIP_GATES:
      if (mode != FLIP_BRIGHT) changeIndi = (mode != FLIP_TRAIN) ? 1 : FLIP_MODE_5_STEP; //перешли к второму этапу
      else anim.timeBright = FLIP_MODE_2_TIME / indi.maxBright; //расчёт шага яркости
      for (uint8_t i = 0; i < LAMP_NUM; i++) {
        if (anim.flipBuffer[i] != INDI_NULL) {
          changeCnt = indi.maxBright; //установили максимальную яркость
          changeIndi = 0; //перешли к второму этапу
          break; //продолжаем
        }
      }
      break;
    case FLIP_ORDER_OF_NUMBERS:
    case FLIP_ORDER_OF_CATHODES:
    case FLIP_SLOT_MACHINE:
      for (uint8_t i = 0; i < LAMP_NUM; i++) {
        anim.flipBuffer[i] = animDecodeNum(anim.flipBuffer[i]);
        changeBuffer[i] = animDecodeNum(anim.flipBuffer[i + 6]);
        if (type == FLIP_DEMO) anim.flipBuffer[i]--;
        else if ((anim.flipBuffer[i] != changeBuffer[i]) || (mode == FLIP_SLOT_MACHINE)) {
          if (changeBuffer[i] == 10) changeBuffer[i] = (anim.flipBuffer[i]) ? (anim.flipBuffer[i] - 1) : 9;
          if (anim.flipBuffer[i] == 10) anim.flipBuffer[i] = (changeBuffer[i]) ? (changeBuffer[i] - 1) : 9;
        }
        if (mode == FLIP_ORDER_OF_CATHODES) {
          for (uint8_t i = 0; i < LAMP_NUM; i++) {
            for (uint8_t c = 0; c < 10; c++) {
              if (cathodeMask[c] == anim.flipBuffer[i]) {
                anim.flipBuffer[i] = c;
                break;
              }
            }
            for (uint8_t c = 0; c < 10; c++) {
              if (cathodeMask[c] == changeBuffer[i]) {
                changeBuffer[i] = c;
                break;
              }
            }
          }
        }
      }
      break;
  }

  _timer_ms[TMR_MS] = FLIP_TIMEOUT; //устанавливаем таймер
  _timer_ms[TMR_ANIM] = 0; //сбрасываем таймер

  while (!buttonState()) {
    dataUpdate(); //обработка данных

    if (type != FLIP_NORMAL) { //если анимация времени
      if (!secUpd) { //если пришло время обновить индикаторы
        secUpd = 1; //сбрасываем флаг
        animUpdateTime(); //обновляем буфер анимации текущего времени
        switch (mode) { //режим анимации перелистывания
          case FLIP_RUBBER_BAND: if (changeCnt) animPrintBuff(LAMP_NUM - changeNum, (LAMP_NUM + 6) - changeNum, changeNum); break; //вывод часов
          case FLIP_HIGHLIGHTS:
          case FLIP_EVAPORATION:
            if (changeCnt || (mode == FLIP_HIGHLIGHTS)) for (uint8_t f = 0; f < changeNum; f++) indiSet(anim.flipBuffer[6 + changeBuffer[f]], changeBuffer[f]); //вывод часов
            break;
          case FLIP_SLOT_MACHINE:
            animPrintBuff(0, 6, changeNum); //вывод часов
            for (uint8_t f = changeNum; f < LAMP_NUM; f++) changeBuffer[f] = animDecodeNum(anim.flipBuffer[f + 6]);
            break;
        }
      }
    }

    if (!_timer_ms[TMR_MS]) break; //выходим если тайм-аут
    if (!_timer_ms[TMR_ANIM]) { //если таймер истек

      switch (mode) { //режим анимации перелистывания
        case FLIP_BRIGHT: { //плавное угасание и появление
            if (!changeIndi) { //если режим уменьшения яркости
              if (changeCnt) {
                changeCnt--;
                if (type != FLIP_DEMO) animBright(changeCnt);
                else indiSetBright(changeCnt); //уменьшение яркости
              }
              else {
                animPrintBuff(0, 6, LAMP_NUM); //вывод буфера
                changeIndi = 1; //перешли к разгоранию
              }
            }
            else { //иначе режим увеличения яркости
              if (changeCnt < indi.maxBright) {
                changeCnt++;
                if (type != FLIP_DEMO) animBright(changeCnt);
                else indiSetBright(changeCnt); //увеличение яркости
              }
              else return; //выходим
            }
            _timer_ms[TMR_ANIM] = anim.timeBright; //устанавливаем таймер
          }
          break;
        case FLIP_ORDER_OF_NUMBERS:  //перемотка по порядку числа
        case FLIP_ORDER_OF_CATHODES: { //перемотка по порядку катодов в лампе
            changeIndi = LAMP_NUM; //загрузили буфер
            for (uint8_t i = 0; i < LAMP_NUM; i++) {
              if (anim.flipBuffer[i] != changeBuffer[i]) { //если не достигли конца анимации разряда
                if (--anim.flipBuffer[i] > 9) anim.flipBuffer[i] = 9; //меняем цифру разряда
                indiPrintNum((mode != FLIP_ORDER_OF_CATHODES) ? anim.flipBuffer[i] : cathodeMask[anim.flipBuffer[i]], i);
              }
              else {
                if (anim.flipBuffer[i + 6] == INDI_NULL) indiClr(i); //очистка индикатора
                changeIndi--; //иначе завершаем анимацию для разряда
              }
            }
            if (!changeIndi) return; //выходим
            _timer_ms[TMR_ANIM] = (mode != FLIP_ORDER_OF_CATHODES) ? FLIP_MODE_3_TIME : FLIP_MODE_4_TIME; //устанавливаем таймер
          }
          break;
        case FLIP_TRAIN: { //поезд
            if (changeIndi < (LAMP_NUM + FLIP_MODE_5_STEP - 1)) {
              indiClr(); //очистка индикатора
              animPrintBuff(changeIndi + 1, 0, LAMP_NUM);
              animPrintBuff(changeIndi - (LAMP_NUM + FLIP_MODE_5_STEP - 1), 6, LAMP_NUM);
              changeIndi++;
              _timer_ms[TMR_ANIM] = FLIP_MODE_5_TIME; //устанавливаем таймер
            }
            else return; //выходим
          }
          break;
        case FLIP_RUBBER_BAND: { //резинка
            if (changeCnt < 2) {
              if (changeNum < LAMP_NUM) {
                switch (changeCnt) {
                  case 0:
                    for (uint8_t b = changeNum + 1; b > 0; b--) {
                      if ((b - 1) == (changeNum - changeIndi)) indiSet(anim.flipBuffer[(LAMP_NUM - 1) - changeNum], LAMP_NUM - b); //вывод часов
                      else indiClr(LAMP_NUM - b); //очистка индикатора
                    }
                    if (changeIndi++ >= changeNum) {
                      changeIndi = 0; //сбрасываем позицию индикатора
                      changeNum++; //прибавляем цикл
                    }
                    break;
                  case 1:
                    for (uint8_t b = 0; b < (LAMP_NUM - changeNum); b++) {
                      if (b == changeIndi) indiSet(anim.flipBuffer[((LAMP_NUM + 6) - 1) - changeNum], b); //вывод часов
                      else indiClr(b); //очистка индикатора
                    }
                    if (changeIndi++ >= (LAMP_NUM - 1) - changeNum) {
                      changeIndi = 0; //сбрасываем позицию индикатора
                      changeNum++; //прибавляем цикл
                    }
                    break;
                }
                if (anim.flipBuffer[((LAMP_NUM - 1) - changeNum) + (changeCnt * 6)] != INDI_NULL) _timer_ms[TMR_ANIM] = FLIP_MODE_6_TIME; //устанавливаем таймер
              }
              else {
                changeCnt++; //прибавляем цикл
                changeNum = 0; //сбросили счетчик
              }
            }
            else return; //выходим
          }
          break;
        case FLIP_GATES: { //ворота
            if (changeIndi < 2) {
              if (changeNum < ((LAMP_NUM / 2) + 1)) {
                indiClr(); //очистка индикатора
                if (!changeIndi) {
                  animPrintBuff(-changeNum, 0, (LAMP_NUM / 2));
                  animPrintBuff(changeNum + (LAMP_NUM / 2), (LAMP_NUM / 2), (LAMP_NUM / 2));
                }
                else {
                  animPrintBuff(changeNum - (LAMP_NUM / 2), 6, (LAMP_NUM / 2));
                  animPrintBuff(LAMP_NUM - changeNum, 6 + (LAMP_NUM / 2), (LAMP_NUM / 2));
                }
                changeNum++; //прибавляем цикл
                _timer_ms[TMR_ANIM] = FLIP_MODE_7_TIME; //устанавливаем таймер
              }
              else {
                changeIndi++; //прибавляем цикл
                changeNum = 0; //сбросили счетчик
              }
            }
            else return; //выходим
          }
          break;
        case FLIP_WAVE: { //волна
            if (changeCnt < 2) {
              if (changeNum < LAMP_NUM) {
                switch (changeCnt) {
                  case 0: indiClr((LAMP_NUM - 1) - changeNum); break; //очистка индикатора
                  case 1: animPrintBuff((LAMP_NUM - 1) - changeNum, (LAMP_NUM + 5) - changeNum, changeNum + 1); break; //вывод часов
                }
                if (anim.flipBuffer[changeNum + (changeCnt * 6)] != INDI_NULL) _timer_ms[TMR_ANIM] = FLIP_MODE_8_TIME; //устанавливаем таймер
                changeNum++; //прибавляем цикл
              }
              else {
                changeCnt++; //прибавляем цикл
                changeNum = 0; //сбросили счетчик
              }
            }
            else return; //выходим
          }
          break;
        case FLIP_HIGHLIGHTS: { //блики
            if (changeNum < LAMP_NUM) {
              changeIndi = random(0, LAMP_NUM);
              if (changeCnt < 2) {
                for (uint8_t b = 0; b < changeNum; b++) {
                  while (changeBuffer[b] == changeIndi) {
                    changeIndi = random(0, LAMP_NUM);
                    b = 0;
                  }
                }
                changeBuffer[changeNum] = changeIndi;
                switch (changeCnt) {
                  case 0: indiClr(changeIndi); break; //очистка индикатора
                  case 1:
                    indiSet(anim.flipBuffer[6 + changeIndi], changeIndi); //вывод часов
                    changeNum++; //прибавляем цикл
                    break; //вывод часов
                }
                changeCnt++; //прибавляем цикл
                if (anim.flipBuffer[changeIndi + (changeCnt * 6)] != INDI_NULL) _timer_ms[TMR_ANIM] = FLIP_MODE_9_TIME; //устанавливаем таймер
              }
              else changeCnt = 0; //сбросили счетчик
            }
            else return; //выходим
          }
          break;
        case FLIP_EVAPORATION: { //испарение
            if (changeCnt < 2) {
              changeIndi = random(0, LAMP_NUM);
              if (changeNum < LAMP_NUM) {
                for (uint8_t b = 0; b < changeNum; b++) {
                  while (changeBuffer[b] == changeIndi) {
                    changeIndi = random(0, LAMP_NUM);
                    b = 0;
                  }
                }
                changeBuffer[changeNum] = changeIndi;
                switch (changeCnt) {
                  case 0: indiClr(changeIndi); break; //очистка индикатора
                  case 1:
                    indiSet(anim.flipBuffer[6 + changeIndi], changeIndi); //вывод часов
                    break;
                }
                changeNum++; //прибавляем цикл
                if (anim.flipBuffer[changeIndi + (changeCnt * 6)] != INDI_NULL) _timer_ms[TMR_ANIM] = FLIP_MODE_10_TIME; //устанавливаем таймер
              }
              else {
                changeCnt++; //прибавляем цикл
                changeNum = 0; //сбросили счетчик
              }
            }
            else return; //выходим
          }
          break;

        case FLIP_SLOT_MACHINE: { //игровой автомат
            if (changeNum < LAMP_NUM) {
              for (uint8_t b = changeNum; b < LAMP_NUM; b++) {
                if (--anim.flipBuffer[b] > 9) anim.flipBuffer[b] = 9; //меняем цифру разряда
                indiPrintNum(anim.flipBuffer[b], b); //выводим разряд
              }
              if (anim.flipBuffer[changeNum] == changeBuffer[changeNum]) { //если разряд совпал
                if (anim.flipBuffer[changeNum + 6] == INDI_NULL) indiClr(changeNum); //очистка индикатора
                changeIndi += FLIP_MODE_11_STEP; //добавили шаг
                changeNum++; //завершаем анимацию для разряда
              }
              _timer_ms[TMR_ANIM] = (uint16_t)FLIP_MODE_11_TIME + changeIndi; //устанавливаем таймер
            }
            else return; //выходим
          }
          break;
      }
    }
  }
}
//-----------------------------Главный экран------------------------------------------------
uint8_t mainScreen(void) //главный экран
{
  if (animShow < ANIM_MAIN) animShow = 0; //сбрасываем флаг анимации цифр
  else if (animShow >= ANIM_OTHER) animIndi(animShow - ANIM_OTHER, FLIP_TIME); //анимация цифр

  if (indi.sleepMode) { //если активен режим сна
    if (!changeAnimState) _timer_sec[TMR_SLEEP] = mainSettings.timeSleep[indi.sleepMode - 1]; //установли время ожидания режима пробуждения
    else if (_timer_sec[TMR_SLEEP] < RESET_TIME_SLEEP) _timer_sec[TMR_SLEEP] = RESET_TIME_SLEEP; //установли минимальное время ожидания режима пробуждения
  }

  if (_timer_sec[TMR_GLITCH] < RESET_TIME_GLITCH) _timer_sec[TMR_GLITCH] = RESET_TIME_GLITCH; //если время вышло то устанавливаем минимальное время
  if (_timer_sec[TMR_BURN] < RESET_TIME_BURN) _timer_sec[TMR_BURN] = RESET_TIME_BURN; //если время вышло то устанавливаем минимальное время
  if (_timer_sec[TMR_TEMP] < RESET_TIME_TEMP) _timer_sec[TMR_TEMP] = RESET_TIME_TEMP; //если время вышло то устанавливаем минимальное время

#if LAMP_NUM > 4
  anim.flipSeconds = 0; //сбрасываем флаги анимации секунд
#endif
  changeAnimState = 0; //сбрасываем флаг установки таймера сна

  for (;;) { //основной цикл
    dataUpdate(); //обработка данных

    if (!secUpd) { //если пришло время обновить индикаторы
#if ALARM_TYPE
      if (alarms.now && !alarms.wait) return ALARM_PROGRAM; //тревога будильника
#endif
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
      if (timer.mode == 2 && !timer.count) return WARN_PROGRAM; //тревога таймера
#endif

      if (indi.sleepMode != SLEEP_NIGHT) { //если режим сна не ночной
        if (!_timer_sec[TMR_BURN]) { //если пришло время отобразить анимацию антиотравления
#if BURN_BRIGHT
          changeBrightDisable(CHANGE_INDI_BLOCK); //запретить смену яркости индикаторов
          indiSetBright(BURN_BRIGHT); //установка общей яркости индикаторов
#endif
          if (mainSettings.burnMode != BURN_SINGLE_TIME) mainTask = SLEEP_PROGRAM; //подмена текущей программы
          burnIndi(mainSettings.burnMode, BURN_NORMAL); //антиотравление индикаторов
          _timer_sec[TMR_BURN] = getPhaseTime(BURN_PERIOD, BURN_PHASE); //установка таймера антиотравления
          if (mainSettings.burnMode != BURN_SINGLE_TIME) changeAnimState = 2; //установили тип анимации
          else changeAnimState = 1; //установили тип анимации
          return MAIN_PROGRAM; //перезапуск основной программы
        }

        if (mainSettings.autoTempTime && !_timer_sec[TMR_TEMP]) { //если пришло время отобразить температуру
          mainTask = SLEEP_PROGRAM; //подмена текущей программы
          autoShowTemp(); //автоматическое отображение температуры
          _timer_sec[TMR_TEMP] = getPhaseTime(mainSettings.autoTempTime, AUTO_TEMP_PHASE); //установка таймера показа температуры
          changeAnimState = 2; //установили тип анимации
          return MAIN_PROGRAM; //перезапуск основной программы
        }

        if (animShow >= ANIM_MINS) animIndi(fastSettings.flipMode, FLIP_TIME); //анимация минут
      }
      else animShow = 0; //сбрасываем флаг анимации
      if (indi.sleepMode && !_timer_sec[TMR_SLEEP]) return SLEEP_PROGRAM; //режим сна индикаторов

      indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
      indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
#if LAMP_NUM > 4
      if (animShow != ANIM_SECS) indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
#endif
      secUpd = 1; //сбрасываем флаг

      glitchIndi(); //имитация глюков
    }

#if LAMP_NUM > 4
    flipSecs(); //анимация секунд
#endif

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        return TEMP_PROGRAM; //показать температуру

#if ALARM_TYPE
      case LEFT_KEY_HOLD: //удержание левой кнопки
        return ALARM_SET_PROGRAM; //настройка будильника
#endif

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        return DATE_PROGRAM; //показать дату

      case RIGHT_KEY_HOLD: //удержание правой кнопки
        return CLOCK_SET_PROGRAM; //иначе настройки времени

      case SET_KEY_PRESS: //клик средней кнопкой
        return FAST_SET_PROGRAM; //переключение настроек

      case SET_KEY_HOLD: //удержание средней кнопки
#if ALARM_TYPE
        if (alarms.wait) {
          alarmDisable(); //отключение будильника
          break;
        }
#endif
        return MAIN_SET_PROGRAM; //настроки основные

#if BTN_ADD_TYPE || IR_PORT_ENABLE
#if TIMER_ENABLE
      case ADD_KEY_PRESS: //клик дополнительной кнопкой
        return TIMER_PROGRAM; //таймер-секундомер
#elif RADIO_ENABLE
      case ADD_KEY_PRESS: //клик дополнительной кнопкой
        return RADIO_PROGRAM; //радиоприемник
#endif
      case ADD_KEY_HOLD: //удержание дополнительной кнопки
#if ALARM_TYPE
        if (alarms.wait) {
          alarmDisable(); //отключение будильника
          break;
        }
#endif
#if RADIO_ENABLE && TIMER_ENABLE
        return RADIO_PROGRAM; //радиоприемник
#else
        break;
#endif

#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
      case PWR_KEY_PRESS: //управление питанием
        if (getPowerStatusRDA() != RDA_ERROR) { //если радиоприемник доступен
          if (getPowerStatusRDA() == RDA_OFF) { //если радио выключено
            radioPowerOn(); //включить питание радиоприемника
            radio.powerState = RDA_ON; //установили флаг питания радио
          }
          else {
            setPowerRDA(RDA_OFF); //выключаем радио
            radio.powerState = RDA_OFF; //сбросили флаг питания радио
          }
        }
        break;
      case VOL_UP_KEY_PRESS: //прибавить громкость
        if (radioFastSettings(VOL_UP_KEY_PRESS)) { //если настройка изменилась
          updateData((uint8_t*)&radioSettings, sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO); //записываем настройки радио в память
          return MAIN_PROGRAM; //выходим
        }
        break;
      case VOL_DOWN_KEY_PRESS: //убавить громкость
        if (radioFastSettings(VOL_DOWN_KEY_PRESS)) { //если настройка изменилась
          updateData((uint8_t*)&radioSettings, sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO); //записываем настройки радио в память
          return MAIN_PROGRAM; //выходим
        }
        break;
      case STATION_UP_KEY_PRESS: //следующая станция
        if (radioFastSettings(STATION_UP_KEY_PRESS)) { //если настройка изменилась
          updateData((uint8_t*)&radioSettings, sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO); //записываем настройки радио в память
          return MAIN_PROGRAM; //выходим
        }
        break;
      case STATION_DOWN_KEY_PRESS: //предыдущая станция
        if (radioFastSettings(STATION_DOWN_KEY_PRESS)) { //если настройка изменилась
          updateData((uint8_t*)&radioSettings, sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO); //записываем настройки радио в память
          return MAIN_PROGRAM; //выходим
        }
        break;
#endif
#endif
    }
  }
  return INIT_PROGRAM;
}
