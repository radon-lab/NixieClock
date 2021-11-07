#define _INDI_ON  TCNT0 = FREQ_TICK; TIMSK0 |= (0x01 << OCIE0B | 0x01 << OCIE0A) //запуск динамической индикации
#define _INDI_OFF TIMSK0 &= ~(0x01 << OCIE0B | 0x01 << OCIE0A); indiState = 0 //остановка динамической индикации

#define ANODE_OFF 0x00 //выключенный анод

#define FREQ_TICK (uint8_t)(1000 / (float)(FREQ_ADG * LAMP_NUM) / 0.016) //расчет переполнения таймера динамической индикации
#define LIGHT_STEP (uint8_t)((FREQ_TICK - 30) / 30) //расчет шага яркости

#define TIME_PERIOD (uint16_t)(FREQ_TICK * 16) //период тика таймера счета времени
#define TIME_PERIOD_MIN (uint16_t)(TIME_PERIOD - (TIME_PERIOD % 100) - 400) //минимальный период тика таймера счета времени
#define TIME_PERIOD_MAX (uint16_t)(TIME_PERIOD - (TIME_PERIOD % 100) + 400) //максимальный период тика таймера счета времени

//тип плат часов
#if (BOARD_TYPE == 0)
volatile uint8_t* anodePort[] = {&DOT_PORT, &ANODE_1_PORT, &ANODE_2_PORT, &ANODE_3_PORT, &ANODE_4_PORT, ANODE_OFF, ANODE_OFF}; //таблица портов анодов ламп
const uint8_t anodeBit[] = {0x01 << DOT_BIT, 0x01 << ANODE_1_BIT, 0x01 << ANODE_2_BIT, 0x01 << ANODE_3_BIT, 0x01 << ANODE_4_BIT, ANODE_OFF, ANODE_OFF}; //таблица бит анодов ламп
const uint8_t digitMask[] = {7, 3, 6, 4, 1, 9, 8, 0, 5, 2, 10};   //маска дешифратора платы in12 (цифры нормальные)(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 6, 2, 7, 5, 0, 4, 9, 8, 3};     //порядок катодов in12
#elif (BOARD_TYPE == 1)
volatile uint8_t* anodePort[] = {&DOT_PORT, &ANODE_4_PORT, &ANODE_3_PORT, &ANODE_2_PORT, &ANODE_1_PORT, ANODE_OFF, ANODE_OFF}; //таблица портов анодов ламп
const uint8_t anodeBit[] = {0x01 << DOT_BIT, 0x01 << ANODE_4_BIT, 0x01 << ANODE_3_BIT, 0x01 << ANODE_2_BIT, 0x01 << ANODE_1_BIT, ANODE_OFF, ANODE_OFF}; //таблица бит анодов ламп
const uint8_t digitMask[] = {2, 8, 1, 9, 6, 4, 3, 5, 0, 7, 10};   //маска дешифратора платы in12 turned (цифры вверх ногами)(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 6, 2, 7, 5, 0, 4, 9, 8, 3};     //порядок катодов in12
#elif (BOARD_TYPE == 2)
volatile uint8_t* anodePort[] = {&DOT_PORT, &ANODE_4_PORT, &ANODE_3_PORT, &ANODE_2_PORT, &ANODE_1_PORT, ANODE_OFF, ANODE_OFF}; //таблица портов анодов ламп
const uint8_t anodeBit[] = {0x01 << DOT_BIT, 0x01 << ANODE_4_BIT, 0x01 << ANODE_3_BIT, 0x01 << ANODE_2_BIT, 0x01 << ANODE_1_BIT, ANODE_OFF, ANODE_OFF}; //таблица бит анодов ламп
const uint8_t digitMask[] = {9, 8, 0, 5, 4, 7, 3, 6, 2, 1, 10};   //маска дешифратора платы in14(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 0, 2, 9, 3, 8, 4, 7, 5, 6};     //порядок катодов in14
#elif (BOARD_TYPE == 3)
volatile uint8_t* anodePort[] = {&DOT_PORT, &ANODE_4_PORT, &ANODE_3_PORT, &ANODE_2_PORT, &ANODE_1_PORT, ANODE_OFF, ANODE_OFF}; //таблица портов анодов ламп
const uint8_t anodeBit[] = {0x01 << DOT_BIT, 0x01 << ANODE_4_BIT, 0x01 << ANODE_3_BIT, 0x01 << ANODE_2_BIT, 0x01 << ANODE_1_BIT, ANODE_OFF, ANODE_OFF}; //таблица бит анодов ламп
const uint8_t digitMask[] = {9, 8, 0, 5, 2, 7, 3, 6, 4, 1, 10};   //маска дешифратора платы in12(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 6, 2, 7, 5, 0, 4, 9, 8, 3};     //порядок катодов in12
#elif (BOARD_TYPE == 4)
volatile uint8_t* anodePort[] = {&DOT_PORT, &ANODE_1_PORT, &ANODE_2_PORT, &ANODE_3_PORT, &ANODE_4_PORT, &ANODE_5_PORT, &ANODE_6_PORT}; //таблица портов анодов ламп
const uint8_t anodeBit[] = {0x01 << DOT_BIT, 0x01 << ANODE_1_BIT, 0x01 << ANODE_2_BIT, 0x01 << ANODE_3_BIT, 0x01 << ANODE_4_BIT, 0x01 << ANODE_5_BIT, 0x01 << ANODE_6_BIT}; //таблица бит анодов ламп
const uint8_t digitMask[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 10};   //тут вводим свой порядок пинов лампы(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};     //свой порядок катодов
#endif

