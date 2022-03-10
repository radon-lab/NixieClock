#define _INDI_ON  TCNT0 = FREQ_TICK; TIMSK0 |= (0x01 << OCIE0B | 0x01 << OCIE0A) //запуск динамической индикации
#define _INDI_OFF TIMSK0 &= ~(0x01 << OCIE0B | 0x01 << OCIE0A); indiState = 0 //остановка динамической индикации

#define ANODE_OFF 0x00 //выключенный анод

#define DEAD_TIME 30 //период тишины для закрытия оптопар
#define FREQ_TICK (uint8_t)(1000 / (float)(INDI_FREQ_ADG * LAMP_NUM) / 0.016) //расчет переполнения таймера динамической индикации
#define LIGHT_STEP (uint8_t)((FREQ_TICK - DEAD_TIME) / 30) //расчет шага яркости

#define US_PERIOD (uint16_t)(FREQ_TICK * 16) //период тика таймера в мкс
#define US_PERIOD_MIN (uint16_t)(US_PERIOD - (US_PERIOD % 100) - 400) //минимальный период тика таймера
#define US_PERIOD_MAX (uint16_t)(US_PERIOD - (US_PERIOD % 100) + 400) //максимальный период тика таймера

#define MS_PERIOD (US_PERIOD / 1000) //период тика таймера в целых мс

#define R_COEF(low, high) (((float)low + (float)high) / (float)low) //коэффициент делителя напряжения
#define HV_ADC(vcc) (uint8_t)(256.0 / (float)vcc * ((float)GEN_HV_VCC / (float)R_COEF(GEN_HV_R_LOW, GEN_HV_R_HIGH)))

#define RESET_SYSTEM __asm__ __volatile__ ("JMP 0x0000") //перезагрузка

//тип плат часов
#if (BOARD_TYPE == 0) //IN-12 (индикаторы стоят правильно)
volatile uint8_t* anodePort[] = {&DOT_PORT, &ANODE_1_PORT, &ANODE_2_PORT, &ANODE_3_PORT, &ANODE_4_PORT, ANODE_OFF, ANODE_OFF}; //таблица портов анодов ламп
const uint8_t anodeBit[] = {0x01 << DOT_BIT, 0x01 << ANODE_1_BIT, 0x01 << ANODE_2_BIT, 0x01 << ANODE_3_BIT, 0x01 << ANODE_4_BIT, ANODE_OFF, ANODE_OFF}; //таблица бит анодов ламп
const uint8_t digitMask[] = {7, 3, 6, 4, 1, 9, 8, 0, 5, 2, 10};   //маска дешифратора платы in12 (цифры нормальные)(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 6, 2, 7, 5, 0, 4, 9, 8, 3};     //порядок катодов in12
#elif (BOARD_TYPE == 1) //IN-12 turned (индикаторы перевёрнуты)
volatile uint8_t* anodePort[] = {&DOT_PORT, &ANODE_4_PORT, &ANODE_3_PORT, &ANODE_2_PORT, &ANODE_1_PORT, ANODE_OFF, ANODE_OFF}; //таблица портов анодов ламп
const uint8_t anodeBit[] = {0x01 << DOT_BIT, 0x01 << ANODE_4_BIT, 0x01 << ANODE_3_BIT, 0x01 << ANODE_2_BIT, 0x01 << ANODE_1_BIT, ANODE_OFF, ANODE_OFF}; //таблица бит анодов ламп
const uint8_t digitMask[] = {2, 8, 1, 9, 6, 4, 3, 5, 0, 7, 10};   //маска дешифратора платы in12 turned (цифры вверх ногами)(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 6, 2, 7, 5, 0, 4, 9, 8, 3};     //порядок катодов in12
#elif (BOARD_TYPE == 2) //IN-14 (обычная и neon dot)
volatile uint8_t* anodePort[] = {&DOT_PORT, &ANODE_4_PORT, &ANODE_3_PORT, &ANODE_2_PORT, &ANODE_1_PORT, ANODE_OFF, ANODE_OFF}; //таблица портов анодов ламп
const uint8_t anodeBit[] = {0x01 << DOT_BIT, 0x01 << ANODE_4_BIT, 0x01 << ANODE_3_BIT, 0x01 << ANODE_2_BIT, 0x01 << ANODE_1_BIT, ANODE_OFF, ANODE_OFF}; //таблица бит анодов ламп
const uint8_t digitMask[] = {9, 8, 0, 5, 4, 7, 3, 6, 2, 1, 10};   //маска дешифратора платы in14(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 0, 2, 9, 3, 8, 4, 7, 5, 6};     //порядок катодов in14
#elif (BOARD_TYPE == 3) //другие индикаторы(4 лампы)
volatile uint8_t* anodePort[] = {&DOT_PORT, &ANODE_4_PORT, &ANODE_3_PORT, &ANODE_2_PORT, &ANODE_1_PORT, ANODE_OFF, ANODE_OFF}; //таблица портов анодов ламп
const uint8_t anodeBit[] = {0x01 << DOT_BIT, 0x01 << ANODE_4_BIT, 0x01 << ANODE_3_BIT, 0x01 << ANODE_2_BIT, 0x01 << ANODE_1_BIT, ANODE_OFF, ANODE_OFF}; //таблица бит анодов ламп
const uint8_t digitMask[] = {9, 8, 0, 5, 2, 7, 3, 6, 4, 1, 10};   //маска дешифратора платы in12(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 6, 2, 7, 5, 0, 4, 9, 8, 3};     //порядок катодов in12
#elif (BOARD_TYPE == 4) //другие индикаторы(6 ламп)
volatile uint8_t* anodePort[] = {&DOT_PORT, &ANODE_1_PORT, &ANODE_2_PORT, &ANODE_3_PORT, &ANODE_4_PORT, &ANODE_5_PORT, &ANODE_6_PORT}; //таблица портов анодов ламп
const uint8_t anodeBit[] = {0x01 << DOT_BIT, 0x01 << ANODE_1_BIT, 0x01 << ANODE_2_BIT, 0x01 << ANODE_3_BIT, 0x01 << ANODE_4_BIT, 0x01 << ANODE_5_BIT, 0x01 << ANODE_6_BIT}; //таблица бит анодов ламп
const uint8_t digitMask[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 10};   //тут вводим свой порядок пинов лампы(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};     //свой порядок катодов
#endif

