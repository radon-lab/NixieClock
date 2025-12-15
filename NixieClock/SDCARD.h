struct cardBufferData {
  uint8_t cardType; //тип карты памяти
  uint8_t readMode; //режим чтения
  uint8_t readState; //состояние обработчика чтения
  uint16_t readStart; //байт к прочтению сектора
  uint16_t readSize; //байт к прочтению сектора
  uint32_t readSector; //текущий сектор чтения
  uint8_t readData[DAC_BUFF_SIZE]; //буфер карты памяти
} buffer;

struct cardOutputData {
  uint8_t dacVolume; //громкость трека
  uint8_t dacSampl; //частота дискретизации
  uint8_t dacStart; //начало буфера чтения
  uint8_t dacEnd; //конец буфера чтения
} output;

enum {
  BUFFER_READY,
  BUFFER_ERROR,
  BUFFER_STOP,
  BUFFER_WAIT,
  BUFFER_INIT,
  BUFFER_READ_BLOCK,
  BUFFER_READ_FILE,
  BUFFER_READ_DATA,
  BUFFER_READ_FAT
};

//------------Команды-------------
#define CMD0   0x40 //GO_IDLE_STATE
#define CMD1   0x41 //SEND_OP_COND(MMC)
#define CMD8   0x48 //SEND_IF_COND
#define CMD16  0x50 //SET_BLOCKLEN
#define CMD17  0x51 //READ_SINGLE_BLOCK
#define ACMD41 0xE9 //SEND_OP_COND(SD)
#define CMD55  0x77 //APP_CMD
#define CMD58  0x7A //READ_OCR
#define CMD59  0x7B //CRC_ON_OFF

//--------Типы карт памяти---------
#define CT_MMC   0x01 //карта MMC
#define CT_SD1   0x02 //карта SD ver1
#define CT_SD2   0x04 //карта SD ver2
#define CT_BLOCK 0x08 //флаг блочной адресации

#define get_uint16_t(ptr) (uint16_t)(*(uint16_t*)(ptr))
#define get_uint32_t(ptr) (uint32_t)(*(uint32_t*)(ptr))

#include "SPI.h"

void cardSetReadSector(uint32_t _sector, uint8_t _type = BUFFER_READ_FILE);

