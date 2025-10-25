#define BUS_WAIT_DATA 0x00

#define BUS_WRITE_TIME 0x01
#define BUS_READ_TIME 0x02

#define BUS_WRITE_FAST_SET 0x03
#define BUS_READ_FAST_SET 0x04

#define BUS_WRITE_MAIN_SET 0x05
#define BUS_READ_MAIN_SET 0x06

#define BUS_READ_ALARM_NUM 0x07
#define BUS_WRITE_SELECT_ALARM 0x08
#define BUS_READ_SELECT_ALARM 0x09
#define BUS_WRITE_ALARM_DATA 0x0A
#define BUS_READ_ALARM_DATA 0x0B
#define BUS_DEL_ALARM 0x0C
#define BUS_NEW_ALARM 0x0D

#define BUS_READ_RADIO_SET 0x0E
#define BUS_WRITE_RADIO_STA 0x0F
#define BUS_WRITE_RADIO_VOL 0x10
#define BUS_WRITE_RADIO_FREQ 0x11
#define BUS_WRITE_RADIO_MODE 0x12
#define BUS_WRITE_RADIO_POWER 0x13
#define BUS_SEEK_RADIO_UP 0x14
#define BUS_SEEK_RADIO_DOWN 0x15
#define BUS_READ_RADIO_POWER 0x16

#define BUS_CHECK_TEMP 0x17
#define BUS_READ_TEMP 0x18

#define BUS_WRITE_EXTENDED_SET 0x19
#define BUS_READ_EXTENDED_SET 0x1A

#define BUS_SET_SHOW_TIME 0x1B
#define BUS_SET_BURN_TIME 0x1C
#define BUS_SET_UPDATE 0x1E

#define BUS_WRITE_TIMER_SET 0x1F
#define BUS_READ_TIMER_SET 0x20
#define BUS_WRITE_TIMER_MODE 0x21

#define BUS_WRITE_SENS_DATA 0x22
#define BUS_WRITE_MAIN_SENS_DATA 0x23

#define BUS_READ_FAILURE 0xA0

#define BUS_ALARM_DISABLE 0xDA
#define BUS_CHANGE_BRIGHT 0xDC

#define BUS_TEST_FLIP 0xEA
#define BUS_TEST_SOUND 0xEB
#define BUS_STOP_SOUND 0xEC

#define BUS_CONTROL_DEVICE 0xFA

#define BUS_SELECT_BYTE 0xFD
#define BUS_READ_STATUS 0xFE
#define BUS_READ_DEVICE 0xFF

#define DEVICE_RESET 0xCC
#define DEVICE_UPDATE 0xDD
#define DEVICE_REBOOT 0xEE

#define BOOTLOADER_OK 0xAA
#define BOOTLOADER_START 0xBB
#define BOOTLOADER_FLASH 0xCC

enum {
  BUS_COMMAND_BIT_0,
  BUS_COMMAND_BIT_1,
  BUS_COMMAND_BIT_2,
  BUS_COMMAND_BIT_3,
  BUS_COMMAND_BIT_4,
  BUS_COMMAND_WAIT,
  BUS_COMMAND_UPDATE
};
enum {
  BUS_COMMAND_NULL,
  BUS_COMMAND_RADIO_MODE,
  BUS_COMMAND_RADIO_POWER,
  BUS_COMMAND_RADIO_SEEK_UP,
  BUS_COMMAND_RADIO_SEEK_DOWN,
  BUS_COMMAND_TIMER_MODE
};
enum {
#if DS3231_ENABLE
  BUS_EXT_COMMAND_SEND_TIME,
#endif
#if RADIO_ENABLE
  BUS_EXT_COMMAND_RADIO_VOL,
  BUS_EXT_COMMAND_RADIO_FREQ,
#endif
  BUS_EXT_MAX_DATA
};

//переменные работы с шиной
struct busData {
  uint8_t position; //текущая позиция
  uint8_t counter; //счетчик байт
  uint8_t comand; //текущая команда
  uint8_t status; //статус шины
  uint8_t statusExt; //статус шины
  uint8_t buffer[10]; //буфер шины
} bus;

