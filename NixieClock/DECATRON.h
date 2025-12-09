uint8_t decatron_start; //начальная позиция декатрона
uint8_t decatron_end; //конечная позиция декатрона

volatile uint8_t decatron_step; //текущий шаг декатрона
volatile uint8_t decatron_pos; //текущая позиция декатрона
volatile boolean decatron_dir; //напрвление движения декатрона

//-----------------------------Прерывание декатрона--------------------------------------
#if DECATRON_ENABLE
ISR(TIMER2_COMPA_vect) //прерывание декатрона
{
  if (!decatron_dir) {
    if (decatron_pos != decatron_start) {
      if (decatron_step > 0) decatron_step--;
      else decatron_step = 2;
      if (decatron_pos > 0) decatron_pos--;
      else decatron_pos = (DECATRON_DOTS_NUM - 1);
    }
    else {
      if (decatron_start != decatron_end) decatron_dir = !decatron_dir; //сменили напрвление
      else TIMSK2 &= ~(0x01 << OCIE2A); //выключаем таймер
    }
  }
  else {
    if (decatron_pos != decatron_end) {
      if (decatron_step < 2) decatron_step++;
      else decatron_step = 0;
      if (decatron_pos < (DECATRON_DOTS_NUM - 1)) decatron_pos++;
      else decatron_pos = 0;
    }
    else {
      if (decatron_start != decatron_end) decatron_dir = !decatron_dir; //сменили напрвление
      else TIMSK2 &= ~(0x01 << OCIE2A); //выключаем таймер
    }
  }

  switch (decatron_step) {
    case 0:
      DECATRON_SET(DECATRON_K0_PIN);
      DECATRON_CLEAR(DECATRON_PK1_PIN);
      DECATRON_CLEAR(DECATRON_PK2_PIN);
      break;
    case 1:
      DECATRON_SET(DECATRON_PK2_PIN);
      DECATRON_CLEAR(DECATRON_K0_PIN);
      DECATRON_CLEAR(DECATRON_PK1_PIN);
      break;
    case 2:
      DECATRON_SET(DECATRON_PK1_PIN);
      DECATRON_CLEAR(DECATRON_K0_PIN);
      DECATRON_CLEAR(DECATRON_PK2_PIN);
      break;
  }
}
#endif
//----------------------------Инициализация декатрона------------------------------------
void decatronInit(void) //инициализация декатрона
{
  DECATRON_INIT(DECATRON_K0_PIN);
  DECATRON_INIT(DECATRON_PK1_PIN);
  DECATRON_INIT(DECATRON_PK2_PIN);
  decatron_pos = 255;
}
//--------------------------Установка позиции декатрона----------------------------------
void decatronSetLine(uint8_t start, uint8_t end) //установка позиции декатрона
{
  if (start >= DECATRON_DOTS_NUM) start = (DECATRON_DOTS_NUM - 1);
  if (end >= DECATRON_DOTS_NUM) end = (DECATRON_DOTS_NUM - 1);

  TIMSK2 &= ~(0x01 << OCIE2A); //выключаем таймер

  decatron_start = start;
  decatron_end = end;

  if (decatron_pos == 255) { //если декатрон выключен
    decatron_step = 0; //сбросили шаг
    decatron_pos = 0; //сбросили позицию
    decatron_dir = 0; //сбросили направление
    DECATRON_SET(DECATRON_K0_PIN);
    OCR2A = TCNT2; //устанавливаем COMA в начало
    TIFR2 |= (0x01 << OCF2A); //сбрасываем флаг прерывания
  }

  if (decatron_pos > decatron_start) {
    if ((decatron_pos - decatron_start) > 15) decatron_dir = 1;
    else decatron_dir = 0;
  }
  else {
    if ((decatron_start - decatron_pos) > 15) decatron_dir = 0;
    else decatron_dir = 1;
  }

  TIMSK2 |= (0x01 << OCIE2A); //запускаем таймер
}
//--------------------------Установка позиции декатрона----------------------------------
void decatronSetDot(uint8_t dot) //установка позиции декатрона
{
  decatronSetLine(dot, dot);
}
//------------------------------Отключение декатрона-------------------------------------
void decatronDisable(void) //отключение декатрона
{
  TIMSK2 &= ~(0x01 << OCIE2A); //выключаем таймер
  DECATRON_CLEAR(DECATRON_K0_PIN);
  DECATRON_CLEAR(DECATRON_PK1_PIN);
  DECATRON_CLEAR(DECATRON_PK2_PIN);
  decatron_pos = 255;
}
