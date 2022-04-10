boolean initDS = 0; //флаг инициализации датчика
boolean typeDS = 1; //тип датчика температуры

#define SKIP_ROM 0xCC //пропуск адресации
#define READ_ROM 0x33 //чтение адреса
#define CONVERT_T 0x44 //запрос на преобразование температуры
#define WRITE_STRATCHPAD 0x4E //запись в память
#define READ_STRATCHPAD 0xBE //чтенеие памяти

#define DS_CHECK_TIME 120     //время ожидания нового замера(мс)
#define DS_RESET_TIME 520     //длительность сигнала сброса(мкс)
#define DS_RESET_WAIT_TIME 80 //длительность ожидания сигнала присутствия(мкс)
#define DS_PRESENCE_TIME 200  //длительность сигнала присутствия(мкс)
#define DS_HIGH_TIME 60       //максимальное время слота(мкс)
#define DS_LOW_TIME 5         //минимальное время слота(мкс)

//-----------------------------------Сигнал сброса шины--------------------------------------------
boolean oneWireReset(void)
{
  SENS_LO;
  _delay_us(DS_RESET_TIME);
  SENS_HI;
  _delay_us(2);
  for (uint8_t c = DS_RESET_WAIT_TIME; c; c--) {
    if (!SENS_CHK) {
      for (uint8_t i = DS_PRESENCE_TIME; !SENS_CHK && i; i--) _delay_us(1);
      return 0;
    }
    _delay_us(1);
  }
  return 1;
}
//----------------------------------Отправка данных в шину-----------------------------------------
void oneWireWrite(uint8_t data)
{
  for (uint8_t i = 0; i < 8; i++) {
    if ((data >> i) & 0x01) {
      SENS_LO;
      _delay_us(DS_LOW_TIME);
      SENS_HI;
      _delay_us(DS_HIGH_TIME);
    }
    else {
      SENS_LO;
      _delay_us(DS_HIGH_TIME);
      SENS_HI;
      _delay_us(DS_LOW_TIME);
    }
  }
}
//-----------------------------------------Чтение шины--------------------------------------------
uint8_t oneWireRead(void)
{
  uint8_t data = 0;
  for (uint8_t i = 0; i < 8; i++) {
    SENS_LO;
    _delay_us(2);
    SENS_HI;
    _delay_us(8);
    if (SENS_CHK) data |= (0x01 << i);
    _delay_us(DS_HIGH_TIME);
  }
  return data;
}
//---------------------------------------Запрос температуры-----------------------------------------
void requestTemp(void)
{
  if (oneWireReset()) return;
  oneWireWrite(SKIP_ROM); //пропуск адресации
  oneWireWrite(CONVERT_T); //запрос на преобразование температуры
}
//-----------------------------------Установка разрешения датчика-----------------------------------
void setResolution(void)
{
  if (oneWireReset()) return;
  oneWireWrite(SKIP_ROM); //пропуск адресации
  oneWireWrite(WRITE_STRATCHPAD); //запись в память
  oneWireWrite(0xFF); //устанавливаем разрешение
  oneWireWrite(0x00);
  oneWireWrite(0x1F);
}
//-------------------------------------Чтение семейства датчика-------------------------------------
boolean readSensCode(void)
{
  if (oneWireReset()) return 1;
  oneWireWrite(READ_ROM); //пропуск адресации
  return (oneWireRead() != 0x10); //возрщаем тип датчика
}
//--------------------------------------Чтение температуры------------------------------------------
void readTempDS(void)
{
  if (!initDS) {
    initDS = 1;
    SENS_INIT; //инициализируем датчик
    if (readSensCode()) {
      setResolution(); //устанавливаем разрешение датчика
      typeDS = 1; //датчик DS18B20
    }
    else typeDS = 0; //датчик DS18S20
  }

  requestTemp(); //запрашиваем температуру
  for (_timer_ms[TMR_SENS] = DS_CHECK_TIME; _timer_ms[TMR_SENS];) dataUpdate(); //ждем

  if (oneWireReset()) {
    readTempRTC(); //читаем температуру DS3231
    return; //выходим
  }

  oneWireWrite(SKIP_ROM); //пропуск адресации
  oneWireWrite(READ_STRATCHPAD); //чтение памяти

  uint16_t raw = oneWireRead() | ((uint16_t)oneWireRead() << 8); //читаем сырое значение
  if (raw & 0x8000) raw = 0;

  switch (typeDS) {
    case 0: sens.temp = raw * 50; break; //переводим в температуру для DS18S20
    case 1: sens.temp = (raw * 100) >> 4; break; //переводим в температуру для DS18B20
  }
  sens.press = 0; //сбросили давление
  sens.hum = 0; //сбросили влажность
  sens.err = 0; //сбросили ошибку датчика температуры
}