struct Settings {
  uint16_t timePeriod = US_PERIOD; //коррекция хода внутреннего осцилятора
  uint8_t min_pwm = DEFAULT_MIN_PWM; //минимальный шим
  uint8_t max_pwm = DEFAULT_MAX_PWM; //максимальный шим
  int8_t hvCorrect = 0; //коррекция напряжения
} debugSettings;

uint8_t hv_treshold = HV_ADC(5); //буфер сравнения напряжения

const uint8_t decoderMask[] = {DECODER_1, DECODER_2, DECODER_3, DECODER_4}; //порядок пинов дешефратора(0, 1, 2, 3)

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

  if (!++tick_ms) RESET_SYSTEM; //прибавляем тик
}
ISR(TIMER0_COMPB_vect) {
  *anodePort[indiState] &= ~anodeBit[indiState]; //выключаем индикатор
  if (++indiState > LAMP_NUM) indiState = 0; //переходим к следующему индикатору
}
//---------------------------------Динамическая подсветка---------------------------------------
#if (!BACKL_WS2812B && BACKL_MODE)
ISR(TIMER2_OVF_vect, ISR_NAKED) //прерывание подсветки
{
  BACKL_SET; //включили подсветку
  reti(); //возврат
}
ISR(TIMER2_COMPA_vect, ISR_NAKED) //прерывание подсветки
{
  BACKL_CLEAR; //выключили подсветку
  reti(); //возврат
}
#endif
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

  OCR1B = 0; //выключаем точки
  TIMSK1 = 0; //отключаем прерывания Таймера1
  TCCR1A = 0; //отключаем шим

#if !NEON_DOT
  TCCR1A |= (0x01 << COM1B1); //подключаем D10
#endif
#if !GEN_DISABLE
  OCR1A = constrain(debugSettings.min_pwm, 100, 200); //устанавливаем первичное значение шим
  TCCR1A |= (0x01 << COM1A1); //подключаем D9
#endif
  TCCR1A |= (0x01 << WGM10); //режим коррекция фазы шим
  TCCR1B = (0x01 << CS10);  //задаем частоту 31 кГц

  OCR2A = 0; //выключаем подсветку
  OCR2B = 0; //сбравсывем бузер

  TIMSK2 = 0; //выключаем прерывания Таймера2
