//-----------------------------------Динамическая подсветка---------------------------------------
#if (BACKL_TYPE == 2) && (SECS_DOT != 4) && !IR_PORT_ENABLE
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
//-------------------------------Получить яркости подсветки---------------------------------------
inline uint8_t ledBacklGetBright(void) //получить яркости подсветки
{
  return OCR2A;
}
//------------------------------Установка яркости подсветки---------------------------------------
void ledBacklSetBright(uint8_t pwm) //установка яркости подсветки
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
boolean ledBacklDecBright(uint8_t _step, uint8_t _min)
{
  if (((int16_t)ledBacklGetBright() - _step) > _min) ledBacklSetBright(ledBacklGetBright() - _step);
  else {
    ledBacklSetBright(_min);
    return 1;
  }
  return 0;
}
//-----------------------------------Увеличение яркости------------------------------------------
boolean ledBacklIncBright(uint8_t _step, uint8_t _max)
{
  if (((uint16_t)ledBacklGetBright() + _step) < _max) ledBacklSetBright(ledBacklGetBright() + _step);
  else {
    ledBacklSetBright(_max);
    return 1;
  }
  return 0;
}
