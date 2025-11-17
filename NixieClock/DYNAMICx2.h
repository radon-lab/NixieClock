#define _BIT(value, bit) (((value) >> (bit)) & 0x01)
#define ID(digit) ((_BIT(digit, 0) << DECODER_1) | (_BIT(digit, 1) << DECODER_2) | (_BIT(digit, 2) << DECODER_3) | (_BIT(digit, 3) << DECODER_4))
#define INDI_NULL ((0x01 << DECODER_2) | (0x01 << DECODER_4)) //пустой символ(отключеный индикатор)

enum {INDI_0_POS, INDI_1_POS, INDI_2_POS, INDI_3_POS}; //порядок индикации ламп
#include "boards.h"

#define TIME_TICK 16.0 //время одной единицы шага таймера(мкс)
#define FREQ_TICK (uint8_t)(CONSTRAIN((1000.0 / ((uint16_t)INDI_FREQ_ADG * ((LAMP_NUM / 2) + (boolean)((SECS_DOT == 1) || (SECS_DOT == 2) || INDI_SYMB_TYPE)))) / 0.016, 125, 255)) //расчет переполнения таймера динамической индикации

#define TIMER_START (255 - FREQ_TICK) //начальное значение таймера
#define LAMP_MAX_STEP (LAMP_NUM / 2) //количиство ламп в группе

#include "CORE.h"

//----------------------------------Динамическая индикация---------------------------------------
ISR(TIMER0_OVF_vect) //динамическая индикация
{
#if TIMER_START
  TCNT0 = TIMER_START; //установили счетчик в начало
#endif

  if (++indiState > LAMP_MAX_STEP) { //переходим к следующему индикатору
#if (SECS_DOT == 1) || (SECS_DOT == 2) || INDI_SYMB_TYPE
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

  REG_LATCH_ENABLE; //открыли защелку
  SPDR = indi_buf[indiState] | (indi_buf[indiState + LAMP_MAX_STEP] << 4); //загрузили данные

  OCR0A = indi_dimm[indiState] + TIMER_START; //устанавливаем яркость индикатора
  OCR0B = indi_dimm[indiState + LAMP_MAX_STEP] + TIMER_START; //устанавливаем яркость индикатора

  if (indi_buf[indiState] != INDI_NULL) {
    switch (indiState) {
#if SECS_DOT == 1
      case INDI_0_POS: DOT_1_SET; break;
#elif SECS_DOT == 2
      case INDI_0_POS: if (indi_buf[indiState] & 0x80) DOT_1_SET; if (indi_buf[indiState] & 0x40) DOT_2_SET; break;
#endif
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

  if (indi_buf[indiState + LAMP_MAX_STEP] != INDI_NULL) {
    switch (indiState) {
        //----------------------------------------------- ???
        //#if INDI_SYMB_TYPE
        //      case INDI_0_POS: ANODE_SET(ANODE_0_PIN); break;
        //#endif
        //----------------------------------------------- ???
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
  //#if DOTS_PORT_ENABLE
  //#if (DOTS_TYPE == 1) || (DOTS_TYPE == 2)
  //  if (indi_dot_r & indi_dot_pos) INDI_DOTR_ON; //включаем правые точки
  //#endif
  //#if DOTS_TYPE != 1
  //  if (indi_dot_l & indi_dot_pos) INDI_DOTL_ON; //включаем левые точки
  //#endif
  //#endif
  //----------------------------------------------- ???

  tickCheck(); //проверка переполнения тиков
  stackCheck(); //проверка переполнения стека

  while (!(SPSR & (0x01 << SPIF))); //ждем отправки
  REG_LATCH_DISABLE; //закрыли защелку
}
ISR(TIMER0_COMPA_vect) {
  switch (indiState) {
#if SECS_DOT == 1
    case INDI_0_POS: DOT_1_CLEAR; break;
#elif SECS_DOT == 2
    case INDI_0_POS: DOT_1_CLEAR; DOT_2_CLEAR; break;
#endif
#if LAMP_NUM > 4
    case INDI_1_POS: ANODE_CLEAR(ANODE_1_PIN); break;
    case INDI_2_POS: ANODE_CLEAR(ANODE_2_PIN); break;
    case INDI_3_POS: ANODE_CLEAR(ANODE_3_PIN); break;
#else
    case INDI_1_POS: ANODE_CLEAR(ANODE_1_PIN); break;
    case INDI_2_POS: ANODE_CLEAR(ANODE_2_PIN); break;
#endif
  }
}
ISR(TIMER0_COMPB_vect) {
  switch (indiState) {
      //----------------------------------------------- ???
#if INDI_SYMB_TYPE
    case INDI_0_POS: ANODE_CLEAR(ANODE_0_PIN); break;
#endif
      //----------------------------------------------- ???
#if LAMP_NUM > 4
    case INDI_1_POS: ANODE_CLEAR(ANODE_4_PIN); break;
    case INDI_2_POS: ANODE_CLEAR(ANODE_5_PIN); break;
    case INDI_3_POS: ANODE_CLEAR(ANODE_6_PIN); break;
#else
    case INDI_1_POS: ANODE_CLEAR(ANODE_3_PIN); break;
    case INDI_2_POS: ANODE_CLEAR(ANODE_4_PIN); break;
#endif
  }
}
//------------------------Проверка состояния динамической индикации-------------------------------
void indiStateCheck(void) //проверка состояния динамической индикации
{
  if (TIMSK0 != ((0x01 << OCIE0B) | (0x01 << OCIE0A) | (0x01 << TOIE0))) { //если настройка изменилась
    TIMSK0 = (0x01 << OCIE0B) | (0x01 << OCIE0A) | (0x01 << TOIE0); //установили настройку
    SET_ERROR(INDI_ERROR); //устанавливаем ошибку сбоя работы динамической индикации
  }
#if TIMER_START
  if (OCR0A < TIMER_START) { //если вышли за предел
    OCR0A = TIMER_START; //установили максимум
    SET_ERROR(INDI_ERROR); //устанавливаем ошибку сбоя работы динамической индикации
  }
  if (OCR0B < TIMER_START) { //если вышли за предел
    OCR0B = TIMER_START; //установили максимум
    SET_ERROR(INDI_ERROR); //устанавливаем ошибку сбоя работы динамической индикации
  }
#endif
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
  DOT_1_INIT; //инициализация секундных точек
#endif
#if (SECS_DOT == 2)
  DOT_2_INIT; //инициализация секундных точек
#endif

#if (DOTS_TYPE == 1) || (DOTS_TYPE == 2)
  INDI_DOTR_INIT; //инициализация правых разделительных точек в индикаторах
#endif
#if (DOTS_TYPE != 1)
  INDI_DOTL_INIT; //инициализация левых разделительных точек в индикаторах
#endif

#if (INDI_SYMB_TYPE == 1)
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
    indi_dimm[i] = LIGHT_MAX; //устанавливаем максимальную яркость
  }

  OCR0A = OCR0B = (LIGHT_MAX + TIMER_START); //максимальная яркость

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
