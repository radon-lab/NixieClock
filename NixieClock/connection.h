//Соединения периферии с пинами МК

//Дешифратор ламп
#define DECODER_1 3 //пин дешефратора X1(0..3)(pin A)
#define DECODER_2 1 //пин дешефратора X2(0..3)(pin A)
#define DECODER_3 0 //пин дешефратора X4(0..3)(pin A)
#define DECODER_4 2 //пин дешефратора X8(0..3)(pin A)

//Аноды ламп
#define ANODE_1_PIN 3  //пин анода 1(ЧЧ)(0..13)(pin D)
#define ANODE_2_PIN 4  //пин анода 2(ЧЧ)(0..13)(pin D)
#define ANODE_3_PIN 5  //пин анода 3(ММ)(0..13)(pin D)
#define ANODE_4_PIN 6  //пин анода 4(ММ)(0..13)(pin D)
#define ANODE_5_PIN 7  //пин анода 5(СС)(0..13)(pin D)
#define ANODE_6_PIN 8  //пин анода 6(СС)(0..13)(pin D)

//Цифровые кнопки
#define SET_PIN   8  //пин кнопки ОК(0..13)(pin D)
#define LEFT_PIN  7  //пин левой кнопки(0..13)(pin D)
#define RIGHT_PIN 12 //пин правой кнопки(0..13)(pin D)
#define ADD_PIN   0  //пин доплнительной кнопки(0..13)(pin D)

//Основная периферия
#define SQW_PIN   2  //пин SQW(только пин 2)(pin D)
#define CONV_PIN  9  //пин преобразователя(только пин 9)(pin D)
#define SENS_PIN  1  //пин сенсора температуры(для DS18xx и DHTxx)(0..13)(pin D)
#define DOTS_PIN  1  //пин разделительных точек в индикаторах(0..13)(pin D)
#define DOT_PIN   10 //пин секундных точек(неоновая точка - 0..13 | светодиодная точка - 10)(pin D)
#define BACKL_PIN 11 //пин подсветки(софтверный шим(обычные диоды) или адресные диоды - 0..13 | хардверный шим(обычные диоды) - 11)(pin D)
#define BUZZ_PIN  13 //пин пищалки(0..13)(pin D)

//Обратная связь
#define ANALOG_DET_PIN 6 //пин обратной связи(6..7)(pin A)

//Аналоговые кнопки
#define ANALOG_BTN_PIN 7 //пин аналоговых кнопок(6..7)(pin A)


//Соединения периферии с портами МК
//    PORTD (0 - D0 | 1 - D1 | 2 - D2 | 3 - D3 | 4 - D4 | 5 - D5 | 6 - D6 | 7 - D7)
//    PORTB (0 - D8 | 1 - D9 | 2 - D10 | 3 - D11 | 4 - D12 | 5 - D13)
//    PORTC (0 - A0 | 1 - A1 | 2 - A2 | 3 - A3 | 4 - A4 | 5 - A5)

#define DDR_REG(portx)  (*(&portx - 1))
#define PIN_REG(portx)  (*(&portx - 2))
#define BIT_READ(value, bit) (((value) >> (bit)) & 0x01)
#define BIT_SET(value, bit) ((value) |= (0x01 << (bit)))
#define BIT_CLEAR(value, bit) ((value) &= ~(0x01 << (bit)))
#define BIT_WRITE(value, bit, bitvalue) (bitvalue ? BIT_SET(value, bit) : BIT_CLEAR(value, bit))

#define DECODE_PORT(pin) ((pin < 8) ? PORTD : PORTB)
#define DECODE_BIT(pin) ((pin < 8) ? pin : (pin - 8))

#define ANODE_OFF 0x00 //выключенный анод

//Оптопары(аноды ламп)
#define ANODE_1_BIT DECODE_BIT(ANODE_1_PIN) //(ЧЧ)
#define ANODE_1_PORT DECODE_PORT(ANODE_1_PIN)

#define ANODE_2_BIT DECODE_BIT(ANODE_2_PIN) //(ЧЧ)
#define ANODE_2_PORT DECODE_PORT(ANODE_2_PIN)

#define ANODE_3_BIT DECODE_BIT(ANODE_3_PIN) //(ММ)
#define ANODE_3_PORT DECODE_PORT(ANODE_3_PIN)

