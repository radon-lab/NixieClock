#define PLAYER_START_BYTE 0x7E
#define PLAYER_END_BYTE 0xEF
#define PLAYER_VERSION 0xFF
#define PLAYER_LENGTH 0x06

#define PLAYER_RECEIVE 0x01
#define PLAYER_NO_RECEIVE 0x00

#define PLAYER_CMD_STOP 0x16
#define PLAYER_CMD_MUTE 0x1A
#define PLAYER_CMD_PLAY_TRACK_IN_FOLDER 0x0F
#define PLAYER_CMD_SET_VOL 0x06

#define PLAYER_MUTE_OFF 0x00 //выключить приглушение звука
#define PLAYER_MUTE_ON 0x01 //включить приглушение звука

#include "READER.h"

enum {
  _START_BYTE, //стартовый байт
  _VERSION, //версия
  _LENGTH, //длинна посылки
  _COMMAND, //команда
  _RECEIVE, //ответ на запрос
  _DATA_H, //старший байт данных
  _DATA_L, //младший байт данных
  _CRC_H, //старший байт контрольной суммы
  _CRC_L, //младший байт контрольной суммы
  _END_BYTE //байт окончания посылки
};

enum {
  NORMAL_NUM, //числа из обычного ряда
  OTHER_NUM   //числа из дополнительного ряда
};

struct playerData { //буфер обмена
  boolean playbackMute; //флаг работы без звука
  boolean playbackNow; //флаг срочной отправки данных
  uint8_t playbackEnd; //последний байт буфера
  uint8_t playbackStart; //первый байт буфера
  uint8_t playbackBuff[PLAYER_MAX_BUFFER * 3]; //буфер команд
  uint8_t transferBuff[10] = {PLAYER_START_BYTE, PLAYER_VERSION, PLAYER_LENGTH, 0x00, PLAYER_NO_RECEIVE, 0x00, 0x00, 0x00, 0x00, PLAYER_END_BYTE}; //буфер отправки
} player;

const uint8_t speakTable[] PROGMEM = {2, 0, 1, 1, 1, 2, 2, 2, 2, 2}; //таблица воспроизведения окончаний фраз

volatile uint8_t uartData; //буфер UART
volatile uint8_t uartByte; //текущий байт UART
volatile uint8_t uartBit; //текущий бит UART
#define SOFT_UART_TIME (uint8_t)((1e9 / PLAYER_UART_SPEED) / 500) //период фрейма UART


void playerSendData(uint8_t cmd, uint8_t data_low = 0x00, uint8_t data_high = 0x00);
void playerSendDataNow(uint8_t cmd, uint8_t data_low = 0x00, uint8_t data_high = 0x00);
void playerSpeakNumber(uint16_t _num, boolean _type = NORMAL_NUM);
void playerSendCommand(uint8_t cmd, uint8_t data_low = 0x00, uint8_t data_high = 0x00);

//------------------------------Отключение uart---------------------------------
void uartDisable(void) //отключение uart
{
  UCSR0B = 0; //выключаем UART
  PRR |= (0x01 << PRUSART0); //выключаем питание UART
}
//-------------------------------------Статус UART--------------------------------------
inline boolean uartStatus(void)
{
#if UART_MODE
  return (boolean)!(TIMSK2 & (0x01 << OCIE2B));
#else
  return (boolean)!(UCSR0B & (0x01 << UDRIE0));
#endif
}
//-------------------------------Отправка данных в UART---------------------------------
void uartSendData(void)
{
#if UART_MODE
  uartData = player.transferBuff[0];
  uartByte = 0;
  uartBit = 0;
  OCR2B = 0; //устанавливаем COMB в начало
  TIFR2 |= (0x01 << OCF2B); //сбросили флаг прерывания
  TIMSK2 |= (0x01 << OCIE2B); //запускаем таймер
#else
  uartByte = 0;
  UCSR0B |= (0x01 << UDRIE0); //разрешаем прерывание по завершению передачи
#endif
}

