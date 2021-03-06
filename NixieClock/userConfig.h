//Основные настройки периферии
#define BACKL_TYPE 0              //тип родсветки индикаторов(0 - хардверный шим(обычные светодиоды)(только D11)(только без SD плеера и хардверным UART) | 1 - софтверный шим(обычные светодиоды)(любой пин)(только без IR приемника) | 2 - светодиоды WS2812B(любой пин))
#define BOARD_TYPE 0              //тип платы часов(0 - IN-12 (индикаторы стоят правильно) | 1 - IN-12 turned (индикаторы перевёрнуты) | 2 - IN-14 (обычная и neon dot) | 3 - другие индикаторы(4 лампы) | 4 - другие индикаторы(6 ламп))
#define NEON_DOT 0                //использовать неоновую лампу в качестве секундных точек(0 - точка светодиодная(только без SD плеера) | 1 - точка неоновая лампа)
#define LAMP_NUM 4                //количесвто используемых ламп в часах(4 или 6)

#define PLAYER_TYPE 0             //тип плеера озвучки(0 - бузер | 1 - DF плеер | 2 - SD плеер)
#define PLAYER_UART_MODE 0        //режим работы UART DF плеера(0 - хардверный UART(только D1) | 1 - софтверный UART(любой пин)(только если адресные светодиоды или софтовый шим подсветки))

#define SENS_BME_ENABLE 1         //включить поддержку датчиков температуры BMPxxx и BMExxx(0 - датчик не используется | 1 - датчик используется)
#define SENS_PORT_ENABLE 0        //включить использование порта датчиков температуры DS18xx и DHTxx(0 - порт не используется | 1 - порт используется)
#define DOTS_PORT_ENABLE 0        //включить использование порта разделительных точек в индикаторах(0 - порт не используется | 1 - порт используется)
#define AMP_PORT_ENABLE 0         //включить использование порта управления питанием усилителя(0 - порт не используется | 1 - порт используется)
#define SQW_PORT_ENABLE 0         //включить использование порта и синхронизации по SQW(0 - порт и синхронизация не используются | 1 - порт и синхронизация используются)
#define IR_PORT_ENABLE 0          //включить использование порта инфракрасного приемника(0 - порт не используется | 1 - порт используется)

//Настройки преобразователя
#define GEN_ENABLE 1              //включить генерацию шим для преобразователя высокого напряжения(нужно если используется внешний шим контроллер)(0 - внешний шим преобразователь | 1 - шим на стороне МК(пин D9))
#define GEN_FEEDBACK 0            //разрешить обратную связь вв преобразователя через делитель напряжения(0 - обратная связь не используется | 1 - обратная связь используется)
#define GEN_HV_VCC 170            //напряжение питания ламп(150..190)(в)
#define GEN_HV_R_HIGH 680         //сопротивление верхнего плеча резисторного делителя(кОм)
#define GEN_HV_R_LOW 10           //сопротивление нижнего плеча резисторного делителя(кОм)

#define DEFAULT_MIN_PWM 100       //минимальная скважность(индикаторы выключены, яркость минимальная)(100..150)(шаг 5)
#define DEFAULT_MAX_PWM 180       //максимальная скважность(индикаторы включены, яркость максимальная)(150..200)(шаг 5)

//Настройки сенсора яркости освещения
#define LIGHT_SENS_ENABLE 0       //включить сенсор яркости освещения(0 - сенсор не используется | 1 - сенсор используется)
#define LIGHT_SENS_TIME 100       //время обновления показаний сенсора яркости освещения(50..300)(мс)
#define LIGHT_SENS_PULL 0         //тип подтяжки сенсора яркости освещения(0 - сенсор подтянут к земле (LOW_PULL) | 1 - сенсор подтянут к питанию (HIGH_PULL))

//Настройки кнопок
#define BTN_TYPE 0                //тип основных кнопок(0 - используются цифровые кнопки | 1 - используются аналоговые кнопки)
#define BTN_PULL 1                //тип подтяжки основных цифровых кнопок(0 - кнопки подтянуты к земле (LOW_PULL) | 1 - кнопки подтянуты к питанию (HIGH_PULL))

#define BTN_ADD_TYPE 0            //тип дополнительной кнопки(0 - кнопка не используется | 1 - цифровая кнопка | 2 - аналоговая кнопка(только если основные кнопки аналоговые))
#define BTN_ADD_PULL 1            //тип подтяжки цифровой дополнительной кнопки(0 - кнопка подтянута к земле (LOW_PULL) | 1 - кнопка подтянута к питанию (HIGH_PULL))

