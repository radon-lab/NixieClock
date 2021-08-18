uint8_t ledColor[12]; //массив цветов
uint8_t ledBright[4]; //массив яркости

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
    "CLI                  \n\t" //запретили прерывания
    "_BYTE_START_%=:      \n\t" //начало цикла отправки байта
    "LD r21, X+           \n\t" //загрузили байт из масива
    "LDI r19, 8           \n\t" //счетчик циклов байта
    //-----------------------------------------------------------------------------------------
    "_LOOP_START_%=:      \n\t" //начало цикла отправки бита
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
    //-----------------------------------------------------------------------------------------
    "LSL r21              \n\t" //сдвигаем байт влево
    "DEC r19              \n\t" //декремент счетчика циклов байта
    "BRNE _LOOP_START_%=  \n\t" //переход в начало цикла отправки бита
    //-----------------------------------------------------------------------------------------
    "SBIW %[COUNT], 1     \n\t" //отнимаем от счетчика байт
    "BRNE _BYTE_START_%=  \n\t" //переход к загрузке нового байта
    "SEI                  \n\t" //разрешили прерывания
    :
    :"x"(data),
    [COUNT]"w"(size),
    [PORT]"I"(_SFR_IO_ADDR(BACKL_PORT)),
    [PIN]"I"(BACKL_BIT)
  );
}
//--------------------------------------Установка цвета в формате HV------------------------------------------
void setLedHV(uint8_t led, uint8_t color)
{
  uint8_t pallet = (float)((color % 85) * ((float)ledBright[led] / 85.0));
  switch (color / 85) {
    case 0:
      ledColor[led * 3 + 1] = ledBright[led] - pallet;
      ledColor[led * 3] = pallet;
      break;
    case 1:
      ledColor[led * 3] = ledBright[led] - pallet;
      ledColor[led * 3 + 2] = pallet;
      break;
    case 2:
      ledColor[led * 3 + 2] = ledBright[led] - pallet;
      ledColor[led * 3 + 1] = pallet;
      break;
  }
  ledWrite(ledColor, sizeof(ledColor));
}
//--------------------------------------Установка цвета в формате HV------------------------------------------
void setLedHV(uint8_t color)
{
  for (uint8_t f = 0; f < 4; f++) {
    uint8_t pallet = (float)((color % 85) * ((float)ledBright[f] / 85.0));
    switch (color / 85) {
      case 0:
        ledColor[f * 3 + 1] = ledBright[f] - pallet;
        ledColor[f * 3] = pallet;
        break;
      case 1:
        ledColor[f * 3] = ledBright[f] - pallet;
        ledColor[f * 3 + 2] = pallet;
        break;
      case 2:
        ledColor[f * 3 + 2] = ledBright[f] - pallet;
        ledColor[f * 3 + 1] = pallet;
        break;
    }
  }
  ledWrite(ledColor, sizeof(ledColor));
}
//--------------------------------------Установка по цветовой палитре------------------------------------------
void setLedColor(uint8_t led, uint8_t clr)
{
  for (uint8_t i = 0; i < 3; i++) {
    if (((clr & 0x7F) >> i) & 0x01) ledColor[led * 3 + i] = ledBright[led];
    else ledColor[led * 3 + i] = 0;
  }
  ledWrite(ledColor, sizeof(ledColor));
}
//--------------------------------------Установка по цветовой палитре------------------------------------------
void setLedColor(uint8_t clr)
{
  for (uint8_t f = 0; f < 4; f++) {
    for (uint8_t i = 0; i < 3; i++) {
      if (((clr & 0x7F) >> i) & 0x01) ledColor[f * 3 + i] = ledBright[f];
      else ledColor[f * 3 + i] = 0;
    }
  }
  ledWrite(ledColor, sizeof(ledColor));
}
//--------------------------------------Установка яркости------------------------------------------
void setLedBright(uint8_t led, uint8_t brt)
{
  ledBright[led] = brt;
}
//--------------------------------------Установка яркости------------------------------------------
void setLedBright(uint8_t brt)
{
  for (uint8_t f = 0; f < 4; f++) ledBright[f] = brt;
}
