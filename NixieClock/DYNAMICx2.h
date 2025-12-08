#define TIME_TICK 16 //время одной единицы шага таймера(мкс)
#define FREQ_TICK (uint8_t)(CONSTRAIN(1e6 / TIME_TICK / ((uint16_t)INDI_FREQ_ADG * ((LAMP_NUM / 2) + (boolean)((SECS_DOT == 1) || (SECS_DOT == 2) || INDI_SYMB_TYPE))), 60 + INDI_DEAD_TIME, 255)) //расчет переполнения таймера динамической индикации

#include "IO.h"
#include "CORE.h"

#define _BIT(value, bit) (((value) >> (bit)) & 0x01)
#define ID(digit) ((_BIT(digit, 0) << DECODER_1) | (_BIT(digit, 1) << DECODER_2) | (_BIT(digit, 2) << DECODER_3) | (_BIT(digit, 3) << DECODER_4))
#define INDI_NULL ((0x01 << DECODER_2) | (0x01 << DECODER_4)) //пустой символ(отключеный индикатор)

#define LIGHT_MAX (uint8_t)(FREQ_TICK - INDI_DEAD_TIME) //расчет максимального шага яркости
#define DOT_LIGHT_MAX (uint8_t)(CONSTRAIN_MAX(((uint16_t)FREQ_TICK - 2) + (FREQ_TICK >> 5), 255)) //расчет максимального шага яркости для точек
#define INDI_LIGHT_MAX (uint16_t)(((uint16_t)LIGHT_MAX * 8) + (LIGHT_MAX >> 1)) //расчет максимального шага яркости для индикаторов

const uint8_t digitMask[] = {DIGIT_MASK}; //порядок пинов лампы
const uint8_t cathodeMask[] = {CATHODE_MASK}; //порядок катодов

enum {INDI_0_POS, INDI_1_POS, INDI_2_POS, INDI_3_POS}; //порядок индикации ламп

#define TIMER_START (255 - FREQ_TICK) //начальное значение таймера
#define LAMP_MAX_STEP (LAMP_NUM / 2) //количиство ламп в группе

const uint8_t indi_dots_l_pin[2] = {DOTSL_PIN};
#define INDI_DOTSL_0 (indi_dots_l_pin[0])
#define INDI_DOTSL_1 (indi_dots_l_pin[1])

const uint8_t indi_dots_r_pin[2] = {DOTSR_PIN};
#define INDI_DOTSR_0 (indi_dots_r_pin[0])
#define INDI_DOTSR_1 (indi_dots_r_pin[1])

#if (SECS_DOT == 1) || (SECS_DOT == 2) || INDI_SYMB_TYPE
#if LAMP_NUM > 4
#define INDI_DOTS_START 0x09
#define INDI_DOTS0_MASK
#define INDI_DOTS1_MASK
#else
#define INDI_DOTS_START 0x05
#endif
#define INDI_POS_START 0x00
#else
#if LAMP_NUM > 4
#define INDI_DOTS_START 0x12
#else
#define INDI_DOTS_START 0x0A
#endif
#define INDI_POS_START 0x01
#endif

#if LAMP_NUM > 4
#define INDI_DOTS0_MASK 0x0E
#define INDI_DOTS1_MASK 0x70
#else
#define INDI_DOTS0_MASK 0x06
#define INDI_DOTS1_MASK 0x18
#endif