//Настройки аналоговых кнопок
#define BTN_R_LOW 10              //сопротивление резистора нижнего плеча клавиш(кОм)
#define BTN_ADD_R_HIGH 52.7       //сопротивление резистора клавиши "ДОП"(полное сопротивление от кнопки до пина)(кОм)
#define BTN_SET_R_HIGH 19.7       //сопротивление резистора клавиши "ОК"(полное сопротивление от кнопки до пина)(кОм)
#define BTN_LEFT_R_HIGH 4.7       //сопротивление резистора "ЛЕВОЙ" клавиши(полное сопротивление от кнопки до пина)(кОм)
#define BTN_RIGHT_R_HIGH 0        //сопротивление резистора "ПРАВОЙ" клавиши(полное сопротивление от кнопки до пина)(кОм)

//Подключение аналоговых кнопок к порту МК
//               __/ __VCC          __/ __VCC         __/ __VCC         __/ __VCC
//              |                  |                 |                 |
//  analog------|------|4.7K|------|------|15K|------|------|47K|------|
//              |
//             10K
//              |         
//             GND


//Основные настройки прошивки
#define DEFAULT_TIME_FORMAT 0      //формат времени по умолчанию(0 - 24 часа | 1 - 12 часов)
#define DEFAULT_KNOCK_SOUND 1      //звук нажатия клавиш по умолчанию(0 - звук выключен | 1 - звук включен)
#define DEFAULT_TEMP_CORRECT 0     //коррекция температуры по умолчанию(-127..127)(0.1°c)(поддерживаются датчики - DS3231(встроенный), BMP180/BMP085/BMP280/BME280, DHT11/DHT12/MW33, DHT21/DHT22(AM2301/AM2302), DS18B20/DS18S20/DS1820)
#define DEFAULT_BACKL_MODE 1       //режим подсветки по умолчанию(0 - выкл | 1 - статичная | 2 - дыхание), для WS2812B(0 - выкл | 1 - статичная | 2 - дыхание | 3 - дыхание со сменой цвета при затухании | 4 - бегущий огонь | 5 - бегущий огонь со сменой цвета | 6 - волна | 7 - волна со сменой цвета | 8 - плавная смена цвета | 9 - радуга | 10 - конфетти)
#define DEFAULT_BACKL_COLOR 0      //цвет подсветки по умолчанию(только для WS2812B)(0..25)
#define DEFAULT_DOT_MODE 2         //режим работы точки по умолчанию(0 - выкл | 1 - статичная | 2 - динамичная(плавно мигает))

//Настройки анимации перелистования цифр
#define SECONDS_ANIM 1             //режим постоянной анимации секунд(только для 6-ти ламп)
#define SECONDS_ANIM_TIME 50       //скорость шага эффекта анимации секунд(только для 6-ти ламп)(50..80)(мс)

#define DEFAULT_FLIP_ANIM 1        //режим перелистывания цифр по умолчанию(0 - выкл | 1 - рандомная смена эффектов | 2 - плавное угасание и появление | 3 - перемотка по порядку числа | 4 - перемотка по порядку катодов в лампе | 5 - поезд | 6 - резинка | 7 - ворота | 8 - волна | 9 - блики | 10 - испарение)

#define FLIP_MODE_2_TIME 1000      //период смены яркости эффекта номер 2(500..1500)(мс)
#define FLIP_MODE_3_TIME 80        //скорость шага эффекта номер 3(50..80)(мс)
#define FLIP_MODE_4_TIME 80        //скорость шага эффекта номер 4(50..80)(мс)
#define FLIP_MODE_5_TIME 150       //скорость шага эффекта номер 5(50..200)(мс)
#define FLIP_MODE_6_TIME 80        //скорость шага эффекта номер 6(50..200)(мс)
#define FLIP_MODE_7_TIME 200       //скорость шага эффекта номер 6(100..300)(мс)
#define FLIP_MODE_8_TIME 200       //скорость шага эффекта номер 7(100..300)(мс)
#define FLIP_MODE_9_TIME 200       //скорость шага эффекта номер 8(100..300)(мс)
#define FLIP_MODE_10_TIME 200      //скорость шага эффекта номер 9(100..300)(мс)

