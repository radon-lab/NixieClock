#define WIRELESS_PACKET_SIZE 16 //длинна пакета данных(16)
#define WIRELESS_LOCAL_PORT 8888 //локальный порт(8888)
#define WIRELESS_SENSOR_PORT 888 //внешний порт(888)

#define WIRELESS_SET_CMD 0xCC //команда обнаружение нового беспроводного датчика(0xCC)
#define WIRELESS_ANSW_CMD 0xAA //команда отправки ответа беспроводному датчику(0xAA)

enum {
  WIRELESS_STOPPED, //сервис не запущен
  WIRELESS_SEARCH, //поиск беспроводного датчика
  WIRELESS_ONLINE, //беспроводного датчика в сети
  WIRELESS_OFFLINE, //беспроводного датчика не в сети
  WIRELESS_NOT_SENSOR //сенсор температуры не обнаружен
};
uint8_t wireless_status = WIRELESS_STOPPED; //флаг состояние беспроводного датчика

uint8_t wireless_buffer[WIRELESS_PACKET_SIZE]; //буфер данных с беспроводного датчика
uint8_t wireless_found_buffer[WIRELESS_PACKET_SIZE]; //буфер данных с нового беспроводного датчика
uint32_t wireless_timer = 0; //таймер ожидания передачи данных с беспроводного датчика
uint32_t wireless_timeout = 0; //интервал ожидания передачи данных с беспроводного датчика

boolean wireless_found = false; //флаг обнаружения нового беспроводного датчика
boolean wireless_found_success = false; //флаг подтверждения добавления нового беспроводного датчика

uint8_t wireless_signal = 0; //уровень сигнала беспроводного датчика
uint8_t wireless_battery = 0; //заряд батареи беспроводного датчика
uint8_t wireless_interval = 0; //интервал передачи беспроводного датчика

const char *wirelessStatusList[] = {"Ошибка...", "Не обнаружен...", "Подключен", "Потеряна связь...", "Нет сенсора..."};

#include <WiFiUdp.h>
WiFiUDP wireless;

