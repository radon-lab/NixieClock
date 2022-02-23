boolean initDS = 0; //флаг инициализации датчика

//-----------------------------------Сигнал сброса шины--------------------------------------------
boolean oneWireReset(void)
{
  SENS_OUT;
  SENS_LO;
  _delay_us(520);
  SENS_INP;
  SENS_HI;
  _delay_us(2);
  for (uint8_t c = 80; c; c--) {
    if (!SENS_CHK) {
      for (uint8_t i = 200; !SENS_CHK && i; i--) _delay_us(1);
      return 0;
    }
    _delay_us(1);
  }
  return 1;
}
//----------------------------------Отправка данных в шину-----------------------------------------
void oneWireWrite(uint8_t data)
{
  SENS_OUT;
  for (uint8_t i = 0; i < 8; i++) {
    if ((data >> i) & 0x01) {
      SENS_LO;
      _delay_us(5);
      SENS_HI;
      _delay_us(60);
    }
    else {
      SENS_LO;
      _delay_us(60);
      SENS_HI;
      _delay_us(5);
    }
  }
  SENS_INP;
}
//-----------------------------------------Чтение шины--------------------------------------------
uint8_t oneWireRead(void)
{
  uint8_t data = 0;
  for (uint8_t i = 0; i < 8; i++) {
    SENS_OUT;
    SENS_LO;
    _delay_us(2);
    SENS_HI;
    SENS_INP;
    _delay_us(8);
    if (SENS_CHK) data |= (0x01 << i);
    _delay_us(60);
  }
  return data;
}
//---------------------------------------Запрос температуры-----------------------------------------
void requestTemp(void)
{
  if (oneWireReset()) return;
  oneWireWrite(0xCC);
  oneWireWrite(0x44);
}
//-----------------------------------Установка разрешения датчика-----------------------------------
void setResolution(void)
{
  if (oneWireReset()) return;
  oneWireWrite(0xCC);
  oneWireWrite(0x4E);
  oneWireWrite(0xFF);
  oneWireWrite(0x00);
  oneWireWrite(0x1F);
}
//--------------------------------------Чтение температуры------------------------------------------
void readTempDS(void)
{
  if (!initDS) {
    initDS = 1;
    SENS_INIT; //инициализируем датчик
    setResolution(); //устанавливаем разрешение датчика
  }

  if (_timer_ms[TMR_SENS]) return;

  requestTemp(); //запрашиваем температуру
  for (_timer_ms[TMR_SENS] = 100; _timer_ms[TMR_SENS];) ; //ждем

  _timer_ms[TMR_SENS] = 1000;

  if (oneWireReset()) {
    readTempRTC(); //читаем температуру DS3231
    return; //выходим
  }

  oneWireWrite(0xCC);
  oneWireWrite(0xBE);

  uint16_t raw = (uint16_t)(oneWireRead() | (oneWireRead() << 8));
  if (raw & 0x8000) raw = 0; //если значение ниже нуля

  sens.temp = (raw * 100) >> 1;
  sens.temp -= sens.temp % 50;
  sens.press = 0;
  sens.hum = 0;
}