//переменные работы с устройством
struct deviceData {
  uint8_t light; //яркость подсветки часов
  uint8_t status; //флаги состояния часов
  uint16_t failure; //сбои при запуске часов
} device;

const uint8_t deviceInformation[] = { //комплектация часов
  CONVERT_CHAR(FIRMWARE_VERSION[0]),
  CONVERT_CHAR(FIRMWARE_VERSION[2]),
  CONVERT_CHAR(FIRMWARE_VERSION[4]),
  HARDWARE_VERSION,
  ((DS3231_ENABLE == 2) | SENS_AHT_ENABLE | SENS_SHT_ENABLE | SENS_BME_ENABLE | SENS_PORT_ENABLE),
  BTN_EASY_MAIN_MODE,
  LAMP_NUM,
  BACKL_TYPE,
  SECS_DOT,
  DOTS_PORT_ENABLE,
  DOTS_NUM,
  DOTS_TYPE,
  LIGHT_SENS_ENABLE,
  (BTN_ADD_TYPE | IR_PORT_ENABLE),
  DS3231_ENABLE,
  TIMER_ENABLE,
  RADIO_ENABLE,
  ALARM_TYPE,
  PLAYER_TYPE,
#if PLAYER_TYPE
  PLAYER_ALARM_MAX,
#else
  SOUND_MAX(alarm_sound),
#endif
  PLAYER_VOICE_MAX,
  ALARM_AUTO_VOL_MAX
};

enum {
  STATUS_UPDATE_MAIN_SET,
  STATUS_UPDATE_FAST_SET,
  STATUS_UPDATE_RADIO_SET,
  STATUS_UPDATE_EXTENDED_SET,
  STATUS_UPDATE_ALARM_SET,
  STATUS_UPDATE_TIME_SET,
  STATUS_UPDATE_SENS_DATA,
  STATUS_UPDATE_ALARM_STATE
};

void radioSearchStation(void); //поиск радиостанции в памяти
void radioPowerSwitch(void); //переключить питание радиоприемника
void radioSeekUp(void); //автопоиск радиостанций
void radioSeekDown(void); //автопоиск радиостанций

void alarmWriteBlock(uint8_t almNum, uint8_t* data); //записать блок основных данных будильника
void alarmReadBlock(uint8_t almNum, uint8_t* data); //получить блок основных данных будильника
void alarmDisable(void); //отключение будильника
void alarmRemove(uint8_t alarm); //удалить будильник
void alarmCreate(void); //создать новый будильник
void alarmCheck(uint8_t check); //проверка будильников

uint16_t getPhaseTime(uint8_t time, int8_t phase); //получить время со сдвигом фазы

