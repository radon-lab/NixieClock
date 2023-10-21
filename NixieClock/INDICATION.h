#define FREQ_TICK (uint8_t)(CONSTRAIN((1000.0 / ((uint16_t)INDI_FREQ_ADG * (LAMP_NUM + (boolean)((NEON_DOT != 0) && (NEON_DOT != 3))))) / 0.016, 125, 255)) //расчет переполнения таймера динамической индикации

#define US_PERIOD (uint16_t)(((uint16_t)FREQ_TICK + 1) * 16.0) //период тика таймера в мкс
#define US_PERIOD_MIN (uint16_t)(US_PERIOD - (US_PERIOD % 100) - 400) //минимальный период тика таймера
#define US_PERIOD_MAX (uint16_t)(US_PERIOD - (US_PERIOD % 100) + 400) //максимальный период тика таймера

#define MS_PERIOD (uint8_t)(US_PERIOD / 1000) //период тика таймера в целых мс

#define LIGHT_MAX (uint8_t)(FREQ_TICK - INDI_DEAD_TIME) //расчет максимального шага яркости
#define DOT_LIGHT_MAX (uint8_t)(CONSTRAIN(((uint16_t)LIGHT_MAX - 2) + (LIGHT_MAX >> 5), 100, 255)) //расчет максимального шага яркости для точек
#define INDI_LIGHT_MAX (uint16_t)(((uint16_t)LIGHT_MAX * 8) + (LIGHT_MAX >> 1)) //расчет максимального шага яркости для индикаторов

#define R_COEF(low, high) (((float)low + (float)high) / (float)low) //коэффициент делителя напряжения
#define HV_ADC(vcc) (uint16_t)((1023.0 / (float)vcc) * ((float)GEN_HV_VCC / (float)R_COEF(GEN_HV_R_LOW, GEN_HV_R_HIGH))) //значение ацп удержания напряжения
#define GET_VCC(ref, adc) (float)((ref * 1023.0) / (float)adc) //расчет напряжения питания

#define RESET_SYSTEM __asm__ __volatile__ ("JMP 0x0000") //перезагрузка
#define RESET_WDT __asm__ __volatile__ ("WDR") //сброс WDT

#define _BIT(value, bit) (((value) >> (bit)) & 0x01)
#if WIRE_PULL && !ESP_ENABLE
#define ID(digit) ((_BIT(digit, 0) << DECODER_1) | (_BIT(digit, 1) << DECODER_2) | (_BIT(digit, 2) << DECODER_3) | (_BIT(digit, 3) << DECODER_4) | 0x30)
#define INDI_NULL ((0x01 << DECODER_2) | (0x01 << DECODER_4) | 0x30) //пустой символ(отключеный индикатор)
#else
#define ID(digit) ((_BIT(digit, 0) << DECODER_1) | (_BIT(digit, 1) << DECODER_2) | (_BIT(digit, 2) << DECODER_3) | (_BIT(digit, 3) << DECODER_4))
#define INDI_NULL ((0x01 << DECODER_2) | (0x01 << DECODER_4)) //пустой символ(отключеный индикатор)
#endif

#if DOTS_TYPE == 2
#define DOTS_ALL (DOTS_NUM * 2) //всего разделительных точек
#else
#define DOTS_ALL (DOTS_NUM) //всего разделительных точек
#endif

//Типы плат часов
#if (BOARD_TYPE == 0) //IN-12 (индикаторы стоят правильно)
enum {DOT_POS, ANODE_1_POS, ANODE_2_POS, ANODE_3_POS, ANODE_4_POS}; //порядок анодов ламп(точки всегда должны быть первыми)(только для прямого подключения к микроконтроллеру)
const uint8_t digitMask[] = {ID(7), ID(3), ID(6), ID(4), ID(1), ID(9), ID(8), ID(0), ID(5), ID(2), ID(10)}; //маска дешифратора платы in12 (цифры нормальные)(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 6, 2, 7, 5, 0, 4, 9, 8, 3}; //порядок катодов in12
#elif (BOARD_TYPE == 1) //IN-12 turned (индикаторы перевёрнуты)
enum {DOT_POS, ANODE_4_POS, ANODE_3_POS, ANODE_2_POS, ANODE_1_POS}; //порядок анодов ламп(точки всегда должны быть первыми)(только для прямого подключения к микроконтроллеру)
const uint8_t digitMask[] = {ID(2), ID(8), ID(1), ID(9), ID(6), ID(4), ID(3), ID(5), ID(0), ID(7), ID(10)}; //маска дешифратора платы in12 turned (цифры вверх ногами)(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 6, 2, 7, 5, 0, 4, 9, 8, 3}; //порядок катодов in12
#elif (BOARD_TYPE == 2) //IN-14 (обычная и neon dot)
enum {DOT_POS, ANODE_4_POS, ANODE_3_POS, ANODE_2_POS, ANODE_1_POS}; //порядок анодов ламп(точки всегда должны быть первыми)(только для прямого подключения к микроконтроллеру)
const uint8_t digitMask[] = {ID(9), ID(8), ID(0), ID(5), ID(4), ID(7), ID(3), ID(6), ID(2), ID(1), ID(10)}; //маска дешифратора платы in14(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 0, 2, 9, 3, 8, 4, 7, 5, 6}; //порядок катодов in14
#else
enum {DOT_POS, ANODE_1_POS, ANODE_2_POS, ANODE_3_POS, ANODE_4_POS, ANODE_5_POS, ANODE_6_POS}; //порядок анодов ламп(точки всегда должны быть первыми)(только для прямого подключения к микроконтроллеру)
const uint8_t digitMask[] = {DIGIT_MASK}; //порядок пинов лампы(другие платы)
const uint8_t cathodeMask[] = {CATHODE_MASK}; //порядок катодов(другие платы)
#endif

