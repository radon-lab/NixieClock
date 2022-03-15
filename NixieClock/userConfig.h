#define BACKL_WS2812B 0         //использовать адресные светодиоды подсветки вместо обычных(0 - обычные светодиоды | 1 - светодиоды WS2812B)
#define BACKL_MODE 0            //режим подсветки на обычных светодиодах(0 - хардверный шим(только D11) | 1 - софтверный шим(любой пин))
#define BOARD_TYPE 0            //тип платы часов(0 - IN-12 (индикаторы стоят правильно) | 1 - IN-12 turned (индикаторы перевёрнуты) | 2 - IN-14 (обычная и neon dot) | 3 - другие индикаторы(4 лампы) | 4 - другие индикаторы(6 ламп))
#define NEON_DOT 0              //использовать неоновую лампу в качестве секундных точек(0 - точка светодиодная | 1 - точка неоновая лампа)
#define LAMP_NUM 4              //количесвто используемых ламп в часах(4 или 6)

#define BTN_TYPE 0              //тип кнопок(0 - используются цифровые кнопки | 1 - используются аналоговые кнопки)(доп кнопка в любом режиме цифровая)
#define BTN_PULL 1              //тип подтяжки цифровых кнопок(0 - кнопки подтянуты к земле (LOW_PULL) | 1 - кнопки подтянуты к питанию (HIGH_PULL))

#define BTN_R_LOW 10                 //сопротивление резистора нижнего плеча клавиш(кОм)
#define BTN_SET_R_HIGH 15            //сопротивление резистора клавиши "ОК"(кОм)
#define BTN_LEFT_R_HIGH 4.7          //сопротивление резистора "ЛЕВОЙ" клавиши(кОм)
#define BTN_RIGHT_R_HIGH 0           //сопротивление резистора "ПРАВОЙ" клавиши(кОм)

#define BTN_ADD_DISABLE 0       //отключить использование порта дополнительной кнопки(0 - порт используется | 1 - порт не используется)
#define BTN_ADD_PULL 1          //тип подтяжки цифровой дополнительной кнопки(0 - кнопка подтянута к земле (LOW_PULL) | 1 - кнопка подтянута к питанию (HIGH_PULL))

#define SENS_PORT_DISABLE 0     //отключить использование порта датчиков температуры DS18xx и DHTxx(0 - порт используется | 1 - порт не используется)

#define GEN_DISABLE 0           //отключить генерацию шим для преобразователя высокого напряжения(нужно если используется внешний шим контроллер)(0 - шим на стороне МК(пин D9) | 1 - внешний шим преобразователь)
#define GEN_FEEDBACK 0          //разрешить обратную связь вв преобразователя через делитель напряжения(0 - обратная связь не используется | 1 - обратная связь используется)
#define GEN_HV_VCC 170          //напряжение питания ламп(150..190)(в)
#define GEN_HV_R_HIGH 680       //сопротивление верхнего плеча резисторного делителя(кОм)
#define GEN_HV_R_LOW 10         //сопротивление нижнего плеча резисторного делителя(кОм)

#define DEFAULT_MIN_PWM 100     //минимальная скважность(индикаторы выключены, яркость минимальная)(100..150)
#define DEFAULT_MAX_PWM 180     //максимальная скважность(индикаторы включены, яркость максимальная)(150..200)

#define INDI_FREQ_ADG 70        //частота динамической индикации(4 лампы - (62..122) | 6 ламп - (40..80))(гц)
#define INDI_BURN_TYPE 0        //тип анимации антиотравления ламп(0 - с отображением времени | 1 - без отображения времени)

#define DEFAULT_TIME_FORMAT 0   //формат времени по умолчанию(0 - 24 часа | 1 - 12 часов)
#define DEFAULT_TEMP_SENSOR 0   //датчик температуры по умолчанию(0 - DS3231(встроенный) | 1 - BME/BMP_280(180) | 2 - DHT11/12 | 3 - DHT21/22(AM2301) | 4 - DS18B20/DS18S20/DS1820)
#define DEFAULT_TEMP_CORRECT 0  //коррекция температуры по умолчанию(-127..127)(0.1°c)
#define DEFAULT_BACKL_MODE 1    //режим подсветки по умолчанию(0 - выкл | 1 - статичная | 2 - дыхание), для WS2812B(0 - выкл | 1 - статичная | 2 - дыхание | 3 - дыхание со сменой цвета при затухании | 4 - бегущий огонь | 5 - бегущий огонь со сменой цвета | 6 - волна | 7 - волна со сменой цвета | 8 - плавная смена цвета | 9 - радуга | 10 - конфетти)
#define DEFAULT_BACKL_COLOR 0   //цвет подсветки по умолчанию(только для WS2812B)(0..25)
#define DEFAULT_DOT_MODE 2      //режим работы точки по умолчанию(0 - выкл | 1 - статичная | 2 - динамичная(плавно мигает))

