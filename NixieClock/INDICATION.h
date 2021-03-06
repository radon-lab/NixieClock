#define FREQ_TICK constrain((uint8_t)((1000.0 / ((uint16_t)INDI_FREQ_ADG * (LAMP_NUM + 1))) / 0.016), 125, 255) //расчет переполнения таймера динамической индикации
#define LIGHT_MAX (uint8_t)(FREQ_TICK - INDI_DEAD_TIME) //расчет максимального шага яркости

#define US_PERIOD (uint16_t)(((uint16_t)FREQ_TICK + 1) * 16.0) //период тика таймера в мкс
#define US_PERIOD_MIN (uint16_t)(US_PERIOD - (US_PERIOD % 100) - 400) //минимальный период тика таймера
#define US_PERIOD_MAX (uint16_t)(US_PERIOD - (US_PERIOD % 100) + 400) //максимальный период тика таймера

#define MS_PERIOD (US_PERIOD / 1000) //период тика таймера в целых мс

#define R_COEF(low, high) (((float)low + (float)high) / (float)low) //коэффициент делителя напряжения
#define HV_ADC(vcc) (uint16_t)((1023.0 / (float)vcc) * ((float)GEN_HV_VCC / (float)R_COEF(GEN_HV_R_LOW, GEN_HV_R_HIGH))) //значение ацп удержания напряжения

#define RESET_SYSTEM __asm__ __volatile__ ("JMP 0x0000") //перезагрузка

#define _INDI_START TCNT0 = FREQ_TICK; TIMSK0 |= (0x01 << OCIE0B | 0x01 << OCIE0A) //запуск динамической индикации
#define _INDI_STOP TIMSK0 &= ~(0x01 << OCIE0B | 0x01 << OCIE0A); indiState = 0 //остановка динамической индикации

struct Settings_4 {
  uint16_t timePeriod = US_PERIOD; //коррекция хода внутреннего осцилятора
  uint8_t irButtons[8];
  uint8_t min_pwm = DEFAULT_MIN_PWM; //минимальный шим
  uint8_t max_pwm = DEFAULT_MAX_PWM; //максимальный шим
  uint8_t min_light = LIGHT_SENS_START_MIN; //минимальный шим
  uint8_t max_light = LIGHT_SENS_START_MAX; //максимальный шим
  int8_t hvCorrect; //коррекция напряжения
} debugSettings;

uint16_t hv_treshold = HV_ADC(5); //буфер сравнения напряжения
float pwm_coef; //коэффициент Linear Advance

uint8_t adc_light; //значение АЦП сенсора яркости освещения
boolean state_light = 1; //состояние сенсора яркости освещения

const uint8_t decoderMask[] = {DECODER_1, DECODER_2, DECODER_3, DECODER_4}; //порядок пинов дешефратора(0, 1, 2, 3)

uint8_t indi_dot; //буфер точек индикаторов
uint8_t dot_dimm; //яркость секундной точки
volatile uint8_t indi_dot_pos = 0x01; //текущей номер точек индикаторов

uint8_t indi_buf[7]; //буфер индикаторов
uint8_t indi_dimm[7]; //яркость индикаторов
uint8_t indi_null; //пустой сивол(отключеный индикатор)
volatile uint8_t indiState; //текущей номер отрисовки индикатора

void indiPrintNum(uint16_t num, int8_t indi, uint8_t length = 0, char filler = ' '); //отрисовка чисел

//----------------------------------Динамическая индикация---------------------------------------
ISR(TIMER0_COMPA_vect) //динамическая индикация
{
  OCR0B = indi_dimm[indiState]; //устанавливаем яркость индикатора

  PORTC = (PORTC & 0xF0) | indi_buf[indiState]; //отправляем в дешефратор буфер индикатора
  *anodePort[indiState] |= (indi_buf[indiState] != indi_null) ? anodeBit[indiState] : ANODE_OFF; //включаем индикатор если не пустой символ
#if DOTS_PORT_ENABLE
  if (indi_dot & indi_dot_pos) INDI_DOTS_ON; //включаем точки
#endif

  if (!++tick_ms) { //если превыше количество тиков
    SET_ERROR(RESET_ERROR); //устанавливаем ошибку аварийной перезагрузки
    RESET_SYSTEM; //перезагрузка
  }
}
ISR(TIMER0_COMPB_vect) {
  *anodePort[indiState] &= ~anodeBit[indiState]; //выключаем индикатор
#if DOTS_PORT_ENABLE
  INDI_DOTS_OFF; //выключаем точки
  indi_dot_pos <<= 1; //сместили текущей номер точек индикаторов
#endif
  if (++indiState > LAMP_NUM) { //переходим к следующему индикатору
#if DOTS_PORT_ENABLE
    indi_dot_pos = 0x01; //сбросили текущей номер точек индикаторов
#endif
    indiState = 0; //сбросили позицию индикатора
  }
}
//-----------------------------------Динамическая подсветка---------------------------------------
#if (BACKL_TYPE == 1) && !IR_PORT_ENABLE
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

