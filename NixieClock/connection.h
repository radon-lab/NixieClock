//Соединения периферии с пинами МК

//Дешифратор ламп
#define DECODER_1 3 //пин дешифратора X1(A)(0..3)(pin A)
#define DECODER_2 1 //пин дешифратора X2(B)(0..3)(pin A)
#define DECODER_3 0 //пин дешифратора X4(C)(0..3)(pin A)
#define DECODER_4 2 //пин дешифратора X8(D)(0..3)(pin A)

//Аноды ламп
#define ANODE_0_PIN 2 //пин анода 0(СИМВ)(0..13)(pin D)

#define ANODE_1_PIN 3 //пин анода 1(ЧЧ)(0..13)(pin D)
#define ANODE_2_PIN 4 //пин анода 2(ЧЧ)(0..13)(pin D)
#define ANODE_3_PIN 5 //пин анода 3(ММ)(0..13)(pin D)
#define ANODE_4_PIN 6 //пин анода 4(ММ)(0..13)(pin D)
#define ANODE_5_PIN 7 //пин анода 5(СС)(0..13)(pin D)
#define ANODE_6_PIN 8 //пин анода 6(СС)(0..13)(pin D)

//Цифровые кнопки
#define SET_PIN   8  //пин кнопки ОК(0..13)(pin D)
#define LEFT_PIN  7  //пин левой кнопки(0..13)(pin D)
#define RIGHT_PIN 12 //пин правой кнопки(0..13)(pin D)
#define ADD_PIN   0  //пин доплнительной кнопки(0..13)(pin D)

//Основная периферия
#define CONV_PIN  9  //пин преобразователя(9..10)(pin D)
#define BUZZ_PIN  13 //пин пищалки(бузер - 0..13 | SD карта - 9..10)(pin D)
#define SECL_PIN  10 //пин левой секундной точки(основной)(неоновая точка - 0..13 | светодиодная точка - 9..10)(pin D)
#define SECR_PIN  11 //пин правой секундной точки(дополнительной)(0..13)(pin D)
#define BACKL_PIN 11 //пин подсветки(софтверный шим(обычные светодиоды) или светодиоды WS2812B - 0..13 | хардверный шим(обычные светодиоды) - 11)(pin D)

//Дополнительная периферия
#define DOTSL_PIN 0 //пин левых(основных) разделительных точек в индикаторах(0..13)(pin D)
#define DOTSR_PIN 1 //пин правых(дополнительных) разделительных точек в индикаторах(0..13)(pin D)
#define SENS_PIN  1 //пин сенсора температуры(для DS18xx и DHTxx)(0..13)(pin D)
#define SQW_PIN   2 //пин SQW(только пин 2)(pin D)
#define AMP_PIN   8 //пин управления питанием усилителя(0..13)(pin D)
#define MOV_PIN   8 //пин датчика движения(0..13)(pin D)
#define IR_PIN    7 //пин инфракрасного приемника(0..13)(pin D)

//DF плеер
#define DF_RX_PIN   1 //пин DF плеера RX(софтверный UART - 0..13 | хардверный UART - 1)(pin D)
#define DF_BUSY_PIN 0 //пин DF плеера BUSY(0..13)(pin D)

//SD плеер
#define SD_MISO_PIN 12 //пин SD карты MISO(0..13)(pin D)
#define SD_MOSI_PIN 11 //пин SD карты MOSI(0..13)(pin D)
#define SD_SCK_PIN  13 //пин SD карты SCK(0..13)(pin D)
#define SD_CS_PIN   0  //пин SD карты CS(0..13)(pin D)

//Сдвиговый регистр
#define REG_LATCH_PIN 0 //пин сдвигового регистра LATCH(кроме 12)(0..13)(pin D)
#define REG_DATA_PIN 11 //пин сдвигового регистра DATA(только 11)(pin D)
#define REG_SCK_PIN  13 //пин сдвигового регистра SCK(только 13)(pin D)

//Обратная связь
#define ANALOG_DET_PIN 6 //пин обратной связи(для АЦП - (6..7)(pin A))(для компаратора - (только пин 7)(pin D))

//Аналоговые кнопки
#define ANALOG_BTN_PIN 7 //пин аналоговых кнопок(6..7)(pin A)

//Сенсор яркости освещения
#define ANALOG_LIGHT_PIN 7 //пин сенсора яркости освещения(6..7)(pin A)
