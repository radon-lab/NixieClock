uint8_t ledColor[LAMP_NUM]; //массив цветов
uint8_t ledBright[LAMP_NUM]; //массив яркости

//---------------------------------Передача массива данных на шину-------------------------------------
void ledWrite(uint8_t* data, uint16_t size) {
  __asm__ __volatile__ (
    "CBI %[PORT], %[PIN]  \n\t" //LOW на выход пина
    "LDI r19, 200         \n\t" //счетчик сигнала reset(50мкс)
    //-----------------------------------------------------------------------------------------
    "_LOOP_DELAY_%=:      \n\t" //цикл задержки
    "NOP                  \n\t" //пропускаем цикл
    "DEC r19              \n\t" //декремент счетчика циклов
    "BRNE _LOOP_DELAY_%=  \n\t" //переход в начало цикла задержки
    //-----------------------------------------------------------------------------------------
    "_BYTE_START_%=:      \n\t" //начало цикла отправки байта
    "LD r21, X+           \n\t" //загрузили байт из масива
    "LDI r19, 8           \n\t" //счетчик циклов байта
    //-----------------------------------------------------------------------------------------
    "_LOOP_START_%=:      \n\t" //начало цикла отправки бита
    "CLI                  \n\t" //запретили прерывания
    "SBI %[PORT], %[PIN]  \n\t" //HIGH на выход пина
    "NOP                  \n\t" //пропускаем цикл
    "NOP                  \n\t" //пропускаем цикл
    "NOP                  \n\t" //пропускаем цикл
    "SBRS r21, 7          \n\t" //если бит "7" установлен то пропускаем переход
    "CBI %[PORT], %[PIN]  \n\t" //LOW на выход пина
    "NOP                  \n\t" //пропускаем цикл
    "NOP                  \n\t" //пропускаем цикл
    "NOP                  \n\t" //пропускаем цикл
    "NOP                  \n\t" //пропускаем цикл
    "NOP                  \n\t" //пропускаем цикл
    "NOP                  \n\t" //пропускаем цикл
    "NOP                  \n\t" //пропускаем цикл
    "CBI %[PORT], %[PIN]  \n\t" //LOW на выход пина
    "SEI                  \n\t" //разрешили прерывания
    //-----------------------------------------------------------------------------------------
    "LSL r21              \n\t" //сдвигаем байт влево
    "DEC r19              \n\t" //декремент счетчика циклов байта
    "BRNE _LOOP_START_%=  \n\t" //переход в начало цикла отправки бита
    //-----------------------------------------------------------------------------------------
    "SBIW %[COUNT], 1     \n\t" //отнимаем от счетчика байт
    "BRNE _BYTE_START_%=  \n\t" //переход к загрузке нового байта
    :
    :"x"(data),
    [COUNT]"w"(size),
    [PORT]"I"(_SFR_IO_ADDR(BACKL_PORT)),
    [PIN]"I"(BACKL_BIT)
  );
}
//-----------------------------------Отрисовка светодиодов------------------------------------------
void showLeds(void)
{
  uint8_t ledBuff[LAMP_NUM * 3]; //массив светодиодов
  for (uint8_t f = 0; f < LAMP_NUM; f++) {
    uint8_t pallet = (float)(ledColor[f] % 85) * ((float)ledBright[f] / 85.0);
    switch (ledColor[f] / 85) {
      default:
        ledBuff[f * 3] = pallet;
        ledBuff[f * 3 + 1] = ledBright[f] - pallet;
        ledBuff[f * 3 + 2] = 0;
        break;
      case 1:
        ledBuff[f * 3] = ledBright[f] - pallet;
        ledBuff[f * 3 + 1] = 0;
        ledBuff[f * 3 + 2] = pallet;
        break;
      case 2:
        ledBuff[f * 3] = 0;
        ledBuff[f * 3 + 1] = pallet;
        ledBuff[f * 3 + 2] = ledBright[f] - pallet;
        break;
    }
  }
  ledWrite(ledBuff, sizeof(ledBuff));
}
//--------------------------------------Очистка светодиодов-----------------------------------------
void clrLeds(void)
{
  uint8_t ledBuff[LAMP_NUM * 3];
  for (uint8_t f = 0; f < sizeof(ledBuff); f++) ledBuff[f] = 0;
  ledWrite(ledBuff, sizeof(ledBuff));
}
//---------------------------------Установка цвета в формате HV-------------------------------------
void setLedHue(uint8_t _led, uint8_t _color)
{
  if (_led < LAMP_NUM) ledColor[_led] = _color;
}
//---------------------------------Установка цвета в формате HV-------------------------------------
void setLedHue(uint8_t _color)
{
  for (uint8_t f = 0; f < LAMP_NUM; f++) ledColor[f] = _color;
}
//--------------------------------------Уменьшение яркости------------------------------------------
void decLedsBright(uint8_t _led, uint8_t _step)
{
  for (uint8_t f = 0; f < LAMP_NUM; f++) {
    if (_led != f) {
      if (ledBright[f] > _step) ledBright[f] -= _step;
      else ledBright[f] = 0;
    }
  }
}
//--------------------------------------Уменьшение яркости------------------------------------------
boolean decLedBright(uint8_t _led, uint8_t _step, uint8_t _min)
{
  if (((int16_t)ledBright[_led] - _step) > _min) ledBright[_led] -= _step;
  else {
    ledBright[_led] = _min;
    return 1;
  }
  return 0;
}
//--------------------------------------Увеличение яркости------------------------------------------
boolean incLedBright(uint8_t _led, uint8_t _step, uint8_t _max)
{
  if (((uint16_t)ledBright[_led] + _step) < _max) ledBright[_led] += _step;
  else {
    ledBright[_led] = _max;
    return 1;
  }
  return 0;
}
//--------------------------------------Уменьшение яркости------------------------------------------
boolean decLedBright(uint8_t _step, uint8_t _min)
{
  if (((int16_t)ledBright[0] - _step) > _min) {
    ledBright[0] -= _step;
    for (uint8_t f = 1; f < LAMP_NUM; f++) ledBright[f] = ledBright[0];
  }
  else {
    for (uint8_t f = 0; f < LAMP_NUM; f++) ledBright[f] = _min;
    return 1;
  }
  return 0;
}
//--------------------------------------Увеличение яркости------------------------------------------
boolean incLedBright(uint8_t _step, uint8_t _max)
{
  if (((uint16_t)ledBright[0] + _step) < _max) {
    ledBright[0] += _step;
    for (uint8_t f = 1; f < LAMP_NUM; f++) ledBright[f] = ledBright[0];
  }
  else {
    for (uint8_t f = 0; f < LAMP_NUM; f++) ledBright[f] = _max;
    return 1;
  }
  return 0;
}
//--------------------------------------Установка яркости------------------------------------------
void setLedBright(uint8_t _led, uint8_t brt)
{
  if (_led < LAMP_NUM) ledBright[_led] = brt;
}
//--------------------------------------Установка яркости------------------------------------------
void setLedBright(uint8_t brt)
{
  for (uint8_t f = 0; f < LAMP_NUM; f++) ledBright[f] = brt;
}
