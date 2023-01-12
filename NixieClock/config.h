//Типы плат часов
#if (BOARD_TYPE == 0) //IN-12 (индикаторы стоят правильно)
volatile uint8_t* anodePort[] = {&DOT_PORT, &ANODE_1_PORT, &ANODE_2_PORT, &ANODE_3_PORT, &ANODE_4_PORT, ANODE_OFF, ANODE_OFF}; //таблица портов анодов ламп
const uint8_t anodeBit[] = {0x01 << DOT_BIT, 0x01 << ANODE_1_BIT, 0x01 << ANODE_2_BIT, 0x01 << ANODE_3_BIT, 0x01 << ANODE_4_BIT, ANODE_OFF, ANODE_OFF}; //таблица бит анодов ламп
const uint8_t digitMask[] = {7, 3, 6, 4, 1, 9, 8, 0, 5, 2, 10};   //маска дешифратора платы in12 (цифры нормальные)(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 6, 2, 7, 5, 0, 4, 9, 8, 3};     //порядок катодов in12
#elif (BOARD_TYPE == 1) //IN-12 turned (индикаторы перевёрнуты)
volatile uint8_t* anodePort[] = {&DOT_PORT, &ANODE_4_PORT, &ANODE_3_PORT, &ANODE_2_PORT, &ANODE_1_PORT, ANODE_OFF, ANODE_OFF}; //таблица портов анодов ламп
const uint8_t anodeBit[] = {0x01 << DOT_BIT, 0x01 << ANODE_4_BIT, 0x01 << ANODE_3_BIT, 0x01 << ANODE_2_BIT, 0x01 << ANODE_1_BIT, ANODE_OFF, ANODE_OFF}; //таблица бит анодов ламп
const uint8_t digitMask[] = {2, 8, 1, 9, 6, 4, 3, 5, 0, 7, 10};   //маска дешифратора платы in12 turned (цифры вверх ногами)(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 6, 2, 7, 5, 0, 4, 9, 8, 3};     //порядок катодов in12
#elif (BOARD_TYPE == 2) //IN-14 (обычная и neon dot)
volatile uint8_t* anodePort[] = {&DOT_PORT, &ANODE_4_PORT, &ANODE_3_PORT, &ANODE_2_PORT, &ANODE_1_PORT, ANODE_OFF, ANODE_OFF}; //таблица портов анодов ламп
const uint8_t anodeBit[] = {0x01 << DOT_BIT, 0x01 << ANODE_4_BIT, 0x01 << ANODE_3_BIT, 0x01 << ANODE_2_BIT, 0x01 << ANODE_1_BIT, ANODE_OFF, ANODE_OFF}; //таблица бит анодов ламп
const uint8_t digitMask[] = {9, 8, 0, 5, 4, 7, 3, 6, 2, 1, 10};   //маска дешифратора платы in14(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 0, 2, 9, 3, 8, 4, 7, 5, 6};     //порядок катодов in14
#elif (BOARD_TYPE == 3) //другие индикаторы(4 лампы)
volatile uint8_t* anodePort[] = {&DOT_PORT, &ANODE_1_PORT, &ANODE_2_PORT, &ANODE_3_PORT, &ANODE_4_PORT, ANODE_OFF, ANODE_OFF}; //таблица портов анодов ламп
const uint8_t anodeBit[] = {0x01 << DOT_BIT, 0x01 << ANODE_1_BIT, 0x01 << ANODE_2_BIT, 0x01 << ANODE_3_BIT, 0x01 << ANODE_4_BIT, ANODE_OFF, ANODE_OFF}; //таблица бит анодов ламп
const uint8_t digitMask[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};   //тут вводим свой порядок пинов лампы(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 6, 2, 7, 5, 0, 4, 9, 8, 3};     //порядок катодов in12
#elif (BOARD_TYPE == 4) //другие индикаторы(6 ламп)
volatile uint8_t* anodePort[] = {&DOT_PORT, &ANODE_1_PORT, &ANODE_2_PORT, &ANODE_3_PORT, &ANODE_4_PORT, &ANODE_5_PORT, &ANODE_6_PORT}; //таблица портов анодов ламп
const uint8_t anodeBit[] = {0x01 << DOT_BIT, 0x01 << ANODE_1_BIT, 0x01 << ANODE_2_BIT, 0x01 << ANODE_3_BIT, 0x01 << ANODE_4_BIT, 0x01 << ANODE_5_BIT, 0x01 << ANODE_6_BIT}; //таблица бит анодов ламп
const uint8_t digitMask[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};   //тут вводим свой порядок пинов лампы(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 0, 2, 9, 3, 8, 4, 7, 5, 6};     //порядок катодов in14
#endif

