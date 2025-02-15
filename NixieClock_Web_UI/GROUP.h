#define GROUP_PACKET_SIZE 24 //длинна всего пакета данных(24)
#define GROUP_DATA_SIZE 20 //длинна информации в пакете данных(20)
#define GROUP_LOCAL_PORT 8889 //локальный порт(8889)

#define GROUP_WAIT_TIME 7000 //время ожидания обновления группы(1000..60000)(мс)
#define GROUP_UPDATE_TIME 1000 //время между попытками обновления группы(100..2000)(мс)

#define GROUP_SEND_CMD 0xAA //команда отправки ответа на запрос(0xCC)
#define GROUP_UPDATE_CMD 0xCC //команда обновления списка устройств(0xCC)
#define GROUP_SEARCH_CMD 0xEE //команда поиска устройств(0xEE)

enum {
  GROUP_STOPPED, //сервис не запущен
  GROUP_READY, //ожидание подключения к локальной сети
  GROUP_UPDATE_WAIT, //ожидание поиска часов в локальной сети
  GROUP_UPDATE_START, //начало поиска часов в локальной сети
  GROUP_UPDATE_END, //окончание поиска часов в локальной сети
};
uint8_t group_status = GROUP_STOPPED; //флаг состояние группового управления

boolean group_update = false; //флаг обновления списка группы
boolean group_check = false; //флаг проверки списка группы

uint8_t group_packet[GROUP_PACKET_SIZE]; //входящий пакет данных

uint16_t group_time = 0; //время ожидания нового запроса состояния группы
uint32_t group_timer = 0; //таймер ожидания нового запроса состояния группы

String group_list; //список устройств в группе
String group_buffer; //временный буфер списока устройств в группе

uint16_t group_list_pos; //текущая позиция парсинга строки группы
String group_list_ip; //текущий адрес устройства
String group_list_name; //текщее имя устройства

IPAddress wifiGetBroadcastIP(void);
boolean wifiGetConnectStatus(void);

#include <WiFiUdp.h>
WiFiUDP group;

