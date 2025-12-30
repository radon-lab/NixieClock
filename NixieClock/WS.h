#define LEDS_NUM (LAMP_NUM) //количество светодиодов

boolean ledUpdate = 0; //флаг отрисовки светодиодов
uint8_t ledColor[LEDS_NUM]; //массив цветов
uint8_t ledBright[LEDS_NUM]; //массив яркости

const uint8_t ledWhiteTable[][3] = { //таблица оттенков белого GRB
  155, 255, 60,  //2400
  208, 255, 170, //4100
  255, 255, 255  //6500
};

enum {
  WHITE_OFF, //выключить установку белого цвета
  WHITE_ON //включить установку белого цвета
};

//---------------------------------Передача массива данных на шину-------------------------------------
void wsLedWrite(uint8_t* data, uint8_t size) {
  __asm__ __volatile__ (
    "CBI %[PORT], %[PIN]  \n\t" //LOW на выход пина
    "LDI r19, 230         \n\t" //счетчик сигнала reset(50мкс)
    //-----------------------------------------------------------------------------------------
    "_LOOP_DELAY_%=:      \n\t" //цикл задержки
    "NOP                  \n\t" //пропускаем цикл
    "DEC r19              \n\t" //декремент счетчика циклов
    "BRNE _LOOP_DELAY_%=  \n\t" //переход в начало цикла задержки
    //-----------------------------------------------------------------------------------------
    "_BYTE_START_%=:      \n\t" //начало цикла отправки байта
    "CLI                  \n\t" //запретили прерывания
    "LD r20, X+           \n\t" //загрузили байт из масива
    "LDI r19, 8           \n\t" //счетчик циклов байта
    //-----------------------------------------------------------------------------------------
    "_LOOP_START_%=:      \n\t" //начало цикла отправки бита
    "CLI                  \n\t" //запретили прерывания
    "SBI %[PORT], %[PIN]  \n\t" //HIGH на выход пина
    "NOP                  \n\t" //пропускаем цикл
    "NOP                  \n\t" //пропускаем цикл
    "NOP                  \n\t" //пропускаем цикл
    "SBRS r20, 7          \n\t" //если бит "7" установлен то пропускаем переход
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
    "LSL r20              \n\t" //сдвигаем байт влево
    "DEC r19              \n\t" //декремент счетчика циклов байта
    "BRNE _LOOP_START_%=  \n\t" //переход в начало цикла отправки бита
    //-----------------------------------------------------------------------------------------
    "DEC %[SIZE]          \n\t" //отнимаем от счетчика байт
    "BRNE _BYTE_START_%=  \n\t" //переход к загрузке нового байта
    :
    :"x"(data),
    [SIZE]"r"(size),
    [PORT]"I"(_SFR_IO_ADDR(BACKL_PORT)),
    [PIN]"I"(BACKL_BIT)
    :"r19", "r20"
  );
}
//-----------------------------------Отрисовка светодиодов------------------------------------------
void wsBacklShowLeds(void)
{
  if (ledUpdate) { //если что-то изменилось
    ledUpdate = 0; //сбрасываем флаг обновления

    uint8_t ledBuff[LEDS_NUM * 3]; //массив светодиодов
    uint8_t* ledLink = ledBuff; //ссылка на текущий элемент

#if BACKL_REVERSE
    for (uint8_t f = LEDS_NUM; f;) {
      f--; //сместили номер светодиода
#else
    for (uint8_t f = 0; f < LEDS_NUM; f++) {
#endif
      uint8_t count = 0;
      uint8_t pallet = ledColor[f];
      if (pallet < 253) {
        while (pallet > 85) {
          pallet -= 85;
          count++;
        }
        if (ledBright[f]) pallet = (uint8_t)(((uint8_t)(pallet * 3) * ledBright[f]) >> 8) + 1;
        else pallet = 0;
        switch (count) {
          case 0:
            *ledLink++ = pallet;
            *ledLink++ = ledBright[f] - pallet;
            *ledLink++ = 0;
            break;
          case 1:
            *ledLink++ = ledBright[f] - pallet;
            *ledLink++ = 0;
            *ledLink++ = pallet;
            break;
          case 2:
            *ledLink++ = 0;
            *ledLink++ = pallet;
            *ledLink++ = ledBright[f] - pallet;
            break;
        }
      }
      else {
        pallet -= 253;
        *ledLink++ = (uint8_t)((ledBright[f] * ledWhiteTable[pallet][0]) >> 8);
        *ledLink++ = (uint8_t)((ledBright[f] * ledWhiteTable[pallet][1]) >> 8);
        *ledLink++ = (uint8_t)((ledBright[f] * ledWhiteTable[pallet][2]) >> 8);
      }
    }

    wsLedWrite(ledBuff, sizeof(ledBuff));
  }
}
//--------------------------------------Очистка светодиодов-----------------------------------------
void wsBacklClearLeds(void)
{
  ledUpdate = 1; //устанавливаем флаг обновления
  for (uint8_t f = 0; f < LEDS_NUM; f++) ledBright[f] = 0;
}
//---------------------------------Установка цвета в формате HV-------------------------------------
void wsBacklSetLedHue(uint8_t _led, uint8_t _color, boolean _mode)
{
  if (_led < LEDS_NUM) {
    ledUpdate = 1; //устанавливаем флаг обновления
    if (_mode == WHITE_OFF && _color >= 253) _color = 0;
    ledColor[_led] = _color;
  }
}
//---------------------------------Установка цвета в формате HV-------------------------------------
void wsBacklSetLedHue(uint8_t _color, boolean _mode)
{
  ledUpdate = 1; //устанавливаем флаг обновления
  if (_mode == WHITE_OFF && _color >= 253) _color = 0;
  for (uint8_t f = 0; f < LEDS_NUM; f++) ledColor[f] = _color;
}
//------------------------------Установка двух цветов в формате HV----------------------------------
void wsBacklSetMultiHue(uint8_t _start, uint8_t _count, uint8_t _color, uint8_t _color_main)
{
  ledUpdate = 1; //устанавливаем флаг обновления
  for (uint8_t f = 0; f < LEDS_NUM; f++) {
    if (_count && f >= _start) {
      ledColor[f] = _color;
      _count--;
    }
    else ledColor[f] = _color_main;
  }
}
//--------------------------------------Уменьшение яркости------------------------------------------
void wsBacklDecLedsBright(uint8_t _led, uint8_t _step)
{
  ledUpdate = 1; //устанавливаем флаг обновления
  for (uint8_t f = 0; f < LEDS_NUM; f++) {
    if (_led != f) {
      if (ledBright[f] > _step) ledBright[f] -= _step;
      else ledBright[f] = 0;
    }
  }
}
//--------------------------------------Уменьшение яркости------------------------------------------
boolean wsBacklDecLedBright(uint8_t _led, uint8_t _step, uint8_t _min)
{
  ledUpdate = 1; //устанавливаем флаг обновления
  if (_led < LEDS_NUM) {
    if (((int16_t)ledBright[_led] - _step) > _min) ledBright[_led] -= _step;
    else {
      ledBright[_led] = _min;
      return 1;
    }
  }
  return 0;
}
//--------------------------------------Увеличение яркости------------------------------------------
boolean wsBacklIncLedBright(uint8_t _led, uint8_t _step, uint8_t _max)
{
  ledUpdate = 1; //устанавливаем флаг обновления
  if (_led < LEDS_NUM) {
    if (((uint16_t)ledBright[_led] + _step) < _max) ledBright[_led] += _step;
    else {
      ledBright[_led] = _max;
      return 1;
    }
  }
  return 0;
}
//--------------------------------------Уменьшение яркости------------------------------------------
boolean wsBacklDecLedBright(uint8_t _step, uint8_t _min)
{
  ledUpdate = 1; //устанавливаем флаг обновления
  if (((int16_t)ledBright[0] - _step) > _min) {
    ledBright[0] -= _step;
    for (uint8_t f = 1; f < LEDS_NUM; f++) ledBright[f] = ledBright[0];
  }
  else {
    for (uint8_t f = 0; f < LEDS_NUM; f++) ledBright[f] = _min;
    return 1;
  }
  return 0;
}
//--------------------------------------Увеличение яркости------------------------------------------
boolean wsBacklIncLedBright(uint8_t _step, uint8_t _max)
{
  ledUpdate = 1; //устанавливаем флаг обновления
  if (((uint16_t)ledBright[0] + _step) < _max) {
    ledBright[0] += _step;
    for (uint8_t f = 1; f < LEDS_NUM; f++) ledBright[f] = ledBright[0];
  }
  else {
    for (uint8_t f = 0; f < LEDS_NUM; f++) ledBright[f] = _max;
    return 1;
  }
  return 0;
}
//--------------------------------------Установка яркости------------------------------------------
void wsBacklSetLedBright(uint8_t _led, uint8_t _brt)
{
  if (_led < LEDS_NUM) {
    ledUpdate = 1; //устанавливаем флаг обновления
    ledBright[_led] = _brt;
  }
}
//--------------------------------------Установка яркости------------------------------------------
void wsBacklSetLedBright(uint8_t _brt)
{
  ledUpdate = 1; //устанавливаем флаг обновления
  for (uint8_t f = 0; f < LEDS_NUM; f++) ledBright[f] = _brt;
}
//--------------------------Установка яркости только во включенные диоды---------------------------
void wsBacklSetOnLedBright(uint8_t _brt)
{
  for (uint8_t f = 0; f < LEDS_NUM; f++) {
    if (ledBright[f]) {
      ledUpdate = 1; //устанавливаем флаг обновления
      ledBright[f] = _brt;
    }
  }
}
