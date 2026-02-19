//Конфигурации плат часов

//------------------------[AlexGyver v1] ИН12 (только 4x ИН12)------------------------//
#if (BOARD_TYPE == 1)
#undef DIGIT_MASK
#define DIGIT_MASK ID(0), ID(1), ID(2), ID(3), ID(4), ID(5), ID(6), ID(7), ID(8), ID(9), ID(10) //маска дешифратора
#undef CATHODE_MASK
#define CATHODE_MASK 1, 6, 2, 7, 5, 0, 4, 9, 8, 3 //порядок катодов ИН12

#undef DECODER_1
#define DECODER_1 0 //пин дешифратора X1(A)(0..3)(pin A)
#undef DECODER_2
#define DECODER_2 2 //пин дешифратора X2(B)(0..3)(pin A)
#undef DECODER_3
#define DECODER_3 3 //пин дешифратора X4(C)(0..3)(pin A)
#undef DECODER_4
#define DECODER_4 1 //пин дешифратора X8(D)(0..3)(pin A)

#undef ANODE_1_PIN
#define ANODE_1_PIN 8  //пин анода 1(ЧЧ)(0..13)(pin D)
#undef ANODE_2_PIN
#define ANODE_2_PIN 7  //пин анода 2(ЧЧ)(0..13)(pin D)
#undef ANODE_3_PIN
#define ANODE_3_PIN 6  //пин анода 3(ММ)(0..13)(pin D)
#undef ANODE_4_PIN
#define ANODE_4_PIN 5  //пин анода 4(ММ)(0..13)(pin D)
#undef ANODE_5_PIN
#define ANODE_5_PIN 9  //пин анода 5(СС)(0..13)(pin D)
#undef ANODE_6_PIN
#define ANODE_6_PIN 10 //пин анода 6(СС)(0..13)(pin D)

#undef SECL_PIN
#define SECL_PIN 4 //пин левой секундной точки(основной)(неоновая точка - 0..13 | светодиодная точка - 9..10)(pin D)
#undef BUZZ_PIN
#define BUZZ_PIN 3 //пин пищалки(бузер - 0..13 | SD карта - 9..10)(pin D)

#undef SECS_DOT
#define SECS_DOT 1 //тип основных секундных точек(0 - точка светодиод | 1 - точка неоновая лампа(1 шт) | 2 - точка неоновая лампа(2 шт) | 3 - точка в индикаторе | 4 - точка в декатроне)
#undef LAMP_NUM
#define LAMP_NUM 4 //количество используемых ламп в часах(4 или 6)

#undef GEN_ENABLE
#define GEN_ENABLE 0 //включить генерацию шим для преобразователя высокого напряжения(0 - внешний шим преобразователь | 1 - шим преобразователь от мк часов)

#undef BTN_TYPE
#define BTN_TYPE 1 //тип основных кнопок(0 - используются цифровые кнопки | 1 - используются аналоговые кнопки)
#undef BTN_PULL
#define BTN_PULL 0 //тип подтяжки основных кнопок(0 - кнопки подтянуты к земле (LOW_PULL) | 1 - кнопки подтянуты к питанию (HIGH_PULL))

#undef BTN_R_LOW
#define BTN_R_LOW 10.0       //сопротивление резистора нижнего плеча клавиш(кОм)
#undef BTN_SET_R_HIGH
#define BTN_SET_R_HIGH 3.0   //сопротивление резистора клавиши "ОК"(полное сопротивление от кнопки до пина)(кОм)
#undef BTN_LEFT_R_HIGH
#define BTN_LEFT_R_HIGH 66.0 //сопротивление резистора "ЛЕВОЙ" клавиши(полное сопротивление от кнопки до пина)(кОм)
#undef BTN_RIGHT_R_HIGH
#define BTN_RIGHT_R_HIGH 0   //сопротивление резистора "ПРАВОЙ" клавиши(полное сопротивление от кнопки до пина)(кОм)

//-----------------[AlexGyver v1] ИН14 (6x ИН14 или 4x ИН14 + 2x ИН16)----------------//
#elif (BOARD_TYPE == 2)
#undef DIGIT_MASK
#define DIGIT_MASK ID(0), ID(1), ID(2), ID(3), ID(4), ID(5), ID(6), ID(7), ID(8), ID(9), ID(10) //маска дешифратора
#undef CATHODE_MASK
#define CATHODE_MASK 1, 0, 2, 9, 3, 8, 4, 7, 5, 6 //порядок катодов ИН14

#undef DECODER_1
#define DECODER_1 0 //пин дешифратора X1(A)(0..3)(pin A)
#undef DECODER_2
#define DECODER_2 2 //пин дешифратора X2(B)(0..3)(pin A)
#undef DECODER_3
#define DECODER_3 3 //пин дешифратора X4(C)(0..3)(pin A)
#undef DECODER_4
#define DECODER_4 1 //пин дешифратора X8(D)(0..3)(pin A)

