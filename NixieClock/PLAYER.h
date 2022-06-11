#define PLAYER_START_BYTE 0x7E
#define PLAYER_END_BYTE 0xEF
#define PLAYER_VERSION 0xFF
#define PLAYER_LENGTH 0x06

#define PLAYER_RECEIVE 0x01
#define PLAYER_NO_RECEIVE 0x00

#define PLAYER_CMD_STOP 0x16
#define PLAYER_CMD_MUTE 0x1A
#define PLAYER_CMD_PAUSE 0x0E
#define PLAYER_CMD_REPLAY 0x41
#define PLAYER_CMD_PLAY_TRACK_IN_FOLDER 0x0F
#define PLAYER_CMD_SET_VOL 0x06
#define PLAYER_CMD_RESET 0x0C

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
  NORMAL_NUM, //числа из обычного ряда
  OTHER_NUM   //числа из дополнительного ряда
};

struct playerData { //буфер обмена
  boolean playbackNow; //флаг срочной отправки данных
  uint8_t playbackEnd; //последний байт буфера
  uint8_t playbackStart; //первый байт буфера
  uint8_t playbackBuff[PLAYER_MAX_BUFFER * 3]; //буфер команд
  uint8_t transferBuff[10] = {PLAYER_START_BYTE, PLAYER_VERSION, PLAYER_LENGTH, 0x00, PLAYER_NO_RECEIVE, 0x00, 0x00, 0x00, 0x00, PLAYER_END_BYTE}; //буфер отправки
} player;

const uint8_t speakTable[] PROGMEM = {2, 0, 1, 1, 1, 2, 2, 2, 2, 2}; //таблица воспроизведения окончаний фраз

volatile uint8_t uartData; //буфер UART
volatile uint8_t uartBit; //текущий бит UART
#define SOFT_UART_TIME (uint8_t)((1e9 / PLAYER_UART_SPEED) / 500) //период фрейма UART


void playerSendData(uint8_t cmd, uint8_t data_low = 0x00, uint8_t data_high = 0x00);
void playerSendDataNow(uint8_t cmd, uint8_t data_low = 0x00, uint8_t data_high = 0x00);
void playerSpeakNumber(uint16_t _num, boolean _type = NORMAL_NUM);
void playerSendCommandNow(uint8_t cmd, uint8_t data_low = 0x00, uint8_t data_high = 0x00);