#if (BACKL_WS2812B || BACKL_MODE)
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
  for (uint8_t i = 1; i < (LAMP_NUM + 1); i++) if (indi_buf[i] != indi_null) dimm_all += indi_dimm[i];
  OCR1A = constrain(debugSettings.min_pwm + (float)(dimm_all / LAMP_NUM) * ((float)(debugSettings.max_pwm - debugSettings.min_pwm) / 120.0), 100, 200);
}
//-------------------------Очистка индикаторов----------------------------------------------------
void indiClr(void) //очистка индикаторов
{
  for (uint8_t cnt = 0; cnt < LAMP_NUM; cnt++) indi_buf[cnt + 1] = indi_null;
#if !GEN_DISABLE
  indiChangePwm(); //установка Linear Advance
#endif
}
//-------------------------Очистка индикатора----------------------------------------------------
void indiClr(uint8_t indi) //очистка индикатора
{
  indi_buf[indi + 1] = indi_null;
#if !GEN_DISABLE
  indiChangePwm(); //установка Linear Advance
#endif
}
//-------------------------Установка индикатора----------------------------------------------------
void indiSet(uint8_t buf, uint8_t indi) //установка индикатора
{
  indi_buf[indi + 1] = buf; //устанавливаем в ячейку буфера
#if !GEN_DISABLE
  indiChangePwm(); //установка Linear Advance
#endif
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
#if !GEN_DISABLE
  TCCR1A |= (0x01 << COM1A1); //включаем шим преобразователя
#endif
}
//---------------------------------Выключение индикаторов---------------------------------------
void indiDisable(void) //выключение индикаторов
{
  _INDI_OFF; //отключаем генерацию
  for (uint8_t i = 0; i < (LAMP_NUM + 1); i++) {
    *anodePort[i] &= ~anodeBit[i]; //сбрасываем аноды
    if (i < 4) PORTC |= (0x01 << decoderMask[i]); //сбрасываем катоды
  }
#if !GEN_DISABLE
  TCCR1A &= ~(0x01 << COM1A1); //выключаем шим преобразователя
  CONV_OFF; //выключаем пин преобразователя
#endif
}
//---------------------------------Установка яркости индикатора---------------------------------------
void indiSetBright(uint8_t pwm, uint8_t indi) //установка яркости индикатора
{
  if (pwm > 30) pwm = 30;
  indi_dimm[indi + 1] = pwm * LIGHT_STEP;
#if !GEN_DISABLE
  indiChangePwm(); //установка Linear Advance
#endif
}
//--------------------------------Установка общей яркости-----------------------------------------
void indiSetBright(uint8_t pwm) //установка общей яркости
{
  if (pwm > 30) pwm = 30;
  for (uint8_t i = 0; i < LAMP_NUM; i++) indi_dimm[i + 1] = pwm * LIGHT_STEP;
#if !GEN_DISABLE
  indiChangePwm(); //установка Linear Advance
#endif
}
//------------------------------Установка яркости подсветки---------------------------------------
void backlSetBright(uint8_t pwm) //установка яркости подсветки
{
  OCR2A = pwm; //устанавливаем яркость точек
#if BACKL_MODE
  if (pwm) TIMSK2 |= (0x01 << OCIE2A | 0x01 << TOIE2); //включаем таймер
  else {
    TIMSK2 &= ~(0x01 << OCIE2A | 0x01 << TOIE2); //выключаем таймер
    BACKL_CLEAR; //выключили подсветку
  }
#else
  if (pwm) TCCR2A |= (0x01 << COM2A1); //подключаем D11
  else TCCR2A &= ~(0x01 << COM2A1); //отключаем D11
#endif
}
//-----------------------------------Уменьшение яркости------------------------------------------
boolean backlDecBright(uint8_t _step, uint8_t _min)
{
  if ((int16_t)(OCR2A - _step) > _min) backlSetBright(OCR2A - _step);
  else {
    backlSetBright(_min);
    return 1;
  }
  return 0;
}
//-----------------------------------Увеличение яркости------------------------------------------
boolean backlIncBright(uint8_t _step, uint8_t _max)
{
  if ((uint16_t)(OCR2A + _step) < _max) backlSetBright(OCR2A + _step);
  else {
    backlSetBright(_max);
    return 1;
  }
  return 0;
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
#endif
}
//--------------------------------Уменьшение яркости точек----------------------------------------
boolean dotDecBright(uint8_t _step, uint8_t _min)
{
  if ((int16_t)(OCR1B - _step) > _min) dotSetBright(OCR1B - _step);
  else {
    dotSetBright(_min);
    return 1;
  }
  return 0;
}
//--------------------------------Увеличение яркости точек----------------------------------------
boolean dotIncBright(uint8_t _step, uint8_t _max)
{
  if ((uint16_t)(OCR1B + _step) < _max) dotSetBright(OCR1B + _step);
  else {
    dotSetBright(_max);
    return 1;
  }
  return 0;
}
//-------------------------Вывод чисел----------------------------------------------------
void indiPrintNum(uint16_t _num, int8_t _indi, uint8_t _length, char _filler) //вывод чисел
{
  uint8_t buf[6]; //временный буфер
  uint8_t _count = 0; //счетчик символов

  if (!_num) { //если ноль
    buf[0] = digitMask[0]; //устанавливаем ноль
    _count = 1; //прибавляем счетчик
  }
  else { //иначе заполняем буфер числами
    while (_num && (_count < 6)) { //если есть число
      buf[_count++] = digitMask[_num % 10]; //забираем младший разряд в буфер
      _num /= 10; //отнимаем младший разряд от числа
    }
  }

  while ((_length > _count) && (_count < 6)) buf[_count++] = digitMask[(_filler != ' ') ? _filler : 10]; //заполняем символами заполнителями

  while (_count) { //расшивровка символов
    _count--; //убавили счетчик символов
    uint8_t mergeBuf = 0; //временный буфер дешефратора
    for (uint8_t dec = 0; dec < 4; dec++) { //расставляем биты дешефратора
      if ((buf[_count] >> dec) & 0x01) mergeBuf |= (0x01 << decoderMask[dec]); //устанавливаем бит дешефратора
    }
    if (_indi < 0) _indi++; //если число за гранью поля индикаторов
    else if (_indi < LAMP_NUM) indi_buf[++_indi] = mergeBuf; //если число в поле индикатора то устанавливаем его
  }
#if !GEN_DISABLE
  indiChangePwm(); //установка Linear Advance
#endif
}