//Настройки часа смены общей яркости
#define DEFAULT_NIGHT_START 20     //час перехода на ночную подсветку по умолчанию(BRIGHT_N)(NIGHT_START = NIGHT_END - только дневная подсветка)
#define DEFAULT_NIGHT_END 8        //час перехода на дневную подсветку по умолчанию(BRIGHT)

//Настройки яркости индикаторов
#define DEFAULT_INDI_BRIGHT 25     //яркость цифр дневная по умолчанию(0..30)
#define DEFAULT_INDI_BRIGHT_N 5    //яркость цифр ночная по умолчанию(0..30)

//Настройки подсветки
#define DEFAULT_BACKL_BRIGHT 150   //яркость подсветки ламп дневная по умолчанию(10..250)(шаг 10)
#define DEFAULT_BACKL_BRIGHT_N 60  //яркость подсветки ламп ночная по умолчанию(0..250)(0 - подсветка выключена)(шаг 10)
#define BACKL_MIN_BRIGHT 20        //минимальная яркость подсветки ламп(10..120)

#define BACKL_MODE_2_TIME 5000     //период эффекта подсветки номер 2(1500..12000)(мс)
#define BACKL_MODE_2_PAUSE 200     //пазуа между вспышками подсветки ламп в режиме номер 2(100..1000)(мс)
#define BACKL_MODE_2_STEP_TIME 10  //минимальный размер шага яркости подсветки номер 2(4..50)(мс)
#define BACKL_MODE_3_COLOR 25      //шаг изменения цвета эффекта подсветки номер 3(5..50)

#define BACKL_MODE_4_TIME 1000     //период эффекта подсветки номер 4(1000..5000)(мс)
#define BACKL_MODE_4_FADING 5      //количество шагов декркминента яркости эффекта подсветки номер 4(5..50)
#define BACKL_MODE_4_TAIL 3        //длинна эффекта подсветки номер 4(1..4)

#define BACKL_MODE_6_TIME 5000     //период эффекта подсветки номер 6(1000..12000)(мс)
#define BACKL_MODE_6_STEP_TIME 10  //минимальный размер шага яркости эффекта подсветки номер 6(4..50)(мс)

#define BACKL_MODE_8_TIME 30       //скорость шага эффекта подсветки "плавная смена цвета"("бегущий огонь со сменой цвета", "волна со сменой цвета")(4..100)(мс)
#define BACKL_MODE_8_COLOR 1       //шаг изменения цвета эффекта подсветки "плавная смена цвета"("бегущий огонь со сменой цвета", "волна со сменой цвета")(1..10)

#define BACKL_MODE_9_TIME 80       //скорость шага эффекта подсветки номер 9(4..100)(мс)
#define BACKL_MODE_9_STEP 15       //размер шага эффекта подсветки номер 9(1..50)

#define BACKL_MODE_10_TIME 80      //скорость шага эффекта подсветки номер 10(4..100)(мс)

//Настройки секундных точек
#define DEFAULT_DOT_BRIGHT 50      //яркость точки дневная по умолчанию(10..250)(шаг 10)
#define DEFAULT_DOT_BRIGHT_N 20    //яркость точки ночная по умолчанию(0..250)(0 - точки выключены)(шаг 10)
#define DOT_TIME 250               //время мигания точки(100..1000)(мс)
#define DOT_STEP_TIME 4            //шаг яркости точки(4..10)(мс)

//Настройки глюков
#define DEFAULT_GLITCH_MODE 1      //режим глюков по умолчанию(1 - вкл | 0 - выкл)
#define GLITCH_MIN_TIME 40         //минимальное время между глюками(1..3600)(с)
#define GLITCH_MAX_TIME 160        //максимальное время между глюками(1..3600)(с)
#define GLITCH_TIME 20             //время одного глюка(4..25)(мс)
#define GLITCH_NUM_MIN 2           //минимальное количество глюков(1..4)
#define GLITCH_NUM_MAX 6           //максимальное количество глюков(4..15)

//Настройки анти-отравления
#define DEFAULT_BURN_MODE 2        //режим анимации антиотравления ламп по умолчанию(0 - перебор всех индикаторов | 1 - перебор одного индикатора | 2 - перебор одного индикатора с отображением времени)

