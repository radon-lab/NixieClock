#define PLAYER_START_BYTE 0x7E //первый байт посылки
#define PLAYER_END_BYTE 0xEF //последний байт посылки
#define PLAYER_VERSION 0xFF //версия плеера
#define PLAYER_LENGTH 0x06 //длинна команды

#define PLAYER_RECEIVE 0x01 //отправлять ответ на команду
#define PLAYER_NO_RECEIVE 0x00 //не отправлять ответ на команду

#define PLAYER_CMD_STOP 0x16 //команда стоп
#define PLAYER_CMD_MUTE 0x1A //команда приглушения звука
#define PLAYER_CMD_PLAY_TRACK_IN_FOLDER 0x0F //команда запустить трек в папке
#define PLAYER_CMD_SET_VOL 0x06 //команда установить громкость

#define PLAYER_MUTE_OFF 0x00 //выключить приглушение звука
#define PLAYER_MUTE_ON 0x01 //включить приглушение звука

#if PLAYER_TYPE == 2
#define PLAYER_MIN_VOL 0 //минимальная громкость SD плеер
#define PLAYER_MAX_VOL 9 //максимальная громкость SD плеер
#else
#define PLAYER_MIN_VOL 0 //минимальная громкость DF плеер
#define PLAYER_MAX_VOL 30 //максимальная громкость DF плеер
#endif

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
  _VOL_REG, //регистр громкости
  _MUTE_REG, //регистр приглушения
  _MAX_REG //всего регистров
};

enum {
  NORMAL_NUM, //числа из обычного ряда
  OTHER_NUM //числа из дополнительного ряда
};

struct playerData { //буфер обмена
  uint8_t playbackVol; //текущая громкость звука
  uint8_t playbackRetVol; //флаг возврата громкости звука
  uint8_t playbackVoice; //текущий голос озвучки
  boolean playbackMute; //флаг работы без звука
  boolean playbackNow; //флаг срочной отправки данных
  uint8_t playbackEnd; //последний байт буфера воспроизведения
  uint8_t playbackStart; //первый байт буфера воспроизведения
  uint8_t playbackBuff[PLAYER_MAX_BUFFER * 3]; //буфер воспроизведения
  uint8_t commandStatus; //статус буфера команд
  uint8_t commandBuff[_MAX_REG][2]; //буфер команд
  uint8_t transferBuff[10] = {PLAYER_START_BYTE, PLAYER_VERSION, PLAYER_LENGTH, 0x00, PLAYER_NO_RECEIVE, 0x00, 0x00, 0x00, 0x00, PLAYER_END_BYTE}; //буфер отправки
} player;

const uint8_t speakTable[] PROGMEM = {2, 0, 1, 1, 1, 2, 2, 2, 2, 2}; //таблица воспроизведения окончаний фраз

inline boolean playerPlaybackStatus(void);
void playerSetVolNow(uint8_t _vol);
void playerSendData(uint8_t cmd, uint8_t data_low = 0x00, uint8_t data_high = 0x00);
void playerSendDataNow(uint8_t cmd, uint8_t data_low = 0x00, uint8_t data_high = 0x00);
void playerSendDataCommand(uint8_t cmd, uint8_t data_low, uint8_t reg = 0x00);
void playerSendCommand(uint8_t cmd, uint8_t data_low = 0x00, uint8_t data_high = 0x00);
void playerSpeakNumber(uint16_t _num, boolean _type = NORMAL_NUM);

#include "READER.h"

volatile uint8_t uartData; //буфер UART
volatile uint8_t uartByte; //текущий байт UART
volatile uint8_t uartBit; //текущий бит UART
#define SOFT_UART_TIME (uint8_t)((1e9 / PLAYER_UART_SPEED) / 500) //период фрейма UART

//-----------------------------------Отключение uart------------------------------------
void uartDisable(void) //отключение uart
{
  UCSR0B = 0; //выключаем UART
}
//-------------------------------------Статус UART--------------------------------------
inline boolean uartStatus(void)
{
#if PLAYER_UART_MODE
  return (boolean)!(TIMSK2 & (0x01 << OCIE2B));
#else
  return (boolean)!(UCSR0B & (0x01 << UDRIE0));
#endif
}
//-------------------------------Отправка данных в UART---------------------------------
void uartSendData(void)
{
#if PLAYER_UART_MODE
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
  _timer_ms[TMR_PLAYER] = PLAYER_COMMAND_WAIT;
}

