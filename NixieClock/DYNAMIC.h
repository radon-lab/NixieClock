#define TIME_TICK 16 //время одной единицы шага таймера(мкс)
#define FREQ_TICK (uint8_t)(CONSTRAIN(1e6 / TIME_TICK / ((uint16_t)INDI_FREQ_ADG * (LAMP_NUM + (boolean)((SECS_DOT == 1) || (SECS_DOT == 2) || INDI_SYMB_TYPE))), 30 + INDI_DEAD_TIME, 255)) //расчет переполнения таймера динамической индикации

#include "IO.h"
#include "CORE.h"

#define _BIT(value, bit) (((value) >> (bit)) & 0x01)
#define ID(digit) ((_BIT(digit, 0) << DECODER_1) | (_BIT(digit, 1) << DECODER_2) | (_BIT(digit, 2) << DECODER_3) | (_BIT(digit, 3) << DECODER_4))
#define INDI_NULL ((0x01 << DECODER_2) | (0x01 << DECODER_4)) //пустой символ(отключеный индикатор)
#define INDI_ANODE_OFF 0x00 //выключенный анод

#define LIGHT_MAX (uint8_t)(FREQ_TICK - INDI_DEAD_TIME) //расчет максимального шага яркости
#define DOT_LIGHT_MAX (uint8_t)(CONSTRAIN_MAX(((uint16_t)FREQ_TICK - 2) + (FREQ_TICK >> 5), 255)) //расчет максимального шага яркости для точек
#define INDI_LIGHT_MAX (uint16_t)(((uint16_t)LIGHT_MAX * 8) + (LIGHT_MAX >> 1)) //расчет максимального шага яркости для индикаторов

const uint8_t digitMask[] = {DIGIT_MASK}; //порядок пинов лампы(другие платы)
const uint8_t cathodeMask[] = {CATHODE_MASK}; //порядок катодов(другие платы)

enum {INDI_POS, ANODE_1_POS, ANODE_2_POS, ANODE_3_POS, ANODE_4_POS, ANODE_5_POS, ANODE_6_POS}; //порядок анодов ламп(точки всегда должны быть первыми)(только для прямого подключения к микроконтроллеру)

#if INDI_PORT_TYPE
const uint8_t regMask[] = {((SECS_DOT == 1) && INDI_DOT_TYPE) ? (0x01 << SECL_PIN) : ((INDI_SYMB_TYPE == 2) ? (0x01 << ANODE_0_PIN) : INDI_ANODE_OFF), (0x01 << ANODE_1_PIN), (0x01 << ANODE_2_PIN), (0x01 << ANODE_3_PIN), (0x01 << ANODE_4_PIN), (0x01 << ANODE_5_PIN), (0x01 << ANODE_6_PIN)}; //таблица бит анодов ламп
#endif