//переменные работы с анимациями
struct animData {
#if LAMP_NUM > 4
  uint8_t flipSeconds; //флаги анимации секунд
#endif
  uint8_t flipBuffer[12]; //буфер анимации секунд
  uint16_t timeBright; //буфер времени для анимации яркости
} anim;

const uint8_t _anim_set[] PROGMEM = {FLIP_ANIM_RANDOM}; //массив случайных режимов

//перечисления неоновых точек
enum {
  DOT_NULL, //неоновые лампы выключены
  DOT_LEFT, //левая неоновая лампа
  DOT_RIGHT, //правая неоновая лампа
  DOT_ALL //две неоновые лампы
};

//перечисления кнопок
enum {
  KEY_NULL, //кнопка не нажата
  LEFT_KEY_PRESS, //клик левой кнопкой
  LEFT_KEY_HOLD, //удержание левой кнопки
  RIGHT_KEY_PRESS, //клик правой кнопкой
  RIGHT_KEY_HOLD, //удержание правой кнопки
  SET_KEY_PRESS, //клик средней кнопкой
  SET_KEY_HOLD, //удержание средней кнопки
  ADD_KEY_PRESS, //клик дополнительной кнопкой
  ADD_KEY_HOLD, //удержание дополнительной кнопки
#if IR_EXT_BTN_ENABLE
#if IR_PORT_ENABLE && RADIO_ENABLE
  PWR_KEY_PRESS, //клик кнопки питания
  VOL_UP_KEY_PRESS, //клик кнопки громкости вверх
  VOL_DOWN_KEY_PRESS, //клик кнопки громкости вниз
  STATION_UP_KEY_PRESS, //клик кнопки станции вверх
  STATION_DOWN_KEY_PRESS, //клик кнопки станции вниз
#endif
#endif
  KEY_MAX_ITEMS //максимум кнопок
};

struct Settings_5 {
  uint16_t irButtons[KEY_MAX_ITEMS - 1]; //коды кнопок пульта
  uint16_t timePeriod = US_PERIOD; //коррекция хода внутреннего осцилятора
  uint8_t min_pwm = DEFAULT_MIN_PWM; //минимальный шим
  uint8_t max_pwm = DEFAULT_MAX_PWM; //максимальный шим
  uint8_t light_zone[2][3]; //зоны яркости датчика освещения
  int8_t hvCorrect; //коррекция напряжения
  int8_t aging; //коррекция регистра старения
} debugSettings;

uint8_t pwm_coef; //коэффициент линейного регулирования
uint16_t hv_treshold = HV_ADC(5); //буфер сравнения напряжения

uint8_t adc_light; //значение АЦП сенсора яркости освещения
uint8_t state_light = 2; //состояние сенсора яркости освещения

const uint8_t decoderMask[] = {DECODER_1, DECODER_2, DECODER_3, DECODER_4}; //порядок пинов дешифратора(0, 1, 2, 3)
#if INDI_PORT_TYPE
const uint8_t regMask[] = {((NEON_DOT == 1) && INDI_DOT_TYPE) ? (0x01 << DOT_1_PIN) : ANODE_OFF, (0x01 << ANODE_1_PIN), (0x01 << ANODE_2_PIN), (0x01 << ANODE_3_PIN), (0x01 << ANODE_4_PIN), (0x01 << ANODE_5_PIN), (0x01 << ANODE_6_PIN)}; //таблица бит анодов ламп
#endif

uint8_t indi_dot_l; //буфер левых точек индикаторов
uint8_t indi_dot_r; //буфер правых точек индикаторов
volatile uint8_t indi_dot_pos = 0x01; //текущей номер точек индикаторов

uint8_t dot_dimm; //яркость секундной точки
uint8_t indi_buf[7]; //буфер индикаторов
uint8_t indi_dimm[7]; //яркость индикаторов
volatile uint8_t indiState; //текущей номер отрисовки индикатора

void indiSetBright(uint8_t pwm, uint8_t start = 0, uint8_t end = LAMP_NUM); //установка общей яркости
void indiPrintNum(uint16_t num, int8_t indi, uint8_t length = 0, char filler = ' '); //отрисовка чисел
void animPrintNum(uint16_t num, int8_t indi, uint8_t length = 0, char filler = ' '); //отрисовка чисел
#if NEON_DOT == 2
boolean dotDecBright(uint8_t _step, uint8_t _min, uint8_t _mode = DOT_ALL); //умегьшение яркости точек
boolean dotIncBright(uint8_t _step, uint8_t _max, uint8_t _mode = DOT_ALL); //увеличение яркости точек
#endif

