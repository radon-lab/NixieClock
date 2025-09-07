#define FREQ_TICK (uint8_t)(CONSTRAIN((1000.0 / ((uint16_t)INDI_FREQ_ADG * (LAMP_NUM + (boolean)((NEON_DOT == 1) || (NEON_DOT == 2) || INDI_SYMB_TYPE)))) / 0.016, 125, 255)) //расчет переполнения таймера динамической индикации

#define US_PERIOD (uint16_t)(((uint16_t)FREQ_TICK + 1) * 16.0) //период тика таймера в мкс
#define US_PERIOD_MIN (uint16_t)(US_PERIOD - (US_PERIOD % 100) - 400) //минимальный период тика таймера
#define US_PERIOD_MAX (uint16_t)(US_PERIOD - (US_PERIOD % 100) + 400) //максимальный период тика таймера

#define MS_PERIOD (uint8_t)(US_PERIOD / 1000) //период тика таймера в целых мс

#define LIGHT_MAX (uint8_t)(FREQ_TICK - INDI_DEAD_TIME) //расчет максимального шага яркости
#define DOT_LIGHT_MAX (uint8_t)(CONSTRAIN(((uint16_t)LIGHT_MAX - 2) + (LIGHT_MAX >> 5), 100, 255)) //расчет максимального шага яркости для точек
#define INDI_LIGHT_MAX (uint16_t)(((uint16_t)LIGHT_MAX * 8) + (LIGHT_MAX >> 1)) //расчет максимального шага яркости для индикаторов

#define _BIT(value, bit) (((value) >> (bit)) & 0x01)
#if WIRE_PULL && !ESP_ENABLE
#define ID(digit) ((_BIT(digit, 0) << DECODER_1) | (_BIT(digit, 1) << DECODER_2) | (_BIT(digit, 2) << DECODER_3) | (_BIT(digit, 3) << DECODER_4) | 0x30)
#define INDI_NULL ((0x01 << DECODER_2) | (0x01 << DECODER_4) | 0x30) //пустой символ(отключеный индикатор)
#else
#define ID(digit) ((_BIT(digit, 0) << DECODER_1) | (_BIT(digit, 1) << DECODER_2) | (_BIT(digit, 2) << DECODER_3) | (_BIT(digit, 3) << DECODER_4))
#define INDI_NULL ((0x01 << DECODER_2) | (0x01 << DECODER_4)) //пустой символ(отключеный индикатор)
#endif

