#define NTP_ANSWER_TIMEOUT 3000 //время ожидания ответа от сервера(2000..5000)(мс)
#define NTP_ATTEMPTS_TIMEOUT 5000 //пауза перед следующим запросом на сервер(5000..10000)(мс)
#define NTP_ATTEMPTS_ALL 5 //количесво попыток отправки запроса(1..10)

#define NTP_PACKET_SIZE 48 //длинна пакета данных(48)
#define NTP_LOCAL_PORT 1234 //локальный порт(1234)
#define NTP_SERVER_PORT 123 //порт сервера(123)

#define NTP_UNIX_DIFF 2208988800UL //смещение времени

enum {
  NTP_STOPPED, //сервис не запущен
  NTP_CONNECTION, //идет подключение к серверу
  NTP_WAIT_ANSWER, //ожидание ответа от сервера
  NTP_SYNCED, //время синхронизировано
  NTP_DESYNCED, //время рассинхронизировано
  NTP_NOT_SYNCED, //время на сервере не синхронизировано
  NTP_INVALID, //неккоректный ответ сервера
  NTP_ERROR //ошибка запроса
};

uint8_t ntp_buffer[NTP_PACKET_SIZE]; //буфер обмена с ntp сервером
uint8_t ntp_status = NTP_STOPPED; //флаг состояние ntp сервера
uint8_t ntp_attempts = 0; //текущее количество попыток подключение к ntp серверу
uint32_t ntp_timer = 0; //таймер ожидания ответа от ntp сервера

uint32_t ntp_millis = 0; //количество миллисекунд с момента запроса времени
uint32_t ntp_unix = 0; //последнее запрошенное время

const uint8_t ntpSyncTime[] = {15, 30, 60, 120, 180}; //время синхронизации ntp
const char *ntpStatusList[] = {LANG_NTP_STATUS_1, LANG_NTP_STATUS_2, LANG_NTP_STATUS_3, LANG_NTP_STATUS_4, LANG_NTP_STATUS_5, LANG_NTP_STATUS_6, LANG_NTP_STATUS_7, LANG_NTP_STATUS_8};

#include <WiFiUdp.h>
WiFiUDP udp;

//--------------------------------------------------------------------
void ntpStart(void) {
  if (udp.begin(NTP_LOCAL_PORT)) {
    ntp_status = NTP_CONNECTION;
    ntp_timer = millis();
    ntp_attempts = 0;
  }
  else ntp_status = NTP_STOPPED;
}
void ntpStop(void) {
  udp.stop();
  ntp_status = NTP_STOPPED;
}
void ntpRequest(void) {
  if (ntp_status > NTP_WAIT_ANSWER) {
    ntp_status = NTP_CONNECTION;
    ntp_timer = millis();
    ntp_attempts = 0;
  }
}
//--------------------------------------------------------------------
boolean ntpCheckTime(uint32_t unix, int8_t dst) {
  if (dst <= 0) return true;
  int32_t diff = ntp_unix - unix;
  if (dst == 1) diff += 3600;
  if (diff < 0) diff = -diff;
  if (diff < 60) return true;
  ntp_status = NTP_DESYNCED;
  return false;
}
//--------------------------------------------------------------------
uint8_t ntpGetStatus(void) {
  return ntp_status;
}
boolean ntpGetRunStatus(void) {
  return (ntp_status != NTP_STOPPED);
}
boolean ntpGetSyncStatus(void) {
  return (ntp_status == NTP_SYNCED);
}
uint8_t ntpGetAttempts(void) {
  if (ntp_status != NTP_CONNECTION) return 0;
  return ntp_attempts;
}
//--------------------------------------------------------------------
String ntpGetState(void) {
  String str = "";
  str.reserve(70);

  if (!ntpGetAttempts()) str = ntpStatusList[ntpGetStatus()];
  else {
    str = F(LANG_NTP_ATTEMPT);
    str += '[';
    str += ntpGetAttempts();
    str += F("]...");
  }

  return str;
}
//--------------------------------------------------------------------
uint32_t ntpGetMillis(void) {
  if (ntp_status != NTP_SYNCED) return 0;
  return ntp_millis % 1000;
}
uint32_t ntpGetUnix(void) {
  if (ntp_status != NTP_SYNCED) return 0;
  ntp_millis = millis() - ntp_timer;
  return ntp_unix + (ntp_millis / 1000);
}
//--------------------------------------------------------------------
uint32_t ntpParsePacketTime(uint8_t* packet) {
  return ((((uint32_t)packet[0] << 24) | ((uint32_t)packet[1] << 16) | ((uint32_t)packet[2] << 8) | packet[3]) - NTP_UNIX_DIFF);
}
//--------------------------------------------------------------------
void ntpMakePacketTime(uint8_t* packet, uint32_t unix) {
  unix += NTP_UNIX_DIFF;
  packet[0] = (unix >> 24) & 0xFF;
  packet[1] = (unix >> 16) & 0xFF;
  packet[2] = (unix >> 8) & 0xFF;
  packet[3] = unix & 0xFF;
}
//--------------------------------------------------------------------
void ntpInitPacketRequest(void) {
  for (uint8_t i = 0; i < NTP_PACKET_SIZE; i++) ntp_buffer[i] = 0;

  ntp_buffer[0] = 0x1B;
  ntp_buffer[2] = 0x06;
  ntp_buffer[3] = 0xEC;
}
//--------------------------------------------------------------------
void ntpChangeAttempt(void) {
  if (++ntp_attempts > NTP_ATTEMPTS_ALL) ntp_status = NTP_ERROR;
  else ntp_status = NTP_CONNECTION;
  ntp_timer = millis();
}
//--------------------------------------------------------------------
boolean ntpUpdate(void) {
  switch (ntp_status) {
    case NTP_CONNECTION:
      if (!ntp_attempts || ((millis() - ntp_timer) >= NTP_ATTEMPTS_TIMEOUT)) {
        ntp_unix = timeGetUnix();

        ntpInitPacketRequest();
        ntpMakePacketTime(&ntp_buffer[40], ntp_unix);

        if (!udp.beginPacket(settings.ntpHost, NTP_SERVER_PORT) || (udp.write(ntp_buffer, NTP_PACKET_SIZE) != NTP_PACKET_SIZE) || !udp.endPacket()) ntpChangeAttempt();
        else {
          ntp_status = NTP_WAIT_ANSWER;
          ntp_timer = millis();
        }
      }
      break;
    case NTP_WAIT_ANSWER:
      if (udp.parsePacket() == NTP_PACKET_SIZE) {
        if (udp.remotePort() == NTP_SERVER_PORT) {
          if (udp.read(ntp_buffer, NTP_PACKET_SIZE) == NTP_PACKET_SIZE) {
            if (((ntp_buffer[0] & 0xC0) != 0xC0) && ntp_buffer[40]) {
              ntp_timer = millis() - ((((ntp_buffer[44] << 8) | ntp_buffer[45]) * 1000UL) >> 16);
              if (ntp_unix == ntpParsePacketTime(&ntp_buffer[24])) {
                ntp_unix = ntpParsePacketTime(&ntp_buffer[40]);
                ntp_status = NTP_SYNCED;
                return true;
              }
              else {
                ntp_status = NTP_INVALID;
                return false;
              }
            }
            else {
              ntp_status = NTP_NOT_SYNCED;
              return false;
            }
          }
        }
        ntpChangeAttempt();
      }
      else if ((millis() - ntp_timer) >= NTP_ATTEMPTS_TIMEOUT) ntpChangeAttempt();
      break;
  }

  return false;
}