//---------------------------Первичный запуск WDT-------------------------------
inline void startEnableWDT(void) //первичный запуск WDT
{
  cli(); //запрещаем прерывания
  RESET_WDT; //сбрасываем таймер WDT
  MCUSR &= ~(0x01 << WDRF); //сбросли флаг перезагрузки WDT
  WDTCSR = (0x01 << WDCE) | (0x01 << WDE); //разрешаем изменения WDT
  WDTCSR = (0x01 << WDE) | (0x01 << WDP3) | (0x01 << WDP0); //устанавливаем таймер WDT на 8сек
}
//---------------------------Основной запуск WDT--------------------------------
inline void mainEnableWDT(void) //основной запуск WDT
{
  RESET_WDT; //сбрасываем таймер WDT
  WDTCSR = (0x01 << WDCE) | (0x01 << WDE); //разрешаем изменения WDT
  WDTCSR = (0x01 << WDE) | (0x01 << WDP2) | (0x01 << WDP1) | (0x01 << WDP0); //устанавливаем таймер WDT на 2сек
}
//----------------------------------Динамическая индикация---------------------------------------
ISR(TIMER0_COMPA_vect) //динамическая индикация
{
#if INDI_PORT_TYPE
  uint8_t temp = (indi_buf[indiState] != INDI_NULL) ? regMask[indiState] : ANODE_OFF; //включаем индикатор если не пустой символ
#if DOTS_PORT_ENABLE == 2
#if (DOTS_TYPE == 1) || (DOTS_TYPE == 2)
  if (indi_dot_r & indi_dot_pos) temp |= (0x01 << DOTR_PIN); //включаем правые точки
#endif
#if DOTS_TYPE != 1
  if (indi_dot_l & indi_dot_pos) temp |= (0x01 << DOTL_PIN); //включаем левые точки
#endif
#endif

#if (NEON_DOT == 2) && INDI_DOT_TYPE
  if (!indiState) {
    if (indi_buf[indiState] & 0x80) temp |= (0x01 << DOT_1_PIN); //включили точки
    if (indi_buf[indiState] & 0x40) temp |= (0x01 << DOT_2_PIN); //включили точки
  }
#endif

  REG_LATCH_ENABLE; //включили защелку
  SPDR = temp; //загрузили данные
#if !INDI_DOT_TYPE
#if NEON_DOT == 1
  if (!indiState && (indi_buf[indiState] != INDI_NULL)) DOT_1_SET; //включили точки
#elif NEON_DOT == 2
  if (!indiState) {
    if (indi_buf[indiState] & 0x80) DOT_1_SET; //включили точки
    if (indi_buf[indiState] & 0x40) DOT_2_SET; //включили точки
  }
#endif
#endif
#endif

  OCR0B = indi_dimm[indiState]; //устанавливаем яркость индикатора
  PORTC = indi_buf[indiState]; //отправляем в дешефратор буфер индикатора

#if !INDI_PORT_TYPE
  if (indi_buf[indiState] != INDI_NULL) {
    switch (indiState) {
#if NEON_DOT == 1
      case DOT_POS: DOT_1_SET; break;
#elif NEON_DOT == 2
      case DOT_POS: if (indi_buf[indiState] & 0x80) DOT_1_SET; if (indi_buf[indiState] & 0x40) DOT_2_SET; break;
#endif
      case ANODE_1_POS: ANODE_SET(ANODE_1_PIN); break;
      case ANODE_2_POS: ANODE_SET(ANODE_2_PIN); break;
      case ANODE_3_POS: ANODE_SET(ANODE_3_PIN); break;
      case ANODE_4_POS: ANODE_SET(ANODE_4_PIN); break;
#if LAMP_NUM > 4
      case ANODE_5_POS: ANODE_SET(ANODE_5_PIN); break;
      case ANODE_6_POS: ANODE_SET(ANODE_6_PIN); break;
#endif
    }
  }
#endif

#if DOTS_PORT_ENABLE == 1
#if (DOTS_TYPE == 1) || (DOTS_TYPE == 2)
  if (indi_dot_r & indi_dot_pos) INDI_DOTR_ON; //включаем правые точки
#endif
#if DOTS_TYPE != 1
  if (indi_dot_l & indi_dot_pos) INDI_DOTL_ON; //включаем левые точки
#endif
#endif

  if (!++tick_ms) { //если превышено количество тиков
    SET_ERROR(TICK_OVF_ERROR); //устанавливаем ошибку переполнения тиков времени
    SET_ERROR(RESET_ERROR); //устанавливаем ошибку аварийной перезагрузки
    RESET_SYSTEM; //перезагрузка
  }
  if (!(SPH & 0xFC)) { //если стек переполнен
    SET_ERROR(STACK_OVF_ERROR); //устанавливаем ошибку переполнения стека
    SET_ERROR(RESET_ERROR); //устанавливаем ошибку аварийной перезагрузки
    RESET_SYSTEM; //перезагрузка
  }

#if INDI_PORT_TYPE
  while (!(SPSR & (0x01 << SPIF))); //ждем отправки
  REG_LATCH_DISABLE; //выключили защелку
#endif
}
ISR(TIMER0_COMPB_vect) {
#if INDI_PORT_TYPE
  REG_LATCH_ENABLE; //включили защелку
  SPDR = 0x00; //загрузили данные
#if (NEON_DOT == 1) && !INDI_DOT_TYPE
  if (!indiState) DOT_1_CLEAR; //выключили точки
#elif (NEON_DOT == 2) && !INDI_DOT_TYPE
  if (!indiState) {
    DOT_1_CLEAR; //выключили точки
    DOT_2_CLEAR; //выключили точки
  }
#endif
#else
  switch (indiState) {
#if NEON_DOT == 1
    case DOT_POS: DOT_1_CLEAR; break;
#elif NEON_DOT == 2
    case DOT_POS: DOT_1_CLEAR; DOT_2_CLEAR; break;
#endif
    case ANODE_1_POS: ANODE_CLEAR(ANODE_1_PIN); break;
    case ANODE_2_POS: ANODE_CLEAR(ANODE_2_PIN); break;
    case ANODE_3_POS: ANODE_CLEAR(ANODE_3_PIN); break;
    case ANODE_4_POS: ANODE_CLEAR(ANODE_4_PIN); break;
#if LAMP_NUM > 4
    case ANODE_5_POS: ANODE_CLEAR(ANODE_5_PIN); break;
    case ANODE_6_POS: ANODE_CLEAR(ANODE_6_PIN); break;
#endif
  }
#endif

#if DOTS_PORT_ENABLE
#if DOTS_PORT_ENABLE == 1
#if (DOTS_TYPE == 1) || (DOTS_TYPE == 2)
  INDI_DOTR_OFF; //выключаем правые точки
#endif
#if DOTS_TYPE != 1
  INDI_DOTL_OFF; //выключаем левые точки
#endif
#endif
  indi_dot_pos <<= 1; //сместили текущей номер точек индикаторов
#endif

  if (++indiState > LAMP_NUM) { //переходим к следующему индикатору
#if (NEON_DOT != 0) && (NEON_DOT != 3)
#if DOTS_PORT_ENABLE
    indi_dot_pos = 0x01; //сбросили текущей номер точек индикаторов
#endif
    indiState = 0; //сбросили позицию индикатора
#else
#if DOTS_PORT_ENABLE
    indi_dot_pos = 0x02; //сбросили текущей номер точек индикаторов
#endif
    indiState = 1; //сбросили позицию индикатора
#endif
  }

#if INDI_PORT_TYPE
  while (!(SPSR & (0x01 << SPIF))); //ждем отправки
  REG_LATCH_DISABLE; //выключили защелку
#endif
}
//-----------------------------------Динамическая подсветка---------------------------------------
#if (BACKL_TYPE == 2) && !IR_PORT_ENABLE
ISR(TIMER2_OVF_vect, ISR_NAKED) //прерывание подсветки
{
  __asm__ __volatile__ (
    "SBI %[_BACKL_PORT], %[_BACKL_BIT] \n\t" //HIGH на выход пина
    "RETI                              \n\t" //выход из прерывания
    :
    : [_BACKL_PORT]"I"(_SFR_IO_ADDR(BACKL_PORT)),
    [_BACKL_BIT]"I"(BACKL_BIT)
  );
}
ISR(TIMER2_COMPA_vect, ISR_NAKED) //прерывание подсветки
{
  __asm__ __volatile__ (
    "CBI %[_BACKL_PORT], %[_BACKL_BIT] \n\t" //LOW на выход пина
    "RETI                              \n\t" //выход из прерывания
    :
    : [_BACKL_PORT]"I"(_SFR_IO_ADDR(BACKL_PORT)),
    [_BACKL_BIT]"I"(BACKL_BIT)
  );
}
#endif

