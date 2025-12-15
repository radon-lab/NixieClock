#define PLAYER_START_BYTE 0x7E //первый байт посылки
#define PLAYER_END_BYTE 0xEF //последний байт посылки
#define PLAYER_VERSION 0xFF //версия плеера
#define PLAYER_LENGTH 0x06 //длинна команды

#define PLAYER_CMD_RESET 0x0C //команда перезагрузки

#define PLAYER_CMD_STOP 0x16 //команда стоп
#define PLAYER_CMD_MUTE 0x1A //команда приглушения звука
#define PLAYER_CMD_SET_VOL 0x06 //команда установить громкость

#define PLAYER_CMD_PLAY_TRACK_IN_FOLDER 0x0F //команда запустить трек в папке

#define PLAYER_RECEIVE 0x01 //отправлять ответ на команду
#define PLAYER_NO_RECEIVE 0x00 //не отправлять ответ на команду

#define PLAYER_MUTE_OFF 0x00 //выключить приглушение звука
#define PLAYER_MUTE_ON 0x01 //включить приглушение звука

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
  _RESET_REG, //регистр перезагрузки
  _VOL_RET_REG, //регистр возврата громкости
  _VOL_REG, //регистр громкости
  _MUTE_REG, //регистр приглушения
  _MAX_REG //всего регистров
};

enum {
  NORMAL_NUM, //числа из обычного ряда
  OTHER_NUM //числа из дополнительного ряда
};

const uint8_t speakTable[] PROGMEM = {2, 0, 1, 1, 1, 2, 2, 2, 2, 2}; //таблица воспроизведения окончаний фраз

struct playerPlaybackData {
  boolean mute; //флаг работы без звука
  uint8_t voice; //текущий голос озвучки
  uint8_t volume; //текущая громкость звука
  uint8_t trackCount; //текущее количество треков в буфере
  boolean executeNow; //флаг срочной отправки данных
  uint8_t bufferEnd; //последний байт буфера воспроизведения
  uint8_t bufferStart; //первый байт буфера воспроизведения
  uint8_t buffer[PLAYER_MAX_BUFFER * 3]; //буфер воспроизведения
} playback;

struct playerCommandData {
  uint8_t status; //статус буфера команд
  uint8_t buffer[_MAX_REG][2]; //буфер команд
  uint8_t transferBuffer[10] = {PLAYER_START_BYTE, PLAYER_VERSION, PLAYER_LENGTH, 0x00, PLAYER_NO_RECEIVE, 0x00, 0x00, 0x00, 0x00, PLAYER_END_BYTE}; //буфер отправки
} command;

void playerPlaybackEnd(void);

void playerSendData(uint8_t cmd, uint8_t data_low = 0x00, uint8_t data_high = 0x00);
void playerSendDataNow(uint8_t cmd, uint8_t data_low = 0x00, uint8_t data_high = 0x00);
void playerSendDataReg(uint8_t cmd, uint8_t data_low, uint8_t reg = 0x00);
void playerSendCommand(uint8_t cmd, uint8_t data_low = 0x00, uint8_t data_high = 0x00);

void playerSetVolNow(uint8_t _vol);
void playerSpeakNumber(uint16_t _num, boolean _type = NORMAL_NUM);

void playerGenCRC(uint8_t* arr);

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
  playerGenCRC(command.transferBuffer);
#if PLAYER_UART_MODE
  uartData = command.transferBuffer[0];
  uartByte = 0;
  uartBit = 0;
  OCR2B = 0; //устанавливаем COMB в начало
  TIFR2 |= (0x01 << OCF2B); //сбросили флаг прерывания
  TIMSK2 |= (0x01 << OCIE2B); //запускаем таймер
#else
  uartByte = 0;
  UCSR0B |= (0x01 << UDRIE0); //разрешаем прерывание по завершению передачи