//Типы плат часов
#if (BOARD_TYPE == 0) //IN-12 (индикаторы стоят правильно)
enum {INDI_POS, ANODE_1_POS, ANODE_2_POS, ANODE_3_POS, ANODE_4_POS}; //порядок анодов ламп(точки всегда должны быть первыми)(только для прямого подключения к микроконтроллеру)
const uint8_t digitMask[] = {ID(7), ID(3), ID(6), ID(4), ID(1), ID(9), ID(8), ID(0), ID(5), ID(2), ID(10)}; //маска дешифратора платы in12 (цифры нормальные)(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 6, 2, 7, 5, 0, 4, 9, 8, 3}; //порядок катодов in12
#elif (BOARD_TYPE == 1) //IN-12 turned (индикаторы перевёрнуты)
enum {INDI_POS, ANODE_4_POS, ANODE_3_POS, ANODE_2_POS, ANODE_1_POS}; //порядок анодов ламп(точки всегда должны быть первыми)(только для прямого подключения к микроконтроллеру)
const uint8_t digitMask[] = {ID(2), ID(8), ID(1), ID(9), ID(6), ID(4), ID(3), ID(5), ID(0), ID(7), ID(10)}; //маска дешифратора платы in12 turned (цифры вверх ногами)(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 6, 2, 7, 5, 0, 4, 9, 8, 3}; //порядок катодов in12
#elif (BOARD_TYPE == 2) //IN-14 (обычная и neon dot)
enum {INDI_POS, ANODE_4_POS, ANODE_3_POS, ANODE_2_POS, ANODE_1_POS}; //порядок анодов ламп(точки всегда должны быть первыми)(только для прямого подключения к микроконтроллеру)
const uint8_t digitMask[] = {ID(9), ID(8), ID(0), ID(5), ID(4), ID(7), ID(3), ID(6), ID(2), ID(1), ID(10)}; //маска дешифратора платы in14(цифра "10" - это пустой символ, должен быть всегда в конце)
const uint8_t cathodeMask[] = {1, 0, 2, 9, 3, 8, 4, 7, 5, 6}; //порядок катодов in14
#else
enum {INDI_POS, ANODE_1_POS, ANODE_2_POS, ANODE_3_POS, ANODE_4_POS, ANODE_5_POS, ANODE_6_POS}; //порядок анодов ламп(точки всегда должны быть первыми)(только для прямого подключения к микроконтроллеру)
#if INDI_PORT_TYPE
const uint8_t regMask[] = {((NEON_DOT == 1) && INDI_DOT_TYPE) ? (0x01 << DOT_1_PIN) : ((INDI_SYMB_TYPE == 2) ? (0x01 << ANODE_0_PIN) : ANODE_OFF), (0x01 << ANODE_1_PIN), (0x01 << ANODE_2_PIN), (0x01 << ANODE_3_PIN), (0x01 << ANODE_4_PIN), (0x01 << ANODE_5_PIN), (0x01 << ANODE_6_PIN)}; //таблица бит анодов ламп
#endif
const uint8_t digitMask[] = {DIGIT_MASK}; //порядок пинов лампы(другие платы)
const uint8_t cathodeMask[] = {CATHODE_MASK}; //порядок катодов(другие платы)
#endif

#include "CORE.h"

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

  REG_LATCH_ENABLE; //открыли защелку
  SPDR = temp; //загрузили данные
#endif

  OCR0B = indi_dimm[indiState]; //устанавливаем яркость индикатора
  PORTC = indi_buf[indiState]; //отправляем в дешефратор буфер индикатора

