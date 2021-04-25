#define BOARD_TYPE 4  // тип платы часов(0 - IN-12 (индикаторы стоят правильно) | 1 - IN-12 turned (индикаторы перевёрнуты) | 2 - IN-14 (обычная и neon dot) | 3 - другие индикаторы)
#define BT_UART_BAUND 9600           //скорость последовательного порта UART установленного в блютуз модуле(по умолчанию 9600 или 38400)

#define DEFAULT_TIME_FORMAT 0        //формат времени по умолчанию(0 - 24 часа | 1 - 12 часов)
#define DEFAULT_TEMP_SENSOR 0        //датчик температуры по умолчанию(0 - DS3231(встроенны) | 1 - BME/BMP_280(180))
#define DEFAULT_TEMP_CORRECT 0       //коррекция температуры по умолчанию(-127..127)(0.1°c)
#define DEFAULT_BACKL_MODE 1         //режим подсветки по умолчанию(0 - выкл | 1 - статичная | 2 - динамичная(дыхание))

// 0 - выкл
// 1 - автоматическоя смена эффектов
// 2 - плавное угасание и появление (рекомендуемая скорость: 100-150)
// 3 - перемотка по порядку числа (рекомендуемая скорость: 50-80)
// 4 - перемотка по порядку катодов в лампе (рекомендуемая скорость: 30-80)
// 5 - поезд (рекомендуемая скорость: 50-170)
// 6 - резинка (рекомендуемая скорость: 50-150)
#define DEFAULT_FLIP_ANIM 2          //режим перелистования цифр по умолчанию
       
#define DEFAULT_NIGHT_START 20       //час перехода на ночную подсветку по умолчанию(BRIGHT_N)(DEFAULT_NIGHT_START = DEFAULT_NIGHT_END - только дневная подсветка)
#define DEFAULT_NIGHT_END 8          //час перехода на дневную подсветку по умолчанию(BRIGHT)(DEFAULT_NIGHT_START = DEFAULT_NIGHT_END - только дневная подсветка)
#define DEFAULT_INDI_BRIGHT 23       //яркость цифр дневная по умолчанию(0..30)
#define DEFAULT_INDI_BRIGHT_N 5      //яркость цифр ночная по умолчанию(0..30)

#define DEFAULT_BACKL_BRIGHT 255     //яркость подсветки ламп дневная по умолчанию(1..255)
#define DEFAULT_BACKL_BRIGHT_N 120   //яркость подсветки ламп ночная по умолчанию(0..255)(0 - подсветка выключена)
#define DEFAULT_BACKL_MIN_BRIGHT 32  //минимальная яркость подсветки ламп в режиме дыхание по умолчанию(0..255)
#define DEFAULT_BACKL_PAUSE 200      //пазуа между вспышками подсветки ламп в режиме дыхание по умолчанию(100..1000)(мс)
#define DEFAULT_BACKL_STEP 1         //шаг яркости подсветки по умолчанию(1..255)
#define DEFAULT_BACKL_TIME 12000     //период подсветки по умолчанию(100..12000)(мс)

#define DEFAULT_DOT_BRIGHT 50       //яркость точки дневная по умолчанию(1..255)
#define DEFAULT_DOT_BRIGHT_N 20      //яркость точки ночная по умолчанию(1..255)
#define DEFAULT_DOT_TIMER 4          //шаг яркости точки по умолчанию(4..255)(мс)
#define DEFAULT_DOT_TIME 250         //время мигания точки по умолчанию(100..1000)(мс)

#define DEFAULT_GLITCH_MODE 1        // минимальное время между глюками по умолчанию(1 - вкл | 0 - выкл)
#define DEFAULT_GLITCH_MIN 40        // минимальное время между глюками по умолчанию(1..240)(сек)
#define DEFAULT_GLITCH_MAX 160       // максимальное время между глюками по умолчанию(1..240)(сек)

#define DEFAULT_FLIP_TIME 120        //шаг яркости эффекта номер 2 дневной по умолчанию(мс)

#define DEFAULT_BURN_TIME 40         //период обхода индикаторов в режиме очистки по умолчанию(10..240)(мс)
#define DEFAULT_BURN_LOOPS 4         //количество циклов очистки за каждый период по умолчанию
#define DEFAULT_BURN_PERIOD 12       //период антиотравления по умолчанию(1..240)(мин)
#define BURN_PHASE 25                //смещение фазы периода антиотравления(5..30)(сек)

#define DEFAULT_KNOCK_SOUND 1        //запрет звука нажатия клавиш(1 - звук вкл | 0 - звук выкл)
#define KNOCK_SOUND_FREQ 1000        //частота звука клавиш(10..10000)(Гц)
#define KNOCK_SOUND_TIME 30          //длительность звука клавиш(10..500)(мс)

#define DEFAULT_HOUR_SOUND_START 8   //время начала звука смены часа(0..23)(ч)(DEFAULT_HOUR_SOUND_START = DEFAULT_HOUR_SOUND_END - выключить звук)
#define DEFAULT_HOUR_SOUND_END 23    //время окончания звука смены часа(0..23)(ч)(DEFAULT_HOUR_SOUND_START = DEFAULT_HOUR_SOUND_END - выключить звук)
#define HOUR_SOUND_FREQ 1000         //частота звука смены часа(10..10000)(Гц)
#define HOUR_SOUND_TIME 500          //длительность звука смены часа(10..500)(мс)
 