#define ANODE_4_BIT DECODE_BIT(ANODE_4_PIN) //(ММ)
#define ANODE_4_PORT DECODE_PORT(ANODE_4_PIN)

#define ANODE_5_BIT DECODE_BIT(ANODE_5_PIN) //(СС)
#define ANODE_5_PORT DECODE_PORT(ANODE_5_PIN)

#define ANODE_6_BIT DECODE_BIT(ANODE_6_PIN) //(СС)
#define ANODE_6_PORT DECODE_PORT(ANODE_6_PIN)

//Пин точек
#define DOT_BIT   DECODE_BIT(DOT_PIN)
#define DOT_PORT  DECODE_PORT(DOT_PIN)

//Пин точек индикаторов
#define INDI_DOTS_BIT   DECODE_BIT(DOTS_PIN)
#define INDI_DOTS_PORT  DECODE_PORT(DOTS_PIN)

#define INDI_DOTS_OFF   (BIT_CLEAR(INDI_DOTS_PORT, INDI_DOTS_BIT))
#define INDI_DOTS_ON    (BIT_SET(INDI_DOTS_PORT, INDI_DOTS_BIT))
#define INDI_DOTS_OUT   (BIT_SET(DDR_REG(INDI_DOTS_PORT), INDI_DOTS_BIT))

#define INDI_DOTS_INIT  INDI_DOTS_OFF; INDI_DOTS_OUT

#if !BTN_TYPE
//Пин кнопки ОК
#define SET_BIT   DECODE_BIT(SET_PIN)
#define SET_PORT  DECODE_PORT(SET_PIN)

#if BTN_PULL
#define SET_CHK   (BIT_READ(PIN_REG(SET_PORT), SET_BIT))
#define SET_SET   (BIT_SET(SET_PORT, SET_BIT))
#define SET_INP   (BIT_CLEAR(DDR_REG(SET_PORT), SET_BIT))

#define SET_INIT  SET_SET; SET_INP
#else
#define SET_CHK   (BIT_READ(PIN_REG(SET_PORT), SET_BIT) ^ 0x01)
#define SET_CLR   (BIT_CLEAR(SET_PORT, SET_BIT))
#define SET_INP   (BIT_CLEAR(DDR_REG(SET_PORT), SET_BIT))

#define SET_INIT  SET_CLR; SET_INP
#endif

//Пин кнопки DOWN
#define LEFT_BIT   DECODE_BIT(LEFT_PIN)
#define LEFT_PORT  DECODE_PORT(LEFT_PIN)

#if BTN_PULL
#define LEFT_CHK   (BIT_READ(PIN_REG(LEFT_PORT), LEFT_BIT))
#define LEFT_SET   (BIT_SET(LEFT_PORT, LEFT_BIT))
#define LEFT_INP   (BIT_CLEAR(DDR_REG(LEFT_PORT), LEFT_BIT))

#define LEFT_INIT  LEFT_SET; LEFT_INP
#else
#define LEFT_CHK   (BIT_READ(PIN_REG(LEFT_PORT), LEFT_BIT) ^ 0x01)
#define LEFT_CLR   (BIT_CLEAR(LEFT_PORT, LEFT_BIT))
#define LEFT_INP   (BIT_CLEAR(DDR_REG(LEFT_PORT), LEFT_BIT))

#define LEFT_INIT  LEFT_CLR; LEFT_INP
#endif

//Пин кнопки UP
#define RIGHT_BIT   DECODE_BIT(RIGHT_PIN)
#define RIGHT_PORT  DECODE_PORT(RIGHT_PIN)

#if BTN_PULL
#define RIGHT_CHK   (BIT_READ(PIN_REG(RIGHT_PORT), RIGHT_BIT))
#define RIGHT_SET   (BIT_SET(RIGHT_PORT, RIGHT_BIT))
#define RIGHT_INP   (BIT_CLEAR(DDR_REG(RIGHT_PORT), RIGHT_BIT))