#define BURN_TIME 40               //период обхода индикаторов в режиме очистки(10..240)(мс)
#define BURN_LOOPS 4               //количество циклов очистки за каждый период(1..10)
#define BURN_PERIOD 12             //период антиотравления(1..180)(м)
#define BURN_PHASE 25              //смещение фазы периода антиотравления(5..30)(с)

//Настройки авто-показа температуры
#define DEFAULT_AUTO_TEMP_TIME 60  //интервал автоматического показа температуры(5..240)(шаг - 5)(0 - выключить автоматический показ)(с)
#define AUTO_TEMP_PAUSE_TIME 5000  //пауза между сменой данных на индикаторе(1000..15000)(мс)
#define AUTO_TEMP_ANIM_TIME 300    //скорость шага анимации(50..500)(мс)

//Настройки показа температуры/даты
#define SHOW_TEMP_TIME 3000        //время отображения температуры(влажности/давления)(1000..15000)(мс)
#define SHOW_DATE_TIME 3000        //время отображения даты(1000..15000)(мс)

//Настройки звука смены часа
#define DEFAULT_HOUR_SOUND_START 8 //время начала звука смены часа по умолчанию(0..23)(ч)(DEFAULT_HOUR_SOUND_START = DEFAULT_HOUR_SOUND_END - выключить звук)
#define DEFAULT_HOUR_SOUND_END 23  //время окончания звука смены часа по умолчанию(0..23)(ч)

//Настройки радиоприемника
#define DEFAULT_RADIO_STATIONS 877, 1025, 1057 //радиостанции по умолчанию(не более 9 пресетов)(870..1080)(МГц * 10)
#define DEFAULT_RADIO_VOLUME 15    //громкость радио по умолчанию(0..15)
#define RADIO_MONO 0               //режим работы аудиовыхода(0 - стерео | 1 - моно)
#define RADIO_BASS 1               //усиление низких частот(0 - без усиления | 1 - с усилением)
#define RADIO_SEEK_SNR 8           //пороговое значение SNR для системы автопоиска станций(0..15)

#define RADIO_SAVE_SOUND_FREQ 2000 //частота звука успешной записи радиостанции в память(10..10000)(Гц)
#define RADIO_SAVE_SOUND_TIME 300  //длительность звука успешной записи радиостанции в память(10..500)(мс)

//Настройки плеера
#define DEFAULT_PLAYER_VOLUME 15   //громкость звуков плеера по умолчанию(DF - 0..30 | SD - 0..9)

//Настройки будильника
#define ALARM_TYPE 1               //использовать только один будильник(0 - будильник отключен | 1 - один будильник(простое меню будильника) | 2 - неограниченое количество будильников(сложное меню будильников))

#define DEFAULT_ALARM_TIME_HH 8    //час будильника по умолчанию(0..23)
#define DEFAULT_ALARM_TIME_MM 0    //минута будильника по умолчанию(0..59)
#define DEFAULT_ALARM_MODE 0       //режим будильника по умолчанию(0..2)(0 - выкл | 2 - одиночный | 1 - вкл)
#define DEFAULT_ALARM_SOUND 0      //мелодия будильника по умолчанию(0..3)

#define ALARM_TIMEOUT_SOUND 1      //таймаут до автоматического включения ожидания будильника по умолчанию(0.1..240.0)(0 - выкл повторное включение)(м)
#define ALARM_TIMEOUT 15           //таймаут до полного отключения будильника по умолчанию(0.1..240.0)(м)
#define ALARM_WAINT 4              //ожидание будильника для повторного включения по умолчанию(0.1..240.0)(0 - выкл ожидание по короткому нажатию)(м)

#define ALM_ON_BLINK_DOT 3         //режим мигания точек если хоть один будильник включен(0 - выкл | 1 - статичная | 2 - без реакции | 3 - мигает один раз в секунду | 4 - мигает два раза в секунду)
#define ALM_WAINT_BLINK_DOT 4      //режим мигания точек если ожидаем повторного сигнала(0 - выкл | 1 - статичная | 2 - без реакции | 3 - мигает один раз в секунду | 4 - мигает два раза в секунду)
#define ALM_BLINK_TIME 500         //период мигания времени во время активного будильника(100..1000)(мс)

//Настройки таймера
#define TIMER_TIME 60              //значение таймера по умолчанию(1..5999)(с)
#define TIMER_BLINK_TIME 500       //период мигания таймера по окончанию отсчета(100..1000)(мс)