#define DEFAULT_ALM_TIMEOUT_SOUND 1  //время до автоматического включения ожидания будильника по умолчанию(1..240)(мин)(0 - выкл повторное включение)
#define DEFAULT_ALM_TIMEOUT 15       //время до полного отключения будильника по умолчанию(1..240)(мин)
#define DEFAULT_ALM_WAINT 4          //время ожидания будильника для повторного включения после короткого нажатия по умолчанию(1..240)(мин)(0 - выкл ожидание по короткому нажатию)
#define DEFAULT_ALM_MELODY 0         //мелодия будильника по умолчанию
#define ALM_BLINK_TIME 500           //период мигания времени во время активного будильника(100..1000)(мс)

const uint16_t _sound_1[][3] PROGMEM = { //массив семплов 1-й мелодии будильника
  {550, 100, 150}, //семпл 0 - частота(10..10000)(Hz) | длительность звука(50..500)(ms) | длительность паузы(50..1000)(ms)
  {550, 100, 150}, //семпл 1 - частота(10..10000)(Hz) | длительность звука(50..500)(ms) | длительность паузы(50..1000)(ms)
  {550, 100, 150}, //семпл 2 - частота(10..10000)(Hz) | длительность звука(50..500)(ms) | длительность паузы(50..1000)(ms)
  {550, 100, 150}, //семпл 3 - частота(10..10000)(Hz) | длительность звука(50..500)(ms) | длительность паузы(50..1000)(ms)
  {1500, 200, 250}, //семпл 4 - частота(10..10000)(Hz) | длительность звука(50..500)(ms) | длительность паузы(50..1000)(ms)
  {1500, 200, 250}, //семпл 5 - частота(10..10000)(Hz) | длительность звука(50..500)(ms) | длительность паузы(50..1000)(ms)
  {550, 100, 150}, //семпл 6 - частота(10..10000)(Hz) | длительность звука(50..500)(ms) | длительность паузы(50..1000)(ms)
  {550, 100, 150}, //семпл 7 - частота(10..10000)(Hz) | длительность звука(50..500)(ms) | длительность паузы(50..1000)(ms)
  {550, 100, 550}  //семпл 8 - частота(10..10000)(Hz) | длительность звука(50..500)(ms) | длительность паузы(50..1000)(ms)
};
const uint16_t alarm_sound[][2] = {  //массив мелодий будильника
  {(uint16_t)&_sound_1, sizeof(_sound_1) / 6}
};

#define SETTINGS_TIMEOUT 30          //тайм-аут выхода из настройки времени по бездействию(5..60)(сек)
#define SETTINGS_BLINK_TIME 500      //период мигания активного пункта меню(100..1000)(мс)

#define WAINT_BEFORE_REBOOT 1500     //ожидание перед перзагрузкой для сброса настроек(500..2000)(мс)

#define SHOW_TIME 3000               //время отображения дополнительной информации(мс)
#define ANIM_TIME 150                //время анимации(мс)
#define SWITCH_TIME 1500             //время отображения настроек(мс)

#define MIN_PWM 100                  //минимальная скважность(индикаторы выключены, яркость минимальная)
#define MAX_PWM 180                  //максимальная скважность(индикаторы включены, яркость максимальная)

#define BTN_GIST_TICK 8              //количество циклов для защиты от дребезга(0..255)(1 цикл -> 4мс)
#define BTN_HOLD_TICK 127            //количество циклов после которого считается что кнопка зажата(0..255)(1 цикл -> 4мс)

#define EEPROM_BLOCK_TIME 0             //блок памяти времени
#define EEPROM_BLOCK_ALARM 8            //блок памяти будильников
#define EEPROM_BLOCK_SETTINGS_BRIGHT 28 //блок памяти настроек свечения
#define EEPROM_BLOCK_SETTINGS_MAIN 45   //блок памяти основных настроек
#define EEPROM_BLOCK_VERSION_FW 255     //блок памяти версии прошивки

#define COMMAND_SEND_TIME 0x50          //комманда отправить текущее время
#define COMMAND_GET_TIME 0x51           //комманда принять текущее время
#define COMMAND_SEND_ALARM 0x52         //комманда отправить настройки будильников
#define COMMAND_GET_ALARM 0x53          //комманда принять настройки будильников
#define COMMAND_SEND_SET_BRIGHT 0x54    //комманда отправить настройки свечения
#define COMMAND_GET_SET_BRIGHT 0x55     //комманда принять настройки свечения
#define COMMAND_SEND_SET_MAIN 0x56      //комманда отправить основные настройки
#define COMMAND_GET_SET_MAIN 0x57       //комманда принять основные настройки

#define COMMAND_SEND_VERSION 0xFC       //комманда отправить версию прошивки
#define COMMAND_RESET_SETTINGS 0xFE     //комманда сбросить настройки часов до заводских

#define ANSWER_SEND_TIME 0x10           //комманда ответа текущего времени
#define ANSWER_SEND_ALARM 0x11          //комманда ответа настроек будильников
#define ANSWER_SEND_SET_BRIGHT 0x12     //комманда ответа настроек свечения
#define ANSWER_SEND_SET_MAIN 0x13       //комманда ответа основных настроек

#define ANSWER_OK 0xFF                  //комманда ответа успешного принятия пакета
#define ANSWER_RESET_OK 0x03            //комманда ответа успешного сброса настроек
#define ANSWER_CRC_ERROR 0x02           //комманда ответа - несовпадения контрольной суммы
#define ANSWER_LENGTH_ERROR 0x01        //комманда ответа - несовпадения длинны пакета
#define ANSWER_UNKNOWN_COMMAND 0x00     //комманда ответа - неизвестная команда