#define SECONDS_ANIM 1          //режим постоянной анимации секунд(только для 6-ти ламп)
#define SECONDS_ANIM_TIME 50    //скорость шага эффекта анимации секунд(только для 6-ти ламп)(50..80)(мс)

#define DEFAULT_FLIP_ANIM 1     //режим перелистования цифр по умолчанию(0 - выкл | 1 - рандомная смена эффектов | 2 - плавное угасание и появление | 3 - перемотка по порядку числа | 4 - перемотка по порядку катодов в лампе | 5 - поезд | 6 - резинка | 7 - ворота | 8 - волна | 9 - блики | 10 - испарение)

#define FLIP_MODE_2_TIME 1000   //период смены яркости эффекта номер 2(500..1500)(мс)
#define FLIP_MODE_3_TIME 80     //скорость шага эффекта номер 3(50..80)(мс)
#define FLIP_MODE_4_TIME 80     //скорость шага эффекта номер 4(30..80)(мс)
#define FLIP_MODE_5_TIME 150    //скорость шага эффекта номер 5(50..170)(мс)
#define FLIP_MODE_6_TIME 80     //скорость шага эффекта номер 6(50..150)(мс)
#define FLIP_MODE_7_TIME 100     //скорость шага эффекта номер 6(50..150)(мс)
#define FLIP_MODE_8_TIME 200    //скорость шага эффекта номер 7(100..300)(мс)
#define FLIP_MODE_9_TIME 200    //скорость шага эффекта номер 8(100..300)(мс)
#define FLIP_MODE_10_TIME 200   //скорость шага эффекта номер 9(100..300)(мс)

#define DEFAULT_NIGHT_START 20       //час перехода на ночную подсветку по умолчанию(BRIGHT_N)(NIGHT_START = NIGHT_END - только дневная подсветка)
#define DEFAULT_NIGHT_END 8          //час перехода на дневную подсветку по умолчанию(BRIGHT)

#define DEFAULT_INDI_BRIGHT 25       //яркость цифр дневная по умолчанию(0..30)
#define DEFAULT_INDI_BRIGHT_N 5      //яркость цифр ночная по умолчанию(0..30)

#define DEFAULT_BACKL_BRIGHT 150     //яркость подсветки ламп дневная(10..250)(шаг 10)
#define DEFAULT_BACKL_BRIGHT_N 60    //яркость подсветки ламп ночная(0..250)(0 - подсветка выключена)(шаг 10)
#define BACKL_MIN_BRIGHT 20          //минимальная яркость подсветки ламп(10..120)

#define BACKL_MODE_2_TIME 5000       //период эффекта подсветки номер 2(1500..12000)(мс)
#define BACKL_MODE_2_PAUSE 200       //пазуа между вспышками подсветки ламп в режиме номер 2(100..1000)(мс)
#define BACKL_MODE_2_STEP_TIME 10    //минимальный размер шага яркости подсветки номер 2(4..50)(мс)
#define BACKL_MODE_3_COLOR 25        //шаг изменения цвета эффекта подсветки номер 3(5..50)

#define BACKL_MODE_4_TIME 1000       //период эффекта подсветки номер 4(1000..5000)(мс)
#define BACKL_MODE_4_FADING 5        //количество шагов декркминента яркости эффекта подсветки номер 4(5..50)
#define BACKL_MODE_4_TAIL 3          //длинна эффекта подсветки номер 4(1..4)

#define BACKL_MODE_6_TIME 5000       //период эффекта подсветки номер 6(1000..12000)(мс)
#define BACKL_MODE_6_STEP_TIME 10    //минимальный размер шага яркости эффекта подсветки номер 6(4..50)(мс)

#define BACKL_MODE_8_TIME 30         //скорость шага эффекта подсветки "плавная смена цвета"("бегущий огонь со сменой цвета", "волна со сменой цвета")(4..100)(мс)
#define BACKL_MODE_8_COLOR 1         //шаг изменения цвета эффекта подсветки "плавная смена цвета"("бегущий огонь со сменой цвета", "волна со сменой цвета")(1..10)

#define BACKL_MODE_9_TIME 80         //скорость шага эффекта подсветки номер 9(4..100)(мс)
#define BACKL_MODE_9_STEP 15         //размер шага эффекта подсветки номер 9(1..50)

#define BACKL_MODE_10_TIME 80        //скорость шага эффекта подсветки номер 10(4..100)(мс)

