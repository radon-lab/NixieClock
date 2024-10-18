#define NTP_ANSWER_TIMEOUT 3000 //время ожидания ответа от сервера(2000..5000)(мс)
#define NTP_ATTEMPTS_TIMEOUT 5000 //пауза перед следующим запросом на сервер(5000..10000)(мс)
#define NTP_ATTEMPTS_ALL 5 //количесво попыток отправки запроса(1..10)

#define NTP_PACKET_SIZE 48 //длинна пакета данных(48)
#define NTP_LOCAL_PORT 1234 //локальный порт(1234)
#define NTP_SERVER_PORT 123 //порт сервера(123)

enum {
  NTP_STOPPED, //сервис не запущен
  NTP_CONNECTION, //идет подключение к серверу
  NTP_WAIT_ANSWER, //ожидание ответа от сервера
  NTP_SYNCED, //время синхронизировано
  NTP_DESYNCED, //время рассинхронизировано
  NTP_ERROR //ошибка запроса
};

uint8_t ntp_buffer[NTP_PACKET_SIZE]; //буфер обмена с ntp сервером
uint8_t ntp_status = NTP_STOPPED; //флаг состояние ntp сервера
uint8_t ntp_attempts = 0; //текущее количество попыток подключение к ntp серверу
uint32_t ntp_timer = 0; //таймер ожидания ответа от ntp сервера

uint32_t ntp_millis = 0; //количество миллисекунд с момента запроса времени
uint32_t ntp_unix = 0; //последнее запрошенное время

const uint8_t ntpSyncTime[] = {15, 30, 60, 120, 180}; //время синхронизации ntp
const char *ntpStatusList[] = {"Отсутсвует подключение к сети", "Подключение к серверу...", "Ожидание ответа...", "Синхронизировано", "Рассинхронизация", "Сервер не отвечает"};

#include <time.h>

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
boolean ntpCheckTime(uint32_t _unix, int8_t _dst) {
  if (_dst <= 0) return true;
  int32_t diff = ntp_unix - _unix;
  if (_dst == 1) diff += 3600;
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
String getNtpState(void) {
  String data = "";
  if (!ntpGetAttempts()) data += ntpStatusList[ntpGetStatus()];
  else {
    data += "Попытка подключения[";
    data += ntpGetAttempts();
    data += "]...";
  }
  return data;
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
        for (uint8_t i = 0; i < NTP_PACKET_SIZE; i++) ntp_buffer[i] = 0;

        ntp_buffer[0] = 0xE3;
        ntp_buffer[1] = 0;
        ntp_buffer[2] = 6;
        ntp_buffer[3] = 0xEC;
        ntp_buffer[12] = 49;
        ntp_buffer[13] = 0x4E;
        ntp_buffer[14] = 49;
        ntp_buffer[15] = 52;

        if (!udp.beginPacket(settings.host, NTP_SERVER_PORT) || (udp.write(ntp_buffer, NTP_PACKET_SIZE) != NTP_PACKET_SIZE) || !udp.endPacket()) ntpChangeAttempt();
        else {
          ntp_status = NTP_WAIT_ANSWER;
          ntp_timer = millis();
        }
      }
      break;
    case NTP_WAIT_ANSWER:
      if (udp.parsePacket() == NTP_PACKET_SIZE) {
        if (udp.remotePort() == NTP_SERVER_PORT) {
          if ((udp.read(ntp_buffer, NTP_PACKET_SIZE) == NTP_PACKET_SIZE) && ntp_buffer[40]) {
            ntp_timer = millis() - ((((ntp_buffer[44] << 8) | ntp_buffer[45]) * 1000UL) >> 16);
            ntp_unix = (((uint32_t)ntp_buffer[40] << 24) | ((uint32_t)ntp_buffer[41] << 16) | ((uint32_t)ntp_buffer[42] << 8) | ntp_buffer[43]) - 2208988800UL;
            ntp_status = NTP_SYNCED;
            ntp_millis = 0;
            return true;
          }
        }
        ntpChangeAttempt();
      }
      else if ((millis() - ntp_timer) >= NTP_ATTEMPTS_TIMEOUT) ntpChangeAttempt();
      break;
  }

  return false;
}