//-------------------------------------Статус UART--------------------------------------
boolean uartStatus(void)
{
#if UART_MODE
  return (boolean)!(TIMSK2 & (0x01 << OCIE2B));
#else
  return (boolean)(UCSR0A & (0x01 << UDRE0));
#endif
}
//-------------------------------Отправка данных в UART---------------------------------
void uartSendData(uint8_t _byte)
{
#if UART_MODE
  uartData = _byte;
  uartBit = 0;
  OCR2B = 0; //устанавливаем COMB в начало
  TIFR2 |= (0x01 << OCF2B); //сбросили флаг прерывания
  TIMSK2 |= (0x01 << OCIE2B); //запускаем таймер
#else
  UDR0 = _byte;
#endif
}
//------------------------------Софтовая обработка UART----------------------------------
#if PLAYER_MODE == 1
#if UART_MODE
ISR(TIMER2_COMPB_vect)
{
  OCR2B += SOFT_UART_TIME;
  switch (uartBit++) {
    case 0: DF_RX_CLEAR; break;
    case 9: DF_RX_SET; break;
    case 10: TIMSK2 &= ~(0x01 << OCIE2B); break; //выключаем таймер
    default:
      if (uartData & 0x01) DF_RX_SET;
      else DF_RX_CLEAR;
      uartData >>= 1;
      break;
  }
}
#endif
#elif PLAYER_MODE == 2
//------------------------------Обновление буфера ЦАП----------------------------------
ISR(TIMER2_COMPB_vect)
{
  OCR2B += 182;
  if (buffer.dacStart != buffer.dacEnd) {
    if (++buffer.dacStart >= DAC_BUFF_SIZE) buffer.dacStart = 0;
    if (buffer.readData[buffer.dacStart] < 128) OCR1B = buffer.readData[buffer.dacStart] + ((((uint16_t)(buffer.readData[buffer.dacStart] ^ 0x7F) * buffer.dacVolume) * 26) >> 8);
    else OCR1B = buffer.readData[buffer.dacStart] - ((((uint16_t)(buffer.readData[buffer.dacStart] & 0x7F) * buffer.dacVolume) * 26) >> 8);
  }
}
#endif
//----------------------------------Статус буфера плеера--------------------------------------
inline boolean playerWriteStatus(void)
{
  return player.playbackEnd;
}
//-------------------------------Статус воспроизведения плеера--------------------------------
boolean playerPlayStatus(void)
{
  return (DF_BUSY_CHK && !_timer_ms[TMR_PLAYER]);
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
void playerSendCommandNow(uint8_t cmd, uint8_t data_low, uint8_t data_high)
{
  player.transferBuff[_COMMAND] = cmd;
  player.transferBuff[_DATA_H] = data_high;
  player.transferBuff[_DATA_L] = data_low;
  playerGenCRC(player.transferBuff);

  for (uint8_t i = 0; i < sizeof(player.transferBuff); i++) {
    while (!uartStatus());
    uartSendData(player.transferBuff[i]);
    tick_ms = 0;
  }
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
  player.playbackBuff[player.playbackStart + 1] = data_high;
  player.playbackBuff[player.playbackStart + 2] = data_low;
}
//-------------------------------------Отправить команду---------------------------------------
void playerSendData(uint8_t cmd, uint8_t data_low, uint8_t data_high)
{
  if (player.playbackEnd >= (sizeof(player.playbackBuff) - 1)) player.playbackEnd = 0;
  if (player.playbackEnd) player.playbackEnd++;
  player.playbackBuff[player.playbackEnd] = cmd;
  player.playbackBuff[++player.playbackEnd] = data_high;
  player.playbackBuff[++player.playbackEnd] = data_low;
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
#if PLAYER_MODE == 1
  static boolean busyState;
  static boolean writeState;
  static uint8_t transferByte;

  if (!_timer_ms[TMR_PLAYER]) {
    if (busyState != DF_BUSY_CHK) {
      busyState = !busyState;
      if (!busyState) _timer_ms[TMR_PLAYER] = PLAYER_COMMAND_WAIT;
      else writeState = 0;
    }
  }

  if (transferByte || player.playbackEnd) {
    if (uartStatus()) {
      if (!transferByte) {
        if ((writeState || _timer_ms[TMR_PLAYER]) && !player.playbackNow) return;
        if (player.playbackStart >= sizeof(player.playbackBuff)) player.playbackStart = 0;
        player.playbackNow = 0;
        player.transferBuff[_COMMAND] = player.playbackBuff[player.playbackStart++];
        player.transferBuff[_DATA_H] = player.playbackBuff[player.playbackStart++];
        player.transferBuff[_DATA_L] = player.playbackBuff[player.playbackStart++];
        playerGenCRC(player.transferBuff);
      }
      uartSendData(player.transferBuff[transferByte]);

      if (++transferByte >= 10) {
        transferByte = 0;
        _timer_ms[TMR_PLAYER] = PLAYER_COMMAND_WAIT;
        writeState = (player.transferBuff[_COMMAND] != PLAYER_CMD_PLAY_TRACK_IN_FOLDER ) ? 0 : 1;
        if ((player.playbackEnd + 1) == player.playbackStart) player.playbackEnd = player.playbackStart = 0;
      }
    }
  }
#elif PLAYER_MODE == 2
  readerUpdate();

  if (player.playbackEnd) {
    if (reader.playerState != READER_IDLE) {
      if (player.playbackNow) {
        player.playbackNow = 0;
        buffer.readSize = 0;
        buffer.dacStart = buffer.dacEnd = 0;
        reader.playerState = READER_SOUND_END;
      }
      return;
    }
    if (player.playbackStart >= sizeof(player.playbackBuff)) player.playbackStart = 0;

    switch (player.playbackBuff[player.playbackStart++]) {
      case PLAYER_CMD_PLAY_TRACK_IN_FOLDER:
        reader.playerState = READER_INIT;
        reader.playerFolder = player.playbackBuff[player.playbackStart++];
        reader.playerTrack = player.playbackBuff[player.playbackStart++];
        break;
      case PLAYER_CMD_STOP: player.playbackStart += 2; break;
      case PLAYER_CMD_MUTE:
        player.playbackStart++;
        if (player.playbackBuff[player.playbackStart++]) BUZZ_INP;
        else BUZZ_OUT;
        break;
    }

    if ((player.playbackEnd + 1) == player.playbackStart) player.playbackEnd = player.playbackStart = 0;
  }
#endif
}
//------------------------------------Инициализация плеера------------------------------------
void playerInint(void)
{
  DF_BUSY_INIT;
  DF_RX_INIT;

#if !UART_MODE
  UBRR0 = (F_CPU / (8UL * PLAYER_UART_SPEED)) - 1; //устанавливаем битрейт
  UCSR0A = (0x01 << U2X0); //устанавливаем удвоение скорости
  UCSR0B = (0x01 << TXEN0); //разрешаем передачу
  UCSR0C = ((0x01 << UCSZ01) | (0x01 << UCSZ00)); //устанавливаем длинну посылки 8 бит
#endif

  playerSendCommandNow(PLAYER_CMD_SET_VOL, PLAYER_VOLUME);
  _timer_ms[TMR_PLAYER] = PLAYER_START_WAIT;
}
