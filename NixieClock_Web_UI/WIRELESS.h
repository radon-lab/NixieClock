#define WIRELESS_PACKET_SIZE 16 //длинна пакета данных(16)
#define WIRELESS_LOCAL_PORT 8888 //локальный порт(8888)
#define WIRELESS_SENSOR_PORT 888 //локальный порт(888)

#define WIRELESS_SET_CMD 0xCC //команда обнаружение нового беспроводного датчика

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

#include <WiFiUdp.h>
WiFiUDP wireless;

void checkCRC(uint8_t* crc, uint8_t data) {
  for (uint8_t i = 0; i < 8; i++) { //считаем для всех бит
    *crc = ((*crc ^ data) & 0x01) ? (*crc >> 0x01) ^ 0x8C : (*crc >> 0x01); //рассчитываем значение
    data >>= 0x01; //сдвигаем буфер
  }
}

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

uint8_t wirelessGetStastus(void) {
  return wireless_status;
}
boolean wirelessGetOnlineStastus(void) {
  return (wireless_status == WIRELESS_ONLINE);
}
boolean wirelessGetSensorStastus(void) {
  return (wireless_status >= WIRELESS_ONLINE);
}

uint8_t wirelessGetBattery(void) {
  if (wireless_status == WIRELESS_ONLINE) return wireless_battery;
  return wireless_battery;
}
uint8_t wirelessGetSignal(void) {
  if (wireless_status == WIRELESS_ONLINE) return wireless_signal;
  return 0;
}
uint8_t wirelessGetInterval(void) {
  if (wireless_status == WIRELESS_ONLINE) return wireless_interval;
  return 0;
}

String wirelessGetFoundId(void) {
  String _id = "";
  for (uint8_t i = 0; i < 6; i++) {
    _id +=  String(wireless_found_buffer[i], HEX);
  }
  _id.toUpperCase();
  return _id;
}

void wirelessSetData(uint8_t* _buff) {
  int16_t _temp = _buff[7] | ((int16_t)_buff[8] << 8);
  if (_temp != 0x7FFF) {
    sens.mainTemp[2] = _temp;
    sens.mainPress[2] = _buff[9] | ((uint16_t)_buff[10] << 8);
    sens.mainHum[2] = _buff[11];
    wireless_battery = _buff[12];
    wireless_signal = _buff[13];
    wireless_interval = _buff[14];
    wireless_status = WIRELESS_ONLINE;
  }
  else wireless_status = WIRELESS_NOT_SENSOR;
  wireless_timeout = 120000UL * _buff[14];
  wireless_timer = millis();
}
void wirelessCopyData(void) {
  if (wireless_found != true) {
    wireless_found = true;
    for (uint8_t i = 0; i < WIRELESS_PACKET_SIZE; i++) {
      wireless_found_buffer[i] = wireless_buffer[i];
    }
  }
}
boolean wirelessCheckData(void) {
  uint8_t _crc = 0; //буфер контрольной суммы
  for (uint8_t i = 0; i < WIRELESS_PACKET_SIZE; i++) checkCRC(&_crc, wireless_buffer[i]);
  return (boolean)!_crc;
}

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

void wirelessUpdate(void) {
  if (wireless_status != WIRELESS_STOPPED) {
    if (wireless.parsePacket() == WIRELESS_PACKET_SIZE) {
      if (wireless.remotePort() == WIRELESS_SENSOR_PORT) {
        if (wireless.read(wireless_buffer, WIRELESS_PACKET_SIZE) == WIRELESS_PACKET_SIZE) {
          if (wirelessCheckData() && wireless_buffer[6]) {
            if (wirelessCheckId()) wirelessSetData(wireless_buffer);
            else if (wireless_buffer[6] == WIRELESS_SET_CMD) wirelessCopyData();
          }
        }
      }
    }
    if (wireless_timeout && ((millis() - wireless_timer) >= wireless_timeout)) {
      wireless_timeout = 0;
      wireless_status = WIRELESS_OFFLINE;
    }
  }
}