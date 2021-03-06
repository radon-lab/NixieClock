//Соединения периферии с портами МК
//    PORTD (2 - D2 | 3 - D3 | 4 - D4 | 5 - D5 | 6 - D6 | 7 - D7) PIND
//  PORTB (0 - D8 | 1 - D9 | 2 - D10 | 3 - D11 | 4 - D12 | 5 - D13) PINB
//    PORTC (0 - A0 | 1 - A1 | 2 - A2 | 3 - A3 | 4 - A4 | 5 - A5) PINC


#define DDR_REG(portx)  (*(&portx-1))
#define BIT_READ(value, bit) (((value) >> (bit)) & 0x01)
#define BIT_SET(value, bit) ((value) |= (0x01 << (bit)))
#define BIT_CLEAR(value, bit) ((value) &= ~(0x01 << (bit)))
#define BIT_WRITE(value, bit, bitvalue) (bitvalue ? BIT_SET(value, bit) : BIT_CLEAR(value, bit))

//оптопары(только порты D2-D7)
#define ANODE_1 3 //D3
#define ANODE_2 4 //D4
#define ANODE_3 5 //D5
#define ANODE_4 6 //D6

//дешифратор(только порты A0-A5)
#define DECODER_1 A0 //A0
#define DECODER_2 A1 //A1
#define DECODER_3 A2 //A2
#define DECODER_4 A3 //A3

//распиновка ламп
#if (BOARD_TYPE == 0)
const uint8_t digitMask[] = {7, 3, 6, 4, 1, 9, 8, 0, 5, 2, 10};   //маска дешифратора платы in12 (цифры нормальные)(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t anodeMask[] = {ANODE_1, ANODE_2, ANODE_3, ANODE_4}; //порядок и номера пинов анодов индикатора(0, 1, 2, 3)
const uint8_t cathodeMask[] = {1, 6, 2, 7, 5, 0, 4, 9, 8, 3};     //порядок катодов in12
#elif (BOARD_TYPE == 1)
const uint8_t digitMask[] = {2, 8, 1, 9, 6, 4, 3, 5, 0, 7, 10};   //маска дешифратора платы in12 turned (цифры вверх ногами)(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t anodeMask[] = {ANODE_4, ANODE_3, ANODE_2, ANODE_1}; //порядок и номера пинов анодов индикатора(0, 1, 2, 3)
const uint8_t cathodeMask[] = {1, 6, 2, 7, 5, 0, 4, 9, 8, 3};     //порядок катодов in12
#elif (BOARD_TYPE == 2)
const uint8_t digitMask[] = {9, 8, 0, 5, 4, 7, 3, 6, 2, 1, 10};   //маска дешифратора платы in14(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t anodeMask[] = {ANODE_4, ANODE_3, ANODE_2, ANODE_1}; //порядок и номера пинов анодов индикатора(0, 1, 2, 3)
const uint8_t cathodeMask[] = {1, 0, 2, 9, 3, 8, 4, 7, 5, 6};     //порядок катодов in14
#elif (BOARD_TYPE == 3)
const uint8_t digitMask[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 10};   //тут вводим свой порядок пинов лампы(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t anodeMask[] = {ANODE_1, ANODE_2, ANODE_3, ANODE_4}; //порядок и номера пинов анодов индикатора(0, 1, 2, 3)
const uint8_t cathodeMask[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};     //свой порядок катодов
#elif (BOARD_TYPE == 4)
const uint8_t digitMask[] = {9, 8, 0, 5, 2, 7, 3, 6, 4, 1, 10};   //маска дешифратора платы in12(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t anodeMask[] = {ANODE_4, ANODE_3, ANODE_2, ANODE_1}; //порядок и номера пинов анодов индикатора(0, 1, 2, 3)
const uint8_t cathodeMask[] = {1, 6, 2, 7, 5, 0, 4, 9, 8, 3};     //порядок катодов in12
#endif