const uint8_t decoderMask[] = {DECODER_4, DECODER_2, DECODER_1, DECODER_3}; //порядок пинов дешефратора(0, 1, 2, 3)

uint8_t indi_buf[7]; //буфер индикаторов
uint8_t indi_dimm[7]; //яркость индикаторов
uint8_t indi_null; //пустой сивол(отключеный индикатор)
volatile uint8_t indiState; //текущей номер отрисовки индикатора

volatile uint8_t tick_ms; //счетчик тиков миллисекунд
volatile uint8_t tick_sec; //счетчик тиков от RTC

void indiPrintNum(uint16_t num, int8_t indi, uint8_t length = 0, char filler = ' ');

//---------------------------------Динамическая индикация---------------------------------------
ISR(TIMER0_COMPA_vect) //динамическая индикация
{
  OCR0B = indi_dimm[indiState]; //устанавливаем яркость индикатора

  PORTC = (PORTC & 0xF0) | indi_buf[indiState]; //отправляем в дешефратор буфер индикатора
  *anodePort[indiState] |= (indi_buf[indiState] != indi_null) ? anodeBit[indiState] : ANODE_OFF; //включаем индикатор если не пустой символ

  tick_ms++; //прибавляем тик
}
ISR(TIMER0_COMPB_vect) {
  *anodePort[indiState] &= ~anodeBit[indiState]; //выключаем индикатор
  if (++indiState > LAMP_NUM) indiState = 0; //переходим к следующему индикатору
}
//-------------------------Инициализация индикаторов----------------------------------------------------
void IndiInit(void) //инициализация индикаторов
{
  for (uint8_t i = 0; i < 4; i++) {
    PORTC |= (0x01 << decoderMask[i]); //устанавливаем высокий уровень катода
    DDRC |= (0x01 << decoderMask[i]); //устанавливаем катод как выход

    if ((0x0A >> i) & 0x01) indi_null |= (0x01 << decoderMask[i]); //находим пустой символ
  }
  for (uint8_t i = 0; i < (LAMP_NUM + 1); i++) { //инициализируем пины
    *anodePort[i] &= ~anodeBit[i]; //устанавливаем низкий уровень анода
    *(anodePort[i] - 1) |= anodeBit[i]; //устанавливаем анод как выход

    indi_dimm[i] = 120; //устанавливаем максимальную юркость
    indi_buf[i] = indi_null; //очищаем буфер пустыми символами
  }

  OCR0A = FREQ_TICK; //максимальная частота
  OCR0B = 120; //максимальная яркость

  TIMSK0 = 0; //отключаем прерывания Таймера0
  TCCR0A = (0x01 << WGM01); //режим CTC
  TCCR0B = (0x01 << CS02); //пределитель 256

  OCR1A = MIN_PWM; //устанавливаем первичное значение шим
  OCR1B = 0; //выключаем точки

  TIMSK1 = 0; //отключаем прерывания Таймера1
#if NEON_DOT
  TCCR1A = (0x01 << COM1A1 | 0x01 << WGM10); //подключаем D9
#else
  TCCR1A = (0x01 << COM1B1 | 0x01 << COM1A1 | 0x01 << WGM10); //подключаем D9 и D10
#endif
  TCCR1B = (0x01 << CS10);  //задаем частоту ШИМ на 9 и 10 выводах 31 кГц

  OCR2A = 0; //выключаем подсветку
  OCR2B = 0; //сбравсывем бузер

  TIMSK2 = 0; //выключаем прерывания Таймера2
#if BACKL_WS2812B
  TCCR2A = (0x01 << WGM20 | 0x01 << WGM21); //отключаем OCR2A и OCR2B
#else
  TCCR2A = (0x01 << COM2A1 | 0x01 << WGM20 | 0x01 << WGM21); //подключаем D11
#endif
  TCCR2B = (0x01 << CS21); //пределитель 8

  sei(); //разрешаем прерывания глобально
  _INDI_ON; //запускаем динамическую индикацию
}
//---------------------------------Установка Linear Advance---------------------------------------
void indiChangePwm(void) //установка Linear Advance
{
  uint16_t dimm_all = 0;
  for (uint8_t i = !NEON_DOT; i < (LAMP_NUM + 1); i++) if (indi_buf[i] != indi_null) dimm_all += indi_dimm[i];
  OCR1A = MIN_PWM + (float)(dimm_all / (LAMP_NUM + NEON_DOT)) * ((float)(MAX_PWM - MIN_PWM) / 120.0);
}
//-------------------------Очистка индикаторов----------------------------------------------------
void indiClr(void) //очистка индикаторов
{
  for (uint8_t cnt = 0; cnt < LAMP_NUM; cnt++) indi_buf[cnt + 1] = indi_null;
  indiChangePwm(); //установка Linear Advance
}
//-------------------------Очистка индикатора----------------------------------------------------
void indiClr(uint8_t indi) //очистка индикатора
{
  indi_buf[indi + 1] = indi_null;
  indiChangePwm(); //установка Linear Advance
}
//-------------------------Установка индикатора----------------------------------------------------
void indiSet(uint8_t buf, uint8_t indi) //установка индикатора
{
  indi_buf[indi + 1] = buf; //устанавливаем в ячейку буфера
  indiChangePwm(); //установка Linear Advance
}
//-------------------------Получить состояние индикатора----------------------------------------------------
uint8_t indiGet(uint8_t indi) //получить состояние индикатора
{
  return indi_buf[indi + 1]; //возвращаем содержимое ячейки буфера
}
//---------------------------------Включение индикаторов---------------------------------------
void indiEnable(void) //включение индикаторов
{
  indiClr(); //очистка индикаторов
  _INDI_ON; //запускаем генерацию
  TCCR1A |= (0x01 << COM1A1); //включаем шим преобразователя
}
//---------------------------------Выключение индикаторов---------------------------------------
void indiDisable(void) //выключение индикаторов
{
  _INDI_OFF; //отключаем генерацию
  for (uint8_t i = 0; i < (LAMP_NUM + 1); i++) {
    *anodePort[i] &= ~anodeBit[i]; //сбрасываем аноды
    if (i < 4) PORTC |= (0x01 << decoderMask[i]); //сбрасываем катоды
  }
  TCCR1A &= ~(0x01 << COM1A1); //выключаем шим преобразователя
  CONV_OFF; //выключаем пин преобразователя
}
//---------------------------------Установка яркости индикатора---------------------------------------
void indiSetBright(uint8_t pwm, uint8_t indi) //установка яркости индикатора
{
  if (pwm > 30) pwm = 30;
  indi_dimm[indi + 1] = pwm * LIGHT_STEP;
  indiChangePwm(); //установка Linear Advance
}
//---------------------------------Установка общей яркости---------------------------------------
void indiSetBright(uint8_t pwm) //установка общей яркости
{
  if (pwm > 30) pwm = 30;
  for (uint8_t i = 0; i < LAMP_NUM; i++) indi_dimm[i + 1] = pwm * LIGHT_STEP;
  indiChangePwm(); //установка Linear Advance
}
//---------------------------------Установка яркости точек---------------------------------------
void dotSetBright(uint8_t pwm) //установка яркости точек
{
  OCR1B = pwm; //устанавливаем яркость точек
#if NEON_DOT
  pwm >>= 1; //ограничиваем диапазон
  indi_dimm[0] = pwm; //устанавливаем яркость точек
  if (pwm) indi_buf[0] = 0; //разрешаем включать точки
  else indi_buf[0] = indi_null; //запрещаем включать точки
  indiChangePwm(); //установка Linear Advance
#endif
}
//-------------------------Вывод чисел----------------------------------------------------
void indiPrintNum(uint16_t num, int8_t indi, uint8_t length, char filler) //вывод чисел
{
  uint8_t buf[LAMP_NUM]; //временный буфер
  uint8_t st[LAMP_NUM]; //основной буфер
  uint8_t c = 0, f = 0; //счетчики

  if (!num) { //если ноль
    if (length) { //если указана длинна строки
      for (c = 1; f < (length - 1); f++) st[f] = (filler != ' ') ? filler : 10; //заполняем символами заполнителями
      st[f] = 0; //устанавливаем ноль
    }
    else {
      st[0] = 0; //устанавливаем ноль
      c = 1; //прибавляем счетчик
    }
  }
  else {
    while (num > 0) { //если есть число
      buf[c] = num % 10; //забираем младший разряд в буфер
      num = (num - (num % 10)) / 10; //отнимаем младший разряд от числа
      c++; //прибавляем счетчик
    }

    if (length > c) { //если осталось место для символов заполнителей
      for (f = 0; f < (length - c); f++) st[f] = (filler != ' ') ? filler : 10; //заполняем символами заполнителями
    }
    for (uint8_t i = 0; i < c; i++) st[i + f] = buf[c - i - 1]; //переписываем в основной буфер
  }

  for (uint8_t cnt = 0; cnt < (c + f); cnt++) { //расшивровка символов
    uint8_t mergeBuf = 0; //временный буфер дешефратора
    for (uint8_t dec = 0; dec < 4; dec++) { //расставляем биты дешефратора
      if ((digitMask[st[cnt]] >> dec) & 0x01) mergeBuf |= (0x01 << decoderMask[dec]); //устанавливаем бит дешефратора
    }
    if (indi < 0) indi++; //если число за гранью поля индикаторов
    else if (indi < LAMP_NUM) indi_buf[1 + indi++] = mergeBuf; //если число в поле индикатора то устанавливаем его
  }
  indiChangePwm(); //установка Linear Advance
}
