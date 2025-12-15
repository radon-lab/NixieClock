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

struct fatfsData {
  uint8_t fatType; //тип файловой системы
  uint8_t clusterSize; //количество секторов в кластере
  uint8_t rootSize; //размер корневой директории
  uint32_t rootBase; //первый сектор корневой директории
  uint32_t fatSize; //размер таблицы файловой системы
  uint32_t fatBase; //первый сектор таблицы файловой системы
  uint32_t dataBase; //первый сектор расположения данных
} fs;

enum {
  RES_DISK_READY, //успешная инициализация
  RES_READ_ERROR, //ошибка чтения
  RES_DISK_ERROR, //ошибка файловой системы
  RES_NOT_READY, //ошибка инициализации
  RES_NO_FILESYSTEM //не найдена подходящая файловая система
};

enum {
  FS_NULL, //нет файловой системы
  FS_FAT16, //файловая система FAT16
  FS_FAT32 //файловая система FAT32
};

struct readerData {
  uint8_t playerState; //состояние плеера
  uint8_t playerTrack; //номер трека
  uint8_t playerFolder; //номер папки трека
  uint8_t currentSector; //текщий сектор в кластере
  uint8_t returnState; //предыдущее состояние плеера
  uint8_t dataByte; //начальный байт чтения трека
  uint32_t dataSize; //размер трека
  uint32_t dataSector; //сектор трека
  uint32_t dataCluster; //кластер трека
} reader;

enum {
  READER_IDLE,
  READER_ERROR,
  READER_FINISH,
  READER_INIT_PLAY,
  READER_FOLDER_WAIT,
  READER_TRACK_WAIT,
  READER_DATA_WAIT,
  READER_SOUND_PLAY,
  READER_SOUND_END,
  READER_FAT_WAIT
};

#include "SDCARD.h"