//--------------------------------------------------------------------
void groupInitStr(void) {
  group_buffer.reserve(200);
  group_list.reserve(200);
  group_list_ip.reserve(20);
  group_list_name.reserve(20);
  group_buffer = "";
  group_list = "";
  group_list_ip = "";
  group_list_name = "";
}
//--------------------------------------------------------------------
boolean groupGetUpdateStatus(void) {
  return group_update;
}
boolean groupGetSearchStatus(void) {
  return (group_status != GROUP_STOPPED);
}
//--------------------------------------------------------------------
boolean groupResetList(void) {
  group_list_pos = 0;
  group_update = false;
  if (group_status == GROUP_STOPPED) return false;
  return (group_list[0] != '\0');
}
boolean groupNextList(void) {
  int16_t _start = group_list.indexOf(':', group_list_pos);
  int16_t _end = group_list.indexOf(',', group_list_pos);

  if (_start < 0) return false;

  group_list_ip = group_list.substring(group_list_pos, _start);
  group_list_name = group_list.substring(_start + 1, _end);

  group_list_pos = _end + 1;

  return true;
}
String groupGetName(void) {
  return group_list_name;
}
String groupGetIP(void) {
  return group_list_ip;
}
//--------------------------------------------------------------------
void groupSortBuffer(void) {
  uint32_t _timer = millis();
  uint16_t _pos = 0;

  String _data;
  _data.reserve(40);
  _data = "";

  IPAddress _prev = 0;
  IPAddress _next = 0;

  while ((millis() - _timer) < 1000) {
    int16_t _start = group_buffer.indexOf(':', _pos);
    int16_t _end = group_buffer.indexOf(',', _pos);

    if (_end < 0) return;

    _next.fromString(group_buffer.substring(_pos, _start));

    if (_prev[3] > _next[3]) {
      _data = group_buffer.substring(_pos, _end + 1);
      group_buffer.remove(_pos, (_end + 1) - _pos);
      group_buffer = _data + group_buffer;
      _pos = 0;
    }
    else _pos = _end + 1;

    _prev = _next;
  }
}
void groupAddBuffer(IPAddress ip, char* name) {
  if (group_buffer.indexOf(ip.toString()) >= 0) return;
  group_buffer += ip.toString();
  group_buffer += ':';
  group_buffer += name;
  group_buffer += ',';
}
//--------------------------------------------------------------------
uint8_t groupGetEncodeByte(uint8_t low, uint8_t high, uint8_t data) {
  return ((data + high) ^ low);
}
void groupEncodePacket(IPAddress ip, uint8_t cmd) {
  uint8_t _key_low = random(1, 256);
  uint8_t _key_high = random(32, 192);
  group.beginPacket(ip, GROUP_LOCAL_PORT);
  group.write(_key_low);
  group.write(_key_high);
  group.write(groupGetEncodeByte(_key_low, _key_high, 0xFF));
  group.write(groupGetEncodeByte(_key_low, _key_high, cmd));
  for (uint8_t i = 0; i < GROUP_DATA_SIZE; i++) group.write(groupGetEncodeByte(_key_low, _key_high, settings.name[i]));
  group.endPacket();
}
uint8_t groupGetDecodeByte(uint8_t low, uint8_t high, uint8_t data) {
  return ((data ^ low) - high);
}
uint8_t groupDecodePacket(void) {
  uint8_t _key_low = group_packet[0];
  uint8_t _key_high = group_packet[1];
  uint8_t _command = 0;

  if (_key_low && (_key_high >= 32) && (_key_high < 192)) {
    if (groupGetDecodeByte(_key_low, _key_high, group_packet[2]) == 0xFF) {
      _command = groupGetDecodeByte(_key_low, _key_high, group_packet[3]);
      for (uint8_t i = 0; i < GROUP_DATA_SIZE; i++) group_packet[i] = groupGetDecodeByte(_key_low, _key_high, group_packet[i + (GROUP_PACKET_SIZE - GROUP_DATA_SIZE)]);
      for (uint8_t i = GROUP_DATA_SIZE; i < GROUP_PACKET_SIZE; i++) group_packet[i] = 0;
    }
  }
  return _command;
}
//--------------------------------------------------------------------
void groupSearch(void) {
  if (group_status > GROUP_UPDATE_WAIT) return;
  group_status = GROUP_UPDATE_START;
  group_time = GROUP_UPDATE_TIME;
  group_timer = millis();
  group_buffer = "";
  groupEncodePacket(wifiGetBroadcastIP(), GROUP_SEARCH_CMD);
}
//--------------------------------------------------------------------
void groupReload(void) {
  groupEncodePacket(wifiGetBroadcastIP(), GROUP_UPDATE_CMD);
}
//--------------------------------------------------------------------
void groupStart(void) {
  if (group_status != GROUP_STOPPED) {
    groupSearch();
    groupReload();
  }
  else if (group.begin(GROUP_LOCAL_PORT)) {
    group_check = ui.online();
    group_update = false;
    group_time = 0;
    group_list = "";
    group_status = GROUP_READY;
  }
  else group_status = GROUP_STOPPED;
}
void groupStop(void) {
  if (group_status != GROUP_STOPPED) {
    group_status = GROUP_READY;
    group_update = false;
    group_time = 0;
    group_list = "";
  }
}
//--------------------------------------------------------------------
void groupUpdate(void) {
  if (group_status != GROUP_STOPPED) {
    if (group.parsePacket() == GROUP_PACKET_SIZE) {
      if (group.remotePort() == GROUP_LOCAL_PORT) {
        if (group.read(group_packet, GROUP_PACKET_SIZE) == GROUP_PACKET_SIZE) {
          switch (groupDecodePacket()) {
            case GROUP_SEND_CMD: groupAddBuffer(group.remoteIP(), (char*)group_packet); break;
            case GROUP_SEARCH_CMD: groupEncodePacket(group.remoteIP(), GROUP_SEND_CMD); break;
            case GROUP_UPDATE_CMD: groupSearch(); break;
          }
        }
      }
    }

    if (group_check != ui.online()) {
      group_check = ui.online();
      if (group_check) groupSearch();
    }

    if (group_time && ((millis() - group_timer) >= group_time)) {
      group_timer = millis();

      if (group_status > GROUP_UPDATE_END) {
        group_status = GROUP_UPDATE_WAIT;
        if (group_buffer[0] != '\0') {
          groupAddBuffer(WiFi.localIP(), settings.name);
          groupSortBuffer();
        }
        if (!group_list.equals(group_buffer)) {
          group_list = group_buffer;
          group_update = true;
        }
        group_buffer = "";

        if (!group_check) group_time = 0;
        else group_time = GROUP_WAIT_TIME;

        return;
      }
      else {
        group_status++;
        group_time = GROUP_UPDATE_TIME;
      }

      groupEncodePacket(wifiGetBroadcastIP(), GROUP_SEARCH_CMD);
    }
  }
}
