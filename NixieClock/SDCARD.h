#define FATFS_SECT_PER_CLUST 0
#define FATFS_RSVD_SECT_CNT 1
#define FATFS_NUM_TABLES 3
#define FATFS_ROOT_ENT_CNT 4
#define FATFS_TOTAL_SECT_16 6
#define FATFS_TABLE_16 9
#define FATFS_TOTAL_SECT_32 19
#define FATFS_TABLE_32 23
#define FATFS_ROOT_CLUST 31

#define FATFS_FILE_SYS_TYPE_16 54
#define FATFS_FILE_SYS_TYPE_32 82

#define FATFS_MBR_TABLE 446
#define FATFS_BOOT_RECORD 510

struct FATFS {
  uint8_t fatType; //тип файловой системы
  uint8_t clusterSize; //количество секторов в кластере
  uint32_t rootBase; //первый сектор корневой директории
  uint32_t fatSize; //размер таблицы файловой системы
  uint32_t fatBase; //первый сектор таблицы файловой системы
  uint32_t dataBase; //первый сектор расположения данных
} fs;

struct BUFFER {
  uint8_t cardType; //тип карты памяти
  uint8_t readMode; //режим чтения
  uint8_t readState; //состояние обработчика чтения
  uint16_t readStart; //байт к прочтению сектора
  uint16_t readSize; //байт к прочтению сектора
  uint32_t readSector; //текущий сектор чтения
  uint8_t readData[DAC_BUFF_SIZE]; //буфер карты памяти
  uint8_t dacVolume = DAC_VOLUME; //громкость трека
  uint8_t dacStart; //начало буфера чтения
  uint8_t dacEnd; //конец буфера чтения
} buffer;

struct READER {
  uint8_t playerState; //состояние плеера
  uint8_t playerTrack; //номер трека
  uint8_t playerFolder; //номер папки трека
  uint8_t currentSector; //текщий сектор в кластере
  uint8_t dataByte; //начальный байт чтения трека
  uint32_t dataSize; //размер трека
  uint32_t dataSector; //сектор трека
  uint32_t dataCluster; //кластер трека
} reader;

enum {
  RES_DISK_READY,    //успешная инициализация
  RES_READ_ERROR,    //ошибка чтения
  RES_DISK_ERROR,    //ошибка файловой системы
  RES_NOT_READY,     //ошибка инициализации
  RES_NO_FILESYSTEM  //не найдена подходящая файловая система
};

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

enum {
  READER_IDLE,
  READER_INIT,
  READER_FOLDER_WAIT,
  READER_TRACK_WAIT,
  READER_DATA_WAIT,
  READER_SOUND_WAIT,
  READER_SOUND_END,
  READER_FAT_WAIT
};

enum {
  FS_NULL,   //нет файловой системы
  FS_FAT16,  //файловая система FAT16
  FS_FAT32   //файловая система FAT32
};

//---------Команды----------
#define CMD0    (0x40 | 0)  //GO_IDLE_STATE
#define CMD1    (0x40 | 1)  //SEND_OP_COND (MMC)
#define CMD8    (0x40 | 8)  //SEND_IF_COND
#define CMD16   (0x40 | 16) //SET_BLOCKLEN
#define CMD17   (0x40 | 17) //READ_SINGLE_BLOCK
#define ACMD41  (0xC0 | 41) //SEND_OP_COND (SDC)
#define CMD55   (0x40 | 55) //APP_CMD
#define CMD58   (0x40 | 58) //READ_OCR
#define CMD59   (0x40 | 59) //CRC_ON_OFF

//--------Типы карт памяти---------
#define CT_MMC        0x01  //карта MMC
#define CT_SD1        0x02  //карта SD ver1
#define CT_SD2        0x04  //карта SD ver2
#define CT_BLOCK      0x08  //флаг блочной адресации

#define get_uint16_t(ptr) (uint16_t)(*(uint16_t*)(ptr))
#define get_uint32_t(ptr) (uint32_t)(*(uint32_t*)(ptr))


void setReadSector(uint32_t _sector, uint8_t _type = BUFFER_READ_FILE);

