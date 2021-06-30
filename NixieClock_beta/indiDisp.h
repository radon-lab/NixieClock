const uint8_t decoderBit[] = {3, 1, 0, 2}; //порядок битов дешефратора(3, 1, 0, 2)
const uint8_t decoderMask[] = {DECODER_1, DECODER_2, DECODER_3, DECODER_4}; //порядок и номера пинов дешефратора(0, 1, 2, 3)

uint8_t indi_buf[4]; //буфер индикаторов
uint8_t indi_dimm[4]; //яркость индикаторов
uint8_t indi_null; //пустой сивол(отключеный индикатор)
volatile uint8_t indi_state; //текущей номер отрисовки индикатора

volatile uint8_t tick_ms; //счетчик тиков миллисекунд
volatile uint8_t tick_sec; //счетчик тиков от RTC

#define LEFT 0
#define RIGHT 255
#define CENTER 254

#define _INDI_ON  TCNT0 = 255; TIMSK0 |= (0x01 << OCIE0A | 0x01 << TOIE0)
#define _INDI_OFF TIMSK0 &= ~(0x01 << OCIE0A | 0x01 << TOIE0); indi_state = 0

void indiPrintNum(uint16_t num, uint8_t indi, uint8_t length = 0, char filler = ' ');

void setPin(uint8_t pin, boolean x) {
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
ISR(TIMER0_OVF_vect) //динамическая индикация
{
  OCR0A = indi_dimm[indi_state]; //устанавливаем яркость индикатора

  PORTC = (PORTC & 0xF0) | indi_buf[indi_state]; //отправляем в дешефратор буфер индикатора
  PORTD |= (indi_buf[indi_state] != indi_null) ? (0x01 << anodeMask[indi_state]) : 0x00; //включаем индикатор если не пустой символ

#if USE_NEON_DOT
  if (OCR0B) DOT_ON;
#endif

  tick_ms++; //прибавляем тик
}
ISR(TIMER0_COMPA_vect) {
  PORTD &= ~(0x01 << anodeMask[indi_state]); //выключаем индикатор
  if (++indi_state > 3) indi_state = 0; //переходим к следующему индикатору
}
#if USE_NEON_DOT
ISR(TIMER0_COMPB_vect) {
  DOT_OFF;
}
#endif
//-------------------------Инициализация индикаторов----------------------------------------------------
void indiInit(void) //инициализация индикаторов
{
  for (uint8_t dec = 0; dec < 4; dec++) {
    if ((0x0A >> dec) & 0x01) indi_null |= (0x01 << decoderBit[dec]); //находим пустой символ
  }
  for (uint8_t i = 0; i < 4; i++) { //инициализируем пины
    setPin(anodeMask[i], 0); //устанавливаем в 0
    outPin(anodeMask[i]); //устанавливаем как выход

    setPin(decoderMask[i], 1); //устанавливаем в 1
    outPin(decoderMask[i]); //устанавливаем как выход

    indi_dimm[i] = 120; //устанавливаем максимальную юркость
    indi_buf[i] = indi_null; //очищаем буфер пустыми символами
  }

  OCR0A = 120; //максимальная яркость

  TIMSK0 = 0; //отключаем прерывания Таймера0
  TCCR0A = 0; //отключаем OC0A/OC0B
  TCCR0B = (1 << CS02); //пределитель 256

  OCR1A = MIN_PWM; //устанавливаем первичное значение шим
  OCR1B = 0; //выключаем точки

  TIMSK1 = 0; //отключаем прерывания Таймера1
#if USE_NEON_DOT
  TCCR1A = (1 << COM1A1 | 1 << WGM10); //подключаем D9
#else
  TCCR1A = (1 << COM1B1 | 1 << COM1A1 | 1 << WGM10); //подключаем D9 и D10
#endif
  TCCR1B = (1 << CS10);  //задаем частоту ШИМ на 9 и 10 выводах 31 кГц

  OCR2A = 0; //выключаем подсветку

  TIMSK2 = 0; //отключаем прерывания Таймера2
  TCCR2A = (1 << COM2A1 | 1 << WGM20 | 1 << WGM21); //подключаем D11
  TCCR2B = (1 << CS21); //пределитель 8

  sei(); //разрешаем прерывания глобально

  _INDI_ON;
}
//---------------------------------Установка Linear Advance---------------------------------------
void indiChangePwm(void) //установка Linear Advance
{
  uint16_t dimm_all = 0;
  for (byte i = 0; i < 4; i++) if (indi_buf[i] != indi_null) dimm_all += indi_dimm[i];
  OCR1A = MIN_PWM + (float)(dimm_all >> 2) * ((float)(MAX_PWM - MIN_PWM) / 120.0);
}
//-------------------------Очистка индикаторов----------------------------------------------------
void indiClr(void) //очистка индикаторов
{
  for (uint8_t cnt = 0; cnt < 4; cnt++) indi_buf[cnt] = indi_null;
  indiChangePwm(); //установка Linear Advance
}
//-------------------------Очистка индикатора----------------------------------------------------
void indiClr(uint8_t indi) //очистка индикатора
{
  indi_buf[indi] = indi_null;
  indiChangePwm(); //установка Linear Advance
}
//-------------------------Установка индикатора----------------------------------------------------
void indiSet(uint8_t buf, uint8_t indi) //установка индикатора
{
  indi_buf[indi] = buf;
  indiChangePwm(); //установка Linear Advance
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
void indiSetBright(uint8_t pwm, uint8_t indi) //установка яркости индикатора
{
  if (pwm > 30) pwm = 30;
  indi_dimm[indi] = pwm << 2;
  indiChangePwm(); //установка Linear Advance
}
//---------------------------------Установка общей яркости---------------------------------------
void indiSetBright(uint8_t pwm) //установка общей яркости
{
  if (pwm > 30) pwm = 30;
  for (byte i = 0; i < 4; i++) indi_dimm[i] = pwm << 2;
  indiChangePwm(); //установка Linear Advance
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
  indiChangePwm(); //установка Linear Advance
}