//------------------------------Обновление буфера ЦАП----------------------------------
#if PLAYER_TYPE == 2
ISR(TIMER2_COMPB_vect)
{
  OCR2B += output.dacSampl;
  if (output.dacStart != output.dacEnd) {
    if (++output.dacStart >= DAC_BUFF_SIZE) output.dacStart = 0;
#if BUZZ_PIN == 9
    if (buffer.readData[output.dacStart] < 128) OCR1A = buffer.readData[output.dacStart] + ((((uint16_t)(buffer.readData[output.dacStart] ^ 0x7F) * output.dacVolume) * 26) >> 8);
    else OCR1A = buffer.readData[output.dacStart] - ((((uint16_t)(buffer.readData[output.dacStart] & 0x7F) * output.dacVolume) * 26) >> 8);
#elif BUZZ_PIN == 10
    if (buffer.readData[output.dacStart] < 128) OCR1B = buffer.readData[output.dacStart] + ((((uint16_t)(buffer.readData[output.dacStart] ^ 0x7F) * output.dacVolume) * 26) >> 8);
    else OCR1B = buffer.readData[output.dacStart] - ((((uint16_t)(buffer.readData[output.dacStart] & 0x7F) * output.dacVolume) * 26) >> 8);
#endif
  }
}
#endif
//---------------------------Получить сектор--------------------------------
uint32_t cardGetSector(uint32_t _cluster)
{
  if (_cluster >= fs.fatSize) return 0; //если кластер за пределами таблицы
  return (uint32_t)(_cluster - 2) * fs.clusterSize + fs.dataBase; //возвращаем сектор
}
//---------------------------Получить кластер-------------------------------
uint32_t cardGetCluster(uint8_t* _file)
{
  uint32_t _cluster = 0;

  if (fs.fatType == FS_FAT32) { //если файловая система FAT32
    _cluster = (uint32_t)get_uint16_t(_file + 4) << 16; //получаем старшие байты кластера
  }
  _cluster |= get_uint16_t(_file + 6); //получаем младшие байты кластера

  return _cluster; //возвращаем кластер
}
//-------------------Получить новый кластер из таблицы-----------------------
boolean cardGetFat(void)
{
  if (++reader.currentSector >= fs.clusterSize) {
    reader.currentSector = 0;

    if (fs.fatType == FS_FAT32) {
      buffer.readStart = ((uint16_t)reader.dataCluster % 128) * 4; //новый кластер
      cardSetReadSector(fs.fatBase + reader.dataCluster / 128, BUFFER_READ_FAT);
    }
    else {
      buffer.readStart = ((uint16_t)reader.dataCluster % 256) * 2; //новый кластер
      cardSetReadSector(fs.fatBase + reader.dataCluster / 256, BUFFER_READ_FAT);
    }

    reader.returnState = reader.playerState;
    reader.playerState = READER_FAT_WAIT;
    return 0;
  }
  return 1;
}
//-----------------------Проверка файловой системы--------------------------
uint8_t cardCheckFs(uint32_t _sector)
{
  if (cardWaitReadBlock(_sector, FATFS_BOOT_RECORD, 2)) return 3; //чтение загрузочной записи
  if (get_uint16_t(buffer.readData) != 0xAA55) return 2; //чтение подписи

  if (!cardWaitReadBlock(_sector, FATFS_FILE_SYS_TYPE_16, 2) && get_uint16_t(buffer.readData) == 0x4146) return 0; //проверка на файловую систему FAT16
  if (!cardWaitReadBlock(_sector, FATFS_FILE_SYS_TYPE_32, 2) && get_uint16_t(buffer.readData) == 0x4146) return 0; //проверка на файловую систему FAT32

  return 1; //возвращаем ошибку
}
//---------------------Инициализация файловой системы-----------------------
uint8_t cardMountFs(void)
{
  uint32_t fat_size = 0;
  uint32_t base_sector = 0;
  uint32_t total_sectors = 0;

  if (!cardInit()) return RES_NOT_READY; //инициализация карты

  uint8_t format = cardCheckFs(base_sector); //поиск файловой системы FAT
  if (format == 1) { //если файловая система FAT не обнаружена
    if (cardWaitReadBlock(base_sector, FATFS_MBR_TABLE, 16)) return RES_DISK_ERROR; //ищем в первой файловой записи
    else if (buffer.readData[4]) { //если запись обнаружена
      base_sector = get_uint32_t(buffer.readData + 8); //устанавливаем новый сектор
      format = cardCheckFs(base_sector); //поиск файловой системы FAT
    }
  }
  if (format) return RES_NO_FILESYSTEM; //если файловая система FAT не обнаружена

  if (cardWaitReadBlock(base_sector, 13, 36)) return RES_DISK_ERROR; //инициализация файловой системы

  fat_size = get_uint16_t(buffer.readData + FATFS_TABLE_16); //находим количество секторов таблицы
  if (!fat_size) fat_size = get_uint32_t(buffer.readData + FATFS_TABLE_32);

  fat_size *= buffer.readData[FATFS_NUM_TABLES]; //находим количество секторов в таблице
  fs.fatBase = base_sector + get_uint16_t(buffer.readData + FATFS_RSVD_SECT_CNT); //устанавливаем первый сектор таблицы
  fs.clusterSize = buffer.readData[FATFS_SECT_PER_CLUST]; //устанавливаем количество секторов в кластере
  fs.rootSize = get_uint16_t(buffer.readData + FATFS_ROOT_ENT_CNT) / 16; //устанавливаем количество записей в корневом каталоге
  total_sectors = get_uint16_t(buffer.readData + FATFS_TOTAL_SECT_16); //находим общее количество секторов
  if (!total_sectors) total_sectors = get_uint32_t(buffer.readData + FATFS_TOTAL_SECT_32);
  fs.fatSize = (total_sectors - get_uint16_t(buffer.readData + FATFS_RSVD_SECT_CNT) - fat_size - fs.rootSize) / fs.clusterSize + 2; //устанавливаем количество секторов в таблице

  if (fs.fatSize >= 0xFF8 && fs.fatSize < 0xFFF7) fs.fatType = FS_FAT16; //если файловая система FAT16
  else if (fs.fatSize >= 0xFFF7) fs.fatType = FS_FAT32; //если файловая система FAT32
  else return RES_NO_FILESYSTEM; //иначе не подходящая файловая система

  fs.dataBase = fs.fatBase + fat_size + fs.rootSize; //устанавливаем первый сектор данных
  if (fs.fatType == FS_FAT32) fs.rootBase = get_uint32_t(buffer.readData + FATFS_ROOT_CLUST); //устанавливаем первый сектор корневого каталога FAT32
  else fs.rootBase = fs.fatBase + fat_size; //устанавливаем первый сектор корневого каталога

  return RES_DISK_READY; //возвращаем успешную инициализацию
}
//---------------Получить новый сектор корневой директории-------------------
boolean diskGetRoot(void)
{
  if (fs.rootSize) {
    if (reader.currentSector < fs.rootSize) {
      reader.currentSector++;
      return 1;
    }
    else reader.playerState = READER_ERROR;
  }
  else return cardGetFat();
  return 0;
}
//-----------------------Получить название файла--------------------------
void diskGetFileName(uint8_t* _buff, uint8_t _file, uint8_t _size)
{
  while (_size) { //заполняем буфер
    _buff[--_size] = (_file % 10) + 48; //забираем младший разряд в буфер
    _file /= 10; //отнимаем младший разряд от числа
  }
}
//-------------------------Поиск названия файла--------------------------
uint8_t diskSearchFileName(uint8_t* _buff, uint8_t* _data, uint8_t _size)
{
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
uint8_t diskSearchFileData(uint8_t* _buff)
{
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
//--------------------------Получить состояние------------------------------
inline uint8_t readerGetState(void)
{
  return reader.playerState;
}
//--------------------------Получить состояние------------------------------
inline boolean readerGetIdleStatus(void)
{
  return (boolean)(reader.playerState == READER_IDLE);
}
//--------------------------Получить состояние------------------------------
inline boolean readerGetPlayStatus(void)
{
  return (boolean)(reader.playerState == READER_SOUND_PLAY);
}
//-------------------------Установка приглушения----------------------------
void readerSetMute(uint8_t _mute)
{
  if (_mute) BUZZ_INP;
  else BUZZ_OUT;
}
//--------------------------Установка громкости-----------------------------
void readerSetVolume(uint8_t _volume)
{
  output.dacVolume = 9 - _volume;
  if (output.dacVolume > 9) output.dacVolume = 0;
}
//----------------------------Установка папки-------------------------------
void readerSetFolder(uint8_t _folder)
{
  reader.playerFolder = _folder;
}
//----------------------------Установка трека-------------------------------
void readerSetTrack(uint8_t _track)
{
  reader.playerTrack = _track;
}
//------------------------Запуск звукового файла----------------------------
void readerStart(void)
{
  reader.playerState = READER_INIT_PLAY;
}
//-----------------------Остановка звукового файла--------------------------
void readerStop(void)
{
  TIMSK2 &= ~(0x01 << OCIE2B); //выключаем таймер
  buffer.readSize = 0;
  output.dacStart = output.dacEnd = 0;
  reader.playerState = READER_SOUND_END;
}
//-----------------------Обработка звукового файла--------------------------
void readerUpdate(void)
{
  static uint8_t lableBuff[3];
  static uint8_t fileBuff;

  cardBufferUpdate();

  if (buffer.readState == BUFFER_READY || reader.playerState == READER_DATA_WAIT) {
    switch (reader.playerState) {
      case READER_ERROR:
      case READER_FINISH:
        playerPlaybackEnd();
        reader.playerState = READER_IDLE;
        break;
        
      case READER_INIT_PLAY:
        if (buffer.cardType) {
          reader.currentSector = 0;
          reader.playerState = READER_FOLDER_WAIT;
          reader.dataCluster = fs.rootBase;
          if (fs.fatType == FS_FAT32) reader.dataSector = cardGetSector(reader.dataCluster);
          else reader.dataSector = reader.dataCluster;
          cardSetReadSector(reader.dataSector);
          diskGetFileName(lableBuff, reader.playerFolder, 2);
        }
        else reader.playerState = READER_ERROR;
        break;

      case READER_FOLDER_WAIT:
        fileBuff = diskSearchFileName(buffer.readData, lableBuff, 2);
        if (fileBuff != 255) {
          reader.currentSector = 0;
          reader.dataCluster = cardGetCluster(buffer.readData + fileBuff);
          reader.dataSector = cardGetSector(reader.dataCluster);
          reader.playerState = READER_TRACK_WAIT;
          cardSetReadSector(reader.dataSector);
          diskGetFileName(lableBuff, reader.playerTrack, 3);
        }
        else if (diskGetRoot()) cardSetReadSector(++reader.dataSector);
        break;

      case READER_TRACK_WAIT:
        fileBuff = diskSearchFileName(buffer.readData, lableBuff, 3);
        if (fileBuff != 255) {
          buffer.readSize = 512;
          output.dacStart = output.dacEnd = 0;
          reader.dataCluster = cardGetCluster(buffer.readData + fileBuff);
          reader.dataSector = cardGetSector(reader.dataCluster);
          reader.playerState = READER_DATA_WAIT;
          cardSetReadSector(reader.dataSector, BUFFER_READ_DATA);
        }
        else if (cardGetFat()) cardSetReadSector(++reader.dataSector);
        break;

      case READER_DATA_WAIT:
        fileBuff = output.dacEnd + 1;
        if (fileBuff >= DAC_BUFF_SIZE) fileBuff = 0;
        if (fileBuff == output.dacStart) {
          if (!output.dacStart) {
            fileBuff = diskSearchFileData(buffer.readData + 1);
            if (fileBuff != 255) {
              output.dacSampl = (uint8_t)(1000000UL / get_uint32_t(buffer.readData + 25)) * 2;
              output.dacStart = fileBuff + 8;
              reader.currentSector = 0;
              reader.dataSize = get_uint32_t(buffer.readData + fileBuff + 5);
              if (reader.dataSize > 512) reader.dataSize -= 512;
              else {
                buffer.readSize = reader.dataSize;
                reader.dataSize = 0;
              }
            }
            else reader.playerState = READER_ERROR;
          }
          else {
#if BUZZ_PIN == 9
            OCR1A = 128; //выключаем dac
#elif BUZZ_PIN == 10
            OCR1B = 128; //выключаем dac
#endif
            OCR2B = 255; //устанавливаем COMB в начало
            TIFR2 |= (0x01 << OCF2B); //сбрасываем флаг прерывания
            TIMSK2 |= (0x01 << OCIE2B); //запускаем таймер
            reader.playerState = READER_SOUND_PLAY;
          }
        }
        break;

      case READER_SOUND_PLAY:
        if (reader.dataSize) {
          if (reader.dataSize > 512) reader.dataSize -= 512;
          else {
            buffer.readSize = reader.dataSize;
            reader.dataSize = 0;
          }
          if (cardGetFat()) cardSetReadSector(++reader.dataSector, BUFFER_READ_DATA);
        }
        else reader.playerState = READER_SOUND_END;
        break;

      case READER_SOUND_END:
        if (output.dacStart == output.dacEnd) {
          TIMSK2 &= ~(0x01 << OCIE2B); //выключаем таймер
#if BUZZ_PIN == 9
          OCR1A = 128; //выключаем dac
#elif BUZZ_PIN == 10
          OCR1B = 128; //выключаем dac
#endif
          reader.playerState = READER_FINISH;
        }
        break;

      case READER_FAT_WAIT:
        if (reader.dataCluster < 2 || reader.dataCluster >= fs.fatSize) reader.playerState = (reader.returnState == READER_SOUND_PLAY) ? READER_SOUND_END : READER_ERROR; //если кластер за пределами таблицы
        else {
          reader.dataSector = cardGetSector(reader.dataCluster); //новый сектор
          reader.playerState = reader.returnState;
          cardSetReadSector(reader.dataSector, (reader.returnState == READER_SOUND_PLAY) ? BUFFER_READ_DATA : BUFFER_READ_FILE);
        }
        break;
    }
  }
}