//Работа со звуками
#define SOUND_PATTERN(ptr) {(uint16_t)&ptr, sizeof(ptr)} //маска для массива мелодий
#define SOUND_LINK(link) (uint16_t)&link //ссылка на массив мелодии
#define SOUND_MAX(arr) (sizeof(arr) >> 2) //максимальное количество мелодий в массиве

//Мелодии будильника
const uint16_t _alarm_1[][3] PROGMEM = { //массив семплов 1-й мелодии будильника || {семпл - частота(10..10000)(Hz), | длительность звука(4..10000)(ms), | длительность семпла(4..10000)(ms)}
  {2000, 500, 1000}
};
const uint16_t _alarm_2[][3] PROGMEM = { //массив семплов 2-й мелодии будильника || {семпл - частота(10..10000)(Hz), | длительность звука(4..10000)(ms), | длительность семпла(4..10000)(ms)}
  {2000, 100, 170}, {2000, 100, 170}, {2000, 100, 500}
};
const uint16_t _alarm_3[][3] PROGMEM = { //массив семплов 3-й мелодии будильника || {семпл - частота(10..10000)(Hz), | длительность звука(4..10000)(ms), | длительность семпла(4..10000)(ms)}
  {2000, 250, 250}, {1500, 250, 350}, {2000, 250, 250}, {1500, 250, 350},
  {2000, 250, 250}, {1500, 250, 350}, {2000, 250, 250}, {1500, 250, 750}
};
const uint16_t _alarm_4[][3] PROGMEM = { //массив семплов 4-й мелодии будильника || {семпл - частота(10..10000)(Hz), | длительность звука(4..10000)(ms), | длительность семпла(4..10000)(ms)}
  {392, 700, 700}, {392, 700, 700}, {392, 700, 700}, {311, 500, 500},
  {466, 200, 200}, {392, 700, 700}, {311, 500, 500}, {466, 200, 200},
  {392, 1400, 1400}, {587, 700, 700}, {587, 700, 700}, {587, 700, 700},
  {622, 500, 500}, {466, 200, 200}, {369, 700, 700}, {311, 500, 500},
  {466, 200, 200}, {392, 1400, 1400}, {784, 700, 700}, {392, 500, 500},
  {392, 200, 200}, {784, 700, 700}, {739, 500, 500}, {698, 200, 200},
  {659, 200, 200}, {622, 200, 200}, {659, 900, 900}, {415, 300, 300},
  {554, 700, 700}, {523, 500, 500}, {493, 200, 200}, {466, 200, 200},
  {440, 200, 200}, {466, 900, 900}, {311, 300, 300}, {369, 700, 700},
  {311, 500, 500}, {466, 200, 200}, {392, 1500, 2500}
};

//Массив мелодий будильника
const uint16_t alarm_sound[][2] PROGMEM = {
  SOUND_PATTERN(_alarm_1),
  SOUND_PATTERN(_alarm_2),
  SOUND_PATTERN(_alarm_3),
  SOUND_PATTERN(_alarm_4)
};


//Звуки ошибок
const uint16_t _error_low[][3] PROGMEM = { //массив семплов || {семпл - частота(10..10000)(Hz), | длительность звука(4..10000)(ms), | длительность семпла(4..10000)(ms)}
  {2000, 200, 400}
};
const uint16_t _error_high[][3] PROGMEM = { //массив семплов || {семпл - частота(10..10000)(Hz), | длительность звука(4..10000)(ms), | длительность семпла(4..10000)(ms)}
  {2000, 600, 800}
};

//Массив мелодий ошибок
const uint16_t error_sound[][2] PROGMEM = {
  SOUND_PATTERN(_error_low),
  SOUND_PATTERN(_error_high)
};