//---------------------------Обновление коэффициента Linear Advance-------------------------------
void indiChangeCoef(void) //обновление коэффициента Linear Advance
{
  pwm_coef = ((float)(debugSettings.max_pwm - debugSettings.min_pwm) / (120.0 * LAMP_NUM));
}
//---------------------------------Установка Linear Advance---------------------------------------
void indiChangePwm(void) //установка Linear Advance
{
  uint16_t dimm_all = 0;
  for (uint8_t i = 1; i < (LAMP_NUM + 1); i++) if (indi_buf[i] != indi_null) dimm_all += indi_dimm[i];
  OCR1A = constrain(debugSettings.min_pwm + ((float)dimm_all * pwm_coef), 100, 200);
}
//--------------------------------Инициализация индикаторов---------------------------------------
void indiInit(void) //инициализация индикаторов
{
  for (uint8_t i = 0; i < 4; i++) {
    PORTC |= (0x01 << decoderMask[i]); //устанавливаем высокий уровень катода
    DDRC |= (0x01 << decoderMask[i]); //устанавливаем катод как выход

    if ((0x0A >> i) & 0x01) indi_null |= (0x01 << decoderMask[i]); //находим пустой символ
  }
  for (uint8_t i = 0; i < (LAMP_NUM + 1); i++) { //инициализируем пины
    *anodePort[i] &= ~anodeBit[i]; //устанавливаем низкий уровень анода
    *(anodePort[i] - 1) |= anodeBit[i]; //устанавливаем анод как выход

    indi_dimm[i] = LIGHT_MAX; //устанавливаем максимальную яркость
    indi_buf[i] = indi_null; //очищаем буфер пустыми символами
  }

#if DOTS_PORT_ENABLE
  INDI_DOTS_INIT; //инициализация разделительных точек в индикаторах
#endif

  OCR0A = FREQ_TICK; //максимальная частота
  OCR0B = LIGHT_MAX; //максимальная яркость

  TIMSK0 = 0; //отключаем прерывания Таймера0
  TCCR0A = (0x01 << WGM01); //режим CTC
  TCCR0B = (0x01 << CS02); //пределитель 256

#if PLAYER_TYPE == 2
  OCR1B = 128; //выключаем dac
#else
  OCR1B = 0; //выключаем точки
#endif

  TIMSK1 = 0; //отключаем прерывания Таймера1
  TCCR1A = (0x01 << WGM10); //режим коррекция фазы шим
  TCCR1B = (0x01 << CS10);  //задаем частоту 31 кГц

#if !NEON_DOT || PLAYER_TYPE == 2
  TCCR1A |= (0x01 << COM1B1); //подключаем D10
#endif
#if GEN_ENABLE
  OCR1A = constrain(debugSettings.min_pwm, 100, 200); //устанавливаем первичное значение шим
  TCCR1A |= (0x01 << COM1A1); //подключаем D9
#endif

  OCR2A = 0; //выключаем подсветку
  OCR2B = 0; //сбравсывем бузер

  TIMSK2 = 0; //выключаем прерывания Таймера2
#if (BACKL_TYPE)
  TCCR2A = 0; //отключаем OCR2A и OCR2B
#else
  TCCR2A = (0x01 << COM2A1 | 0x01 << WGM20 | 0x01 << WGM21); //подключаем D11
#endif
  TCCR2B = (0x01 << CS21); //пределитель 8

  sei(); //разрешаем прерывания глобально

  _INDI_START; //запускаем динамическую индикацию
}
//-------------------------Очистка индикаторов----------------------------------------------------
void indiClr(void) //очистка индикаторов
{
  for (uint8_t cnt = 0; cnt < LAMP_NUM; cnt++) indi_buf[cnt + 1] = indi_null;
#if GEN_ENABLE
  indiChangePwm(); //установка Linear Advance
#endif
}
//-------------------------Очистка индикатора----------------------------------------------------
void indiClr(uint8_t indi) //очистка индикатора
{
  indi_buf[indi + 1] = indi_null;
#if GEN_ENABLE
  indiChangePwm(); //установка Linear Advance
#endif
}
//-------------------------Установка индикатора----------------------------------------------------
void indiSet(uint8_t buf, uint8_t indi) //установка индикатора
{
  indi_buf[indi + 1] = buf; //устанавливаем в ячейку буфера
#if GEN_ENABLE
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
  _INDI_START; //запускаем генерацию
#if GEN_ENABLE
  TCCR1A |= (0x01 << COM1A1); //включаем шим преобразователя
#endif
}
//---------------------------------Выключение индикаторов---------------------------------------
void indiDisable(void) //выключение индикаторов
{
  _INDI_STOP; //отключаем генерацию
  for (uint8_t i = 0; i < (LAMP_NUM + 1); i++) {
    *anodePort[i] &= ~anodeBit[i]; //сбрасываем аноды
    if (i < 4) PORTC |= (0x01 << decoderMask[i]); //сбрасываем катоды
  }
#if GEN_ENABLE
  TCCR1A &= ~(0x01 << COM1A1); //выключаем шим преобразователя
  CONV_OFF; //выключаем пин преобразователя
#endif
}
//------------------------------Установка яркости индикатора--------------------------------------
void indiSetBright(uint8_t pwm, uint8_t indi) //установка яркости индикатора
{
  if (pwm > 30) pwm = 30;
  indi_dimm[indi + 1] = map(pwm, 0, 30, 0, LIGHT_MAX);
#if GEN_ENABLE
  indiChangePwm(); //установка Linear Advance
#endif
}
//--------------------------------Установка общей яркости-----------------------------------------
void indiSetBright(uint8_t pwm) //установка общей яркости
{
  if (pwm > 30) pwm = 30;
  pwm = map(pwm, 0, 30, 0, LIGHT_MAX);
  for (uint8_t i = 0; i < LAMP_NUM; i++) indi_dimm[i + 1] = pwm;
#if GEN_ENABLE
  indiChangePwm(); //установка Linear Advance
#endif
}
//----------------------------Установка разделителной точки---------------------------------------
void indiSetDots(uint8_t dot) //установка разделителной точки
{
  if (dot < LAMP_NUM) indi_dot |= (0x02 << dot);
}
//-----------------------------Очистка разделителных точек----------------------------------------
void indiClrDots(void) //очистка разделителных точек
{
  indi_dot = 0x00;
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
#if GEN_ENABLE
  indiChangePwm(); //установка Linear Advance
#endif
}
//-------------------------------Получить яркости подсветки---------------------------------------
inline uint8_t backGetBright(void) //получить яркости подсветки
{
  return OCR2A;
}
//------------------------------Установка яркости подсветки---------------------------------------
void backlSetBright(uint8_t pwm) //установка яркости подсветки
{
  OCR2A = pwm; //устанавливаем яркость точек
#if (BACKL_TYPE == 1) && !IR_PORT_ENABLE
  if (pwm) TIMSK2 |= (0x01 << OCIE2A | 0x01 << TOIE2); //включаем таймер
  else {
    TIMSK2 &= ~(0x01 << OCIE2A | 0x01 << TOIE2); //выключаем таймер
    BACKL_CLEAR; //выключили подсветку
  }
#elif !BACKL_TYPE
  if (pwm) TCCR2A |= (0x01 << COM2A1); //подключаем D11
  else {
    TCCR2A &= ~(0x01 << COM2A1); //отключаем D11
    BACKL_CLEAR; //выключили подсветку
  }
#endif
}
//-----------------------------------Уменьшение яркости------------------------------------------
boolean backlDecBright(uint8_t _step, uint8_t _min)
{
  if (((int16_t)backGetBright() - _step) > _min) backlSetBright(backGetBright() - _step);
  else {
    backlSetBright(_min);
    return 1;
  }
  return 0;
}
//-----------------------------------Увеличение яркости------------------------------------------
boolean backlIncBright(uint8_t _step, uint8_t _max)
{
  if (((uint16_t)backGetBright() + _step) < _max) backlSetBright(backGetBright() + _step);
  else {
    backlSetBright(_max);
    return 1;
  }
  return 0;
}
//----------------------------------Получить яркость точек---------------------------------------
inline uint8_t dotGetBright(void) //получить яркость точек
{
#if NEON_DOT
  return dot_dimm;
#else
  return OCR1B;
#endif
}
//---------------------------------Установка яркости точек---------------------------------------
void dotSetBright(uint8_t _pwm) //установка яркости точек
{
#if NEON_DOT
  dot_dimm = _pwm;
  indi_dimm[0] = map(_pwm, 0, 250, 0, LIGHT_MAX); //устанавливаем яркость точек
  if (_pwm) indi_buf[0] = 0; //разрешаем включать точки
  else indi_buf[0] = indi_null; //запрещаем включать точки
#else
  OCR1B = _pwm; //устанавливаем яркость точек
  if (_pwm) TCCR1A |= (0x01 << COM1B1); //подключаем D10
  else {
    TCCR1A &= ~(0x01 << COM1B1); //отключаем D10
    DOT_CLEAR; //выключили точки
  }
#endif
}
//--------------------------------Уменьшение яркости точек----------------------------------------
boolean dotDecBright(uint8_t _step, uint8_t _min)
{
  if (((int16_t)dotGetBright() - _step) > _min) dotSetBright(dotGetBright() - _step);
  else {
    dotSetBright(_min);
    return 1;
  }
  return 0;
}
//--------------------------------Увеличение яркости точек----------------------------------------
boolean dotIncBright(uint8_t _step, uint8_t _max)
{
  if (((uint16_t)dotGetBright() + _step) < _max) dotSetBright(dotGetBright() + _step);
  else {
    dotSetBright(_max);
    return 1;
  }
  return 0;
}
