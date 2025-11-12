/*
  Arduino IDE 1.8.13 версия прошивки 2.2.8 релиз от 18.09.25
  Универсальная прошивка для различных проектов часов на ГРИ под 4/6 ламп
  Страница прошивки на форуме - https://community.alexgyver.ru/threads/chasy-na-gri-alternativnaja-proshivka.5843/

  Исходник - https://github.com/radon-lab/NixieClock
  Автор Radon-lab.
*/
//-----------------Ошибки-----------------
enum {
  DS3231_ERROR,        //0001 - нет связи с модулем DS3231
  DS3231_OSF_ERROR,    //0002 - сбой осцилятора модуля DS3231
  SQW_SHORT_ERROR,     //0003 - слишком короткий сигнал SQW
  SQW_LONG_ERROR,      //0004 - сигнал SQW отсутсвует или слишком длинный сигнал SQW
  TEMP_SENS_ERROR,     //0005 - выбранный датчик температуры не обнаружен
  VCC_ERROR,           //0006 - напряжения питания вне рабочего диапазона
  MEMORY_ERROR,        //0007 - сбой памяти еепром
  RESET_ERROR,         //0008 - софтовая перезагрузка
  CONVERTER_ERROR,     //0009 - сбой работы преобразователя
  PWM_OVF_ERROR,       //0010 - переполнение заполнения шим преобразователя
  STACK_OVF_ERROR,     //0011 - переполнение стека
  TICK_OVF_ERROR,      //0012 - переполнение тиков времени
  INDI_ERROR,          //0013 - сбой работы динамической индикации
  PCF8591_ERROR        //0014 - ошибка сбоя работы PCF8591
};
void systemTask(void); //процедура системной задачи
void SET_ERROR(uint8_t err); //процедура установка ошибки

//-----------------Таймеры------------------
enum {
  TMR_MS,       //таймер общего назначения
  TMR_IR,       //таймер инфракрасного приемника
  TMR_SENS,     //таймер сенсоров температуры
  TMR_PLAYER,   //таймер плеера/мелодий
  TMR_LIGHT,    //таймер сенсора яркости
  TMR_BACKL,    //таймер подсветки
  TMR_COLOR,    //таймер смены цвета подсветки
  TMR_DOT,      //таймер точек
  TMR_ANIM,     //таймер анимаций
  TMR_PCF8591,   //таймер PCF8591
  TIMERS_MS_NUM //количество таймеров
};
uint16_t _timer_ms[TIMERS_MS_NUM]; //таймер отсчета миллисекунд

enum {
  TMR_ALM,       //таймер тайм-аута будильника
  TMR_ALM_WAIT,  //таймер ожидания повторного включения будильника
  TMR_ALM_SOUND, //таймер отключения звука будильника
  TMR_MEM,       //таймер обновления памяти
  TMR_SYNC,      //таймер синхронизации
  TMR_BURN,      //таймер антиотравления
  TMR_SHOW,      //таймер автопоказа
  TMR_GLITCH,    //таймер глюков
  TMR_SLEEP,     //таймер ухода в сон
  TIMERS_SEC_NUM //количество таймеров
};
uint16_t _timer_sec[TIMERS_SEC_NUM]; //таймер отсчета секунд

volatile uint8_t tick_ms;  //счетчик тиков миллисекунд
volatile uint8_t tick_sec; //счетчик тиков от RTC

//----------------Температура--------------
struct sensorData {
  int16_t temp = 0x7FFF; //температура со встроенного сенсора
  uint16_t press; //давление со встроенного сенсора
  uint8_t hum; //влажность со встроенного сенсора
  uint8_t type; //тип встроенного датчика температуры
  boolean init; //флаг инициализации датчика температуры
  boolean update; //флаг обновления датчика температуры
} sens;

enum {
  SENS_DS3231, //датчик DS3231
  SENS_AHT, //датчики AHT
  SENS_SHT, //датчики SHT
  SENS_BME, //датчики BME/BMP
  SENS_DS18, //датчики DS18B20
  SENS_DHT, //датчики DHT
  SENS_ALL //датчиков всего
};

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
#include "AHT.h"
#include "SHT.h"
#include "BME.h"
#include "DHT.h"
#include "DS.h"
#include "IR.h"
#include "WS.h"
#include "INDICATION.h"
#include "PCF8591.h"

//-----------------Настройки----------------
struct Settings_1 {
  uint8_t indiBright[2] = {DEFAULT_INDI_BRIGHT_N, DEFAULT_INDI_BRIGHT}; //яркость индикаторов
  uint8_t backlBright[2] = {DEFAULT_BACKL_BRIGHT_N, DEFAULT_BACKL_BRIGHT}; //яркость подсветки
  uint8_t dotBright[2] = {DEFAULT_DOT_BRIGHT_N, DEFAULT_DOT_BRIGHT}; //яркость точек
  uint8_t timeBright[2] = {DEFAULT_NIGHT_START, DEFAULT_NIGHT_END}; //время перехода яркости
  uint8_t timeHour[2] = {DEFAULT_HOUR_SOUND_START, DEFAULT_HOUR_SOUND_END}; //время звукового оповещения нового часа
  uint8_t timeSleep[2] = {DEFAULT_SLEEP_WAKE_TIME_N, DEFAULT_SLEEP_WAKE_TIME}; //время режима сна
  boolean timeFormat = DEFAULT_TIME_FORMAT; //формат времени
  boolean knockSound = DEFAULT_KNOCK_SOUND; //звук кнопок или озвучка
  uint8_t hourSound = (DEFAULT_HOUR_SOUND_TYPE & 0x03) | ((DEFAULT_HOUR_SOUND_TEMP) ? 0x80 : 0x00); //тип озвучки смены часа
  uint8_t volumeSound = DEFAULT_PLAYER_VOLUME; //громкость озвучки
  uint8_t voiceSound = DEFAULT_VOICE_SOUND; //голос озвучки
  int8_t tempCorrect = DEFAULT_TEMP_CORRECT; //коррекция температуры
  boolean glitchMode = DEFAULT_GLITCH_MODE; //режим глюков
  uint8_t autoShowTime = DEFAULT_AUTO_SHOW_TIME; //интервал времени автопоказа
  uint8_t autoShowFlip = DEFAULT_AUTO_SHOW_ANIM; //режим анимации автопоказа
  uint8_t burnMode = DEFAULT_BURN_MODE; //режим антиотравления индикаторов
  uint8_t burnTime = BURN_PERIOD; //интервал антиотравления индикаторов
} mainSettings;

struct Settings_2 { //быстрые настройки
  uint8_t flipMode = DEFAULT_FLIP_ANIM; //режим анимации минут
  uint8_t secsMode = DEFAULT_SECONDS_ANIM; //режим анимации секунд
  uint8_t dotMode = DEFAULT_DOT_MODE; //режим точек
  uint8_t backlMode = DEFAULT_BACKL_MODE; //режим подсветки
  uint8_t backlColor = (DEFAULT_BACKL_COLOR > 25) ? (DEFAULT_BACKL_COLOR + 227) : (DEFAULT_BACKL_COLOR * 10); //цвет подсветки
  uint8_t neonDotMode = DEFAULT_DOT_EXT_MODE; //режим неоновых точек
} fastSettings;

struct Settings_3 { //настройки радио
  uint8_t volume = DEFAULT_RADIO_VOLUME; //текущая громкость
  uint8_t stationNum = 0; //текущий номер радиостанции из памяти
  uint16_t stationsFreq = RADIO_MIN_FREQ; //текущая частота
  uint16_t stationsSave[RADIO_MAX_STATIONS] = {DEFAULT_RADIO_STATIONS}; //память радиостанций
} radioSettings;


//переменные работы с радио
struct radioData {
  boolean powerState; //текущее состояние радио
  uint8_t seekRun; //флаг автопоиска радио
  uint8_t seekAnim; //анимация автопоиска
  uint16_t seekFreq; //частота автопоиска радио
} radio;

//переменные таймера/секундомера
struct timerData {
  uint8_t mode; //режим таймера/секундомера
  uint16_t count; //счетчик таймера/секундомера
  uint16_t time = TIMER_TIME; //время таймера сек
} timer;

//переменные будильника
struct alarmData {
  uint8_t num; //текущее количество будильников
  uint8_t now; //флаг активоного будильника
  uint8_t sound; //мелодия активоного будильника
  uint8_t radio; //режим радио активоного будильника
  uint8_t volume; //громкость активоного будильника
} alarms;

//переменные работы с точками
struct dotData {
  boolean update; //флаг обновления точек
  boolean drive; //направление яркости
  uint8_t count; //счетчик мигания точки
  uint8_t steps; //шаги точек
  uint8_t maxBright; //максимальная яркость точек
  uint8_t menuBright; //максимальная яркость точек в меню
  uint8_t brightStep; //шаг мигания точек
  uint16_t brightTime; //период шага мигания точек
  uint8_t brightTurnStep; //шаг мигания двойных точек
  uint16_t brightTurnTime; //период шага мигания двойных точек
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
  uint8_t mode_2_step; //шаг эффекта номер 2
  uint16_t mode_2_time; //время эффекта номер 2
  uint8_t mode_4_step; //шаг эффекта номер 4
  uint8_t mode_8_step; //шаг эффекта номер 6
  uint16_t mode_8_time; //время эффекта номер 6
} backl;

//переменные обработки кнопок
struct buttonData {
  uint8_t state; //текущее состояние кнопок
  uint8_t adc; //результат опроса аналоговых кнопок
} btn;
uint8_t analogState; //флаги обновления аналоговых портов
uint16_t vcc_adc; //напряжение питания

//переменные работы с индикаторами
struct indiData {
  boolean update; //флаг обновления индикаторов
  uint8_t sleepMode; //флаг режима сна индикаторов
  uint8_t maxBright; //максимальная яркость индикаторов
} indi;

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

//флаги анимаций
uint8_t changeBrightState; //флаг состояния смены яркости подсветки
uint8_t changeAnimState; //флаг состояния анимаций
uint8_t animShow; //флаг анимации смены времени

#define CONVERT_NUM(x) ((x[0] - 48) * 100 + (x[2] - 48) * 10 + (x[4] - 48)) //преобразовать строку в число
#define CONVERT_CHAR(x) (x - 48) //преобразовать символ в число

#define ALARM_AUTO_VOL_TIMER (uint16_t)(((uint16_t)ALARM_AUTO_VOL_TIME * 1000) / (ALARM_AUTO_VOL_MAX - ALARM_AUTO_VOL_MIN))

#define BTN_GIST_TICK (BTN_GIST_TIME / (US_PERIOD / 1000.0)) //количество циклов для защиты от дребезга
#define BTN_HOLD_TICK (BTN_HOLD_TIME / (US_PERIOD / 1000.0)) //количество циклов после которого считается что кнопка зажата

#if BTN_TYPE
#define BTN_MIN_RANGE 5 //минимальный диапазон аналоговых кнопок
#define BTN_MAX_RANGE 255 //максимальный диапазон аналоговых кнопок

#if BTN_PULL
#define BTN_CHECK_ADC(low, high) (!(((255 - (high)) <= btn.adc) && (btn.adc < (255 - (low))))) //проверка аналоговой кнопки
#else
#define BTN_CHECK_ADC(low, high) (!(((low) < btn.adc) && (btn.adc <= (high)))) //проверка аналоговой кнопки
#endif

#define GET_ADC(low, high) (int16_t)(255.0 / (float)R_COEF(low, high)) //рассчет значения ацп кнопок

#define SET_MIN_ADC (uint8_t)(CONSTRAIN(GET_ADC(BTN_R_LOW, BTN_SET_R_HIGH) - BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE))
#define SET_MAX_ADC (uint8_t)(CONSTRAIN(GET_ADC(BTN_R_LOW, BTN_SET_R_HIGH) + BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE))

#define LEFT_MIN_ADC (uint8_t)(CONSTRAIN(GET_ADC(BTN_R_LOW, BTN_LEFT_R_HIGH) - BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE))
#define LEFT_MAX_ADC (uint8_t)(CONSTRAIN(GET_ADC(BTN_R_LOW, BTN_LEFT_R_HIGH) + BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE))

#define RIGHT_MIN_ADC (uint8_t)(CONSTRAIN(GET_ADC(BTN_R_LOW, BTN_RIGHT_R_HIGH) - BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE))
#define RIGHT_MAX_ADC (uint8_t)(CONSTRAIN(GET_ADC(BTN_R_LOW, BTN_RIGHT_R_HIGH) + BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE))

#define SET_CHK BTN_CHECK_ADC(SET_MIN_ADC, SET_MAX_ADC) //чтение средней аналоговой кнопки
#define LEFT_CHK BTN_CHECK_ADC(LEFT_MIN_ADC, LEFT_MAX_ADC) //чтение левой аналоговой кнопки
#define RIGHT_CHK BTN_CHECK_ADC(RIGHT_MIN_ADC, RIGHT_MAX_ADC) //чтение правой аналоговой кнопки

#if (BTN_ADD_TYPE == 2)
#define ADD_MIN_ADC (uint8_t)(CONSTRAIN(GET_ADC(BTN_R_LOW, BTN_ADD_R_HIGH) - BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE))
#define ADD_MAX_ADC (uint8_t)(CONSTRAIN(GET_ADC(BTN_R_LOW, BTN_ADD_R_HIGH) + BTN_ANALOG_GIST, BTN_MIN_RANGE, BTN_MAX_RANGE))

#define ADD_CHK BTN_CHECK_ADC(ADD_MIN_ADC, ADD_MAX_ADC) //чтение правой аналоговой кнопки
#endif
#endif

//перечисления меню настроек
enum {
  SET_TIME_FORMAT, //формат времени и анимация глюков
  SET_GLITCH_MODE, //режим глюков или громкость и голос озвучки
  SET_BTN_SOUND, //звук кнопок или озвучка смены часа и действий
  SET_HOUR_TIME, //звук смены часа
  SET_BRIGHT_TIME, //время смены яркости
  SET_INDI_BRIGHT, //яркость индикаторов
  SET_BACKL_BRIGHT, //яркость подсветки
  SET_DOT_BRIGHT, //яркость точек
  SET_CORRECT_SENS, //настройка датчика температуры
  SET_AUTO_SHOW, //автопоказ данных
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
  FAST_FLIP_MODE, //режим смены минут
  FAST_SECS_MODE, //режим смены минут
  FAST_DOT_MODE, //режим точек
  FAST_BACKL_MODE, //режим подсветки
  FAST_BACKL_COLOR //цвет подсветки
};

//перечисления режимов автопоказа
enum {
  SHOW_NULL, //показ отключен
  SHOW_DATE, //показ даты
  SHOW_YEAR, //показ года
  SHOW_DATE_YEAR, //показ даты и года
  SHOW_TEMP, //показ температуры
  SHOW_HUM, //показ влажности
  SHOW_PRESS, //показ давления
  SHOW_TEMP_HUM, //показ температуры и влажности
  SHOW_TEMP_ESP, //показ температуры из esp
  SHOW_HUM_ESP, //показ влажности из esp
  SHOW_PRESS_ESP, //показ давления из esp
  SHOW_TEMP_HUM_ESP //показ температуры и влажности из esp
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
#if BACKL_TYPE == 3
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
#endif
  BACKL_EFFECT_NUM //максимум эффектов подсветки
};

//перечисления режимов точек
enum {
  DOT_OFF, //выключена
  DOT_STATIC, //статичная
  DOT_MAIN_BLINK, //мигание раз в секунду
  DOT_MAIN_DOOBLE_BLINK, //мигание два раза в секунду
#if NEON_DOT != 3
  DOT_MAIN_PULS, //плавно мигает
#endif
#if NEON_DOT == 2
  DOT_MAIN_TURN_BLINK, //мигание неоновых ламп раз в секунду по очереди
  DOT_MAIN_TURN_PULS, //мигание неоновых ламп плавно по очереди
#endif
#if DOTS_PORT_ENABLE
  DOT_BLINK, //одиночное мигание
  DOT_RUNNING, //бегущая
  DOT_SNAKE, //змейка
  DOT_RUBBER_BAND, //резинка
#if (DOTS_NUM > 4) || (DOTS_TYPE == 2)
  DOT_TURN_BLINK, //мигание одной точки по очереди
#endif
#if (DOTS_NUM > 4) && (DOTS_TYPE == 2)
  DOT_DUAL_TURN_BLINK, //мигание двумя точками по очереди
#endif
#endif
  DOT_EFFECT_NUM //количество эффектов точек
};

//перечисления режимов точек
enum {
  DOT_EXT_OFF, //выключена
  DOT_EXT_STATIC, //статичная
  DOT_EXT_BLINK, //мигание раз в секунду
  DOT_EXT_TURN_BLINK //мигание неоновых ламп раз в секунду по очереди
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
  ALARM_STATUS, //статус будильника
  ALARM_MAX_ARR //максимальное количество данных
};

//перечисления состояний будильника
enum {
  ALARM_DISABLE, //будильники выключены
  ALARM_ENABLE, //будильник включен
  ALARM_WAIT, //будильник в режиме ожидания
  ALARM_WARN //будильник в режиме тревоги
};

//перечисления режимов проверок будильника
enum {
  ALARM_CHECK_MAIN, //основной режим проверки будильников
  ALARM_CHECK_SET, //проверка на установленный будильник
  ALARM_CHECK_INIT //проверка будильников при запуске
};

//перечисления системных звуков
enum {
  SOUND_TEST_SPEAKER, //звук ошибки ввода пароля
  SOUND_PASS_ERROR, //звук ошибки ввода пароля
  SOUND_RESET_SETTINGS, //звук сброса настроек
  SOUND_ALARM_DISABLE, //звук отключения будильника
  SOUND_ALARM_WAIT, //звук ожидания будильника
  SOUND_TIMER_WARN, //звук окончания таймера
  SOUND_HOUR //звук смены часа
};

//перечисления режимов воспроизведения мелодий
enum {
  REPLAY_STOP, //остановить воспроизведение
  REPLAY_ONCE, //проиграть один раз
  REPLAY_CYCLE //проиграть по кругу
};

//перечисления режимов озвучки температуры
enum {
  SPEAK_TEMP_MAIN, //озвучка основной температуры
  SPEAK_TEMP_HOUR //озвучка температуры при смене часа
};

//перечисления режимов анимации времени
enum {
  ANIM_NULL, //нет анимации
  ANIM_SECS, //запуск анимации секунд
  ANIM_MINS, //запуск анимации минут
  ANIM_MAIN, //запуск основной анимации
  ANIM_DEMO, //запуск демострации анимации
  ANIM_OTHER //запуск иной анимации
};

//перечисления типа сброса режимов анимации точек
enum {
  ANIM_RESET_ALL, //сброс всех анимаций
  ANIM_RESET_CHANGE, //сброс только если анимация изменилась
  ANIM_RESET_DOT //сброс анимации точек
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

struct Settings_4 { //расширенные настройки
  uint8_t autoShowModes[5] = {AUTO_SHOW_MODES};
  uint8_t autoShowTimes[5] = {AUTO_SHOW_TIMES};
  uint8_t alarmTime = ALARM_TIMEOUT;
  uint8_t alarmWaitTime = ALARM_WAIT_TIME;
  uint8_t alarmSoundTime = ALARM_SOUND_TIME;
  uint8_t alarmDotOn = ((!ALARM_ON_BLINK_DOT) ? (DOT_EFFECT_NUM) : (ALARM_ON_BLINK_DOT));
  uint8_t alarmDotWait = ((!ALARM_WAIT_BLINK_DOT) ? (DOT_EFFECT_NUM) : (ALARM_WAIT_BLINK_DOT));
  uint8_t tempCorrectSensor = SHOW_TEMP_CORRECT_MODE;
  uint8_t tempMainSensor = SHOW_TEMP_MAIN_SENS;
  uint8_t tempHourSensor = HOUR_SOUND_MAIN_SENS;
} extendedSettings;

const uint8_t deviceInformation[] = { //комплектация часов
  CONVERT_CHAR(FIRMWARE_VERSION[0]),
  CONVERT_CHAR(FIRMWARE_VERSION[2]),
  CONVERT_CHAR(FIRMWARE_VERSION[4]),
  HARDWARE_VERSION,
  ((DS3231_ENABLE == 2) | SENS_AHT_ENABLE | SENS_SHT_ENABLE | SENS_BME_ENABLE | SENS_PORT_ENABLE),
  BTN_EASY_MAIN_MODE,
  LAMP_NUM,
  BACKL_TYPE,
  NEON_DOT,
  DOTS_PORT_ENABLE,
  DOTS_NUM,
  DOTS_TYPE,
  LIGHT_SENS_ENABLE,
  (BTN_ADD_TYPE | IR_PORT_ENABLE),
  DS3231_ENABLE,
  TIMER_ENABLE,
  RADIO_ENABLE,
  ALARM_TYPE,
  PLAYER_TYPE,
#if PLAYER_TYPE
  PLAYER_ALARM_MAX,
#else
  SOUND_MAX(alarm_sound),
#endif
  PLAYER_VOICE_MAX,
  ALARM_AUTO_VOL_MAX
};

//переменные работы с температурой
struct mainSensorData {
  int16_t temp = 0x7FFF; //температура
  uint16_t press; //давление
  uint8_t hum; //влажность
} mainSens;

//переменные работы с устройством
struct deviceData {
  uint8_t light; //яркость подсветки часов
  uint8_t status; //флаги состояния часов
  uint16_t failure; //сбои при запуске часов
} device;

//переменные работы с шиной
struct busData {
  uint8_t position; //текущая позиция
  uint8_t counter; //счетчик байт
  uint8_t comand; //текущая команда
  uint8_t status; //статус шины
  uint8_t statusExt; //статус шины
  uint8_t buffer[10]; //буфер шины
} bus;

#define BUS_WAIT_DATA 0x00

#define BUS_WRITE_TIME 0x01
#define BUS_READ_TIME 0x02

#define BUS_WRITE_FAST_SET 0x03
#define BUS_READ_FAST_SET 0x04

#define BUS_WRITE_MAIN_SET 0x05
#define BUS_READ_MAIN_SET 0x06

#define BUS_READ_ALARM_NUM 0x07
#define BUS_WRITE_SELECT_ALARM 0x08
#define BUS_READ_SELECT_ALARM 0x09
#define BUS_WRITE_ALARM_DATA 0x0A
#define BUS_READ_ALARM_DATA 0x0B
#define BUS_DEL_ALARM 0x0C
#define BUS_NEW_ALARM 0x0D

#define BUS_READ_RADIO_SET 0x0E
#define BUS_WRITE_RADIO_STA 0x0F
#define BUS_WRITE_RADIO_VOL 0x10
#define BUS_WRITE_RADIO_FREQ 0x11
#define BUS_WRITE_RADIO_MODE 0x12
#define BUS_WRITE_RADIO_POWER 0x13
#define BUS_SEEK_RADIO_UP 0x14
#define BUS_SEEK_RADIO_DOWN 0x15
#define BUS_READ_RADIO_POWER 0x16

#define BUS_CHECK_TEMP 0x17
#define BUS_READ_TEMP 0x18

#define BUS_WRITE_EXTENDED_SET 0x19
#define BUS_READ_EXTENDED_SET 0x1A

#define BUS_SET_SHOW_TIME 0x1B
#define BUS_SET_BURN_TIME 0x1C
#define BUS_SET_UPDATE 0x1E

#define BUS_WRITE_TIMER_SET 0x1F
#define BUS_READ_TIMER_SET 0x20
#define BUS_WRITE_TIMER_MODE 0x21

#define BUS_WRITE_SENS_DATA 0x22
#define BUS_WRITE_MAIN_SENS_DATA 0x23

#define BUS_READ_FAILURE 0xA0

#define BUS_ALARM_DISABLE 0xDA
#define BUS_CHANGE_BRIGHT 0xDC

#define BUS_TEST_FLIP 0xEA
#define BUS_TEST_SOUND 0xEB
#define BUS_STOP_SOUND 0xEC

#define BUS_CONTROL_DEVICE 0xFA

#define BUS_SELECT_BYTE 0xFD
#define BUS_READ_STATUS 0xFE
#define BUS_READ_DEVICE 0xFF

#define DEVICE_RESET 0xCC
#define DEVICE_UPDATE 0xDD
#define DEVICE_REBOOT 0xEE

#define BOOTLOADER_OK 0xAA
#define BOOTLOADER_START 0xBB
#define BOOTLOADER_FLASH 0xCC

enum {
  BUS_COMMAND_BIT_0,
  BUS_COMMAND_BIT_1,
  BUS_COMMAND_BIT_2,
  BUS_COMMAND_BIT_3,
  BUS_COMMAND_BIT_4,
  BUS_COMMAND_WAIT,
  BUS_COMMAND_UPDATE
};
enum {
  BUS_COMMAND_NULL,
  BUS_COMMAND_RADIO_MODE,
  BUS_COMMAND_RADIO_POWER,
  BUS_COMMAND_RADIO_SEEK_UP,
  BUS_COMMAND_RADIO_SEEK_DOWN,
  BUS_COMMAND_TIMER_MODE
};
enum {
#if DS3231_ENABLE
  BUS_EXT_COMMAND_SEND_TIME,
#endif
#if RADIO_ENABLE
  BUS_EXT_COMMAND_RADIO_VOL,
  BUS_EXT_COMMAND_RADIO_FREQ,
#endif
  BUS_EXT_MAX_DATA
};

enum {
  STATUS_UPDATE_MAIN_SET,
  STATUS_UPDATE_FAST_SET,
  STATUS_UPDATE_RADIO_SET,
  STATUS_UPDATE_EXTENDED_SET,
  STATUS_UPDATE_ALARM_SET,
  STATUS_UPDATE_TIME_SET,
  STATUS_UPDATE_SENS_DATA,
  STATUS_UPDATE_ALARM_STATE
};

enum {
  MEM_UPDATE_MAIN_SET,
  MEM_UPDATE_FAST_SET,
  MEM_UPDATE_RADIO_SET,
  MEM_UPDATE_EXTENDED_SET,
  MEM_MAX_DATA
};
uint8_t memoryUpdate;

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

#define EEPROM_BLOCK_SETTINGS_FAST (EEPROM_BLOCK_NULL + 8) //блок памяти быстрых настроек
#define EEPROM_BLOCK_SETTINGS_MAIN (EEPROM_BLOCK_SETTINGS_FAST + sizeof(fastSettings)) //блок памяти основных настроек
#define EEPROM_BLOCK_SETTINGS_RADIO (EEPROM_BLOCK_SETTINGS_MAIN + sizeof(mainSettings)) //блок памяти настроек радио
#define EEPROM_BLOCK_SETTINGS_DEBUG (EEPROM_BLOCK_SETTINGS_RADIO + sizeof(radioSettings)) //блок памяти настроек отладки
#define EEPROM_BLOCK_SETTINGS_EXTENDED (EEPROM_BLOCK_SETTINGS_DEBUG + sizeof(debugSettings)) //блок памяти расширенных настроек
#define EEPROM_BLOCK_ERROR (EEPROM_BLOCK_SETTINGS_EXTENDED + sizeof(extendedSettings)) //блок памяти ошибок
#define EEPROM_BLOCK_EXT_ERROR (EEPROM_BLOCK_ERROR + 1) //блок памяти расширеных ошибок
#define EEPROM_BLOCK_ALARM (EEPROM_BLOCK_EXT_ERROR + 1) //блок памяти количества будильников

#define EEPROM_BLOCK_CRC_DEFAULT (EEPROM_BLOCK_ALARM + 1) //блок памяти контрольной суммы настроек
#define EEPROM_BLOCK_CRC_FAST (EEPROM_BLOCK_CRC_DEFAULT + 2) //блок памяти контрольной суммы быстрых настроек
#define EEPROM_BLOCK_CRC_MAIN (EEPROM_BLOCK_CRC_FAST + 1) //блок памяти контрольной суммы основных настроек
#define EEPROM_BLOCK_CRC_RADIO (EEPROM_BLOCK_CRC_MAIN + 1) //блок памяти контрольной суммы настроек радио
#define EEPROM_BLOCK_CRC_DEBUG (EEPROM_BLOCK_CRC_RADIO + 1) //блок памяти контрольной суммы настроек отладки
#define EEPROM_BLOCK_CRC_DEBUG_DEFAULT (EEPROM_BLOCK_CRC_DEBUG + 1) //блок памяти контрольной суммы настроек отладки
#define EEPROM_BLOCK_CRC_EXTENDED (EEPROM_BLOCK_CRC_DEBUG_DEFAULT + 1) //блок памяти контрольной суммы расширеных настроек
#define EEPROM_BLOCK_CRC_ERROR (EEPROM_BLOCK_CRC_EXTENDED + 1) //блок контрольной суммы памяти ошибок
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
#if ESP_ENABLE
    busCommand(); //проверка статуса шины
#endif
    dotReset(changeAnimState); //сброс анимации точек
#if BACKL_TYPE
    backlAnimEnable(); //разрешили эффекты подсветки
#endif
    changeBrightEnable(); //разрешить смену яркости
    changeBright(); //установка яркости от времени суток
    secsReset(); //сброс анимации секунд
#if INDI_SYMB_TYPE
    indiClrSymb(); //очистка индикатора символов
#endif