#if INDI_PORT_TYPE || (INDI_SYMB_TYPE == 1) || (!INDI_DOT_TYPE && !INDI_SYMB_TYPE)
  if (indi_buf[indiState] != INDI_NULL) {
    switch (indiState) {
#if (INDI_SYMB_TYPE == 1)
      case INDI_POS: ANODE_SET(ANODE_0_PIN); break;
#elif !INDI_DOT_TYPE && !INDI_SYMB_TYPE
#if NEON_DOT == 1
      case INDI_POS: DOT_1_SET; break;
#elif NEON_DOT == 2
      case INDI_POS: if (indi_buf[indiState] & 0x80) DOT_1_SET; if (indi_buf[indiState] & 0x40) DOT_2_SET; break;
#endif
#endif
#if !INDI_PORT_TYPE
      case ANODE_1_POS: ANODE_SET(ANODE_1_PIN); break;
      case ANODE_2_POS: ANODE_SET(ANODE_2_PIN); break;
      case ANODE_3_POS: ANODE_SET(ANODE_3_PIN); break;
      case ANODE_4_POS: ANODE_SET(ANODE_4_PIN); break;
#if LAMP_NUM > 4
      case ANODE_5_POS: ANODE_SET(ANODE_5_PIN); break;
      case ANODE_6_POS: ANODE_SET(ANODE_6_PIN); break;
#endif
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
  REG_LATCH_DISABLE; //закрыли защелку
#endif
}
ISR(TIMER0_COMPB_vect) {
#if INDI_PORT_TYPE
  REG_LATCH_ENABLE; //открыли защелку
  SPDR = 0x00; //загрузили данные
#if (INDI_SYMB_TYPE == 1)
  if (!indiState) ANODE_CLEAR(ANODE_0_PIN); //выключили символ
#elif !INDI_DOT_TYPE && !INDI_SYMB_TYPE
#if (NEON_DOT == 1)
  if (!indiState) DOT_1_CLEAR; //выключили точки
#elif (NEON_DOT == 2)
  if (!indiState) {
    DOT_1_CLEAR; //выключили точки
    DOT_2_CLEAR; //выключили точки
  }
#endif
#endif
#else
  switch (indiState) {
#if (INDI_SYMB_TYPE == 1)
    case INDI_POS: ANODE_CLEAR(ANODE_0_PIN); break;
#elif !INDI_SYMB_TYPE
#if NEON_DOT == 1
    case INDI_POS: DOT_1_CLEAR; break;
#elif NEON_DOT == 2
    case INDI_POS: DOT_1_CLEAR; DOT_2_CLEAR; break;
#endif
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
#if (NEON_DOT == 1) || (NEON_DOT == 2) || INDI_SYMB_TYPE
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
  REG_LATCH_DISABLE; //закрыли защелку
#endif
}
//------------------------Проверка состояния динамической индикации-------------------------------
void indiStateCheck(void) //проверка состояния динамической индикации
{
  if (TIMSK0 != ((0x01 << OCIE0B) | (0x01 << OCIE0A))) { //если настройка изменилась
    TIMSK0 = (0x01 << OCIE0B) | (0x01 << OCIE0A); //установили настройку
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
//----------------------------Инициализация портов индикации------------------------------------
void indiPortInit(void) //инициализация портов индикации
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
#if (INDI_SYMB_TYPE == 1)
  ANODE_INIT(ANODE_0_PIN); //инициализация анода 0
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
  REG_DATA_INIT; //инициализация линии данных сдвигового регистра
  REG_SCK_INIT; //инициализация линии тактирования сдвигового регистра
  for (uint8_t i = 0; i < 16; i++) REG_SCK_INV; //очищаем сдвиговый регистр
  REG_LATCH_DISABLE; //закрываем защелку
#endif
}
//----------------------------Инициализация индикации------------------------------------
void indiInit(void) //инициализация индикации
{
  cli(); //запрещаем прерывания глобально

  for (uint8_t i = 0; i < (LAMP_NUM + 1); i++) { //инициализируем буферы
    indi_buf[i] = INDI_NULL; //очищаем буфер пустыми символами
    indi_dimm[i] = (LIGHT_MAX - 1); //устанавливаем максимальную яркость
  }

  OCR0A = FREQ_TICK; //максимальная частота
  OCR0B = (LIGHT_MAX - 1); //максимальная яркость

  TIMSK0 = 0; //отключаем прерывания
  TCCR0A = (0x01 << WGM01); //режим CTC
  TCCR0B = (0x01 << CS02); //пределитель 256

  TCNT0 = FREQ_TICK; //установили счетчик в начало
  TIFR0 |= (0x01 << OCF0B) | (0x01 << OCF0A); //сбрасываем флаги прерывания
  TIMSK0 = (0x01 << OCIE0B) | (0x01 << OCIE0A); //разрешаем прерывания

  sei(); //разрешаем прерывания глобально
}
//-----------------------------Декодирование чисел индикации--------------------------------------
uint8_t indiDecodeNum(uint8_t _num) //декодирование чисел индикации
{
  for (uint8_t mask = 0; mask < 11; mask++) if (_num == digitMask[mask]) return mask;
  return 10;
}
//---------------------------------Вывод чисел индикации------------------------------------------
void printNum(uint16_t _num, uint8_t* _out, int8_t _indi, uint8_t _length, uint8_t _filler) //вывод чисел индикации
{
  uint8_t buff[6]; //временный буфер
  uint8_t count = 0; //счетчик символов

  if (_filler > 9) _filler = 10;

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

  while ((_length > count) && (count < 6)) buff[count++] = digitMask[_filler]; //заполняем символами заполнителями

  if (_length && (_length < count)) _indi -= count - _length; //смещаем буфер если число длиннее

  while (count) { //расшивровка символов
    count--; //убавили счетчик символов
    if ((uint8_t)_indi++ < LAMP_NUM) { //если число в поле индикатора
      _out[_indi] = buff[count]; //устанавливаем новое число
    }
  }
}
