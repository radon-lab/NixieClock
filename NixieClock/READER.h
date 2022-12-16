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
  uint8_t rootSize; //размер корневой директории
  uint32_t rootBase; //первый сектор корневой директории
  uint32_t fatSize; //размер таблицы файловой системы
  uint32_t fatBase; //первый сектор таблицы файловой системы
  uint32_t dataBase; //первый сектор расположения данных
} fs;

struct READER {
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
  RES_DISK_READY,    //успешная инициализация
  RES_READ_ERROR,    //ошибка чтения
  RES_DISK_ERROR,    //ошибка файловой системы
  RES_NOT_READY,     //ошибка инициализации
  RES_NO_FILESYSTEM  //не найдена подходящая файловая система
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

#include "SDCARD.h"

//------------------------------Обновление буфера ЦАП----------------------------------
#if PLAYER_TYPE == 2
ISR(TIMER2_COMPB_vect)
{
  OCR2B += buffer.dacSampl;
  if (buffer.dacStart != buffer.dacEnd) {
    if (++buffer.dacStart >= DAC_BUFF_SIZE) buffer.dacStart = 0;
    if (buffer.readData[buffer.dacStart] < 128) OCR1B = buffer.readData[buffer.dacStart] + ((((uint16_t)(buffer.readData[buffer.dacStart] ^ 0x7F) * buffer.dacVolume) * 26) >> 8);
    else OCR1B = buffer.readData[buffer.dacStart] - ((((uint16_t)(buffer.readData[buffer.dacStart] & 0x7F) * buffer.dacVolume) * 26) >> 8);
  }
}
#endif
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
void getFileName(uint8_t* _buff, uint8_t _file, uint8_t _size)
{
  while (_size) { //заполняем буфер
    _buff[--_size] = (_file % 10) + 48; //забираем младший разряд в буфер
    _file /= 10; //отнимаем младший разряд от числа
  }
}
//-------------------------Поиск названия файла--------------------------
uint8_t searchFileName(uint8_t* _buff, uint8_t* _data, uint8_t _size)
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
uint8_t searchFileData(uint8_t* _buff)
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
//-------------------Получить новый кластер из таблицы-----------------------
boolean get_fat(void)
{
  if (++reader.currentSector >= fs.clusterSize) {
    reader.currentSector = 0;

    if (fs.fatType == FS_FAT32) {
      buffer.readStart = ((uint16_t)reader.dataCluster % 128) * 4; //новый кластер
      setReadSector(fs.fatBase + reader.dataCluster / 128, BUFFER_READ_FAT);
    }
    else {
      buffer.readStart = ((uint16_t)reader.dataCluster % 256) * 2; //новый кластер
      setReadSector(fs.fatBase + reader.dataCluster / 256, BUFFER_READ_FAT);
    }

    reader.returnState = reader.playerState;
    reader.playerState = READER_FAT_WAIT;
    return 0;
  }
  return 1;
}
//---------------Получить новый сектор корневой директории-------------------
boolean get_root(void)
{
  if (fs.rootSize) {
    if (reader.currentSector < fs.rootSize) {
      reader.currentSector++;
      return 1;
    }
    else reader.playerState = READER_IDLE;
  }
  else return get_fat();
  return 0;
}
//-----------------------Обработка звукового файла--------------------------
void readerUpdate(void)
{
  static uint8_t lableBuff[3];
  static uint8_t fileBuff;

  bufferUpdate();

  if (buffer.readState == BUFFER_READY || reader.playerState == READER_DATA_WAIT) {
    switch (reader.playerState) {
      case READER_INIT:
        if (buffer.cardType) {
          reader.currentSector = 0;
          reader.playerState = READER_FOLDER_WAIT;
          reader.dataCluster = fs.rootBase;
          if (fs.fatType == FS_FAT32) reader.dataSector = get_sector(reader.dataCluster);
          else reader.dataSector = reader.dataCluster;
          setReadSector(reader.dataSector);
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
        else if (get_root()) setReadSector(++reader.dataSector);
        break;

      case READER_TRACK_WAIT:
        fileBuff = searchFileName(buffer.readData, lableBuff, 3);
        if (fileBuff != 255) {
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
              buffer.dacSampl = (uint8_t)(1000000UL / get_uint32_t(buffer.readData + 25)) * 2;
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
#if AMP_PORT_ENABLE
          if (!playerPlaybackStatus() && !player.playbackMute) AMP_DISABLE;
#endif
          TIMSK2 &= ~(0x01 << OCIE2B); //выключаем таймер
          OCR1B = 128; //выключаем dac
          reader.playerState = READER_IDLE;
        }
        break;

      case READER_FAT_WAIT:
        if (reader.dataCluster < 2 || reader.dataCluster >= fs.fatSize) reader.playerState = (reader.returnState == READER_SOUND_WAIT) ? READER_SOUND_END : READER_IDLE; //если кластер за пределами таблицы
        else {
          reader.dataSector = get_sector(reader.dataCluster); //новый сектор
          reader.playerState = reader.returnState;
          setReadSector(reader.dataSector, (reader.returnState == READER_SOUND_WAIT) ? BUFFER_READ_DATA : BUFFER_READ_FILE);
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
