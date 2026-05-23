#if BACKL_REVERSE
#define LED_DIGIT(digit) (CONSTRAIN_MAX(digit + (LEDS_NUM - LAMP_NUM), (LEDS_NUM - 1)))
#else
#define LED_DIGIT(digit) (CONSTRAIN_MAX(digit, (LEDS_NUM - 1)))
#endif

const uint8_t ledMask[] PROGMEM = {LED_DIGIT(LED_DIGIT_H1), LED_DIGIT(LED_DIGIT_H2), LED_DIGIT(LED_DIGIT_M1),
                                   LED_DIGIT(LED_DIGIT_M2), LED_DIGIT(LED_DIGIT_S1), LED_DIGIT(LED_DIGIT_S2)
                                  }; //маска разрядов для светодиодов

boolean ledUpdate = 0; //флаг отрисовки светодиодов

uint8_t ledMode[LEDS_NUM]; //массив режимов
uint8_t ledColor[LEDS_NUM]; //массив цветов
uint8_t ledBright[LEDS_NUM]; //массив яркости

const uint8_t ledWhiteTable[][3] = { //таблица оттенков белого RGB
  255, 155, 60,  //2400
  255, 208, 170, //4100
  255, 255, 255  //6500
};

enum {
  WHITE_OFF, //выключить установку белого цвета
  WHITE_ON, //включить установку белого цвета
  HEAT_ON //включить установку температуры цвета
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

    uint8_t diff = 0; //выбор порядка цвета
    uint8_t pallet = 0; //выбор палитры
    uint8_t bright = 0; //текущая яркость