#if PLAYER_TYPE == 1
#if PLAYER_UART_MODE
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
//---------------------------Статус буфера воспроизведения плеера-----------------------------
inline boolean playerPlaybackStatus(void)
{
  return player.playbackEnd;
}
//-------------------------------Статус буфера команд плеера----------------------------------
inline boolean playerCommandStatus(void)
{
  return player.commandStatus;
}
//--------------------------------Статус приглушения плеера-----------------------------------
inline boolean playerMuteStatus(void)
{
  return player.playbackMute;
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
//------------------------------Отправить команду немедленно----------------------------------
void playerSendCommand(uint8_t cmd, uint8_t data_low, uint8_t data_high)
{
  player.transferBuff[_COMMAND] = cmd;
  player.transferBuff[_DATA_L] = data_low;
  player.transferBuff[_DATA_H] = data_high;
  playerGenCRC(player.transferBuff);

  uartSendData();
}
//--------------------------Отправить команду вне основного буфера---------------------------
void playerSendDataCommand(uint8_t cmd, uint8_t data_low, uint8_t reg)
{
  player.commandStatus |= (0x01 << reg);
  player.commandBuff[reg][0] = cmd;
  player.commandBuff[reg][1] = data_low;
}
//------------------------------Отправить команду без очереди---------------------------------
void playerSendDataNow(uint8_t cmd, uint8_t data_low, uint8_t data_high)
{
  uint8_t _buff = player.playbackEnd + 1;
  if (_buff >= sizeof(player.playbackBuff)) _buff = 0;
  if (_buff != player.playbackStart) {
    if (!player.playbackNow) {
      player.playbackNow = 1;
      if (!player.playbackEnd) player.playbackEnd = (sizeof(player.playbackBuff) - 1);
      if ((player.playbackStart -= 3) >= sizeof(player.playbackBuff)) player.playbackStart = (sizeof(player.playbackBuff) - 3);
    }
    player.playbackBuff[player.playbackStart] = cmd;
    player.playbackBuff[player.playbackStart + 1] = data_low;
    player.playbackBuff[player.playbackStart + 2] = data_high;
  }
}
//-------------------------------------Отправить команду---------------------------------------
void playerSendData(uint8_t cmd, uint8_t data_low, uint8_t data_high)
{
  uint8_t _buff = player.playbackEnd + 1;
  if (_buff >= sizeof(player.playbackBuff)) _buff = 0;
  if (_buff != player.playbackStart) {
    if (player.playbackEnd >= (sizeof(player.playbackBuff) - 1)) player.playbackEnd = 0;
    if (player.playbackEnd) player.playbackEnd++;
    player.playbackBuff[player.playbackEnd] = cmd;
    player.playbackBuff[++player.playbackEnd] = data_low;
    player.playbackBuff[++player.playbackEnd] = data_high;
  }
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
  playerSendDataNow(PLAYER_CMD_STOP);
}
//-----------------------------------Воспроизвести трек в папке----------------------------------
void playerSetTrack(uint8_t _track, uint8_t _folder)
{
  if (_folder > 1) _folder += player.playbackVoice;
  playerSendData(PLAYER_CMD_PLAY_TRACK_IN_FOLDER, _track, _folder);
}
//----------------------------Воспроизвести трек в папке без очереди----------------------------
void playerSetTrackNow(uint8_t _track, uint8_t _folder)
{
  if (_folder > 1) _folder += player.playbackVoice;
  playerSendDataNow(PLAYER_CMD_PLAY_TRACK_IN_FOLDER, _track, _folder);
}
//-------------------------------------Установить громкость-------------------------------------
void playerSetVol(uint8_t _vol)
{
  playerSendDataNow(PLAYER_CMD_SET_VOL, _vol);
}
//------------------------------Установить громкость без очереди--------------------------------
void playerSetVolNow(uint8_t _vol)
{
  if (player.playbackVol != _vol) {
    player.playbackVol = _vol;
#if PLAYER_TYPE == 1 //DF плеер
    playerSendDataCommand(PLAYER_CMD_SET_VOL, _vol, _VOL_REG);
#else //SD плеер
    buffer.dacVolume = 9 - _vol;
    if (buffer.dacVolume > 9) buffer.dacVolume = 0;
#endif
  }
}
//----------------------Установить громкость по окончанию воспроизведения------------------------
inline void playerRetVol(uint8_t _vol)
{
  player.playbackRetVol = _vol + 1;
}
//------------------------------------Установить приглушение-------------------------------------
void playerSetMute(boolean _mute)
{
  playerSendData(PLAYER_CMD_MUTE, _mute);
}
//------------------------------------Установить приглушение-------------------------------------
void playerSetMuteNow(boolean _mute)
{
  if (player.playbackMute != _mute) {
    player.playbackMute = _mute;
#if PLAYER_TYPE == 1 //DF плеер
    playerSendDataCommand(PLAYER_CMD_MUTE, _mute, _MUTE_REG);
#else //SD плеер
    if (_mute) BUZZ_INP;
    else BUZZ_OUT;
#endif
  }
}
//----------------------------------Установить голос озвучки------------------------------------
void playerSetVoice(uint8_t _voice)
{
  if (_voice < PLAYER_VOICE_MAX) player.playbackVoice = _voice * 4;
}
//-------------------------------------Воспроизвести число--------------------------------------
void playerSpeakNumber(uint16_t _num, boolean _type)
{
  uint8_t buff[4]; //временный буфер

  for (uint8_t _count = 0; _count < 4; _count++) { //заполняем буфер
    buff[_count] = _num % 10; //забираем младший разряд в буфер
    _num /= 10; //отнимаем младший разряд от числа
  }

  if (buff[3]) playerSetTrack(PLAYER_NUMBERS_START + 37, PLAYER_NUMBERS_FOLDER); //воспроизведение тысяч
  if (buff[2]) playerSetTrack((PLAYER_NUMBERS_START + 27) + buff[2], PLAYER_NUMBERS_FOLDER); //воспроизведение сотен
  if (buff[1] > 1) {
    playerSetTrack((PLAYER_NUMBERS_START + 18) + buff[1], PLAYER_NUMBERS_FOLDER); //воспроизведение десятков
    if (!buff[0]) return;
  }
  else if (buff[1]) buff[0] += 10;
  if (_type && (buff[0] == 1 || buff[0] == 2)) playerSetTrack((PLAYER_NUMBERS_OTHER - 1) + buff[0], PLAYER_NUMBERS_FOLDER); //воспроизведение единиц
  else playerSetTrack(PLAYER_NUMBERS_START + buff[0], PLAYER_NUMBERS_FOLDER); //воспроизведение единиц
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
#if PLAYER_TYPE == 1 //DF плеер
  static boolean busyState;
  static boolean playState;

  if (!_timer_ms[TMR_PLAYER]) {
    if (busyState == (boolean)DF_BUSY_CHK) {
      busyState = !busyState;
      if (busyState) _timer_ms[TMR_PLAYER] = PLAYER_BUSY_WAIT;
      else {
        if (!playerPlaybackStatus()) {
          if (player.playbackRetVol) {
            playerSetVolNow(player.playbackRetVol - 1);
            player.playbackRetVol = 0;
          }
#if AMP_PORT_ENABLE
          if (!player.playbackMute) AMP_DISABLE;
#endif
        }
        playState = 0;
      }
    }

    if (playerCommandStatus()) { //если нужно отправить команду
      if (uartStatus()) { //если команда не отправляется
        uint8_t _reg = 0x01; //указатель регистра
        for (uint8_t cmd = 0; cmd < _MAX_REG; cmd++) {
          if (player.commandStatus & _reg) {
            player.commandStatus &= ~_reg;

            player.transferBuff[_COMMAND] = player.commandBuff[cmd][0];
            player.transferBuff[_DATA_L] = player.commandBuff[cmd][1];
            player.transferBuff[_DATA_H] = 0;

            playerGenCRC(player.transferBuff);
            uartSendData(); //отправляем команду в плеер
            return;
          }
          _reg <<= 1; //сместили указатель
        }
      }
    }
    else if (playerPlaybackStatus()) { //иначе если есть команды в буфере
      if (uartStatus()) { //если команда не отправляется
        if (playState && !player.playbackNow) return;
#if AMP_PORT_ENABLE
        if (!AMP_CHK && !player.playbackMute && (player.playbackBuff[player.playbackStart] == PLAYER_CMD_PLAY_TRACK_IN_FOLDER)) {
          AMP_ENABLE;
          _timer_ms[TMR_PLAYER] = AMP_WAIT_TIME;
          return;
        }
#endif
        player.playbackNow = 0;

        player.transferBuff[_COMMAND] = player.playbackBuff[player.playbackStart++];
        player.transferBuff[_DATA_L] = player.playbackBuff[player.playbackStart++];
        player.transferBuff[_DATA_H] = player.playbackBuff[player.playbackStart++];

        playerGenCRC(player.transferBuff);

        if ((player.playbackEnd + 1) == player.playbackStart) player.playbackEnd = player.playbackStart = 0;
        if (player.playbackStart >= sizeof(player.playbackBuff)) player.playbackStart = 0;

        switch (player.transferBuff[_COMMAND]) {
          case PLAYER_CMD_PLAY_TRACK_IN_FOLDER:
            if (player.playbackMute) return;
            playState = 1;
            busyState = 0;
            break;
          case PLAYER_CMD_MUTE: player.playbackMute = player.transferBuff[_DATA_L]; break;
          case PLAYER_CMD_SET_VOL: player.playbackVol = player.transferBuff[_DATA_L]; break;
          case PLAYER_CMD_STOP:
#if AMP_PORT_ENABLE
            if (!player.playbackMute) AMP_DISABLE;
#endif
            playState = 0;
            break;
        }
        uartSendData(); //отправляем команду в плеер
      }
    }
  }
#elif PLAYER_TYPE == 2 //SD плеер
  if (playerPlaybackStatus()) {
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
          reader.playerTrack = player.playbackBuff[player.playbackStart++];
          reader.playerFolder = player.playbackBuff[player.playbackStart++];
#if AMP_PORT_ENABLE
          if (!AMP_CHK) {
            AMP_ENABLE;
            _timer_ms[TMR_PLAYER] = AMP_WAIT_TIME;
          }
#endif
        }
        else player.playbackStart += 2;
        break;
      case PLAYER_CMD_MUTE:
        player.playbackMute = player.playbackBuff[player.playbackStart++];
        player.playbackStart++;
        if (player.playbackMute) BUZZ_INP;
        else BUZZ_OUT;
        break;
      case PLAYER_CMD_SET_VOL:
        player.playbackVol = player.playbackBuff[player.playbackStart++];
        buffer.dacVolume = 9 - player.playbackVol;
        player.playbackStart++;
        if (buffer.dacVolume > 9) buffer.dacVolume = 0;
        break;
#if AMP_PORT_ENABLE
      case PLAYER_CMD_STOP:
        if (!player.playbackMute) AMP_DISABLE;
        player.playbackStart += 2;
        break;
#endif
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
#if !PLAYER_UART_MODE
  UBRR0 = (F_CPU / (8UL * PLAYER_UART_SPEED)) - 1; //устанавливаем битрейт
  UCSR0A = (0x01 << U2X0); //устанавливаем удвоение скорости
  UCSR0B = (0x01 << TXEN0); //разрешаем передачу
  UCSR0C = ((0x01 << UCSZ01) | (0x01 << UCSZ00)); //устанавливаем длинну посылки 8 бит
#endif

  playerSetVolNow((PLAYER_MAX_VOL * (PLAYER_START_VOL / 100.0)));
  _timer_ms[TMR_PLAYER] = PLAYER_START_WAIT;
}
//------------------------------------Инициализация SD плеера------------------------------------
void sdPlayerInit(void)
{
  for (uint8_t i = 0; i < DAC_INIT_ATTEMPTS; i++) { //инициализация карты памяти
    if (!cardMount()) { //если карта обнаружена
      BUZZ_INIT; //инициализация ЦАП
      break; //продолжаем
    }
    else buffer.cardType = 0; //иначе ошибка инициализации
  }
  playerSetVolNow((PLAYER_MAX_VOL * (PLAYER_START_VOL / 100.0)));
}