//----------------------------------Динамическая индикация---------------------------------------
ISR(TIMER0_COMPA_vect) //динамическая индикация
{
#if INDI_PORT_TYPE
  uint8_t temp = (indi_buf[indiState] != INDI_NULL) ? regMask[indiState] : INDI_ANODE_OFF; //включаем индикатор если не пустой символ
#if DOTS_PORT_ENABLE == 2
#if (DOTS_TYPE == 1) || (DOTS_TYPE == 2)
  if (indi_dot_r & indi_dot_pos) temp |= (0x01 << DOTSR_PIN); //включаем правые точки
#endif
#if DOTS_TYPE != 1
  if (indi_dot_l & indi_dot_pos) temp |= (0x01 << DOTSL_PIN); //включаем левые точки
#endif
#endif

#if (SECS_DOT == 2) && INDI_DOT_TYPE
  if (!indiState) {
    if (indi_buf[indiState] & 0x80) temp |= (0x01 << SECL_PIN); //включили точки
    if (indi_buf[indiState] & 0x40) temp |= (0x01 << SECR_PIN); //включили точки
  }
#endif

  REG_LATCH_ENABLE; //открыли защелку
  SPDR = temp; //загрузили данные
#endif

  OCR0B = indi_dimm[indiState]; //устанавливаем яркость индикатора
  
#if WIRE_PULL && !ESP_ENABLE
  PORTC = indi_buf[indiState] | 0x30; //отправляем в дешефратор буфер индикатора и устанавливаем подтяжку
#else
  PORTC = indi_buf[indiState]; //отправляем в дешефратор буфер индикатора
#endif

#if INDI_PORT_TYPE || (INDI_SYMB_TYPE == 1) || (!INDI_DOT_TYPE && !INDI_SYMB_TYPE)
  if (indi_buf[indiState] != INDI_NULL) {
    switch (indiState) {
#if (INDI_SYMB_TYPE == 1)
      case INDI_POS: ANODE_SET(ANODE_0_PIN); break;
#elif !INDI_DOT_TYPE && !INDI_SYMB_TYPE
#if SECS_DOT == 1
      case INDI_POS: SECS_DOT_SET(SECL_PIN); break;
#elif SECS_DOT == 2
      case INDI_POS: if (indi_buf[indiState] & 0x80) SECS_DOT_SET(SECL_PIN); if (indi_buf[indiState] & 0x40) SECS_DOT_SET(SECR_PIN); break;
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
  if (indi_dot_r & indi_dot_pos) INDI_DOT_SET(DOTSR_PIN); //включаем правые точки
#endif
#if DOTS_TYPE != 1
  if (indi_dot_l & indi_dot_pos) INDI_DOT_SET(DOTSL_PIN); //включаем левые точки
#endif
#endif

  tickCheck(); //проверка переполнения тиков
  stackCheck(); //проверка переполнения стека

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
#if (SECS_DOT == 1)
  if (!indiState) SECS_DOT_CLEAR(SECL_PIN); //выключили точки
#elif (SECS_DOT == 2)
  if (!indiState) {
    SECS_DOT_CLEAR(SECL_PIN); //выключили точки
    SECS_DOT_CLEAR(SECR_PIN); //выключили точки
  }
#endif
#endif
#else
  switch (indiState) {
#if (INDI_SYMB_TYPE == 1)
    case INDI_POS: ANODE_CLEAR(ANODE_0_PIN); break;
#elif !INDI_SYMB_TYPE
#if SECS_DOT == 1
    case INDI_POS: SECS_DOT_CLEAR(SECL_PIN); break;
#elif SECS_DOT == 2
    case INDI_POS: SECS_DOT_CLEAR(SECL_PIN); SECS_DOT_CLEAR(SECR_PIN); break;
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
  INDI_DOT_CLEAR(DOTSR_PIN); //выключаем правые точки
#endif
#if DOTS_TYPE != 1
  INDI_DOT_CLEAR(DOTSL_PIN); //выключаем левые точки
#endif
#endif
  indi_dot_pos <<= 1; //сместили текущей номер точек индикаторов
#endif

  if (++indiState > LAMP_NUM) { //переходим к следующему индикатору
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
#if (SECS_DOT != 3) && !INDI_DOT_TYPE
  SECS_DOT_INIT(SECL_PIN); //инициализация секундных точек
#endif
#if (SECS_DOT == 2) && !INDI_DOT_TYPE
  SECS_DOT_INIT(SECR_PIN); //инициализация секундных точек
#endif
#if DOTS_PORT_ENABLE == 1
#if (DOTS_TYPE == 1) || (DOTS_TYPE == 2)
  INDI_DOT_INIT(DOTSR_PIN); //инициализация правых разделительных точек в индикаторах
#endif
#if DOTS_TYPE != 1
  INDI_DOT_INIT(DOTSL_PIN); //инициализация левых разделительных точек в индикаторах
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

  TCNT0 = 0; //установили счетчик в начало
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
