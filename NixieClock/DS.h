#define SKIP_ROM 0xCC //пропуск адресации
#define READ_ROM 0x33 //чтение адреса
#define CONVERT_T 0x44 //запрос на преобразование температуры
#define WRITE_STRATCHPAD 0x4E //запись в память
#define READ_STRATCHPAD 0xBE //чтенеие памяти

#define DS18S20_ADDR 0x10 //идентификатор датчика DS18S20/DS1820
#define DS18B20_ADDR 0x28 //идентификатор датчика DS18B20

#define DS_CONVERT_TIME 120   //время ожидания нового замера(95..150)(мс)
#define DS_RESET_TIME 520     //длительность сигнала сброса(480..700)(мкс)
#define DS_RESET_WAIT_TIME 80 //длительность ожидания сигнала присутствия(50..150)(мкс)
#define DS_PRESENCE_TIME 250  //длительность сигнала присутствия(200..300)(мкс)
#define DS_SLOT_MAX_TIME 60   //максимальное время слота(60..120)(мкс)
#define DS_SLOT_MIN_TIME 5    //минимальное время слота(1..15)(мкс)

//-----------------------------------Сигнал сброса шины--------------------------------------------
boolean oneWireReset(void)
{
  SENS_LO;
  _delay_us(DS_RESET_TIME);
  SENS_HI;
  _delay_us(2);
  for (uint8_t c = DS_RESET_WAIT_TIME; c; c--) {
    if (!SENS_CHK) {
      for (uint8_t i = DS_PRESENCE_TIME; i; i--) {
        if (SENS_CHK) return 0;
        _delay_us(1);
      }
      return 1;
    }
    _delay_us(1);
  }
  return 1;
}
//----------------------------------Отправка данных в шину-----------------------------------------
void oneWireWrite(uint8_t data)
{
  for (uint8_t i = 0; i < 8; i++) {
    if (data & 0x01) {
      SENS_LO;
      _delay_us(DS_SLOT_MIN_TIME);
      SENS_HI;
      _delay_us(DS_SLOT_MAX_TIME);
    }
    else {
      SENS_LO;
      _delay_us(DS_SLOT_MAX_TIME);
      SENS_HI;
      _delay_us(DS_SLOT_MIN_TIME);
    }
    data >>= 1;
  }
}
//-----------------------------------------Чтение шины--------------------------------------------
uint8_t oneWireRead(void)
{
  uint8_t data = 0;
  for (uint8_t i = 0; i < 8; i++) {
    data >>= 1;
    SENS_LO;
    _delay_us(2);
    SENS_HI;
    _delay_us(8);
    if (SENS_CHK) data |= 0x80;
    _delay_us(DS_SLOT_MAX_TIME);
  }
  return data;
}
//---------------------------------------Запрос температуры-----------------------------------------
boolean requestTemp(void)
{
  if (oneWireReset()) return 1; //выходим
  oneWireWrite(SKIP_ROM); //пропуск адресации
  oneWireWrite(CONVERT_T); //запрос на преобразование температуры
  return 0; //выходим
}
//---------------------------------------Чтение температуры-----------------------------------------
boolean readTemp(void)
{
  if (oneWireReset()) return 1; //выходим
  oneWireWrite(SKIP_ROM); //пропуск адресации
  oneWireWrite(READ_STRATCHPAD); //чтение памяти
  return 0; //выходим
}
//-----------------------------------Установка разрешения датчика-----------------------------------
boolean setResolution(void)
{
  if (oneWireReset()) return 1; //выходим
  oneWireWrite(SKIP_ROM); //пропуск адресации
  oneWireWrite(WRITE_STRATCHPAD); //запись в память
  oneWireWrite(0xFF); //устанавливаем разрешение
  oneWireWrite(0x00);
  oneWireWrite(0x1F);
  return 0; //выходим
}
//-------------------------------------Чтение семейства датчика-------------------------------------
uint8_t readSensCode(void)
{
  if (oneWireReset()) return 0; //выходим
  oneWireWrite(READ_ROM); //пропуск адресации
  return oneWireRead(); //возвращаем тип датчика
}
//--------------------------------------Чтение температуры------------------------------------------
void readTempDS(void)
{
  static uint8_t typeDS; //тип датчика DS

  if (!typeDS) { //если тип датчика не определен
    typeDS = readSensCode(); //читаем тип датчика
    switch (typeDS) {
      case DS18S20_ADDR: break; //датчик DS18S20
      case DS18B20_ADDR: if (setResolution()) return; break; //датчик DS18B20
      default: typeDS = 0; return; //выходим
    }
  }

  if (requestTemp()) return; //запрашиваем температуру, если нет ответа выходим
  for (_timer_ms[TMR_SENS] = DS_CONVERT_TIME; _timer_ms[TMR_SENS];) dataUpdate(); //ждем окончания преобразования
  if (readTemp()) return; //чтаем температуру, если нет ответа выходим

  int16_t raw = oneWireRead() | ((uint16_t)oneWireRead() << 8); //читаем сырое значение

  switch (typeDS) {
    case DS18S20_ADDR: sens.temp = raw * 5; break; //переводим в температуру для DS18S20
    case DS18B20_ADDR: sens.temp = (raw * 10) / 16; break; //переводим в температуру для DS18B20
  }
  if (sens.temp > 850) sens.temp = 850;
  sens.press = 0; //сбросили давление
  sens.hum = 0; //сбросили влажность
  sens.err = 0; //сбросили ошибку датчика температуры
}