//Основные звуки
const uint16_t _pass_error[][3] PROGMEM = { //массив семплов || {семпл - частота(10..10000)(Hz), | длительность звука(4..10000)(ms), | длительность семпла(4..10000)(ms)}
  {500, 500, 600}, {500, 500, 600}
};
const uint16_t _reset_settings[][3] PROGMEM = { //массив семплов || {семпл - частота(10..10000)(Hz), | длительность звука(4..10000)(ms), | длительность семпла(4..10000)(ms)}
  {1500, 500, 600}, {1000, 500, 600}, {500, 500, 600}
};
const uint16_t _alarm_disable[][3] PROGMEM = { //массив семплов || {семпл - частота(10..10000)(Hz), | длительность звука(4..10000)(ms), | длительность семпла(4..10000)(ms)}
  {1000, 200, 210}, {500, 200, 210}
};
const uint16_t _alarm_wait[][3] PROGMEM = { //массив семплов || {семпл - частота(10..10000)(Hz), | длительность звука(4..10000)(ms), | длительность семпла(4..10000)(ms)}
  {1500, 100, 200}, {1500, 100, 200}, {1500, 100, 200}
};
const uint16_t _hour_sound[][3] PROGMEM = { //массив семплов || {семпл - частота(10..10000)(Hz), | длительность звука(4..10000)(ms), | длительность семпла(4..10000)(ms)}
  {800, 225, 225}, {500, 300, 300}, {600, 250, 250}, {400, 350, 350}
};
const uint16_t _timer_warn[][3] PROGMEM = { //массив семплов || {семпл - частота(10..10000)(Hz), | длительность звука(4..10000)(ms), | длительность семпла(4..10000)(ms)}
  {1500, 300, 350}, {1000, 300, 350}, {1500, 300, 350}, {1000, 300, 1500}
};

//Массив основных мелодий
const uint16_t general_sound[][2] PROGMEM = {
  SOUND_PATTERN(_pass_error),
  SOUND_PATTERN(_reset_settings),
  SOUND_PATTERN(_alarm_disable),
  SOUND_PATTERN(_alarm_wait),
  SOUND_PATTERN(_timer_warn),
  SOUND_PATTERN(_hour_sound)
};


//Настройки треков плеера
#define PLAYER_NUMBERS_FOLDER 1      //папка с озвучкой цифр(1..16)
#define PLAYER_NUMBERS_START 3       //первый трек озвучки цифр(1..255)("ноль".."девятнадцать", "двадцать".."девяносто", "сто".."тысяча")
#define PLAYER_NUMBERS_OTHER 1       //первый трек дополнительной озвучки цифр(1..255)("одна", "две")

#define PLAYER_END_NUMBERS_FOLDER 2  //папка с озвучкой окончаний цифр(1..16)
#define PLAYER_TIME_MINS_START 1     //первый трек озвучки минут(1..255)("минута", "минуты", "минут")
#define PLAYER_TIME_HOUR_START 4     //первый трек озвучки часа(1..255)("час", "часа", "часов")
#define PLAYER_SENS_TEMP_START 7     //первый трек озвучки температуры(1..255)("градус", "градуса", "градусов")
#define PLAYER_SENS_CEIL_START 10    //первый трек озвучки целых чисел(1..255)("целая", "целых")
#define PLAYER_SENS_DEC_START 12     //первый трек озвучки десятых долей чисел(1..255)("десятая", "десятых")
#define PLAYER_SENS_HUM_START 14     //первый трек озвучки влажности(1..255)("процент", "процента", "процентов")
#define PLAYER_SENS_PRESS_START 17   //первый трек озвучки давления(1..255)("миллиметр", "миллиметра", "миллиметров")
#define PLAYER_SENS_PRESS_OTHER 20   //трек дополнительной озвучки давления(1..255)("ртутного столба")

#define PLAYER_MENU_FOLDER 3         //папка с озвучкой меню(1..16)
#define PLAYER_MAIN_MENU_START 1     //первый трек озвучки основного меню(1..255)
#define PLAYER_FAST_MENU_START 13    //первый трек озвучки быстрого меню(1..255)
#define PLAYER_DEBUG_MENU_START 17   //первый трек озвучки меню отладки(1..255)