    switch (mainTask) {
      default: RESET_SYSTEM; break; //перезагрузка
      case MAIN_PROGRAM: mainTask = mainScreen(); break; //главный экран
#if !BTN_EASY_MAIN_MODE
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
      case TEMP_PROGRAM: mainTask = showTemp(); break; //показать температуру
#endif
      case DATE_PROGRAM: mainTask = showDate(); break; //показать дату
#endif
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
      case WARN_PROGRAM: mainTask = timerWarn(); break; //предупреждение таймера
#endif
#if ALARM_TYPE
      case ALARM_PROGRAM: //тревога будильника
        mainTask = alarmWarn(); //переход в программу
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
        radioPowerRet(); //вернуть питание радиоприемника
#endif
#if PLAYER_TYPE
        playerSetVolNow(mainSettings.volumeSound); //установили громкость
#endif
        break;
#endif
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
      case RADIO_PROGRAM: mainTask = radioMenu(); break; //радиоприемник
#endif
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
      case TIMER_PROGRAM: mainTask = timerStopwatch(); break; //таймер-секундомер
#endif
      case SLEEP_PROGRAM: //режим сна индикаторов
        mainTask = sleepIndi(); //переход в программу
        setAnimTimers(); //установка таймеров анимаций
        break;
#if !BTN_EASY_MAIN_MODE
      case FAST_SET_PROGRAM: mainTask = fastSetSwitch(); break; //переключение настроек
      case MAIN_SET_PROGRAM: mainTask = settings_main(); break; //основные настроки
#endif
      case CLOCK_SET_PROGRAM: mainTask = settings_time(); break; //настройки времени
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
        checkAlarms(ALARM_CHECK_SET); //проверяем будильники на совпадение
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

#if ESP_ENABLE
  wireDisable(); //отключение шины
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

#if SENS_PORT_ENABLE
  SENS_INIT; //инициализация порта датчиков температуры
#endif

#if !BTN_TYPE
  SET_INIT; //инициализация средней кнопки
  LEFT_INIT; //инициализация левой кнопки
  RIGHT_INIT; //инициализация правой кнопки
#endif

#if BTN_ADD_TYPE == 1
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
  irInit(); //инициализация ик приемника
#endif

#if PLAYER_TYPE == 1
  DF_BUSY_INIT; //инициализация busy
  DF_RX_INIT; //инициализация rx
#elif PLAYER_TYPE == 2
  SD_CS_INIT; //иничиализация CS
  SD_SCK_INIT; //иничиализация SCK
  SD_MISO_INIT; //иничиализация MISO
  SD_MOSI_INIT; //иничиализация MOSI
#endif

#if GEN_ENABLE && (GEN_FEEDBACK == 2)
  FB_INIT; //инициализация обратной связи
  ACSR = (0x01 << ACBG); //включаем компаратор
#endif

  indiPortInit(); //инициализация портов индикаторов

#if ESP_ENABLE
  if (GPIOR0 == BOOTLOADER_START) { //если был запрос начала обновления прошивки
    GPIOR0 = 0x00; //сбросили запрос начала обновления прошивки

    if (EEPROM_ReadByte(EEPROM_BLOCK_BOOT) == BOOTLOADER_OK) { //если загрузчик готов
      EEPROM_UpdateByte(EEPROM_BLOCK_BOOT, BOOTLOADER_START); //устанавливаем флаг запуска загрузчика
      RESET_BOOTLOADER; //переход к загрузчику
    }
  }
#endif

  if (checkByte(EEPROM_BLOCK_ERROR, EEPROM_BLOCK_CRC_ERROR)) updateByte(0x00, EEPROM_BLOCK_ERROR, EEPROM_BLOCK_CRC_ERROR); //если контрольная сумма ошибок не совпала
  if (checkByte(EEPROM_BLOCK_EXT_ERROR, EEPROM_BLOCK_CRC_EXT_ERROR)) updateByte(0x00, EEPROM_BLOCK_EXT_ERROR, EEPROM_BLOCK_CRC_EXT_ERROR); //если контрольная сумма расширеных ошибок не совпала

  checkVCC(); //чтение напряжения питания

  if (checkSettingsCRC() || !SET_CHK) { //если контрольная сумма не совпала или зажата средняя кнопка то восстанавливаем настройеи по умолчанию
    updateData((uint8_t*)&fastSettings, sizeof(fastSettings), EEPROM_BLOCK_SETTINGS_FAST, EEPROM_BLOCK_CRC_FAST); //записываем настройки яркости в память
    updateData((uint8_t*)&mainSettings, sizeof(mainSettings), EEPROM_BLOCK_SETTINGS_MAIN, EEPROM_BLOCK_CRC_MAIN); //записываем основные настройки в память
    updateData((uint8_t*)&radioSettings, sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO); //записываем настройки радио в память
#if ESP_ENABLE
    updateData((uint8_t*)&extendedSettings, sizeof(extendedSettings), EEPROM_BLOCK_SETTINGS_EXTENDED, EEPROM_BLOCK_CRC_EXTENDED); //записываем расширенные настройки в память
#endif
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
#if ESP_ENABLE
    if (checkData(sizeof(extendedSettings), EEPROM_BLOCK_SETTINGS_EXTENDED, EEPROM_BLOCK_CRC_EXTENDED)) { //проверяем расширенные настройки
      updateData((uint8_t*)&extendedSettings, sizeof(extendedSettings), EEPROM_BLOCK_SETTINGS_EXTENDED, EEPROM_BLOCK_CRC_EXTENDED); //записываем расширенные настройки в память
      SET_ERROR(MEMORY_ERROR); //устанавливаем ошибку памяти
    }
    else EEPROM_ReadBlock((uint16_t)&extendedSettings, EEPROM_BLOCK_SETTINGS_EXTENDED, sizeof(extendedSettings)); //считываем настройки радио из памяти
#endif
#if ALARM_TYPE
    if (checkByte(EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM)) { //проверяем количетво будильников
      updateByte(alarms.num, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
      SET_ERROR(MEMORY_ERROR); //устанавливаем ошибку памяти
    }
    else alarms.num = EEPROM_ReadByte(EEPROM_BLOCK_ALARM); //считываем количество будильников из памяти
#endif
  }

  if (checkDebugSettingsCRC()) { //проверяем настройки отладки по умолчанию
    updateData((uint8_t*)&debugSettings, sizeof(debugSettings), EEPROM_BLOCK_SETTINGS_DEBUG, EEPROM_BLOCK_CRC_DEBUG); //записываем настройки отладки в память
#if LIGHT_SENS_ENABLE
    lightSensZoneUpdate(LIGHT_SENS_START_MIN, LIGHT_SENS_START_MAX); //обновление зон сенсора яркости освещения
#endif
  }
  if (checkData(sizeof(debugSettings), EEPROM_BLOCK_SETTINGS_DEBUG, EEPROM_BLOCK_CRC_DEBUG)) { //проверяем настройки отладки
    updateData((uint8_t*)&debugSettings, sizeof(debugSettings), EEPROM_BLOCK_SETTINGS_DEBUG, EEPROM_BLOCK_CRC_DEBUG); //записываем настройки отладки в память
    SET_ERROR(MEMORY_ERROR); //устанавливаем ошибку памяти
  }
  else EEPROM_ReadBlock((uint16_t)&debugSettings, EEPROM_BLOCK_SETTINGS_DEBUG, sizeof(debugSettings)); //считываем настройки отладки из памяти

#if GEN_ENABLE
#if GEN_FEEDBACK == 1
  updateTresholdADC(); //обновление предела удержания напряжения
#endif
  indiChangeCoef(); //обновление коэффициента линейного регулирования
#endif

#if PLAYER_TYPE == 1
  dfPlayerInit(); //инициализация DF плеера
#elif PLAYER_TYPE == 2
  sdPlayerInit(); //инициализация SD плеера
#endif

#if DS3231_ENABLE || ESP_ENABLE || RADIO_ENABLE || SENS_AHT_ENABLE || SENS_BME_ENABLE || SENS_SHT_ENABLE
  wireInit(); //инициализация шины wire
#endif
  indiInit(); //инициализация индикации

  backlAnimDisable(); //запретили эффекты подсветки
  changeBrightDisable(CHANGE_DISABLE); //запретить смену яркости

#if RADIO_ENABLE
  radioPowerOff(); //выключить питание радиоприемника
#endif

#if DS3231_ENABLE || SQW_PORT_ENABLE
  checkRTC(); //проверка модуля часов
#endif
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_BME_ENABLE || SENS_SHT_ENABLE || SENS_PORT_ENABLE
  checkTempSens(); //проверка установленного датчика температуры
#endif

  mainEnableWDT(); //основной запуск WDT

#if PLAYER_TYPE
  playerSetVoice(mainSettings.voiceSound); //установили голос озвучки
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

#if ALARM_TYPE
  alarmInit(); //инициализация будильника
#endif

  randomSeed(RTC.s * (RTC.m + RTC.h) + RTC.DD * RTC.MM); //радомный сид для глюков
  setAnimTimers(); //установка таймеров анимаций

#if DS3231_ENABLE
  _timer_sec[TMR_SYNC] = ((uint16_t)RTC_SYNC_TIME * 60); //устанавливаем таймер синхронизации
#endif

#if ALARM_TYPE
  checkAlarms(ALARM_CHECK_INIT); //проверка будильников
#endif

#if ESP_ENABLE
  wireSetAddress(WIRE_SLAVE_ADDR); //установка slave адреса шины
#endif

#if PCF8591_ENABLE
  checkPCF8591(); //проверка PCF8591
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
  _timer_sec[TMR_SHOW] = getPhaseTime(mainSettings.autoShowTime, AUTO_SHOW_PHASE); //установка таймера показа температуры
  _timer_sec[TMR_BURN] = getPhaseTime(mainSettings.burnTime, BURN_PHASE); //установка таймера антиотравления
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
//-------------------Установить флаг обновления данных в памяти---------------------
void setUpdateMemory(uint8_t mask) //установить флаг обновления данных в памяти
{
  memoryUpdate |= mask; //установили флаг
#if ESP_ENABLE
  device.status |= mask; //запоминаем статус
#endif
}
//----------------------------Обновить данные в памяти------------------------------
void updateMemory(void) //обновить данные в памяти
{
  if (!_timer_sec[TMR_MEM] && memoryUpdate) { //если нужно сохранить настройки
    _timer_sec[TMR_MEM] = 3; //установили таймер
    for (uint8_t i = 0; i < MEM_MAX_DATA; i++) { //проверяем все флаги
      if (memoryUpdate & 0x01) { //если флаг установлен
        switch (i) { //выбираем действие
          case MEM_UPDATE_MAIN_SET: updateData((uint8_t*)&mainSettings, sizeof(mainSettings), EEPROM_BLOCK_SETTINGS_MAIN, EEPROM_BLOCK_CRC_MAIN); break; //записываем основные настройки в память
          case MEM_UPDATE_FAST_SET: updateData((uint8_t*)&fastSettings, sizeof(fastSettings), EEPROM_BLOCK_SETTINGS_FAST, EEPROM_BLOCK_CRC_FAST); break; //записываем быстрые настройки в память
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
          case MEM_UPDATE_RADIO_SET: updateData((uint8_t*)&radioSettings, sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO); break; //записываем настройки радио в память
#endif
#if ESP_ENABLE
          case MEM_UPDATE_EXTENDED_SET: updateData((uint8_t*)&extendedSettings, sizeof(extendedSettings), EEPROM_BLOCK_SETTINGS_EXTENDED, EEPROM_BLOCK_CRC_EXTENDED); break; //записываем расширенные настройки в память
#endif
        }
      }
      memoryUpdate >>= 1; //сместили буфер флагов
    }
    memoryUpdate = 0; //сбрасываем флаги
  }
}
//------------------Чтение установленных датчиков температуры-----------------------
void readTempSens(void) //чтение установленных датчиков температуры
{
  if (sens.type & ~((0x01 << SENS_DS3231) | (0x01 << SENS_ALL))) { //если датчик обнаружен
    uint8_t pos = (0x01 << SENS_DHT); //установили тип датчика
    for (uint8_t sensor = (SENS_ALL - 1); sensor; sensor--) { //перебираем все датчики температуры
      sens.update = 0; //сбросили флаг проверки датчика температуры на ошибку связи
      if (sens.type & pos) { //если флаг датчика установлен
        switch (sensor) { //выбор датчика температуры
#if SENS_AHT_ENABLE
          case SENS_AHT: readTempAHT(); break; //чтение температуры/влажности с датчика AHT
#endif
#if SENS_SHT_ENABLE
          case SENS_SHT: readTempSHT(); break; //чтение температуры/влажности с датчика SHT
#endif
#if SENS_BME_ENABLE
          case SENS_BME: readTempBME(); break; //чтение температуры/давления/влажности с датчика BME/BMP
#endif
#if (SENS_PORT_ENABLE == 1) || (SENS_PORT_ENABLE == 3)
          case SENS_DS18: readTempDS(); break; //чтение температуры с датчика DS18x20
#endif
#if (SENS_PORT_ENABLE == 2) || (SENS_PORT_ENABLE == 3)
          case SENS_DHT: readTempDHT(); break; //чтение температуры/влажности с датчика DHT/MW/AM
#endif
        }
        if (!sens.update) sens.type &= ~pos; //сбросили флаг сенсора
        else if (sensor >= SENS_DS18) { //если тип датчика DHT/DS18
          sens.type = pos | (0x01 << SENS_ALL); //установили тип датчика
          break; //выходим
        }
      }
      pos >>= 1; //сместили тип датчика
    }
#if SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE
    if (sens.temp > 850) sens.temp = 850; //ограничили температуру
    if (sens.temp < -850) sens.temp = -850; //ограничили температуру
    if (sens.hum > 99) sens.hum = 99; //ограничили влажность
#endif
  }
}
//------------------Обновление установленных датчиков температуры-----------------------
void updateTempSens(void) //обновление установленных датчиков температуры
{
#if SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE
  readTempSens(); //чтение установленного датчика температуры
#endif

  if (!(sens.type & ~((0x01 << SENS_DS3231) | (0x01 << SENS_ALL)))) { //если основной датчик не отвечает
#if DS3231_ENABLE == 2
    if (readTempRTC()) { //чтение температуры с датчика DS3231
      sens.type |= (0x01 << SENS_DS3231); //установили флаг сенсора
      sens.update = 1; //установили флаг обновления сенсора
    }
    else {
      sens.type &= ~(0x01 << SENS_DS3231); //сбросили флаг сенсора
#endif
      sens.temp = 0x7FFF; //сбрасываем температуру
      sens.hum = 0; //сбрасываем влажность
      sens.press = 0; //сбрасываем давление
#if DS3231_ENABLE == 2
    }
#endif
  }

#if ESP_ENABLE
  if (sens.init && !(device.status & (0x01 << STATUS_UPDATE_SENS_DATA))) device.status |= (0x01 << STATUS_UPDATE_SENS_DATA); //если первичная инициализация пройдена и есп получила последние данные
  else { //иначе копируем внутренние данные
    mainSens.temp = 0x7FFF; //сбрасываем температуру
    mainSens.hum = 0; //сбрасываем влажность
    mainSens.press = 0; //сбрасываем давление
    sens.init = 1; //установили флаг инициализации сенсора
  }
#endif

#if ESP_ENABLE
  _timer_ms[TMR_SENS] = TEMP_ESP_UPDATE_TIME; //установили таймаут ожидания опроса есп
#else
  _timer_ms[TMR_SENS] = TEMP_UPDATE_TIME; //установили интервал следующего опроса
#endif
}
//-----------------Проверка установленного датчика температуры----------------------
void checkTempSens(void) //проверка установленного датчика температуры
{
#if SENS_AHT_ENABLE || SENS_BME_ENABLE || SENS_SHT_ENABLE || SENS_PORT_ENABLE
  sens.type = (0x01 << SENS_AHT) | (0x01 << SENS_SHT) | (0x01 << SENS_BME) | (0x01 << SENS_DS18) | (0x01 << SENS_DHT) | (0x01 << SENS_ALL);
#elif DS3231_ENABLE == 2
  sens.type = (0x01 << SENS_ALL);
#endif
  updateTempSens(); //чтение установленных датчиков температуры
#if SENS_AHT_ENABLE || SENS_BME_ENABLE || SENS_SHT_ENABLE || SENS_PORT_ENABLE
  if (!(sens.type & ~((0x01 << SENS_DS3231) | (0x01 << SENS_ALL)))) SET_ERROR(TEMP_SENS_ERROR); //иначе выдаем ошибку
#endif
}
//-------------------------Обновить показания температуры---------------------------
void updateTemp(void) //обновить показания температуры
{
  if (!_timer_ms[TMR_SENS]) { //если пришло время нового запроса температуры
    updateTempSens(); //обновление установленных датчиков температуры
  }
}
//-----------------------Получить текущий основной датчик--------------------------
uint8_t getMainSens(void)
{
  return (extendedSettings.tempMainSensor) ? SHOW_TEMP_ESP : SHOW_TEMP;
}
//---------------------Получить текущий датчик озвучки часа------------------------
uint8_t getHourSens(void)
{
  return (extendedSettings.tempHourSensor) ? SHOW_TEMP_ESP : SHOW_TEMP;
}
//------------------------Получить показания температуры---------------------------
int16_t getTemperatureData(uint8_t data)
{
  if (data >= SHOW_TEMP_ESP) return mainSens.temp + ((extendedSettings.tempCorrectSensor == 2) ? mainSettings.tempCorrect : 0);
  return sens.temp + ((extendedSettings.tempCorrectSensor == 1) ? mainSettings.tempCorrect : 0);
}
//------------------------Получить показания температуры---------------------------
int16_t getTemperatureData(void)
{
#if ESP_ENABLE
  return getTemperatureData(getMainSens());
#else
  return sens.temp + mainSettings.tempCorrect;
#endif
}
//--------------------------Получить знак температуры------------------------------
boolean getTemperatureSign(uint8_t data)
{
  return getTemperatureData(data) < 0;
}
//--------------------------Получить знак температуры------------------------------
boolean getTemperatureSign(void)
{
#if ESP_ENABLE
  return getTemperatureSign(getMainSens());
#else
  return getTemperatureData() < 0;
#endif
}
//------------------------Получить показания температуры---------------------------
uint16_t getTemperature(uint8_t data)
{
  int16_t temp = getTemperatureData(data);
  return (temp < 0) ? -temp : temp;
}
//------------------------Получить показания температуры---------------------------
uint16_t getTemperature(void)
{
  int16_t temp = getTemperatureData();
  return (temp < 0) ? -temp : temp;
}
//--------------------------Получить показания давления----------------------------
uint16_t getPressure(uint8_t data)
{
  if (data >= SHOW_TEMP_ESP) return mainSens.press;
  return sens.press;
}
//--------------------------Получить показания давления----------------------------
uint16_t getPressure(void)
{
#if ESP_ENABLE
  return getPressure(getMainSens());
#else
  return sens.press;
#endif
}
//-------------------------Получить показания влажности----------------------------
uint8_t getHumidity(uint8_t data)
{
  if (data >= SHOW_TEMP_ESP) return mainSens.hum;
  return sens.hum;
}
//-------------------------Получить показания влажности----------------------------
uint8_t getHumidity(void)
{
#if ESP_ENABLE
  return getHumidity(getMainSens());
#else
  return sens.hum;
#endif
}
//-----------------Обновление предела удержания напряжения-------------------------
void updateTresholdADC(void) //обновление предела удержания напряжения
{
  hv_treshold = HV_ADC(GET_VCC(REFERENCE, vcc_adc)) + CONSTRAIN(debugSettings.hvCorrect, -25, 25);
}
//------------------------Обработка аналоговых входов------------------------------
void analogUpdate(void) //обработка аналоговых входов
{
  if (!(ADCSRA & (0x01 << ADSC))) { //ждем окончания преобразования
    switch (ADMUX & 0x0F) {
#if GEN_ENABLE && (GEN_FEEDBACK == 1)
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
          else ADCSRA |= (0x01 << ADSC); //перезапускаем преобразование
        }
        break;
#endif
#if BTN_TYPE
      case ANALOG_BTN_PIN:
        btn.adc = ADCH; //записываем результат опроса
        ADMUX = 0; //сбросли признак чтения АЦП
        break;
#endif
#if LIGHT_SENS_ENABLE && LIGHT_SENS_TYPE == 1
      case ANALOG_LIGHT_PIN:
#if !LIGHT_SENS_PULL
        light_adc = ADCH; //записываем результат опроса
#else
        light_adc = 255 - ADCH; //записываем результат опроса
#endif
        ADMUX = 0; //сбросли признак чтения АЦП
        break;
#endif
      default:
#if LIGHT_SENS_ENABLE && LIGHT_SENS_TYPE == 1
        if (analogState & 0x01) { //сенсор яркости
          analogState &= ~0x01; //сбросили флаг обновления АЦП сенсора яркости
          ADMUX = (0x01 << REFS0) | (0x01 << ADLAR) | ANALOG_LIGHT_PIN; //настройка мультиплексатора АЦП
          ADCSRA |= (0x01 << ADSC); //запускаем преобразование
          return; //выходим
        }
#endif
#if BTN_TYPE
        if (analogState & 0x02) { //аналоговые кнопки
          analogState &= ~0x02; //сбросили флаг обновления АЦП кнопок
          ADMUX = (0x01 << REFS0) | (0x01 << ADLAR) | ANALOG_BTN_PIN; //настройка мультиплексатора АЦП
          ADCSRA |= (0x01 << ADSC); //запускаем преобразование
          return; //выходим
        }
#endif
#if GEN_ENABLE && (GEN_FEEDBACK == 1)
        if (analogState & 0x04) { //обратная связь
          analogState &= ~0x04; //сбросили флаг обновления АЦП обратной связи преобразователя
          ADMUX = (0x01 << REFS0) | ANALOG_DET_PIN; //настройка мультиплексатора АЦП
          ADCSRA |= (0x01 << ADSC); //запускаем преобразование
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
    ADCSRA |= (0x01 << ADSC); //запускаем преобразование
    while (ADCSRA & (0x01 << ADSC)); //ждем окончания преобразования
    temp += ADCL | ((uint16_t)ADCH << 8); //записали результат
  }
  vcc_adc = temp / CYCLE_VCC_CHECK; //получаем напряжение питания

  if (GET_VCC(REFERENCE, vcc_adc) < MIN_VCC || GET_VCC(REFERENCE, vcc_adc) > MAX_VCC) SET_ERROR(VCC_ERROR); //устанвливаем ошибку по питанию

#if BTN_TYPE
  ADMUX = (0x01 << REFS0) | (0x01 << ADLAR) | ANALOG_BTN_PIN; //настройка мультиплексатора АЦП
  ADCSRA |= (0x01 << ADSC); //запускаем преобразование
  while (ADCSRA & (0x01 << ADSC)); //ждем окончания преобразования
  btn.adc = ADCH; //записываем результат опроса
#endif
#if GEN_ENABLE && (GEN_FEEDBACK == 1)
  ADMUX = (0x01 << REFS0) | ANALOG_DET_PIN; //настройка мультиплексатора АЦП
#endif

#if (GEN_ENABLE && (GEN_FEEDBACK == 1)) || BTN_TYPE || (LIGHT_SENS_ENABLE && LIGHT_SENS_TYPE == 1)
  ADCSRA = (0x01 << ADEN) | (0x01 << ADPS0) | (0x01 << ADPS2); //настройка АЦП пределитель 32
  ADCSRA |= (0x01 << ADSC); //запускаем преобразование
#endif
}
//----------------Обновление зон  сенсора яркости освещения------------------------
void lightSensZoneUpdate(uint8_t min, uint8_t max) //обновление зон сенсора яркости освещения
{
  debugSettings.light_zone[0][2] = min;
  debugSettings.light_zone[1][0] = max;

  min = (max - min) / 3;
  max = min * 2;

  debugSettings.light_zone[1][2] = min + LIGHT_SENS_GIST;
  debugSettings.light_zone[0][1] = min - LIGHT_SENS_GIST;
  debugSettings.light_zone[1][1] = max + LIGHT_SENS_GIST;
  debugSettings.light_zone[0][0] = max - LIGHT_SENS_GIST;
}
//-------------------Обработка сенсора яркости освещения---------------------------
void lightSensUpdate(void) //обработка сенсора яркости освещения
{
  static uint8_t now_light_state;
  if (mainSettings.timeBright[0] == mainSettings.timeBright[1]) { //если разрешена робота сенсора
    _timer_ms[TMR_LIGHT] = (1000 - LIGHT_SENS_TIME); //установили таймер

    if (light_adc < debugSettings.light_zone[0][now_light_state]) {
      if (now_light_state < 2) now_light_state++;
    }
    else if (light_adc > debugSettings.light_zone[1][now_light_state]) {
      if (now_light_state) now_light_state--;
    }

    if (now_light_state != light_state) {
      light_state = now_light_state;
      light_update = 1; //устанавливаем флаг изменения яркости
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
#if DS3231_ENABLE
  if (!disable32K()) return; //отключение вывода 32K
#endif

#if SQW_PORT_ENABLE
#if DS3231_ENABLE
  if (!setSQW()) return; //установка SQW на 1Гц
#endif
  EICRA = (0x01 << ISC01); //настраиваем внешнее прерывание по спаду импульса на INT0
  EIFR |= (0x01 << INTF0); //сбрасываем флаг прерывания INT0

  for (_timer_ms[TMR_MS] = SQW_TEST_TIME; !(EIFR & (0x01 << INTF0)) && _timer_ms[TMR_MS];) { //ждем сигнала от SQW
    for (uint8_t _tick = tick_ms; _tick > 0; _tick--) { //если был тик то обрабатываем данные
      tick_ms--; //убавили счетчик миллисекунд
      if (_timer_ms[TMR_MS] > MS_PERIOD) _timer_ms[TMR_MS] -= MS_PERIOD; //если таймер больше периода
      else if (_timer_ms[TMR_MS]) _timer_ms[TMR_MS] = 0; //иначе сбрасываем таймер
    }
  }

  tick_sec = 0; //сбросили счетчик секунд
#endif

#if DS3231_ENABLE
  if (!getTime(RTC_CLEAR_OSF)) { //считываем время из RTC
    writeAgingRTC(debugSettings.aging); //восстанавливаем коррекцию хода
    sendTime(); //отправляем последнее сохраненное время в RTC
  }
#if ESP_ENABLE
  else device.status |= (0x01 << STATUS_UPDATE_TIME_SET); //установили статус актуального времени
#endif
#endif

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
#if ESP_ENABLE
  device.failure = _error_reg; //скопировали ошибки
#endif
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
        melodyStop(); //остановка воспроизведения мелодии
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
            buzz_pulse(ERROR_SOUND_FREQ, (_sound_bit & 0x01) ? ERROR_SOUND_HIGH_TIME : ERROR_SOUND_LOW_TIME); //воспроизводим звук
            _timer_ms[TMR_PLAYER] = (_sound_bit & 0x01) ? ERROR_SOUND_HIGH_PAUSE : ERROR_SOUND_LOW_PAUSE; //установили таймер
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
  for (_timer_ms[TMR_MS] = TEST_FIRMWARE_TIME; _timer_ms[TMR_MS] && !buttonState();) systemTask(); //ждем

#if PLAYER_TYPE
  playerSetTrackNow(PLAYER_TEST_SOUND, PLAYER_GENERAL_FOLDER); //звук тестирования динамика
#else
  melodyPlay(SOUND_TEST_SPEAKER, SOUND_LINK(general_sound), REPLAY_ONCE); //сигнал тестирования динамика
#endif

#if (BACKL_TYPE != 3) && BACKL_TYPE
  backlSetBright(TEST_BACKL_BRIGHT); //устанавливаем максимальную яркость
#endif
  indiSetBright(TEST_INDI_BRIGHT); //установка яркости индикаторов
#if (NEON_DOT != 3) || !DOTS_PORT_ENABLE
  dotSetBright(TEST_DOT_BRIGHT); //установка яркости точек
#endif

#if DOTS_PORT_ENABLE
#if DOTS_TYPE == 2
  indiSetDots(0, DOTS_NUM * 2); //установка разделительных точек
#else
  indiSetDots(0, DOTS_NUM); //установка разделительных точек
#endif
#endif

  while (1) {
#if INDI_SYMB_TYPE
    indiClr(); //очистка индикаторов
    for (uint8_t symb = 0; symb < 10; symb++) {
      indiSetSymb(ID(symb)); //установка индикатора символов
      for (_timer_ms[TMR_MS] = TEST_LAMP_TIME; _timer_ms[TMR_MS];) { //ждем
        dataUpdate(); //обработка данных
        if (buttonState()) return; //выходим если нажата кнопка
      }
    }
    indiClrSymb(); //очистка индикатора символов
#endif
    for (uint8_t indi = 0; indi < LAMP_NUM; indi++) {
      indiClr(); //очистка индикаторов
#if BACKL_TYPE == 3
      setLedBright(0); //выключаем светодиоды
      setLedBright(indi, TEST_BACKL_BRIGHT); //включаем светодиод
#endif
      for (uint8_t digit = 0; digit < 10; digit++) {
        indiPrintNum(digit, indi); //отрисовываем цифру
#if BACKL_TYPE == 3
        setLedHue(indi, digit * 25, WHITE_OFF); //устанавливаем статичный цвет
#endif
        for (_timer_ms[TMR_MS] = TEST_LAMP_TIME; _timer_ms[TMR_MS];) { //ждем
          dataUpdate(); //обработка данных
          if (buttonState()) return; //выходим если нажата кнопка
        }
      }
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
  uint8_t entry_pass[] = {0, 0, 0, 0}; //введеный пароль

  dotSetBright(0); //выключаем точки
  indiSetBright(30); //устанавливаем максимальную яркость индикаторов

#if PLAYER_TYPE
  playerSetTrack(PLAYER_DEBUG_SOUND, PLAYER_GENERAL_FOLDER);
#endif

  while (1) {
    dataUpdate(); //обработка данных

    if (!indi.update) {
      indi.update = 1; //сбросили флаг
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

      for (uint8_t i = 0; i < 4; i++) indiPrintNum(entry_pass[i], (LAMP_NUM / 2 - 2) + i); //вывод пароля
      if (blink_data) indiClr(cur_indi + (LAMP_NUM / 2 - 2)); //очистка индикатора

      blink_data = !blink_data; //мигаем индикатором
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        if (entry_pass[cur_indi] > 0) entry_pass[cur_indi]--; else entry_pass[cur_indi] = 9;
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        if (entry_pass[cur_indi] < 9) entry_pass[cur_indi]++; else entry_pass[cur_indi] = 0;
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        if (cur_indi < 3) cur_indi++; else cur_indi = 0; //переключаем разряды
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        if (((uint16_t)entry_pass[3] + ((uint16_t)entry_pass[2] * 10) + ((uint16_t)entry_pass[1] * 100) + ((uint16_t)entry_pass[0] * 1000)) == DEBUG_PASS) return 1; //если пароль совпал
        for (uint8_t i = 0; i < 4; i++) entry_pass[i] = 0; //сбросили введеный пароль
        cur_indi = 0; //сбросили текущий индикатор
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
            if (temp_min > light_adc) temp_min = light_adc;
            if (temp_max < light_adc) temp_max = light_adc;
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
#if LAMP_NUM > 4
          indiPrintNum(cur_mode + 1, 5); //режим
#endif
          switch (cur_mode) {
#if DS3231_ENABLE
            case DEB_AGING_CORRECT: indiPrintNum(debugSettings.aging + 128, 0); break; //выводим коррекцию DS3231
#endif
            case DEB_TIME_CORRECT: indiPrintNum(debugSettings.timePeriod, 0); break; //выводим коррекцию внутреннего таймера
#if GEN_ENABLE
            case DEB_DEFAULT_MIN_PWM: indiPrintNum(debugSettings.min_pwm, 0); break; //выводим минимальный шим
            case DEB_DEFAULT_MAX_PWM: indiPrintNum(debugSettings.max_pwm, 0); break; //выводим максимальный шим
#if GEN_FEEDBACK == 1
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
              indiPrintNum(light_adc, 1, 3); //выводим значение АЦП датчика освещения
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
#if DS3231_ENABLE
              case DEB_AGING_CORRECT: if (debugSettings.aging > -127) debugSettings.aging--; else debugSettings.aging = 127; break; //коррекция хода
#endif
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
#if GEN_FEEDBACK == 1
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
#if DS3231_ENABLE
              case DEB_AGING_CORRECT: if (debugSettings.aging < 127) debugSettings.aging++; else debugSettings.aging = -127; break; //коррекция хода
#endif
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
#if GEN_FEEDBACK == 1
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
#if DS3231_ENABLE
            case DEB_AGING_CORRECT: if (!readAgingRTC(&debugSettings.aging)) cur_set = 0; break; //чтение коррекции хода
#endif
            case DEB_TIME_CORRECT: break; //коррекция хода
#if GEN_ENABLE
            case DEB_DEFAULT_MIN_PWM: indiSetBright(1); break; //минимальное значение шим
            case DEB_DEFAULT_MAX_PWM: break; //максимальное значение шим
#if GEN_FEEDBACK == 1
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
            case DEB_LIGHT_SENS: lightSensZoneUpdate(temp_min, temp_max); break; //обновление зон сенсора яркости освещения
#endif
            case DEB_RESET: //сброс настроек отладки
              if (cur_reset) { //подтверждение
                cur_mode = 0; //перешли на первый пункт меню
#if DS3231_ENABLE
                debugSettings.aging = 0; //коррекции хода модуля часов
#endif
                debugSettings.timePeriod = US_PERIOD; //коррекция хода внутреннего осцилятора
#if GEN_ENABLE
                debugSettings.min_pwm = DEFAULT_MIN_PWM; //минимальное значение шим
                debugSettings.max_pwm = DEFAULT_MAX_PWM; //максимальное значение шим
                indiChangeCoef(); //обновление коэффициента линейного регулирования
#if GEN_FEEDBACK == 1
                debugSettings.hvCorrect = 0; //коррекция напряжения преобразователя
                updateTresholdADC(); //обновление предела удержания напряжения
#endif
#endif
#if IR_PORT_ENABLE
                for (uint8_t i = 0; i < (KEY_MAX_ITEMS - 1); i++) debugSettings.irButtons[i] = 0; //сбрасываем значение ячеек кнопок пульта
#endif
#if DS3231_ENABLE
                writeAgingRTC(debugSettings.aging); //запись коррекции хода
#endif
#if LIGHT_SENS_ENABLE
                lightSensZoneUpdate(LIGHT_SENS_START_MIN, LIGHT_SENS_START_MAX); //обновление зон сенсора яркости освещения
#endif
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
#if IR_PORT_ENABLE
        else if (cur_mode == DEB_IR_BUTTONS) { //если програмирование кнопок
          debugSettings.irButtons[cur_button] = 0;
          cur_update = 0; //обновление экрана
        }
#endif
        break;
    }
  }
}
//--------------------------------Проверка PCF8591 ----------------------------------------------------
void checkPCF8591() // проверка PCF8591
{
  if (!isConnectedPCF()) {
    SET_ERROR(PCF8591_ERROR);
  }

  if (!readAnalogInputChannelsPCF()) {
    SET_ERROR(PCF8591_ERROR);
  };
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
#if !ESP_ENABLE && (ALARM_TYPE == 1)
  else if (alarms.num > 1) { //если будильников в памяти больше одного
    alarms.num = 1; //оставляем один будильник
    updateByte(alarms.num, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
  }
#endif
}
//-----------------------------------Отключение будильника------------------------------------------------
void alarmDisable(void) //отключение будильника
{
#if PLAYER_TYPE
  if (mainSettings.knockSound) playerSetTrackNow(PLAYER_ALARM_DISABLE_SOUND, PLAYER_GENERAL_FOLDER); //звук выключения будильника
#else
  melodyPlay(SOUND_ALARM_DISABLE, SOUND_LINK(general_sound), REPLAY_ONCE); //звук выключения будильника
#endif
  alarmReset(); //сброс будильника
}
//--------------------------------------Сброс будильника--------------------------------------------------
void alarmReset(void) //сброс будильника
{
  _timer_sec[TMR_ALM] = 0; //сбрасываем таймер отключения будильника
  _timer_sec[TMR_ALM_WAIT] = 0; //сбрасываем таймер ожидания повторного включения тревоги
  _timer_sec[TMR_ALM_SOUND] = 0; //сбрасываем таймер отключения звука
  alarms.now = ALARM_DISABLE; //сбрасываем флаг тревоги
  checkAlarms(ALARM_CHECK_SET); //проверка будильников
  dotReset(ANIM_RESET_CHANGE); //сброс анимации точек
}
//-----------------------------Получить основные данные будильника-----------------------------------------
uint8_t alarmRead(uint8_t almNum, uint8_t almDataPos) //получить основные данные будильника
{
  return EEPROM_ReadByte(EEPROM_BLOCK_ALARM_DATA + ((uint16_t)almNum * ALARM_MAX_ARR) + almDataPos); //возвращаем запрошеный байт
}
//-----------------------------Записать основные данные будильника-----------------------------------------
void alarmWrite(uint8_t almNum, uint8_t almDataPos, uint8_t almData) //записать основные данные будильника
{
  EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + ((uint16_t)almNum * ALARM_MAX_ARR) + almDataPos, almData); //записываем указанный байт
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
    EEPROM_UpdateByte(newCell + ALARM_VOLUME, DEFAULT_ALARM_VOLUME); //устанавливаем громкость по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_RADIO, 0); //устанавливаем радиобудильник по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_STATUS, 0); //устанавливаем статус по умолчанию
    updateByte(++alarms.num, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
  }
}
//-----------------------------------Удалить будильник-----------------------------------------------------
void delAlarm(uint8_t alarm) //удалить будильник
{
  if (alarms.num > 1) { //если будильник доступен
    for (uint8_t start = alarm; start < alarms.num; start++) { //перезаписываем массив будильников
      uint16_t oldCell = EEPROM_BLOCK_ALARM_DATA + ((uint16_t)start * ALARM_MAX_ARR);
      uint16_t newCell = EEPROM_BLOCK_ALARM_DATA + ((uint16_t)(start - 1) * ALARM_MAX_ARR);
      for (uint8_t block = 0; block < ALARM_MAX_ARR; block++) EEPROM_UpdateByte(newCell + block, EEPROM_ReadByte(oldCell + block));
    }
    updateByte(--alarms.num, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
  }
}
//----------------------------------Проверка будильников----------------------------------------------------
void checkAlarms(uint8_t check) //проверка будильников
{
  if (alarms.now < ALARM_WAIT) { //если тревога не активна
    if (RTC.YY <= 2000) return; //выходим если время не установлено

    alarms.now = ALARM_DISABLE; //сбрасываем флаг будильников
    int16_t time_now = 1440 + ((int16_t)RTC.h * 60) + RTC.m; //рассчитали текущее время
    for (uint8_t alm = 0; alm < alarms.num; alm++) { //опрашиваем все будильники
      uint8_t mode_alarm = alarmRead(alm, ALARM_MODE); //считали режим будильника
      if (mode_alarm) { //если будильник включен
        alarms.now = ALARM_ENABLE; //мигание точек при включенном будильнике
        if (check == ALARM_CHECK_SET) return; //выходим если нужно только проверить

        uint8_t days_alarm = alarmRead(alm, ALARM_DAYS); //считали дни недели будильника
        int16_t time_alarm = ((int16_t)alarmRead(alm, ALARM_HOURS) * 60) + alarmRead(alm, ALARM_MINS);
        switch (mode_alarm) { //устанавливаем дни в зависимости от режима
          case 3: days_alarm = 0x3E; break; //по будням
          case 4: if (!days_alarm) days_alarm = 0xFF; else if (days_alarm & 0x80) days_alarm |= 0x01; break; //по дням недели
          default: days_alarm = 0xFF; break; //каждый день
        }

        uint8_t start_alarm = 0; //установили первоначальное время до будильника
        for (uint8_t dw = RTC.DW - 1; dw <= RTC.DW; dw++) { //проверяем все дни недели
          if (days_alarm & (0x01 << dw)) { //если активирован день недели
            int16_t time_buf = time_now - time_alarm; //расчет интервала
            if (!time_buf) start_alarm = 1; //если будильник в зоне активации
            else if ((time_buf > 0) && (time_buf < 30)) start_alarm = 2; //если будильник в зоне активации
          }
          time_alarm += 1440; //прибавили время будильнику
        }

        uint8_t status_alarm = alarmRead(alm, ALARM_STATUS); //считали статус будильника

        if (status_alarm) { //если будильник заблокирован автоматически
          if (status_alarm == 255) { //если будильник заблокирован пользователем
            if ((check == ALARM_CHECK_INIT) || (start_alarm < 2)) { //если первичная проверка будильников или будильник вне зоны активации
              status_alarm = 0; //сбросили статус блокировки пользователем
              alarmWrite(alm, ALARM_STATUS, status_alarm); //устанавливаем статус активности будильник
            }
          }
          else if ((status_alarm != RTC.DW) && !start_alarm) { //если вышли из зоны активации будильника
            status_alarm = 0; //устанавливаем статус блокировки будильника
            alarmWrite(alm, ALARM_STATUS, status_alarm); //устанавливаем статус активности будильник
          }
        }
        if (!status_alarm && start_alarm) { //если будильник не был заблокирован
          alarms.now = ALARM_WARN; //устанавливаем флаг тревоги
          if (mode_alarm == 1) { //если был установлен режим одиночный
#if ESP_ENABLE
            device.status |= (0x01 << STATUS_UPDATE_ALARM_SET);
#endif
            alarmWrite(alm, ALARM_MODE, 0); //выключаем будильник
          }
          alarmWrite(alm, ALARM_STATUS, RTC.DW); //сбрасываем статус активности будильник
          alarms.sound = alarmRead(alm, ALARM_SOUND); //номер мелодии
          alarms.radio = alarmRead(alm, ALARM_RADIO); //текущий режим звука
          alarms.volume = alarmRead(alm, ALARM_VOLUME); //текущая громкость
          _timer_sec[TMR_ALM] = ((uint16_t)extendedSettings.alarmTime * 60); //установили таймер таймаута будильника
          _timer_sec[TMR_ALM_SOUND] = ((uint16_t)extendedSettings.alarmSoundTime * 60); //установили таймер таймаута звука будильника
          return; //выходим
        }
      }
    }
  }
}
//-------------------------------Обновление данных будильников---------------------------------------------
void alarmDataUpdate(void) //обновление данных будильников
{
  if (alarms.now > ALARM_ENABLE) { //если тревога активна
    if (!_timer_sec[TMR_ALM]) { //если пришло время выключить будильник
      alarmReset(); //сброс будильника
      return; //выходим
    }

    if (extendedSettings.alarmWaitTime && (alarms.now == ALARM_WAIT)) { //если будильник в режиме ожидания
      if (!_timer_sec[TMR_ALM_WAIT]) { //если пришло время повторно включить звук
        _timer_sec[TMR_ALM_SOUND] = ((uint16_t)extendedSettings.alarmSoundTime * 60); //установили таймер таймаута звука будильника
        alarms.now = ALARM_WARN; //устанавливаем флаг тревоги будильника
      }
    }
    else if (extendedSettings.alarmSoundTime) { //если таймаут тревоги включен
      if (!_timer_sec[TMR_ALM_SOUND]) { //если пришло время выключить тревогу
        if (extendedSettings.alarmWaitTime) { //если время ожидания включено
          _timer_sec[TMR_ALM_WAIT] = ((uint16_t)extendedSettings.alarmWaitTime * 60); //установили таймер таймаута ожидания будильника
          alarms.now = ALARM_WAIT; //устанавливаем флаг ожидания тревоги
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

#if PLAYER_TYPE || (RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE))
  boolean auto_vol = 0; //флаг автогромкости
  uint8_t cur_vol = alarms.volume; //текущая громкость

  if (!cur_vol) { //если автогромкость
    auto_vol = 1; //установили флаг автогромкости
    cur_vol = ALARM_AUTO_VOL_MIN; //установили минимальную громкость
  }

  _timer_ms[TMR_ANIM] = ALARM_AUTO_VOL_TIMER; //устанавливаем таймер
#endif

#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
  if (alarms.radio) { //если режим радио
    if (getPowerStatusRDA() != RDA_ERROR) { //если радиоприемник доступен
      setPowerRDA(RDA_ON); //включаем радио
      setVolumeRDA(cur_vol); //устанавливаем громкость
      setFreqRDA(radioSettings.stationsSave[alarms.sound]); //устанавливаем частоту
    }
    else { //иначе переходим в режим мелодии
      alarms.sound = 0; //установили номер мелодии
      alarms.radio = 0; //отключили режим радио
    }
  }
  else radioPowerOff(); //выключить питание радиоприемника
#endif

#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
  if (!alarms.radio) {
#endif
#if PLAYER_TYPE
    playerStop(); //сброс позиции мелодии
    playerSetVolNow(cur_vol); //установка громкости
#else
    melodyPlay(alarms.sound, SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
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

    if (alarms.now != ALARM_WARN) { //если тревога сброшена
#if PLAYER_TYPE
      playerStop(); //сброс позиции мелодии
#else
      melodyStop(); //сброс позиции мелодии
#endif
      return MAIN_PROGRAM; //выходим
    }

#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
#if PLAYER_TYPE
    if (!alarms.radio && !playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarms.sound, PLAYER_ALARM_FOLDER); //воспроизводим мелодию
    if (auto_vol && !_timer_ms[TMR_ANIM]) { //если пришло время
      _timer_ms[TMR_ANIM] = ALARM_AUTO_VOL_TIMER; //устанавливаем таймер
      if (cur_vol < ALARM_AUTO_VOL_MAX) cur_vol++;
      else auto_vol = 0; //сбросили флаг автогромкости

      if (alarms.radio) setVolumeRDA(cur_vol); //устанавливаем громкость
      else playerSetVolNow(cur_vol); //установка громкости
    }
#else
    if (auto_vol && !_timer_ms[TMR_ANIM]) { //если пришло время
      _timer_ms[TMR_ANIM] = ALARM_AUTO_VOL_TIMER; //устанавливаем таймер
      if (cur_vol < ALARM_AUTO_VOL_MAX) cur_vol++;
      else auto_vol = 0; //сбросили флаг автогромкости
      setVolumeRDA(cur_vol); //устанавливаем громкость
    }
#endif
#elif PLAYER_TYPE
    if (!playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarms.sound, PLAYER_ALARM_FOLDER); //воспроизводим мелодию
    if (auto_vol && !_timer_ms[TMR_ANIM]) { //если пришло время
      _timer_ms[TMR_ANIM] = ALARM_AUTO_VOL_TIMER; //устанавливаем таймер
      if (cur_vol < ALARM_AUTO_VOL_MAX) cur_vol++;
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE) && ALARM_RADIO_CONTINUE
        if (extendedSettings.alarmWaitTime && !alarms.radio) //если есть время ожидания и режим музыкального будильника
#else
        if (extendedSettings.alarmWaitTime) //если есть время ожидания
#endif
        {
          alarms.now = ALARM_WAIT; //устанавливаем флаг ожидания
          _timer_sec[TMR_ALM_WAIT] = ((uint16_t)extendedSettings.alarmWaitTime * 60);
          _timer_sec[TMR_ALM_SOUND] = 0;
#if PLAYER_TYPE
          if (mainSettings.knockSound) playerSetTrackNow(PLAYER_ALARM_WAIT_SOUND, PLAYER_GENERAL_FOLDER); //звук ожидания будильника
#else
          melodyPlay(SOUND_ALARM_WAIT, SOUND_LINK(general_sound), REPLAY_ONCE); //звук ожидания будильника
#endif
        }
        else {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE) && ALARM_RADIO_CONTINUE
          if (alarms.radio) {
            radioSettings.stationsFreq = radioSettings.stationsSave[alarms.sound];
            radio.powerState = RDA_ON; //установили флаг питания радио
          }
#endif
          alarmDisable(); //отключение будильника
        }
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
#if ESP_ENABLE
//-------------------------------------Проверка статуса шины-------------------------------------------
uint8_t busCheck(void) //проверка статуса шины
{
#if RADIO_ENABLE || DS3231_ENABLE
  if (bus.statusExt) {
    uint8_t status = bus.statusExt;
    bus.statusExt = 0; //сбросили статус
    if (status) { //если установлены флаги радио
      for (uint8_t i = 0; i < BUS_EXT_MAX_DATA; i++) { //проверяем все флаги
        if (status & 0x01) { //если флаг установлен
          switch (i) { //выбираем действие
#if DS3231_ENABLE
            case BUS_EXT_COMMAND_SEND_TIME: sendTime(); break; //отправить время в RTC
#endif
#if RADIO_ENABLE
            case BUS_EXT_COMMAND_RADIO_VOL: memoryUpdate |= (0x01 << MEM_UPDATE_RADIO_SET); setVolumeRDA(radioSettings.volume); break;
            case BUS_EXT_COMMAND_RADIO_FREQ: memoryUpdate |= (0x01 << MEM_UPDATE_RADIO_SET); setFreqRDA(radioSettings.stationsFreq); if (mainTask == RADIO_PROGRAM) radioSearchStation(); break;
#endif
          }
        }
        status >>= 1; //сместили флаги
      }
    }
  }
#endif
  return bus.status;
}
//-------------------------------------Проверка команды шины-------------------------------------------
void busCommand(void) //проверка команды шины
{
  if (bus.status & ~(0x01 << BUS_COMMAND_WAIT)) {
#if RADIO_ENABLE || (TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE))
    uint8_t status = bus.status & ~((0x01 << BUS_COMMAND_WAIT) | (0x01 << BUS_COMMAND_UPDATE));
    bus.status &= (0x01 << BUS_COMMAND_WAIT); //сбросили статус
    if (status) { //если установлены флаги
      changeAnimState = ANIM_RESET_DOT; //установили сброс анимации

#if PLAYER_TYPE
      playerStop(); //сброс воспроизведения плеера
#else
      melodyStop(); //сброс воспроизведения мелодии
#endif

      switch (status) { //выбираем действие
#if RADIO_ENABLE
        case BUS_COMMAND_RADIO_MODE: if (mainTask != RADIO_PROGRAM) mainTask = RADIO_PROGRAM; else mainTask = MAIN_PROGRAM; break;
        case BUS_COMMAND_RADIO_POWER:
          radioPowerSwitch(); //переключили питание радио
          if (radio.powerState == RDA_ON) { //если питание радио включено
            if (mainTask == MAIN_PROGRAM) mainTask = RADIO_PROGRAM;
          }
          else { //иначе питание радио выключено
            if (mainTask == RADIO_PROGRAM) mainTask = MAIN_PROGRAM;
          }
          break;
        case BUS_COMMAND_RADIO_SEEK_UP: radioSeekUp(); mainTask = RADIO_PROGRAM; break;
        case BUS_COMMAND_RADIO_SEEK_DOWN: radioSeekDown(); mainTask = RADIO_PROGRAM; break;
#endif
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
        case BUS_COMMAND_TIMER_MODE: mainTask = TIMER_PROGRAM; break;
#endif
      }
    }
#else
    bus.status &= (0x01 << BUS_COMMAND_WAIT); //сбросили статус
#endif
  }
}
#endif
//------------------------------------Обновление статуса шины------------------------------------------
uint8_t busUpdate(void) //обновление статуса шины
{
  if (TWCR & (0x01 << TWINT)) {
    switch (TWSR & 0xF8) { //прочитали статус шины
      case 0x00: //ошибка шины
      case 0x20: //передан SLA+W - принят NACK
      case 0x30: //передан байт данных - принят NACK
      case 0x48: //передан SLA+R - принят NACK
      case 0x38: //проигрыш арбитража
        wireEnd(); //остановка шины wire
        return 1; //возвращаем ошибку шины
#if ESP_ENABLE
      case 0x60: //принят SLA+W - передан ACK
        bus.position = 0;
        bus.counter = 0;
        bus.comand = BUS_WAIT_DATA;
        TWCR |= (0x01 << TWINT); //сбросили флаг прерывания
        break;
      case 0x80: //принят байт данных - передан ACK
      case 0x88: //принят байт данных - передан NACK
        switch (bus.comand) {
          case BUS_WAIT_DATA: //установка команды
            bus.comand = TWDR; //записали команду
            switch (bus.comand) {
              case BUS_WRITE_TIME: //настройки времени
              case BUS_READ_TIME: for (uint8_t i = 0; i < sizeof(RTC); i++) bus.buffer[i] = *((uint8_t*)&RTC + i); break; //копируем время
              case BUS_WRITE_FAST_SET: if (mainTask == FAST_SET_PROGRAM) bus.status |= (0x01 << BUS_COMMAND_WAIT); break; //быстрые настройки
              case BUS_WRITE_MAIN_SET: if (mainTask == MAIN_SET_PROGRAM) bus.status |= (0x01 << BUS_COMMAND_WAIT); break; //основные настройки
#if ALARM_TYPE
              case BUS_WRITE_SELECT_ALARM: //настройки будильника
              case BUS_WRITE_ALARM_DATA:
              case BUS_DEL_ALARM:
              case BUS_NEW_ALARM:
#endif
#if RADIO_ENABLE
              case BUS_WRITE_RADIO_VOL: //настройки радио
              case BUS_WRITE_RADIO_FREQ:
#endif
#if ALARM_TYPE || RADIO_ENABLE
                if (mainTask == ALARM_SET_PROGRAM) bus.status |= (0x01 << BUS_COMMAND_WAIT); //настройки будильника
                break;
#endif
            }
            break;
          case BUS_WRITE_TIME: //прием настроек времени
            if (bus.counter < sizeof(RTC)) {
              bus.buffer[bus.counter] = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_WRITE_FAST_SET: //прием быстрых настроек
            if (bus.counter < sizeof(fastSettings)) {
              *((uint8_t*)&fastSettings + bus.counter) = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_WRITE_MAIN_SET: //прием основных настроек
            if (bus.counter < sizeof(mainSettings)) {
              *((uint8_t*)&mainSettings + bus.counter) = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
#if ALARM_TYPE
          case BUS_WRITE_SELECT_ALARM:
          case BUS_READ_SELECT_ALARM:
            if (TWDR < alarms.num) bus.position = TWDR + 1; //выбрали номер будильника

            alarmReadBlock(bus.position, bus.buffer); //читаем блок данных
            if (bus.comand == BUS_WRITE_SELECT_ALARM) bus.comand = BUS_WRITE_ALARM_DATA; //перешли в режим настроек будильника
            else bus.comand = BUS_READ_ALARM_DATA; //перешли в режим настроек будильника
            break;
          case BUS_WRITE_ALARM_DATA: //прием настроек будильника
            if (bus.counter < (ALARM_MAX_ARR - 1)) {
              bus.buffer[bus.counter] = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_DEL_ALARM: //удалить будильник
            if (!bus.counter) {
              bus.position = TWDR + 1; //выбрали номер будильника
              bus.counter++; //сместили указатель
            }
            break;
#endif
#if RADIO_ENABLE
          case BUS_WRITE_RADIO_STA: //прием настроек радиостанций
            if (bus.counter < sizeof(radioSettings.stationsSave)) {
              if (bus.counter & 0x01) radioSettings.stationsSave[bus.counter >> 1] = ((uint16_t)TWDR << 8) | bus.buffer[0];
              else bus.buffer[0] = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_WRITE_RADIO_VOL: //прием громкости радио
            if (!bus.counter) {
              radioSettings.volume = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_WRITE_RADIO_FREQ: //прием частоты радио
            if (bus.counter < sizeof(radioSettings.stationsFreq)) {
              if (bus.counter & 0x01) radioSettings.stationsFreq = ((uint16_t)TWDR << 8) | bus.buffer[0];
              else bus.buffer[0] = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
#endif
          case BUS_WRITE_EXTENDED_SET: //прием расширенных настроек
            if (bus.counter < sizeof(extendedSettings)) {
              *((uint8_t*)&extendedSettings + bus.counter) = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
          case BUS_WRITE_TIMER_SET: //прием настроек таймера
            if (bus.counter < sizeof(timer)) {
              *((uint8_t*)&timer + bus.counter) = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
#endif
#if (DS3231_ENABLE != 2) && !SENS_AHT_ENABLE && !SENS_SHT_ENABLE && !SENS_BME_ENABLE && !SENS_PORT_ENABLE
          case BUS_WRITE_SENS_DATA:
            if (bus.counter < sizeof(sens)) {
              bus.buffer[bus.counter] = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
#endif
          case BUS_WRITE_MAIN_SENS_DATA:
            if (bus.counter < sizeof(mainSens)) {
              bus.buffer[bus.counter] = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
#if !LIGHT_SENS_ENABLE
          case BUS_CHANGE_BRIGHT:
            if (bus.counter < 1) {
              bus.counter = 1; //сместили указатель
              device.light = TWDR;
            }
            break;
#endif
          case BUS_CONTROL_DEVICE:
            if (bus.counter < 1) {
              bus.counter = 1; //сместили указатель
              bus.buffer[0] = TWDR;
            }
            break;
          case BUS_TEST_SOUND:
            if (bus.counter < 3) {
              bus.buffer[bus.counter] = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_SELECT_BYTE: //выбрать произвольное место записи
            if (!bus.counter) {
              bus.counter = TWDR; //установка места записи
              bus.comand = BUS_WAIT_DATA; //установка команды
            }
            break;
        }
        TWCR |= (0x01 << TWINT); //сбросили флаг прерывания
        break;
      case 0xA8: //принят SLA+R - передан ACK
      case 0xB8: //передан байт данных - принят ACK
        switch (bus.comand) {
          case BUS_READ_TIME: //передача настроек времени
            if (bus.counter < sizeof(RTC)) {
              TWDR = bus.buffer[bus.counter];
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_READ_FAST_SET: //передача быстрых настроек
            if (bus.counter < sizeof(fastSettings)) {
              TWDR = *((uint8_t*)&fastSettings + bus.counter);
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_READ_MAIN_SET: //передача основных настроек
            if (bus.counter < sizeof(mainSettings)) {
              TWDR = *((uint8_t*)&mainSettings + bus.counter);
              bus.counter++; //сместили указатель
            }
            break;
#if ALARM_TYPE
          case BUS_READ_ALARM_DATA: //передача настроек будильника
            if (bus.counter < (ALARM_MAX_ARR - 1)) {
              TWDR = bus.buffer[bus.counter];
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_READ_ALARM_NUM: //передача информации о будильниках
            if (bus.counter < 2) {
              TWDR = alarms.num;
              bus.counter++; //сместили указатель
            }
            break;
#endif
#if RADIO_ENABLE
          case BUS_READ_RADIO_SET: //передача настроек радио
            if (bus.counter < sizeof(radioSettings)) {
              TWDR = *((uint8_t*)&radioSettings + bus.counter);
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_READ_RADIO_POWER: //передача состояния радио
            if (!bus.counter) {
              TWDR = radio.powerState;
              bus.counter++; //сместили указатель
            }
            break;
#endif
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE
          case BUS_READ_TEMP: //передача температуры
            if (bus.counter < sizeof(sens)) {
              TWDR = *((uint8_t*)&sens + bus.counter);
              bus.counter++; //сместили указатель
            }
            break;
#endif
          case BUS_READ_EXTENDED_SET: //передача расширенных настроек
            if (bus.counter < sizeof(extendedSettings)) {
              TWDR = *((uint8_t*)&extendedSettings + bus.counter);
              bus.counter++; //сместили указатель
            }
            break;
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
          case BUS_READ_TIMER_SET: //передача настроек таймера
            if (bus.counter < sizeof(timer)) {
              TWDR = *((uint8_t*)&timer + bus.counter);
              bus.counter++; //сместили указатель
            }
            break;
#endif
          case BUS_READ_FAILURE: //передача сбоев при запуске устройства
            if (bus.counter < sizeof(device.failure)) {
              TWDR = *((uint8_t*)&device.failure + bus.counter);
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_READ_STATUS: //передача статуса часов
#if ALARM_TYPE
            if (alarms.now >= ALARM_WAIT) device.status |= (0x01 << STATUS_UPDATE_ALARM_STATE);
#endif
            TWDR = device.status;
            device.status = 0;
            break;
          case BUS_READ_DEVICE: //передача комплектации
            if (bus.counter < sizeof(deviceInformation)) {
              TWDR = deviceInformation[bus.counter];
              bus.counter++; //сместили указатель
            }
            break;
        }
        TWCR |= (0x01 << TWINT); //сбросили флаг прерывания
        break;
      case 0xC0: //передан байт данных - принят NACK
        TWCR |= (0x01 << TWINT); //сбросили флаг прерывания
        break;
#endif
      //case 0xA0: TWCR |= (0x01 << TWINT); break; //принят сигнал STOP
      case 0x08: //передан START
      case 0x10: //передан REPEATED START
      case 0x18: //передан SLA+W - принят ACK
      case 0x28: //передан байт данных - принят ACK
      case 0x40: //передан SLA+R - принят ACK
      case 0x50: //принят байт данных - передан ACK
      case 0x58: //принят байт данных - передан NACK
        return 2; //возвращаем статус готовности шины
      default: //неизвестная ошибка шины или сигнал STOP
#if ESP_ENABLE
        bus.status &= ~(0x01 << BUS_COMMAND_WAIT); //сбросили статус
        switch (bus.comand) {
          case BUS_WRITE_TIME: //настройки времени
#if DS3231_ENABLE
            bus.statusExt |= (0x01 << BUS_EXT_COMMAND_SEND_TIME);
#endif
            light_update = 1;
            for (uint8_t i = 0; i < sizeof(RTC); i++) *((uint8_t*)&RTC + i) = bus.buffer[i]; //устанавливаем время
            break;
          case BUS_WRITE_FAST_SET: memoryUpdate |= (0x01 << MEM_UPDATE_FAST_SET); bus.status |= (0x01 << BUS_COMMAND_UPDATE); break; //быстрые настройки
          case BUS_WRITE_MAIN_SET: memoryUpdate |= (0x01 << MEM_UPDATE_MAIN_SET); bus.status |= (0x01 << BUS_COMMAND_UPDATE); break; //основные настройки
#if ALARM_TYPE
          case BUS_WRITE_ALARM_DATA:
          case BUS_DEL_ALARM:
          case BUS_NEW_ALARM:
            switch (bus.comand) {
              case BUS_WRITE_ALARM_DATA: bus.buffer[ALARM_STATUS] = 255; alarmWriteBlock(bus.position, bus.buffer); break; //записываем настройки будильника
              case BUS_DEL_ALARM: delAlarm(bus.position); break; //удаляем выбранный будильник
              case BUS_NEW_ALARM: newAlarm(); break; //добавляем новый будильник
            }
            if (alarms.now < ALARM_WAIT) { //если не работает тревога
              checkAlarms(ALARM_CHECK_SET); //проверяем будильники на совпадение
              bus.status |= (0x01 << BUS_COMMAND_UPDATE);
            }
            break;
#endif
#if RADIO_ENABLE
          case BUS_WRITE_RADIO_STA: memoryUpdate |= (0x01 << MEM_UPDATE_RADIO_SET); bus.status |= (0x01 << BUS_COMMAND_UPDATE); break; //настройки радио
          case BUS_WRITE_RADIO_VOL: bus.statusExt |= (0x01 << BUS_EXT_COMMAND_RADIO_VOL); break; //настройка громкости радио
          case BUS_WRITE_RADIO_FREQ: bus.statusExt |= (0x01 << BUS_EXT_COMMAND_RADIO_FREQ); break; //настройка частоты радио
          case BUS_WRITE_RADIO_MODE: bus.status |= BUS_COMMAND_RADIO_MODE; break; //переключение режима радио
          case BUS_WRITE_RADIO_POWER: bus.status |= BUS_COMMAND_RADIO_POWER; break; //переключение питания радио
          case BUS_SEEK_RADIO_UP: bus.status |= BUS_COMMAND_RADIO_SEEK_UP; break; //запуск автопоиска радио
          case BUS_SEEK_RADIO_DOWN: bus.status |= BUS_COMMAND_RADIO_SEEK_DOWN; break; //запуск автопоиска радио
#endif
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE
          case BUS_CHECK_TEMP: _timer_ms[TMR_SENS] = 0; device.status &= ~(0x01 << STATUS_UPDATE_SENS_DATA); break; //запрос температуры
#endif
          case BUS_WRITE_EXTENDED_SET: memoryUpdate |= (0x01 << MEM_UPDATE_EXTENDED_SET); break; //расширенные настройки
          case BUS_SET_SHOW_TIME: _timer_sec[TMR_SHOW] = getPhaseTime(mainSettings.autoShowTime, AUTO_SHOW_PHASE); break; //установка таймера показа температуры
          case BUS_SET_BURN_TIME: _timer_sec[TMR_BURN] = getPhaseTime(mainSettings.burnTime, BURN_PHASE); break; //установка таймера антиотравления
          case BUS_SET_UPDATE: bus.status |= (0x01 << BUS_COMMAND_UPDATE); break; //установка флага обновления
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
          case BUS_WRITE_TIMER_MODE: bus.status |= BUS_COMMAND_TIMER_MODE; break; //переключение в режим таймера
#endif
#if (DS3231_ENABLE != 2) && !SENS_AHT_ENABLE && !SENS_SHT_ENABLE && !SENS_BME_ENABLE && !SENS_PORT_ENABLE
          case BUS_WRITE_SENS_DATA: for (uint8_t i = 0; i < sizeof(sens); i++) *((uint8_t*)&sens + i) = bus.buffer[i]; break; //копирование температуры
#endif
          case BUS_WRITE_MAIN_SENS_DATA: for (uint8_t i = 0; i < sizeof(mainSens); i++) *((uint8_t*)&mainSens + i) = bus.buffer[i]; break; //копирование температуры
#if ALARM_TYPE
          case BUS_ALARM_DISABLE: //отключение будильника
            if (alarms.now >= ALARM_WAIT) { //если будильник активен
#if PLAYER_TYPE
              playerStop(); //сброс воспроизведения плеера
#else
              melodyStop(); //сброс воспроизведения мелодии
#endif
              alarmDisable(); //отключить будильник
            }
            break;
#endif
#if !LIGHT_SENS_ENABLE
          case BUS_CHANGE_BRIGHT: light_update = 1; break; //смена яркости
#endif
          case BUS_TEST_FLIP: animShow = ANIM_DEMO; bus.status |= (0x01 << BUS_COMMAND_UPDATE); break; //тест анимации минут
          case BUS_TEST_SOUND: //тест звука
            if ((mainTask == MAIN_PROGRAM) || (mainTask == SLEEP_PROGRAM)) { //если в режиме часов или спим
#if PLAYER_TYPE
              playerSetVoice(mainSettings.voiceSound);
              if (!player.playbackMute) {
                bus.status |= (0x01 << BUS_COMMAND_UPDATE);
                playerStop(); //сброс воспроизведения плеера
                playerSetVolNow(bus.buffer[0]);
                playerSetTrackNow(bus.buffer[1], bus.buffer[2]);
                playerRetVol(mainSettings.volumeSound);
              }
              else playerSetVolNow(mainSettings.volumeSound);
#else
#if RADIO_ENABLE
              if (radio.powerState == RDA_OFF) { //если радио выключено
#endif
                bus.status |= (0x01 << BUS_COMMAND_UPDATE);
                melodyPlay(bus.buffer[1], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#if RADIO_ENABLE
              }
#endif
#endif
            }
            break;
          case BUS_STOP_SOUND: //остановка звука
            if ((mainTask == MAIN_PROGRAM) || (mainTask == SLEEP_PROGRAM)) { //если в режиме часов или спим
#if PLAYER_TYPE
              playerStop(); //сброс воспроизведения плеера
#else
              melodyStop(); //сброс воспроизведения мелодии
#endif
            }
            break;
          case BUS_CONTROL_DEVICE:
            if (bus.counter == 1) {
              switch (bus.buffer[0]) {
                case DEVICE_RESET:
                  EEPROM_UpdateByte(EEPROM_BLOCK_CRC_DEFAULT, EEPROM_ReadByte(EEPROM_BLOCK_CRC_DEFAULT) ^ 0xFF); //сбрасываем настройки
                  RESET_SYSTEM; //перезагрузка
                  break;
                case DEVICE_UPDATE:
                  GPIOR0 = BOOTLOADER_START; //устанавливаем запрос обновления прошивки
                  RESET_SYSTEM; //перезагрузка
                  break;
                case DEVICE_REBOOT: RESET_SYSTEM; break; //перезагрузка
              }
            }
            break;
        }
#endif
        TWCR |= (0x01 << TWINT); //сбросили флаг прерывания
        break;
    }
  }
  return 0; //возвращаем статус ожидания шины
}
//----------------------------------Системная задача------------------------------------------------
void systemTask(void) //системная задача
{
  static uint16_t timeClock; //счетчик реального времени
  static uint16_t timerCorrect; //остаток для коррекции времени
#if DS3231_ENABLE || SQW_PORT_ENABLE
  static uint16_t timerSQW = SQW_MIN_TIME; //таймер контроля сигнала SQW
#endif

#if BACKL_TYPE == 3
  backlEffect(); //анимация подсветки
  showLeds(); //отрисовка светодиодов
#elif BACKL_TYPE
  backlFlash(); //"дыхание" подсветки
#endif

  dotEffect(); //анимации точек

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

#if (GEN_ENABLE && (GEN_FEEDBACK == 1)) || BTN_TYPE || (LIGHT_SENS_ENABLEE && LIGHT_SENS_TYPE == 1)
  analogUpdate(); //обработка аналоговых входов
#endif

#if (GEN_ENABLE && (GEN_FEEDBACK == 2))
  if (ACSR & (0x01 << ACI)) { //если изменилось состояние на входе
    ACSR |= (0x01 << ACI); //сбрасываем флаг прерывания
#if CONV_PIN == 9
    if (ACSR & (0x01 << ACO)) TCCR1A |= (0x01 << COM1A1); //включаем шим преобразователя
    else {
      TCCR1A &= ~(0x01 << COM1A1); //выключаем шим преобразователя
      CONV_OFF; //выключаем пин преобразователя
    }
#elif CONV_PIN == 10
    if (ACSR & (0x01 << ACO)) TCCR1A |= (0x01 << COM1B1); //включаем шим преобразователя
    else {
      TCCR1A &= ~(0x01 << COM1B1); //выключаем шим преобразователя
      CONV_OFF; //выключаем пин преобразователя
    }
#endif
  }
#endif

#if PCF8591_ENABLE
    if (!_timer_ms[TMR_PCF8591]) { //если таймаут нового запроса вышел
      if (!readAnalogInputChannelsPCF()) {
        SET_ERROR(PCF8591_ERROR);
      }
      _timer_ms[TMR_PCF8591] = PCF8591_UPDATE_TIME; //установили таймаут
    }
#endif

#if PCF8591_ENABLE && LIGHT_SENS_ENABLE && LIGHT_SENS_TYPE == 2
  adc_light = LIGTH_VALUE; //записываем результат опроса
#endif

#if LIGHT_SENS_ENABLE && LIGHT_SENS_TYPE == 1
  lightSensCheck(); //проверка сенсора яркости освещения
#endif

#if ESP_ENABLE
  busUpdate(); //обновление статуса шины
#endif

  for (uint8_t _tick = tick_ms; _tick > 0; _tick--) { //если был тик то обрабатываем данные
    tick_ms--; //убавили счетчик миллисекунд

    indiStateCheck(); //проверка состояния динамической индикации
    uint8_t button = buttonStateUpdate(); //обновление состояния кнопок
    if (button) btn.state = button; //скопировали новую кнопку

    timerCorrect += debugSettings.timePeriod; //прибавляем период для коррекции
    uint8_t msDec = timerCorrect / 1000; //находим целые мс
    for (uint8_t tm = 0; tm < TIMERS_MS_NUM; tm++) { //опрашиваем все таймеры
      if (_timer_ms[tm]) { //если таймер активен
        if (_timer_ms[tm] > msDec) _timer_ms[tm] -= msDec; //если таймер больше периода
        else _timer_ms[tm] = 0; //иначе сбрасываем таймер
      }
    }
    timerCorrect %= 1000; //оставляем коррекцию

#if DS3231_ENABLE || SQW_PORT_ENABLE
    if (EIMSK) { //если работаем от внешнего тактирования
      timerSQW += msDec; //прибавили время
      if (timerSQW > SQW_MAX_TIME) { //если сигнал слишком длинный
        EIMSK = 0; //перешли на внутреннее тактирование
        tick_sec = 1; //установили секунду
        SET_ERROR(SQW_LONG_ERROR); //устанавливаем ошибку длинного сигнала
      }
    }
    else { //если внешние тактирование не обнаружено
#endif
      timeClock += msDec; //добавляем ко времени период таймера
      if (timeClock >= 1000) { //если прошла секунда
        timeClock -= 1000; //оставляем остаток
        tick_sec++; //прибавляем секунду
      }
#if DS3231_ENABLE || SQW_PORT_ENABLE
    }
#endif
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

#if DS3231_ENABLE || SQW_PORT_ENABLE
    if (EIMSK) { //если работаем от внешнего тактирования
      if (timerSQW < SQW_MIN_TIME) { //если сигнал слишком короткий
        EIMSK = 0; //перешли на внутреннее тактирование
        tick_sec = 0; //сбросили счетчик секунд
        timeClock = timerSQW; //установили таймер секунды
        SET_ERROR(SQW_SHORT_ERROR); //устанавливаем ошибку короткого сигнала
        return; //выходим
      }
      timerSQW = 0; //сбросили таймер
    }
#endif
#if DS3231_ENABLE
    else if (!_timer_sec[TMR_SYNC] && RTC.s == RTC_SYNC_PHASE) { //если работаем от внутреннего тактирования
      _timer_sec[TMR_SYNC] = ((uint16_t)RTC_SYNC_TIME * 60); //установили таймер
      if (getTime(RTC_CHECK_OSF)) RTC.s--; //синхронизируем время
    }
#endif

    indi.update = dot.update = 0; //очищаем флаги секунды и точек

#if LAMP_NUM > 4
    if (animShow == ANIM_NULL) animShow = ANIM_SECS; //показать анимацию переключения цифр
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
                RTC.YY = 2000; //сбросили год
              }
            }
          }
        }
        hourSound(); //звук смены часа
        light_update = 1; //устанавливаем флаг изменения яркости
      }
      if (fastSettings.flipMode && (animShow < ANIM_MAIN)) animShow = ANIM_MINS; //показать анимацию переключения цифр
#if ALARM_TYPE
      checkAlarms(ALARM_CHECK_MAIN); //проверяем будильники на совпадение
#endif
    }
#if LIGHT_SENS_ENABLE
    lightSensUpdate(); //обработка сенсора яркости освещения
#endif
#if MOV_PORT_ENABLE
    if (indi.sleepMode && MOV_CHK) {
      sleepReset(); //установли время ожидания режима сна
      if (mainTask == SLEEP_PROGRAM) indi.sleepMode = SLEEP_DISABLE; //отключаем сон если в режиме сна
    }
#endif

    if (light_update) { //если нужно изменить яркость
      light_update = 0; //сбрасываем флаг изменения яркости
      changeBright(); //установка текущей яркости
    }

#if (NEON_DOT != 3) && DOTS_PORT_ENABLE && (ESP_ENABLE || DEFAULT_DOT_EXT_MODE)
    dotFlash(); //мигание точек
#endif

    RESET_WDT; //сбрасываем таймер WDT
  }
}
//----------------------------------Обработка данных------------------------------------------------
void dataUpdate(void) //обработка данных
{
  systemTask(); //системная задача
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE
  updateTemp(); //обновить показания температуры
#endif
  updateMemory(); //обновить данные в памяти
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

#if INDI_SYMB_TYPE
  indiSetSymb(SYMB_MENU); //установка индикатора символов
#endif

  while (1) {
    dataUpdate(); //обработка данных

#if ESP_ENABLE
    if (busCheck()) return MAIN_PROGRAM;
#endif

    if (!indi.update) {
      indi.update = 1;
      if (++time_out >= SETTINGS_TIMEOUT) {
#if DS3231_ENABLE
        sendTime(); //отправить время в RTC
#endif
#if ESP_ENABLE
        device.status |= (0x01 << STATUS_UPDATE_TIME_SET); //установили статус актуального времени
#endif
        animShow = ANIM_MAIN; //установили флаг анимации
        return MAIN_PROGRAM;
      }
    }

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

      indiClr(); //очистка индикаторов
#if LAMP_NUM > 4
      indiPrintNum(cur_mode + 1, 5); //режим
#endif
      switch (cur_mode) {
        case 0:
        case 1:
          indiPrintMenuData(blink_data, cur_mode, RTC.h, 0, RTC.m, 2); //вывод часов/минут
          break;
        case 2:
        case 3:
          indiPrintMenuData(blink_data, cur_mode & 0x01, RTC.DD, 0, RTC.MM, 2); //вывод даты/месяца
          break;
        case 4:
          if (!blink_data) indiPrintNum(RTC.YY, 0); //вывод года
          break;
      }
#if BACKL_TYPE == 3
      setBacklHue((cur_mode & 0x01) * 2, (cur_mode != 4) ? 2 : 4, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); //подсветка активных разрядов
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
          case 4: if (RTC.YY > 2000) RTC.YY--; else RTC.YY = 2099; break; //год
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
          case 4: if (RTC.YY < 2099) RTC.YY++; else RTC.YY = 2000; break; //год
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
#if DS3231_ENABLE
        sendTime(); //отправить время в RTC
#endif
#if ESP_ENABLE
        device.status |= (0x01 << STATUS_UPDATE_TIME_SET); //установили статус актуального времени
#endif
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

#if INDI_SYMB_TYPE
  indiSetSymb(SYMB_MENU); //установка индикатора символов
#endif

  while (1) {
    dataUpdate(); //обработка данных

#if ESP_ENABLE
    if (busCheck()) {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
      if ((cur_mode == 3) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
#if PLAYER_TYPE
      playerStop(); //сброс воспроизведения мелодии
#else
      melodyStop(); //сброс воспроизведения мелодии
#endif
      return MAIN_PROGRAM; //выходим
    }
#endif

#if PLAYER_TYPE
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
    if (cur_mode == 3 && !alarm[ALARM_RADIO] && !playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarm[ALARM_SOUND], PLAYER_ALARM_FOLDER); //воспроизводим мелодию будильника
#else
    if (cur_mode == 3 && !playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarm[ALARM_SOUND], PLAYER_ALARM_FOLDER); //воспроизводим мелодию будильника
#endif
#endif

    if (!indi.update) {
      indi.update = 1;
      if (++time_out >= SETTINGS_TIMEOUT) {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
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
#if LAMP_NUM > 4
      indiPrintNum(cur_mode + 1, 5); //режим
#endif
      switch (cur_mode) {
        case 0:
          indiPrintMenuData(blink_data, cur_indi, alarm[ALARM_HOURS], 0, alarm[ALARM_MINS], 2); //вывод часов/минут
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
#if PLAYER_TYPE
          indiPrintMenuData(blink_data, cur_indi, alarm[ALARM_VOLUME], 0, alarm[ALARM_SOUND] + !alarm[ALARM_RADIO], 2); //вывод громкости мелодии/номера мелодии
#else
          if (alarm[ALARM_RADIO]) indiPrintMenuData(blink_data, cur_indi, alarm[ALARM_VOLUME], 0, alarm[ALARM_SOUND], 2); //вывод громкости радио/номера радиостанции
          else if (!blink_data) indiPrintNum(alarm[ALARM_SOUND] + 1, 2, 2, 0); //вывод номера мелодии
#endif
#else
#if PLAYER_TYPE
          indiPrintMenuData(blink_data, cur_indi, alarm[ALARM_VOLUME], 0, alarm[ALARM_SOUND] + 1, 2); //вывод громкости мелодии/номера мелодии
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] > 0) alarm[ALARM_VOLUME]--; else alarm[ALARM_VOLUME] = MAIN_MAX_VOL; //громкость
                if (alarm[ALARM_RADIO]) setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //устанавливаем громкость
                else playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
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
                if (alarm[ALARM_VOLUME] > 0) alarm[ALARM_VOLUME]--; else alarm[ALARM_VOLUME] = MAIN_MAX_VOL; //громкость
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] < MAIN_MAX_VOL) alarm[ALARM_VOLUME]++; else alarm[ALARM_VOLUME] = 0; //громкость
                if (alarm[ALARM_RADIO]) setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //устанавливаем громкость
                else playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
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
                if (alarm[ALARM_VOLUME] < MAIN_MAX_VOL) alarm[ALARM_VOLUME]++; else alarm[ALARM_VOLUME] = 0; //громкость
                playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
        if ((cur_mode == 3) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
        if (cur_mode > 0) cur_mode--; else cur_mode = 3; //переключение пунктов

        if ((cur_mode == 2) && (alarm[ALARM_MODE] < 4)) cur_mode = 1; //если нет дней недели
        if (cur_mode == 3) { //если режим настройки мелодии
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
          if (alarm[ALARM_RADIO] && (getPowerStatusRDA() != RDA_ERROR)) { //если режим радиобудильника
            setPowerRDA(RDA_ON); //включаем радио
            setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //устанавливаем громкость
            setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
          }
          else { //иначе обычный режим
            alarm[ALARM_RADIO] = 0; //обычный режим
            setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
            playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
        if ((cur_mode == 3) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
        if (cur_mode < 3) cur_mode++; else cur_mode = 0; //переключение пунктов

        if ((cur_mode == 2) && (alarm[ALARM_MODE] < 4)) cur_mode = 3; //если нет дней недели
        if (cur_mode == 3) { //если режим настройки мелодии
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
          if (alarm[ALARM_RADIO] && (getPowerStatusRDA() != RDA_ERROR)) { //если режим радиобудильника
            setPowerRDA(RDA_ON); //включаем радио
            setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //устанавливаем громкость
            setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
          }
          else { //иначе обычный режим
            alarm[ALARM_RADIO] = 0; //обычный режим
            setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
            playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
        if ((cur_mode == 3) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
#if ESP_ENABLE
        device.status |= (0x01 << STATUS_UPDATE_ALARM_SET);
#endif
        alarm[ALARM_STATUS] = 255; //установили статус изменения будильника
        alarmWriteBlock(1, alarm); //записать блок основных данных будильника и выйти
        return MAIN_PROGRAM;

#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
      case ADD_KEY_HOLD: //удержание дополнительной кнопки
        if (cur_mode == 3) {
          if (alarm[ALARM_RADIO]) { //если режим радиобудильника
            alarm[ALARM_RADIO] = 0;
            alarm[ALARM_SOUND] = 0;
            setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
            playerSetVolNow(alarm[ALARM_VOLUME]); //установка громкости
#endif
          }
          else if (getPowerStatusRDA() != RDA_ERROR) { //иначе если радиоприемник доступен
            alarm[ALARM_RADIO] = 1;
            alarm[ALARM_SOUND] = 0;
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
  uint8_t cur_alarm = alarms.num > 0;

  alarmReset(); //сброс будильника
  alarmReadBlock(cur_alarm, alarm); //читаем блок данных

  _timer_ms[TMR_MS] = 0; //сбросили таймер

#if BACKL_TYPE == 3
  backlAnimDisable(); //запретили эффекты подсветки
  setLedBright(backl.menuBright); //установили максимальную яркость
#endif

#if PLAYER_TYPE
  if (mainSettings.knockSound) playerSetTrackNow(PLAYER_ALARM_SET_SOUND, PLAYER_GENERAL_FOLDER); //воспроизводим название меню
#endif

#if INDI_SYMB_TYPE
  indiSetSymb(SYMB_MENU); //установка индикатора символов
#endif

  while (1) {
    dataUpdate(); //обработка данных

#if ESP_ENABLE
    if (busCheck()) {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
      if ((cur_mode == 4) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
#if PLAYER_TYPE
      playerStop(); //сброс воспроизведения мелодии
#else
      melodyStop(); //сброс воспроизведения мелодии
#endif
      return MAIN_PROGRAM; //выходим
    }
#endif

#if PLAYER_TYPE
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
    if (cur_mode == 4 && !alarm[ALARM_RADIO] && !playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarm[ALARM_SOUND], PLAYER_ALARM_FOLDER); //воспроизводим мелодию будильника
#else
    if (cur_mode == 4 && !playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarm[ALARM_SOUND], PLAYER_ALARM_FOLDER); //воспроизводим мелодию будильника
#endif
#endif

    if (!indi.update) {
      indi.update = 1;
      if (++time_out >= SETTINGS_TIMEOUT) {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
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
#if LAMP_NUM > 4
      indiPrintNum(cur_mode + 1, 5); //режим
#endif
      switch (cur_mode) {
        case 0:
          if (!blink_data) indiPrintNum(cur_alarm, 0, 2, 0); //вывод номера будильника
          if (cur_alarm) indiPrintNum(alarm[ALARM_MODE], 3); //вывод режима
          break;
        case 1:
          indiPrintMenuData(blink_data, cur_indi, alarm[ALARM_HOURS], 0, alarm[ALARM_MINS], 2); //вывод часов/минут
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
#if PLAYER_TYPE
          indiPrintMenuData(blink_data, cur_indi, alarm[ALARM_VOLUME], 0, alarm[ALARM_SOUND] + !alarm[ALARM_RADIO], 2); //вывод громкости мелодии/номера мелодии
#else
          if (alarm[ALARM_RADIO]) indiPrintMenuData(blink_data, cur_indi, alarm[ALARM_VOLUME], 0, alarm[ALARM_SOUND], 2); //вывод громкости радио/номера радиостанции
          else if (!blink_data) indiPrintNum(alarm[ALARM_SOUND] + 1, 2, 2, 0); //вывод номера мелодии
#endif
#else
#if PLAYER_TYPE
          indiPrintMenuData(blink_data, cur_indi, alarm[ALARM_VOLUME], 0, alarm[ALARM_SOUND] + 1, 2); //вывод громкости мелодии/номера мелодии
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
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
          case 0: if (cur_alarm > (alarms.num > 0)) cur_alarm--; else cur_alarm = alarms.num; alarmReadBlock(cur_alarm, alarm); break; //будильник

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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] > 0) alarm[ALARM_VOLUME]--; else alarm[ALARM_VOLUME] = MAIN_MAX_VOL; //громкость
                if (alarm[ALARM_RADIO]) setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //устанавливаем громкость
                else playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
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
                if (alarm[ALARM_VOLUME] > 0) alarm[ALARM_VOLUME]--; else alarm[ALARM_VOLUME] = MAIN_MAX_VOL; //громкость
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
          case 0: if (cur_alarm < alarms.num) cur_alarm++; else cur_alarm = (alarms.num > 0); alarmReadBlock(cur_alarm, alarm); break; //будильник

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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] < MAIN_MAX_VOL) alarm[ALARM_VOLUME]++; else alarm[ALARM_VOLUME] = 0; //громкость
                if (alarm[ALARM_RADIO]) setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //устанавливаем громкость
                else playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
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
                if (alarm[ALARM_VOLUME] < MAIN_MAX_VOL) alarm[ALARM_VOLUME]++; else alarm[ALARM_VOLUME] = 0; //громкость
                playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
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
        if (!cur_mode && cur_alarm) {
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
          if (cur_alarm) { //если есть будильники в памяти
            delAlarm(cur_alarm); //удалить текущий будильник
            dotSetBright(dot.menuBright); //включаем точки
            for (_timer_ms[TMR_MS] = 500; _timer_ms[TMR_MS];) systemTask(); //обработка данных
            dotSetBright(0); //выключаем точки
            if (cur_alarm > (alarms.num > 0)) cur_alarm--; //убавляем номер текущего будильника
            else cur_alarm = (alarms.num > 0);
            alarmReadBlock(cur_alarm, alarm); //читаем блок данных
#if ESP_ENABLE
            device.status |= (0x01 << STATUS_UPDATE_ALARM_SET);
#endif
          }
        }
        else {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
          if ((cur_mode == 4) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
          if (cur_mode > 1) cur_mode--; else cur_mode = 4; //переключение пунктов

          if ((cur_mode == 3) && (alarm[ALARM_MODE] < 4)) cur_mode = 2; //если нет дней недели
          if (cur_mode == 4) { //если режим настройки мелодии
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
            if (alarm[ALARM_RADIO] && (getPowerStatusRDA() != RDA_ERROR)) { //если режим радиобудильника
              setPowerRDA(RDA_ON); //включаем радио
              setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //устанавливаем громкость
              setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
            }
            else { //иначе обычный режим
              alarm[ALARM_RADIO] = 0; //обычный режим
              setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
              playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
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
          for (_timer_ms[TMR_MS] = 500; _timer_ms[TMR_MS];) systemTask(); //обработка данных
          dotSetBright(0); //выключаем точки
          cur_alarm = alarms.num;
          alarmReadBlock(cur_alarm, alarm); //читаем блок данных
#if ESP_ENABLE
          device.status |= (0x01 << STATUS_UPDATE_ALARM_SET);
#endif
        }
        else {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
          if ((cur_mode == 4) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
          if (cur_mode < 4) cur_mode++; else cur_mode = 1; //переключение пунктов

          if ((cur_mode == 3) && (alarm[ALARM_MODE] < 4)) cur_mode = 4; //если нет дней недели
          if (cur_mode == 4) { //если режим настройки мелодии
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
            if (alarm[ALARM_RADIO] && (getPowerStatusRDA() != RDA_ERROR)) { //если режим радиобудильника
              setPowerRDA(RDA_ON); //включаем радио
              setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //устанавливаем громкость
              setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
            }
            else { //иначе обычный режим
              alarm[ALARM_RADIO] = 0; //обычный режим
              setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
              playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
          if ((cur_mode == 4) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
#if ESP_ENABLE
          device.status |= (0x01 << STATUS_UPDATE_ALARM_SET);
#endif
          alarm[ALARM_STATUS] = 255; //установили статус изменения будильника
          alarmWriteBlock(cur_alarm, alarm); //записать блок основных данных будильника
          dotSetBright(0); //выключаем точки
          cur_mode = 0; //выбор будильника
          blink_data = 0; //сбрасываем флаги
          _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        }
        break;

#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
      case ADD_KEY_HOLD: //удержание дополнительной кнопки
        if (cur_mode == 4) {
          if (alarm[ALARM_RADIO]) { //если режим радиобудильника
            alarm[ALARM_RADIO] = 0;
            alarm[ALARM_SOUND] = 0;
            setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
            playerSetVolNow(alarm[ALARM_VOLUME]); //установка громкости
#endif
          }
          else if (getPowerStatusRDA() != RDA_ERROR) { //иначе если радиоприемник доступен
            alarm[ALARM_RADIO] = 1;
            alarm[ALARM_SOUND] = 0;
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
  uint8_t anim_demo = 0; //флаг демонстрации анимации
  uint8_t time_out = 0; //таймаут автовыхода

  _timer_ms[TMR_MS] = 0; //сбросили таймер

#if BACKL_TYPE == 3
  backlAnimDisable(); //запретили эффекты подсветки
  setLedBright(backl.menuBright); //установили максимальную яркость
#endif

#if PLAYER_TYPE
  if (mainSettings.knockSound) playerSetTrackNow(PLAYER_MAIN_MENU_START, PLAYER_MENU_FOLDER); //воспроизводим название меню
#endif

#if INDI_SYMB_TYPE
  indiSetSymb(SYMB_MENU); //установка индикатора символов
#endif

  while (1) {
    dataUpdate(); //обработка данных

#if ESP_ENABLE
    if (busCheck()) return MAIN_PROGRAM;
#endif

    if (!indi.update) { //если установлен флаг
      indi.update = 1; //сбрасываем флаг
      if (++time_out >= SETTINGS_TIMEOUT) {
        setUpdateMemory(0x01 << MEM_UPDATE_MAIN_SET); //записываем основные настройки в память
        animShow = ANIM_MAIN; //установили флаг анимации
        return MAIN_PROGRAM;
      }
    }

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

      indiClr(); //очистка индикаторов
      if (!set) {
        indiPrintNum(cur_mode + 1, (LAMP_NUM / 2 - 1), 2, 0); //вывод режима
#if BACKL_TYPE == 3
        setBacklHue((LAMP_NUM / 2 - 1), 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); //подсветка активных разрядов
#endif
      }
      else {
        if (anim_demo == 1) { //если нужно отобразить демонстрацию эффекта
          anim_demo = 0; //сбросили флаг демонстрации
#if BACKL_TYPE == 3
          setLedHue(BACKL_MENU_COLOR_1, WHITE_ON); //подсветка активных разрядов
#endif
          switch (cur_mode) {
            case SET_AUTO_SHOW: animIndi(mainSettings.autoShowFlip, FLIP_DEMO); break; //демонстрация анимации показа температуры
            case SET_BURN_MODE:
              burnIndi(mainSettings.burnMode, BURN_DEMO); //демонстрация антиотравления индикаторов
              dotSetBright(dot.menuBright); //включаем точки
              break;
          }
          _timer_ms[TMR_MS] = blink_data = 0; //сбрасываем флаги
        }
        else {
          if (anim_demo > 1) { //если нужно отобразить анимацию
            anim_demo = 1; //перешли в режим анимации
            _timer_ms[TMR_MS] = SETTINGS_WAIT_TIME; //устанавливаем таймер
          }
#if LAMP_NUM > 4
          indiPrintNum(cur_mode + 1, 4, 2); //режим
#endif
          switch (cur_mode) {
#if PLAYER_TYPE
            case SET_TIME_FORMAT: //вывод формата времени
              indiPrintMenuData(blink_data, cur_indi, (mainSettings.timeFormat) ? 12 : 24, 0, mainSettings.glitchMode, 3); //вывод формата времени/режима глюков
              break;
            case SET_GLITCH_MODE: //вывод озвучки
              indiPrintMenuData(blink_data, cur_indi, mainSettings.volumeSound, 0, mainSettings.voiceSound, 3); //вывод громкости озвучки/голоса озвучки
              break;
            case SET_BTN_SOUND: //вывод озвучки
              indiPrintMenuData(blink_data, cur_indi, (mainSettings.hourSound & 0x03) + ((mainSettings.hourSound & 0x80) ? 10 : 0), 0, mainSettings.knockSound, 3); //вывод озвучки смены часа/действий
              break;
#else
            case SET_TIME_FORMAT: if (!blink_data) indiPrintNum((mainSettings.timeFormat) ? 12 : 24, 0); break; //вывод формата времени
            case SET_GLITCH_MODE: if (!blink_data) indiPrintNum(mainSettings.glitchMode, 3); break; //вывод глюков
            case SET_BTN_SOUND: if (!blink_data) indiPrintNum(mainSettings.knockSound, 3); break; //звук кнопок или озвучка
#endif
            case SET_HOUR_TIME:
              indiPrintMenuData(blink_data, cur_indi, mainSettings.timeHour[TIME_NIGHT], 0, mainSettings.timeHour[TIME_DAY], 2); //вывод часа начала звукового оповещения нового часа/окончания звукового оповещения нового часа
              break;
            case SET_BRIGHT_TIME:
              indiPrintMenuData(blink_data, cur_indi, mainSettings.timeBright[TIME_NIGHT], 0, mainSettings.timeBright[TIME_DAY], 2); //вывод часа начала ночной посветки/окончания ночной посветки
              break;
            case SET_INDI_BRIGHT:
              indiPrintMenuData(blink_data, cur_indi, mainSettings.indiBright[TIME_NIGHT], 0, mainSettings.indiBright[TIME_DAY], 2); //вывод яркости индикаторов ночь/день
              break;
#if BACKL_TYPE
            case SET_BACKL_BRIGHT:
              indiPrintMenuData(blink_data, cur_indi, mainSettings.backlBright[TIME_NIGHT] / 10, 0, mainSettings.backlBright[TIME_DAY] / 10, 2); //вывод яркости подсветки ночь/день
              break;
#endif
            case SET_DOT_BRIGHT:
#if (NEON_DOT != 3) || !DOTS_PORT_ENABLE
              indiPrintMenuData(blink_data, cur_indi, mainSettings.dotBright[TIME_NIGHT] / 10, 0, mainSettings.dotBright[TIME_DAY] / 10, 2); //вывод яркости точек ночь/день
#else
              if (!blink_data) indiPrintNum((boolean)mainSettings.dotBright[TIME_NIGHT], 3); //вывод яркости ночь
#endif
              break;
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
            case SET_CORRECT_SENS: {
                if (!blink_data) {
#if ESP_ENABLE
                  uint16_t temperature = getTemperature((extendedSettings.tempCorrectSensor == 2) ? SHOW_TEMP_ESP : SHOW_TEMP); //буфер температуры
#else
                  uint16_t temperature = getTemperature(); //буфер температуры
#endif
                  if (temperature > 990) indiPrintNum(0, 0); //вывод ошибки
                  else indiPrintNum(temperature, 1, 2, 0); //вывод температуры
                }
              }
              break;
#endif
            case SET_AUTO_SHOW:
              indiPrintMenuData(blink_data, cur_indi, mainSettings.autoShowTime, 0, mainSettings.autoShowFlip, 2); //вывод времени автопоказа/анимации автопоказа
              break;
            case SET_BURN_MODE:
              indiPrintMenuData(blink_data, cur_indi, mainSettings.burnTime, 0, mainSettings.burnMode, 3); //вывод анимации антиотравления/анимации секунд
              break;
            case SET_SLEEP_TIME:
              indiPrintMenuData(blink_data, cur_indi, mainSettings.timeSleep[TIME_NIGHT], 0, mainSettings.timeSleep[TIME_DAY], 2); //вывод времени сна ночь/ночь
              break;
          }

#if BACKL_TYPE == 3
          switch (cur_mode) {
#if PLAYER_TYPE
            case SET_TIME_FORMAT:
            case SET_GLITCH_MODE:
            case SET_BTN_SOUND:
              setBacklHue((cur_indi) ? 3 : 0, (cur_indi) ? 1 : 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
#endif
#if (NEON_DOT == 3) && DOTS_PORT_ENABLE
            case SET_DOT_BRIGHT:
#endif
#if !PLAYER_TYPE
            case SET_GLITCH_MODE:
            case SET_BTN_SOUND:
#endif
#if ((NEON_DOT == 3) && DOTS_PORT_ENABLE) || !PLAYER_TYPE
              setBacklHue(3, 1, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
#endif
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
            case SET_CORRECT_SENS: setBacklHue(0, 3, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
#endif
            case SET_BURN_MODE: setBacklHue((cur_indi) ? 3 : 0, (cur_indi) ? 1 : 3, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
            default: setBacklHue(cur_indi * 2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
          }
#endif
          blink_data = !blink_data; //мигание сигментами
        }
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
              case SET_TIME_FORMAT:
#if PLAYER_TYPE
                switch (cur_indi) {
                  case 0: mainSettings.timeFormat = 0; break; //формат времени
                  case 1: mainSettings.glitchMode = 0; break; //глюки
                }
#else
                mainSettings.timeFormat = 0; //формат времени
#endif
                break;
              case SET_GLITCH_MODE: //озвучка
#if PLAYER_TYPE
                switch (cur_indi) {
                  case 0: if (mainSettings.volumeSound > MAIN_MIN_VOL) mainSettings.volumeSound--; playerSetVolNow(mainSettings.volumeSound); playerSetTrackNow(PLAYER_TEST_VOL_SOUND, PLAYER_GENERAL_FOLDER); break; //установили громкость
                  case 1: //голос озвучки
                    if (mainSettings.voiceSound > 0) {
                      mainSettings.voiceSound--;
                      playerSetVoice(mainSettings.voiceSound);
                      playerSetTrackNow(PLAYER_VOICE_SOUND, PLAYER_GENERAL_FOLDER);
                    }
                    break;
                }
#else
                mainSettings.glitchMode = 0; //глюки
#endif
                break;

              case SET_BTN_SOUND: //звук кнопок
#if PLAYER_TYPE
                switch (cur_indi) {
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
                  case 0: if (mainSettings.hourSound & 0x80) mainSettings.hourSound &= ~0x80; else mainSettings.hourSound |= 0x80; break; //установили озвучку темепературы
#endif
                  case 1: mainSettings.knockSound = 0; break; //выключили озвучку действий
                }
#else
                if (!mainSettings.knockSound) buzz_pulse(KNOCK_SOUND_FREQ, KNOCK_SOUND_TIME); //щелчок пищалкой
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
                indiSetBright(mainSettings.indiBright[cur_indi]); //установка общей яркости индикаторов
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
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
              case SET_CORRECT_SENS: //настройка коррекции температуры
                if (mainSettings.tempCorrect > -127) mainSettings.tempCorrect--; else mainSettings.tempCorrect = 127;
                break;
#endif
              case SET_AUTO_SHOW: //автопоказ температуры
                switch (cur_indi) {
                  case 0:
                    if (mainSettings.autoShowTime > 0) mainSettings.autoShowTime--; else mainSettings.autoShowTime = 15;
                    _timer_sec[TMR_SHOW] = getPhaseTime(mainSettings.autoShowTime, AUTO_SHOW_PHASE); //установка таймера показа температуры
                    break;
                  case 1:
                    if (mainSettings.autoShowFlip > 0) mainSettings.autoShowFlip--; else mainSettings.autoShowFlip = (FLIP_EFFECT_NUM + 1); //устанавливаем анимацию автопоказа температуры
                    if (mainSettings.autoShowFlip > 1) anim_demo = 2; //установили флаг демонстрации анимации
                    break;
                }
                break;
              case SET_BURN_MODE: //анимация антиотравления индикаторов
                switch (cur_indi) {
                  case 0:
                    if (mainSettings.burnTime > 10) mainSettings.burnTime -= 5; else mainSettings.burnTime = 180;
                    _timer_sec[TMR_BURN] = getPhaseTime(mainSettings.burnTime, BURN_PHASE); //установка таймера антиотравления
                    break;
                  case 1:
                    if (mainSettings.burnMode) mainSettings.burnMode--; else mainSettings.burnMode = (BURN_EFFECT_NUM - 1);
                    anim_demo = 2; //установили флаг демонстрации анимации
                    break;
                }
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
              case SET_TIME_FORMAT:
#if PLAYER_TYPE
                switch (cur_indi) {
                  case 0: mainSettings.timeFormat = 1; break; //формат времени
                  case 1: mainSettings.glitchMode = 1; break; //глюки
                }
#else
                mainSettings.timeFormat = 1; //формат времени
#endif
                break;
              case SET_GLITCH_MODE: //глюки
#if PLAYER_TYPE
                switch (cur_indi) {
                  case 0: if (mainSettings.volumeSound < MAIN_MAX_VOL) mainSettings.volumeSound++; playerSetVolNow(mainSettings.volumeSound); playerSetTrackNow(PLAYER_TEST_VOL_SOUND, PLAYER_GENERAL_FOLDER); break; //установили громкость
                  case 1: //голос озвучки
                    if (mainSettings.voiceSound < (PLAYER_VOICE_MAX - 1)) {
                      mainSettings.voiceSound++;
                      playerSetVoice(mainSettings.voiceSound);
                      playerSetTrackNow(PLAYER_VOICE_SOUND, PLAYER_GENERAL_FOLDER);
                    }
                    break;
                }
#else
                mainSettings.glitchMode = 1; //глюки
#endif
                break;
              case SET_BTN_SOUND: //звук кнопок
#if PLAYER_TYPE
                switch (cur_indi) {
                  case 0: if ((mainSettings.hourSound & 0x7F) < 3) mainSettings.hourSound++; else mainSettings.hourSound = 0; break; //установили тип озвучки часа
                  case 1: mainSettings.knockSound = 1; break; //включили озвучку действий
                }
#else
                if (!mainSettings.knockSound) buzz_pulse(KNOCK_SOUND_FREQ, KNOCK_SOUND_TIME); //щелчок пищалкой
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
                indiSetBright(mainSettings.indiBright[cur_indi]); //установка общей яркости индикаторов
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
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
              case SET_CORRECT_SENS: //настройка коррекции температуры
                if (mainSettings.tempCorrect < 127) mainSettings.tempCorrect++; else mainSettings.tempCorrect = -127;
                break;
#endif
              case SET_AUTO_SHOW: //автопоказ
                switch (cur_indi) {
                  case 0:
                    if (mainSettings.autoShowTime < 15) mainSettings.autoShowTime++; else mainSettings.autoShowTime = 0;
                    _timer_sec[TMR_SHOW] = getPhaseTime(mainSettings.autoShowTime, AUTO_SHOW_PHASE); //установка таймера показа температуры
                    break;
                  case 1:
                    if (mainSettings.autoShowFlip < (FLIP_EFFECT_NUM + 1)) mainSettings.autoShowFlip++; else mainSettings.autoShowFlip = 0; //устанавливаем анимацию автопоказа температуры
                    if (mainSettings.autoShowFlip > 1) anim_demo = 2; //установили флаг демонстрации анимации
                    break;
                }
                break;
              case SET_BURN_MODE: //анимация антиотравления индикаторов
                switch (cur_indi) {
                  case 0:
                    if (mainSettings.burnTime < 180) mainSettings.burnTime += 5; else mainSettings.burnTime = 10;
                    _timer_sec[TMR_BURN] = getPhaseTime(mainSettings.burnTime, BURN_PHASE); //установка таймера антиотравления
                    break;
                  case 1:
                    if (mainSettings.burnMode < (BURN_EFFECT_NUM - 1)) mainSettings.burnMode++; else mainSettings.burnMode = 0;
                    anim_demo = 2; //установили флаг демонстрации анимации
                    break;
                }
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
#endif
              break;
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
#if ((NEON_DOT != 3) && DOTS_PORT_ENABLE) || ESP_ENABLE
            case SET_CORRECT_SENS: //настройка коррекции температуры
#if ESP_ENABLE
              if (!extendedSettings.tempCorrectSensor) set = 0; //заблокировали пункт меню
#if (NEON_DOT != 3) && DOTS_PORT_ENABLE
              else
#endif
#endif
#if (NEON_DOT != 3) && DOTS_PORT_ENABLE
#if (DOTS_TYPE == 1) || ((DOTS_DIV == 1) && (DOTS_TYPE == 2))
                indiSetDotR(1); //включаем разделительную точку
#else
                indiSetDotL(2); //включаем разделительную точку
#endif
#endif
              break;
#endif
#endif
          }
          if (set) {
#if PLAYER_TYPE
            if (mainSettings.knockSound) playerSetTrackNow((PLAYER_MAIN_MENU_OTHER + TIME_NIGHT) + (cur_mode * 2), PLAYER_MENU_FOLDER);
#endif
            changeBrightDisable(CHANGE_DISABLE); //запретить смену яркости
            dotSetBright((cur_mode != SET_DOT_BRIGHT) ? dot.menuBright : mainSettings.dotBright[TIME_NIGHT]); //включаем точки
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
        cur_indi = TIME_NIGHT;
        _timer_ms[TMR_MS] = time_out = anim_demo = blink_data = 0; //сбрасываем флаги
        break;

      case LEFT_KEY_HOLD: //удержание левой кнопки
        if (set) {
          cur_indi = TIME_NIGHT;
          switch (cur_mode) {
            case SET_INDI_BRIGHT: indiSetBright(mainSettings.indiBright[TIME_NIGHT]); break; //установка ночной яркости индикаторов
            case SET_BACKL_BRIGHT: //яркость подсветки
#if BACKL_TYPE == 3
              setLedBright(mainSettings.backlBright[TIME_NIGHT]); //установка ночной яркости подсветки
#elif BACKL_TYPE
              backlSetBright(mainSettings.backlBright[TIME_NIGHT]); //установка ночной яркости подсветки
#endif
              break;
            case SET_DOT_BRIGHT: dotSetBright(mainSettings.dotBright[TIME_NIGHT]); break; //установка ночной яркости точек
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
            case SET_CORRECT_SENS: mainSettings.tempCorrect = 0; break; //сброс коррекции температуры
#endif
          }
#if PLAYER_TYPE
          if (mainSettings.knockSound) playerSetTrackNow((PLAYER_MAIN_MENU_OTHER + TIME_NIGHT) + (cur_mode * 2), PLAYER_MENU_FOLDER);
#endif
        }
        _timer_ms[TMR_MS] = time_out = anim_demo = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_HOLD: //удержание правой кнопки
        if (set) {
          cur_indi = TIME_DAY;
          switch (cur_mode) {
#if !PLAYER_TYPE && (BACKL_TYPE == 3)
            case SET_TIME_FORMAT: cur_indi = TIME_NIGHT; break; //возврат к формату времени
#endif
            case SET_INDI_BRIGHT: indiSetBright(mainSettings.indiBright[TIME_DAY]); break; //установка дневной яркости индикаторов
            case SET_BACKL_BRIGHT: //яркость подсветки
#if BACKL_TYPE == 3
              setLedBright(mainSettings.backlBright[TIME_DAY]); //установка дневной яркости подсветки
#elif BACKL_TYPE
              backlSetBright(mainSettings.backlBright[TIME_DAY]); //установка дневной яркости подсветки
#endif
              break;
            case SET_DOT_BRIGHT: dotSetBright(mainSettings.dotBright[TIME_DAY]); break; //установка дневной яркости точек
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
            case SET_CORRECT_SENS: mainSettings.tempCorrect = 0; break; //сброс коррекции температуры
#endif
          }
#if PLAYER_TYPE
          if (mainSettings.knockSound) playerSetTrackNow((PLAYER_MAIN_MENU_OTHER + cur_indi) + (cur_mode * 2), PLAYER_MENU_FOLDER);
#endif
        }
        _timer_ms[TMR_MS] = time_out = anim_demo = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        setUpdateMemory(0x01 << MEM_UPDATE_MAIN_SET); //записываем основные настройки в память
        return MAIN_PROGRAM;
    }
  }
  return INIT_PROGRAM;
}
//----------------------------Воспроизвести температуру--------------------------------------
void speakTemp(boolean mode) //воспроизвести температуру
{
#if ESP_ENABLE
  uint8_t _sens = (!mode) ? getMainSens() : getHourSens();
  uint16_t _int = getTemperature(_sens) / 10;
  uint16_t _dec = getTemperature(_sens) % 10;
#else
  uint16_t _int = getTemperature() / 10;
  uint16_t _dec = getTemperature() % 10;
#endif

  if (!mode) playerSetTrackNow(PLAYER_TEMP_SOUND, PLAYER_GENERAL_FOLDER);
  else playerSetTrack(PLAYER_TEMP_SOUND, PLAYER_GENERAL_FOLDER);
#if ESP_ENABLE
  if (getTemperatureSign(_sens)) playerSetTrack(PLAYER_SENS_TEMP_OTHER, PLAYER_END_NUMBERS_FOLDER);
#elif SENS_PORT_ENABLE
  if (getTemperatureSign()) playerSetTrack(PLAYER_SENS_TEMP_OTHER, PLAYER_END_NUMBERS_FOLDER);
#endif
  if (_dec && !mode) {
    playerSpeakNumber(_int, OTHER_NUM);
    playerSetTrack(PLAYER_SENS_CEIL_START + (boolean)playerGetSpeak(_int), PLAYER_END_NUMBERS_FOLDER);
    playerSpeakNumber(_dec, OTHER_NUM);
    playerSetTrack(PLAYER_SENS_DEC_START + (boolean)playerGetSpeak(_dec), PLAYER_END_NUMBERS_FOLDER);
    playerSetTrack(PLAYER_SENS_TEMP_START + 1, PLAYER_END_NUMBERS_FOLDER);
  }
  else {
    playerSpeakNumber(_int);
    playerSetTrack(PLAYER_SENS_TEMP_START + playerGetSpeak(_int), PLAYER_END_NUMBERS_FOLDER);
  }
}
//------------------------------Воспроизвести влажность---------------------------------------
void speakHum(uint8_t hum) //воспроизвести влажность
{
  playerSetTrackNow(PLAYER_HUM_SOUND, PLAYER_GENERAL_FOLDER);
  playerSpeakNumber(hum);
  playerSetTrack(PLAYER_SENS_HUM_START + playerGetSpeak(hum), PLAYER_END_NUMBERS_FOLDER);
}
//-------------------------------Воспроизвести давление---------------------------------------
void speakPress(uint16_t press) //воспроизвести давление
{
  playerSetTrackNow(PLAYER_PRESS_SOUND, PLAYER_GENERAL_FOLDER);
  playerSpeakNumber(press);
  playerSetTrack(PLAYER_SENS_PRESS_START + playerGetSpeak(press), PLAYER_END_NUMBERS_FOLDER);
  playerSetTrack(PLAYER_SENS_PRESS_OTHER, PLAYER_END_NUMBERS_FOLDER);
}
//-----------------------------Установить разделяющую точку-----------------------------------
void setDivDot(boolean set) {
#if DOTS_PORT_ENABLE
#if (DOTS_TYPE == 1) || ((DOTS_DIV == 1) && (DOTS_TYPE == 2))
  if (!set) indiClrDots(); //выключаем разделительные точки
  else indiSetDotR(1); //включаем разделительную точку
#else
  if (!set) indiClrDots(); //выключаем разделительные точки
  else indiSetDotL(2); //включаем разделительную точку
#endif
#elif NEON_DOT == 2
  if (!set) neonDotSet(DOT_NULL); //выключаем разделительной точки
  else {
    neonDotSetBright(dot.menuBright); //установка яркости неоновых точек
    neonDotSet(DOT_LEFT); //включаем неоновую точку
  }
#else
  if (!set) dotSetBright(0); //выключаем точки
  else dotSetBright(dot.menuBright); //включаем точки
#endif
}
//--------------------------------Показать температуру----------------------------------------
uint8_t showTemp(void) //показать температуру
{
  uint8_t mode = 0; //текущий режим

  uint16_t temperature = getTemperature(); //буфер температуры
  uint16_t pressure = getPressure(); //буфер давления
  uint8_t humidity = getHumidity(); //буфер влажности

  if (temperature > 990) return MAIN_PROGRAM; //выходим

#if (BACKL_TYPE == 3) && SHOW_TEMP_BACKL_TYPE
  backlAnimDisable(); //запретили эффекты подсветки
#if SHOW_TEMP_BACKL_TYPE == 1
  changeBrightDisable(CHANGE_STATIC_BACKL); //разрешить смену яркости статичной подсветки
  setLedBright((fastSettings.backlMode & 0x7F) ? backl.maxBright : 0); //установили яркость в зависимости от режима подсветки
#else
  setLedBright(backl.menuBright); //установили максимальную яркость
#endif
#endif

  setDivDot(1); //установить точку температуры

#if (ESP_ENABLE || SENS_PORT_ENABLE) && !INDI_SYMB_TYPE
  boolean dot = 0; //флаг мигания точками
  boolean sign = getTemperatureSign(); //знак температуры
  _timer_ms[TMR_ANIM] = SHOW_TEMP_SIGN_TIME; //устанавливаем таймер
#endif

#if PLAYER_TYPE
  if (mainSettings.knockSound) speakTemp(SPEAK_TEMP_MAIN); //воспроизвести температуру
#endif

  for (_timer_ms[TMR_MS] = SHOW_TEMP_TIME; _timer_ms[TMR_MS];) {
    dataUpdate(); //обработка данных

#if (ESP_ENABLE || SENS_PORT_ENABLE) && !INDI_SYMB_TYPE
    if (!mode && sign) { //если температура отрицательная
      if (!_timer_ms[TMR_ANIM]) { //если пришло время
        _timer_ms[TMR_ANIM] = SHOW_TEMP_SIGN_TIME; //устанавливаем таймер
        setDivDot(dot); //инвертировать точку температуры
        dot = !dot; //инвертировали точки
      }
    }
#endif

    if (!indi.update) {
      indi.update = 1; //сбрасываем флаг
      indiClr(); //очистка индикаторов
#if (LAMP_NUM > 4) && MENU_SHOW_NUMBER
      indiPrintNum(mode + 1, 5); //режим
#endif
      switch (mode) {
        case 0:
          indiPrintNum(temperature, 1, 2, 0);
#if INDI_SYMB_TYPE
          indiSetSymb(getTemperatureSign() ? SYMB_NEGATIVE : SYMB_POSITIVE); //установка индикатора символов
#endif
#if (BACKL_TYPE == 3) && SHOW_TEMP_BACKL_TYPE
          setLedHue(SHOW_TEMP_COLOR_T, WHITE_ON); //установили цвет температуры
#endif
          break;
        case 1:
          indiPrintNum(humidity, 0, 4);
#if INDI_SYMB_TYPE
          indiSetSymb(SYMB_HUMIDITY); //установка индикатора символов
#endif
#if (BACKL_TYPE == 3) && SHOW_TEMP_BACKL_TYPE
          setLedHue(SHOW_TEMP_COLOR_H, WHITE_ON); //установили цвет влажности
#endif
          break;
        case 2:
          indiPrintNum(pressure, 0, 4);
#if INDI_SYMB_TYPE
          indiSetSymb(SYMB_PRESSURE); //установка индикатора символов
#endif
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
            if (!humidity) {
              if (!pressure) mode = 0;
              else mode = 2;
            }
            break;
          case 2: if (!pressure) mode = 0; break;
        }
        if (!mode) { //если режим отображения температуры
          setDivDot(1); //установить точку температуры
#if (ESP_ENABLE || SENS_PORT_ENABLE) && !INDI_SYMB_TYPE
          dot = 0; //установили флаг мигания точками
          _timer_ms[TMR_ANIM] = SHOW_TEMP_SIGN_TIME; //устанавливаем таймер
#endif
        }
        else { //иначе давление или влажность
          setDivDot(0); //очистить точку температуры
        }
#if PLAYER_TYPE
        if (mainSettings.knockSound) {
          switch (mode) {
            case 0: speakTemp(SPEAK_TEMP_MAIN); break; //воспроизвести температуру
            case 1: speakHum(humidity); break; //воспроизвести влажность
            case 2: speakPress(pressure); break; //воспроизвести давление
          }
        }
#endif
        _timer_ms[TMR_MS] = SHOW_TEMP_TIME;
        indi.update = 0; //обновление экрана
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
void speakTime(boolean mode) //воспроизвести время
{
  if (!mode) playerSetTrackNow(PLAYER_TIME_NOW_SOUND, PLAYER_GENERAL_FOLDER);
  else playerSetTrack(PLAYER_TIME_NOW_SOUND, PLAYER_GENERAL_FOLDER);
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
#if DOTS_NUM > 4
#if (DOTS_TYPE == 1) || ((DOTS_DIV == 1) && (DOTS_TYPE == 2))
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
#if (DOTS_TYPE == 1) || ((DOTS_DIV == 1) && (DOTS_TYPE == 2))
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
  if (mainSettings.knockSound) speakTime(0); //воспроизвести время
#endif

  for (_timer_ms[TMR_MS] = SHOW_DATE_TIME; _timer_ms[TMR_MS];) {
    dataUpdate(); //обработка данных

    if (!indi.update) {
      indi.update = 1; //сбрасываем флаг
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
#if (LAMP_NUM > 4) && MENU_SHOW_NUMBER && !SHOW_DATE_WEEK
      indiPrintNum(mode + 1, 5); //режим
#endif
      switch (mode) {
        case 0:
#if SHOW_DATE_TYPE == 1
          indiPrintNum(RTC.MM, 0, 2, 0); //вывод месяца
          indiPrintNum(RTC.DD, 2, 2, 0); //вывод даты
#else
          indiPrintNum(RTC.DD, 0, 2, 0); //вывод даты
          indiPrintNum(RTC.MM, 2, 2, 0); //вывод месяца
#endif
#if (LAMP_NUM > 4) && SHOW_DATE_WEEK
          indiPrintNum(RTC.DW, 5); //день недели
#endif
#if (BACKL_TYPE == 3) && SHOW_DATE_BACKL_TYPE
          setBacklHue(0, 4, SHOW_DATE_BACKL_DM, SHOW_DATE_BACKL_NN);
#if SHOW_DATE_WEEK
          setLedHue(5, SHOW_DATE_BACKL_DW, WHITE_ON);
#endif
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
#if (DOTS_TYPE == 1) || ((DOTS_DIV == 1) && (DOTS_TYPE == 2))
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
        indi.update = 0; //обновление экрана
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
//------------------------------Анимация автоматического показа--------------------------------------
uint8_t autoShowAnimMode(void) //анимация автоматического показа
{
  return (mainSettings.autoShowFlip) ? mainSettings.autoShowFlip : fastSettings.flipMode;
}
//--------------------------------Меню автоматического показа----------------------------------------
void autoShowMenu(void) //меню автоматического показа
{
#if (BACKL_TYPE == 3) && AUTO_SHOW_BACKL_TYPE
  boolean state = 0; //состояние подсветки
#endif
  uint8_t show_mode = 0; //текущий режим

#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
  uint16_t temperature = 0; //буфер температуры
  uint16_t pressure = 0; //буфер давления
  uint8_t humidity = 0; //буфер влажности
#endif

  for (uint8_t mode = 0; mode < sizeof(extendedSettings.autoShowModes); mode++) {
#if DOTS_PORT_ENABLE
    indiClrDots(); //выключаем разделительные точки
#endif
#if (NEON_DOT != 3) || !DOTS_PORT_ENABLE
    dotSetBright(0); //выключаем секундные точки
#endif
#if INDI_SYMB_TYPE
    indiClrSymb(); //очистка индикатора символов
#endif
    animClearBuff(); //очистка буфера анимации
    show_mode = extendedSettings.autoShowModes[mode];
    switch (show_mode) {
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
      case SHOW_TEMP: //режим отображения температуры
#if LAMP_NUM > 4
      case SHOW_TEMP_HUM: //режим отображения температуры и влажности
#endif
#if ESP_ENABLE
      case SHOW_TEMP_ESP:
#if LAMP_NUM > 4
      case SHOW_TEMP_HUM_ESP:
#endif
        temperature = getTemperature(show_mode);
#if LAMP_NUM > 4
        humidity = getHumidity(show_mode);
#endif
#else
        temperature = getTemperature();
#if LAMP_NUM > 4
        humidity = getHumidity();
#endif
#endif
        if (temperature > 990) continue; //возвращаемся назад
        animPrintNum(temperature, 1, 2, 0); //вывод температуры
#if LAMP_NUM > 4
        if (humidity && (show_mode != SHOW_TEMP) && (show_mode != SHOW_TEMP_ESP)) animPrintNum(humidity, 4, 2); //вывод влажности
#endif
        animIndi(autoShowAnimMode(), FLIP_NORMAL); //анимация цифр
        setDivDot(1); //установить точку температуры
#if INDI_SYMB_TYPE
#if ESP_ENABLE
        indiSetSymb(getTemperatureSign(show_mode) ? SYMB_NEGATIVE : SYMB_POSITIVE); //установка индикатора символов
#else
        indiSetSymb(getTemperatureSign() ? SYMB_NEGATIVE : SYMB_POSITIVE); //установка индикатора символов
#endif
#endif
#if (BACKL_TYPE == 3) && AUTO_SHOW_BACKL_TYPE
#if LAMP_NUM > 4
        if (humidity && (show_mode != SHOW_TEMP) && (show_mode != SHOW_TEMP_ESP)) { //если режим отображения температуры и влажности
          setBacklHue(4, 2, SHOW_TEMP_COLOR_H, SHOW_TEMP_COLOR_T); //установили цвет температуры и влажности
          setLedHue(3, SHOW_TEMP_COLOR_P, WHITE_ON); //установили цвет пустого сегмента
        }
        else setLedHue(SHOW_TEMP_COLOR_T, WHITE_ON); //установили цвет температуры
#else
        setLedHue(SHOW_TEMP_COLOR_T, WHITE_ON); //установили цвет температуры
#endif
#endif
        break;

      case SHOW_HUM: //режим отображения влажности
#if ESP_ENABLE
      case SHOW_HUM_ESP:
        humidity = getHumidity(show_mode);
#else
        humidity = getHumidity();
#endif
        if (!humidity) continue; //возвращаемся назад
        animPrintNum(humidity, 0, 4); //вывод влажности

        animIndi(autoShowAnimMode(), FLIP_NORMAL); //анимация цифр
#if INDI_SYMB_TYPE
        indiSetSymb(SYMB_HUMIDITY); //установка индикатора символов
#endif
#if (BACKL_TYPE == 3) && AUTO_SHOW_BACKL_TYPE
        setLedHue(SHOW_TEMP_COLOR_H, WHITE_ON); //установили цвет влажности
#endif
        break;

      case SHOW_PRESS: //режим отображения давления
#if ESP_ENABLE
      case SHOW_PRESS_ESP:
        pressure = getPressure(show_mode);
#else
        pressure = getPressure();
#endif
        if (!pressure) continue; //возвращаемся назад
        animPrintNum(pressure, 0, 4); //вывод давления

        animIndi(autoShowAnimMode(), FLIP_NORMAL); //анимация цифр
#if INDI_SYMB_TYPE
        indiSetSymb(SYMB_PRESSURE); //установка индикатора символов
#endif
#if (BACKL_TYPE == 3) && AUTO_SHOW_BACKL_TYPE
        setLedHue(SHOW_TEMP_COLOR_P, WHITE_ON); //установили цвет давления
#endif
        break;
#endif

      case SHOW_DATE: //режим отображения даты
#if (SHOW_DATE_TYPE == 1) || (SHOW_DATE_TYPE == 3)
        animPrintNum(RTC.MM, 0, 2, 0); //вывод месяца
        animPrintNum(RTC.DD, 2, 2, 0); //вывод даты
#else
        animPrintNum(RTC.DD, 0, 2, 0); //вывод даты
        animPrintNum(RTC.MM, 2, 2, 0); //вывод месяца
#endif
#if SHOW_DATE_WEEK
        animPrintNum(RTC.DW, 5); //день недели
#endif
        animIndi(autoShowAnimMode(), FLIP_NORMAL); //анимация цифр
#if DOTS_PORT_ENABLE
#if (DOTS_TYPE == 1) || ((DOTS_DIV == 1) && (DOTS_TYPE == 2))
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
#if (BACKL_TYPE == 3) && SHOW_DATE_BACKL_TYPE
        setBacklHue(0, 4, SHOW_DATE_BACKL_DM, SHOW_DATE_BACKL_NN);
#if SHOW_DATE_WEEK
        setLedHue(5, SHOW_DATE_BACKL_DW, WHITE_ON);
#endif
#endif
        break;

      case SHOW_YEAR: //режим отображения года
        animPrintNum(RTC.YY, 0); //вывод года
        animIndi(autoShowAnimMode(), FLIP_NORMAL); //анимация цифр
#if (BACKL_TYPE == 3) && SHOW_DATE_BACKL_TYPE
        setBacklHue(0, 4, SHOW_DATE_BACKL_YY, SHOW_DATE_BACKL_NN);
#endif
        break;

#if LAMP_NUM > 4
      case SHOW_DATE_YEAR: //режим отображения даты и года
#if (SHOW_DATE_TYPE == 1) || (SHOW_DATE_TYPE == 3)
        animPrintNum(RTC.MM, 0, 2, 0); //вывод месяца
        animPrintNum(RTC.DD, 2, 2, 0); //вывод даты
#else
        animPrintNum(RTC.DD, 0, 2, 0); //вывод даты
        animPrintNum(RTC.MM, 2, 2, 0); //вывод месяца
#endif
        animPrintNum(RTC.YY - 2000, 4, 2, 0); //вывод года
        animIndi(autoShowAnimMode(), FLIP_NORMAL); //анимация цифр
#if DOTS_PORT_ENABLE && (DOTS_NUM > 4)
#if (DOTS_TYPE == 1) || ((DOTS_DIV == 1) && (DOTS_TYPE == 2))
        indiSetDotR(1); //включаем разделительную точку
        indiSetDotR(3); //включаем разделительную точку
#else
        indiSetDotL(2); //включаем разделительную точку
        indiSetDotL(4); //включаем разделительную точку
#endif
#elif NEON_DOT != 3
        dotSetBright(dot.menuBright); //включаем точки
#endif
#if (BACKL_TYPE == 3) && AUTO_SHOW_BACKL_TYPE
        setBacklHue(0, 4, SHOW_DATE_BACKL_DM, SHOW_DATE_BACKL_YY);
#endif
        break;
#endif
      default: continue; //иначе неизвестный режим
    }

#if (BACKL_TYPE == 3) && AUTO_SHOW_BACKL_TYPE
    if (!state) { //если первый запуск
      state = 1; //установили флаг подсветки
      backlAnimDisable(); //запретили эффекты подсветки
#if AUTO_SHOW_BACKL_TYPE == 1
      changeBrightDisable(CHANGE_STATIC_BACKL); //разрешить смену яркости статичной подсветки
      setLedBright((fastSettings.backlMode & 0x7F) ? backl.maxBright : 0); //установили яркость в зависимости от режима подсветки
#else
      setLedBright(backl.menuBright); //установили максимальную яркость
#endif
    }
#endif

#if AUTO_SHOW_DATE_BLINK
    boolean blink = 0; //флаг мигания индикаторами
#endif
#if (ESP_ENABLE || SENS_PORT_ENABLE) && !INDI_SYMB_TYPE
    boolean dot = 0; //флаг мигания точками
    boolean sign = 0; //знак температуры

    switch (show_mode) {
      case SHOW_TEMP: //режим отображения температуры
#if LAMP_NUM > 4
      case SHOW_TEMP_HUM: //режим отображения температуры и влажности
#endif
#if ESP_ENABLE
      case SHOW_TEMP_ESP:
#if LAMP_NUM > 4
      case SHOW_TEMP_HUM_ESP:
#endif
#endif
#if ESP_ENABLE
        if (getTemperatureSign(show_mode)) sign = 1;
#else
        if (getTemperatureSign()) sign = 1;
#endif
        _timer_ms[TMR_ANIM] = SHOW_TEMP_SIGN_TIME; //устанавливаем таймер
        break;
    }
#endif
    _timer_ms[TMR_MS] = (uint16_t)extendedSettings.autoShowTimes[mode] * 1000; //устанавливаем таймер
    while (_timer_ms[TMR_MS]) { //если таймер истек
      dataUpdate(); //обработка данных

#if ESP_ENABLE
      if (busCheck() & ~(0x01 << BUS_COMMAND_WAIT)) return; //обновление шины
#endif

#if (ESP_ENABLE || SENS_PORT_ENABLE) && !INDI_SYMB_TYPE
      if (sign) { //если температура отрицательная
        if (!_timer_ms[TMR_ANIM]) { //если пришло время
          _timer_ms[TMR_ANIM] = SHOW_TEMP_SIGN_TIME; //устанавливаем таймер
          setDivDot(dot); //инвертировать точку температуры
          dot = !dot; //инвертировали точки
        }
      }
#endif
#if AUTO_SHOW_DATE_BLINK
      if (show_mode <= SHOW_DATE_YEAR) { //если режим отображения даты или года
        if (!_timer_ms[TMR_ANIM]) { //если пришло время
          _timer_ms[TMR_ANIM] = AUTO_SHOW_DATE_TIME; //установили время
          if (blink) indiClr(); //очистили индикаторы
          else animPrintBuff(0, 6, LAMP_NUM); //отрисовали предыдущий буфер
          blink = !blink; //изменили состояние
        }
      }
#endif
      if (buttonState()) return; //возврат если нажата кнопка
    }
#if AUTO_SHOW_DATE_BLINK
    if (blink) animPrintBuff(0, 6, LAMP_NUM); //отрисовали предыдущий буфер
#endif
  }
  animShow = (mainSettings.autoShowFlip) ? (ANIM_OTHER + mainSettings.autoShowFlip) : ANIM_MAIN; //установили флаг анимации
}
//-------------------------Сменить режим анимации минут быстрых настроек----------------------------
void changeFastSetFlip(void) //сменить режим анимации минут быстрых настроек
{
  if (++fastSettings.flipMode > (FLIP_EFFECT_NUM + 1)) fastSettings.flipMode = 0;
}
//-------------------------Сменить режим анимации секунд быстрых настроек----------------------------
void changeFastSetSecs(void) //сменить режим анимации секунд быстрых настроек
{
  if (++fastSettings.secsMode >= SECS_EFFECT_NUM) fastSettings.secsMode = 0;
}
//-------------------------Сменить режим анимации точек быстрых настроек----------------------------
void changeFastSetDot(void) //сменить режим анимации точек быстрых настроек
{
  if (++fastSettings.dotMode >= DOT_EFFECT_NUM) fastSettings.dotMode = 0;
}
//-------------------------Сменить режим анимации подсветки быстрых настроек----------------------------
void changeFastSetBackl(void) //сменить режим анимации подсветки быстрых настроек
{
#if BTN_EASY_MAIN_MODE && (BACKL_TYPE == 3)
  switch (fastSettings.backlMode) {
    case BACKL_STATIC:
    case BACKL_PULS:
    case BACKL_RUNNING_FIRE:
    case BACKL_WAVE:
      if (fastSettings.backlColor < 253) {
        fastSettings.backlColor -= fastSettings.backlColor % 50;
        if (fastSettings.backlColor < 200) fastSettings.backlColor += 50;
        else fastSettings.backlColor = 253;
      }
      else fastSettings.backlColor++;
      if (fastSettings.backlColor) return;
      else setLedHue(fastSettings.backlColor, WHITE_ON); //устанавливаем статичный цвет
      break;
  }
#endif

  if (++fastSettings.backlMode >= BACKL_EFFECT_NUM) fastSettings.backlMode = 0; //переключили режим подсветки
  switch (fastSettings.backlMode) {
#if BACKL_TYPE == 3
    case BACKL_OFF:
      clrLeds(); //выключили светодиоды
      break;
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
#else
    case BACKL_OFF: backlSetBright(0); break; //выключаем подсветку
    case BACKL_STATIC: backlSetBright(backl.maxBright); break; //включаем подсветку
    case BACKL_PULS: backlSetBright(backl.maxBright ? backl.minBright : 0); break; //выключаем подсветку
#endif
  }
}
//-------------------------Сменить цвет режима анимации подсвеетки быстрых настроек----------------------------
void changeFastSetColor(void) //сменить цвет режима анимации подсвеетки быстрых настроек
{
  if (fastSettings.backlColor < 250) fastSettings.backlColor += 10;
  else if (fastSettings.backlColor == 250) fastSettings.backlColor = 253;
  else fastSettings.backlColor++;
  setLedHue(fastSettings.backlColor, WHITE_ON); //устанавливаем статичный цвет
}
//-------------------------------Получить значение быстрых настроек---------------------------------
uint8_t getFastSetData(uint8_t pos) //получить значение быстрых настроек
{
  switch (pos) {
    case FAST_FLIP_MODE: return fastSettings.flipMode; //вывод режима смены минут
#if LAMP_NUM > 4
    case FAST_SECS_MODE: return fastSettings.secsMode; //вывод режима смены секунд
#endif
    case FAST_DOT_MODE: return fastSettings.dotMode; //вывод режима секундных точек
#if BACKL_TYPE
    case FAST_BACKL_MODE: return fastSettings.backlMode; //вывод режима подсветки
#endif
#if BACKL_TYPE == 3
    case FAST_BACKL_COLOR: return (fastSettings.backlColor >= 253) ? (fastSettings.backlColor - 227) : (fastSettings.backlColor / 10); //вывод цвета подсветки
#endif
  }
  return 0;
}
//----------------------------------Переключение быстрых настроек-----------------------------------
uint8_t fastSetSwitch(void) //переключение быстрых настроек
{
  uint8_t show = 1; //флаг запуска анимации
  uint8_t mode = FAST_DOT_MODE; //режим быстрой настройки

  while (1) {
    if (show) {
      switch (show) {
        case 1:
#if PLAYER_TYPE
          if (mainSettings.knockSound) playerSetTrackNow(PLAYER_FAST_MENU_START + mode, PLAYER_MENU_FOLDER);
#endif
          animClearBuff(); //очистка буфера анимации
          animPrintNum(getFastSetData(mode), (LAMP_NUM / 2 - 1), 2, 0); //вывод информации
          animIndi(FLIP_GATES + 2, FLIP_NORMAL); //анимация цифр
          break;
        default:
          switch (mode) {
            case FAST_FLIP_MODE:
              changeFastSetFlip(); //сменить режим анимации минут быстрых настроек
              break;
#if LAMP_NUM > 4
            case FAST_SECS_MODE:
              changeFastSetSecs(); //сменить режим анимации секунд быстрых настроек
              break;
#endif
            case FAST_DOT_MODE:
              changeFastSetDot(); //сменить режим анимации точек быстрых настроек
              break;
#if BACKL_TYPE
            case FAST_BACKL_MODE:
              changeFastSetBackl(); //сменить режим анимации подсветки быстрых настроек
              break;
#endif
#if BACKL_TYPE == 3
            case FAST_BACKL_COLOR:
              changeFastSetColor(); //сменить цвет режима анимации подсвеетки быстрых настроек
              break;
#endif
          }
          indiClr(); //очистка индикаторов
          indiPrintNum(getFastSetData(mode), (LAMP_NUM / 2 - 1), 2, 0); //вывод информации
          break;
      }
      show = 0;
      _timer_ms[TMR_MS] = FAST_SHOW_TIME;
    }

    dataUpdate(); //обработка данных

#if ESP_ENABLE
    if (busCheck()) return MAIN_PROGRAM;
#endif

    if (!_timer_ms[TMR_MS]) break; //выходим

    switch (buttonState()) {
      case SET_KEY_PRESS: //клик средней кнопкой
        if (mode != FAST_DOT_MODE) {
          show = 1; //запустить анимацию
          mode = FAST_DOT_MODE; //демострация текущего режима работы
        }
        else show = 2; //изменить и отобразить данные
        break;

#if BACKL_TYPE
      case LEFT_KEY_PRESS: //клик левой кнопкой
#if BACKL_TYPE == 3
        if ((mode != FAST_BACKL_MODE) && (mode != FAST_BACKL_COLOR)) {
#else
        if (mode != FAST_BACKL_MODE) {
#endif
          show = 1; //запустить анимацию
          mode = FAST_BACKL_MODE; //демострация текущего режима работы
        }
        else show = 2; //изменить и отобразить данные
        break;

#if BACKL_TYPE == 3
      case LEFT_KEY_HOLD: //удержание левой кнопки
        if (mode == FAST_BACKL_MODE) {
          switch (fastSettings.backlMode) {
            case BACKL_STATIC:
            case BACKL_PULS:
            case BACKL_RUNNING_FIRE:
            case BACKL_WAVE:
              show = 1; //запустить анимацию
              mode = FAST_BACKL_COLOR;
              break;
          }
        }
        break;
#endif
#endif

      case RIGHT_KEY_PRESS: //клик правой кнопкой
#if (LAMP_NUM > 4) && !BTN_ADD_TYPE
        if ((mode != FAST_FLIP_MODE) && (mode != FAST_SECS_MODE)) {
#else
        if (mode != FAST_FLIP_MODE) {
#endif
          show = 1; //запустить анимацию
          mode = FAST_FLIP_MODE; //демострация текущего режима работы
        }
        else show = 2; //изменить и отобразить данные
        break;

#if (LAMP_NUM > 4)
#if BTN_ADD_TYPE
      case ADD_KEY_PRESS: //клик доп кнопкой
        if (mode != FAST_SECS_MODE) {
          show = 1; //запустить анимацию
          mode = FAST_SECS_MODE; //демострация текущего режима работы
        }
        else show = 2; //изменить и отобразить данные
        break;
#else
      case RIGHT_KEY_HOLD: //удержание правой кнопки
        if (mode != FAST_SECS_MODE) {
          show = 1; //запустить анимацию
          mode = FAST_SECS_MODE; //демострация текущего режима работы
        }
        break;
#endif
#endif
    }
  }
  if (mode == FAST_FLIP_MODE) animShow = ANIM_DEMO; //демонстрация анимации цифр
  setUpdateMemory(0x01 << MEM_UPDATE_FAST_SET); //записываем настройки в память
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
//-----------------------------Включить радиоприемник-----------------------------------
void radioStartup(void) //включить радиоприемник
{
  radio.powerState = RDA_ON; //установили флаг питания радио
  radioPowerOn(); //включить питание радиоприемника
}
//-----------------------------Выключить радиоприемник----------------------------------
void radioShutdown(void) //выключить радиоприемник
{
  radio.powerState = RDA_OFF; //сбросили флаг питания радио
  radioPowerOff(); //выключить питание радиоприемника
}
//--------------------------Вернуть питание радиоприемника------------------------------
void radioPowerRet(void) //вернуть питание радиоприемника
{
  if (radio.powerState == RDA_ON) { //если радио было включено
    radioPowerOn(); //включить питание радиоприемника
  }
  else radioPowerOff(); // иначе выключить питание радиоприемника
}
//------------------------Переключить питание радиоприемника-----------------------------
void radioPowerSwitch(void) //переключить питание радиоприемника
{
  if (getPowerStatusRDA() != RDA_ERROR) { //если радиоприемник доступен
    if (getPowerStatusRDA() == RDA_OFF) radioStartup(); //включить радиоприемник
    else radioShutdown(); //выключить радиоприемник
  }
}
//--------------------------Поиск радиостанции в памяти---------------------------------
void radioSearchStation(void) //поиск радиостанции в памяти
{
  setUpdateMemory(0x01 << MEM_UPDATE_RADIO_SET); //записываем настройки радио в память
  for (uint8_t i = 0; i < RADIO_MAX_STATIONS; i++) { //ищем среди всех ячеек
    if (radioSettings.stationsSave[i] == radioSettings.stationsFreq) { //если частота совпадает с радиостанцией
      radioSettings.stationNum = i; //установили номер радиостанции
      return; //выходим
    }
  }
  radioSettings.stationNum |= 0x80; //установили номер ячейки за пределом видимости
}
//-----------------------Переключить радиостанцию в памяти------------------------------
void radioSwitchStation(boolean _sta) //переключить радиостанцию в памяти
{
  setUpdateMemory(0x01 << MEM_UPDATE_RADIO_SET); //записываем настройки радио в память
  if (radioSettings.stationNum & 0x80) { //если установлен флаг ячейки
    radioSettings.stationNum &= 0x7F; //сбросили флаг
    radioSettings.stationsFreq = radioSettings.stationsSave[radioSettings.stationNum]; //прочитали частоту
    setFreqRDA(radioSettings.stationsFreq); //установили частоту
    return; //выходим
  }
  for (uint8_t i = 0; i < RADIO_MAX_STATIONS; i++) { //ищем среди всех ячеек
    if (_sta) { //ищем вперед
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
//------------------------Автопоиск радиостанций вверх-----------------------------
void radioSeekUp(void) //автопоиск радиостанций
{
  if (radioSettings.stationsFreq < RADIO_MAX_FREQ) { //если не достигли предела поиска
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
    radio.seekAnim = 0; //сбросили анимацию поиска
#endif
    radio.seekRun = 2; //включили поиск
    radio.seekFreq = RADIO_MAX_FREQ; //установили максимальную частоту
    radioSettings.stationNum |= 0x80; //установили номер ячейки за пределом видимости
    setMuteRDA(RDA_MUTE_ON); //включаем приглушение звука
    startSeekRDA(RDA_SEEK_UP); //начинаем поиск вверх
    dotSetBright(0); //выключаем точки
#if (NEON_DOT != 3) && DOTS_PORT_ENABLE
    indiClrDots(); //очистка разделителных точек
#endif
  }
}
//------------------------Автопоиск радиостанций вниз-----------------------------
void radioSeekDown(void) //автопоиск радиостанций
{
  if (radioSettings.stationsFreq > RADIO_MIN_FREQ) { //если не достигли предела поиска
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
    radio.seekAnim = ((LAMP_NUM + 1) * 2); //сбросили анимацию поиска
#endif
    radio.seekRun = 1; //включили поиск
    radio.seekFreq = RADIO_MIN_FREQ; //установили минимальную частоту
    radioSettings.stationNum |= 0x80; //установили номер ячейки за пределом видимости
    setMuteRDA(RDA_MUTE_ON); //включаем приглушение звука
    startSeekRDA(RDA_SEEK_DOWN); //начинаем поиск вниз
    dotSetBright(0); //выключаем точки
#if (NEON_DOT != 3) && DOTS_PORT_ENABLE
    indiClrDots(); //очистка разделителных точек
#endif
  }
}
//-----------------------------Быстрые настройки радио-----------------------------------
uint8_t radioFastSettings(void) //быстрые настройки радио
{
  if (btn.state) { //если радио включено и нажата кнопка
    uint8_t _state = btn.state; //буфер кнопки

#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
    if (_state == PWR_KEY_PRESS) { //управление питанием
      radioPowerSwitch(); //переключить питание радиоприемника
      buttonState(); //сбросили состояние кнопки
      return 2; //выходим
    }
#endif
    if (radio.powerState) { //если питание радио включено
#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
      if (_state <= ADD_KEY_HOLD) { //если нажата кнопка из стандартного набора
        if (mainTask == RADIO_PROGRAM) { //если текущая подпрограмма радио
#endif
          if (_state == SET_KEY_PRESS) _state = KEY_MAX_ITEMS; //установили режим просмотра громкости
          else if (_state != ADD_KEY_PRESS) return 0; //иначе выходим
#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
        }
        else return 0; //иначе выходим
      }
      else if (mainTask != RADIO_PROGRAM) { //если текущая подпрограмма не радио
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
#endif

#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
      setBacklHue(((LAMP_NUM / 2) - 1), 2, RADIO_BACKL_COLOR_1, RADIO_BACKL_COLOR_2);
#endif

      dotSetBright(0); //выключаем точки
#if (NEON_DOT != 3) && DOTS_PORT_ENABLE
      indiClrDots(); //очистка разделителных точек
#endif

#if INDI_SYMB_TYPE
      indiSetSymb(SYMB_MENU); //установка индикатора символов
#endif

      buttonState(); //сбросили состояние кнопки
      radioSeekStop(); //остановка автопоиска радиостанции

      while (1) {
        dataUpdate(); //обработка данных

        switch (_state) {
          case KEY_NULL:
          case KEY_MAX_ITEMS:
            break;
          case SET_KEY_PRESS: return 2; //выходим
          case RIGHT_KEY_PRESS: //прибавить громкость
#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
          case VOL_UP_KEY_PRESS: //прибавить громкость
#endif
            if (radioSettings.volume < MAIN_MAX_VOL) {
              setUpdateMemory(0x01 << MEM_UPDATE_RADIO_SET);
              setVolumeRDA(++radioSettings.volume); //прибавитиь громкость
            }
            break;
          case LEFT_KEY_PRESS: //убавить громкость
#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
          case VOL_DOWN_KEY_PRESS: //убавить громкость
#endif
            if (radioSettings.volume > MAIN_MIN_VOL) {
              setUpdateMemory(0x01 << MEM_UPDATE_RADIO_SET);
              setVolumeRDA(--radioSettings.volume); //убавить громкость
            }
            break;
          case ADD_KEY_PRESS: //следующая станция
#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
          case STATION_UP_KEY_PRESS: //следующая станция
#endif
            radioSwitchStation(1); //переключить радиостанцию в памяти
            break;
#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
          case STATION_DOWN_KEY_PRESS: //предыдущая станция
            radioSwitchStation(0); //переключить радиостанцию в памяти
            break;
#if IR_EXT_BTN_ENABLE == 2
          case STATION_CELL_0_PRESS: //клик кнопки станции 0
          case STATION_CELL_1_PRESS: //клик кнопки станции 1
          case STATION_CELL_2_PRESS: //клик кнопки станции 2
          case STATION_CELL_3_PRESS: //клик кнопки станции 3
          case STATION_CELL_4_PRESS: //клик кнопки станции 4
          case STATION_CELL_5_PRESS: //клик кнопки станции 5
          case STATION_CELL_6_PRESS: //клик кнопки станции 6
          case STATION_CELL_7_PRESS: //клик кнопки станции 7
          case STATION_CELL_8_PRESS: //клик кнопки станции 8
          case STATION_CELL_9_PRESS: //клик кнопки станции 9
            if (radioSettings.stationsSave[_state - STATION_CELL_0_PRESS]) { //если в памяти записана частота
              radioSettings.stationNum = _state - STATION_CELL_0_PRESS; //установили номер ячейки
              radioSettings.stationsFreq = radioSettings.stationsSave[radioSettings.stationNum]; //прочитали частоту
              setFreqRDA(radioSettings.stationsFreq); //установили частоту
              setUpdateMemory(0x01 << MEM_UPDATE_RADIO_SET);
            }
            break;
#endif
#endif
          default: return 1; //выходим
        }

        switch (_state) {
          case RIGHT_KEY_PRESS:
          case LEFT_KEY_PRESS:
          case KEY_MAX_ITEMS:
#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
          case VOL_UP_KEY_PRESS:
          case VOL_DOWN_KEY_PRESS:
            _timer_ms[TMR_MS] = ((_state == VOL_UP_KEY_PRESS) || (_state == VOL_DOWN_KEY_PRESS)) ? RADIO_FAST_TIME : RADIO_VOL_TIME; //устанавливаем таймер
#else
            _timer_ms[TMR_MS] = RADIO_VOL_TIME; //устанавливаем таймер
#endif
            indiClr(); //очистка индикаторов
            indiPrintNum(radioSettings.volume, ((LAMP_NUM / 2) - 1), 2, 0); //вывод настройки
            break;
          case ADD_KEY_PRESS:
#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
          case STATION_UP_KEY_PRESS:
          case STATION_DOWN_KEY_PRESS:
#endif
            _timer_ms[TMR_MS] = RADIO_FAST_TIME; //устанавливаем таймер
            indiClr(); //очистка индикаторов
            indiPrintNum(radioSettings.stationNum, ((LAMP_NUM / 2) - 1), 2, 0); //вывод настройки
            break;
#if RADIO_ENABLE && IR_PORT_ENABLE && (IR_EXT_BTN_ENABLE == 2)
          case STATION_CELL_0_PRESS: //клик кнопки станции 0
          case STATION_CELL_1_PRESS: //клик кнопки станции 1
          case STATION_CELL_2_PRESS: //клик кнопки станции 2
          case STATION_CELL_3_PRESS: //клик кнопки станции 3
          case STATION_CELL_4_PRESS: //клик кнопки станции 4
          case STATION_CELL_5_PRESS: //клик кнопки станции 5
          case STATION_CELL_6_PRESS: //клик кнопки станции 6
          case STATION_CELL_7_PRESS: //клик кнопки станции 7
          case STATION_CELL_8_PRESS: //клик кнопки станции 8
          case STATION_CELL_9_PRESS: //клик кнопки станции 9
            _timer_ms[TMR_MS] = RADIO_FAST_TIME; //устанавливаем таймер
            indiClr(); //очистка индикаторов
            indiPrintNum(_state - STATION_CELL_0_PRESS, ((LAMP_NUM / 2) - 1), 2, 0); //вывод настройки
            break;
#endif
        }

        _state = buttonState(); //прочитали кнопку

        if (!_timer_ms[TMR_MS]) return 1; //выходим
      }
    }
  }
  return 0;
}
//------------------------------Меню настроек радио-------------------------------------
boolean radioMenuSettings(void) //меню настроек радио
{
  boolean _state = 0; //флаг бездействия
  uint8_t _station = radioSettings.stationNum & 0x7F; //текущий номер радиостанции
  _timer_ms[TMR_MS] = 0; //сбросили таймер

  dotSetBright(0); //выключаем точки
#if (NEON_DOT != 3) && DOTS_PORT_ENABLE
  indiClrDots(); //очистка разделителных точек
#endif

#if INDI_SYMB_TYPE
  indiSetSymb(SYMB_MENU); //установка индикатора символов
#endif

  while (1) {
    dataUpdate(); //обработка данных

    if (!_timer_ms[TMR_MS]) { //если таймер истек
      indiClr(); //очистка индикаторов
      _timer_ms[TMR_MS] = RADIO_STATION_TIME; //устанавливаем таймер
      if (_state) return 1; //выходим по бездействию
      indiPrintNum((boolean)radioSettings.stationsSave[_station], ((LAMP_NUM / 2) - 2)); //вывод настройки
      indiPrintNum(_station, (LAMP_NUM / 2), 2, 0); //вывод настройки
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
      setBacklHue((LAMP_NUM / 2), 2, RADIO_BACKL_COLOR_1, RADIO_BACKL_COLOR_2);
      setLedHue(((LAMP_NUM / 2) - 2), RADIO_BACKL_COLOR_1, WHITE_ON);
#endif
      _state = 1; //установили флаг бездействия
    }

    switch (buttonState()) {
      case RIGHT_KEY_PRESS: //клик правой кнопкой
        if (_station < (RADIO_MAX_STATIONS - 1)) _station++; else _station = 0;
        _state = 0;
        _timer_ms[TMR_MS] = 0; //сбросили таймер
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
        if (_station > 0) _station--; else _station = (RADIO_MAX_STATIONS - 1);
        _state = 0;
        _timer_ms[TMR_MS] = 0; //сбросили таймер
        break;

      case ADD_KEY_PRESS: //клик дополнительной кнопкой
        radioSettings.stationsSave[_station] = radioSettings.stationsFreq; //сохранили радиостанцию
        radioSettings.stationNum = _station; //установили номер радиостанции
        setUpdateMemory(0x01 << MEM_UPDATE_RADIO_SET);
        return 0; //выходим

      case SET_KEY_PRESS: //клик средней кнопкой
        return 1; //выходим

      case ADD_KEY_HOLD: //удержание дополнительной кнопкой
        radioSettings.stationsSave[_station] = 0; //сбросили радиостанцию
        setUpdateMemory(0x01 << MEM_UPDATE_RADIO_SET);
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
    uint8_t time_out = 0; //таймаут автовыхода

#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
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
      radioStartup(); //включить радиоприемник
#endif
    }

    radioSearchStation(); //поиск радиостанции в памяти

    _timer_ms[TMR_MS] = 0; //сбросили таймер

    while (1) {
      dataUpdate(); //обработка данных

#if ESP_ENABLE
      if (busCheck() & ~(0x01 << BUS_COMMAND_WAIT)) { //обновились настройки
        radioSeekStop(); //остановка автопоиска радиостанции
        return RADIO_PROGRAM;
      }
#endif

      if (!indi.update) { //если прошла секунда
        indi.update = 1; //сбросили флаг секунды

#if ALARM_TYPE
        if (alarms.now == ALARM_WARN) { //тревога будильника
          radioSeekStop(); //остановка автопоиска радиостанции
          return ALARM_PROGRAM;
        }
#endif
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
        if ((timer.mode == 2) && !timer.count) { //тревога таймера
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
#if (RADIO_STATUS_DOT_TYPE != 3)
#if ((RADIO_STATUS_DOT_TYPE == 1) && (NEON_DOT < 3)) || !DOTS_PORT_ENABLE
#if NEON_DOT == 2
          neonDotSetBright(dot.menuBright); //установка яркости неоновых точек
          if (getStationStatusRDA()) neonDotSet(DOT_RIGHT); //включаем разделительную точку
          else neonDotSet(DOT_NULL); //выключаем разделительную точку
#elif NEON_DOT < 2
          dotSetBright((getStationStatusRDA()) ? dot.menuBright : 0); //управление точками в зависимости от устойчивости сигнала
#endif
#else
          indiClrDots(); //очистка разделителных точек
#if DOTS_TYPE == 1
          if (getStationStatusRDA()) indiSetDotR(3); //установка разделительной точки
#else
#if DOTS_SHIFT
          if (getStationStatusRDA()) indiSetDotL(4); //установка разделительной точки
#if DOTS_TYPE == 2
          if (getStationStatusRDA()) indiSetDotR(3); //установка разделительной точки
#endif
#else
          if (getStationStatusRDA()) indiSetDotL(radioSettings.stationsFreq < 1000); //установка разделительной точки
#endif
#endif
#endif
#endif
        }
        else { //иначе идет автопоиск
          if (getSeekCompleteStatusRDA()) { //если поиск завершился
            clrSeekCompleteStatusRDA(); //очищаем флаг окончания поиска
            radio.seekFreq = getFreqRDA(); //прочитали частоту
          }
          switch (radio.seekRun) {
            case 1:
              if (radioSettings.stationsFreq > radio.seekFreq) radioSettings.stationsFreq--; else radio.seekRun = 0;
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
              if (radio.seekAnim > 0) radio.seekAnim--; else radio.seekAnim = ((LAMP_NUM + 1) * 2);
#endif
              break;
            case 2:
              if (radioSettings.stationsFreq < radio.seekFreq) radioSettings.stationsFreq++; else radio.seekRun = 0;
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
              if (radio.seekAnim < ((LAMP_NUM + 1) * 2)) radio.seekAnim++; else radio.seekAnim = 0;
#endif
              break;
          }
          if (!radio.seekRun) {
            setMuteRDA(RDA_MUTE_OFF); //выключаем приглушение звука
            radioSettings.stationsFreq = radio.seekFreq; //прочитали частоту
            radioSearchStation(); //поиск радиостанции в памяти
          }
          else _timer_ms[TMR_MS] = RADIO_ANIM_TIME; //устанавливаем таймер
        }

#if DOTS_PORT_ENABLE
#if (DOTS_TYPE == 1) || ((DOTS_DIV == 1) && (DOTS_TYPE == 2))
        indiSetDotR(2); //включаем разделительную точку
#else
        indiSetDotL(3); //включаем разделительную точку
#endif
#endif

#if INDI_SYMB_TYPE && (RADIO_STATUS_DOT_TYPE == 3)
        if (getStationStatusRDA()) indiSetSymb(SYMB_RADIO); //установка индикатора символов
        else indiClrSymb(); //очистка индикатора символов
#endif

        indiClr(); //очистка индикаторов
        indiPrintNum(radioSettings.stationsFreq, 0, 4); //текущаяя частота
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
        if (!radio.seekRun) { //если не идет поиск
          boolean freq_backl = (radioSettings.stationsFreq >= 1000);
          setBacklHue((freq_backl) ? 0 : 1, (freq_backl) ? 3 : 2, RADIO_BACKL_COLOR_1, RADIO_BACKL_COLOR_2);
          setLedHue(3, RADIO_BACKL_COLOR_3, WHITE_ON);
        }
        else setBacklHue((radio.seekAnim >> 1) - 1, 1, RADIO_BACKL_COLOR_1, RADIO_BACKL_COLOR_2); //иначе анимация
#endif
#if LAMP_NUM > 4
        if (radioSettings.stationNum < RADIO_MAX_STATIONS) {
          indiPrintNum(radioSettings.stationNum, 5); //номер станции
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
          setLedHue(5, RADIO_BACKL_COLOR_3, WHITE_ON);
#endif
        }
#endif
      }

#if PLAYER_TYPE
      if (!radio.powerState) { //если питание выключено
        if (!playerPlaybackStatus()) { //если все команды отправлены
          radioStartup(); //включить радиоприемник
        }
      }
#endif

      switch (radioFastSettings()) { //быстрые настройки радио
        case 1: //клик
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;
        case 2: return MAIN_PROGRAM; //выходим
      }

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

        case RIGHT_KEY_HOLD: //удержание правой кнопки
          if (!radio.seekRun) radioSeekUp(); //автопоиск радиостанций
          else radioSeekStop(); //остановка автопоиска радиостанции
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = RADIO_ANIM_TIME; //устанавливаем таймер
          break;

        case LEFT_KEY_HOLD: //удержание левой кнопки
          if (!radio.seekRun) radioSeekDown(); //автопоиск радиостанций
          else radioSeekStop(); //остановка автопоиска радиостанции
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = RADIO_ANIM_TIME; //устанавливаем таймер
          break;

        case ADD_KEY_HOLD: //удержание дополнительной кнопки
          if (!radio.seekRun) { //если не идет поиск
            if (!radioMenuSettings()) { //настройки радио
#if !PLAYER_TYPE
              buzz_pulse(RADIO_SAVE_SOUND_FREQ, RADIO_SAVE_SOUND_TIME); //сигнал успешной записи радиостанции в память
#endif
            }
          }
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;

        case SET_KEY_HOLD: //удержание средней кнопк
          radioSeekStop(); //остановка автопоиска радиостанции
          radioShutdown(); //выключить радиоприемник
          return MAIN_PROGRAM; //выходим
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
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
#if ESP_ENABLE
    if (!timer.mode) { //если таймер отключен
#if PLAYER_TYPE
      playerStop(); //сброс воспроизведения плеера
#else
      melodyStop(); //сброс воспроизведения мелодии
#endif
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
      radioPowerRet(); //вернуть питание радиоприемника
#endif
      return MAIN_PROGRAM; //выходим
    }
#endif
#if PLAYER_TYPE
    if (!playerPlaybackStatus()) playerSetTrack(PLAYER_TIMER_WARN_SOUND, PLAYER_GENERAL_FOLDER);
#endif
    if (!_timer_ms[TMR_ANIM]) {
      _timer_ms[TMR_ANIM] = TIMER_BLINK_TIME;
      switch (blink_data) {
        case 0: indiClr(); break; //очищаем индикаторы
        case 1: indiPrintNum(0, 0, LAMP_NUM, 0); break; //вывод минут/часов/секунд
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
  radioPowerRet(); //вернуть питание радиоприемника
#endif
  return TIMER_PROGRAM;
}
//----------------------------Настройки таймера----------------------------------
void timerSettings(void) //настройки таймера
{
  boolean mode = 0; //текущий режим
  boolean blink_data = 0; //флаг мигания индикаторами
  uint8_t mins = 99; //буфер минут
  uint8_t secs = 59; //буфер секунд

  if (timer.time < 6000) { //если в диапазоне настройки
    mins = timer.time / 60; //установили минуты
    secs = timer.time % 60; //установили секунды
  }

#if PLAYER_TYPE
  if (mainSettings.knockSound) playerSetTrackNow(PLAYER_TIMER_SET_SOUND, PLAYER_GENERAL_FOLDER);
#endif

#if INDI_SYMB_TYPE
  indiSetSymb(SYMB_MENU); //установка индикатора символов
#endif

  dotSetBright(0); //выключаем точки
  while (1) {
    dataUpdate(); //обработка данных

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

      indiClr(); //очистка индикаторов
#if LAMP_NUM > 4
      indiPrintNum(2, 5); //вывод режима
#endif
      indiPrintMenuData(blink_data, mode, mins, 0, secs, 2); //вывод минут/секунд

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
          case 0: if (mins < 99) mins++; else mins = 0; break; //прибавляем минуты
          case 1: if (secs < 59) secs++; else secs = 0; break; //прибавляем секунды
        }
        _timer_ms[TMR_MS] = blink_data = 0; //сбрасываем флаги
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (mode) {
          case 0: if (mins > 0) mins--; else mins = 99; break; //убавляем минуты
          case 1: if (secs > 0) secs--; else secs = 59; break; //убавляем секунды
        }
        _timer_ms[TMR_MS] = blink_data = 0; //сбрасываем флаги
        break;

      case ADD_KEY_HOLD: //удержание дополнительной кнопки
        return; //выходим

      case SET_KEY_HOLD: //удержание средней кнопки
        if (!mins && !secs) timer.time = TIMER_TIME; //устанавливаем значение по умолчанию
        else timer.time = (mins * 60) + secs; //установили настроенное время
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
#if LAMP_NUM > 4
  static uint8_t millis_cnt; //счетчик миллисекунд
#endif

  if (timer.mode) mode = (timer.mode & 0x7F) - 1; //если таймер был запущен
  else { //иначе таймер выключен
    timer.count = 0; //сбрасываем таймер
#if LAMP_NUM > 4
    millis_cnt = 0; //сбрасываем счетчик миллисекунд
#endif
  }

#if PLAYER_TYPE
  if (mainSettings.knockSound && (!timer.mode || (timer.mode > 2))) playerSetTrackNow((mode) ? PLAYER_TIMER_SOUND : PLAYER_STOPWATCH_SOUND, PLAYER_GENERAL_FOLDER);
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

#if ESP_ENABLE
    if (busCheck() & ~(0x01 << BUS_COMMAND_WAIT)) {
      if (!timer.mode || (timer.mode > 2)) return MAIN_PROGRAM; //выходим
      else return TIMER_PROGRAM; //выходим
    }
#endif

    if ((timer.mode == 2) && !timer.count) return WARN_PROGRAM; //тревога таймера

    if (!indi.update) {
      indi.update = 1; //сбрасываем флаг

      if (!timer.mode || (timer.mode > 2)) {
        if (++time_out >= TIMER_TIMEOUT) {
          animShow = ANIM_MAIN; //установили флаг анимации
          return MAIN_PROGRAM;
        }
      }

      if (!timer.count) dotSetBright(0); //выключили точки
      else if (timer.mode > 2 || !timer.mode) dotSetBright(dot.menuBright); //установили точки
      else dotSetBright((dotGetBright()) ? 0 : dot.menuBright); //установили точки

      uint8_t hour = timer.count / 3600; //часы
      uint8_t mins = (timer.count / 60) % 60; //минуты
      uint8_t secs = timer.count % 60; //секунды

      indiClr(); //очистка индикаторов
#if INDI_SYMB_TYPE
      indiClrSymb(); //очистка индикатора символов
#endif
#if LAMP_NUM > 4
      if (timer.mode) {
        if (timer.mode < 3) millis_cnt = 0; //сбрасываем счетчик миллисекунд
        indiPrintNum((timer.count < 3600) ? ((mode) ? (100 - millis_cnt) : millis_cnt) : secs, 4, 2, 0); //вывод милиекунд/секунд
      }
      else indiPrintNum(mode + 1, 5); //вывод режима
#endif

      indiPrintNum((timer.count < 3600) ? mins : hour, 0, 2, 0); //вывод минут/часов
      indiPrintNum((timer.count < 3600) ? secs : mins, 2, 2, 0); //вывод секунд/минут

#if (BACKL_TYPE == 3) && TIMER_BACKL_TYPE
      switch (timer.mode) {
        case 0: setLedHue(TIMER_STOP_COLOR, WHITE_ON); break; //установили цвет остановки
        case 1: setLedHue(TIMER_RUN_COLOR_1, WHITE_ON); break; //установили цвет секундомера
        case 2: setLedHue(TIMER_RUN_COLOR_2, WHITE_ON); break; //установили цвет таймера
        default: setLedHue(TIMER_PAUSE_COLOR, WHITE_ON); break; //установили цвет паузы
      }
#endif
    }

#if LAMP_NUM > 4
    switch (timer.mode) {
      case 1: //секундомер
      case 2: //таймер
        if (!_timer_ms[TMR_MS]) {
          _timer_ms[TMR_MS] = 10;
          if (timer.count < 3600) {
            millis_cnt += 1;
            indiPrintNum((mode) ? (100 - millis_cnt) : millis_cnt, 4, 2, 0); //вывод милиекунд
          }
        }
        break;
    }
#endif

    switch (buttonState()) {
      case SET_KEY_PRESS: //клик средней кнопкой
        if (mode && !timer.mode) { //если режим таймера и таймер/секундомер не запущен
          timerSettings(); //настройки таймера
          time_out = 0; //сбрасываем таймер автовыхода
          indi.update = 0; //обновление экрана
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
        indi.update = 0; //обновление экрана
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
        indi.update = 0; //обновление экрана
        break;

      case ADD_KEY_PRESS: //клик дополнительной кнопкой
        if (!timer.mode) {
#if LAMP_NUM > 4
          millis_cnt = 0; //сбрасываем счетчик миллисекунд
#endif
          timer.mode = mode + 1;
        }
        else timer.mode ^= 0x80; //приостановка таймера/секундомера
        time_out = 0; //сбрасываем таймер автовыхода
        indi.update = 0; //обновление экрана
        break;

      case ADD_KEY_HOLD: //удержание дополнительной кнопки
        timer.mode = 0; //деактивируем таймер
        switch (mode & 0x01) {
          case 0: timer.count = 0; break; //сбрасываем секундомер
          case 1: timer.count = timer.time; break; //сбрасываем таймер
        }
        time_out = 0; //сбрасываем таймер автовыхода
        indi.update = 0; //обновление экрана
        break;
    }
  }
  return INIT_PROGRAM;
}
//------------------------------------Звук смены часа------------------------------------
void hourSound(void) //звук смены часа
{
  if (checkHourStrart(mainSettings.timeHour[0], mainSettings.timeHour[1])) {
    if ((mainTask == MAIN_PROGRAM) || (mainTask == SLEEP_PROGRAM)) { //если в режиме часов или спим
#if PLAYER_TYPE
      uint8_t temp = mainSettings.hourSound;
      if (!(temp & 0x03)) {
        if (mainSettings.knockSound) temp |= 0x02;
        else temp = 0x01;
      }
      playerStop(); //сброс воспроизведения плеера
      if (temp & 0x01) playerSetTrackNow(PLAYER_HOUR_SOUND, PLAYER_GENERAL_FOLDER); //звук смены часа
      if (temp & 0x02) speakTime(temp & 0x01); //воспроизвести время
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
      if (temp & 0x80) { //воспроизвести температуру
#if ESP_ENABLE
        if (getTemperature(getHourSens()) <= 990) speakTemp(SPEAK_TEMP_HOUR); //воспроизвести целую температуру
#else
        if (getTemperature() <= 990) speakTemp(SPEAK_TEMP_HOUR); //воспроизвести целую температуру
#endif
      }
#endif
#else
      if (mainSettings.knockSound) melodyPlay(SOUND_HOUR, SOUND_LINK(general_sound), REPLAY_ONCE); //звук смены часа
#endif
    }
  }
}
//---------------------Установка яркости от времени суток-----------------------------
void changeBright(void) //установка яркости от времени суток
{
  indi.sleepMode = SLEEP_DISABLE; //сбросили флаг режима сна индикаторов

#if LIGHT_SENS_ENABLE || ESP_ENABLE
  if (mainSettings.timeBright[TIME_NIGHT] != mainSettings.timeBright[TIME_DAY])
#endif
    light_state = (checkHourStrart(mainSettings.timeBright[TIME_NIGHT], mainSettings.timeBright[TIME_DAY])) ? 2 : 0;
#if !LIGHT_SENS_ENABLE && ESP_ENABLE
  else light_state = device.light;
#endif

  switch (light_state) {
    case 0: //дневной режим
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
      break;
#if LIGHT_SENS_ENABLE || ESP_ENABLE
    case 1: //промежуточный режим
#if (NEON_DOT != 3) || !DOTS_PORT_ENABLE
      dot.maxBright = dot.menuBright = mainSettings.dotBright[TIME_NIGHT] + ((mainSettings.dotBright[TIME_DAY] - mainSettings.dotBright[TIME_NIGHT]) >> 1); //установка максимальной яркости точек
#else
      dot.menuBright = dot.maxBright = 1; //установка максимальной яркости точек
#endif
#if BACKL_TYPE
      backl.menuBright = backl.maxBright = mainSettings.backlBright[TIME_NIGHT] + ((mainSettings.backlBright[TIME_DAY] - mainSettings.backlBright[TIME_NIGHT]) >> 1); //установка максимальной яркости подсветки
#endif
      indi.maxBright = mainSettings.indiBright[TIME_NIGHT] + ((mainSettings.indiBright[TIME_DAY] - mainSettings.indiBright[TIME_NIGHT]) >> 1); //установка максимальной яркости индикаторов
      if (mainSettings.timeSleep[TIME_DAY]) indi.sleepMode = SLEEP_DAY; //установили флаг режима сна индикаторов
      break;
#endif
    default: //ночной режим
      dot.maxBright = mainSettings.dotBright[TIME_NIGHT]; //установка максимальной яркости точек
      dot.menuBright = (dot.maxBright) ? dot.maxBright : 10; //установка максимальной яркости точек в меню
#if BACKL_TYPE
      backl.maxBright = mainSettings.backlBright[TIME_NIGHT]; //установка максимальной яркости подсветки
      backl.menuBright = (backl.maxBright) ? backl.maxBright : 10; //установка максимальной яркости подсветки в меню
#endif
      indi.maxBright = mainSettings.indiBright[TIME_NIGHT]; //установка максимальной яркости индикаторов
      if (mainSettings.timeSleep[TIME_NIGHT]) indi.sleepMode = SLEEP_NIGHT; //установили флаг режима сна индикаторов
      break;
  }

  if (changeBrightState) { //если разрешено менять яркость
    if (mainTask == MAIN_PROGRAM) { //если основной режим
      switch (dotGetMode()) { //мигание точек
        case DOT_OFF: dotSetBright(0); break; //точки выключены
        case DOT_STATIC: dotSetBright(dot.maxBright); break; //точки включены
#if (NEON_DOT != 3) || !DOTS_PORT_ENABLE
        case DOT_MAIN_PULS: //плавное мигание
#if NEON_DOT == 2
        case DOT_MAIN_TURN_PULS:
#endif
          if (!dot.maxBright) dotSetBright(0); //если яркость не установлена
#if DOT_PULS_TIME || ((NEON_DOT == 2) && DOT_PULS_TURN_TIME)
          else { //иначе пересчитываем шаги
#if DOT_PULS_TIME
            dot.brightStep = setBrightStep(dot.maxBright * 2, DOT_PULS_STEP_TIME, DOT_PULS_TIME); //расчёт шага яркости точки
            dot.brightTime = setBrightTime(dot.maxBright * 2, DOT_PULS_STEP_TIME, DOT_PULS_TIME); //расчёт шага яркости точки
#endif
#if (NEON_DOT == 2) && DOT_PULS_TURN_TIME
            dot.brightTurnStep = setBrightStep(dot.maxBright * 2, DOT_PULS_TURN_STEP_TIME, DOT_PULS_TURN_TIME); //расчёт шага яркости точки
            dot.brightTurnTime = setBrightTime(dot.maxBright * 2, DOT_PULS_TURN_STEP_TIME, DOT_PULS_TURN_TIME); //расчёт шага яркости точки
#endif
          }
#endif
          break;
#endif
        default:
#if NEON_DOT != 3
          if (!dot.maxBright) dotSetBright(0); //если яркость не установлена
          else if (dotGetBright()) dotSetBright(dot.maxBright); //установка яркости точек
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
        backl.mode_8_time = setBrightTime((uint16_t)backlNowBright * LEDS_NUM, BACKL_MODE_8_STEP_TIME, BACKL_MODE_8_TIME); //расчёт шага яркости
        backl.mode_8_step = setBrightStep((uint16_t)backlNowBright * LEDS_NUM, BACKL_MODE_8_STEP_TIME, BACKL_MODE_8_TIME); //расчёт шага яркости
#endif
      }
    }
#endif
#if BURN_BRIGHT
    if (changeBrightState != CHANGE_INDI_BLOCK) indiSetBright(indi.maxBright); //установка общей яркости индикаторов
#else
    indiSetBright(indi.maxBright); //установка общей яркости индикаторов
#endif
#if INDI_SYMB_TYPE
    indiSetSymbBright(indi.maxBright); //установка яркости индикатора символов
#endif
  }
}
#if BACKL_TYPE == 3
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
            _timer_ms[TMR_BACKL] = BACKL_MODE_4_TIME / LEDS_NUM / BACKL_MODE_4_FADING; //установили таймер
            if (backl.steps) { //если есть шаги затухания
              decLedsBright(backl.position - 1, backl.mode_4_step); //уменьшаем яркость
              backl.steps--; //уменьшаем шаги затухания
            }
            else { //иначе двигаем голову
              if (backl.drive) { //если направление вправо
                if (backl.position > 0) backl.position--; else backl.drive = 0; //едем влево
              }
              else { //иначе напрвление влево
                if (backl.position < (LEDS_NUM + 1)) backl.position++; else backl.drive = 1; //едем вправо
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
            switch (backl.steps) { //в зависимости от текущего шага анимации
              case 0:
              case 2:
                if (incLedBright(backl.position, backl.mode_8_step, backl.maxBright)) { //прибавили шаг яркости
                  backl.drive = 1; //установили флаг завершения анимации
                }
                break;
              default:
                if (decLedBright(backl.position, backl.mode_8_step, backl.minBright)) { //убавили шаг яркости
                  backl.drive = 1; //установили флаг завершения анимации
                }
                break;
            }
            if (backl.drive) { //если анимация завершена
              backl.drive = 0; //сбросили флаг завершения анимации
              switch (backl.steps) {
                case 0:
                case 1:
                  if (backl.position < (LEDS_NUM - 1)) backl.position++; //сменили позицию
                  else { //иначе меняем режим анимации
                    if (backl.steps < 1) { //если был режим разгорания
                      backl.steps = 1; //перешли в затухание
                      backl.position = 0; //сбросили позицию
                    }
                    else { //иначе режим затухания
                      backl.steps = 2; //перешли в разгорание
                      backl.position = (LEDS_NUM - 1); //сбросили позицию
                    }
                  }
                  break;
                default:
                  if (backl.position > 0) backl.position--; //сменили позицию
                  else { //иначе меняем режим анимации
                    if (backl.steps < 3) { //если был режим разгорания
                      backl.steps = 3; //перешли в затухание
                      backl.position = (LEDS_NUM - 1); //сбросили позицию
                    }
                    else { //иначе режим затухания
                      backl.steps = 0; //перешли в разгорание
                      backl.position = 0; //сбросили позицию
                    }
                  }
                  break;
              }
            }
            if (fastSettings.backlMode == BACKL_WAVE) { //если режим статичного цвета
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
            for (uint8_t f = 0; f < LEDS_NUM; f++) setLedHue(f, backl.color + (f * BACKL_MODE_13_STEP), WHITE_OFF); //установили цвет
          }
          break;
        case BACKL_RUNNING_FIRE_CONFETTI:
        case BACKL_WAVE_CONFETTI:
        case BACKL_CONFETTI: { //рандомный цвет
            _timer_ms[TMR_COLOR] = BACKL_MODE_14_TIME; //установили таймер
            setLedHue(random(0, LEDS_NUM), random(0, 256), WHITE_ON); //установили цвет
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
#elif BACKL_TYPE
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
#endif
//-------------------------------Анимации точек------------------------------------
void dotEffect(void) //анимации точек
{
  if (mainTask == MAIN_PROGRAM) { //если основная программа
    if (!dot.update && !_timer_ms[TMR_DOT]) { //если пришло время
      if (!dot.maxBright || (animShow >= ANIM_MAIN)) { //если яркость выключена или запущена сторонняя анимация
        dot.update = 1; //сбросили флаг секунд
        return; //выходим
      }

      switch (dotGetMode()) { //режим точек
        case DOT_MAIN_BLINK:
          if (!dot.drive) dotSetBright(dot.maxBright); //включаем точки
          else dotSetBright(0); //выключаем точки
          dot.drive = !dot.drive; //сменили направление
          dot.update = 1; //сбросили флаг обновления точек
          break;
        case DOT_MAIN_DOOBLE_BLINK:
          if (dot.count & 0x01) dotSetBright(0); //выключаем точки
          else dotSetBright(dot.maxBright); //включаем точки

          if (++dot.count > 3) {
            dot.count = 0;  //сбрасываем счетчик
            dot.update = 1; //сбросили флаг обновления точек
          }
          else _timer_ms[TMR_DOT] = DOT_MAIN_DOOBLE_TIME; //установили таймер
          break;
#if NEON_DOT != 3
        case DOT_MAIN_PULS:
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
          break;
#endif
#if NEON_DOT == 2
        case DOT_MAIN_TURN_BLINK:
          neonDotSetBright(dot.maxBright); //установка яркости неоновых точек
          if (!dot.drive) neonDotSet(DOT_LEFT); //установка неоновой точки
          else neonDotSet(DOT_RIGHT); //установка неоновой точки
          dot.drive = !dot.drive; //сменили направление
          dot.update = 1; //сбросили флаг обновления точек
          break;
        case DOT_MAIN_TURN_PULS:
          switch (dot.count) {
            case 0: if (dotIncBright(dot.brightTurnStep, dot.maxBright, DOT_LEFT)) dot.count = 1; break; //сменили направление
            case 1: if (dotDecBright(dot.brightTurnStep, 0, DOT_LEFT)) dot.count = 2; break; //сменили направление
            case 2: if (dotIncBright(dot.brightTurnStep, dot.maxBright, DOT_RIGHT)) dot.count = 3; break; //сменили направление
            default:
              if (dotDecBright(dot.brightTurnStep, 0, DOT_RIGHT)) {
                dot.count = 0; //сменили направление
                dot.update = 1; //сбросили флаг обновления точек
                return; //выходим
              }
              break;
          }
          _timer_ms[TMR_DOT] = dot.brightTurnTime; //установили таймер
          break;
#endif
#if DOTS_PORT_ENABLE
        case DOT_BLINK:
          if (!dot.drive) {
            indiSetDotsMain(DOT_ALL); //установка разделительных точек
            dot.drive = 1; //сменили направление
#if DOT_BLINK_TIME
            _timer_ms[TMR_DOT] = DOT_BLINK_TIME; //установили таймер
#else
            dot.update = 1; //сбросили флаг секунд
#endif
          }
          else {
            indiClrDots(); //очистка разделительных точек
            dot.drive = 0; //сменили направление
            dot.update = 1; //сбросили флаг секунд
          }
          break;
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
        case DOT_TURN_BLINK: //мигание одной точкой по очереди
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
          indiClrDots(); //очистка разделительных точек
          switch (dot.drive) {
            case 0: indiSetDotsMain(DOT_LEFT); dot.drive = 1; break; //включаем левую точку
            case 1: indiSetDotsMain(DOT_RIGHT); dot.drive = 0; break; //включаем правую точку
          }
          break;
#endif
#if (DOTS_NUM > 4) && (DOTS_TYPE == 2)
        case DOT_DUAL_TURN_BLINK: //мигание двумя точками по очереди
#if DOT_DUAL_TURN_TIME
          if (dot.count < ((1000 / DOT_DUAL_TURN_TIME) - 1)) {
            dot.count++; //прибавили шаг
            _timer_ms[TMR_DOT] = DOT_DUAL_TURN_TIME; //установили таймер
          }
          else {
            dot.count = 0; //сбросили счетчик
            dot.update = 1; //сбросили флаг секунд
          }
#else
          dot.update = 1; //сбросили флаг секунд
#endif
          switch (dot.drive) {
            case 0: indiSetDotL(2); indiSetDotR(3); indiClrDotR(1); indiClrDotL(4); dot.drive = 1; break; //включаем левую точку
            case 1: indiSetDotR(1); indiSetDotL(4); indiClrDotL(2); indiClrDotR(3); dot.drive = 0; break; //включаем правую точку
          }
          break;
#endif
#endif
        default: dot.update = 1; break; //сбросили флаг обновления точек
      }
    }
  }
}
#if (NEON_DOT != 3) && DOTS_PORT_ENABLE
//--------------------------------Мигание точек------------------------------------
void dotFlash(void) //мигание точек
{
  static boolean state; //текущее состояние точек

  if (mainTask == MAIN_PROGRAM) { //если основная программа
    if (!dot.maxBright || (animShow >= ANIM_MAIN)) { //если яркость выключена или запущена сторонняя анимация
      return; //выходим
    }

    if (dotGetMode() >= DOT_BLINK) {
      switch (fastSettings.neonDotMode) { //режим точек
        case DOT_EXT_OFF: dotSetBright(0); break; //точки выключены
        case DOT_EXT_STATIC: dotSetBright(dot.maxBright); break; //точки включены
        case DOT_EXT_BLINK:
          if (!state) dotSetBright(dot.maxBright); //включаем точки
          else dotSetBright(0); //выключаем точки
          state = !state; //сменили направление
          break;
        case DOT_EXT_TURN_BLINK:
          neonDotSetBright(dot.maxBright); //установка яркости неоновых точек
          if (!state) neonDotSet(DOT_LEFT); //установка неоновой точки
          else neonDotSet(DOT_RIGHT); //установка неоновой точки
          state = !state; //сменили направление
          break;
      }
    }
  }
}
#endif
//---------------------------Получить анимацию точек-------------------------------
uint8_t dotGetMode(void) //получить анимацию точек
{
#if ALARM_TYPE
  switch (alarms.now) {
    case ALARM_ENABLE: if (extendedSettings.alarmDotOn != DOT_EFFECT_NUM) return extendedSettings.alarmDotOn; break;
    case ALARM_WAIT: if (extendedSettings.alarmDotWait != DOT_EFFECT_NUM) return extendedSettings.alarmDotWait; break;
  }
#endif
  return fastSettings.dotMode;
}
//-----------------------------Сброс анимации точек--------------------------------
void dotReset(uint8_t state) //сброс анимации точек
{
  static uint8_t mode; //предыдущий режим точек

  if ((state != ANIM_RESET_CHANGE) || (mode != dotGetMode())) { //если нужно сбросить анимацию точек
#if DOTS_PORT_ENABLE
    indiClrDots(); //выключаем разделительные точки
#endif
#if (NEON_DOT != 3) || !DOTS_PORT_ENABLE
    dotSetBright(0); //выключаем секундные точки
#endif
    _timer_ms[TMR_DOT] = 0; //сбросили таймер
    if (dotGetMode() > 1) { //если анимация точек включена
      dot.update = 1; //установли флаг обновления точек
      dot.drive = 0; //сбросили флаг направления яркости точек
      dot.count = 0; //сбросили счетчик вспышек точек
#if DOTS_PORT_ENABLE
      dot.steps = 0; //сбросили шаги точек
#endif
    }
  }
  mode = dotGetMode(); //запоминаем анимацию точек
}
//----------------------------Сброс анимации секунд--------------------------------
void secsReset(void) //сброс анимации секунд
{
#if LAMP_NUM > 4
  anim.flipSeconds = 0; //сбрасываем флаг анимации секунд
#endif
  indi.update = 0; //устанавливаем флаг обновления индикаторов
}
//------------------------------Сброс режима сна------------------------------------
void sleepReset(void) //сброс режима сна
{
  _timer_sec[TMR_SLEEP] = mainSettings.timeSleep[indi.sleepMode - SLEEP_NIGHT]; //установли время ожидания режима сна
  if (indi.sleepMode == SLEEP_DAY) _timer_sec[TMR_SLEEP] *= 60; //если режим сна день
}
//----------------------------Режим сна индикаторов---------------------------------
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
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
#if RADIO_SLEEP_ENABLE == 1
  if (indi.sleepMode != SLEEP_DAY) radioShutdown(); //выключить радиоприемник
#elif !RADIO_SLEEP_ENABLE
  radioShutdown(); //выключить радиоприемник
#endif
#endif
  while (!buttonState()) { //если не нажата кнопка
    dataUpdate(); //обработка данных

#if ESP_ENABLE
    if (busCheck() & ~(0x01 << BUS_COMMAND_WAIT)) return MAIN_PROGRAM; //выходим
#endif

    if (!indi.update) { //если пришло время обновить индикаторы
      indi.update = 1; //сбрасываем флаг

#if ALARM_TYPE
      if (alarms.now == ALARM_WARN) return ALARM_PROGRAM; //тревога будильника
#endif
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
      if ((timer.mode == 2) && !timer.count) return WARN_PROGRAM; //тревога таймера
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
    if (!_timer_sec[TMR_GLITCH] && (RTC.s >= GLITCH_PHASE_MIN) && (RTC.s < GLITCH_PHASE_MAX)) { //если пришло время
      uint8_t indiSave = 0; //текущая цифра в индикаторе
      uint8_t glitchCounter = random(GLITCH_NUM_MIN, GLITCH_NUM_MAX); //максимальное количество глюков
      uint8_t glitchIndic = random(0, LAMP_NUM); //номер индикатора
      _timer_ms[TMR_ANIM] = 0; //сбрасываем таймер
      while (!buttonState()) { //если не нажата кнопка
        systemTask(); //обработка данных
#if LAMP_NUM > 4
        flipSecs(); //анимация секунд
#endif
        if (!_timer_ms[TMR_ANIM]) { //если таймер истек
          if (indiGet(glitchIndic) != INDI_NULL) { //если индикатр включен
            indiSave = indiGet(glitchIndic); //сохраняем текущую цифру в индикаторе
            indiClr(glitchIndic); //выключаем индикатор
          }
          else indiSet(indiSave, glitchIndic); //включаем индикатор
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
#if BURN_DOTS
    if (!demo) indiSetDots(0, DOTS_ALL); //установка разделительных точек
#else
    indiClrDots(); //выключаем разделительные точки
#endif
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
  switch (fastSettings.secsMode) {
    case SECS_BRIGHT: //плавное угасание и появление
      if (animShow == ANIM_SECS) { //если сменились секунды
        animShow = ANIM_NULL; //сбрасываем флаг анимации цифр
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
        animShow = ANIM_NULL; //сбрасываем флаг анимации цифр
        _timer_ms[TMR_MS] = 0; //сбрасываем таймер
        anim.flipSeconds = (RTC.s) ? (RTC.s - 1) : 59; //предыдущая секунда
        anim.flipBuffer[0] = anim.flipSeconds % 10; //старые секунды
        anim.flipBuffer[1] = anim.flipSeconds / 10; //старые секунды
        anim.flipBuffer[2] = RTC.s % 10; //новые секунды
        anim.flipBuffer[3] = RTC.s / 10; //новые секунды
        anim.flipSeconds = 0x03; //устанавливаем флаги анимации

        if (fastSettings.secsMode == SECS_ORDER_OF_CATHODES) {
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
        _timer_ms[TMR_MS] = (fastSettings.secsMode == SECS_ORDER_OF_NUMBERS) ? SECONDS_ANIM_2_TIME : SECONDS_ANIM_3_TIME; //установили таймер
        for (uint8_t i = 0; i < 2; i++) { //перебираем все цифры
          if (anim.flipBuffer[i] != anim.flipBuffer[i + 2]) { //если не достигли конца анимации разряда
            if (--anim.flipBuffer[i] > 9) anim.flipBuffer[i] = 9; //меняем цифру разряда
          }
          else anim.flipSeconds &= ~(0x01 << i); //иначе завершаем анимацию для разряда
          indiPrintNum((fastSettings.secsMode == SECS_ORDER_OF_NUMBERS) ? anim.flipBuffer[i] : cathodeMask[anim.flipBuffer[i]], (LAMP_NUM - 1) - i); //вывод секунд
        }
      }
      break;
    default:
      if (animShow == ANIM_SECS) { //если сменились секунды
        animShow = ANIM_NULL; //сбрасываем флаг анимации цифр
        indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
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
    case 0: if (type == FLIP_NORMAL) animPrintBuff(0, 6, LAMP_NUM); animShow = ANIM_NULL; return; //без анимации
    case 1: if (type == FLIP_DEMO) return; else mode = pgm_read_byte(&_anim_set[random(0, sizeof(_anim_set))]); break; //случайный режим
  }

  mode -= 2; //установили режим
  flipIndi(mode, type); //перешли в отрисовку анимации
  if (mode == FLIP_BRIGHT) indiSetBright(indi.maxBright); //возвращаем максимальную яркость

  animPrintBuff(0, 6, LAMP_NUM); //отрисовали буфер
  animShow = ANIM_NULL; //сбрасываем флаг анимации
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

#if ESP_ENABLE
    if (busCheck() & ~(0x01 << BUS_COMMAND_WAIT)) return; //обновление шины
#endif

    if (type != FLIP_NORMAL) { //если анимация времени
      if (!indi.update) { //если пришло время обновить индикаторы
        indi.update = 1; //сбрасываем флаг
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
              if (changeCnt < 2) {
                switch (changeCnt) {
                  case 0:
                    changeIndi = random(0, LAMP_NUM);
                    for (uint8_t b = 0; b < changeNum; b++) {
                      while (changeBuffer[b] == changeIndi) {
                        changeIndi = random(0, LAMP_NUM);
                        b = 0;
                      }
                    }
                    changeBuffer[changeNum] = changeIndi;
                    indiClr(changeIndi); //очистка индикатора
                    break;
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
        default: return; //неизвестная анимация
      }
    }
  }
}
//-----------------------------Главный экран------------------------------------------------
uint8_t mainScreen(void) //главный экран
{
  if (animShow < ANIM_MAIN) animShow = ANIM_NULL; //сбрасываем флаг анимации цифр
  else if (animShow == ANIM_DEMO) animIndi(fastSettings.flipMode, FLIP_DEMO); //демонстрация анимации цифр
  else if (animShow >= ANIM_OTHER) animIndi(animShow - ANIM_OTHER, FLIP_TIME); //анимация цифр

  if (indi.sleepMode) { //если активен режим сна
    if (!changeAnimState) sleepReset(); //установли время ожидания режима сна
    else if (_timer_sec[TMR_SLEEP] < RESET_TIME_SLEEP) _timer_sec[TMR_SLEEP] = RESET_TIME_SLEEP; //установли минимальное время ожидания режима сна
  }

  if (_timer_sec[TMR_GLITCH] < RESET_TIME_GLITCH) _timer_sec[TMR_GLITCH] = RESET_TIME_GLITCH; //если время вышло то устанавливаем минимальное время
  if (_timer_sec[TMR_BURN] < RESET_TIME_BURN) _timer_sec[TMR_BURN] = RESET_TIME_BURN; //если время вышло то устанавливаем минимальное время
  if (_timer_sec[TMR_SHOW] < RESET_TIME_SHOW) _timer_sec[TMR_SHOW] = RESET_TIME_SHOW; //если время вышло то устанавливаем минимальное время

  changeAnimState = ANIM_RESET_ALL; //сбрасываем флаг изменения ремов анимации

  for (;;) { //основной цикл
    dataUpdate(); //обработка данных

#if ESP_ENABLE
    if (busCheck() & ~(0x01 << BUS_COMMAND_WAIT)) { //обновление шины
      if (!changeAnimState) changeAnimState = ANIM_RESET_CHANGE; //установили тип сброса анимации
      return MAIN_PROGRAM; //перезапуск основной программы
    }
#endif

#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
    if (radioFastSettings() == 1) return MAIN_PROGRAM; //перезапуск основной программы
#endif

    if (!indi.update) { //если пришло время обновить индикаторы
#if ALARM_TYPE
#if INDI_SYMB_TYPE
      if (alarms.now != ALARM_DISABLE) indiSetSymb(SYMB_ALARM); //установка индикатора символов
      else indiClrSymb(); //очистка индикатора символов
#endif
      if (alarms.now == ALARM_WARN) return ALARM_PROGRAM; //тревога будильника
#endif
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
      if ((timer.mode == 2) && !timer.count) return WARN_PROGRAM; //тревога таймера
#endif

      if (indi.sleepMode != SLEEP_NIGHT) { //если режим сна не ночной
        if (!_timer_sec[TMR_BURN]) { //если пришло время отобразить анимацию антиотравления
#if BURN_BRIGHT
          changeBrightDisable(CHANGE_INDI_BLOCK); //запретить смену яркости индикаторов
          indiSetBright(BURN_BRIGHT); //установка общей яркости индикаторов
#endif
          if (mainSettings.burnMode != BURN_SINGLE_TIME) mainTask = SLEEP_PROGRAM; //подмена текущей программы
          burnIndi(mainSettings.burnMode, BURN_NORMAL); //антиотравление индикаторов
          _timer_sec[TMR_BURN] = getPhaseTime(mainSettings.burnTime, BURN_PHASE); //установка таймера антиотравления
          if (mainSettings.burnMode != BURN_SINGLE_TIME) changeAnimState = ANIM_RESET_DOT; //установили тип сброса анимации
          else changeAnimState = ANIM_RESET_CHANGE; //установили тип сброса анимации
          return MAIN_PROGRAM; //перезапуск основной программы
        }

        if (mainSettings.autoShowTime && !_timer_sec[TMR_SHOW]) { //если пришло время отобразить температуру
          mainTask = SLEEP_PROGRAM; //подмена текущей программы
          autoShowMenu(); //автоматическое отображение данных
          _timer_sec[TMR_SHOW] = getPhaseTime(mainSettings.autoShowTime, AUTO_SHOW_PHASE); //установка таймера показа температуры
          changeAnimState = ANIM_RESET_DOT; //установили тип сброса анимации
          return MAIN_PROGRAM; //перезапуск основной программы
        }

        if (animShow >= ANIM_MINS) animIndi(fastSettings.flipMode, FLIP_TIME); //анимация минут
      }
      else animShow = ANIM_NULL; //сбрасываем флаг анимации
      if (indi.sleepMode && !_timer_sec[TMR_SLEEP]) return SLEEP_PROGRAM; //режим сна индикаторов

      indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
      indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
#if LAMP_NUM > 4
      if (animShow != ANIM_SECS) indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
#endif
      indi.update = 1; //сбрасываем флаг

      glitchIndi(); //имитация глюков
    }

#if LAMP_NUM > 4
    flipSecs(); //анимация секунд
#endif

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
#if BTN_EASY_MAIN_MODE //упрощенный режим
#if BACKL_TYPE
      case LEFT_KEY_PRESS: //клик левой кнопкой
        if (indi.sleepMode) sleepReset(); //сброс режима сна
        changeFastSetBackl(); //сменить режим анимации подсветки быстрых настроек
        setUpdateMemory(0x01 << MEM_UPDATE_FAST_SET); //записываем настройки в память
        break;
#endif

#if ALARM_TYPE
      case LEFT_KEY_HOLD: //удержание левой кнопки
        return ALARM_SET_PROGRAM; //настройка будильника
#endif

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        if (indi.sleepMode) sleepReset(); //сброс режима сна
        changeFastSetDot(); //сменить режим анимации точек быстрых настроек
        setUpdateMemory(0x01 << MEM_UPDATE_FAST_SET); //записываем настройки в память
        dotReset(ANIM_RESET_CHANGE); //установили тип сброса анимации
        break;

      case RIGHT_KEY_HOLD: //удержание правой кнопки
        return CLOCK_SET_PROGRAM; //иначе настройки времени

      case SET_KEY_PRESS: //клик средней кнопкой
        changeFastSetFlip(); //сменить режим анимации минут быстрых настроек
        animShow = ANIM_DEMO; //демонстрация анимации цифр
        setUpdateMemory(0x01 << MEM_UPDATE_FAST_SET); //записываем настройки в память
        return MAIN_PROGRAM; //перезапуск основной программы

      case SET_KEY_HOLD: //удержание средней кнопки
#if ALARM_TYPE
        if (alarms.now == ALARM_WAIT) { //если будильник в режиме ожидания
          alarmDisable(); //отключение будильника
          break;
        }
#endif
#if LAMP_NUM > 4
        changeFastSetSecs(); //сменить режим анимации секунд быстрых настроек
        setUpdateMemory(0x01 << MEM_UPDATE_FAST_SET); //записываем настройки в память
        changeAnimState = ANIM_RESET_CHANGE; //установили тип сброса анимации
        return MAIN_PROGRAM; //перезапуск основной программы
#else
        break;
#endif
#else //обычный режим
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
      case LEFT_KEY_PRESS: //клик левой кнопкой
        return TEMP_PROGRAM; //показать температуру
#endif

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
        if (alarms.now == ALARM_WAIT) { //если будильник в режиме ожидания
          alarmDisable(); //отключение будильника
          break;
        }
#endif
        return MAIN_SET_PROGRAM; //настроки основные
#endif

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
        if (alarms.now == ALARM_WAIT) { //если будильник в режиме ожидания
          alarmDisable(); //отключение будильника
          break;
        }
#endif
#if RADIO_ENABLE && TIMER_ENABLE
        return RADIO_PROGRAM; //радиоприемник
#else
        break;
#endif
#endif
    }
  }
  return INIT_PROGRAM;
}