//--------------------------Отправка команды----------------------------
uint8_t cardSendCommand(uint8_t _command, uint32_t _data)
{
  uint8_t answer = 0; //ответ на команду

  if (_command & 0x80) { //если расширенная команда
    _command &= 0x7F; //убираем бит расширенной команды
    answer = cardSendCommand(CMD55, 0); //отправляем команду 55
    if (answer > 1) return answer; //выходим
  }

  //выбираем карту
  SD_CS_DISABLE; //отключаем пин CS
  readSPI(); //чтение шины
  SD_CS_ENABLE; //включаем пин CS
  readSPI(); //чтение шины

  //отправляем пакет данных
  writeSPI(_command);
  writeSPI((uint8_t)(_data >> 24));
  writeSPI((uint8_t)(_data >> 16));
  writeSPI((uint8_t)(_data >> 8));
  writeSPI((uint8_t)_data);

  //отправляем контрольную сумму
  switch (_command) {
    case CMD0: writeSPI(0x95); break; //команда 0
    case CMD8: writeSPI(0x87); break; //команда 8
    default: writeSPI(0x01); break; //игнорируем контрольную сумму
  }

  //ожидаем ответа на команду
  for (uint8_t i = 10; i; i--) {
    answer = readSPI(); //чтение шины
    if (!(answer & 0x80)) break; //если поступил ответ
  }

  return answer; //возвращаем ответ
}
//-------------------------Запись в буфер ЦАП-------------------------
boolean cardOutputWriteData(void)
{
  uint8_t buff = output.dacEnd + 1;
  if (buff >= DAC_BUFF_SIZE) buff = 0;
  if (buff != output.dacStart) {
    output.dacEnd = buff;
    buffer.readData[output.dacEnd] = readSPI(); //читаем данные в буфер
    return 1;
  }
  return 0;
}
//------------------Обновление данных буфера чтения--------------------
void cardBufferUpdate(void)
{
  static uint16_t readByte;
  static uint16_t readWait;

  switch (buffer.readState) {
    case BUFFER_ERROR: buffer.readState = BUFFER_STOP; break;
    case BUFFER_STOP:
      readSPI(); //чтение шины
      readSPI(); //чтение шины
      SD_CS_DISABLE; //отключаем пин CS
      readSPI(); //чтение шины
      buffer.readState = BUFFER_READY;
      break;
    case BUFFER_WAIT:
      if (!readWait--) buffer.readState = BUFFER_ERROR;
      else if (readSPI() == 0xFE) {
        buffer.readState = buffer.readMode;
        if (buffer.readMode == BUFFER_READ_FILE) buffer.readSize = buffer.readStart = 0;
      }
      break;
    case BUFFER_INIT:
      if (!(buffer.cardType & CT_BLOCK)) buffer.readSector *= 512; //конвертируем сектор в байты
      if (!cardSendCommand(CMD17, buffer.readSector)) {
        readByte = 0;
        readWait = 40000;
        buffer.readState = BUFFER_WAIT;
      }
      else buffer.readState = BUFFER_ERROR;
      break;
    case BUFFER_READ_BLOCK:
      if (readByte < 512) {
        if (readByte < buffer.readStart) {
          readSPI(); //чтение шины
          readByte++; //пропускаем байт
        }
        else {
          buffer.readStart = 512;
          for (uint8_t i = 0; i < buffer.readSize; i++) {
            buffer.readData[i] = readSPI(); //читаем данные в буфер
            readByte++; //пропускаем байт
          }
        }
      }
      else buffer.readState = BUFFER_STOP;
      break;
    case BUFFER_READ_FILE:
      if (readByte < 512) {
        if (readByte < buffer.readStart) {
          readSPI(); //чтение шины
          readByte++; //пропускаем байт
        }
        else {
          if ((uint8_t)buffer.readSize & 0x07) buffer.readStart += 6;
          else {
            buffer.readStart += 20;
            readByte += 2;
            buffer.readData[buffer.readSize++] = readSPI(); //читаем данные в буфер
            buffer.readData[buffer.readSize++] = readSPI(); //читаем данные в буфер
          }
          readByte += 2;
          buffer.readData[buffer.readSize++] = readSPI(); //читаем данные в буфер
          buffer.readData[buffer.readSize++] = readSPI(); //читаем данные в буфер
        }
      }
      else buffer.readState = BUFFER_STOP;
      break;
    case BUFFER_READ_DATA:
      for (uint8_t i = 0; i < DAC_READ_DEPTH; i++) {
        if (readByte < 512) {
          if (readByte < buffer.readSize) {
            if (cardOutputWriteData()) readByte++; //читаем данные в буфер
            else break;
          }
          else {
            readSPI(); //чтение шины
            readByte++; //пропускаем байт
          }
        }
        else {
          buffer.readState = BUFFER_STOP;
          break;
        }
      }
      break;
    case BUFFER_READ_FAT:
      for (uint8_t i = 0; i < DAC_FAT_DEPTH; i++) {
        if (readByte < 512) {
          if (readByte < buffer.readStart) {
            readSPI(); //чтение шины
            readByte++; //пропускаем байт
          }
          else {
            reader.dataCluster = (uint16_t)readSPI() | ((uint16_t)readSPI() << 8);
            readByte += 2;
            if (fs.fatType == FS_FAT32) {
              reader.dataCluster |= ((uint32_t)readSPI() << 16) | ((uint32_t)readSPI() << 24);
              reader.dataCluster &= 0x0FFFFFFF;
              readByte += 2;
            }
            buffer.readStart = 512;
          }
        }
        else buffer.readState = BUFFER_STOP;
      }
      break;
  }
}
//-----------------------Установка чтения сектора--------------------------
void cardSetReadSector(uint32_t _sector, uint8_t _type)
{
  buffer.readSector = _sector;
  buffer.readMode = _type;
  buffer.readState = BUFFER_INIT;
}
//------------------------Прочитать блок данных----------------------------
boolean cardWaitReadBlock(uint32_t _sector, uint16_t _start, uint16_t _size)
{
  buffer.readSector = _sector;
  buffer.readSize = _size;
  buffer.readStart = _start;
  buffer.readMode = BUFFER_READ_BLOCK;
  buffer.readState = BUFFER_INIT;

  while (buffer.readState > BUFFER_ERROR) cardBufferUpdate();

  return buffer.readState;
}
//------------------------Ожидание состояния idle------------------------
boolean cardWaitIdle(uint8_t _command, uint32_t _data)
{
  for (uint16_t tmr = 10000; tmr; tmr--) {
    if (!cardSendCommand(_command, _data)) return 1;
    _delay_us(100);
  }
  return 0;
}
//-----------------------Инициализация карты памяти-----------------------
boolean cardInit(void)
{
  buffer.cardType = 0; //сбрасываем тип карты

  //отправляем 80 импульсов для перехода в SPI режими
  for (uint8_t i = 160; i; i--) {
    SD_SCK_INV; //инвертируем пин SCK
    _delay_us(5); //ждем
  }
  SD_SCK_CLEAR; //очищаем пин SCK

  if (cardSendCommand(CMD0, 0) == 1) { //если карта перешла в idle режим
    _delay_ms(150); //ждем
    cardSendCommand(CMD59, 0); //отключаем проверку CRC
    if (cardSendCommand(CMD8, 0x1AA) == 1) { //если карта версии SDv2
      for (uint8_t i = 0; i < 4; i++) buffer.readData[i] = readSPI(); //читаем ответ R7
      if (buffer.readData[2] == 0x01 && buffer.readData[3] == 0xAA) { //если карта работает в диапазоне 2.7в - 3.6в
        if (cardWaitIdle(ACMD41, 0x40000000)) { //ожидаем состояния idle
          if (!cardSendCommand(CMD58, 0)) { //проверяем бит CCS в регистре OCR
            for (uint8_t i = 0; i < 4; i++) buffer.readData[i] = readSPI(); //читаем ответ R7
            buffer.cardType = (buffer.readData[0] & 0x40) ? (CT_SD2 | CT_BLOCK) : CT_SD2;  //устанавливаем тип карты SD ver2 (с блочной адресаций или без)
          }
        }
      }
    }
    else {
      if (cardSendCommand(ACMD41, 0) > 1) buffer.cardType = CT_MMC; //если карта версии MMC
      else buffer.cardType = CT_SD1; //иначе SD ver1

      if (!cardWaitIdle((buffer.cardType == CT_SD1) ? ACMD41 : CMD1, 0) || cardSendCommand(CMD16, 512)) buffer.cardType = 0; //устанавливаем длинну блока 512 байт
    }
  }

  //завершаем работу с картой
  SD_CS_DISABLE; //отключаем пин CS
  readSPI(); //чтение шины

  return buffer.cardType; //возвращаем состояние инициализации
}