//--------------------------Отправка команды----------------------------
uint8_t cardSendCmd(uint8_t _command, uint32_t _data)
{
  uint8_t answer = 0; //ответ на команду

  if (_command & 0x80) { //если расширенная команда
    _command &= 0x7F; //убираем бит расширенной команды
    answer = cardSendCmd(CMD55, 0); //отправляем команду 55
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
    case CMD0: writeSPI(0x95); break;     //команда 0
    case CMD8: writeSPI(0x87); break;     //команда 8
    default: writeSPI(0x01); break;       //игнорируем контрольную сумму
  }

  //ожидаем ответа на команду
  for (uint8_t i = 10; i; i--) {
    answer = readSPI(); //чтение шины
    if (!(answer & 0x80)) break; //если поступил ответ
  }

  return answer; //возвращаем ответ
}
//-------------------------Запись в буфер ЦАП-------------------------
boolean writeBufferDAC(void)
{
  uint8_t buff = buffer.dacEnd + 1;
  if (buff >= DAC_BUFF_SIZE) buff = 0;
  if (buff != buffer.dacStart) {
    buffer.dacEnd = buff;
    buffer.readData[buffer.dacEnd] = readSPI(); //читаем данные в буфер
    return 1;
  }
  return 0;
}
//------------------Обновление данных буфера чтения--------------------
void bufferUpdate(void)
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
      if (!(buffer.cardType & CT_BLOCK)) buffer.readSector *= 512;  //конвертируем сектор в байты
      if (!cardSendCmd(CMD17, buffer.readSector)) {
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
            if (writeBufferDAC()) readByte++; //читаем данные в буфер
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
void setReadSector(uint32_t _sector, uint8_t _type)
{
  buffer.readSector = _sector;
  buffer.readMode = _type;
  buffer.readState = BUFFER_INIT;
}
//------------------------Прочитать блок данных----------------------------
boolean waitReadBlock(uint32_t _sector, uint16_t _start, uint16_t _size)
{
  buffer.readSector = _sector;
  buffer.readSize = _size;
  buffer.readStart = _start;
  buffer.readMode = BUFFER_READ_BLOCK;
  buffer.readState = BUFFER_INIT;

  while (buffer.readState > BUFFER_ERROR) bufferUpdate();

  return buffer.readState;
}
//------------------------Ожидание состояния idle------------------------
boolean cardWaitIdle(uint8_t _command, uint32_t _data)
{
  for (uint16_t tmr = 10000; tmr; tmr--) {
    if (!cardSendCmd(_command, _data)) return 1;
    _delay_us(100);
  }
  return 0;
}
//-----------------------Инициализация карты памяти-----------------------
boolean cardInit(void)
{
  buffer.cardType = 0; //сбрасываем тип карты

  SD_CS_INIT; //иничиализация CS
  SD_SCK_INIT; //иничиализация SCK
  SD_MISO_INIT; //иничиализация MISO
  SD_MOSI_INIT; //иничиализация MOSI

  //отправляем 80 импульсов для перехода в SPI режими
  for (uint8_t i = 160; i; i--) {
    SD_SCK_INV; //инвертируем пин SCK
    _delay_us(5); //ждем
  }
  SD_SCK_CLEAR; //очищаем пин SCK

  if (cardSendCmd(CMD0, 0) == 1) { //если карта перешла в idle режим
    _delay_ms(150); //ждем
    cardSendCmd(CMD59, 0); //отключаем проверку CRC
    if (cardSendCmd(CMD8, 0x1AA) == 1) { //если карта версии SDv2
      for (uint8_t i = 0; i < 4; i++) buffer.readData[i] = readSPI(); //читаем ответ R7
      if (buffer.readData[2] == 0x01 && buffer.readData[3] == 0xAA) { //если карта работает в диапазоне 2.7в - 3.6в
        if (cardWaitIdle(ACMD41, 0x40000000)) { //ожидаем состояния idle
          if (!cardSendCmd(CMD58, 0)) { //проверяем бит CCS в регистре OCR
            for (uint8_t i = 0; i < 4; i++) buffer.readData[i] = readSPI(); //читаем ответ R7
            buffer.cardType = (buffer.readData[0] & 0x40) ? (CT_SD2 | CT_BLOCK) : CT_SD2;  //устанавливаем тип карты SD ver2 (с блочной адресаций или без)
          }
        }
      }
    }
    else {
      if (cardSendCmd(ACMD41, 0) > 1) buffer.cardType = CT_MMC; //если карта версии MMC
      else buffer.cardType = CT_SD1; //иначе SD ver1

      if (!cardWaitIdle((buffer.cardType == CT_SD1) ? ACMD41 : CMD1, 0) || cardSendCmd(CMD16, 512)) buffer.cardType = 0; //устанавливаем длинну блока 512 байт
    }
  }

  //завершаем работу с картой
  SD_CS_DISABLE; //отключаем пин CS
  readSPI(); //чтение шины

  return buffer.cardType; //возвращаем состояние инициализации
}
//---------------------------Получить сектор--------------------------------
uint32_t get_sector(uint32_t _cluster)
{
  if (_cluster >= fs.fatSize) return 0; //если кластер за пределами таблицы
  return (uint32_t)(_cluster - 2) * fs.clusterSize + fs.dataBase; //возвращаем сектор
}
//---------------------------Получить кластер-------------------------------
uint32_t get_cluster(uint8_t* _file)
{
  uint32_t _cluster = 0;

  if (fs.fatType == FS_FAT32) { //если файловая система FAT32
    _cluster = (uint32_t)get_uint16_t(_file + 4) << 16; //получаем старшие байты кластера
  }
  _cluster |= get_uint16_t(_file + 6); //получаем младшие байты кластера

  return _cluster; //возвращаем кластер
}
//-----------------------Получить название файла--------------------------
void getFileName(uint8_t* _buff, uint8_t _file, uint8_t _size) {
  while (_size) { //заполняем буфер
    _buff[--_size] = (_file % 10) + 48; //забираем младший разряд в буфер
    _file /= 10; //отнимаем младший разряд от числа
  }
}
//-------------------------Поиск названия файла--------------------------
uint8_t searchFileName(uint8_t* _buff, uint8_t* _data, uint8_t _size) {
  uint8_t i = 0;
  uint8_t c = 0;

  while (i < 128) {
    if (_buff[i + c] == _data[c]) c++;
    else {
      c = 0;
      i += 8;
    }
    if (c == _size) return i;
  }
  return 255;
}
//----------------------Поиск информации о файле--------------------------
uint8_t searchFileData(uint8_t* _buff) {
  uint16_t contain = 12;

  if (get_uint32_t(_buff + 8) != 0x45564157) return 255; //WAVE идентификатор

  for (uint8_t i = 0; i < 8; i++) {
    if (get_uint32_t(_buff + contain) != 0x61746164) { //data контейнер
      contain += get_uint16_t(_buff + contain + 4) + 8;
    }
    else return (contain < (DAC_BUFF_SIZE - 8)) ? (uint8_t)contain : 255;
  }
  return 255;
}
//-------------------Получить новый кластер из таблицы-----------------------
boolean get_fat(void)
{
  if (++reader.currentSector >= fs.clusterSize) {
    reader.currentSector = 0;

    switch (fs.fatType) {
      case FS_FAT16:
        buffer.readStart = ((uint16_t)reader.dataCluster % 256) * 2; //новый кластер
        setReadSector(fs.fatBase + reader.dataCluster / 256, BUFFER_READ_FAT);
        reader.playerState = READER_FAT_WAIT;
        break;
      case FS_FAT32:
        buffer.readStart = ((uint16_t)reader.dataCluster % 128) * 4; //новый кластер
        setReadSector(fs.fatBase + reader.dataCluster / 128, BUFFER_READ_FAT);
        reader.playerState = READER_FAT_WAIT;
        break;
    }
    return 0;
  }
  return 1;
}
//-----------------------Обработка звукового файла--------------------------
void readerUpdate(void)
{
  static uint8_t lableBuff[3];
  static uint8_t modeBuff;
  static uint8_t fileBuff;

  bufferUpdate();

  if (buffer.readState == BUFFER_READY || reader.playerState == READER_DATA_WAIT) {
    switch (reader.playerState) {
      case READER_INIT:
        if (buffer.cardType) {
          modeBuff = BUFFER_READ_FILE;
          reader.playerState = READER_FOLDER_WAIT;
          setReadSector(fs.rootBase);
          getFileName(lableBuff, reader.playerFolder, 2);
        }
        else reader.playerState = READER_IDLE;
        break;

      case READER_FOLDER_WAIT:
        fileBuff = searchFileName(buffer.readData, lableBuff, 2);
        if (fileBuff != 255) {
          reader.currentSector = 0;
          reader.dataCluster = get_cluster(buffer.readData + fileBuff);
          reader.dataSector = get_sector(reader.dataCluster);
          reader.playerState = READER_TRACK_WAIT;
          setReadSector(reader.dataSector);
          getFileName(lableBuff, reader.playerTrack, 3);
        }
        else reader.playerState = READER_IDLE;
        break;

      case READER_TRACK_WAIT:
        fileBuff = searchFileName(buffer.readData, lableBuff, 3);
        if (fileBuff != 255) {
          modeBuff = BUFFER_READ_DATA;
          buffer.readSize = 512;
          buffer.dacStart = buffer.dacEnd = 0;
          reader.dataCluster = get_cluster(buffer.readData + fileBuff);
          reader.dataSector = get_sector(reader.dataCluster);
          reader.playerState = READER_DATA_WAIT;
          setReadSector(reader.dataSector, BUFFER_READ_DATA);
        }
        else if (get_fat()) setReadSector(++reader.dataSector);
        break;

      case READER_DATA_WAIT:
        fileBuff = buffer.dacEnd + 1;
        if (fileBuff >= DAC_BUFF_SIZE) fileBuff = 0;
        if (fileBuff == buffer.dacStart) {
          if (!buffer.dacStart) {
            fileBuff = searchFileData(buffer.readData + 1);
            if (fileBuff != 255) {
              buffer.dacStart = fileBuff + 8;
              reader.currentSector = 0;
              reader.dataSize = get_uint32_t(buffer.readData + fileBuff + 5);
              if (reader.dataSize > 512) reader.dataSize -= 512;
              else {
                buffer.readSize = reader.dataSize;
                reader.dataSize = 0;
              }
            }
            else reader.playerState = READER_IDLE;
          }
          else {
            OCR1B = 128; //выключаем dac
            OCR2B = 255; //устанавливаем COMB в начало
            TIMSK2 |= (0x01 << OCIE2B); //запускаем таймер
#if !AMP_PORT_DISABLE
            AMP_ENABLE;
#endif
            reader.playerState = READER_SOUND_WAIT;
          }
        }
        break;

      case READER_SOUND_WAIT:
        if (reader.dataSize) {
          if (reader.dataSize > 512) reader.dataSize -= 512;
          else {
            buffer.readSize = reader.dataSize;
            reader.dataSize = 0;
          }
          if (get_fat()) setReadSector(++reader.dataSector, BUFFER_READ_DATA);
        }
        else reader.playerState = READER_SOUND_END;
        break;

      case READER_SOUND_END:
        if (buffer.dacStart == buffer.dacEnd) {
          TIMSK2 &= ~(0x01 << OCIE2B); //выключаем таймер
          OCR1B = 128; //выключаем dac
#if !AMP_PORT_DISABLE
          AMP_DISABLE;
#endif
          reader.playerState = READER_IDLE;
        }
        break;

      case READER_FAT_WAIT:
        if (reader.dataCluster < 2 || reader.dataCluster >= fs.fatSize) reader.playerState = READER_IDLE; //если кластер за пределами таблицы
        else {
          reader.dataSector = get_sector(reader.dataCluster); //новый сектор
          reader.playerState = (modeBuff == BUFFER_READ_DATA) ? READER_SOUND_WAIT : READER_TRACK_WAIT;
          setReadSector(reader.dataSector, modeBuff);
        }
        break;
    }
  }
}
//-----------------------Проверка файловой системы--------------------------
uint8_t check_fs(uint32_t _sector)
{
  if (waitReadBlock(_sector, FATFS_BOOT_RECORD, 2)) return 3; //чтение загрузочной записи
  if (get_uint16_t(buffer.readData) != 0xAA55) return 2; //чтение подписи

  if (!waitReadBlock(_sector, FATFS_FILE_SYS_TYPE_16, 2) && get_uint16_t(buffer.readData) == 0x4146) return 0; //проверка на файловую систему FAT16
  if (!waitReadBlock(_sector, FATFS_FILE_SYS_TYPE_32, 2) && get_uint16_t(buffer.readData) == 0x4146) return 0; //проверка на файловую систему FAT32

  return 1; //возвращаем ошибку
}
//---------------------Инициализация файловой системы-----------------------
uint8_t cardMount(void)
{
  uint16_t root_size = 0;
  uint32_t fat_size = 0;
  uint32_t base_sector = 0;
  uint32_t total_sectors = 0;

  if (!cardInit()) return RES_NOT_READY; //инициализация карты

  uint8_t format = check_fs(base_sector); //поиск файловой системы FAT
  if (format == 1) { //если файловая система FAT не обнаружена
    if (waitReadBlock(base_sector, FATFS_MBR_TABLE, 16)) return RES_DISK_ERROR; //ищем в первой файловой записи
    else if (buffer.readData[4]) { //если запись обнаружена
      base_sector = get_uint32_t(buffer.readData + 8); //устанавливаем новый сектор
      format = check_fs(base_sector); //поиск файловой системы FAT
    }
  }
  if (format) return RES_NO_FILESYSTEM; //если файловая система FAT не обнаружена

  if (waitReadBlock(base_sector, 13, 36)) return RES_DISK_ERROR; //инициализация файловой системы

  fat_size = get_uint16_t(buffer.readData + FATFS_TABLE_16); //находим количество секторов таблицы
  if (!fat_size) fat_size = get_uint32_t(buffer.readData + FATFS_TABLE_32);

  fat_size *= buffer.readData[FATFS_NUM_TABLES]; //находим количество секторов в таблице
  fs.fatBase = base_sector + get_uint16_t(buffer.readData + FATFS_RSVD_SECT_CNT); //устанавливаем первый сектор таблицы
  fs.clusterSize = buffer.readData[FATFS_SECT_PER_CLUST]; //устанавливаем количество секторов в кластере
  root_size = get_uint16_t(buffer.readData + FATFS_ROOT_ENT_CNT); //устанавливаем количество записей в корневом каталоге
  total_sectors = get_uint16_t(buffer.readData + FATFS_TOTAL_SECT_16); //находим общее количество секторов
  if (!total_sectors) total_sectors = get_uint32_t(buffer.readData + FATFS_TOTAL_SECT_32);
  fs.fatSize = (total_sectors - get_uint16_t(buffer.readData + FATFS_RSVD_SECT_CNT) - fat_size - root_size / 16) / fs.clusterSize + 2; //устанавливаем количество секторов в таблице

  if (fs.fatSize >= 0xFF8 && fs.fatSize < 0xFFF7) fs.fatType = FS_FAT16; //если файловая система FAT16
  else if (fs.fatSize >= 0xFFF7) fs.fatType = FS_FAT32; //если файловая система FAT32
  else return RES_NO_FILESYSTEM; //иначе не подходящая файловая система

  fs.dataBase = fs.fatBase + fat_size + root_size / 16; //устанавливаем первый сектор данных
  if (fs.fatType == FS_FAT32) fs.rootBase = get_sector(get_uint32_t(buffer.readData + FATFS_ROOT_CLUST)); //устанавливаем первый сектор корневого каталога FAT32
  else fs.rootBase = fs.fatBase + fat_size; //устанавливаем первый сектор корневого каталога

  return RES_DISK_READY; //возвращаем успешную инициализацию
}
