uint8_t ledColor[LAMP_NUM * 3]; //массив цветов
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
    "SBRS r21, 7          \n\t" //если бит "7" установлен то пропускаем переход
    "CBI %[PORT], %[PIN]  \n\t" //LOW на выход пина
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
//--------------------------------------Установка цвета в формате HV------------------------------------------
void setLedHue(uint8_t led, uint8_t color)
{
  uint8_t pallet = (float)((color % 85) * ((float)ledBright[led] / 85.0));
  switch (color / 85) {
    case 0:
      ledColor[led * 3] = pallet;
      ledColor[led * 3 + 1] = ledBright[led] - pallet;
      ledColor[led * 3 + 2] = 0;
      break;
    case 1:
      ledColor[led * 3] = ledBright[led] - pallet;
      ledColor[led * 3 + 1] = 0;
      ledColor[led * 3 + 2] = pallet;
      break;
    case 2:
      ledColor[led * 3] = 0;
      ledColor[led * 3 + 1] = pallet;
      ledColor[led * 3 + 2] = ledBright[led] - pallet;
      break;
  }
  ledWrite(ledColor, sizeof(ledColor));
}
//--------------------------------------Установка цвета в формате HV------------------------------------------
void setLedHue(uint8_t color)
{
  for (uint8_t f = 0; f < LAMP_NUM; f++) {
    uint8_t pallet = (float)((color % 85) * ((float)ledBright[f] / 85.0));
    switch (color / 85) {
      case 0:
        ledColor[f * 3] = pallet;
        ledColor[f * 3 + 1] = ledBright[f] - pallet;
        ledColor[f * 3 + 2] = 0;
        break;
      case 1:
        ledColor[f * 3] = ledBright[f] - pallet;
        ledColor[f * 3 + 1] = 0;
        ledColor[f * 3 + 2] = pallet;
        break;
      case 2:
        ledColor[f * 3] = 0;
        ledColor[f * 3 + 1] = pallet;
        ledColor[f * 3 + 2] = ledBright[f] - pallet;
        break;
    }
  }
  ledWrite(ledColor, sizeof(ledColor));
}
//--------------------------------------Очистка светодиодов------------------------------------------
void clrLeds(void)
{
  uint8_t temp[LAMP_NUM * 3];
  for (uint8_t f = 0; f < sizeof(temp); f++) temp[f] = 0;
  ledWrite(temp, sizeof(temp));
}
//--------------------------------------Очистка светодиодов------------------------------------------
void clrLed(uint8_t led)
{
  for (uint8_t f = 0; f < 3; f++) ledColor[led * f] = 0;
  ledWrite(ledColor, sizeof(ledColor));
}
//--------------------------------------Уменьшение яркости------------------------------------------
void decLedsBright(uint8_t led, uint8_t brt)
{
  for (uint8_t f = 0; f < LAMP_NUM; f++) {
    if (led != f) {
      if (ledBright[f] > brt) ledBright[f] -= brt;
      else ledBright[f] = 0;
    }
  }
}
//--------------------------------------Уменьшение яркости------------------------------------------
boolean decLedBright(uint8_t led, uint8_t brt, uint8_t _min)
{
  if ((int16_t)(ledBright[led] - brt) > _min) ledBright[led] -= brt;
  else {
    ledBright[led] = _min;
    return 1;
  }
  return 0;
}
//--------------------------------------Увеличение яркости------------------------------------------
boolean incLedBright(uint8_t led, uint8_t brt, uint8_t _max)
{
  if ((uint16_t)(ledBright[led] + brt) < _max) ledBright[led] += brt;
  else {
    ledBright[led] = _max;
    return 1;
  }
  return 0;
}
//--------------------------------------Уменьшение яркости------------------------------------------
boolean decLedBright(uint8_t brt, uint8_t _min)
{
  if ((int16_t)(ledBright[0] - brt) > _min) {
    ledBright[0] -= brt;
    for (uint8_t f = 1; f < LAMP_NUM; f++) ledBright[f] = ledBright[0];
  }
  else {
    for (uint8_t f = 0; f < LAMP_NUM; f++) ledBright[f] = _min;
    return 1;
  }
  return 0;
}
//--------------------------------------Увеличение яркости------------------------------------------
boolean incLedBright(uint8_t brt, uint8_t _max)
{
  if ((uint16_t)(ledBright[0] + brt) < _max) {
    ledBright[0] += brt;
    for (uint8_t f = 1; f < LAMP_NUM; f++) ledBright[f] = ledBright[0];
  }
  else {
    for (uint8_t f = 0; f < LAMP_NUM; f++) ledBright[f] = _max;
    return 1;
  }
  return 0;
}
//--------------------------------------Установка яркости------------------------------------------
void setLedBright(uint8_t led, uint8_t brt)
{
  if (led < LAMP_NUM) ledBright[led] = brt;
}
//--------------------------------------Установка яркости------------------------------------------
void setLedBright(uint8_t brt)
{
  for (uint8_t f = 0; f < LAMP_NUM; f++) ledBright[f] = brt;
}