    uint8_t ledBuff[LEDS_NUM * 3]; //массив светодиодов
    uint8_t* ledLink = ledBuff; //ссылка на текущий элемент

#if BACKL_REVERSE
    for (uint8_t led = LEDS_NUM; led;) {
      led--; //сместили номер светодиода
#else
    for (uint8_t led = 0; led < LEDS_NUM; led++) {
#endif
      pallet = ledColor[led];
      bright = ledBright[led];
      diff = ledMode[led];

      switch (diff) {
        case WHITE_OFF: //режим палитры без белого цвета
        case WHITE_ON: //режим палитры с белым цветом
          if ((diff == WHITE_OFF) || (pallet < 253)) {
            diff = 0;

            while (pallet > 85) {
              pallet -= 85;
              diff++;
            }

            if (bright) pallet = (uint8_t)(((uint8_t)(pallet * 3) * bright) >> 8) + 1;
            else pallet = 0;

            switch (diff) {
              case 0:
#if BACKL_COLORS
                *ledLink++ = bright - pallet; //R
                *ledLink++ = pallet; //G
#else
                *ledLink++ = pallet; //G
                *ledLink++ = bright - pallet; //R
#endif
                *ledLink++ = 0; //B
                break;
              case 1:
#if BACKL_COLORS
                *ledLink++ = 0; //R
                *ledLink++ = bright - pallet; //G
#else
                *ledLink++ = bright - pallet; //G
                *ledLink++ = 0; //R
#endif
                *ledLink++ = pallet; //B
                break;
              case 2:
#if BACKL_COLORS
                *ledLink++ = pallet; //R
                *ledLink++ = 0; //G
#else
                *ledLink++ = 0; //G
                *ledLink++ = pallet; //R
#endif
                *ledLink++ = bright - pallet; //B
                break;
            }
          }
          else {
            pallet -= 253;

#if BACKL_COLORS
            *ledLink++ = (uint8_t)((bright * ledWhiteTable[pallet][0]) >> 8); //R
            *ledLink++ = (uint8_t)((bright * ledWhiteTable[pallet][1]) >> 8); //G
#else
            *ledLink++ = (uint8_t)((bright * ledWhiteTable[pallet][1]) >> 8); //G
            *ledLink++ = (uint8_t)((bright * ledWhiteTable[pallet][0]) >> 8); //R
#endif
            *ledLink++ = (uint8_t)((bright * ledWhiteTable[pallet][2]) >> 8); //B
          }
          break;
        case HEAT_ON: //режим температуры цвета
          diff = CONSTRAIN_MAX(pallet, 191);

          pallet = (diff & 0x3F) << 2;
          pallet = (uint8_t)((pallet * bright) >> 8) + 1;

          if (diff >= 0x80) { //теплее
#if BACKL_COLORS
            *ledLink++ = bright; //R
            *ledLink++ = bright; //G
#else
            *ledLink++ = bright; //G
            *ledLink++ = bright; //R
#endif
            *ledLink++ = pallet; //B
          }
          else if (diff >= 0x40) { //нейтрально
#if BACKL_COLORS
            *ledLink++ = bright; //R
            *ledLink++ = pallet; //G
#else
            *ledLink++ = pallet; //G
            *ledLink++ = bright; //R
#endif
            *ledLink++ = 0; //B
          }
          else { //холоднее
#if BACKL_COLORS
            *ledLink++ = pallet; //R
            *ledLink++ = 0; //G
#else
            *ledLink++ = 0; //G
            *ledLink++ = pallet; //R
#endif
            *ledLink++ = 0; //B
          }
          break;
          //default: //режим принудительного отключения
          //  *ledLink++ = 0;
          //  *ledLink++ = 0;
          //  *ledLink++ = 0;
          //  break;
      }
    }

    wsLedWrite(ledBuff, sizeof(ledBuff));
  }
}
//--------------------------------------Очистка светодиодов-----------------------------------------
void wsBacklClearLeds(void)
{
  ledUpdate = 1; //устанавливаем флаг обновления
  for (uint8_t i = 0; i < LEDS_NUM; i++) ledBright[i] = 0;
}
//---------------------------------Установка цвета в формате HV-------------------------------------
void wsBacklSetLedHue(uint8_t _led, uint8_t _color)
{
  if (_led < LEDS_NUM) {
    ledUpdate = 1; //устанавливаем флаг обновления
    ledMode[_led] = WHITE_OFF;
    ledColor[_led] = _color;
  }
}
//---------------------------------Установка цвета в формате HV-------------------------------------
void wsBacklSetLedHue(uint8_t _color)
{
  ledUpdate = 1; //устанавливаем флаг обновления
  for (uint8_t i = 0; i < LEDS_NUM; i++) {
    ledMode[i] = WHITE_OFF;
    ledColor[i] = _color;
  }
}
//---------------------------------Установка цвета в формате HV-------------------------------------
void wsBacklSetLedColor(uint8_t _led, uint8_t _color)
{
  if (_led < LEDS_NUM) {
    ledUpdate = 1; //устанавливаем флаг обновления
    ledMode[_led] = WHITE_ON;
    ledColor[_led] = _color;
  }
}
//---------------------------------Установка цвета в формате HV-------------------------------------
void wsBacklSetLedColor(uint8_t _color)
{
  ledUpdate = 1; //устанавливаем флаг обновления
  for (uint8_t i = 0; i < LEDS_NUM; i++) {
    ledMode[i] = WHITE_ON;
    ledColor[i] = _color;
  }
}
//---------------------------------Установка цвета в формате HV-------------------------------------
void wsBacklSetDigitColor(uint8_t _digit, uint8_t _color)
{
  if (_digit < LAMP_NUM) {
    ledUpdate = 1; //устанавливаем флаг обновления
    ledMode[pgm_read_byte(&ledMask[_digit])] = WHITE_ON;
    ledColor[pgm_read_byte(&ledMask[_digit])] = _color;
  }
}
//---------------------------Установка диапазона цветов в формате HV--------------------------------
void wsBacklSetRangeColor(uint8_t _start, uint8_t _count, uint8_t _color)
{
  ledUpdate = 1; //устанавливаем флаг обновления
  while (_count) {
    if (_start < LAMP_NUM) {
      ledMode[pgm_read_byte(&ledMask[_start])] = WHITE_ON;
      ledColor[pgm_read_byte(&ledMask[_start])] = _color;
      _start++;
    }
    _count--;
  }
}
//------------------------------Установка двух цветов в формате HV----------------------------------
void wsBacklSetMultiColor(uint8_t _start, uint8_t _count, uint8_t _color, uint8_t _color_fill)
{
  wsBacklSetLedColor(_color_fill);
  wsBacklSetRangeColor(_start, _count, _color);
}
//---------------------------------Установка цвета в формате K-------------------------------------
void wsBacklSetHeatColor(uint8_t _led, uint8_t _heat) {
  if (_led < LEDS_NUM) {
    ledUpdate = 1; //устанавливаем флаг обновления
    ledMode[_led] = HEAT_ON;
    ledColor[_led] = _heat;
  }
}
//---------------------------------Установка цвета в формате K-------------------------------------
void wsBacklSetHeatColor(uint8_t _heat) {
  ledUpdate = 1; //устанавливаем флаг обновления
  for (uint8_t i = 0; i < LEDS_NUM; i++) {
    ledMode[i] = HEAT_ON;
    ledColor[i] = _heat;
  }
}
//--------------------------------------Уменьшение яркости------------------------------------------
void wsBacklDecLedsBright(uint8_t _led, uint8_t _step)
{
  ledUpdate = 1; //устанавливаем флаг обновления
  for (uint8_t i = 0; i < LEDS_NUM; i++) {
    if (_led != i) {
      if (ledBright[i] > _step) ledBright[i] -= _step;
      else ledBright[i] = 0;
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
//--------------------------------------Уменьшение яркости------------------------------------------
boolean wsBacklDecLedBright(uint8_t _step, uint8_t _min)
{
  ledUpdate = 1; //устанавливаем флаг обновления
  if (((int16_t)ledBright[0] - _step) > _min) {
    ledBright[0] -= _step;
    for (uint8_t i = 1; i < LEDS_NUM; i++) ledBright[i] = ledBright[0];
  }
  else {
    for (uint8_t i = 0; i < LEDS_NUM; i++) ledBright[i] = _min;
    return 1;
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
//--------------------------------------Увеличение яркости------------------------------------------
boolean wsBacklIncLedBright(uint8_t _step, uint8_t _max)
{
  ledUpdate = 1; //устанавливаем флаг обновления
  if (((uint16_t)ledBright[0] + _step) < _max) {
    ledBright[0] += _step;
    for (uint8_t i = 1; i < LEDS_NUM; i++) ledBright[i] = ledBright[0];
  }
  else {
    for (uint8_t i = 0; i < LEDS_NUM; i++) ledBright[i] = _max;
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
  for (uint8_t i = 0; i < LEDS_NUM; i++) ledBright[i] = _brt;
}
//--------------------------Установка яркости только во включенные диоды---------------------------
void wsBacklSetOnLedBright(uint8_t _brt)
{
  for (uint8_t i = 0; i < LEDS_NUM; i++) {
    if (ledBright[i]) {
      ledUpdate = 1; //устанавливаем флаг обновления
      ledBright[i] = _brt;
    }
  }
}
//--------------------------------------Установка яркости------------------------------------------
void wsBacklSetDigitBright(uint8_t _digit, uint8_t _brt)
{
  if (_digit < LAMP_NUM) {
    ledUpdate = 1; //устанавливаем флаг обновления
    ledBright[pgm_read_byte(&ledMask[_digit])] = _brt;
  }
}