#define DEFAULT_DOT_BRIGHT 50        //яркость точки дневная(10..250)(шаг 10)
#define DEFAULT_DOT_BRIGHT_N 20      //яркость точки ночная(0..250)(0 - подсветка выключена)(шаг 10)
#define DOT_TIME 250                 //время мигания точки(100..1000)(мс)
#define DOT_STEP_TIME 4              //шаг яркости точки(4..255)(мс)

#define DEFAULT_GLITCH_MODE 1        //режим глюков по умолчанию(1 - вкл | 0 - выкл)
#define GLITCH_MIN 40                //минимальное время между глюками(1..240)(сек)
#define GLITCH_MAX 160               //максимальное время между глюками(1..240)(сек)
#define GLITCH_NUM_MIN 2             //минимальное количество глюков(1..254)
#define GLITCH_NUM_MAX 6             //максимальное количество глюков(2..255)

#define BURN_TIME 40                 //период обхода индикаторов в режиме очистки(10..240)(мс)
#define BURN_LOOPS 4                 //количество циклов очистки за каждый период(1..10)
#define BURN_PERIOD 12               //период антиотравления(1..180)(мин)
#define BURN_PHASE 25                //смещение фазы периода антиотравления(5..30)(сек)

#define DEFAULT_AUTO_TEMP_TIME 60    //интервал автоматического показа температуры(5..240)(шаг - 5)(0 - выключить автоматический показ)(с)
#define AUTO_TEMP_PAUSE_TIME 5000    //пауза между сменой данных на индикаторе(1000..15000)(мс)
#define AUTO_TEMP_ANIM_TIME 300      //скорость шага анимации(50..500)(мс)

#define DEFAULT_KNOCK_SOUND 1        //запрет звука нажатия клавиш по умолчанию(1 - звук вкл | 0 - звук выкл)
#define KNOCK_SOUND_FREQ 1000        //частота звука клавиш(10..10000)(Гц)
#define KNOCK_SOUND_TIME 30          //длительность звука клавиш(10..500)(мс)

#define DEFAULT_HOUR_SOUND_START 8   //время начала звука смены часа по умолчанию(0..23)(ч)(DEFAULT_HOUR_SOUND_START = DEFAULT_HOUR_SOUND_END - выключить звук)
#define DEFAULT_HOUR_SOUND_END 23    //время окончания звука смены часа по умолчанию(0..23)(ч)
#define HOUR_SOUND_FREQ 1000         //частота звука смены часа(10..10000)(Гц)
#define HOUR_SOUND_TIME 500          //длительность звука смены часа(10..500)(мс)

#define ALARM_TYPE 1                 //использовать только один будильник(0 - будильник отключен | 1 - один будильник(простое меню будильника) | 2 - неограниченое количество будильников(сложное меню будильников))

#define DEFAULT_ALARM_TIME_HH 8      //час будильника по умолчанию(0..23)
#define DEFAULT_ALARM_TIME_MM 0      //минута будильника по умолчанию(0..59)
#define DEFAULT_ALARM_MODE 0         //режим будильника по умолчанию(0..2)(0 - выкл | 2 - одиночный | 1 - вкл)
#define DEFAULT_ALARM_SOUND 0        //мелодия будильника по умолчанию(0..3)

#define ALARM_TIMEOUT_SOUND 1        //таймаут до автоматического включения ожидания будильника по умолчанию(0..240)(0 - выкл повторное включение)(м)
#define ALARM_TIMEOUT 15             //таймаут до полного отключения будильника по умолчанию(1..240)(м)
#define ALARM_WAINT 4                //ожидание будильника для повторного включения по умолчанию(1..240)(0 - выкл ожидание по короткому нажатию)(м)

#define ALM_ON_BLINK_DOT 3           //режим мигания точек если хоть один будильник включен(0 - выкл | 1 - статичная | 2 - без реакции | 3 - мигает один раз в секунду | 4 - мигает два раза в секунду)
#define ALM_WAINT_BLINK_DOT 4        //режим мигания точек если ожидаем повторного сигнала(0 - выкл | 1 - статичная | 2 - без реакции | 3 - мигает один раз в секунду | 4 - мигает два раза в секунду)
#define ALM_BLINK_TIME 500           //период мигания времени во время активного будильника(100..1000)(мс)
#define ALM_OFF_SOUND_FREQ 2000      //частота звука подтверждения отключения будильника(10..10000)(Гц)
#define ALM_OFF_SOUND_TIME 1000      //длительность звука подтверждения отключения будильника(10..2000)(мс)

#define TIMER_TIME 60                //значение таймера по умолчанию(1..5999)(с)