#if ESP_ENABLE
//-------------------------------------Проверка статуса шины-------------------------------------------
uint8_t busCheck(void) //проверка статуса шины
{
#if RADIO_ENABLE || DS3231_ENABLE
  if (bus.statusExt) {
    uint8_t status = bus.statusExt;
    bus.statusExt = 0; //сбросили статус
    if (status) { //если установлены флаги радио
      for (uint8_t i = 0; i < BUS_EXT_MAX_DATA; i++) { //проверяем все флаги
        if (status & 0x01) { //если флаг установлен
          switch (i) { //выбираем действие
#if DS3231_ENABLE
            case BUS_EXT_COMMAND_SEND_TIME: sendTime(); break; //отправить время в RTC
#endif
#if RADIO_ENABLE
            case BUS_EXT_COMMAND_RADIO_VOL: memoryUpdate |= (0x01 << MEM_UPDATE_RADIO_SET); setVolumeRDA(radioSettings.volume); break;
            case BUS_EXT_COMMAND_RADIO_FREQ: memoryUpdate |= (0x01 << MEM_UPDATE_RADIO_SET); setFreqRDA(radioSettings.stationsFreq); if (mainTask == RADIO_PROGRAM) radioSearchStation(); break;
#endif
          }
        }
        status >>= 1; //сместили флаги
      }
    }
  }
#endif
  return bus.status;
}
//-------------------------------------Проверка команды шины-------------------------------------------
void busCommand(void) //проверка команды шины
{
  if (bus.status & ~(0x01 << BUS_COMMAND_WAIT)) {
#if RADIO_ENABLE || (TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE))
    uint8_t status = bus.status & ~((0x01 << BUS_COMMAND_WAIT) | (0x01 << BUS_COMMAND_UPDATE));
    bus.status &= (0x01 << BUS_COMMAND_WAIT); //сбросили статус
    if (status) { //если установлены флаги
      changeAnimState = ANIM_RESET_DOT; //установили сброс анимации

#if PLAYER_TYPE
      playerStop(); //сброс воспроизведения плеера
#else
      melodyStop(); //сброс воспроизведения мелодии
#endif

      switch (status) { //выбираем действие
#if RADIO_ENABLE
        case BUS_COMMAND_RADIO_MODE: if (mainTask != RADIO_PROGRAM) mainTask = RADIO_PROGRAM; else mainTask = MAIN_PROGRAM; break;
        case BUS_COMMAND_RADIO_POWER:
          radioPowerSwitch(); //переключили питание радио
          if (radio.powerState == RDA_ON) { //если питание радио включено
            if (mainTask == MAIN_PROGRAM) mainTask = RADIO_PROGRAM;
          }
          else { //иначе питание радио выключено
            if (mainTask == RADIO_PROGRAM) mainTask = MAIN_PROGRAM;
          }
          break;
        case BUS_COMMAND_RADIO_SEEK_UP: radioSeekUp(); mainTask = RADIO_PROGRAM; break;
        case BUS_COMMAND_RADIO_SEEK_DOWN: radioSeekDown(); mainTask = RADIO_PROGRAM; break;
#endif
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
        case BUS_COMMAND_TIMER_MODE: mainTask = TIMER_PROGRAM; break;
#endif
      }
    }
#else
    bus.status &= (0x01 << BUS_COMMAND_WAIT); //сбросили статус
#endif
  }
}
#endif
//------------------------------------Обновление статуса шины------------------------------------------
uint8_t busUpdate(void) //обновление статуса шины
{
  if (TWCR & (0x01 << TWINT)) {
    switch (TWSR & 0xF8) { //прочитали статус шины
      case 0x00: //ошибка шины
      case 0x20: //передан SLA+W - принят NACK
      case 0x30: //передан байт данных - принят NACK
      case 0x48: //передан SLA+R - принят NACK
      case 0x38: //проигрыш арбитража
        wireEnd(); //остановка шины wire
        return 1; //возвращаем ошибку шины
#if ESP_ENABLE
      case 0x60: //принят SLA+W - передан ACK
        bus.position = 0;
        bus.counter = 0;
        bus.comand = BUS_WAIT_DATA;
        TWCR |= (0x01 << TWINT); //сбросили флаг прерывания
        break;
      case 0x80: //принят байт данных - передан ACK
      case 0x88: //принят байт данных - передан NACK
        switch (bus.comand) {
          case BUS_WAIT_DATA: //установка команды
            bus.comand = TWDR; //записали команду
            switch (bus.comand) {
              case BUS_WRITE_TIME: //настройки времени
              case BUS_READ_TIME: for (uint8_t i = 0; i < sizeof(RTC); i++) bus.buffer[i] = *((uint8_t*)&RTC + i); break; //копируем время
              case BUS_WRITE_FAST_SET: if (mainTask == FAST_SET_PROGRAM) bus.status |= (0x01 << BUS_COMMAND_WAIT); break; //быстрые настройки
              case BUS_WRITE_MAIN_SET: if (mainTask == MAIN_SET_PROGRAM) bus.status |= (0x01 << BUS_COMMAND_WAIT); break; //основные настройки
#if ALARM_TYPE
              case BUS_WRITE_SELECT_ALARM: //настройки будильника
              case BUS_WRITE_ALARM_DATA:
              case BUS_DEL_ALARM:
              case BUS_NEW_ALARM:
#endif
#if RADIO_ENABLE
              case BUS_WRITE_RADIO_VOL: //настройки радио
              case BUS_WRITE_RADIO_FREQ:
#endif
#if ALARM_TYPE || RADIO_ENABLE
                if (mainTask == ALARM_SET_PROGRAM) bus.status |= (0x01 << BUS_COMMAND_WAIT); //настройки будильника
                break;
#endif
            }
            break;
          case BUS_WRITE_TIME: //прием настроек времени
            if (bus.counter < sizeof(RTC)) {
              bus.buffer[bus.counter] = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_WRITE_FAST_SET: //прием быстрых настроек
            if (bus.counter < sizeof(fastSettings)) {
              *((uint8_t*)&fastSettings + bus.counter) = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_WRITE_MAIN_SET: //прием основных настроек
            if (bus.counter < sizeof(mainSettings)) {
              *((uint8_t*)&mainSettings + bus.counter) = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
#if ALARM_TYPE
          case BUS_WRITE_SELECT_ALARM:
          case BUS_READ_SELECT_ALARM:
            if (TWDR < alarms.num) bus.position = TWDR + 1; //выбрали номер будильника

            alarmReadBlock(bus.position, bus.buffer); //читаем блок данных
            if (bus.comand == BUS_WRITE_SELECT_ALARM) bus.comand = BUS_WRITE_ALARM_DATA; //перешли в режим настроек будильника
            else bus.comand = BUS_READ_ALARM_DATA; //перешли в режим настроек будильника
            break;
          case BUS_WRITE_ALARM_DATA: //прием настроек будильника
            if (bus.counter < (ALARM_MAX_ARR - 1)) {
              bus.buffer[bus.counter] = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_DEL_ALARM: //удалить будильник
            if (!bus.counter) {
              bus.position = TWDR + 1; //выбрали номер будильника
              bus.counter++; //сместили указатель
            }
            break;
#endif
#if RADIO_ENABLE
          case BUS_WRITE_RADIO_STA: //прием настроек радиостанций
            if (bus.counter < sizeof(radioSettings.stationsSave)) {
              if (bus.counter & 0x01) radioSettings.stationsSave[bus.counter >> 1] = ((uint16_t)TWDR << 8) | bus.buffer[0];
              else bus.buffer[0] = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_WRITE_RADIO_VOL: //прием громкости радио
            if (!bus.counter) {
              radioSettings.volume = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_WRITE_RADIO_FREQ: //прием частоты радио
            if (bus.counter < sizeof(radioSettings.stationsFreq)) {
              if (bus.counter & 0x01) radioSettings.stationsFreq = ((uint16_t)TWDR << 8) | bus.buffer[0];
              else bus.buffer[0] = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
#endif
          case BUS_WRITE_EXTENDED_SET: //прием расширенных настроек
            if (bus.counter < sizeof(extendedSettings)) {
              *((uint8_t*)&extendedSettings + bus.counter) = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
          case BUS_WRITE_TIMER_SET: //прием настроек таймера
            if (bus.counter < sizeof(timer)) {
              *((uint8_t*)&timer + bus.counter) = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
#endif
#if (DS3231_ENABLE != 2) && !SENS_AHT_ENABLE && !SENS_SHT_ENABLE && !SENS_BME_ENABLE && !SENS_PORT_ENABLE
          case BUS_WRITE_SENS_DATA:
            if (bus.counter < sizeof(sens)) {
              bus.buffer[bus.counter] = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
#endif
          case BUS_WRITE_MAIN_SENS_DATA:
            if (bus.counter < sizeof(extSens)) {
              bus.buffer[bus.counter] = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
#if !LIGHT_SENS_ENABLE
          case BUS_CHANGE_BRIGHT:
            if (bus.counter < 1) {
              bus.counter = 1; //сместили указатель
              device.light = TWDR;
            }
            break;
#endif
          case BUS_CONTROL_DEVICE:
            if (bus.counter < 1) {
              bus.counter = 1; //сместили указатель
              bus.buffer[0] = TWDR;
            }
            break;
          case BUS_TEST_SOUND:
            if (bus.counter < 3) {
              bus.buffer[bus.counter] = TWDR;
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_SELECT_BYTE: //выбрать произвольное место записи
            if (!bus.counter) {
              bus.counter = TWDR; //установка места записи
              bus.comand = BUS_WAIT_DATA; //установка команды
            }
            break;
        }
        TWCR |= (0x01 << TWINT); //сбросили флаг прерывания
        break;
      case 0xA8: //принят SLA+R - передан ACK
      case 0xB8: //передан байт данных - принят ACK
        switch (bus.comand) {
          case BUS_READ_TIME: //передача настроек времени
            if (bus.counter < sizeof(RTC)) {
              TWDR = bus.buffer[bus.counter];
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_READ_FAST_SET: //передача быстрых настроек
            if (bus.counter < sizeof(fastSettings)) {
              TWDR = *((uint8_t*)&fastSettings + bus.counter);
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_READ_MAIN_SET: //передача основных настроек
            if (bus.counter < sizeof(mainSettings)) {
              TWDR = *((uint8_t*)&mainSettings + bus.counter);
              bus.counter++; //сместили указатель
            }
            break;
#if ALARM_TYPE
          case BUS_READ_ALARM_DATA: //передача настроек будильника
            if (bus.counter < (ALARM_MAX_ARR - 1)) {
              TWDR = bus.buffer[bus.counter];
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_READ_ALARM_NUM: //передача информации о будильниках
            if (bus.counter < 2) {
              TWDR = alarms.num;
              bus.counter++; //сместили указатель
            }
            break;
#endif
#if RADIO_ENABLE
          case BUS_READ_RADIO_SET: //передача настроек радио
            if (bus.counter < sizeof(radioSettings)) {
              TWDR = *((uint8_t*)&radioSettings + bus.counter);
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_READ_RADIO_POWER: //передача состояния радио
            if (!bus.counter) {
              TWDR = radio.powerState;
              bus.counter++; //сместили указатель
            }
            break;
#endif
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE
          case BUS_READ_TEMP: //передача температуры
            if (bus.counter < sizeof(sens)) {
              TWDR = *((uint8_t*)&sens + bus.counter);
              bus.counter++; //сместили указатель
            }
            break;
#endif
          case BUS_READ_EXTENDED_SET: //передача расширенных настроек
            if (bus.counter < sizeof(extendedSettings)) {
              TWDR = *((uint8_t*)&extendedSettings + bus.counter);
              bus.counter++; //сместили указатель
            }
            break;
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
          case BUS_READ_TIMER_SET: //передача настроек таймера
            if (bus.counter < sizeof(timer)) {
              TWDR = *((uint8_t*)&timer + bus.counter);
              bus.counter++; //сместили указатель
            }
            break;
#endif
          case BUS_READ_FAILURE: //передача сбоев при запуске устройства
            if (bus.counter < sizeof(device.failure)) {
              TWDR = *((uint8_t*)&device.failure + bus.counter);
              bus.counter++; //сместили указатель
            }
            break;
          case BUS_READ_STATUS: //передача статуса часов
#if ALARM_TYPE
            if (alarms.now >= ALARM_WAIT) device.status |= (0x01 << STATUS_UPDATE_ALARM_STATE);
#endif
            TWDR = device.status;
            device.status = 0;
            break;
          case BUS_READ_DEVICE: //передача комплектации
            if (bus.counter < sizeof(deviceInformation)) {
              TWDR = deviceInformation[bus.counter];
              bus.counter++; //сместили указатель
            }
            break;
        }
        TWCR |= (0x01 << TWINT); //сбросили флаг прерывания
        break;
      case 0xC0: //передан байт данных - принят NACK
        TWCR |= (0x01 << TWINT); //сбросили флаг прерывания
        break;
#endif
      //case 0xA0: TWCR |= (0x01 << TWINT); break; //принят сигнал STOP
      case 0x08: //передан START
      case 0x10: //передан REPEATED START
      case 0x18: //передан SLA+W - принят ACK
      case 0x28: //передан байт данных - принят ACK
      case 0x40: //передан SLA+R - принят ACK
      case 0x50: //принят байт данных - передан ACK
      case 0x58: //принят байт данных - передан NACK
        return 2; //возвращаем статус готовности шины
      default: //неизвестная ошибка шины или сигнал STOP
#if ESP_ENABLE
        bus.status &= ~(0x01 << BUS_COMMAND_WAIT); //сбросили статус
        switch (bus.comand) {
          case BUS_WRITE_TIME: //настройки времени
#if DS3231_ENABLE
            bus.statusExt |= (0x01 << BUS_EXT_COMMAND_SEND_TIME);
#endif
            light_update = 1;
            for (uint8_t i = 0; i < sizeof(RTC); i++) *((uint8_t*)&RTC + i) = bus.buffer[i]; //устанавливаем время
            break;
          case BUS_WRITE_FAST_SET: memoryUpdate |= (0x01 << MEM_UPDATE_FAST_SET); bus.status |= (0x01 << BUS_COMMAND_UPDATE); break; //быстрые настройки
          case BUS_WRITE_MAIN_SET: memoryUpdate |= (0x01 << MEM_UPDATE_MAIN_SET); bus.status |= (0x01 << BUS_COMMAND_UPDATE); break; //основные настройки
#if ALARM_TYPE
          case BUS_WRITE_ALARM_DATA:
          case BUS_DEL_ALARM:
          case BUS_NEW_ALARM:
            switch (bus.comand) {
              case BUS_WRITE_ALARM_DATA: bus.buffer[ALARM_STATUS] = 255; alarmWriteBlock(bus.position, bus.buffer); break; //записываем настройки будильника
              case BUS_DEL_ALARM: alarmRemove(bus.position); break; //удаляем выбранный будильник
              case BUS_NEW_ALARM: alarmCreate(); break; //добавляем новый будильник
            }
            if (alarms.now < ALARM_WAIT) { //если не работает тревога
              alarmCheck(ALARM_CHECK_SET); //проверяем будильники на совпадение
              bus.status |= (0x01 << BUS_COMMAND_UPDATE);
            }
            break;
#endif
#if RADIO_ENABLE
          case BUS_WRITE_RADIO_STA: memoryUpdate |= (0x01 << MEM_UPDATE_RADIO_SET); bus.status |= (0x01 << BUS_COMMAND_UPDATE); break; //настройки радио
          case BUS_WRITE_RADIO_VOL: bus.statusExt |= (0x01 << BUS_EXT_COMMAND_RADIO_VOL); break; //настройка громкости радио
          case BUS_WRITE_RADIO_FREQ: bus.statusExt |= (0x01 << BUS_EXT_COMMAND_RADIO_FREQ); break; //настройка частоты радио
          case BUS_WRITE_RADIO_MODE: bus.status |= BUS_COMMAND_RADIO_MODE; break; //переключение режима радио
          case BUS_WRITE_RADIO_POWER: bus.status |= BUS_COMMAND_RADIO_POWER; break; //переключение питания радио
          case BUS_SEEK_RADIO_UP: bus.status |= BUS_COMMAND_RADIO_SEEK_UP; break; //запуск автопоиска радио
          case BUS_SEEK_RADIO_DOWN: bus.status |= BUS_COMMAND_RADIO_SEEK_DOWN; break; //запуск автопоиска радио
#endif
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE
          case BUS_CHECK_TEMP: _timer_ms[TMR_SENS] = 0; device.status &= ~(0x01 << STATUS_UPDATE_SENS_DATA); break; //запрос температуры
#endif
          case BUS_WRITE_EXTENDED_SET: memoryUpdate |= (0x01 << MEM_UPDATE_EXTENDED_SET); break; //расширенные настройки
          case BUS_SET_SHOW_TIME: _timer_sec[TMR_SHOW] = getPhaseTime(mainSettings.autoShowTime, AUTO_SHOW_PHASE); break; //установка таймера показа температуры
          case BUS_SET_BURN_TIME: _timer_sec[TMR_BURN] = getPhaseTime(mainSettings.burnTime, BURN_PHASE); break; //установка таймера антиотравления
          case BUS_SET_UPDATE: bus.status |= (0x01 << BUS_COMMAND_UPDATE); break; //установка флага обновления
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
          case BUS_WRITE_TIMER_MODE: bus.status |= BUS_COMMAND_TIMER_MODE; break; //переключение в режим таймера
#endif
#if (DS3231_ENABLE != 2) && !SENS_AHT_ENABLE && !SENS_SHT_ENABLE && !SENS_BME_ENABLE && !SENS_PORT_ENABLE
          case BUS_WRITE_SENS_DATA: for (uint8_t i = 0; i < sizeof(sens); i++) *((uint8_t*)&sens + i) = bus.buffer[i]; break; //копирование температуры
#endif
          case BUS_WRITE_MAIN_SENS_DATA: for (uint8_t i = 0; i < sizeof(extSens); i++) *((uint8_t*)&extSens + i) = bus.buffer[i]; break; //копирование температуры
#if ALARM_TYPE
          case BUS_ALARM_DISABLE: //отключение будильника
            if (alarms.now >= ALARM_WAIT) { //если будильник активен
#if PLAYER_TYPE
              playerStop(); //сброс воспроизведения плеера
#else
              melodyStop(); //сброс воспроизведения мелодии
#endif
              alarmDisable(); //отключить будильник
            }
            break;
#endif
#if !LIGHT_SENS_ENABLE
          case BUS_CHANGE_BRIGHT: light_update = 1; break; //смена яркости
#endif
          case BUS_TEST_FLIP: animShow = ANIM_DEMO; bus.status |= (0x01 << BUS_COMMAND_UPDATE); break; //тест анимации минут
          case BUS_TEST_SOUND: //тест звука
            if ((mainTask == MAIN_PROGRAM) || (mainTask == SLEEP_PROGRAM)) { //если в режиме часов или спим
#if PLAYER_TYPE
              playerSetVoice(mainSettings.voiceSound);
              if (!player.playbackMute) {
                bus.status |= (0x01 << BUS_COMMAND_UPDATE);
                playerStop(); //сброс воспроизведения плеера
                playerSetVolNow(bus.buffer[0]);
                playerSetTrackNow(bus.buffer[1], bus.buffer[2]);
                playerRetVol(mainSettings.volumeSound);
              }
              else playerSetVolNow(mainSettings.volumeSound);
#else
#if RADIO_ENABLE
              if (radio.powerState == RDA_OFF) { //если радио выключено
#endif
                bus.status |= (0x01 << BUS_COMMAND_UPDATE);
                melodyPlay(bus.buffer[1], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#if RADIO_ENABLE
              }
#endif
#endif
            }
            break;
          case BUS_STOP_SOUND: //остановка звука
            if ((mainTask == MAIN_PROGRAM) || (mainTask == SLEEP_PROGRAM)) { //если в режиме часов или спим
#if PLAYER_TYPE
              playerStop(); //сброс воспроизведения плеера
#else
              melodyStop(); //сброс воспроизведения мелодии
#endif
            }
            break;
          case BUS_CONTROL_DEVICE:
            if (bus.counter == 1) {
              switch (bus.buffer[0]) {
                case DEVICE_RESET:
                  EEPROM_UpdateByte(EEPROM_BLOCK_CRC_DEFAULT, EEPROM_ReadByte(EEPROM_BLOCK_CRC_DEFAULT) ^ 0xFF); //сбрасываем настройки
                  RESET_SYSTEM; //перезагрузка
                  break;
                case DEVICE_UPDATE:
                  GPIOR0 = BOOTLOADER_START; //устанавливаем запрос обновления прошивки
                  RESET_SYSTEM; //перезагрузка
                  break;
                case DEVICE_REBOOT: RESET_SYSTEM; break; //перезагрузка
              }
            }
            break;
        }
#endif
        TWCR |= (0x01 << TWINT); //сбросили флаг прерывания
        break;
    }
  }
  return 0; //возвращаем статус ожидания шины
}
