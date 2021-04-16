#include <Arduino.h>

const byte decoderBit[] = {3, 1, 0, 2}; //порядок битов дешефратора(3, 1, 0, 2)
const byte decoderMask[] = {DECODER_1, DECODER_2, DECODER_3, DECODER_4}; //порядок и номера пинов дешефратора(0, 1, 2, 3)

uint8_t indi_buf[4];
uint8_t indi_dimm[4];
volatile uint8_t indi_state;

#define LEFT 0
#define RIGHT 255
#define CENTER 254

#define _INDI_ON  TCNT2 = 255; TIMSK2 |= (0x01 << OCIE2A | 0x01 << TOIE2)
#define _INDI_OFF TIMSK2 &= ~(0x01 << OCIE2A | 0x01 << TOIE2); indi_state = 0

void indiInit(void);
void indiEnable(void);
void indiDisable(void);
void indiSetBright(uint8_t indi, uint8_t pwm);
void indiSetBright(uint8_t pwm);
void indiClr(void);
void indiClr(uint8_t indi);
void indiPrintNum(uint16_t num, uint8_t indi, uint8_t length = 0, char filler = ' ');

inline void setPin(uint8_t pin, boolean x) {
  if (pin < 8) BIT_WRITE(PORTD, pin, x);
  else if (pin < 14) BIT_WRITE(PORTB, (pin - 8), x);
  else if (pin < 20) BIT_WRITE(PORTC, (pin - 14), x);
  else return;
}
void outPin(uint8_t pin) {
  if (pin < 8) BIT_SET(DDRD, pin);
  else if (pin < 14) BIT_SET(DDRB, (pin - 8));
  else if (pin < 20) BIT_SET(DDRC, (pin - 14));
  else return;
}

//---------------------------------Динамическая индикация---------------------------------------
ISR(TIMER2_OVF_vect) //динамическая индикация
{
  OCR2A = indi_dimm[indi_state]; //устанавливаем яркость индикатора

  PORTC = (PORTC & 0xF0) | indi_buf[indi_state]; //отправляем в дешефратор буфер индикатора
  PORTD |= (indi_buf[indi_state] != 0x06) ? (0x01 << anodeMask[indi_state]) : 0x00; //включаем индикатор
}
ISR(TIMER2_COMPA_vect) {
  PORTD &= ~(0x01 << anodeMask[indi_state]); //выключаем индикатор
  if (++indi_state > 3) indi_state = 0; //переходим к следующему индикатору
}
//-------------------------Инициализация индикаторов----------------------------------------------------
void indiInit(void) //инициализация индикаторов
{
  for (uint8_t i = 0; i < 4; i++) {
    setPin(anodeMask[i], 0);
    outPin(anodeMask[i]);

    setPin(decoderMask[i], 1);
    outPin(decoderMask[i]);

    indi_dimm[i] = 64;
  }

  OCR2A = 64;

  TIMSK2 = 0; //отключаем прерывания Таймера2
  TCCR2A = 0; //отключаем OC2A/OC2B
  TCCR2B = (1 << CS22); //пределитель 64

  sei(); //разрешаем прерывания глобально

  _INDI_ON;
}
//---------------------------------Включение индикаторов---------------------------------------
void indiEnable(void) //включение индикаторов
{
  indiClr(); //очистка индикаторов
  _INDI_ON; //запускаем генерацию
}
//---------------------------------Выключение индикаторов---------------------------------------
void indiDisable(void) //выключение индикаторов
{
  _INDI_OFF; //отключаем генерацию
  for (uint8_t i = 0; i < 4; i++) {
    setPin(anodeMask[i], 0); //сбрасываем аноды
    setPin(decoderMask[i], 1); //сбрасываем катоды
  }
}
//---------------------------------Установка яркости индикатора---------------------------------------
void indiSetBright(uint8_t indi, uint8_t pwm) //установка яркости индикатора
{
  if (pwm > 64) pwm = 64;
  indi_dimm[indi] = pwm * 3;
}
//---------------------------------Установка общей яркости---------------------------------------
void indiSetBright(uint8_t pwm) //установка общей яркости
{
  if (pwm > 64) pwm = 64;
  for (byte i = 0; i < 4; i++) {
    indi_dimm[i] = pwm * 3;
  }
}
//-------------------------Очистка индикаторов----------------------------------------------------
void indiClr(void) //очистка индикаторов
{
  for (uint8_t cnt = 0; cnt < 4; cnt++) {
    uint8_t mergeBuf = 0;
    for (uint8_t dec = 0; dec < 4; dec++) {
      if ((0x0A >> dec) & 0x01) mergeBuf |= (0x01 << decoderBit[dec]);
    }
    indi_buf[cnt] = mergeBuf;
  }
}
//-------------------------Очистка индикатора----------------------------------------------------
void indiClr(uint8_t indi) //очистка индикатора
{
  uint8_t mergeBuf = 0;
  for (uint8_t dec = 0; dec < 4; dec++) {
    if ((0x0A >> dec) & 0x01) mergeBuf |= (0x01 << decoderBit[dec]);
  }
  indi_buf[indi] = mergeBuf;
}
//-------------------------Вывод чисел----------------------------------------------------
void indiPrintNum(uint16_t num, uint8_t indi, uint8_t length, char filler) //вывод чисел
{
  uint8_t buf[4];
  uint8_t st[4];
  uint8_t c = 0, f = 0;

  if (!num) {
    if (length) {
      for (c = 1; f < (length - 1); f++) st[f] = (filler != ' ') ? filler : 10;
      st[f] = 0;
    }
    else {
      st[0] = 0;
      c = 1;
    }
  }
  else {
    while (num > 0) {
      buf[c] = num % 10;
      num = (num - (num % 10)) / 10;
      c++;
    }

    if (length > c) {
      for (f = 0; f < (length - c); f++) st[f] = (filler != ' ') ? filler : 10;
    }

    for (uint8_t i = 0; i < c; i++) st[i + f] = buf[c - i - 1];
  }

  switch (indi) {
    case RIGHT: indi = 4 - (c + f); break;
    case CENTER: indi = 4 - (c + f) / 2; break;
  }

  for (uint8_t cnt = 0; cnt < (c + f); cnt++) {
    uint8_t mergeBuf = 0;
    for (uint8_t dec = 0; dec < 4; dec++) {
      if ((digitMask[st[cnt]] >> dec) & 0x01) mergeBuf |= (0x01 << decoderBit[dec]);
    }
    indi_buf[indi++] = mergeBuf;
  }
}