//пин кнопки ОК D8
#define SET_BIT   0 // D8
#define SET_PORT  PORTB
#define SET_PIN   PINB

#define SET_CHK   (BIT_READ(SET_PIN, SET_BIT))
#define SET_SET   (BIT_SET(SET_PORT, SET_BIT))
#define SET_INP   (BIT_CLEAR((DDR_REG(SET_PORT)), SET_BIT))

#define SET_INIT  SET_SET; SET_INP

//пин кнопки DOWN D7
#define LEFT_BIT   7 // D7
#define LEFT_PORT  PORTD
#define LEFT_PIN   PIND

#define LEFT_CHK   (BIT_READ(LEFT_PIN, LEFT_BIT))
#define LEFT_SET   (BIT_SET(LEFT_PORT, LEFT_BIT))
#define LEFT_INP   (BIT_CLEAR((DDR_REG(LEFT_PORT)), LEFT_BIT))

#define LEFT_INIT  LEFT_SET; LEFT_INP

//пин кнопки UP D12
#define RIGHT_BIT   4 // D12
#define RIGHT_PORT  PORTB
#define RIGHT_PIN   PINB

#define RIGHT_CHK   (BIT_READ(RIGHT_PIN, RIGHT_BIT))
#define RIGHT_SET   (BIT_SET(RIGHT_PORT, RIGHT_BIT))
#define RIGHT_INP   (BIT_CLEAR((DDR_REG(RIGHT_PORT)), RIGHT_BIT))

#define RIGHT_INIT  RIGHT_SET; RIGHT_INP


//пин SQW D2
#define SQW_BIT   2 // D2
#define SQW_PORT  PORTD

#define SQW_SET   (BIT_SET(SQW_PORT, SQW_BIT))
#define SQW_INP   (BIT_CLEAR((DDR_REG(SQW_PORT)), SQW_BIT))

#define SQW_INIT  SQW_SET; SQW_INP

//пин преобразователя D9
#define CONV_BIT   1 // D9
#define CONV_PORT  PORTB

#define CONV_ON    (BIT_SET(CONV_PORT, CONV_BIT))
#define CONV_OFF   (BIT_CLEAR(CONV_PORT, CONV_BIT))
#define CONV_OUT   (BIT_SET((DDR_REG(CONV_PORT)), CONV_BIT))

#define CONV_INIT  CONV_OFF; CONV_OUT

//пин точек D10
#define DOT_BIT   2 // D10
#define DOT_PORT  PORTB

#define DOT_ON     (BIT_SET(DOT_PORT, DOT_BIT))
#define DOT_OFF    (BIT_CLEAR(DOT_PORT, DOT_BIT))
#define DOT_OUT    (BIT_SET((DDR_REG(DOT_PORT)), DOT_BIT))

#define DOT_INIT  DOT_OFF; DOT_OUT

//пин подсветки D11
#define BACKL_BIT   3 // D11
#define BACKL_PORT  PORTB

#define BACKL_ON   (BIT_SET(BACKL_PORT, BACKL_BIT))
#define BACKL_OFF  (BIT_CLEAR(BACKL_PORT, BACKL_BIT))
#define BACKL_OUT  (BIT_SET((DDR_REG(BACKL_PORT)), BACKL_BIT))

#define BACKL_INIT  BACKL_OFF; BACKL_OUT

//пин пищалки D13
#define BUZZ_BIT   5 // D13
#define BUZZ_PORT  PORTB

#define BUZZ_OFF   (BIT_CLEAR(BUZZ_PORT, BUZZ_BIT))
#define BUZZ_INV   (BUZZ_PORT ^= (1 << BUZZ_BIT))
#define BUZZ_OUT   (BIT_SET((DDR_REG(BUZZ_PORT)), BUZZ_BIT))

#define BUZZ_INIT  BUZZ_OFF; BUZZ_OUT
