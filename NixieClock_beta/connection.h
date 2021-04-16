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
const byte digitMask[] = {7, 3, 6, 4, 1, 9, 8, 0, 5, 2, 10};   //маска дешифратора платы in12 (цифры нормальные)(цифра "10" - это пустой символ, должен быть всешда в конце)
const byte anodeMask[] = {ANODE_1, ANODE_2, ANODE_3, ANODE_4}; //порядок и номера пинов анодов индикатора(0, 1, 2, 3)
#elif (BOARD_TYPE == 1)
const byte digitMask[] = {2, 8, 1, 9, 6, 4, 3, 5, 0, 7, 10};   //маска дешифратора платы in12 turned (цифры вверх ногами)(цифра "10" - это пустой символ, должен быть всешда в конце)
const byte anodeMask[] = {ANODE_4, ANODE_3, ANODE_2, ANODE_1}; //порядок и номера пинов анодов индикатора(0, 1, 2, 3)
#elif (BOARD_TYPE == 2)
const byte digitMask[] = {9, 8, 0, 5, 4, 7, 3, 6, 2, 1, 10};   //маска дешифратора платы in14(цифра "10" - это пустой символ, должен быть всешда в конце)
const byte anodeMask[] = {ANODE_4, ANODE_3, ANODE_2, ANODE_1}; //порядок и номера пинов анодов индикатора(0, 1, 2, 3)
#elif (BOARD_TYPE == 3)
const byte digitMask[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 10};   //тут вводим свой порядок пинов лампы(цифра "10" - это пустой символ, должен быть всешда в конце)
const byte anodeMask[] = {ANODE_1, ANODE_2, ANODE_3, ANODE_4}; //порядок и номера пинов анодов индикатора(0, 1, 2, 3)
#elif (BOARD_TYPE == 4)
const byte digitMask[] = {9, 8, 0, 5, 2, 7, 3, 6, 4, 1, 10};   //маска дешифратора платы in12(цифра "10" - это пустой символ, должен быть всешда в конце)
const byte anodeMask[] = {ANODE_4, ANODE_3, ANODE_2, ANODE_1}; //порядок и номера пинов анодов индикатора(0, 1, 2, 3)
#endif

//назначаем кнопки//
//пин кнопки ОК D3
#define OK_BIT   3 // D3
#define OK_PORT  PORTD
#define OK_PIN   PIND

#define OK_OUT   (BIT_READ(OK_PIN, OK_BIT))
#define OK_SET   (BIT_SET(OK_PORT, OK_BIT))
#define OK_INP   (BIT_CLEAR((DDR_REG(OK_PORT)), OK_BIT))

#define OK_INIT  OK_SET; OK_INP

//пин кнопки DOWN D7
#define DOWN_BIT   7 // D7
#define DOWN_PORT  PORTD
#define DOWN_PIN   PIND

#define DOWN_OUT   (BIT_READ(DOWN_PIN, DOWN_BIT))
#define DOWN_SET   (BIT_SET(DOWN_PORT, DOWN_BIT))
#define DOWN_INP   (BIT_CLEAR((DDR_REG(DOWN_PORT)), DOWN_BIT))

#define DOWN_INIT  DOWN_SET; DOWN_INP

//пин кнопки UP D4
#define UP_BIT   4 // D4
#define UP_PORT  PORTD
#define UP_PIN   PIND

#define UP_OUT   (BIT_READ(UP_PIN, UP_BIT))
#define UP_SET   (BIT_SET(UP_PORT, UP_BIT))
#define UP_INP   (BIT_CLEAR((DDR_REG(UP_PORT)), UP_BIT))

#define UP_INIT  UP_SET; UP_INP

//пин точек D5
#define DOT_BIT   5 // D5
#define DOT_PORT  PORTD

#define DOT_INV   (DOT_PORT ^= (1 << DOT_BIT))
#define DOT_ON    (BIT_SET(DOT_PORT, DOT_BIT))
#define DOT_OFF   (BIT_CLEAR(DOT_PORT, DOT_BIT))
#define DOT_OUT   (BIT_SET((DDR_REG(DOT_PORT)), DOT_BIT))

#define DOT_INIT  DOT_OFF; DOT_OUT

//пин колбы D9
#define FLASK_BIT   1 // D9
#define FLASK_PORT  PORTB

#define is_FLASK_ON   (BIT_READ(FLASK_PORT, FLASK_BIT))
#define FLASK_ON      (BIT_SET(FLASK_PORT, FLASK_BIT))
#define FLASK_OFF     (BIT_CLEAR(FLASK_PORT, FLASK_BIT))
#define FLASK_OUT     (BIT_SET((DDR_REG(FLASK_PORT)), FLASK_BIT))

#define FLASK_INIT  FLASK_OFF; FLASK_OUT

//пин основного питания RTC A3
#define RTC_BIT   3 // A3
#define RTC_PORT  PORTC

#define RTC_ON    (BIT_SET(RTC_PORT, RTC_BIT))
#define RTC_OFF   (BIT_CLEAR(RTC_PORT, RTC_BIT))
#define RTC_OUT   (BIT_SET((DDR_REG(RTC_PORT)), RTC_BIT))

#define RTC_INIT  RTC_OFF; RTC_OUT

//пин дополнительного питания RTC A1
#define RTC_BAT_BIT   1 // A1
#define RTC_BAT_PORT  PORTC

#define RTC_BAT_ON    (BIT_SET(RTC_BAT_PORT, RTC_BAT_BIT))
#define RTC_BAT_OFF   (BIT_CLEAR(RTC_BAT_PORT, RTC_BAT_BIT))
#define RTC_BAT_OUT   (BIT_SET((DDR_REG(RTC_BAT_PORT)), RTC_BAT_BIT))

#define RTC_BAT_INIT  RTC_BAT_ON; RTC_BAT_OUT
