#include <EEPROM.h>

#define MEM_UPDATE_TIME 3000 //период обновления данных в памяти(1000..5000)(мс)
#define MEM_MASK_CRC 0xAA //маска контрольной суммы(0xAA)

struct settingsData { //структура настроек
  uint8_t sensor;
  uint8_t period;
  char ssid[64];
  char pass[64];
  uint8_t attempt[MAX_CLOCK];
  IPAddress send[MAX_CLOCK];
} settings;

boolean memory_state = false; //флаг инициализации памяти настроек
boolean memory_update = false; //флаг обновления настроек в памяти
uint32_t memory_timer = 0; //таймер обновления настроек в памяти

//--------------------------------------------------------------------
void checkCRC(uint8_t* crc, uint8_t data) {
  for (uint8_t i = 0; i < 8; i++) { //считаем для всех бит
    *crc = ((*crc ^ data) & 0x01) ? (*crc >> 0x01) ^ 0x8C : (*crc >> 0x01); //рассчитываем значение
    data >>= 0x01; //сдвигаем буфер
  }
}
//--------------------------------------------------------------------
void memorySaveSettings(void) {
  if (memory_state) {
    memory_update = true;
    memory_timer = millis();
  }
}
//--------------------------------------------------------------------
void memoryWriteSettings(void) {
  memory_update = false;
  if (memory_state) {
    uint8_t _crc = 0;
    uint8_t _buff = 0;
    for (uint16_t i = 0; i < sizeof(settings); i++) {
      _buff = *((uint8_t*)&settings + i);
      checkCRC(&_crc, _buff);
      EEPROM.write(i, _buff);
    }
    EEPROM.write(sizeof(settings) + 1, _crc ^ MEM_MASK_CRC);
    EEPROM.commit();
  }
}
//--------------------------------------------------------------------
void memoryReadSettings(void) {
  EEPROM.begin(sizeof(settings) + 1);
  if (EEPROM.length() >= (sizeof(settings) + 1)) {
    uint8_t _crc = 0;
    memory_state = true;
    for (uint16_t i = 0; i < sizeof(settings); i++) checkCRC(&_crc, EEPROM.read(i));
    if (_crc == (EEPROM.read(sizeof(settings) + 1) ^ MEM_MASK_CRC)) {
      for (uint16_t i = 0; i < sizeof(settings); i++) {
        *((uint8_t*)&settings + i) = EEPROM.read(i);
      }
    }
    else memoryWriteSettings();
#if DEBUG_MODE
    Serial.println F("Memory settings init");
#endif
  }
  else {
    memory_state = false;
#if DEBUG_MODE
    Serial.println F("Memory settings error");
#endif
  }
  memory_update = false;
}
//--------------------------------------------------------------------
void memoryUpdate(void) {
  if (memory_update && ((millis() - memory_timer) >= MEM_UPDATE_TIME)) {
    memoryWriteSettings();
  }
}