#define RIGHT_INIT  RIGHT_SET; RIGHT_INP
#else
#define RIGHT_CHK   (BIT_READ(PIN_REG(RIGHT_PORT), RIGHT_BIT) ^ 0x01)
#define RIGHT_CLR   (BIT_CLEAR(RIGHT_PORT, RIGHT_BIT))
#define RIGHT_INP   (BIT_CLEAR(DDR_REG(RIGHT_PORT), RIGHT_BIT))

#define RIGHT_INIT  RIGHT_CLR; RIGHT_INP
#endif
#endif

//Пин дополнительной кнопки
#define ADD_BIT   DECODE_BIT(ADD_PIN)
#define ADD_PORT  DECODE_PORT(ADD_PIN)

#if BTN_ADD_PULL
#define ADD_CHK   (BIT_READ(PIN_REG(ADD_PORT), ADD_BIT))
#define ADD_SET   (BIT_SET(ADD_PORT, ADD_BIT))
#define ADD_INP   (BIT_CLEAR(DDR_REG(ADD_PORT), ADD_BIT))

#define ADD_INIT  ADD_SET; ADD_INP
#else
#define ADD_CHK   (BIT_READ(PIN_REG(ADD_PORT), ADD_BIT) ^ 0x01)
#define ADD_CLR   (BIT_CLEAR(ADD_PORT, ADD_BIT))
#define ADD_INP   (BIT_CLEAR(DDR_REG(ADD_PORT), ADD_BIT))

#define ADD_INIT  ADD_CLR; ADD_INP
#endif

//Пин сенсора температуры(для DS18x20 и DHTxx)
#define SENS_BIT    DECODE_BIT(SENS_PIN)
#define SENS_PORT   DECODE_PORT(SENS_PIN)

#define SENS_SET   (BIT_SET(SENS_PORT, SENS_BIT))
#define SENS_CLEAR (BIT_CLEAR(SENS_PORT, SENS_BIT))
#define SENS_CHK   (BIT_READ(PIN_REG(SENS_PORT), SENS_BIT))
#define SENS_LO    (BIT_SET(DDR_REG(SENS_PORT), SENS_BIT))
#define SENS_HI    (BIT_CLEAR(DDR_REG(SENS_PORT), SENS_BIT))

#define SENS_INIT  SENS_CLEAR; SENS_HI

//Пин синхронизации SQW
#define SQW_BIT   DECODE_BIT(SQW_PIN)
#define SQW_PORT  DECODE_PORT(SQW_PIN)

#define SQW_SET   (BIT_SET(SQW_PORT, SQW_BIT))
#define SQW_INP   (BIT_CLEAR(DDR_REG(SQW_PORT), SQW_BIT))

#define SQW_INIT  SQW_SET; SQW_INP

//Пин преобразователя
#define CONV_BIT   DECODE_BIT(CONV_PIN)
#define CONV_PORT  DECODE_PORT(CONV_PIN)

#define CONV_ON    (BIT_SET(CONV_PORT, CONV_BIT))
#define CONV_OFF   (BIT_CLEAR(CONV_PORT, CONV_BIT))
#define CONV_OUT   (BIT_SET(DDR_REG(CONV_PORT), CONV_BIT))

#define CONV_INIT  CONV_OFF; CONV_OUT

//Пин подсветки
#define BACKL_BIT   DECODE_BIT(BACKL_PIN)
#define BACKL_PORT  DECODE_PORT(BACKL_PIN)

#define BACKL_SET   (BIT_SET(BACKL_PORT, BACKL_BIT))
#define BACKL_CLEAR (BIT_CLEAR(BACKL_PORT, BACKL_BIT))
#define BACKL_OUT   (BIT_SET(DDR_REG(BACKL_PORT), BACKL_BIT))

#define BACKL_INIT  BACKL_CLEAR; BACKL_OUT

//Пин пищалки
#define BUZZ_BIT   DECODE_BIT(BUZZ_PIN)
#define BUZZ_PORT  DECODE_PORT(BUZZ_PIN)

#define BUZZ_OFF   (BIT_CLEAR(BUZZ_PORT, BUZZ_BIT))
#define BUZZ_INV   (BUZZ_PORT ^= (1 << BUZZ_BIT))
#define BUZZ_OUT   (BIT_SET(DDR_REG(BUZZ_PORT), BUZZ_BIT))

#define BUZZ_INIT  BUZZ_OFF; BUZZ_OUT