//--------------------------------------------------------------------
void wirelessStart(void) {
  if (wireless.begin(WIRELESS_LOCAL_PORT)) {
    wireless_timeout = 0;
    wireless_found = false;
    wireless_status = WIRELESS_SEARCH;
  }
  else wireless_status = WIRELESS_STOPPED;
}
void wirelessStop(void) {
  wireless.stop();
  wireless_status = WIRELESS_STOPPED;
}
//--------------------------------------------------------------------
void wirelessResetFoundState(void) {
  wireless_found = false;
  wireless_found_success = false;
}
boolean wirelessGetFoundState(void) {
  return wireless_found;
}
boolean wirelessGetFoundSuccessState(void) {
  return wireless_found_success;
}
//--------------------------------------------------------------------
uint8_t wirelessGetStastus(void) {
  return wireless_status;
}
boolean wirelessGetOnlineStastus(void) {
  return (wireless_status == WIRELESS_ONLINE);
}
boolean wirelessGetSensorStastus(void) {
  return (wireless_status >= WIRELESS_ONLINE);
}
//--------------------------------------------------------------------
const char* wirelessGetStrStastus(void) {
  return wirelessStatusList[wirelessGetStastus()];
}
//--------------------------------------------------------------------
uint8_t wirelessGetBattery(void) {
  if (wireless_status >= WIRELESS_ONLINE) return wireless_battery;
  return wireless_battery;
}
uint8_t wirelessGetSignal(void) {
  if (wireless_status >= WIRELESS_ONLINE) return wireless_signal;
  return 0;
}
uint8_t wirelessGetInterval(void) {
  if (wireless_status >= WIRELESS_ONLINE) return wireless_interval;
  return 0;
}
//--------------------------------------------------------------------
char wirelessGetHexChar(uint8_t hex) {
  if (hex > 15) return 'F';
  if (hex > 9) return ('A' + (hex - 10));
  return ('0' + hex);
}
String wirelessGetId(uint8_t* id) {
  String str;
  str.reserve(20);
  str = "";

  for (uint8_t i = 0; i < 6; i++) {
    str += wirelessGetHexChar(id[i] >> 4);
    str += wirelessGetHexChar(id[i] & 0x0F);
    if (i < 5) str += ':';
  }

  return str;
}
//--------------------------------------------------------------------
void wirelessResetData(void) {
  sens.temp[SENS_WIRELESS] = 0x7FFF;
  sens.press[SENS_WIRELESS] = 0;
  sens.hum[SENS_WIRELESS] = 0;
}
void wirelessCopyData(void) {
  if (wireless_found != true) {
    wireless_found = true;
    for (uint8_t i = 0; i < WIRELESS_PACKET_SIZE; i++) {
      wireless_found_buffer[i] = wireless_buffer[i];
    }
  }
}
boolean wirelessSetData(uint8_t* _buff) {
  boolean _update = false;
  int16_t _temp = _buff[7] | ((int16_t)_buff[8] << 8);
  if (_temp != 0x7FFF) {
    uint16_t _press = _buff[9] | ((uint16_t)_buff[10] << 8);
    if (((millis() - wireless_timer) >= 30000) || (sens.temp[SENS_WIRELESS] != _temp) || (sens.press[SENS_WIRELESS] != _press) || (sens.hum[SENS_WIRELESS] != _buff[11])) _update = true;
    sens.temp[SENS_WIRELESS] = _temp;
    sens.press[SENS_WIRELESS] = _press;
    sens.hum[SENS_WIRELESS] = _buff[11];
    wireless_status = WIRELESS_ONLINE;
  }
  else {
    _update = true;
    wirelessResetData();
    wireless_status = WIRELESS_NOT_SENSOR;
  }
  wireless_battery = _buff[12];
  wireless_signal = _buff[13];
  wireless_interval = _buff[14];
  wireless_timeout = 120000UL * (_buff[14] + 1);
  wireless_timer = millis();
  return _update;
}
boolean wirelessCheckData(void) {
  uint8_t _crc = 0; //буфер контрольной суммы
  for (uint8_t i = 0; i < WIRELESS_PACKET_SIZE; i++) checkCRC(&_crc, wireless_buffer[i]);
  return (boolean)!_crc;
}
//--------------------------------------------------------------------
void wirelessSetNewId(void) {
  wireless_found_success = true;
  for (uint8_t i = 0; i < 6; i++) {
    settings.wirelessId[i] = wireless_found_buffer[i];
  }
}
boolean wirelessCheckId(void) {
  for (uint8_t i = 0; i < 6; i++) {
    if (wireless_buffer[i] != settings.wirelessId[i]) return 0;
  }
  return 1;
}
//--------------------------------------------------------------------
void wirelessSendAnswer(uint8_t answ) {
  wireless.beginPacket(wireless.remoteIP(), wireless.remotePort());
  wireless.write(answ);
  wireless.endPacket();
}
//--------------------------------------------------------------------
boolean wirelessUpdate(void) {
  if (wireless_status != WIRELESS_STOPPED) {
    if (wireless.parsePacket() == WIRELESS_PACKET_SIZE) {
      if (wireless.remotePort() == WIRELESS_SENSOR_PORT) {
        if (wireless.read(wireless_buffer, WIRELESS_PACKET_SIZE) == WIRELESS_PACKET_SIZE) {
          if (wirelessCheckData() && wireless_buffer[6]) {
            if (wireless_buffer[6] == WIRELESS_ANSW_CMD) wirelessSendAnswer(WIRELESS_ANSW_CMD);
            if (wirelessCheckId()) return wirelessSetData(wireless_buffer);
            else if (wireless_buffer[6] == WIRELESS_SET_CMD) wirelessCopyData();
          }
        }
      }
    }
    if (wireless_timeout && ((millis() - wireless_timer) >= wireless_timeout)) {
      wirelessResetData();
      wireless_timeout = 0;
      wireless_status = WIRELESS_OFFLINE;
      return true;
    }
  }
  return false;
}
