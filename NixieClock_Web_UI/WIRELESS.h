#define WIRELESS_PACKET_SIZE 9 //длинна пакета данных(9)
#define WIRELESS_LOCAL_PORT 8888 //локальный порт(8888)
#define WIRELESS_SENSOR_PORT 888 //локальный порт(888)

enum {
  WIRELESS_STOPPED, //сервис не запущен
  WIRELESS_SEARCH, //поиск беспроводного датчика
  WIRELESS_ONLINE, //беспроводного датчика в сети
  WIRELESS_OFFLINE, //беспроводного датчика не в сети
  WIRELESS_NOT_SENSOR //сенсор температуры не обнаружен
};
uint8_t wireless_status = WIRELESS_STOPPED; //флаг состояние беспроводного датчика

uint8_t wireless_buffer[WIRELESS_PACKET_SIZE]; //буфер данных с беспроводного датчика
uint32_t wireless_timeout = 0; //таймер ожидания передачи данных с беспроводного датчика

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
    wireless_status = WIRELESS_SEARCH;
  }
  else wireless_status = WIRELESS_STOPPED;
}
void wirelessStop(void) {
  wireless.stop();
  wireless_status = WIRELESS_STOPPED;
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
  if (wireless_status == WIRELESS_ONLINE) return wireless_buffer[5];
  return 0;
}
uint8_t wirelessGetSignal(void) {
  if (wireless_status == WIRELESS_ONLINE) return wireless_buffer[6];
  return 0;
}
uint8_t wirelessGetInterval(void) {
  if (wireless_status == WIRELESS_ONLINE) return wireless_buffer[7];
  return 0;
}

void wirelessUpdate(void) {
  static uint32_t wireless_timer;
  if (wireless_status != WIRELESS_STOPPED) {
    if (wireless.parsePacket() == WIRELESS_PACKET_SIZE) {
      if (wireless.remotePort() == WIRELESS_SENSOR_PORT) {
        if (wireless.read(wireless_buffer, WIRELESS_PACKET_SIZE) == WIRELESS_PACKET_SIZE) {
          uint8_t _crc = 0; //буфер контрольной суммы
          for (uint8_t i = 0; i < WIRELESS_PACKET_SIZE; i++) checkCRC(&_crc, wireless_buffer[i]);
          if (!_crc && wireless_buffer[7]) {
            int16_t _temp = wireless_buffer[0] | ((int16_t)wireless_buffer[1] << 8);
            if (_temp != 0x7FFF) {
              sens.mainTemp[2] = _temp;
              sens.mainPress[2] = wireless_buffer[2] | ((uint16_t)wireless_buffer[3] << 8);
              sens.mainHum[2] = wireless_buffer[4];
              wireless_status = WIRELESS_ONLINE;
            }
            else wireless_status = WIRELESS_NOT_SENSOR;
            wireless_timeout = 120000UL * wireless_buffer[7];
            wireless_timer = millis();
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
