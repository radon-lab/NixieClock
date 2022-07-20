#define RDA_ADDR 0x11 //адрес радио RDA5807M

#define RDA_CONFIG_REG 0x02 //регистр основных настроек
#define RDA_TUNING_REG 0x03 //регистр настройки частоты
#define RDA_VOLUME_REG 0x05 //регистр управления громкостью
#define RDA_STATUS_REG 0x0A //регистр статуса
#define RDA_SIGNAL_REG 0x0B //регистр сигнала

#define RDA_SEEK_DOWN 0x00 //поиск вниз
#define RDA_SEEK_UP 0x01 //поиск вверх
#define RDA_MUTE_OFF 0x00 //включить приглушение звука
#define RDA_MUTE_ON 0x01 //выключить приглушение звука
#define RDA_ERROR 0xFF //ошибка связи с радиоприемником
#define RDA_OFF 0x00 //радио выключено
#define RDA_ON 0x01 //радио включено

#define RADIO_MIN_FREQ 870 //минимальная частота(870..1080)
#define RADIO_MAX_FREQ 1080 //максимальная частота(870..1080)
#define RADIO_MIN_VOL 0 //минмальная громкость(0..15)
#define RADIO_MAX_VOL 15 //максимальная громкость(1..15)

struct rdaData { //буфер обмена радио
  uint8_t highReg; //старший байт
  uint8_t lowReg; //младший байт
} rda;