#endif
  _timer_ms[TMR_PLAYER] = PLAYER_BUSY_ERROR + ((command.transferBuffer[_COMMAND] != PLAYER_CMD_RESET) ? PLAYER_COMMAND_WAIT : PLAYER_RESET_WAIT);
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
      if (++uartByte >= sizeof(command.transferBuffer)) TIMSK2 &= ~(0x01 << OCIE2B); //выключаем таймер
      else {
        uartData = command.transferBuffer[uartByte];
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
  if (uartByte >= sizeof(command.transferBuffer)) UCSR0B &= ~(0x01 << UDRIE0); //выключаем прерывания передачи
  else UDR0 = command.transferBuffer[uartByte++]; //записываем байт в буфер UART
}
#endif
#endif
//---------------------------Статус буфера воспроизведения плеера-----------------------------
inline boolean playerPlaybackStatus(void)
{
  return playback.bufferEnd;
}
//--------------------------------Статус приглушения плеера-----------------------------------
inline boolean playerMuteStatus(void)
{
  return playback.mute;
}
//-------------------------------Статус буфера команд плеера----------------------------------
inline boolean playerCommandStatus(void)
{
  return command.status;
}
//-------------------------------Статус воспроизведения плеера--------------------------------
boolean playerPlayStatus(void)
{
#if PLAYER_TYPE == 1
  return (DF_BUSY_CHK && (_timer_ms[TMR_PLAYER] <= PLAYER_BUSY_ERROR));
#elif PLAYER_TYPE == 2
  return (readerGetIdleStatus());
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
  command.transferBuffer[_COMMAND] = cmd;
  command.transferBuffer[_DATA_L] = data_low;
  command.transferBuffer[_DATA_H] = data_high;

  uartSendData();
}
//--------------------------Отправить команду вне основного буфера---------------------------
void playerSendDataReg(uint8_t cmd, uint8_t data_low, uint8_t reg)
{
  command.status |= (0x01 << reg);
  command.buffer[reg][0] = cmd;
  command.buffer[reg][1] = data_low;
}
//------------------------------Отправить команду без очереди---------------------------------
void playerSendDataNow(uint8_t cmd, uint8_t data_low, uint8_t data_high)
{
  uint8_t _buff = playback.bufferEnd + 1;
  if (_buff >= sizeof(playback.buffer)) _buff = 0;
  if (_buff != playback.bufferStart) {
    if (!playback.executeNow) {
      playback.executeNow = 1;
      if (!playback.bufferEnd) playback.bufferEnd = (sizeof(playback.buffer) - 1);
      if ((playback.bufferStart -= 3) >= sizeof(playback.buffer)) playback.bufferStart = (sizeof(playback.buffer) - 3);
    }
    if (cmd == PLAYER_CMD_PLAY_TRACK_IN_FOLDER) playback.trackCount++;
    playback.buffer[playback.bufferStart] = cmd;
    playback.buffer[playback.bufferStart + 1] = data_low;
    playback.buffer[playback.bufferStart + 2] = data_high;
  }
}
//-------------------------------------Отправить команду---------------------------------------
void playerSendData(uint8_t cmd, uint8_t data_low, uint8_t data_high)
{
  uint8_t _buff = playback.bufferEnd + 1;
  if (_buff >= sizeof(playback.buffer)) _buff = 0;
  if (_buff != playback.bufferStart) {
    if (playback.bufferEnd >= (sizeof(playback.buffer) - 1)) playback.bufferEnd = 0;
    if (playback.bufferEnd) playback.bufferEnd++;
    if (cmd == PLAYER_CMD_PLAY_TRACK_IN_FOLDER) playback.trackCount++;
    playback.buffer[playback.bufferEnd] = cmd;
    playback.buffer[++playback.bufferEnd] = data_low;
    playback.buffer[++playback.bufferEnd] = data_high;
  }
}
//-------------------------------Окончание воспроизведения трека--------------------------------
void playerPlaybackEnd(void)
{
  if (playback.trackCount) playback.trackCount--;
}
//----------------------------------Остановка очереди команд------------------------------------
void playerStop(void)
{
  playback.executeNow = 1;
  playback.trackCount = 0;
  playback.bufferEnd = playback.bufferStart = 0;
  playerSendData(PLAYER_CMD_STOP);
}
//-----------------------------------Воспроизвести трек в папке----------------------------------
void playerSetTrack(uint8_t _track, uint8_t _folder)
{
  if (_folder > 1) _folder += playback.voice;
  playerSendData(PLAYER_CMD_PLAY_TRACK_IN_FOLDER, _track, _folder);
}
//----------------------------Воспроизвести трек в папке без очереди----------------------------
void playerSetTrackNow(uint8_t _track, uint8_t _folder)
{
  if (_folder > 1) _folder += playback.voice;
  playerSendDataNow(PLAYER_CMD_PLAY_TRACK_IN_FOLDER, _track, _folder);
}
//-------------------------------------Ограничить громкость-------------------------------------
uint8_t playerConstrainVol(uint8_t _vol)
{
  if (_vol > MAIN_MAX_VOL) _vol = MAIN_MAX_VOL;
#if PLAYER_TYPE == 2
  _vol = ((uint16_t)_vol * 154) >> 8; //ограничили для SD
#else
  _vol *= 2; //ограничили для DF
#endif
  return _vol;
}
//-------------------------------------Установить громкость-------------------------------------
void playerSetVol(uint8_t _vol)
{
  playerSendDataNow(PLAYER_CMD_SET_VOL, playerConstrainVol(_vol));
}
//------------------------------Установить громкость без очереди--------------------------------
void playerSetVolNow(uint8_t _vol)
{
  _vol = playerConstrainVol(_vol);
  if (playback.volume != _vol) {
    playback.volume = _vol;
    playerSendDataReg(PLAYER_CMD_SET_VOL, _vol, _VOL_REG);
  }
}
//----------------------Установить громкость по окончанию воспроизведения------------------------
void playerRetVol(uint8_t _vol)
{
  playerSendDataReg(PLAYER_CMD_SET_VOL, playerConstrainVol(_vol), _VOL_RET_REG);
}
//------------------------------------Установить приглушение-------------------------------------
void playerSetMute(boolean _mute)
{
  playerSendData(PLAYER_CMD_MUTE, _mute);
}
//------------------------------------Установить приглушение-------------------------------------
void playerSetMuteNow(boolean _mute)
{
  if (playback.mute != _mute) {
    playback.mute = _mute;
    playerSendDataReg(PLAYER_CMD_MUTE, _mute, _MUTE_REG);
  }
}
//----------------------------------Установить голос озвучки------------------------------------
void playerSetVoice(uint8_t _voice)
{
  if (_voice < PLAYER_VOICE_MAX) playback.voice = _voice * 4;
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

  if (_timer_ms[TMR_PLAYER] <= PLAYER_BUSY_ERROR) {
    if (busyState == (boolean)DF_BUSY_CHK) {
      busyState = !busyState;
      if (busyState) _timer_ms[TMR_PLAYER] = PLAYER_BUSY_ERROR + PLAYER_BUSY_WAIT;
      else {
        playerPlaybackEnd();
        playState = 0;
      }
    }

    if (playState && !busyState && !_timer_ms[TMR_PLAYER]) {
      playState = 0;
      playback.executeNow = 0;
      playback.trackCount = 0;
      playback.bufferEnd = playback.bufferStart = 0;
      playerSendDataReg(PLAYER_CMD_RESET, 0x00, _RESET_REG);
      playerSendDataReg(PLAYER_CMD_SET_VOL, playback.volume, _VOL_REG);
    }

#if AMP_PORT_ENABLE
    if (!playback.mute) {
      if ((boolean)AMP_CHK != (boolean)playback.trackCount) {
        if (!AMP_CHK) {
          _timer_ms[TMR_PLAYER] = PLAYER_BUSY_ERROR + AMP_WAIT_TIME;
          AMP_ENABLE;
          return;
        }
        else if (!playState) AMP_DISABLE;
      }
    }
#endif

    if (uartStatus()) { //если команда не отправляется
      if (playerCommandStatus()) { //если нужно отправить команду
        uint8_t _reg = 0x01; //указатель регистра
        for (uint8_t cmd = 0; cmd < _MAX_REG; cmd++) {
          if (command.status & _reg) {
            if ((cmd != _VOL_RET_REG) || !playback.trackCount) {
              command.status &= ~_reg;
              playerSendCommand(command.buffer[cmd][0], command.buffer[cmd][1]);
              return;
            }
          }
          _reg <<= 1; //сместили указатель
        }
      }

      if (playerPlaybackStatus()) { //иначе если есть команды в буфере
        if (playState && !playback.executeNow) return;

        command.transferBuffer[_COMMAND] = playback.buffer[playback.bufferStart++];
        command.transferBuffer[_DATA_L] = playback.buffer[playback.bufferStart++];
        command.transferBuffer[_DATA_H] = playback.buffer[playback.bufferStart++];

        if ((playback.bufferEnd + 1) == playback.bufferStart) playback.bufferEnd = playback.bufferStart = 0;
        if (playback.bufferStart >= sizeof(playback.buffer)) playback.bufferStart = 0;
        playback.executeNow = 0;

        switch (command.transferBuffer[_COMMAND]) {
          case PLAYER_CMD_PLAY_TRACK_IN_FOLDER:
            if (playback.mute) return;
            playState = 1;
            busyState = 0;
            break;
          case PLAYER_CMD_MUTE: playback.mute = command.transferBuffer[_DATA_L]; break;
          case PLAYER_CMD_SET_VOL: playback.volume = command.transferBuffer[_DATA_L]; break;
          case PLAYER_CMD_STOP:
#if AMP_PORT_ENABLE
            if (!playback.mute) AMP_DISABLE;
#endif
            playState = 0;
            break;
        }
        uartSendData(); //отправляем команду в плеер
      }
    }
  }
#elif PLAYER_TYPE == 2 //SD плеер
#if AMP_PORT_ENABLE
  if (!playback.mute) {
    if ((boolean)AMP_CHK != (boolean)playback.trackCount) {
      if (!AMP_CHK) {
        _timer_ms[TMR_PLAYER] = AMP_WAIT_TIME;
        AMP_ENABLE;
        return;
      }
      else AMP_DISABLE;
    }
  }
#endif

  if (playerCommandStatus()) { //если нужно отправить команду
    uint8_t _reg = 0x01; //указатель регистра
    for (uint8_t cmd = 0; cmd < _MAX_REG; cmd++) {
      if (command.status & _reg) {
        if ((cmd != _VOL_RET_REG) || !playback.trackCount) {
          command.status &= ~_reg;

          switch (cmd) {
            case _MUTE_REG:
              readerSetMute(playback.mute);
              break;
            case _VOL_REG:
            case _VOL_RET_REG:
              readerSetVolume(playback.volume);
              break;
          }
        }
      }
      _reg <<= 1; //сместили указатель
    }
  }

  if (playerPlaybackStatus()) {
    if (!readerGetIdleStatus()) {
      if (playback.executeNow && readerGetPlayStatus()) {
        switch (playback.buffer[playback.bufferStart]) {
          case PLAYER_CMD_PLAY_TRACK_IN_FOLDER:
          case PLAYER_CMD_STOP:
            readerStop();
            return;
        }
      }
      else return;
    }

    command.transferBuffer[_COMMAND] = playback.buffer[playback.bufferStart++];
    command.transferBuffer[_DATA_L] = playback.buffer[playback.bufferStart++];
    command.transferBuffer[_DATA_H] = playback.buffer[playback.bufferStart++];

    switch (command.transferBuffer[_COMMAND]) {
      case PLAYER_CMD_PLAY_TRACK_IN_FOLDER:
        if (!playback.mute) {
          readerSetTrack(command.transferBuffer[_DATA_L]);
          readerSetFolder(command.transferBuffer[_DATA_H]);
          readerStart();
        }
        break;
      case PLAYER_CMD_MUTE:
        playback.mute = command.transferBuffer[_DATA_L];
        readerSetMute(playback.mute);
        break;
      case PLAYER_CMD_SET_VOL:
        playback.volume = command.transferBuffer[_DATA_L];
        readerSetVolume(playback.volume);
        break;
#if AMP_PORT_ENABLE
      case PLAYER_CMD_STOP:
        if (!playback.mute) AMP_DISABLE;
        break;
#endif
    }

    if ((playback.bufferEnd + 1) == playback.bufferStart) playback.bufferEnd = playback.bufferStart = 0;
    if (playback.bufferStart >= sizeof(playback.buffer)) playback.bufferStart = 0;
    playback.executeNow = 0;
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

  playerSetVolNow(PLAYER_START_VOL);
  _timer_ms[TMR_PLAYER] = PLAYER_BUSY_ERROR + PLAYER_START_WAIT;
}
//------------------------------------Инициализация SD плеера------------------------------------
void sdPlayerInit(void)
{
  for (uint8_t i = 0; i < DAC_INIT_ATTEMPTS; i++) { //инициализация карты памяти
    if (!cardMountFs()) { //если карта обнаружена
      BUZZ_INIT; //инициализация ЦАП
      break; //продолжаем
    }
    else buffer.cardType = 0; //иначе ошибка инициализации
  }
  playerSetVolNow(PLAYER_START_VOL);
}