#undef ANODE_1_PIN
#define ANODE_1_PIN 10 //пин анода 1(ЧЧ)(0..13)(pin D)
#undef ANODE_2_PIN
#define ANODE_2_PIN 9  //пин анода 2(ЧЧ)(0..13)(pin D)
#undef ANODE_3_PIN
#define ANODE_3_PIN 5  //пин анода 3(ММ)(0..13)(pin D)
#undef ANODE_4_PIN
#define ANODE_4_PIN 6  //пин анода 4(ММ)(0..13)(pin D)
#undef ANODE_5_PIN
#define ANODE_5_PIN 7  //пин анода 5(СС)(0..13)(pin D)
#undef ANODE_6_PIN
#define ANODE_6_PIN 8  //пин анода 6(СС)(0..13)(pin D)

#undef SECL_PIN
#define SECL_PIN 4 //пин левой секундной точки(основной)(неоновая точка - 0..13 | светодиодная точка - 9..10)(pin D)
#undef BUZZ_PIN
#define BUZZ_PIN 3 //пин пищалки(бузер - 0..13 | SD карта - 9..10)(pin D)

#undef SECS_DOT
#define SECS_DOT 1 //тип основных секундных точек(0 - точка светодиод | 1 - точка неоновая лампа(1 шт) | 2 - точка неоновая лампа(2 шт) | 3 - точка в индикаторе | 4 - точка в декатроне)
#undef LAMP_NUM
#define LAMP_NUM 6 //количество используемых ламп в часах(4 или 6)

#undef GEN_ENABLE
#define GEN_ENABLE 0 //включить генерацию шим для преобразователя высокого напряжения(0 - внешний шим преобразователь | 1 - шим преобразователь от мк часов)

#undef BTN_TYPE
#define BTN_TYPE 1 //тип основных кнопок(0 - используются цифровые кнопки | 1 - используются аналоговые кнопки)
#undef BTN_PULL
#define BTN_PULL 0 //тип подтяжки основных кнопок(0 - кнопки подтянуты к земле (LOW_PULL) | 1 - кнопки подтянуты к питанию (HIGH_PULL))

#undef BTN_R_LOW
#define BTN_R_LOW 10.0       //сопротивление резистора нижнего плеча клавиш(кОм)
#undef BTN_SET_R_HIGH
#define BTN_SET_R_HIGH 3.0   //сопротивление резистора клавиши "ОК"(полное сопротивление от кнопки до пина)(кОм)
#undef BTN_LEFT_R_HIGH
#define BTN_LEFT_R_HIGH 66.0 //сопротивление резистора "ЛЕВОЙ" клавиши(полное сопротивление от кнопки до пина)(кОм)
#undef BTN_RIGHT_R_HIGH
#define BTN_RIGHT_R_HIGH 0   //сопротивление резистора "ПРАВОЙ" клавиши(полное сопротивление от кнопки до пина)(кОм)

//------------------[AlexGyver v2] ИН12 (индикаторы стоят правильно)------------------//
#elif (BOARD_TYPE == 3)
#undef DIGIT_MASK
#define DIGIT_MASK ID(7), ID(3), ID(6), ID(4), ID(1), ID(9), ID(8), ID(0), ID(5), ID(2), ID(10) //маска дешифратора
#undef CATHODE_MASK
#define CATHODE_MASK 1, 6, 2, 7, 5, 0, 4, 9, 8, 3 //порядок катодов ИН12

//------------------[AlexGyver v2] ИН12 turned (индикаторы перевёрнуты)------------------//
#elif (BOARD_TYPE == 4)
#undef DIGIT_MASK
#define DIGIT_MASK ID(2), ID(8), ID(1), ID(9), ID(6), ID(4), ID(3), ID(5), ID(0), ID(7), ID(10) //маска дешифратора
#undef CATHODE_MASK
#define CATHODE_MASK 1, 6, 2, 7, 5, 0, 4, 9, 8, 3 //порядок катодов ИН12

#undef ANODE_1_PIN
#define ANODE_1_PIN 6 //пин анода 1(ЧЧ)(0..13)(pin D)
#undef ANODE_2_PIN
#define ANODE_2_PIN 5 //пин анода 2(ЧЧ)(0..13)(pin D)
#undef ANODE_3_PIN
#define ANODE_3_PIN 4 //пин анода 3(ММ)(0..13)(pin D)
#undef ANODE_4_PIN
#define ANODE_4_PIN 3 //пин анода 4(ММ)(0..13)(pin D)

//------------------[AlexGyver v2] ИН14 (обычная и неоновая точка)------------------//
#elif (BOARD_TYPE == 5)
#undef DIGIT_MASK
#define DIGIT_MASK ID(9), ID(8), ID(0), ID(5), ID(4), ID(7), ID(3), ID(6), ID(2), ID(1), ID(10) //маска дешифратора
#undef CATHODE_MASK
#define CATHODE_MASK 1, 0, 2, 9, 3, 8, 4, 7, 5, 6 //порядок катодов ИН14

#undef ANODE_1_PIN
#define ANODE_1_PIN 6 //пин анода 1(ЧЧ)(0..13)(pin D)
#undef ANODE_2_PIN
#define ANODE_2_PIN 5 //пин анода 2(ЧЧ)(0..13)(pin D)
#undef ANODE_3_PIN
#define ANODE_3_PIN 4 //пин анода 3(ММ)(0..13)(pin D)
#undef ANODE_4_PIN
#define ANODE_4_PIN 3 //пин анода 4(ММ)(0..13)(pin D)

#else

//------------------Для кастомных пресетов прошивки------------------//

#endif