//------------------------------------------Запись в регистр--------------------------------------------------
boolean readRegRDA(uint8_t _reg)
{
  if (wireRequestFrom(RDA_ADDR, _reg)) return 1; //запрашиваем чтение данных, если нет ответа то выходим
  rda.highReg = wireRead(); //читаем старший байт
  rda.lowReg = wireReadEndByte(); //читаем младший байт
  return 0;
}
//-----------------------------------------Чтение из регистра-------------------------------------------------
void writeRegRDA(uint8_t _reg)
{
  if (wireBeginTransmission(RDA_ADDR)) return; //начало передачи
  wireWrite(_reg); //устанавливаем адрес записи
  wireWrite(rda.highReg); //отправляем старший байт
  wireWrite(rda.lowReg); //отправляем младший байт
  wireEnd(); //конец передачи
}
//-------------------------------Получить статус настройки на радиостанцию------------------------------------
boolean getStationStatusRDA(void)
{
  if (readRegRDA(RDA_SIGNAL_REG)) return 0; //запрашиваем чтение данных, если нет ответа то выходим
  return (rda.highReg & 0x01); //читаем бит FM TRUE
}
//--------------------------Получить статус успешной настройки на радиостанцию--------------------------------
boolean getTuneStatusRDA(void)
{
  if (readRegRDA(RDA_TUNING_REG)) return 0; //запрашиваем чтение данных, если нет ответа то выходим
  return (rda.lowReg & 0x10); //читаем бит TUNE
}
//--------------------------Получить статус успешной автопоиска радиостанции----------------------------------
boolean getSeekStatusRDA(void)
{
  if (readRegRDA(RDA_CONFIG_REG)) return 0; //запрашиваем чтение данных, если нет ответа то выходим
  return (rda.highReg & 0x01); //читаем бит SEEK
}
//-----------------------------Получить статус ошибки поиска радиостанции-------------------------------------
boolean getSeekFailRDA(void)
{
  if (readRegRDA(RDA_STATUS_REG)) return 0; //запрашиваем чтение данных, если нет ответа то выходим
  return (rda.highReg & 0x20); //читаем бит SF
}
//-----------------------------Очистить статус ошибки поиска радиостанции-------------------------------------
void clrSeekFailRDA(void)
{
  if (readRegRDA(RDA_STATUS_REG)) return; //запрашиваем чтение данных, если нет ответа то выходим
  rda.highReg &= 0xDF; //очищаем бит SF
  writeRegRDA(RDA_STATUS_REG); //отправляем данные
}
//--------------------------Получить статус успешной настройки на радиостанцию--------------------------------
boolean getSeekCompleteStatusRDA(void)
{
  if (readRegRDA(RDA_STATUS_REG)) return 0; //запрашиваем чтение данных, если нет ответа то выходим
  return (rda.highReg & 0x40); //читаем бит STC
}
//--------------------------Очистить статус успешной настройки на радиостанцию--------------------------------
void clrSeekCompleteStatusRDA(void)
{
  if (readRegRDA(RDA_STATUS_REG)) return; //запрашиваем чтение данных, если нет ответа то выходим
  rda.highReg &= 0xBF; //очищаем бит STC
  writeRegRDA(RDA_STATUS_REG); //отправляем данные
}
//----------------------------------Получить статус питания модуля радио--------------------------------------
uint8_t getPowerStatusRDA(void)
{
  if (readRegRDA(RDA_CONFIG_REG)) return 255; //запрашиваем чтение данных, если нет ответа то выходим
  return (rda.lowReg & 0x01); //читаем бит ENABLE
}
//-------------------------------------Установить питание модуля радио----------------------------------------
void setPowerRDA(boolean _pwr)
{
  rda.highReg = (_pwr) ? (0xC0 | (RADIO_MONO << 5) | (RADIO_BASS << 4)) : 0x00; //записываем настройку
  rda.lowReg = (_pwr) ? 0x81 : 0x00; //записываем настройку
  writeRegRDA(RDA_CONFIG_REG); //отправляем данные
#if AMP_PORT_ENABLE
  if (_pwr) AMP_ENABLE;
  else AMP_DISABLE;
#endif
}
//-------------------------------------Установить частоту радиостанции----------------------------------------
void setFreqRDA(uint16_t _freq)
{
  _freq = ((_freq - 870) << 6) | 0x10; //устанавливаем биты CHAN
  rda.highReg = (uint8_t)(_freq >> 8); //записываем настройку
  rda.lowReg = (uint8_t)_freq; //записываем настройку
  writeRegRDA(RDA_TUNING_REG); //отправляем данные
}
//--------------------------------------Получить частоту радиостанции-----------------------------------------
uint16_t getFreqRDA(void)
{
  if (readRegRDA(RDA_TUNING_REG)) return 0; //запрашиваем чтение данных, если нет ответа то выходим
  return (((uint16_t)rda.highReg << 2) | (rda.lowReg >> 6)) + 870; //читаем биты CHAN
}
//----------------------------------------Начать поиск радиостанции--------------------------------------------
void startSeekRDA(boolean _direct)
{
  if (readRegRDA(RDA_CONFIG_REG)) return; //запрашиваем чтение данных, если нет ответа то выходим
  rda.highReg = (rda.highReg & 0xFD) | ((_direct) ? 0x02 : 0x00) | 0x01; //записываем настройку
  writeRegRDA(RDA_CONFIG_REG); //отправляем данные
}
//--------------------------------------Остановить поиск радиостанции------------------------------------------
void stopSeekRDA(void)
{
  if (readRegRDA(RDA_CONFIG_REG)) return; //запрашиваем чтение данных, если нет ответа то выходим
  rda.highReg = (rda.highReg & 0xFE); //записываем настройку
  writeRegRDA(RDA_CONFIG_REG); //отправляем данные
}
//---------------------------------------Установить громкость звука-------------------------------------------
void setVolumeRDA(uint8_t _vol)
{
  if (readRegRDA(RDA_VOLUME_REG)) return; //запрашиваем чтение данных, если нет ответа то выходим
  rda.highReg = (rda.highReg & 0xF0) | RADIO_SEEK_SNR; //установили порог SNR
  rda.lowReg = (rda.lowReg & 0xF0) | (_vol & 0x0F); //записываем настройку
  writeRegRDA(RDA_VOLUME_REG); //отправляем данные
}
//--------------------------------------Установить приглушение звука-------------------------------------------
void setMuteRDA(boolean _mute)
{
  if (readRegRDA(RDA_CONFIG_REG)) return; //запрашиваем чтение данных, если нет ответа то выходим
  rda.highReg = (rda.highReg & 0xBF) | ((_mute) ? 0x00 : 0x40); //записываем настройку
  writeRegRDA(RDA_CONFIG_REG); //отправляем данные
}
