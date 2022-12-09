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
#define CONV_PIN  9  //пин преобразователя(только пин 9)(pin D)
#define DOT_PIN   10 //пин секундных точек(неоновая точка - 0..13 | светодиодная точка - 10)(pin D)
#define BACKL_PIN 11 //пин подсветки(софтверный шим(обычные светодиоды) или адресные светодиоды - 0..13 | хардверный шим(обычные светодиоды) - 11)(pin D)
#define BUZZ_PIN  13 //пин пищалки(бузер - 0..13 | SD карта - 10)(pin D)

//Дополнительная периферия
#define DOTL_PIN 0  //пин левых(основных) разделительных точек в индикаторах(0..13)(pin D)
#define DOTR_PIN 1  //пин правых(дополнительных) разделительных точек в индикаторах(0..13)(pin D)
#define SENS_PIN 1  //пин сенсора температуры(для DS18xx и DHTxx)(0..13)(pin D)
#define SQW_PIN  2  //пин SQW(только пин 2)(pin D)
#define AMP_PIN  8  //пин управления питанием усилителя(0..13)(pin D)
#define MOV_PIN  8  //пин датчика движения(0..13)(pin D)
#define IR_PIN   7  //пин инфракрасного приемника(0..13)(pin D)

//DF плеер
#define DF_RX_PIN   1  //пин DF плеера RX(софтверный UART - 0..13 | хардверный UART - 1)(pin D)
#define DF_BUSY_PIN 0  //пин DF плеера BUSY(0..13)(pin D)

//SD плеер
#define SD_MISO_PIN 12 //пин SD карты MISO(0..13)(pin D)
#define SD_MOSI_PIN 11 //пин SD карты MOSI(0..13)(pin D)
#define SD_SCK_PIN  13 //пин SD карты SCK(0..13)(pin D)
#define SD_CS_PIN   0  //пин SD карты CS(0..13)(pin D)

//Обратная связь
#define ANALOG_DET_PIN 6 //пин обратной связи(6..7)(pin A)

//Аналоговые кнопки
#define ANALOG_BTN_PIN 7 //пин аналоговых кнопок(6..7)(pin A)

//Сенсор яркости освещения
#define ANALOG_LIGHT_PIN 7 //пин сенсора яркости освещения(6..7)(pin A)


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

#define DECODE_PCMSK(pin) ((pin < 8) ? PCMSK2 : PCMSK0)
#define DECODE_PCIE(pin) ((pin < 8) ? PCIE2 : PCIE0)
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

#define DOT_CLEAR (BIT_CLEAR(DOT_PORT, DOT_BIT))

//Пин левых точек индикаторов
#define INDI_DOTL_BIT   DECODE_BIT(DOTL_PIN)
#define INDI_DOTL_PORT  DECODE_PORT(DOTL_PIN)

#define INDI_DOTL_OFF   (BIT_CLEAR(INDI_DOTL_PORT, INDI_DOTL_BIT))
#define INDI_DOTL_ON    (BIT_SET(INDI_DOTL_PORT, INDI_DOTL_BIT))
#define INDI_DOTL_OUT   (BIT_SET(DDR_REG(INDI_DOTL_PORT), INDI_DOTL_BIT))

#define INDI_DOTL_INIT  INDI_DOTL_OFF; INDI_DOTL_OUT

//Пин правых точек индикаторов
#define INDI_DOTR_BIT   DECODE_BIT(DOTR_PIN)
#define INDI_DOTR_PORT  DECODE_PORT(DOTR_PIN)

#define INDI_DOTR_OFF   (BIT_CLEAR(INDI_DOTR_PORT, INDI_DOTR_BIT))
#define INDI_DOTR_ON    (BIT_SET(INDI_DOTR_PORT, INDI_DOTR_BIT))
#define INDI_DOTR_OUT   (BIT_SET(DDR_REG(INDI_DOTR_PORT), INDI_DOTR_BIT))

#define INDI_DOTR_INIT  INDI_DOTR_OFF; INDI_DOTR_OUT

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

#if (BTN_ADD_TYPE == 1)
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
#define CONV_BIT     DECODE_BIT(CONV_PIN)
#define CONV_PORT    DECODE_PORT(CONV_PIN)

#define CONV_ON      (BIT_SET(CONV_PORT, CONV_BIT))
#define CONV_OFF     (BIT_CLEAR(CONV_PORT, CONV_BIT))
#define CONV_OUT     (BIT_SET(DDR_REG(CONV_PORT), CONV_BIT))
#define CONV_DISABLE TCCR1A = 0; TCCR1B = 0

#define CONV_INIT    CONV_DISABLE; CONV_OFF; CONV_OUT

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
#define BUZZ_INV   (BUZZ_PORT ^= (0x01 << BUZZ_BIT))
#define BUZZ_OUT   (BIT_SET(DDR_REG(BUZZ_PORT), BUZZ_BIT))
#define BUZZ_INP   (BIT_CLEAR(DDR_REG(BUZZ_PORT), BUZZ_BIT))

#define BUZZ_INIT  BUZZ_OFF; BUZZ_OUT

//Пин плеера RX
#define DF_RX_BIT   DECODE_BIT(DF_RX_PIN)
#define DF_RX_PORT  DECODE_PORT(DF_RX_PIN)

#define DF_RX_SET   (BIT_SET(DF_RX_PORT, DF_RX_BIT))
#define DF_RX_CLEAR (BIT_CLEAR(DF_RX_PORT, DF_RX_BIT))
#define DF_RX_OUT   (BIT_SET(DDR_REG(DF_RX_PORT), DF_RX_BIT))

