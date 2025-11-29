//    PORTD (0 - D0 | 1 - D1 | 2 - D2 | 3 - D3 | 4 - D4 | 5 - D5 | 6 - D6 | 7 - D7)
//    PORTB (0 - D8 | 1 - D9 | 2 - D10 | 3 - D11 | 4 - D12 | 5 - D13)
//    PORTC (0 - A0 | 1 - A1 | 2 - A2 | 3 - A3 | 4 - A4 | 5 - A5)
//    PORTE (0 - GND(3) | 1 - VCC(6) | 2 - A6 | 3 - A7)

#define DDR_REG(portx) (*(&portx - 1))
#define PIN_REG(portx) (*(&portx - 2))
#define BIT_READ_INV(value, bit) (((value) ^ (0x01 << (bit))) & (0x01 << (bit)))
#define BIT_READ(value, bit) ((value) & (0x01 << (bit)))
#define BIT_INV(value, bit) ((value) ^= (0x01 << (bit)))
#define BIT_SET(value, bit) ((value) |= (0x01 << (bit)))
#define BIT_CLEAR(value, bit) ((value) &= ~(0x01 << (bit)))
#define BIT_WRITE(value, bit, bitvalue) (bitvalue ? BIT_SET(value, bit) : BIT_CLEAR(value, bit))

#define CONSTRAIN(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#if INDI_MODE != 0
#define DECODE_PCMSK(pin) (((pin) < 8) ? PCMSK2 : (((pin) < 14) ? PCMSK0 : PCMSK1))
#define DECODE_PCIE(pin) (((pin) < 8) ? PCIE2 : (((pin) < 14) ? PCIE0 : PCIE1))

#define DECODE_PORT(pin) (((pin) < 8) ? PORTD : (((pin) < 14) ? PORTB : PORTC))
#define DECODE_BIT(pin) (((pin) < 8) ? pin : (((pin) < 14) ? ((pin) - 8) : ((pin) - 14)))
#else
#define DECODE_PCMSK(pin) (((pin) < 8) ? PCMSK2 : PCMSK0)
#define DECODE_PCIE(pin) (((pin) < 8) ? PCIE2 : PCIE0)

#ifdef PORTE
#define DECODE_PORT(pin) (((pin) < 8) ? PORTD : (((pin) < 14) ? PORTB : PORTE))
#define DECODE_BIT(pin) (((pin) < 8) ? pin : (((pin) < 14) ? ((pin) - 8) : ((pin) - 14)))
#else
#define DECODE_PORT(pin) (((pin) < 8) ? PORTD : PORTB)
#define DECODE_BIT(pin) (((pin) < 8) ? (pin) : ((pin) - 8))
#endif
#endif


//Управление анодами ламп
#define ANODE_CLEAR(pin) (BIT_CLEAR(DECODE_PORT(pin), DECODE_BIT(pin)))
#define ANODE_SET(pin)   (BIT_SET(DECODE_PORT(pin), DECODE_BIT(pin)))
#define ANODE_OUT(pin)   (BIT_SET(DDR_REG(DECODE_PORT(pin)), DECODE_BIT(pin)))

#define ANODE_INIT(pin)  ANODE_CLEAR(pin); ANODE_OUT(pin)

//Управление секундными точками
#define SECS_DOT_CLEAR(pin) (BIT_CLEAR(DECODE_PORT(pin), DECODE_BIT(pin)))
#define SECS_DOT_SET(pin)   (BIT_SET(DECODE_PORT(pin), DECODE_BIT(pin)))
#define SECS_DOT_OUT(pin)   (BIT_SET(DDR_REG(DECODE_PORT(pin)), DECODE_BIT(pin)))

#define SECS_DOT_INIT(pin)  SECS_DOT_CLEAR(pin); SECS_DOT_OUT(pin)

//Управление точками в индикаторах
#define INDI_DOT_CLEAR(pin) (BIT_CLEAR(DECODE_PORT(pin), DECODE_BIT(pin)))
#define INDI_DOT_SET(pin)   (BIT_SET(DECODE_PORT(pin), DECODE_BIT(pin)))
#define INDI_DOT_OUT(pin)   (BIT_SET(DDR_REG(DECODE_PORT(pin)), DECODE_BIT(pin)))

#define INDI_DOT_INIT(pin)  INDI_DOT_CLEAR(pin); INDI_DOT_OUT(pin)

//Пин DATA сдвигового регистра
#define REG_DATA_BIT   DECODE_BIT(REG_DATA_PIN)
#define REG_DATA_PORT  DECODE_PORT(REG_DATA_PIN)

#define REG_DATA_SET   (BIT_SET(REG_DATA_PORT, REG_DATA_BIT))
#define REG_DATA_CLEAR (BIT_CLEAR(REG_DATA_PORT, REG_DATA_BIT))
#define REG_DATA_OUT   (BIT_SET(DDR_REG(REG_DATA_PORT), REG_DATA_BIT))

#define REG_DATA_INIT  REG_DATA_CLEAR; REG_DATA_OUT

//Пин SCK сдвигового регистра
#define REG_SCK_BIT   DECODE_BIT(REG_SCK_PIN)
#define REG_SCK_PORT  DECODE_PORT(REG_SCK_PIN)