//----------------------------------Динамическая индикация---------------------------------------
ISR(TIMER0_OVF_vect) //динамическая индикация
{
  TCNT0 = TIMER_START; //установили счетчик в начало
  
#if DOTS_PORT_ENABLE
  indi_dot_pos <<= 1; //сместили текущей номер точек индикаторов
#endif

  if (++indi_state > LAMP_MAX_STEP) { //переходим к следующему индикатору
#if DOTS_PORT_ENABLE
    indi_dot_pos = INDI_DOTS_START; //сбросили текущей номер точек индикаторов
#endif
    indi_state = INDI_POS_START; //сбросили позицию индикатора
  }

  REG_LATCH_ENABLE; //открыли защелку
  SPDR = indi_buf[indi_state] | (indi_buf[indi_state + LAMP_MAX_STEP] << 4); //загрузили данные

  OCR0A = indi_dimm[indi_state] + TIMER_START; //устанавливаем яркость индикатора
  OCR0B = indi_dimm[indi_state + LAMP_MAX_STEP] + TIMER_START; //устанавливаем яркость индикатора

  if (indi_buf[indi_state] != INDI_NULL) {
    switch (indi_state) {
        //----------------------------------------------- ???
#if INDI_SYMB_TYPE
      case INDI_0_POS: ANODE_SET(ANODE_0_PIN); break;
#elif !INDI_DOT_TYPE && !INDI_SYMB_TYPE
#if SECS_DOT == 1
      case INDI_0_POS: SECS_DOT_SET(SECL_PIN); break;
#elif SECS_DOT == 2
      case INDI_0_POS: if (indi_buf[indi_state] & 0x80) SECS_DOT_SET(SECL_PIN); if (indi_buf[indi_state] & 0x40) SECS_DOT_SET(SECR_PIN); break;
#endif
#endif
        //----------------------------------------------- ???
#if LAMP_NUM > 4
      case INDI_1_POS: ANODE_SET(ANODE_1_PIN); break;
      case INDI_2_POS: ANODE_SET(ANODE_2_PIN); break;
      case INDI_3_POS: ANODE_SET(ANODE_3_PIN); break;
#else
      case INDI_1_POS: ANODE_SET(ANODE_1_PIN); break;
      case INDI_2_POS: ANODE_SET(ANODE_2_PIN); break;
#endif
    }
  }

  if (indi_buf[indi_state + LAMP_MAX_STEP] != INDI_NULL) {
    switch (indi_state) {
#if LAMP_NUM > 4
      case INDI_1_POS: ANODE_SET(ANODE_4_PIN); break;
      case INDI_2_POS: ANODE_SET(ANODE_5_PIN); break;
      case INDI_3_POS: ANODE_SET(ANODE_6_PIN); break;
#else
      case INDI_1_POS: ANODE_SET(ANODE_3_PIN); break;
      case INDI_2_POS: ANODE_SET(ANODE_4_PIN); break;
#endif
    }
  }

  //----------------------------------------------- ???
#if DOTS_PORT_ENABLE
#if (DOTS_TYPE == 1) || (DOTS_TYPE == 2)
  if (indi_dot_r & (indi_dot_pos & INDI_DOTS0_MASK)) INDI_DOT_SET(INDI_DOTSR_0); //включаем правые точки
  if (indi_dot_r & (indi_dot_pos & INDI_DOTS1_MASK)) INDI_DOT_SET(INDI_DOTSR_1); //включаем правые точки
#endif
#if DOTS_TYPE != 1
  if (indi_dot_l & (indi_dot_pos & INDI_DOTS0_MASK)) INDI_DOT_SET(INDI_DOTSL_0); //включаем левые точки
  if (indi_dot_l & (indi_dot_pos & INDI_DOTS1_MASK)) INDI_DOT_SET(INDI_DOTSL_1); //включаем левые точки
#endif
#endif
  //----------------------------------------------- ???

  tickCheck(); //проверка переполнения тиков
  stackCheck(); //проверка переполнения стека

  while (!(SPSR & (0x01 << SPIF))); //ждем отправки
  REG_LATCH_DISABLE; //закрыли защелку
}
ISR(TIMER0_COMPA_vect) {
  switch (indi_state) {
      //----------------------------------------------- ???
#if INDI_SYMB_TYPE
    case INDI_0_POS: ANODE_CLEAR(ANODE_0_PIN); break;
#elif !INDI_SYMB_TYPE
#if SECS_DOT == 1
    case INDI_0_POS: SECS_DOT_CLEAR(SECL_PIN); break;
#elif SECS_DOT == 2
    case INDI_0_POS: SECS_DOT_CLEAR(SECL_PIN); SECS_DOT_CLEAR(SECR_PIN); break;
#endif
#endif
      //----------------------------------------------- ???
#if LAMP_NUM > 4
    case INDI_1_POS: ANODE_CLEAR(ANODE_1_PIN); break;
    case INDI_2_POS: ANODE_CLEAR(ANODE_2_PIN); break;
    case INDI_3_POS: ANODE_CLEAR(ANODE_3_PIN); break;
#else
    case INDI_1_POS: ANODE_CLEAR(ANODE_1_PIN); break;
    case INDI_2_POS: ANODE_CLEAR(ANODE_2_PIN); break;
#endif
  }

  //----------------------------------------------- ???
#if DOTS_PORT_ENABLE
#if (DOTS_TYPE == 1) || (DOTS_TYPE == 2)
  INDI_DOT_CLEAR(INDI_DOTSR_0); //выключаем правые точки
#endif
#if DOTS_TYPE != 1
  INDI_DOT_CLEAR(INDI_DOTSL_0); //выключаем левые точки
#endif
#endif
  //----------------------------------------------- ???
}
ISR(TIMER0_COMPB_vect) {
  switch (indi_state) {
#if LAMP_NUM > 4
    case INDI_1_POS: ANODE_CLEAR(ANODE_4_PIN); break;
    case INDI_2_POS: ANODE_CLEAR(ANODE_5_PIN); break;
    case INDI_3_POS: ANODE_CLEAR(ANODE_6_PIN); break;
#else
    case INDI_1_POS: ANODE_CLEAR(ANODE_3_PIN); break;
    case INDI_2_POS: ANODE_CLEAR(ANODE_4_PIN); break;
#endif
  }

  //----------------------------------------------- ???
#if DOTS_PORT_ENABLE
#if (DOTS_TYPE == 1) || (DOTS_TYPE == 2)
  INDI_DOT_CLEAR(INDI_DOTSR_1); //выключаем правые точки
#endif
#if DOTS_TYPE != 1
  INDI_DOT_CLEAR(INDI_DOTSL_1); //выключаем левые точки
#endif
#endif
  //----------------------------------------------- ???
}
//------------------------Проверка состояния динамической индикации-------------------------------
void indiStateCheck(void) //проверка состояния динамической индикации
{
  if (TIMSK0 != ((0x01 << OCIE0B) | (0x01 << OCIE0A) | (0x01 << TOIE0))) { //если настройка изменилась
    TIMSK0 = (0x01 << OCIE0B) | (0x01 << OCIE0A) | (0x01 << TOIE0); //установили настройку
    SET_ERROR(INDI_ERROR); //устанавливаем ошибку сбоя работы динамической индикации
  }
  if (OCR0A < TIMER_START) { //если вышли за предел
    OCR0A = TIMER_START; //установили максимум
    SET_ERROR(INDI_ERROR); //устанавливаем ошибку сбоя работы динамической индикации
  }
  if (OCR0B < TIMER_START) { //если вышли за предел
    OCR0B = TIMER_START; //установили максимум
    SET_ERROR(INDI_ERROR); //устанавливаем ошибку сбоя работы динамической индикации
  }
}
//------------------------Проверка состояния динамической индикации-------------------------------
void indiCheck(void) //проверка состояния динамической индикации
{
  if (TCCR0A != 0x00) { //если настройка изменилась
    TCCR0A = 0x00; //установили настройку
    SET_ERROR(INDI_ERROR); //устанавливаем ошибку сбоя работы динамической индикации
  }
  if (TCCR0B != (0x01 << CS02)) { //если настройка изменилась
    TCCR0B = (0x01 << CS02); //установили настройку
    SET_ERROR(INDI_ERROR); //устанавливаем ошибку сбоя работы динамической индикации
  }
}
//----------------------------Инициализация портов индикации------------------------------------
void indiPortInit(void) //инициализация портов индикации
{
#if (SECS_DOT != 3)
  SECS_DOT_INIT(SECL_PIN); //инициализация секундных точек
#endif
#if (SECS_DOT == 2)
  SECS_DOT_INIT(SECR_PIN); //инициализация секундных точек
#endif

#if DOTS_PORT_ENABLE
#if (DOTS_TYPE == 1) || (DOTS_TYPE == 2)
  INDI_DOT_INIT(INDI_DOTSR_0); //инициализация правых разделительных точек в индикаторах
  INDI_DOT_INIT(INDI_DOTSR_1); //инициализация правых разделительных точек в индикаторах
#endif
#if (DOTS_TYPE != 1)
  INDI_DOT_INIT(INDI_DOTSL_0); //инициализация левых разделительных точек в индикаторах
  INDI_DOT_INIT(INDI_DOTSL_1); //инициализация левых разделительных точек в индикаторах
#endif
#endif

#if INDI_SYMB_TYPE
  ANODE_INIT(ANODE_0_PIN); //инициализация анода 0
#endif

  ANODE_INIT(ANODE_1_PIN); //инициализация анода 1
  ANODE_INIT(ANODE_2_PIN); //инициализация анода 2
  ANODE_INIT(ANODE_3_PIN); //инициализация анода 3
  ANODE_INIT(ANODE_4_PIN); //инициализация анода 4
#if LAMP_NUM > 4
  ANODE_INIT(ANODE_5_PIN); //инициализация анода 5
  ANODE_INIT(ANODE_6_PIN); //инициализация анода 6
#endif

  REG_LATCH_INIT; //инициализация защелки сдвигового регистра
  REG_DATA_INIT; //инициализация линии данных сдвигового регистра
  REG_SCK_INIT; //инициализация линии тактирования сдвигового регистра
  for (uint8_t i = 0; i < 16; i++) REG_SCK_INV; //очищаем сдвиговый регистр
  REG_LATCH_DISABLE; //закрываем защелку
}
//----------------------------Инициализация индикации------------------------------------
void indiInit(void) //инициализация индикации
{
  cli(); //запрещаем прерывания глобально

  for (uint8_t i = 0; i < (LAMP_NUM + 1); i++) { //инициализируем буферы
    indi_buf[i] = INDI_NULL; //очищаем буфер пустыми символами
    indi_dimm[i] = (LIGHT_MAX - 1); //устанавливаем минимальную яркость
  }

  OCR0A = OCR0B = TIMER_START; //минимальная яркость

  TIMSK0 = 0x00; //отключаем прерывания
  TCCR0A = 0x00; //обычный режим
  TCCR0B = (0x01 << CS02); //пределитель 256

  TCNT0 = 255; //установили счетчик в начало
  TIFR0 |= (0x01 << OCF0B) | (0x01 << OCF0A) | (0x01 << TOV0); //сбрасываем флаги прерывания
  TIMSK0 = (0x01 << OCIE0B) | (0x01 << OCIE0A) | (0x01 << TOIE0); //разрешаем прерывания

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