//------------------------Проверка состояния динамической индикации-------------------------------
void indiStateCheck(void) //проверка состояния динамической индикации
{
  if (TIMSK0 != (0x01 << OCIE0B | 0x01 << OCIE0A)) { //если настройка изменилась
    TIMSK0 = (0x01 << OCIE0B | 0x01 << OCIE0A); //установили настройку
    SET_ERROR(INDI_ERROR); //устанавливаем ошибку сбоя работы динамической индикации
  }
  if (OCR0B > LIGHT_MAX) { //если вышли за предел
    OCR0B = LIGHT_MAX; //установили максимум
    SET_ERROR(INDI_ERROR); //устанавливаем ошибку сбоя работы динамической индикации
  }
}
//------------------------Проверка состояния динамической индикации-------------------------------
void indiCheck(void) //проверка состояния динамической индикации
{
  if (TCCR0A != (0x01 << WGM01)) { //если настройка изменилась
    TCCR0A = (0x01 << WGM01); //установили настройку
    SET_ERROR(INDI_ERROR); //устанавливаем ошибку сбоя работы динамической индикации
  }
  if (TCCR0B != (0x01 << CS02)) { //если настройка изменилась
    TCCR0B = (0x01 << CS02); //установили настройку
    SET_ERROR(INDI_ERROR); //устанавливаем ошибку сбоя работы динамической индикации
  }
  if (OCR0A != FREQ_TICK) { //если вышли за предел
    OCR0A = FREQ_TICK; //установили максимум
    SET_ERROR(INDI_ERROR); //устанавливаем ошибку сбоя работы динамической индикации
  }
}
//----------------------------Проверка состояния преобразователя----------------------------------
void converterCheck(void) //проверка состояния преобразователя
{
#if GEN_FEEDBACK
  if (((TCCR1A & 0x4F) != (0x01 << WGM10)) ||
#else
#if CONV_PIN == 9
  if (((TCCR1A & 0xCF) != ((0x01 << WGM10) | (0x01 << COM1A1))) ||
#elif CONV_PIN == 10
  if (((TCCR1A & 0x3F) != ((0x01 << WGM10) | (0x01 << COM1B1))) ||
#endif
#endif
#if GEN_SPEED_X2
      (TCCR1B != ((0x01 << CS10) | (0x01 << WGM12))))
#else
      (TCCR1B != (0x01 << CS10)))
#endif
  {
    TCCR1A = TCCR1B = 0; //выключаем шим
    CONV_DISABLE; //выключаем преобразователь
    SET_ERROR(CONVERTER_ERROR); //устанавливаем ошибку сбоя работы преобразователя
    SET_ERROR(RESET_ERROR); //устанавливаем ошибку аварийной перезагрузки
    RESET_SYSTEM; //перезагрузка
  }
#if CONV_PIN == 9
  if (OCR1A > 200) { //если вышли за предел
    OCR1A = 200; //установили максимум
    SET_ERROR(PWM_OVF_ERROR); //устанавливаем ошибку переполнения заполнения шим преобразователя
  }
#elif CONV_PIN == 10
  if (OCR1B > 200) { //если вышли за предел
    OCR1B = 200; //установили максимум
    SET_ERROR(PWM_OVF_ERROR); //устанавливаем ошибку переполнения заполнения шим преобразователя
  }
#endif
}
//------------------------Обновление коэффициента линейного регулирования-------------------------
void indiChangeCoef(void) //обновление коэффициента линейного регулирования
{
  if (debugSettings.max_pwm < (debugSettings.min_pwm + 10)) debugSettings.min_pwm = debugSettings.max_pwm - 10;
  pwm_coef = 255 / (uint8_t)(((uint16_t)LIGHT_MAX * LAMP_NUM) / CONSTRAIN(debugSettings.max_pwm - debugSettings.min_pwm, 10, 100));
}
//---------------------Установка нового значения шим линейного регулирования----------------------
void indiChangePwm(void) //установка нового значения шим линейного регулирования
{
  uint16_t dimm_all = 0;
  for (uint8_t i = 1; i < (LAMP_NUM + 1); i++) if (indi_buf[i] != INDI_NULL) dimm_all += indi_dimm[i];
#if CONV_PIN == 9
  OCR1A = CONSTRAIN(debugSettings.min_pwm + (uint8_t)((dimm_all * pwm_coef) >> 8), 100, 200);
#elif CONV_PIN == 10
  OCR1B = CONSTRAIN(debugSettings.min_pwm + (uint8_t)((dimm_all * pwm_coef) >> 8), 100, 200);
#endif
}
//----------------------------Инициализация портов индикаторов------------------------------------
void indiPortInit(void) //инициализация портов индикаторов
{
  PORTC |= 0x0F; //устанавливаем высокие уровни на катоды
  DDRC |= 0x0F; //устанавливаем катоды как выходы

#if (NEON_DOT != 3) && !INDI_DOT_TYPE
  DOT_1_INIT; //инициализация секундных точек
#endif
#if (NEON_DOT == 2) && !INDI_DOT_TYPE
  DOT_2_INIT; //инициализация секундных точек
#endif
#if DOTS_PORT_ENABLE == 1
#if (DOTS_TYPE == 1) || (DOTS_TYPE == 2)
  INDI_DOTR_INIT; //инициализация правых разделительных точек в индикаторах
#endif
#if DOTS_TYPE != 1
  INDI_DOTL_INIT; //инициализация левых разделительных точек в индикаторах
#endif
#endif
#if !INDI_PORT_TYPE
  ANODE_INIT(ANODE_1_PIN); //инициализация анода 1
  ANODE_INIT(ANODE_2_PIN); //инициализация анода 2
  ANODE_INIT(ANODE_3_PIN); //инициализация анода 3
  ANODE_INIT(ANODE_4_PIN); //инициализация анода 4
#if LAMP_NUM > 4
  ANODE_INIT(ANODE_5_PIN); //инициализация анода 5
  ANODE_INIT(ANODE_6_PIN); //инициализация анода 6
#endif
#else
  REG_LATCH_INIT; //инициализация защелки сдвигового регистра
  REG_DATA_INIT; //инициализация линии ланных сдвигового регистра
  REG_SCK_INIT; //инициализация линии тактирования сдвигового регистра
#endif
}
//---------------------------------Инициализация индикации----------------------------------------
void indiInit(void) //инициализация индикации
{
#if INDI_PORT_TYPE
  DDRB |= 0x04; //установили D10 как выход

  SPSR = (0x01 << SPI2X); //включили удвоение скорости
  SPCR = (0x01 << SPE) | (0x01 << MSTR); //запустили SPI в режиме ведущего
#endif

  for (uint8_t i = 0; i < (LAMP_NUM + 1); i++) { //инициализируем буферы
    indi_buf[i] = INDI_NULL; //очищаем буфер пустыми символами
    indi_dimm[i] = (LIGHT_MAX - 1); //устанавливаем максимальную яркость
  }

  OCR0A = FREQ_TICK; //максимальная частота
  OCR0B = (LIGHT_MAX - 1); //максимальная яркость

  TIMSK0 = 0; //отключаем прерывания Таймера0
  TCCR0A = (0x01 << WGM01); //режим CTC
  TCCR0B = (0x01 << CS02); //пределитель 256

#if PLAYER_TYPE == 2
#if BUZZ_PIN == 9
  OCR1A = 128; //выключаем dac
#elif BUZZ_PIN == 10
  OCR1B = 128; //выключаем dac
#endif
#endif
#if !NEON_DOT
#if DOT_1_PIN == 9
  OCR1A = 0; //выключаем точки
#elif DOT_1_PIN == 10
  OCR1B = 0; //выключаем точки
#endif
#endif

  TIMSK1 = 0; //отключаем прерывания Таймера1
  TCCR1A = (0x01 << WGM10); //режим коррекции фазы шим
#if GEN_SPEED_X2
  TCCR1B = (0x01 << CS10) | (0x01 << WGM12);  //задаем частоту 62 кГц
#else
  TCCR1B = (0x01 << CS10);  //задаем частоту 31 кГц
#endif

#if PLAYER_TYPE == 2
#if BUZZ_PIN == 9
  TCCR1A |= (0x01 << COM1A1); //подключаем D9
#elif BUZZ_PIN == 10
  TCCR1A |= (0x01 << COM1B1); //подключаем D10
#endif
#endif
#if !NEON_DOT
#if DOT_1_PIN == 9
  TCCR1A |= (0x01 << COM1A1); //подключаем D9
#elif DOT_1_PIN == 10
  TCCR1A |= (0x01 << COM1B1); //подключаем D10
#endif
#endif

#if GEN_ENABLE
#if CONV_PIN == 9
  OCR1A = CONSTRAIN(debugSettings.min_pwm, 100, 200); //устанавливаем первичное значение шим
  TCCR1A |= (0x01 << COM1A1); //подключаем D9
#elif CONV_PIN == 10
  OCR1B = CONSTRAIN(debugSettings.min_pwm, 100, 200); //устанавливаем первичное значение шим
  TCCR1A |= (0x01 << COM1B1); //подключаем D10
#endif
#endif

  OCR2A = 0; //выключаем подсветку
  OCR2B = 0; //сбравсывем бузер

  TIMSK2 = 0; //выключаем прерывания Таймера2
#if (BACKL_TYPE != 1)
  TCCR2A = 0; //отключаем OCR2A и OCR2B
#else
  TCCR2A = (0x01 << COM2A1 | 0x01 << WGM20 | 0x01 << WGM21); //подключаем D11
#endif
  TCCR2B = (0x01 << CS21); //пределитель 8

  TCNT0 = FREQ_TICK; //установили счетчик в начало
  TIFR0 |= (0x01 << OCF0B) | (0x01 << OCF0A); //сбрасываем флаги прерывания
  TIMSK0 = (0x01 << OCIE0B | 0x01 << OCIE0A); //разрешаем прерывания

  sei(); //разрешаем прерывания глобально
}
//-------------------------Очистка индикаторов----------------------------------------------------
void indiClr(void) //очистка индикаторов
{
  for (uint8_t cnt = 0; cnt < LAMP_NUM; cnt++) indi_buf[cnt + 1] = INDI_NULL;
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
//-------------------------Очистка индикатора----------------------------------------------------
void indiClr(uint8_t indi) //очистка индикатора
{
  indi_buf[indi + 1] = INDI_NULL;
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
//-------------------------Установка индикатора----------------------------------------------------
void indiSet(uint8_t buf, uint8_t indi) //установка индикатора
{
  indi_buf[indi + 1] = buf; //устанавливаем в ячейку буфера
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
//-------------------------Получить состояние индикатора----------------------------------------------------
uint8_t indiGet(uint8_t indi) //получить состояние индикатора
{
  return indi_buf[indi + 1]; //возвращаем содержимое ячейки буфера
}
//------------------------------Установка яркости индикатора--------------------------------------
void indiSetBright(uint8_t pwm, uint8_t indi) //установка яркости индикатора
{
  if (pwm > 30) pwm = 30;
  indi_dimm[indi + 1] = (uint8_t)((INDI_LIGHT_MAX * pwm) >> 8);
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
//--------------------------------Установка общей яркости-----------------------------------------
void indiSetBright(uint8_t pwm, uint8_t start, uint8_t end) //установка общей яркости
{
  if (pwm > 30) pwm = 30;
  pwm = (uint8_t)((INDI_LIGHT_MAX * pwm) >> 8);
  for (uint8_t i = start; i < end; i++) indi_dimm[i + 1] = pwm;
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
//-------------------------Установка левой разделительной точки------------------------------------
void indiSetDotL(uint8_t dot) //установка левой разделительной точки
{
  if (dot < DOTS_NUM) indi_dot_l |= (0x02 << dot);
}
//--------------------------Очистка левой разделительной точки-------------------------------------
void indiClrDotL(uint8_t dot) //очистка левой разделительной точки
{
  if (dot < DOTS_NUM) indi_dot_l &= ~(0x02 << dot);
}
//-------------------------Установка правой разделительной точки-----------------------------------
void indiSetDotR(uint8_t dot) //установка правой разделительной точки
{
  if (dot < DOTS_NUM) indi_dot_r |= (0x02 << dot);
}
//--------------------------Очистка правой разделительной точки------------------------------------
void indiClrDotR(uint8_t dot) //очистка разделительной точки
{
  if (dot < DOTS_NUM) indi_dot_r &= ~(0x02 << dot);
}
//-------------------------Установка разделительных точек-----------------------------------
void indiSetDots(int8_t _dot, uint8_t _num) //установка разделительных точек
{
  while (_num) { //пока есть точки
    _num--; //убавили число точек
    if ((uint8_t)_dot < DOTS_ALL) { //если число в поле индикатора
#if DOTS_TYPE == 2
      if (_dot & 0x01) indiSetDotR(_dot >> 1); //включаем правую точку
      else indiSetDotL(_dot >> 1); //включаем левую точку
#elif DOTS_TYPE == 1
      indiSetDotR(_dot); //включаем правую точку
#else
      indiSetDotL(_dot); //включаем левую точку
#endif
    }
    _dot++; //прибавили текущую позицию точек
  }
}
//-------------------------Установка разделительных точек-----------------------------------
void indiSetDotsMain(uint8_t _dot) //установка разделительных точек
{
#if DOTS_TYPE == 2
#if LAMP_NUM > 4
#if DOTS_MAIN == 0
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotL(2); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotL(4); //установка разделительной точки
#elif DOTS_MAIN == 1
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotR(1); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotR(3); //установка разделительной точки
#elif DOTS_MAIN == 2
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotL(2); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotR(3); //установка разделительной точки
#elif DOTS_MAIN == 3
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotR(1); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotL(4); //установка разделительной точки
#else
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotL(2); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotL(4); //установка разделительной точки
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotR(1); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotR(3); //установка разделительной точки
#endif
#else
#if DOTS_MAIN == 0
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotL(2); //установка разделительной точки
#elif DOTS_MAIN == 1
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotR(1); //установка разделительной точки
#else
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotR(1); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotL(2); //установка разделительной точки
#endif
#endif
#elif DOTS_NUM > 4
#if DOTS_TYPE == 1
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotR(1); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotR(3); //установка разделительной точки
#else
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotL(2); //установка разделительной точки
  if ((_dot == DOT_RIGHT) || (_dot == DOT_ALL)) indiSetDotL(4); //установка разделительной точки
#endif
#else
#if DOTS_TYPE == 1
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotR(1); //установка разделительной точки
#else
  if ((_dot == DOT_LEFT) || (_dot == DOT_ALL)) indiSetDotL(2); //установка разделительной точки
#endif
#endif
}
//-----------------------------Очистка разделительных точек----------------------------------------
void indiClrDots(void) //очистка разделительных точек
{
#if DOTS_TYPE == 2
  indi_dot_l = 0x00; //выключаем левые точки
  indi_dot_r = 0x00; //выключаем правые точки
#elif DOTS_TYPE == 1
  indi_dot_r = 0x00; //выключаем правые точки
#else
  indi_dot_l = 0x00; //выключаем левые точки
#endif
}
//------------------------------------Печать чисел-------------------------------------------------
void printNum(uint16_t _num, uint8_t* _out, int8_t _indi, uint8_t _length, char _filler) //печать чисел
{
  uint8_t buff[6]; //временный буфер
  uint8_t count = 0; //счетчик символов

  if (!_num) { //если ноль
    buff[0] = digitMask[0]; //устанавливаем ноль
    count = 1; //прибавляем счетчик
  }
  else { //иначе заполняем буфер числами
    while (_num && (count < 6)) { //если есть число
      buff[count++] = digitMask[_num % 10]; //забираем младший разряд в буфер
      _num /= 10; //отнимаем младший разряд от числа
    }
  }

  while ((_length > count) && (count < 6)) buff[count++] = digitMask[(_filler != ' ') ? _filler : 10]; //заполняем символами заполнителями

  while (count) { //расшивровка символов
    count--; //убавили счетчик символов
    if ((uint8_t)_indi++ < LAMP_NUM) { //если число в поле индикатора
      _out[_indi] = buff[count]; //устанавливаем новое число
    }
  }
}
//------------------------------------Вывод чисел-------------------------------------------------
void indiPrintNum(uint16_t _num, int8_t _indi, uint8_t _length, char _filler) //вывод чисел
{
  printNum(_num, indi_buf, _indi, _length, _filler); //печать чисел
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
//-----------------------------Декодирование чисел индикации--------------------------------------
uint8_t animDecodeNum(uint8_t _num) //декодирование чисел индикации
{
  for (uint8_t mask = 0; mask < 11; mask++) if (_num == digitMask[mask]) return mask;
  return 10;
}
//-----------------------------Запись чисел в буфер анимации----------------------------------------
void animPrintNum(uint16_t _num, int8_t _indi, uint8_t _length, char _filler) //запись чисел в буфер анимации
{
  printNum(_num, (anim.flipBuffer + 5), _indi, _length, _filler); //печать чисел
}
//-------------------------------Отрисовка буфера анимации-----------------------------------------
void animPrintBuff(int8_t _indi, uint8_t _step, uint8_t _max) //отрисовка буфера анимации
{
  for (uint8_t i = 0; i < _max; i++) {
    if ((uint8_t)_indi < LAMP_NUM) { //если число в поле индикатора
      indi_buf[_indi + 1] = anim.flipBuffer[i + _step]; //устанавливаем новое число
    }
    _indi++;
  }
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
#endif
}
//--------------------------------Очистка буфера анимации-------------------------------------------
void animClearBuff(void) //очистка буфера анимации
{
  for (uint8_t i = 6; i < (LAMP_NUM + 6); i++) anim.flipBuffer[i] = INDI_NULL;
}
//---------------------------------Анимация смены яркости цифр---------------------------------------
void animBright(uint8_t pwm) //анимация смены яркости цифр
{
  if (pwm > 30) pwm = 30;
  pwm = (uint8_t)((INDI_LIGHT_MAX * pwm) >> 8);
  for (uint8_t i = 0; i < LAMP_NUM; i++) {
    if (anim.flipBuffer[i] != anim.flipBuffer[i + 6]) { //если не достигли конца анимации разряда
      indi_dimm[i + 1] = pwm;
    }
  }
#if GEN_ENABLE
  indiChangePwm(); //установка нового значения шим линейного регулирования
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
#if (BACKL_TYPE == 2) && !IR_PORT_ENABLE
  if (pwm) TIMSK2 |= (0x01 << OCIE2A | 0x01 << TOIE2); //включаем таймер
  else {
    TIMSK2 &= ~(0x01 << OCIE2A | 0x01 << TOIE2); //выключаем таймер
    BACKL_CLEAR; //выключили подсветку
  }
#elif BACKL_TYPE == 1
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
#if DOT_1_PIN == 9
  return OCR1A;
#elif DOT_1_PIN == 10
  return OCR1B;
#endif
#endif
}
//-----------------------------------Установка неоновых точек------------------------------------
void neonDotSet(uint8_t _dot) //установка неоновых точек
{
  indi_buf[0] = INDI_NULL; //запрещаем включать точки
  switch (_dot) {
    case DOT_LEFT: indi_buf[0] |= 0x80; break; //включаем левую точку
    case DOT_RIGHT: indi_buf[0] |= 0x40; break; //включаем правую точку
    case DOT_ALL: indi_buf[0] |= 0xC0; break; //включаем обе точки
  }
}
//------------------------------Установка яркости неоновых точек---------------------------------
void neonDotSetBright(uint8_t _pwm) //установка яркости неоновых точек
{
  if (_pwm > 250) _pwm = 250;
  dot_dimm = _pwm;
  indi_dimm[0] = (uint8_t)((DOT_LIGHT_MAX * _pwm) >> 8); //устанавливаем яркость точек
}
//---------------------------------Установка яркости точек---------------------------------------
void dotSetBright(uint8_t _pwm) //установка яркости точек
{
#if (NEON_DOT == 3) && DOTS_PORT_ENABLE
  if (_pwm) indiSetDotsMain(DOT_ALL); //установка разделительных точек
  else indiClrDots(); //очистка разделительных точек
#elif NEON_DOT == 2
  neonDotSetBright(_pwm); //установка яркости неоновых точек
  neonDotSet((_pwm) ? DOT_ALL : DOT_NULL); //установка неоновых точек
#elif NEON_DOT == 1
  neonDotSetBright(_pwm); //установка яркости неоновых точек
  if (_pwm) indi_buf[0] = (INDI_NULL | 0x80); //разрешаем включать точки
  else indi_buf[0] = INDI_NULL; //запрещаем включать точки
#elif NEON_DOT == 0
#if DOT_1_PIN == 9
  OCR1A = _pwm; //устанавливаем яркость точек
  if (_pwm) TCCR1A |= (0x01 << COM1A1); //подключаем D9
  else {
    TCCR1A &= ~(0x01 << COM1A1); //отключаем D9
    DOT_1_CLEAR; //выключили точки
  }
#elif DOT_1_PIN == 10
  OCR1B = _pwm; //устанавливаем яркость точек
  if (_pwm) TCCR1A |= (0x01 << COM1B1); //подключаем D10
  else {
    TCCR1A &= ~(0x01 << COM1B1); //отключаем D10
    DOT_1_CLEAR; //выключили точки
  }
#endif
#endif
}
#if NEON_DOT != 2
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
#else
//--------------------------------Уменьшение яркости точек----------------------------------------
boolean dotDecBright(uint8_t _step, uint8_t _min, uint8_t _mode)
{
  if (((int16_t)dotGetBright() - _step) > _min) {
    _min = dotGetBright() - _step;
    neonDotSetBright(_min); //установка яркости неоновых точек
    neonDotSet((_min) ? _mode : 0); //установка неоновых точек
  }
  else {
    neonDotSetBright(_min); //установка яркости неоновых точек
    neonDotSet((_min) ? _mode : 0); //установка неоновых точек
    return 1;
  }
  return 0;
}
//--------------------------------Увеличение яркости точек----------------------------------------
boolean dotIncBright(uint8_t _step, uint8_t _max, uint8_t _mode)
{
  if (((uint16_t)dotGetBright() + _step) < _max) {
    _max = dotGetBright() + _step;
    neonDotSetBright(_max); //установка яркости неоновых точек
    neonDotSet((_max) ? _mode : 0); //установка неоновых точек
  }
  else {
    neonDotSetBright(_max); //установка яркости неоновых точек
    neonDotSet((_max) ? _mode : 0); //установка неоновых точек
    return 1;
  }
  return 0;
}
#endif