#if PLAYER_TYPE == 1
#if UART_MODE
//---------------------------------Софтовая обработка UART----------------------------------
ISR(TIMER2_COMPB_vect)
{
  OCR2B += SOFT_UART_TIME;
  switch (uartBit++) {
    case 0: DF_RX_CLEAR; break;
    case 9: DF_RX_SET; break;
    case 10:
      if (++uartByte >= sizeof(player.transferBuff)) TIMSK2 &= ~(0x01 << OCIE2B); //выключаем таймер
      else {
        uartData = player.transferBuff[uartByte];
        uartBit = 0;
      }
      break;
    default:
      if (uartData & 0x01) DF_RX_SET;
      else DF_RX_CLEAR;
      uartData >>= 1;
      break;
  }
}
#else
//--------------------------------Хардверная обработка UART----------------------------------
ISR(USART_UDRE_vect)
{
  if (uartByte >= sizeof(player.transferBuff)) UCSR0B &= ~(0x01 << UDRIE0); //выключаем прерывания передачи
  else UDR0 = player.transferBuff[uartByte++]; //записываем байт в буфер UART
}
#endif
#endif
//----------------------------------Статус буфера плеера--------------------------------------
inline boolean playerWriteStatus(void)
{
  return player.playbackEnd;
}
//-------------------------------Статус воспроизведения плеера--------------------------------
boolean playerPlayStatus(void)
{
#if PLAYER_TYPE == 1
  return (DF_BUSY_CHK && !_timer_ms[TMR_PLAYER]);
#elif PLAYER_TYPE == 2
  return (reader.playerState == READER_IDLE);
#else
  return 0;
#endif
}
//-------------------------------Генерация контрольной суммы----------------------------------
void playerGenCRC(uint8_t* arr)
{
  uint16_t crc = 0;
  for (uint8_t i = 1; i < 7; i++) crc += arr[i];
  crc = -crc;
  arr[_CRC_H] = crc >> 8;
  arr[_CRC_L] = crc;
}
//----------------------------------Инициализация UART----------------------------------
void playerSendCommand(uint8_t cmd, uint8_t data_low, uint8_t data_high)
{
  player.transferBuff[_COMMAND] = cmd;
  player.transferBuff[_DATA_L] = data_low;
  player.transferBuff[_DATA_H] = data_high;
  playerGenCRC(player.transferBuff);

  uartSendData();
}
//------------------------------Отправить команду без очереди---------------------------------
void playerSendDataNow(uint8_t cmd, uint8_t data_low, uint8_t data_high)
{
  if (!player.playbackNow) {
    player.playbackNow = 1;
    if (!player.playbackEnd) player.playbackEnd = (sizeof(player.playbackBuff) - 1);
    if ((player.playbackStart -= 3) >= sizeof(player.playbackBuff)) player.playbackStart = (sizeof(player.playbackBuff) - 3);
  }
  player.playbackBuff[player.playbackStart] = cmd;
  player.playbackBuff[player.playbackStart + 1] = data_low;
  player.playbackBuff[player.playbackStart + 2] = data_high;
}
//-------------------------------------Отправить команду---------------------------------------
void playerSendData(uint8_t cmd, uint8_t data_low, uint8_t data_high)
{
  if (player.playbackEnd >= (sizeof(player.playbackBuff) - 1)) player.playbackEnd = 0;
  if (player.playbackEnd) player.playbackEnd++;
  player.playbackBuff[player.playbackEnd] = cmd;
  player.playbackBuff[++player.playbackEnd] = data_low;
  player.playbackBuff[++player.playbackEnd] = data_high;
}
//-----------------------------------Воспроизвести трек в папке----------------------------------
void playerSetTrack(uint8_t _track, uint8_t _folder)
{
  uint8_t _buff = player.playbackEnd + 1;
  if (_buff >= sizeof(player.playbackBuff)) _buff = 0;
  if (_buff != player.playbackStart) playerSendData(PLAYER_CMD_PLAY_TRACK_IN_FOLDER, _track, _folder);
}
//----------------------------Воспроизвести трек в папке без очереди----------------------------
void playerSetTrackNow(uint8_t _track, uint8_t _folder)
{
  uint8_t _buff = player.playbackEnd + 1;
  if (_buff >= sizeof(player.playbackBuff)) _buff = 0;
  if (_buff != player.playbackStart) playerSendDataNow(PLAYER_CMD_PLAY_TRACK_IN_FOLDER, _track, _folder);
}
//----------------------------------Остановка очереди команд------------------------------------
void playerStop(void)
{
  player.playbackNow = 1;
  player.playbackEnd = player.playbackStart = 0;
  playerSendData(PLAYER_CMD_STOP);
}
//----------------------------------Остановка воспроизведения-----------------------------------
void playerStopTrack(void)
{
  uint8_t _buff = player.playbackEnd + 1;
  if (_buff >= sizeof(player.playbackBuff)) _buff = 0;
  if (_buff != player.playbackStart) playerSendDataNow(PLAYER_CMD_STOP);
}
//-------------------------------------Установить громкость-------------------------------------
void playerSetVol(uint8_t _vol)
{
  uint8_t _buff = player.playbackEnd + 1;
  if (_buff >= sizeof(player.playbackBuff)) _buff = 0;
  if (_buff != player.playbackStart) playerSendDataNow(PLAYER_CMD_SET_VOL, _vol);
}
//------------------------------------Установить приглушение-------------------------------------
void playerSetMute(boolean _mute)
{
  uint8_t _buff = player.playbackEnd + 1;
  if (_buff >= sizeof(player.playbackBuff)) _buff = 0;
  if (_buff != player.playbackStart) playerSendData(PLAYER_CMD_MUTE, _mute);
}
//-------------------------------------Воспроизвести число--------------------------------------
void playerSpeakNumber(uint16_t _num, boolean _type)
{
  uint8_t buff[4]; //временный буфер

  for (uint8_t _count = 0; _count < 4; _count++) { //заполняем буфер
    buff[_count] = _num % 10; //забираем младший разряд в буфер
    _num /= 10; //отнимаем младший разряд от числа
  }

  if (buff[3]) playerSetTrack(PLAYER_NUMBERS_START + 37, PLAYER_NUMBERS_FOLDER);
  if (buff[2]) playerSetTrack((PLAYER_NUMBERS_START + 27) + buff[2], PLAYER_NUMBERS_FOLDER);
  if (buff[1] > 1) {
    playerSetTrack((PLAYER_NUMBERS_START + 18) + buff[1], PLAYER_NUMBERS_FOLDER);
    if (!buff[0]) return;
  }
  else if (buff[1]) buff[0] += 10;
  if (_type && (buff[0] == 1 || buff[0] == 2)) playerSetTrack((PLAYER_NUMBERS_OTHER - 1) + buff[0], PLAYER_NUMBERS_FOLDER);
  else playerSetTrack(PLAYER_NUMBERS_START + buff[0], PLAYER_NUMBERS_FOLDER);
}
//--------------------------------Найти окончание фразы по числу--------------------------------
uint8_t playerGetSpeak(uint16_t _num)
{
  _num %= 100; //убрали все лишнее
  if (_num > 10 && _num < 21) return 2; //если число в диапазоне то возвращаем 2
  return pgm_read_byte(&speakTable[_num % 10]); //иначе возвращаем значение из таблицы
}
//------------------------------------Обработка буфера плеера-----------------------------------
void playerUpdate(void)
{
#if PLAYER_TYPE == 1
  static boolean busyState;
  static boolean writeState;

  if (!_timer_ms[TMR_PLAYER]) {
    if (busyState != DF_BUSY_CHK) {
      busyState = !busyState;
      if (!busyState) {
#if AMP_PORT_ENABLE
        AMP_ENABLE;
#endif
        _timer_ms[TMR_PLAYER] = PLAYER_COMMAND_WAIT;
      }
      else {
#if AMP_PORT_ENABLE
        AMP_DISABLE;
#endif
        writeState = 0;
      }
    }
  }

  if (playerWriteStatus() && uartStatus()) {
    if (_timer_ms[TMR_PLAYER] || (writeState && !player.playbackNow)) return;
    if (player.playbackStart >= sizeof(player.playbackBuff)) player.playbackStart = 0;
    player.playbackNow = 0;

    player.transferBuff[_COMMAND] = player.playbackBuff[player.playbackStart++];
    player.transferBuff[_DATA_L] = player.playbackBuff[player.playbackStart++];
    player.transferBuff[_DATA_H] = player.playbackBuff[player.playbackStart++];

    playerGenCRC(player.transferBuff);

    writeState = (player.transferBuff[_COMMAND] != PLAYER_CMD_PLAY_TRACK_IN_FOLDER) ? 0 : 1;
    if (player.transferBuff[_COMMAND] == PLAYER_CMD_MUTE) player.playbackMute = player.transferBuff[_DATA_L];

    if (!player.playbackMute || !writeState) uartSendData();
    if ((player.playbackEnd + 1) == player.playbackStart) player.playbackEnd = player.playbackStart = 0;

    _timer_ms[TMR_PLAYER] = PLAYER_COMMAND_WAIT;
  }
#elif PLAYER_TYPE == 2
  if (playerWriteStatus()) {
    if (reader.playerState != READER_IDLE) {
      if (player.playbackNow && reader.playerState == READER_SOUND_WAIT) {
        switch (player.playbackBuff[player.playbackStart]) {
          case PLAYER_CMD_PLAY_TRACK_IN_FOLDER:
          case PLAYER_CMD_STOP:
            TIMSK2 &= ~(0x01 << OCIE2B); //выключаем таймер
            buffer.readSize = 0;
            buffer.dacStart = buffer.dacEnd = 0;
            reader.playerState = READER_SOUND_END;
            return;
        }
      }
      else return;
    }

    switch (player.playbackBuff[player.playbackStart++]) {
      case PLAYER_CMD_PLAY_TRACK_IN_FOLDER:
        if (!player.playbackMute) {
          reader.playerState = READER_INIT;
          reader.playerFolder = player.playbackBuff[player.playbackStart++];
          reader.playerTrack = player.playbackBuff[player.playbackStart++];
        }
        else player.playbackStart += 2;
        break;
      case PLAYER_CMD_MUTE:
        player.playbackStart++;
        player.playbackMute = player.playbackBuff[player.playbackStart++];
        if (player.playbackMute) BUZZ_INP;
        else BUZZ_OUT;
        break;
      case PLAYER_CMD_SET_VOL:
        player.playbackStart++;
        buffer.dacVolume = 9 - player.playbackBuff[player.playbackStart++];
        if (buffer.dacVolume > 9) buffer.dacVolume = 0;
        break;
      default: player.playbackStart += 2; break;
    }

    player.playbackNow = 0;
    if ((player.playbackEnd + 1) == player.playbackStart) player.playbackEnd = player.playbackStart = 0;
    if (player.playbackStart >= sizeof(player.playbackBuff)) player.playbackStart = 0;
  }
#endif
}
//------------------------------------Инициализация DF плеера------------------------------------
void dfPlayerInit(void)
{
  DF_BUSY_INIT;
  DF_RX_INIT;

#if !UART_MODE
  UBRR0 = (F_CPU / (8UL * PLAYER_UART_SPEED)) - 1; //устанавливаем битрейт
  UCSR0A = (0x01 << U2X0); //устанавливаем удвоение скорости
  UCSR0B = (0x01 << TXEN0); //разрешаем передачу
  UCSR0C = ((0x01 << UCSZ01) | (0x01 << UCSZ00)); //устанавливаем длинну посылки 8 бит
#endif

  playerSendCommand(PLAYER_CMD_SET_VOL, PLAYER_VOLUME);
  _timer_ms[TMR_PLAYER] = PLAYER_START_WAIT;
}
//------------------------------------Инициализация SD плеера------------------------------------
void sdPlayerInit(void)
{
  for (uint8_t i = 0; i < DAC_INIT_ATTEMPTS; i++) { //инициализация карты памяти
    if (!cardMount()) {
      BUZZ_INIT; //инициализация ЦАП
      break;
    }
    else buffer.cardType = 0;
  }
  buffer.dacVolume = (9 - PLAYER_VOLUME);
  if (buffer.dacVolume > 9) buffer.dacVolume = 0;
}
