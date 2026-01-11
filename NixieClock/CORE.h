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
#if (SECS_DOT != 3) && (SECS_DOT != 4)
  DOT_MAIN_PULS, //плавно мигает
#endif
#if SECS_DOT == 2
  DOT_MAIN_TURN_BLINK, //мигание неоновых ламп раз в секунду по очереди
  DOT_MAIN_TURN_PULS, //мигание неоновых ламп плавно по очереди
#endif
#if SECS_DOT == 4
  DOT_DECATRON_METR,
  DOT_DECATRON_STEPS,
  DOT_DECATRON_TIMER,
  DOT_DECATRON_SWAY,
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

//переменные работы с индикаторами
struct indiData {
  boolean update; //флаг обновления индикаторов
  uint8_t sleepMode; //флаг режима сна индикаторов
  uint8_t maxBright; //максимальная яркость индикаторов
} indi;

//флаги анимаций
uint8_t changeBrightState; //флаг состояния смены яркости подсветки
uint8_t changeAnimState; //флаг состояния анимаций
uint8_t animShow; //флаг анимации смены времени


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

#define US_PERIOD (uint16_t)(((uint16_t)FREQ_TICK + 1) * TIME_TICK) //период тика таймера в мкс
#define US_PERIOD_MIN (uint16_t)(US_PERIOD - (US_PERIOD % 100) - 400) //минимальный период тика таймера
#define US_PERIOD_MAX (uint16_t)(US_PERIOD - (US_PERIOD % 100) + 400) //максимальный период тика таймера

#define R_COEF(low, high) (((float)(low) + (float)(high)) / (float)(low)) //коэффициент делителя напряжения
#define HV_ADC(vcc) (uint16_t)((1023.0 / (float)(vcc)) * ((float)GEN_HV_VCC / (float)R_COEF(GEN_HV_R_LOW, GEN_HV_R_HIGH))) //значение ацп удержания напряжения
#define GET_VCC(ref, adc) (float)(((ref) * 1023.0) / (float)(adc)) //расчет напряжения питания

#define RESET_BOOTLOADER __asm__ __volatile__ ("JMP 0x7E00") //загрузчик
#define RESET_SYSTEM __asm__ __volatile__ ("JMP 0x0000") //перезагрузка
#define RESET_WDT __asm__ __volatile__ ("WDR") //сброс WDT

uint8_t pwm_coef; //коэффициент линейного регулирования
uint16_t hv_treshold = HV_ADC(5); //буфер сравнения напряжения

uint8_t light_adc; //значение АЦП сенсора яркости освещения
uint8_t light_state = 2; //состояние сенсора яркости освещения
boolean light_update = 0; //флаг обновления яркости


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
  INDI_ERROR           //0013 - сбой работы динамической индикации
};

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

struct extSensorData {
  int16_t temp = 0x7FFF; //температура
  uint16_t press; //давление
  uint8_t hum; //влажность
} extSens;

enum {
  SENS_DS3231, //датчик DS3231
  SENS_AHT, //датчики AHT
  SENS_SHT, //датчики SHT
  SENS_BME, //датчики BME/BMP
  SENS_DS18, //датчики DS18B20
  SENS_DHT, //датчики DHT
  SENS_ALL //датчиков всего
};

//переменные обработки кнопок
struct buttonData {
  uint8_t state; //текущее состояние кнопок
  uint8_t adc; //результат опроса аналоговых кнопок
} btn;
uint8_t analogState; //флаги обновления аналоговых портов
uint16_t vcc_adc; //напряжение питания

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
  ADD_KEY_HOLD, //удержание дополнительной кнопки
#if IR_EXT_BTN_ENABLE
#if IR_PORT_ENABLE && RADIO_ENABLE
  PWR_KEY_PRESS, //клик кнопки питания
  VOL_UP_KEY_PRESS, //клик кнопки громкости вверх
  VOL_DOWN_KEY_PRESS, //клик кнопки громкости вниз
  STATION_UP_KEY_PRESS, //клик кнопки станции вверх
  STATION_DOWN_KEY_PRESS, //клик кнопки станции вниз