#define PLAYER_GENERAL_FOLDER 4      //папка с основной озвучкой(1..16)
#define PLAYER_ALARM_DISABLE_SOUND 1 //трек озвучки оповещения отключения будильника(1..255)("будильник отключен")
#define PLAYER_ALARM_WAIT_SOUND 2    //трек озвучки оповещения ожидания будильника(1..255)("будильник отложен")
#define PLAYER_TIME_NOW_SOUND 3      //трек озвучки текущего времени(1..255)("текущее время")
#define PLAYER_TEMP_SOUND 4          //трек озвучки температуры(1..255)("температура")
#define PLAYER_PRESS_SOUND 5         //трек озвучки давления(1..255)("давление")
#define PLAYER_HUM_SOUND 6           //трек озвучки влажности(1..255)("влажность")
#define PLAYER_RESET_SOUND 7         //трек озвучки сброса настроек(1..255)("сброс настроек завершен")
#define PLAYER_DEBUG_SOUND 8         //трек озвучки отладки(1..255)("меню отладки, введите пароль")
#define PLAYER_TEST_SOUND 9          //трек озвучки режима тестирования(1..255)("тестирование системы")
#define PLAYER_ERROR_SOUND 10        //трек озвучки ошибок(1..255)("ошибка")
#define PLAYER_FIRMWARE_SOUND 11     //трек озвучки версии прошивки(1..255)("версия прошивки")
#define PLAYER_RADIO_SOUND 12        //трек озвучки меню радио(1..255)("радио")
#define PLAYER_TIMER_SOUND 13        //трек озвучки меню таймера(1..255)("таймер")
#define PLAYER_STOPWATCH_SOUND 14    //трек озвучки меню секундомера(1..255)("секундомер")
#define PLAYER_ALARM_SET_SOUND 15    //трек озвучки меню настройки будильника(1..255)("настройка будильника")
#define PLAYER_TIME_SET_SOUND 16     //трек озвучки меню настройки времени(1..255)("настройка времени")
#define PLAYER_TIMER_SET_SOUND 17    //трек озвучки меню настройки таймера(1..255)("настйрока таймера")
#define PLAYER_PASS_SOUND 18         //трек озвучки неверного пароля(1..255)
#define PLAYER_HOUR_SOUND 19         //трек озвучки смены часа(1..255)
#define PLAYER_TIMER_WARN_SOUND 20   //трек озвучки окончания таймера(1..255)

#define PLAYER_ALARM_FOLDER 5        //папка с озвучкой будильника(1..16)
#define PLAYER_ALARM_START 1         //первый трек будильника(1..255)
#define PLAYER_ALARM_MAX 3           //количество треков будильника(1..99)


//Настройки плеера
#define PLAYER_START_VOL 50          //громкость плеера при запуске(10..100)(%)
#define PLAYER_MAX_BUFFER 10         //максимальное количество ячеек в буфере воспроизведения(1..25)
#define PLAYER_COMMAND_WAIT 200      //ожидание перед отправкой новой команды(50..300)(мс)
#define PLAYER_START_WAIT 500        //ожидание инициализации плеера(500..1000)(мс)
#define PLAYER_BUSY_WAIT 200         //ожидание сигнала занятости плеера(50..300)(мс)
#define PLAYER_UART_SPEED 9600       //скорость UART плеера(9600)

//Настройки радиоприемника
#define RADIO_MAX_STATIONS 10        //максимальное количество ячеек радиостанций(1..10)
#define RADIO_UPDATE_TIME 500        //время обновления информации в режиме радио(100..1000)(мс)
#define RADIO_SHOW_TIME 1000         //время отображения номера станции в режиме радио(500..1500)(мс)
#define RADIO_ANIM_TIME 50           //время анимации в режиме радио(30..100)(мс)
#define RADIO_FAST_TIME 1000         //время отображения быстрого меню радио(1000..5000)(мс)
#define RADIO_VOL_TIME 3000          //время отображения громкости в режиме радио(2000..10000)(мс)
#define RADIO_STATION_TIME 5000      //время отображения выбора номера радиостанции в режиме радио(2000..10000)(мс)
#define RADIO_TIMEOUT 50             //тайм-аут выхода из радио по бездействию(5..240)(сек)