#define REG_SCK_INV   (BIT_INV(REG_SCK_PORT, REG_SCK_BIT))
#define REG_SCK_SET   (BIT_SET(REG_SCK_PORT, REG_SCK_BIT))
#define REG_SCK_CLEAR (BIT_CLEAR(REG_SCK_PORT, REG_SCK_BIT))
#define REG_SCK_OUT   (BIT_SET(DDR_REG(REG_SCK_PORT), REG_SCK_BIT))

#define REG_SCK_INIT  REG_SCK_CLEAR; REG_SCK_OUT

//Пин LATCH сдвигового регистра
#define REG_LATCH_BIT     DECODE_BIT(REG_LATCH_PIN)
#define REG_LATCH_PORT    DECODE_PORT(REG_LATCH_PIN)

#define REG_LATCH_DISABLE (BIT_SET(REG_LATCH_PORT, REG_LATCH_BIT))
#define REG_LATCH_ENABLE  (BIT_CLEAR(REG_LATCH_PORT, REG_LATCH_BIT))
#define REG_LATCH_OUT     (BIT_SET(DDR_REG(REG_LATCH_PORT), REG_LATCH_BIT))

#define REG_LATCH_INIT    REG_LATCH_ENABLE; REG_LATCH_OUT


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
#define SET_CHK   (BIT_READ_INV(PIN_REG(SET_PORT), SET_BIT))
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
#define LEFT_CHK   (BIT_READ_INV(PIN_REG(LEFT_PORT), LEFT_BIT))
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
#define RIGHT_CHK   (BIT_READ_INV(PIN_REG(RIGHT_PORT), RIGHT_BIT))
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
#define ADD_CHK   (BIT_READ_INV(PIN_REG(ADD_PORT), ADD_BIT))
#define ADD_CLR   (BIT_CLEAR(ADD_PORT, ADD_BIT))
#define ADD_INP   (BIT_CLEAR(DDR_REG(ADD_PORT), ADD_BIT))

#define ADD_INIT  ADD_CLR; ADD_INP
#endif
#endif


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

//Пин обратной связи на компараторе
#define FB_BIT   DECODE_BIT(ANALOG_DET_PIN)
#define FB_PORT  DECODE_PORT(ANALOG_DET_PIN)

#define FB_CLEAR (BIT_CLEAR(FB_PORT, FB_BIT))
#define FB_INP   (BIT_CLEAR(DDR_REG(FB_PORT), FB_BIT))

#define FB_INIT  FB_CLEAR; FB_INP


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
#define BUZZ_INV   (BIT_INV(BUZZ_PORT, BUZZ_BIT))
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

#define SD_SCK_INV   (BIT_INV(SD_SCK_PORT, SD_SCK_BIT))
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
#define AMP_CHK     (BIT_READ(AMP_PORT, AMP_BIT))
#else
#define AMP_DISABLE (BIT_SET(AMP_PORT, AMP_BIT))
#define AMP_ENABLE  (BIT_CLEAR(AMP_PORT, AMP_BIT))
#define AMP_CHK     (BIT_READ_INV(AMP_PORT, AMP_BIT))
#endif
#define AMP_OUT     (BIT_SET(DDR_REG(AMP_PORT), AMP_BIT))

#define AMP_INIT    AMP_DISABLE; AMP_OUT


//Пин сенсора температуры(для DS18x20 и DHTxx)
#define SENS_BIT    DECODE_BIT(SENS_PIN)
#define SENS_PORT   DECODE_PORT(SENS_PIN)

#define SENS_SET   (BIT_SET(SENS_PORT, SENS_BIT))
#define SENS_CLEAR (BIT_CLEAR(SENS_PORT, SENS_BIT))
#define SENS_CHK   (BIT_READ(PIN_REG(SENS_PORT), SENS_BIT))
#define SENS_LO    (BIT_SET(DDR_REG(SENS_PORT), SENS_BIT))
#define SENS_HI    (BIT_CLEAR(DDR_REG(SENS_PORT), SENS_BIT))

#define SENS_INIT  SENS_CLEAR; SENS_HI


//Пин датчика движения
#define MOV_BIT   DECODE_BIT(MOV_PIN)
#define MOV_PORT  DECODE_PORT(MOV_PIN)

#if MOV_PORT_PULL
#define MOV_CHK   (BIT_READ_INV(PIN_REG(MOV_PORT), MOV_BIT))
#define MOV_SET   (BIT_SET(MOV_PORT, MOV_BIT))
#define MOV_INP   (BIT_CLEAR(DDR_REG(MOV_PORT), MOV_BIT))

#define MOV_INIT  MOV_SET; MOV_INP
#else
#define MOV_CHK   (BIT_READ(PIN_REG(MOV_PORT), MOV_BIT))
#define MOV_CLEAR (BIT_CLEAR(MOV_PORT, MOV_BIT))
#define MOV_INP   (BIT_CLEAR(DDR_REG(MOV_PORT), MOV_BIT))

#define MOV_INIT  MOV_CLEAR; MOV_INP
#endif


//Пин инфракрасного приемника
#define IR_BIT   DECODE_BIT(IR_PIN)
#define IR_PORT  DECODE_PORT(IR_PIN)

#define IR_CHK   (BIT_READ(PIN_REG(IR_PORT), IR_BIT))
#define IR_SET   (BIT_SET(IR_PORT, IR_BIT))
#define IR_CLEAR (BIT_CLEAR(IR_PORT, IR_BIT))
#define IR_INP   (BIT_CLEAR(DDR_REG(IR_PORT), IR_BIT))

#define IR_INIT  IR_SET; IR_INP