#if IR_EXT_BTN_ENABLE == 2
  STATION_CELL_0_PRESS, //клик кнопки станции 0
  STATION_CELL_1_PRESS, //клик кнопки станции 1
  STATION_CELL_2_PRESS, //клик кнопки станции 2
  STATION_CELL_3_PRESS, //клик кнопки станции 3
  STATION_CELL_4_PRESS, //клик кнопки станции 4
  STATION_CELL_5_PRESS, //клик кнопки станции 5
  STATION_CELL_6_PRESS, //клик кнопки станции 6
  STATION_CELL_7_PRESS, //клик кнопки станции 7
  STATION_CELL_8_PRESS, //клик кнопки станции 8
  STATION_CELL_9_PRESS, //клик кнопки станции 9
#endif
#endif
#endif
  KEY_MAX_ITEMS //максимум кнопок
};

//-----------------Настройки----------------
struct Settings_1 {
  uint8_t indiBright[2] = {DEFAULT_INDI_BRIGHT_N, DEFAULT_INDI_BRIGHT}; //яркость индикаторов
  uint8_t backlBright[2] = {DEFAULT_BACKL_BRIGHT_N, DEFAULT_BACKL_BRIGHT}; //яркость подсветки
  uint8_t dotBright[2] = {DEFAULT_DOT_BRIGHT_N, DEFAULT_DOT_BRIGHT}; //яркость точек
  uint8_t timeBright[2] = {DEFAULT_NIGHT_START, DEFAULT_NIGHT_END}; //время перехода яркости
  uint8_t timeHour[2] = {DEFAULT_HOUR_SOUND_START, DEFAULT_HOUR_SOUND_END}; //время звукового оповещения нового часа
  uint8_t timeSleep[2] = {DEFAULT_SLEEP_WAKE_TIME_N, DEFAULT_SLEEP_WAKE_TIME}; //время режима сна
  boolean timeFormat = DEFAULT_TIME_FORMAT; //формат времени
  uint8_t baseSound = DEFAULT_BASE_SOUND; //основные звуки
  uint8_t hourSound = (DEFAULT_HOUR_SOUND_TYPE & 0x03) | ((DEFAULT_HOUR_SOUND_TEMP) ? 0x80 : 0x00); //тип озвучки смены часа
  uint8_t volumeSound = DEFAULT_PLAYER_VOLUME; //громкость озвучки
  uint8_t voiceSound = DEFAULT_VOICE_SOUND; //голос озвучки
  int8_t tempCorrect = DEFAULT_TEMP_CORRECT; //коррекция температуры
  boolean glitchMode = DEFAULT_GLITCH_MODE; //режим глюков
  uint8_t autoShowTime = DEFAULT_AUTO_SHOW_TIME; //интервал времени автопоказа
  uint8_t autoShowFlip = DEFAULT_AUTO_SHOW_ANIM; //режим анимации автопоказа
  uint8_t burnMode = DEFAULT_BURN_MODE; //режим антиотравления индикаторов
  uint8_t burnTime = DEFAULT_BURN_PERIOD; //интервал антиотравления индикаторов
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

struct Settings_4 { //расширенные настройки
  uint8_t autoShowModes[5] = {AUTO_SHOW_MODES};
  uint8_t autoShowTimes[5] = {AUTO_SHOW_TIMES};
  uint8_t alarmTime = ALARM_TIMEOUT;
  uint8_t alarmWaitTime = ALARM_WAIT_TIME;
  uint8_t alarmSoundTime = ALARM_SOUND_TIME;
  uint8_t alarmDotOn = ((!ALARM_ON_BLINK_DOT) ? (DOT_EFFECT_NUM) : (ALARM_ON_BLINK_DOT - 1));
  uint8_t alarmDotWait = ((!ALARM_WAIT_BLINK_DOT) ? (DOT_EFFECT_NUM) : (ALARM_WAIT_BLINK_DOT - 1));
  uint8_t tempCorrectSensor = SHOW_TEMP_CORRECT_MODE;
  uint8_t tempMainSensor = SHOW_TEMP_MAIN_SENS;
  uint8_t tempHourSensor = HOUR_SOUND_MAIN_SENS;
} extendedSettings;

struct Settings_5 {
  uint16_t irButtons[KEY_MAX_ITEMS - 1]; //коды кнопок пульта
  uint16_t timePeriod = US_PERIOD; //коррекция хода внутреннего осцилятора
  uint8_t min_pwm = DEFAULT_MIN_PWM; //минимальный шим
  uint8_t max_pwm = DEFAULT_MAX_PWM; //максимальный шим
  uint8_t light_zone[2][3]; //зоны яркости датчика освещения
  int8_t hvCorrect; //коррекция напряжения
  int8_t aging; //коррекция регистра старения
} debugSettings;

enum {
  MEM_UPDATE_MAIN_SET,
  MEM_UPDATE_FAST_SET,
  MEM_UPDATE_RADIO_SET,
  MEM_UPDATE_EXTENDED_SET,
  MEM_MAX_DATA
};
uint8_t memoryUpdate;

#define CELL(x) (0x01 << x) //выбор ячейки памяти

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

inline boolean irGetReadyStatus(void); //получить статус готовности IR приемника
inline uint16_t irGetCommand(void); //получить текущую команду IR приемника

void buzzPulse(uint16_t freq, uint16_t time); //генерация частоты бузера
void melodyStop(void); //остановка воспроизведения мелодии
void playerStop(void); //остановка воспроизведения трека

void systemTask(void); //процедура системной задачи

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
//---------------------------Первичный запуск WDT-------------------------------
inline void startEnableWDT(void) //первичный запуск WDT
{
  cli(); //запрещаем прерывания
  RESET_WDT; //сбрасываем таймер WDT
  MCUSR &= ~(0x01 << WDRF); //сбросли флаг перезагрузки WDT
  WDTCSR = (0x01 << WDCE) | (0x01 << WDE); //разрешаем изменения WDT
  WDTCSR = (0x01 << WDE) | (0x01 << WDP3) | (0x01 << WDP0); //устанавливаем таймер WDT на 8сек
}
//---------------------------Основной запуск WDT--------------------------------
inline void mainEnableWDT(void) //основной запуск WDT
{
  RESET_WDT; //сбрасываем таймер WDT
  WDTCSR = (0x01 << WDCE) | (0x01 << WDE); //разрешаем изменения WDT
  WDTCSR = (0x01 << WDE) | (0x01 << WDP2) | (0x01 << WDP1) | (0x01 << WDP0); //устанавливаем таймер WDT на 2сек
}
//------------------------------Проверка переполнения тиков---------------------------------------
inline void tickCheck(void) //проверка переполнения тиков
{
  if (!++tick_ms) { //если превышено количество тиков
    SET_ERROR(TICK_OVF_ERROR); //устанавливаем ошибку переполнения тиков времени
    SET_ERROR(RESET_ERROR); //устанавливаем ошибку аварийной перезагрузки
    RESET_SYSTEM; //перезагрузка
  }
}
//------------------------------Проверка переполнения стека---------------------------------------
inline void stackCheck(void) //проверка переполнения стека
{
  if (!(SPH & 0xFC)) { //если стек переполнен
    SET_ERROR(STACK_OVF_ERROR); //устанавливаем ошибку переполнения стека
    SET_ERROR(RESET_ERROR); //устанавливаем ошибку аварийной перезагрузки
    RESET_SYSTEM; //перезагрузка
  }
}
//----------------------------Проверка состояния преобразователя----------------------------------
void converterCheck(void) //проверка состояния преобразователя
{
#if GEN_ENABLE
#if GEN_FEEDBACK
  if (((TCCR1A & 0x4F) != (0x01 << WGM10)) ||
#else
#if CONV_PIN == 9
  if (((TCCR1A & 0xCF) != ((0x01 << WGM10) | (0x01 << COM1A1))) ||
#elif CONV_PIN == 10
  if (((TCCR1A & 0x3F) != ((0x01 << WGM10) | (0x01 << COM1B1))) ||
#endif
#endif
#if GEN_SPEED_X2
      (TCCR1B != ((0x01 << CS10) | (0x01 << WGM12))))
#else
      (TCCR1B != (0x01 << CS10)))
#endif
  {
    TCCR1A = TCCR1B = 0; //выключаем шим
    CONV_DISABLE; //выключаем преобразователь
    SET_ERROR(CONVERTER_ERROR); //устанавливаем ошибку сбоя работы преобразователя
    SET_ERROR(RESET_ERROR); //устанавливаем ошибку аварийной перезагрузки
    RESET_SYSTEM; //перезагрузка
  }
#if CONV_PIN == 9
  if (OCR1A > 200) { //если вышли за предел
    OCR1A = 200; //установили максимум
    SET_ERROR(PWM_OVF_ERROR); //устанавливаем ошибку переполнения заполнения шим преобразователя
  }
#elif CONV_PIN == 10
  if (OCR1B > 200) { //если вышли за предел
    OCR1B = 200; //установили максимум
    SET_ERROR(PWM_OVF_ERROR); //устанавливаем ошибку переполнения заполнения шим преобразователя
  }
#endif
#endif
}
//---------------------------------Инициализация периферии ядра----------------------------------------
void coreInit(void) //инициализация периферии ядра
{
  cli(); //запрещаем прерывания глобально

#if INDI_PORT_TYPE || (INDI_MODE == 1)
  DDRB |= 0x04; //установили D10 как выход

  SPSR = (0x01 << SPI2X); //включили удвоение скорости
  SPCR = (0x01 << SPE) | (0x01 << MSTR); //запустили SPI в режиме ведущего
#endif

  EIMSK = 0; //запретили прерывания INT0/INT1

#if !IR_PORT_ENABLE
  PCICR = 0; //запретили прерывания PCINT0/PCINT1/PCINT2
#endif

#if PLAYER_TYPE == 2
#if BUZZ_PIN == 9
  OCR1A = 128; //выключаем dac
#elif BUZZ_PIN == 10
  OCR1B = 128; //выключаем dac
#endif
#endif
#if !SECS_DOT
#if SECL_PIN == 9
  OCR1A = 0; //выключаем точки
#elif SECL_PIN == 10
  OCR1B = 0; //выключаем точки
#endif
#endif

  TIMSK1 = 0; //отключаем прерывания
  TCCR1A = (0x01 << WGM10); //режим коррекции фазы шим
#if GEN_SPEED_X2
  TCCR1B = (0x01 << CS10) | (0x01 << WGM12);  //задаем частоту 62 кГц
#else
  TCCR1B = (0x01 << CS10);  //задаем частоту 31 кГц
#endif

#if PLAYER_TYPE == 2
#if BUZZ_PIN == 9
  TCCR1A |= (0x01 << COM1A1); //подключаем D9
#elif BUZZ_PIN == 10
  TCCR1A |= (0x01 << COM1B1); //подключаем D10
#endif
#endif
#if !SECS_DOT
#if SECL_PIN == 9
  TCCR1A |= (0x01 << COM1A1); //подключаем D9
#elif SECL_PIN == 10
  TCCR1A |= (0x01 << COM1B1); //подключаем D10
#endif
#endif

#if GEN_ENABLE
#if CONV_PIN == 9
  OCR1A = CONSTRAIN(debugSettings.min_pwm, 100, 200); //устанавливаем первичное значение шим
  TCCR1A |= (0x01 << COM1A1); //подключаем D9
#elif CONV_PIN == 10
  OCR1B = CONSTRAIN(debugSettings.min_pwm, 100, 200); //устанавливаем первичное значение шим
  TCCR1A |= (0x01 << COM1B1); //подключаем D10
#endif
#endif

  OCR2A = 0; //выключаем подсветку
  OCR2B = 0; //сбрасывем бузер

  TIMSK2 = 0; //отключаем прерывания
#if (BACKL_TYPE != 1)
  TCCR2A = 0; //отключаем OCR2A и OCR2B
#else
  TCCR2A = (0x01 << COM2A1 | 0x01 << WGM20 | 0x01 << WGM21); //подключаем D11
#endif
  TCCR2B = (0x01 << CS21); //пределитель 8

  sei(); //разрешаем прерывания глобально
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
#if LIGHT_SENS_ENABLE
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
#if LIGHT_SENS_ENABLE
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
//----------------Обработка обратной связи преобразователя-------------------------
void feedbackUpdate(void) //обработка обратной связи преобразователя
{
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

#if (GEN_ENABLE && (GEN_FEEDBACK == 1)) || BTN_TYPE || LIGHT_SENS_ENABLE
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
        if (mainSettings.baseSound) buzzPulse(KNOCK_SOUND_FREQ, KNOCK_SOUND_TIME); //щелчок пищалкой
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
  if (irGetReadyStatus()) { //если пришла команда и управление ИК не заблокировано
    for (uint8_t button = 0; button < (KEY_MAX_ITEMS - 1); button++) { //ищем номер кнопки
      if (irGetCommand() == debugSettings.irButtons[button]) { //если команда совпала
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