//Настройки синхронизации времени
#define RTC_SYNC_TIME 15             //период попытки синхронизации с модулем реального времени при отсутствии сигнала SQW(1..30)(м)
#define RTC_SYNC_PHASE 25            //фаза попытки синхронизации с модулем реального времени при отсутствии сигнала SQW(15..45)(с)

//Настройки ошибок
#define ERROR_SHOW_TIME 5000         //время отображения ошибки(3000...15000)(мс)

//Настройки отладки
#define DEBUG_PASS_ENABLE 0          //включить запрос пароля для входа в меню отладки(0 - пароль не запрашивается | 1 - пароль запрашивается)
#define DEBUG_PASS 0                 //пароль для входа в отладку(0..9999)
#define DEBUG_PASS_ATTEMPTS 3        //количество попыток ввода пароля для входа в отладку(0..10)
#define DEBUG_PASS_BLINK_TIME 500    //период мигания активного разряда пароля(100..1000)(мс)

#define DEBUG_IR_BUTTONS_TIME 1000   //период оотображения полученного кода кнопки пульта в отладке(500..1500)(мс)
#define DEBUG_LIGHT_SENS_TIME 200    //период обновления показаний сенсора яркости освещения в отладке(50..300)(мс)
#define DEBUG_TIMEOUT 30             //тайм-аут выхода из ввода пароля отладки по бездействию(5..240)(сек)

//Настройки проверки сигнала SQW
#define SQW_TEST_TIME 2000           //время проверки сигнала с пина SQW(1500..5000)(мс)
#define SQW_MIN_TIME 650             //минимальное время сигнала с пина SQW(500..1000)(мс)
#define SQW_MAX_TIME 2000            //максимальное время сигнала с пина SQW(1500..3000)(мс)

//Настройки таймера
#define TIMER_TIMEOUT 50             //тайм-аут выхода из таймера-секундомера по бездействию(5..240)(сек)
#define TIMER_BLINK_TIME 500         //период мигания таймера по окончанию отсчета(100..1000)(мс)

//Настройки основных настроек
#define SETTINGS_TIMEOUT 30          //тайм-аут выхода из настроек по бездействию(5..240)(сек)
#define SETTINGS_BLINK_TIME 500      //период мигания активного пункта настроек(100..1000)(мс)

#define SETTINGS_WAIT_TIME 1000      //время ожидания до отображения анимации в основных настройках(500..3000)(мс)

//Настройки анимации перелистования цифр
#define FLIP_TIMEOUT 5000            //тайм-аут выхода из анимации перелистования цифр(3000..10000)(мс)

//Настройки быстрого меню
#define FAST_ANIM_TIME 150           //время анимации быстрого меню(2..65500)(мс)

#define FAST_BACKL_TIME 3000         //время отображения настройки режима подсветки(1000..5000)(мс)
#define FAST_FLIP_TIME 1500          //время отображения настройки перелистывания цифр(1000..5000)(мс)
#define FAST_DOT_TIME 1500           //время отображения настройки режима точек(1000..5000)(мс)

//Настройки сброса таймеров
#define RESET_TIME_GLITCH 5          //время ожидания глюков если таймер истек и была перезагрузка основной программы(5..15)(сек)
#define RESET_TIME_SLEEP 5           //время ожидания сна после автопоказа температуры и антиотравления(5..25)(сек)
#define RESET_TIME_BURN 5            //минимальное время для перезапуска таймера антиотравления(5..15)(сек)
#define RESET_TIME_TEMP 5            //минимальное время для перезапуска таймера автоматического показа температуры(5..15)(сек)

//Настройки режима тестирования
#define TEST_LAMP_TIME 500           //время проверки горения цифры в режиме тестирования(500..5000)(мс)
#define TEST_FIRMWARE_TIME 5000      //время отображения версии прошивки(1000..10000)(мс)
#define TEST_BACKL_BRIGHT 250        //яркость подсветки ламп в режиме тестирования(10..250)(шаг 10)
#define TEST_INDI_BRIGHT 30          //яркость цифр в режиме тестирования(0..30)
#define TEST_DOT_BRIGHT 250          //яркость точки в режиме тестирования(10..250)(шаг 10)
#define TEST_FREQ_STEP 500           //шаг перебора частот для теста бузера(100..1000)(Гц)