#define DF_RX_INIT  DF_RX_SET; DF_RX_OUT

//Пин плеера BUSY
#define DF_BUSY_BIT   DECODE_BIT(DF_BUSY_PIN)
#define DF_BUSY_PORT  DECODE_PORT(DF_BUSY_PIN)

#define DF_BUSY_CHK   (BIT_READ(PIN_REG(DF_BUSY_PORT), DF_BUSY_BIT))
#define DF_BUSY_SET   (BIT_SET(DF_BUSY_PORT, DF_BUSY_BIT))
#define DF_BUSY_INP   (BIT_CLEAR(DDR_REG(DF_BUSY_PORT), DF_BUSY_BIT))

#define DF_BUSY_INIT  DF_BUSY_SET; DF_BUSY_INP

//Пин MISO карты памяти
#define SD_MISO_BIT   DECODE_BIT(SD_MISO_PIN)
#define SD_MISO_PORT  DECODE_PORT(SD_MISO_PIN)

#define SD_MISO_SET   (BIT_SET(SD_MISO_PORT, SD_MISO_BIT))
#define SD_MISO_CLEAR (BIT_CLEAR(SD_MISO_PORT, SD_MISO_BIT))
#define SD_MISO_INP   (BIT_CLEAR(DDR_REG(SD_MISO_PORT), SD_MISO_BIT))

#define SD_MISO_INIT  SD_MISO_CLEAR; SD_MISO_INP

//Пин MOSI карты памяти
#define SD_MOSI_BIT   DECODE_BIT(SD_MOSI_PIN)
#define SD_MOSI_PORT  DECODE_PORT(SD_MOSI_PIN)

#define SD_MOSI_SET   (BIT_SET(SD_MOSI_PORT, SD_MOSI_BIT))
#define SD_MOSI_CLEAR (BIT_CLEAR(SD_MOSI_PORT, SD_MOSI_BIT))
#define SD_MOSI_OUT   (BIT_SET(DDR_REG(SD_MOSI_PORT), SD_MOSI_BIT))

#define SD_MOSI_INIT  SD_MOSI_SET; SD_MOSI_OUT

//Пин SCK карты памяти
#define SD_SCK_BIT   DECODE_BIT(SD_SCK_PIN)
#define SD_SCK_PORT  DECODE_PORT(SD_SCK_PIN)

#define SD_SCK_INV   (SD_SCK_PORT ^= SD_SCK_BIT)
#define SD_SCK_SET   (BIT_SET(SD_SCK_PORT, SD_SCK_BIT))
#define SD_SCK_CLEAR (BIT_CLEAR(SD_SCK_PORT, SD_SCK_BIT))
#define SD_SCK_OUT   (BIT_SET(DDR_REG(SD_SCK_PORT), SD_SCK_BIT))

#define SD_SCK_INIT  SD_SCK_CLEAR; SD_SCK_OUT

//Пин CS карты памяти
#define SD_CS_BIT     DECODE_BIT(SD_CS_PIN)
#define SD_CS_PORT    DECODE_PORT(SD_CS_PIN)

#define SD_CS_DISABLE (BIT_SET(SD_CS_PORT, SD_CS_BIT))
#define SD_CS_ENABLE  (BIT_CLEAR(SD_CS_PORT, SD_CS_BIT))
#define SD_CS_OUT     (BIT_SET(DDR_REG(SD_CS_PORT), SD_CS_BIT))

#define SD_CS_INIT    SD_CS_DISABLE; SD_CS_OUT

//Пин управления питание усилителя
#define AMP_BIT     DECODE_BIT(AMP_PIN)
#define AMP_PORT    DECODE_PORT(AMP_PIN)

#if AMP_POWER_MODE
#define AMP_DISABLE (BIT_CLEAR(AMP_PORT, AMP_BIT))
#define AMP_ENABLE  (BIT_SET(AMP_PORT, AMP_BIT))
#else
#define AMP_DISABLE (BIT_SET(AMP_PORT, AMP_BIT))
#define AMP_ENABLE  (BIT_CLEAR(AMP_PORT, AMP_BIT))
#endif
#define AMP_CHK     (BIT_READ(AMP_PORT, AMP_BIT))
#define AMP_OUT     (BIT_SET(DDR_REG(AMP_PORT), AMP_BIT))

#define AMP_INIT    AMP_DISABLE; AMP_OUT

//Пин датчика движения
#define MOV_BIT   DECODE_BIT(MOV_PIN)
#define MOV_PORT  DECODE_PORT(MOV_PIN)

#define MOV_CHK   (BIT_READ(PIN_REG(MOV_PORT), MOV_BIT))
#define MOV_SET   (BIT_SET(MOV_PORT, MOV_BIT))
#define MOV_CLEAR (BIT_CLEAR(MOV_PORT, MOV_BIT))
#define MOV_INP   (BIT_CLEAR(DDR_REG(MOV_PORT), MOV_BIT))

#define MOV_INIT  MOV_CLEAR; MOV_INP

//Пин инфракрасного приемника
#define IR_BIT   DECODE_BIT(IR_PIN)
#define IR_PORT  DECODE_PORT(IR_PIN)

#define IR_CHK   (BIT_READ(PIN_REG(IR_PORT), IR_BIT))
#define IR_SET   (BIT_SET(IR_PORT, IR_BIT))
#define IR_CLEAR (BIT_CLEAR(IR_PORT, IR_BIT))
#define IR_INP   (BIT_CLEAR(DDR_REG(IR_PORT), IR_BIT))

#define IR_INIT  IR_SET; IR_INP