//Настройки обновления температуры
#define TEMP_UPDATE_TIME 2000        //таймаут обновления датчика температуры(2000..5000)(мс)

//Настройки опроса ацп
#define CYCLE_HV_CHECK 10            //количество циклов опроса ацп обратной связи ВВ преобразователя(1..15)
#define CYCLE_VCC_CHECK 10           //количество циклов опроса ацп напряжения питания(1..15)

//Настройки преобразователя
#define GEN_SPEED_X2 0               //повысить частоту генератора шим в 2 раза(0 - частота 31250 Гц | 1 - частота 62500 Гц)

//Настройки динамической индикации
#define INDI_FREQ_ADG 70             //частота динамической индикации(4 лампы - (50..100) | 6 ламп - (40..70))(гц)
#define INDI_DEAD_TIME 30            //период тишины для закрытия оптопар(0..50)
#define INDI_MIN_BRIGHT 5            //минимальная яркость индикаторов(5..10)

//Настройки напряжения
#define REFERENCE 1.1                //опорное напряжение(1.0..1.2)(в)
#define MAX_VCC 5.5                  //максимальное напряжение питания(4.0..5.5)(в)
#define MIN_VCC 4.5                  //минимальное напряжение питания(4.0..5.5)(в)

//Настройки сенсора яркости освещения
#define LIGHT_SENS_TIME 200          //время до обновления показаний сенсора яркости освещения(50..300)(мс)
#define LIGHT_SENS_GIST 8            //гистерезис результата замера АЦП сенсора яркости освещения(2..24)
#define LIGHT_SENS_START_MIN 120     //стартовое значение минимального диапазона(0..255)
#define LIGHT_SENS_START_MAX 136     //стартовое значение максимального диапазона(0..255)

//Настройки инфракрасного приемника
#define IR_HOLD_TIME 1000            //время после которого считается что кнопка пульта зажата(200..1500)(мс)
#define IR_REPEAT_TIME 70            //время ожидания новой команды повтора(50..150)(мс)
#define IR_REPEAT_WAIT_TIME 300      //таймаут ожидания новой команды повтора(150..500)(мс)
#define IR_REPEAT_ENABLE 1           //разрешить удержание кнопки пульта(0 - удержание запрещено | 1 - удержание разрешено)
#define IR_EXT_BTN_ENABLE 1          //разрешить использование расширеных кнопок пульта(0 - запрещено | 1 - разрешено)

//Настройки кнопок
#define BTN_GIST_TIME 50             //время для защиты от дребезга(0..250)(мс)
#define BTN_HOLD_TIME 550            //время после которого считается что кнопка зажата(200..1500)(мс)

#define BTN_ANALOG_GIST 25           //гистерезис значения ацп кнопок(5..50)
#define BTN_MIN_RANGE 5              //минимальный диапазон аналоговых кнопок
#define BTN_MAX_RANGE 255            //максимальный диапазон аналоговых кнопок

//Настройки звука кнопок
#define KNOCK_SOUND_FREQ 1000        //частота звука клавиш(10..10000)(Гц)
#define KNOCK_SOUND_TIME 30          //длительность звука клавиш(10..500)(мс)

//Настройки SD плеера
#define DAC_BUFF_SIZE 128            //размер буфера ЦАП(128..254)
#define DAC_READ_DEPTH 8             //глубина буферизации чтения данных ЦАП(8..16)(шаг 8)
#define DAC_FAT_DEPTH 16             //глубина буферизации чтения данных файловой таблицы(8..64)(шаг 8)
#define DAC_INIT_ATTEMPTS 3          //количество попыток инициализации карты(1..5)

//Настройки памяти
#define EEPROM_BLOCK_NULL 0          //начальный блок памяти(0..511)
#define EEPROM_BLOCK_MAX 1023        //максимальная ячейка памяти(1023)

//Версия прошивки
#define FIRMWARE_VERSION "1.8.5"     //версия прошивки для отображения и озвучки
