/*
  Arduino IDE 1.8.13 версия прошивки 2.2.9 релиз от 27.12.25
  Универсальная прошивка для различных проектов часов на ГРИ под 4/6 ламп
  Страница прошивки на форуме - https://community.alexgyver.ru/threads/chasy-na-gri-alternativnaja-proshivka.5843/

  Исходник - https://github.com/radon-lab/NixieClock
  Автор Radon-lab.
*/

//----------------Библиотеки----------------
#include <util/delay.h>

//---------------Конфигурации---------------
#include "userConfig.h"
#include "connection.h"
#include "config.h"
#include "boards.h"

//----------------Периферия----------------
#include "EEPROM.h"
#include "INDICATION.h"
#include "PLAYER.h"
#include "SOUND.h"
#include "WIRE.h"
#include "RDA.h"
#include "RTC.h"
#include "AHT.h"
#include "SHT.h"
#include "BME.h"
#include "DHT.h"
#include "DS.h"
#include "IR.h"
#include "WS.h"
#include "LED.h"
#include "BUS.h"

//--------------------------------------Главный цикл программ---------------------------------------------------
int main(void) //главный цикл программ
{
  startEnableWDT(); //первичный запуск WDT
  INIT_SYSTEM(); //инициализация

  for (;;) {
#if ESP_ENABLE
    busCommand(); //проверка статуса шины
#endif
    dotReset(changeAnimState); //сброс анимации точек
#if BACKL_TYPE
    backlAnimEnable(); //разрешили эффекты подсветки
#endif
    changeBrightEnable(); //разрешить смену яркости
    changeBright(); //установка яркости от времени суток
    secsReset(); //сброс анимации секунд
#if INDI_SYMB_TYPE
    indiClrSymb(); //очистка индикатора символов
#endif

    switch (mainTask) {
      default: RESET_SYSTEM; break; //перезагрузка
      case MAIN_PROGRAM: mainTask = mainScreen(); break; //главный экран
#if !BTN_EASY_MAIN_MODE
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
      case TEMP_PROGRAM: mainTask = showTemp(); break; //показать температуру
#endif
      case DATE_PROGRAM: mainTask = showDate(); break; //показать дату
#endif
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
      case WARN_PROGRAM: mainTask = timerWarn(); break; //предупреждение таймера
#endif
#if ALARM_TYPE
      case ALARM_PROGRAM: //тревога будильника
        mainTask = alarmWarn(); //переход в программу
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
        radioPowerRet(); //вернуть питание радиоприемника
#endif
#if PLAYER_TYPE
        playerSetVolNow(mainSettings.volumeSound); //установили громкость
#endif
        break;
#endif
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
      case RADIO_PROGRAM: mainTask = radioMenu(); break; //радиоприемник
#endif
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
      case TIMER_PROGRAM: mainTask = timerStopwatch(); break; //таймер-секундомер
#endif
      case SLEEP_PROGRAM: //режим сна индикаторов
        mainTask = sleepIndi(); //переход в программу
        setAnimTimers(); //установка таймеров анимаций
        break;
#if !BTN_EASY_MAIN_MODE
      case FAST_SET_PROGRAM: mainTask = fastSetSwitch(); break; //переключение настроек
      case MAIN_SET_PROGRAM: mainTask = settings_main(); break; //основные настроки
#endif
      case CLOCK_SET_PROGRAM: mainTask = settings_time(); break; //настройки времени
#if ALARM_TYPE
      case ALARM_SET_PROGRAM: //настройка будильника
#if ALARM_TYPE == 1
        mainTask = settings_singleAlarm(); //переход в программу
#elif ALARM_TYPE == 2
        mainTask = settings_multiAlarm(); //переход в программу
#endif
#if PLAYER_TYPE
        playerSetVolNow(mainSettings.volumeSound); //установили громкость
#endif
        alarmCheck(ALARM_CHECK_SET); //проверяем будильники на совпадение
        break;
#endif
    }
  }

  return INIT_PROGRAM;
}
//--------------------------------------Инициализация---------------------------------------------------
void INIT_SYSTEM(void) //инициализация
{
#if GEN_ENABLE
  CONV_INIT; //инициализация преобразователя
#endif

#if ESP_ENABLE
  wireDisable(); //отключение шины
#endif

#if (PLAYER_TYPE != 1) || PLAYER_UART_MODE
  uartDisable(); //отключение uart
#endif

#if AMP_PORT_ENABLE
  AMP_INIT; //инициализация питания усилителя
#endif

#if !PLAYER_TYPE
  BUZZ_INIT; //инициализация бузера
#endif

#if SENS_PORT_ENABLE
  SENS_INIT; //инициализация порта датчиков температуры
#endif

#if !BTN_TYPE
  SET_INIT; //инициализация средней кнопки
  LEFT_INIT; //инициализация левой кнопки
  RIGHT_INIT; //инициализация правой кнопки
#endif

#if BTN_ADD_TYPE == 1
  ADD_INIT; //инициализация дополнительной кнопки
#endif

#if SQW_PORT_ENABLE
  SQW_INIT; //инициализация счета секунд
#endif

#if BACKL_TYPE
  BACKL_INIT; //инициализация подсветки
#endif

#if MOV_PORT_ENABLE
  MOV_INIT; //инициализация датчика движения
#endif

#if IR_PORT_ENABLE
  irInit(); //инициализация ик приемника
#endif

#if PLAYER_TYPE == 1
  DF_BUSY_INIT; //инициализация busy
  DF_RX_INIT; //инициализация rx
#elif PLAYER_TYPE == 2
  SD_CS_INIT; //иничиализация CS
  SD_SCK_INIT; //иничиализация SCK
  SD_MISO_INIT; //иничиализация MISO
  SD_MOSI_INIT; //иничиализация MOSI
#endif

#if GEN_ENABLE && (GEN_FEEDBACK == 2)
  FB_INIT; //инициализация обратной связи
  ACSR = (0x01 << ACBG); //включаем компаратор
#endif

#if SECS_DOT == 4
  decatronInit(); //инициализация декатрона
#endif

  indiPortInit(); //инициализация портов индикаторов

#if ESP_ENABLE
  if (GPIOR0 == BOOTLOADER_START) { //если был запрос начала обновления прошивки
    GPIOR0 = 0x00; //сбросили запрос начала обновления прошивки

    if (EEPROM_ReadByte(EEPROM_BLOCK_BOOT) == BOOTLOADER_OK) { //если загрузчик готов
      EEPROM_UpdateByte(EEPROM_BLOCK_BOOT, BOOTLOADER_START); //устанавливаем флаг запуска загрузчика
      RESET_BOOTLOADER; //переход к загрузчику
    }
  }
#endif

  if (checkByte(EEPROM_BLOCK_ERROR, EEPROM_BLOCK_CRC_ERROR)) updateByte(0x00, EEPROM_BLOCK_ERROR, EEPROM_BLOCK_CRC_ERROR); //если контрольная сумма ошибок не совпала
  if (checkByte(EEPROM_BLOCK_EXT_ERROR, EEPROM_BLOCK_CRC_EXT_ERROR)) updateByte(0x00, EEPROM_BLOCK_EXT_ERROR, EEPROM_BLOCK_CRC_EXT_ERROR); //если контрольная сумма расширеных ошибок не совпала

  checkVCC(); //чтение напряжения питания

  if (checkSettingsCRC() || !SET_CHK) { //если контрольная сумма не совпала или зажата средняя кнопка то восстанавливаем настройеи по умолчанию
    updateData((uint8_t*)&fastSettings, sizeof(fastSettings), EEPROM_BLOCK_SETTINGS_FAST, EEPROM_BLOCK_CRC_FAST); //записываем настройки яркости в память
    updateData((uint8_t*)&mainSettings, sizeof(mainSettings), EEPROM_BLOCK_SETTINGS_MAIN, EEPROM_BLOCK_CRC_MAIN); //записываем основные настройки в память
    updateData((uint8_t*)&radioSettings, sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO); //записываем настройки радио в память
#if ESP_ENABLE
    updateData((uint8_t*)&extendedSettings, sizeof(extendedSettings), EEPROM_BLOCK_SETTINGS_EXTENDED, EEPROM_BLOCK_CRC_EXTENDED); //записываем расширенные настройки в память
#endif
#if ALARM_TYPE
    updateByte(alarms.num, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
#endif
#if PLAYER_TYPE
    playerSetTrack(PLAYER_RESET_SOUND, PLAYER_GENERAL_FOLDER); //звук сброса настроек
#else
    melodyPlay(SOUND_RESET_SETTINGS, SOUND_LINK(general_sound), REPLAY_ONCE); //сигнал сброса настроек
#endif
  }
  else { //иначе загружаем настройки из памяти
    if (checkData(sizeof(fastSettings), EEPROM_BLOCK_SETTINGS_FAST, EEPROM_BLOCK_CRC_FAST)) { //проверяем быстрые настройки
      updateData((uint8_t*)&fastSettings, sizeof(fastSettings), EEPROM_BLOCK_SETTINGS_FAST, EEPROM_BLOCK_CRC_FAST); //записываем быстрые настройки в память
      SET_ERROR(MEMORY_ERROR); //устанавливаем ошибку памяти
    }
    else EEPROM_ReadBlock((uint16_t)&fastSettings, EEPROM_BLOCK_SETTINGS_FAST, sizeof(fastSettings)); //считываем быстрые настройки из памяти
    if (checkData(sizeof(mainSettings), EEPROM_BLOCK_SETTINGS_MAIN, EEPROM_BLOCK_CRC_MAIN)) { //проверяем основные настройки
      updateData((uint8_t*)&mainSettings, sizeof(mainSettings), EEPROM_BLOCK_SETTINGS_MAIN, EEPROM_BLOCK_CRC_MAIN); //записываем основные настройки в память
      SET_ERROR(MEMORY_ERROR); //устанавливаем ошибку памяти
    }
    else EEPROM_ReadBlock((uint16_t)&mainSettings, EEPROM_BLOCK_SETTINGS_MAIN, sizeof(mainSettings)); //считываем основные настройки из памяти
    if (checkData(sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO)) { //проверяем настройки радио
      updateData((uint8_t*)&radioSettings, sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO); //записываем настройки радио в память
      SET_ERROR(MEMORY_ERROR); //устанавливаем ошибку памяти
    }
    else EEPROM_ReadBlock((uint16_t)&radioSettings, EEPROM_BLOCK_SETTINGS_RADIO, sizeof(radioSettings)); //считываем настройки радио из памяти
#if ESP_ENABLE
    if (checkData(sizeof(extendedSettings), EEPROM_BLOCK_SETTINGS_EXTENDED, EEPROM_BLOCK_CRC_EXTENDED)) { //проверяем расширенные настройки
      updateData((uint8_t*)&extendedSettings, sizeof(extendedSettings), EEPROM_BLOCK_SETTINGS_EXTENDED, EEPROM_BLOCK_CRC_EXTENDED); //записываем расширенные настройки в память
      SET_ERROR(MEMORY_ERROR); //устанавливаем ошибку памяти
    }
    else EEPROM_ReadBlock((uint16_t)&extendedSettings, EEPROM_BLOCK_SETTINGS_EXTENDED, sizeof(extendedSettings)); //считываем настройки радио из памяти
#endif
#if ALARM_TYPE
    if (checkByte(EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM)) { //проверяем количетво будильников
      updateByte(alarms.num, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
      SET_ERROR(MEMORY_ERROR); //устанавливаем ошибку памяти
    }
    else alarms.num = EEPROM_ReadByte(EEPROM_BLOCK_ALARM); //считываем количество будильников из памяти
#endif
  }

  if (checkDebugSettingsCRC()) { //проверяем настройки отладки по умолчанию
    updateData((uint8_t*)&debugSettings, sizeof(debugSettings), EEPROM_BLOCK_SETTINGS_DEBUG, EEPROM_BLOCK_CRC_DEBUG); //записываем настройки отладки в память
#if LIGHT_SENS_ENABLE
    lightSensZoneUpdate(LIGHT_SENS_START_MIN, LIGHT_SENS_START_MAX); //обновление зон сенсора яркости освещения
#endif
  }
  if (checkData(sizeof(debugSettings), EEPROM_BLOCK_SETTINGS_DEBUG, EEPROM_BLOCK_CRC_DEBUG)) { //проверяем настройки отладки
    updateData((uint8_t*)&debugSettings, sizeof(debugSettings), EEPROM_BLOCK_SETTINGS_DEBUG, EEPROM_BLOCK_CRC_DEBUG); //записываем настройки отладки в память
    SET_ERROR(MEMORY_ERROR); //устанавливаем ошибку памяти
  }
  else EEPROM_ReadBlock((uint16_t)&debugSettings, EEPROM_BLOCK_SETTINGS_DEBUG, sizeof(debugSettings)); //считываем настройки отладки из памяти

#if GEN_ENABLE
#if GEN_FEEDBACK == 1
  updateTresholdADC(); //обновление предела удержания напряжения
#endif
  indiChangeCoef(); //обновление коэффициента линейного регулирования
#endif

#if PLAYER_TYPE == 1
  dfPlayerInit(); //инициализация DF плеера
#elif PLAYER_TYPE == 2
  sdPlayerInit(); //инициализация SD плеера
#endif

#if DS3231_ENABLE || ESP_ENABLE || RADIO_ENABLE || SENS_AHT_ENABLE || SENS_BME_ENABLE || SENS_SHT_ENABLE
  wireInit(); //инициализация шины wire
#endif
  coreInit(); //инициализация периферии ядра
  indiInit(); //инициализация индикации

  backlAnimDisable(); //запретили эффекты подсветки
  changeBrightDisable(CHANGE_DISABLE); //запретить смену яркости

  mainEnableWDT(); //основной запуск WDT

#if RADIO_ENABLE
  radioPowerOff(); //выключить питание радиоприемника
#endif

#if DS3231_ENABLE || SQW_PORT_ENABLE
  checkRealTimeClock(); //проверка модуля часов
#endif
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_BME_ENABLE || SENS_SHT_ENABLE || SENS_PORT_ENABLE
  checkTempSens(); //проверка установленного датчика температуры
#endif

#if PLAYER_TYPE
  playerSetVoice(mainSettings.voiceSound); //установили голос озвучки
#endif

  if (!LEFT_CHK) { //если левая кнопка зажата
#if DEBUG_PASS_ENABLE
    if (checkPass()) //если пароль верный
#endif
      debugMenu(); //запускаем отладку
  }
  else if (!RIGHT_CHK) testSystem(); //если правая кнопка зажата запускаем тест системы
#if FLIP_ANIM_START == 1
  else animShow = ANIM_MAIN; //установили флаг анимации
#elif FLIP_ANIM_START > 1
  else animShow = (ANIM_OTHER + FLIP_ANIM_START); //установили флаг анимации
#endif

  checkErrors(); //проверка на наличие ошибок

#if PLAYER_TYPE
  playerSetVolNow(mainSettings.volumeSound); //установили громкость
#endif

#if ALARM_TYPE
  alarmInit(); //инициализация будильника
#endif

  randomSeed(RTC.s * (RTC.m + RTC.h) + RTC.DD * RTC.MM); //радомный сид для глюков
  setAnimTimers(); //установка таймеров анимаций

#if DS3231_ENABLE
  _timer_sec[TMR_SYNC] = ((uint16_t)RTC_SYNC_TIME * 60); //устанавливаем таймер синхронизации
#endif

#if ALARM_TYPE
  alarmCheck(ALARM_CHECK_INIT); //проверка будильников
#endif

#if ESP_ENABLE
  wireSetAddress(WIRE_SLAVE_ADDR); //установка slave адреса шины
#endif
  mainTask = MAIN_PROGRAM; //установили основную программу
}
//----------------------------------Системная задача------------------------------------------------
void systemTask(void) //системная задача
{
  static uint16_t timerClock; //счетчик реального времени
  static uint16_t timerCorrect; //остаток для коррекции времени
#if DS3231_ENABLE || SQW_PORT_ENABLE
  static uint16_t timerSQW = SQW_MIN_TIME; //таймер контроля сигнала SQW
#endif

#if BACKL_TYPE == 3
  backlEffect(); //анимация светодиодов ws
  wsBacklShowLeds(); //отрисовка светодиодов ws
#elif BACKL_TYPE
  backlEffect(); //анимация подсветки led
#endif

  dotEffect(); //анимации точек

#if PLAYER_TYPE
  playerUpdate(); //обработка плеера
#if PLAYER_TYPE == 2
#if AMP_PORT_ENABLE
  if (!_timer_ms[TMR_PLAYER]) //если таймер истек
#endif
    readerUpdate(); //обработка SD плеера
#endif
#else
  melodyUpdate(); //обработка мелодий
#endif

#if (GEN_ENABLE && (GEN_FEEDBACK == 1)) || BTN_TYPE || LIGHT_SENS_ENABLE
  analogUpdate(); //обработка аналоговых входов
#endif

#if (GEN_ENABLE && (GEN_FEEDBACK == 2))
  feedbackUpdate(); //обработка обратной связи преобразователя
#endif

#if LIGHT_SENS_ENABLE
  lightSensCheck(); //проверка сенсора яркости освещения
#endif

#if ESP_ENABLE
  busUpdate(); //обновление статуса шины
#endif

  for (uint8_t _tick = tick_ms; _tick > 0; _tick--) { //если был тик то обрабатываем данные
    tick_ms--; //убавили счетчик миллисекунд

    indiStateCheck(); //проверка состояния динамической индикации
    uint8_t button = buttonStateUpdate(); //обновление состояния кнопок
    if (button) btn.state = button; //скопировали новую кнопку

    timerCorrect += debugSettings.timePeriod; //прибавляем период для коррекции
    uint8_t msDec = timerCorrect / 1000; //находим целые мс
    for (uint8_t tm = 0; tm < TIMERS_MS_NUM; tm++) { //опрашиваем все таймеры
      if (_timer_ms[tm]) { //если таймер активен
        if (_timer_ms[tm] > msDec) _timer_ms[tm] -= msDec; //если таймер больше периода
        else _timer_ms[tm] = 0; //иначе сбрасываем таймер
      }
    }
    timerCorrect %= 1000; //оставляем коррекцию

#if DS3231_ENABLE || SQW_PORT_ENABLE
    if (EIMSK) { //если работаем от внешнего тактирования
      timerSQW += msDec; //прибавили время
      if (timerSQW > SQW_MAX_TIME) { //если сигнал слишком длинный
        EIMSK = 0; //перешли на внутреннее тактирование
        tick_sec = 1; //установили секунду
        SET_ERROR(SQW_LONG_ERROR); //устанавливаем ошибку длинного сигнала
      }
    }
    else { //если внешние тактирование не обнаружено
#endif
      timerClock += msDec; //добавляем ко времени период таймера
      if (timerClock >= 1000) { //если прошла секунда
        timerClock -= 1000; //оставляем остаток
        tick_sec++; //прибавляем секунду
      }
#if DS3231_ENABLE || SQW_PORT_ENABLE
    }
#endif
  }

  if (tick_sec) { //если был тик, обрабатываем данные
    tick_sec--; //убавили счетчик секунд

#if GEN_ENABLE
    converterCheck(); //проверка состояния преобразователя
#endif
    indiCheck(); //проверка состояния динамической индикации

    for (uint8_t tm = 0; tm < TIMERS_SEC_NUM; tm++) { //опрашиваем все таймеры
      if (_timer_sec[tm]) _timer_sec[tm]--; //если таймер активен
    }

#if ALARM_TYPE
    alarmDataUpdate(); //проверка таймеров будильников
#endif

#if DS3231_ENABLE || SQW_PORT_ENABLE
    if (EIMSK) { //если работаем от внешнего тактирования
      if (timerSQW < SQW_MIN_TIME) { //если сигнал слишком короткий
        EIMSK = 0; //перешли на внутреннее тактирование
        tick_sec = 0; //сбросили счетчик секунд
        timerClock = timerSQW; //установили таймер секунды
        SET_ERROR(SQW_SHORT_ERROR); //устанавливаем ошибку короткого сигнала
        return; //выходим
      }
      timerSQW = 0; //сбросили таймер
    }
#endif
#if DS3231_ENABLE
    else if (!_timer_sec[TMR_SYNC] && RTC.s == RTC_SYNC_PHASE) { //если работаем от внутреннего тактирования
      _timer_sec[TMR_SYNC] = ((uint16_t)RTC_SYNC_TIME * 60); //установили таймер
      if (rtcGetTime(RTC_CHECK_OSF)) RTC.s--; //синхронизируем время
    }
#endif

    indi.update = dot.update = 0; //очищаем флаги секунды и точек

#if LAMP_NUM > 4
    if (animShow == ANIM_NULL) animShow = ANIM_SECS; //показать анимацию переключения цифр
#endif

#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
    timerUpdate(); //обработка таймера
#endif

    //счет времени
    if (++RTC.s > 59) { //секунды
      RTC.s = 0; //сбросили секунды
      if (++RTC.m > 59) { //минуты
        RTC.m = 0; //сбросили минуты
        if (++RTC.h > 23) { //часы
          RTC.h = 0; //сбросили часы
          if (++RTC.DW > 7) RTC.DW = 1; //день недели
          if (++RTC.DD > maxDays()) { //дата
            RTC.DD = 1; //сбросили день
            if (++RTC.MM > 12) { //месяц
              RTC.MM = 1; //сбросили месяц
              if (++RTC.YY > 2099) { //год
                RTC.YY = 2000; //сбросили год
              }
            }
          }
        }
        hourSound(); //звук смены часа
        light_update = 1; //устанавливаем флаг изменения яркости
      }
      if (fastSettings.flipMode && (animShow < ANIM_MAIN)) animShow = ANIM_MINS; //показать анимацию переключения цифр
#if ALARM_TYPE
      alarmCheck(ALARM_CHECK_MAIN); //проверяем будильники на совпадение
#endif
    }
#if LIGHT_SENS_ENABLE
    lightSensUpdate(); //обработка сенсора яркости освещения
#endif
#if MOV_PORT_ENABLE
    if (indi.sleepMode && MOV_CHK) {
      sleepReset(); //установли время ожидания режима сна
      if (mainTask == SLEEP_PROGRAM) indi.sleepMode = SLEEP_DISABLE; //отключаем сон если в режиме сна
    }
#endif

    if (light_update) { //если нужно изменить яркость
      light_update = 0; //сбрасываем флаг изменения яркости
      changeBright(); //установка текущей яркости
    }

#if (SECS_DOT != 3) && (SECS_DOT != 4) && DOTS_PORT_ENABLE && (ESP_ENABLE || DEFAULT_DOT_EXT_MODE)
    dotFlash(); //мигание точек
#endif

#if !PLAYER_TYPE
    if (mainSettings.baseSound == 2) { //если звук включен
      if (mainTask == MAIN_PROGRAM) { //если в режиме часов
        if (!melodyState()) { //если мелодия не воспроизводится
          if (RTC.s & 0x01) buzzPulse(SECS_UNEVEN_SOUND_FREQ, SECS_UNEVEN_SOUND_TIME); //щелчок пищалкой
          else buzzPulse(SECS_EVEN_SOUND_FREQ, SECS_EVEN_SOUND_TIME); //щелчок пищалкой
        }
      }
    }
#endif

    RESET_WDT; //сбрасываем таймер WDT
  }
}
//----------------------------------Обработка данных------------------------------------------------
void dataUpdate(void) //обработка данных
{
  systemTask(); //системная задача
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE
  updateTemp(); //обновить показания температуры
#endif
  updateMemory(); //обновить данные в памяти
}
//--------------------------Установка таймеров анимаций-------------------------
void setAnimTimers(void) //установка таймеров анимаций
{
  _timer_sec[TMR_SHOW] = getPhaseTime(mainSettings.autoShowTime, AUTO_SHOW_PHASE); //установка таймера показа температуры
  _timer_sec[TMR_BURN] = getPhaseTime(mainSettings.burnTime, BURN_PHASE); //установка таймера антиотравления
  _timer_sec[TMR_GLITCH] = random(GLITCH_MIN_TIME, GLITCH_MAX_TIME); //находим рандомное время появления глюка
}
//-------------------------Разрешить анимации подсветки-------------------------
void backlAnimEnable(void) //разрешить анимации подсветки
{
#if BACKL_TYPE == 3
  if (fastSettings.backlMode & 0x80) { //если эффекты подсветки были запрещены
    fastSettings.backlMode &= 0x7F; //разрешили эффекты подсветки
    backl.steps = 0; //сбросили шаги
    backl.drive = 0; //сбросили направление
    backl.position = 0; //сбросили позицию
    _timer_ms[TMR_COLOR] = 0; //сбросили таймер смены цвета
    _timer_ms[TMR_BACKL] = 0; //сбросили таймер анимации подсветки
    if (fastSettings.backlMode) wsBacklSetLedBright(backl.maxBright); //установили максимальную яркость
  }
#else
  fastSettings.backlMode &= 0x7F; //разрешили эффекты подсветки
#endif
}
//-------------------------Запретить анимации подсветки-------------------------
void backlAnimDisable(void) //запретить анимации подсветки
{
  fastSettings.backlMode |= 0x80; //запретили эффекты подсветки
}
//----------------------Разрешить анимацию секундных точек----------------------
void dotAnimEnable(void) //разрешить анимацию секундных точек
{
  fastSettings.dotMode &= 0x7F; //разрешили эффекты точек
}
//----------------------Запретить анимацию секундных точеки---------------------
void dotAnimDisable(void) //запретить анимацию секундных точек
{
  fastSettings.dotMode |= 0x80; //запретили эффекты точек
}
//----------------------------Разрешить смену яркости---------------------------
void changeBrightEnable(void) //разрешить смену яркости
{
  changeBrightState = CHANGE_ENABLE; //разрешили смену яркости
}
//----------------------------Запретить смену яркости---------------------------
void changeBrightDisable(uint8_t _state) //запретить смену яркости
{
  changeBrightState = _state; //запретили смену яркости
}
//-----------------------------Расчет шага яркости-----------------------------
uint8_t setBrightStep(uint16_t _brt, uint16_t _step, uint16_t _time) //расчет шага яркости
{
  uint8_t temp = ceil((float)_brt / (float)_time * (float)_step); //расчёт шага яркости точки
  if (!temp) temp = 1; //если шаг слишком мал, устанавливаем минимум
  return temp;
}
//-------------------------Расчет периода шага яркости--------------------------
uint16_t setBrightTime(uint16_t _brt, uint16_t _step, uint16_t _time) //расчет периода шага яркости
{
  uint16_t temp = ceil((float)_time / (float)_brt); //расчёт шага яркости точки
  if (temp < _step) temp = _step; //если шаг слишком мал то устанавливаем минимум
  return temp;
}
//---------------------Получить усредненную яркость-----------------------------
uint8_t getMidBright(uint8_t night, uint8_t day) //получить усредненную яркость
{
  return night + ((day - night) >> 1);
}
//---------------------Установка яркости от времени суток-----------------------------
boolean checkHourStrart(uint8_t _start, uint8_t _end) //установка яркости от времени суток
{
  return ((_start > _end && (RTC.h >= _start || RTC.h < _end)) || (_start < _end && RTC.h >= _start && RTC.h < _end));
}
//---------------------------------Получить время со сдвигом фазы-----------------------------------------
uint16_t getPhaseTime(uint8_t time, int8_t phase) //получить время со сдвигом фазы
{
  return ((uint16_t)time * 60) + (phase - RTC.s) - ((RTC.s >= phase) ? 0 : 60);  //возвращаем результат
}
//---------------------------------Инициализация будильника----------------------------------------------
void alarmInit(void) //инициализация будильника
{
  if (!alarms.num) alarmCreate(); //создать новый будильник
#if !ESP_ENABLE && (ALARM_TYPE == 1)
  else if (alarms.num > 1) { //если будильников в памяти больше одного
    alarms.num = 1; //оставляем один будильник
    updateByte(alarms.num, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
  }
#endif
}
//-----------------------------------Отключение будильника------------------------------------------------
void alarmDisable(void) //отключение будильника
{
#if PLAYER_TYPE
  if (mainSettings.baseSound) playerSetTrackNow(PLAYER_ALARM_DISABLE_SOUND, PLAYER_GENERAL_FOLDER); //звук выключения будильника
#else
  melodyPlay(SOUND_ALARM_DISABLE, SOUND_LINK(general_sound), REPLAY_ONCE); //звук выключения будильника
#endif
  alarmReset(); //сброс будильника
}
//--------------------------------------Сброс будильника--------------------------------------------------
void alarmReset(void) //сброс будильника
{
  _timer_sec[TMR_ALM] = 0; //сбрасываем таймер отключения будильника
  _timer_sec[TMR_ALM_WAIT] = 0; //сбрасываем таймер ожидания повторного включения тревоги
  _timer_sec[TMR_ALM_SOUND] = 0; //сбрасываем таймер отключения звука
  alarms.now = ALARM_DISABLE; //сбрасываем флаг тревоги
  alarmCheck(ALARM_CHECK_SET); //проверка будильников
  dotReset(ANIM_RESET_CHANGE); //сброс анимации точек
}
//-----------------------------Получить основные данные будильника-----------------------------------------
uint8_t alarmRead(uint8_t almNum, uint8_t almDataPos) //получить основные данные будильника
{
  return EEPROM_ReadByte(EEPROM_BLOCK_ALARM_DATA + ((uint16_t)almNum * ALARM_MAX_ARR) + almDataPos); //возвращаем запрошеный байт
}
//-----------------------------Записать основные данные будильника-----------------------------------------
void alarmWrite(uint8_t almNum, uint8_t almDataPos, uint8_t almData) //записать основные данные будильника
{
  EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + ((uint16_t)almNum * ALARM_MAX_ARR) + almDataPos, almData); //записываем указанный байт
}
//--------------------------Получить блок основных данных будильника---------------------------------------
void alarmReadBlock(uint8_t almNum, uint8_t* data) //получить блок основных данных будильника
{
  uint16_t curCell = (uint16_t)(almNum - 1) * ALARM_MAX_ARR;
  for (uint8_t i = 0; i < ALARM_MAX_ARR; i++) data[i] = (almNum) ? EEPROM_ReadByte(EEPROM_BLOCK_ALARM_DATA + curCell + i) : 0; //считываем блок данных
}
//---------------------------Записать блок основных данных будильника--------------------------------------
void alarmWriteBlock(uint8_t almNum, uint8_t* data) //записать блок основных данных будильника
{
  if (!almNum) return; //если нет ни одного будильника то выходим
  uint16_t curCell = (uint16_t)(almNum - 1) * ALARM_MAX_ARR;
  for (uint8_t i = 0; i < ALARM_MAX_ARR; i++) EEPROM_UpdateByte(EEPROM_BLOCK_ALARM_DATA + curCell + i, data[i]); //записываем блок данных
}
//---------------------------------Создать новый будильник-------------------------------------------------
void alarmCreate(void) //создать новый будильник
{
  if (alarms.num < MAX_ALARMS) { //если новый будильник меньше максимума
    uint16_t newCell = EEPROM_BLOCK_ALARM_DATA + ((uint16_t)alarms.num * ALARM_MAX_ARR);
    EEPROM_UpdateByte(newCell + ALARM_HOURS, DEFAULT_ALARM_TIME_HH); //устанавливаем час по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_MINS, DEFAULT_ALARM_TIME_MM); //устанавливаем минуты по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_MODE, DEFAULT_ALARM_MODE); //устанавливаем режим по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_DAYS, 0); //устанавливаем дни недели по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_SOUND, 0); //устанавливаем мелодию по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_VOLUME, DEFAULT_ALARM_VOLUME); //устанавливаем громкость по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_RADIO, 0); //устанавливаем радиобудильник по умолчанию
    EEPROM_UpdateByte(newCell + ALARM_STATUS, 0); //устанавливаем статус по умолчанию
    updateByte(++alarms.num, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
  }
}
//-----------------------------------Удалить будильник-----------------------------------------------------
void alarmRemove(uint8_t alarm) //удалить будильник
{
  if (alarms.num > 1) { //если будильник доступен
    for (uint8_t start = alarm; start < alarms.num; start++) { //перезаписываем массив будильников
      uint16_t oldCell = EEPROM_BLOCK_ALARM_DATA + ((uint16_t)start * ALARM_MAX_ARR);
      uint16_t newCell = EEPROM_BLOCK_ALARM_DATA + ((uint16_t)(start - 1) * ALARM_MAX_ARR);
      for (uint8_t block = 0; block < ALARM_MAX_ARR; block++) EEPROM_UpdateByte(newCell + block, EEPROM_ReadByte(oldCell + block));
    }
    updateByte(--alarms.num, EEPROM_BLOCK_ALARM, EEPROM_BLOCK_CRC_ALARM); //записываем количетво будильников в память
  }
}
//----------------------------------Проверка будильников----------------------------------------------------
void alarmCheck(uint8_t check) //проверка будильников
{
  if (alarms.now < ALARM_WAIT) { //если тревога не активна
    if (RTC.YY <= 2000) return; //выходим если время не установлено

    alarms.now = ALARM_DISABLE; //сбрасываем флаг будильников
    int16_t time_now = 1440 + ((int16_t)RTC.h * 60) + RTC.m; //рассчитали текущее время
    for (uint8_t alm = 0; alm < alarms.num; alm++) { //опрашиваем все будильники
      uint8_t mode_alarm = alarmRead(alm, ALARM_MODE); //считали режим будильника
      if (mode_alarm) { //если будильник включен
        alarms.now = ALARM_ENABLE; //мигание точек при включенном будильнике
        if (check == ALARM_CHECK_SET) return; //выходим если нужно только проверить

        uint8_t days_alarm = alarmRead(alm, ALARM_DAYS); //считали дни недели будильника
        int16_t time_alarm = ((int16_t)alarmRead(alm, ALARM_HOURS) * 60) + alarmRead(alm, ALARM_MINS);
        switch (mode_alarm) { //устанавливаем дни в зависимости от режима
          case 3: days_alarm = 0x3E; break; //по будням
          case 4: if (!days_alarm) days_alarm = 0xFF; else if (days_alarm & 0x80) days_alarm |= 0x01; break; //по дням недели
          default: days_alarm = 0xFF; break; //каждый день
        }

        uint8_t start_alarm = 0; //установили первоначальное время до будильника
        for (uint8_t dw = RTC.DW - 1; dw <= RTC.DW; dw++) { //проверяем все дни недели
          if (days_alarm & (0x01 << dw)) { //если активирован день недели
            int16_t time_buf = time_now - time_alarm; //расчет интервала
            if (!time_buf) start_alarm = 1; //если будильник в зоне активации
            else if ((time_buf > 0) && (time_buf < 30)) start_alarm = 2; //если будильник в зоне активации
          }
          time_alarm += 1440; //прибавили время будильнику
        }

        uint8_t status_alarm = alarmRead(alm, ALARM_STATUS); //считали статус будильника

        if (status_alarm) { //если будильник заблокирован автоматически
          if (status_alarm == 255) { //если будильник заблокирован пользователем
            if ((check == ALARM_CHECK_INIT) || (start_alarm < 2)) { //если первичная проверка будильников или будильник вне зоны активации
              status_alarm = 0; //сбросили статус блокировки пользователем
              alarmWrite(alm, ALARM_STATUS, status_alarm); //устанавливаем статус активности будильник
            }
          }
          else if ((status_alarm != RTC.DW) && !start_alarm) { //если вышли из зоны активации будильника
            status_alarm = 0; //устанавливаем статус блокировки будильника
            alarmWrite(alm, ALARM_STATUS, status_alarm); //устанавливаем статус активности будильник
          }
        }
        if (!status_alarm && start_alarm) { //если будильник не был заблокирован
          alarms.now = ALARM_WARN; //устанавливаем флаг тревоги
          if (mode_alarm == 1) { //если был установлен режим одиночный
#if ESP_ENABLE
            device.status |= (0x01 << STATUS_UPDATE_ALARM_SET);
#endif
            alarmWrite(alm, ALARM_MODE, 0); //выключаем будильник
          }
          alarmWrite(alm, ALARM_STATUS, RTC.DW); //сбрасываем статус активности будильник
          alarms.sound = alarmRead(alm, ALARM_SOUND); //номер мелодии
          alarms.radio = alarmRead(alm, ALARM_RADIO); //текущий режим звука
          alarms.volume = alarmRead(alm, ALARM_VOLUME); //текущая громкость
          _timer_sec[TMR_ALM] = ((uint16_t)extendedSettings.alarmTime * 60); //установили таймер таймаута будильника
          _timer_sec[TMR_ALM_SOUND] = ((uint16_t)extendedSettings.alarmSoundTime * 60); //установили таймер таймаута звука будильника
          return; //выходим
        }
      }
    }
  }
}
//-------------------------------Обновление данных будильников---------------------------------------------
void alarmDataUpdate(void) //обновление данных будильников
{
  if (alarms.now > ALARM_ENABLE) { //если тревога активна
    if (!_timer_sec[TMR_ALM]) { //если пришло время выключить будильник
      alarmReset(); //сброс будильника
      return; //выходим
    }

    if (extendedSettings.alarmWaitTime && (alarms.now == ALARM_WAIT)) { //если будильник в режиме ожидания
      if (!_timer_sec[TMR_ALM_WAIT]) { //если пришло время повторно включить звук
        _timer_sec[TMR_ALM_SOUND] = ((uint16_t)extendedSettings.alarmSoundTime * 60); //установили таймер таймаута звука будильника
        alarms.now = ALARM_WARN; //устанавливаем флаг тревоги будильника
      }
    }
    else if (extendedSettings.alarmSoundTime) { //если таймаут тревоги включен
      if (!_timer_sec[TMR_ALM_SOUND]) { //если пришло время выключить тревогу
        if (extendedSettings.alarmWaitTime) { //если время ожидания включено
          _timer_sec[TMR_ALM_WAIT] = ((uint16_t)extendedSettings.alarmWaitTime * 60); //установили таймер таймаута ожидания будильника
          alarms.now = ALARM_WAIT; //устанавливаем флаг ожидания тревоги
        }
        else alarmReset(); //сброс будильника
      }
    }
  }
}
//----------------------------------Тревога будильника---------------------------------------------------------
uint8_t alarmWarn(void) //тревога будильника
{
  boolean blink_data = 0; //флаг мигания индикаторами

#if PLAYER_TYPE || (RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE))
  boolean auto_vol = 0; //флаг автогромкости
  uint8_t cur_vol = alarms.volume; //текущая громкость

  if (!cur_vol) { //если автогромкость
    auto_vol = 1; //установили флаг автогромкости
    cur_vol = ALARM_AUTO_VOL_MIN; //установили минимальную громкость
  }

  _timer_ms[TMR_ANIM] = ALARM_AUTO_VOL_TIMER; //устанавливаем таймер
#endif

#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
  if (alarms.radio) { //если режим радио
    if (getPowerStatusRDA() != RDA_ERROR) { //если радиоприемник доступен
      setPowerRDA(RDA_ON); //включаем радио
      setVolumeRDA(cur_vol); //устанавливаем громкость
      setFreqRDA(radioSettings.stationsSave[alarms.sound]); //устанавливаем частоту
    }
    else { //иначе переходим в режим мелодии
      alarms.sound = 0; //установили номер мелодии
      alarms.radio = 0; //отключили режим радио
    }
  }
  else radioPowerOff(); //выключить питание радиоприемника
#endif

#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
  if (!alarms.radio) {
#endif
#if PLAYER_TYPE
    playerStop(); //остановить воспроизведение
    playerSetVolNow(cur_vol); //установить громкость
#else
    melodyPlay(alarms.sound, SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
  }
#endif

#if (BACKL_TYPE == 3) && ALARM_BACKL_TYPE
  backlAnimDisable(); //запретили эффекты подсветки
#if ALARM_BACKL_TYPE == 1
  changeBrightDisable(CHANGE_DYNAMIC_BACKL); //разрешить смену яркости динамичной подсветки
#endif
  wsBacklSetLedHue(ALARM_BACKL_COLOR, WHITE_ON); //установили цвет будильника
#endif

  _timer_ms[TMR_MS] = 0; //сбросили таймер

  while (1) {
    dataUpdate(); //обработка данных

    if (alarms.now != ALARM_WARN) { //если тревога сброшена
#if PLAYER_TYPE
      playerStop(); //сброс позиции мелодии
#else
      melodyStop(); //сброс позиции мелодии
#endif
      return MAIN_PROGRAM; //выходим
    }

#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
#if PLAYER_TYPE
    if (!alarms.radio && !playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarms.sound, PLAYER_ALARM_FOLDER); //воспроизводим мелодию
    if (auto_vol && !_timer_ms[TMR_ANIM]) { //если пришло время
      _timer_ms[TMR_ANIM] = ALARM_AUTO_VOL_TIMER; //устанавливаем таймер
      if (cur_vol < ALARM_AUTO_VOL_MAX) cur_vol++;
      else auto_vol = 0; //сбросили флаг автогромкости

      if (alarms.radio) setVolumeRDA(cur_vol); //устанавливаем громкость
      else playerSetVolNow(cur_vol); //установка громкости
    }
#else
    if (auto_vol && !_timer_ms[TMR_ANIM]) { //если пришло время
      _timer_ms[TMR_ANIM] = ALARM_AUTO_VOL_TIMER; //устанавливаем таймер
      if (cur_vol < ALARM_AUTO_VOL_MAX) cur_vol++;
      else auto_vol = 0; //сбросили флаг автогромкости
      setVolumeRDA(cur_vol); //устанавливаем громкость
    }
#endif
#elif PLAYER_TYPE
    if (!playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarms.sound, PLAYER_ALARM_FOLDER); //воспроизводим мелодию
    if (auto_vol && !_timer_ms[TMR_ANIM]) { //если пришло время
      _timer_ms[TMR_ANIM] = ALARM_AUTO_VOL_TIMER; //устанавливаем таймер
      if (cur_vol < ALARM_AUTO_VOL_MAX) cur_vol++;
      else auto_vol = 0; //сбросили флаг автогромкости
      playerSetVolNow(cur_vol); //установка громкости
    }
#endif

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = ALARM_BLINK_TIME; //устанавливаем таймер

      switch (blink_data) {
        case 0: indiClr(); break; //очистка индикаторов
        case 1:
          indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
          indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
          indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
          break;
      }
      dotSetBright((blink_data) ? dot.menuBright : 0); //установили точки
#if (BACKL_TYPE == 3) && ALARM_BACKL_TYPE
#if ALARM_BACKL_TYPE == 1
      wsBacklSetLedBright((blink_data) ? backl.maxBright : 0); //установили яркость
#else
      wsBacklSetLedBright((blink_data) ? backl.menuBright : 0); //установили яркость
#endif
#endif
      blink_data = !blink_data; //мигаем временем
    }

    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
      case RIGHT_KEY_PRESS: //клик правой кнопкой
      case SET_KEY_PRESS: //клик средней кнопкой
      case ADD_KEY_PRESS: //клик дополнительной кнопкой
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE) && ALARM_RADIO_CONTINUE
        if (extendedSettings.alarmWaitTime && !alarms.radio) //если есть время ожидания и режим музыкального будильника
#else
        if (extendedSettings.alarmWaitTime) //если есть время ожидания
#endif
        {
          alarms.now = ALARM_WAIT; //устанавливаем флаг ожидания
          _timer_sec[TMR_ALM_WAIT] = ((uint16_t)extendedSettings.alarmWaitTime * 60);
          _timer_sec[TMR_ALM_SOUND] = 0;
#if PLAYER_TYPE
          if (mainSettings.baseSound) playerSetTrackNow(PLAYER_ALARM_WAIT_SOUND, PLAYER_GENERAL_FOLDER); //звук ожидания будильника
#else
          melodyPlay(SOUND_ALARM_WAIT, SOUND_LINK(general_sound), REPLAY_ONCE); //звук ожидания будильника
#endif
        }
        else {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE) && ALARM_RADIO_CONTINUE
          if (alarms.radio) {
            radioSettings.stationsFreq = radioSettings.stationsSave[alarms.sound];
            radio.powerState = RDA_ON; //установили флаг питания радио
          }
#endif
          alarmDisable(); //отключение будильника
        }
        return MAIN_PROGRAM; //выходим

      case LEFT_KEY_HOLD: //удержание левой кнопки
      case RIGHT_KEY_HOLD: //удержание правой кнопки
      case SET_KEY_HOLD: //удержание средней кнопки
      case ADD_KEY_HOLD: //удержание дополнительной кнопки
        alarmDisable(); //отключение будильника
        return MAIN_PROGRAM; //выходим
    }
  }
  return INIT_PROGRAM;
}
//-------------------Установить флаг обновления данных в памяти---------------------
void setUpdateMemory(uint8_t mask) //установить флаг обновления данных в памяти
{
  memoryUpdate |= mask; //установили флаг
#if ESP_ENABLE
  device.status |= mask; //запоминаем статус
#endif
}
//----------------------------Обновить данные в памяти------------------------------
void updateMemory(void) //обновить данные в памяти
{
  if (!_timer_sec[TMR_MEM] && memoryUpdate) { //если нужно сохранить настройки
    _timer_sec[TMR_MEM] = 3; //установили таймер
    for (uint8_t i = 0; i < MEM_MAX_DATA; i++) { //проверяем все флаги
      if (memoryUpdate & 0x01) { //если флаг установлен
        switch (i) { //выбираем действие
          case MEM_UPDATE_MAIN_SET: updateData((uint8_t*)&mainSettings, sizeof(mainSettings), EEPROM_BLOCK_SETTINGS_MAIN, EEPROM_BLOCK_CRC_MAIN); break; //записываем основные настройки в память
          case MEM_UPDATE_FAST_SET: updateData((uint8_t*)&fastSettings, sizeof(fastSettings), EEPROM_BLOCK_SETTINGS_FAST, EEPROM_BLOCK_CRC_FAST); break; //записываем быстрые настройки в память
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
          case MEM_UPDATE_RADIO_SET: updateData((uint8_t*)&radioSettings, sizeof(radioSettings), EEPROM_BLOCK_SETTINGS_RADIO, EEPROM_BLOCK_CRC_RADIO); break; //записываем настройки радио в память
#endif
#if ESP_ENABLE
          case MEM_UPDATE_EXTENDED_SET: updateData((uint8_t*)&extendedSettings, sizeof(extendedSettings), EEPROM_BLOCK_SETTINGS_EXTENDED, EEPROM_BLOCK_CRC_EXTENDED); break; //записываем расширенные настройки в память
#endif
        }
      }
      memoryUpdate >>= 1; //сместили буфер флагов
    }
    memoryUpdate = 0; //сбрасываем флаги
  }
}
//------------------Чтение установленных датчиков температуры-----------------------
void readTempSens(void) //чтение установленных датчиков температуры
{
  if (sens.type & ~((0x01 << SENS_DS3231) | (0x01 << SENS_ALL))) { //если датчик обнаружен
    uint8_t pos = (0x01 << SENS_DHT); //установили тип датчика
    for (uint8_t sensor = (SENS_ALL - 1); sensor; sensor--) { //перебираем все датчики температуры
      sens.update = 0; //сбросили флаг проверки датчика температуры на ошибку связи
      if (sens.type & pos) { //если флаг датчика установлен
        switch (sensor) { //выбор датчика температуры
#if SENS_AHT_ENABLE
          case SENS_AHT: readTempAHT(); break; //чтение температуры/влажности с датчика AHT
#endif
#if SENS_SHT_ENABLE
          case SENS_SHT: readTempSHT(); break; //чтение температуры/влажности с датчика SHT
#endif
#if SENS_BME_ENABLE
          case SENS_BME: readTempBME(); break; //чтение температуры/давления/влажности с датчика BME/BMP
#endif
#if (SENS_PORT_ENABLE == 1) || (SENS_PORT_ENABLE == 3)
          case SENS_DS18: readTempDS(); break; //чтение температуры с датчика DS18x20
#endif
#if (SENS_PORT_ENABLE == 2) || (SENS_PORT_ENABLE == 3)
          case SENS_DHT: readTempDHT(); break; //чтение температуры/влажности с датчика DHT/MW/AM
#endif
        }
        if (!sens.update) sens.type &= ~pos; //сбросили флаг сенсора
        else if (sensor >= SENS_DS18) { //если тип датчика DHT/DS18
          sens.type = pos | (0x01 << SENS_ALL); //установили тип датчика
          break; //выходим
        }
      }
      pos >>= 1; //сместили тип датчика
    }
#if SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE
    if (sens.temp > 850) sens.temp = 850; //ограничили температуру
    if (sens.temp < -850) sens.temp = -850; //ограничили температуру
    if (sens.hum > 99) sens.hum = 99; //ограничили влажность
#endif
  }
}
//------------------Обновление установленных датчиков температуры-----------------------
void updateTempSens(void) //обновление установленных датчиков температуры
{
#if SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE
  readTempSens(); //чтение установленного датчика температуры
#endif

  if (!(sens.type & ~((0x01 << SENS_DS3231) | (0x01 << SENS_ALL)))) { //если основной датчик не отвечает
#if DS3231_ENABLE == 2
    if (rtcReadTemp()) { //чтение температуры с датчика DS3231
      sens.type |= (0x01 << SENS_DS3231); //установили флаг сенсора
      sens.update = 1; //установили флаг обновления сенсора
    }
    else {
      sens.type &= ~(0x01 << SENS_DS3231); //сбросили флаг сенсора
#endif
      sens.temp = 0x7FFF; //сбрасываем температуру
      sens.hum = 0; //сбрасываем влажность
      sens.press = 0; //сбрасываем давление
#if DS3231_ENABLE == 2
    }
#endif
  }

#if ESP_ENABLE
  if (sens.init && !(device.status & (0x01 << STATUS_UPDATE_SENS_DATA))) device.status |= (0x01 << STATUS_UPDATE_SENS_DATA); //если первичная инициализация пройдена и есп получила последние данные
  else { //иначе копируем внутренние данные
    extSens.temp = 0x7FFF; //сбрасываем температуру
    extSens.hum = 0; //сбрасываем влажность
    extSens.press = 0; //сбрасываем давление
    sens.init = 1; //установили флаг инициализации сенсора
  }
#endif

#if ESP_ENABLE
  _timer_ms[TMR_SENS] = TEMP_ESP_UPDATE_TIME; //установили таймаут ожидания опроса есп
#else
  _timer_ms[TMR_SENS] = TEMP_UPDATE_TIME; //установили интервал следующего опроса
#endif
}
//-----------------Проверка установленного датчика температуры----------------------
void checkTempSens(void) //проверка установленного датчика температуры
{
#if SENS_AHT_ENABLE || SENS_BME_ENABLE || SENS_SHT_ENABLE || SENS_PORT_ENABLE
  sens.type = (0x01 << SENS_AHT) | (0x01 << SENS_SHT) | (0x01 << SENS_BME) | (0x01 << SENS_DS18) | (0x01 << SENS_DHT) | (0x01 << SENS_ALL);
#elif DS3231_ENABLE == 2
  sens.type = (0x01 << SENS_ALL);
#endif
  updateTempSens(); //чтение установленных датчиков температуры
#if SENS_AHT_ENABLE || SENS_BME_ENABLE || SENS_SHT_ENABLE || SENS_PORT_ENABLE
  if (!(sens.type & ~((0x01 << SENS_DS3231) | (0x01 << SENS_ALL)))) SET_ERROR(TEMP_SENS_ERROR); //иначе выдаем ошибку
#endif
}
//-------------------------Обновить показания температуры---------------------------
void updateTemp(void) //обновить показания температуры
{
  if (!_timer_ms[TMR_SENS]) { //если пришло время нового запроса температуры
    updateTempSens(); //обновление установленных датчиков температуры
  }
}
//-----------------------Получить текущий основной датчик--------------------------
uint8_t getMainSens(void)
{
  return (extendedSettings.tempMainSensor) ? SHOW_TEMP_ESP : SHOW_TEMP;
}
//---------------------Получить текущий датчик озвучки часа------------------------
uint8_t getHourSens(void)
{
  return (extendedSettings.tempHourSensor) ? SHOW_TEMP_ESP : SHOW_TEMP;
}
//------------------------Получить показания температуры---------------------------
int16_t getTemperatureData(uint8_t data)
{
  if (data >= SHOW_TEMP_ESP) return extSens.temp + ((extendedSettings.tempCorrectSensor == 2) ? mainSettings.tempCorrect : 0);
  return sens.temp + ((extendedSettings.tempCorrectSensor == 1) ? mainSettings.tempCorrect : 0);
}
//------------------------Получить показания температуры---------------------------
int16_t getTemperatureData(void)
{
#if ESP_ENABLE
  return getTemperatureData(getMainSens());
#else
  return sens.temp + mainSettings.tempCorrect;
#endif
}
//--------------------------Получить знак температуры------------------------------
boolean getTemperatureSign(uint8_t data)
{
  return getTemperatureData(data) < 0;
}
//--------------------------Получить знак температуры------------------------------
boolean getTemperatureSign(void)
{
#if ESP_ENABLE
  return getTemperatureSign(getMainSens());
#else
  return getTemperatureData() < 0;
#endif
}
//------------------------Получить показания температуры---------------------------
uint16_t getTemperature(uint8_t data)
{
  int16_t temp = getTemperatureData(data);
  return (temp < 0) ? -temp : temp;
}
//------------------------Получить показания температуры---------------------------
uint16_t getTemperature(void)
{
  int16_t temp = getTemperatureData();
  return (temp < 0) ? -temp : temp;
}
//--------------------------Получить показания давления----------------------------
uint16_t getPressure(uint8_t data)
{
  if (data >= SHOW_TEMP_ESP) return extSens.press;
  return sens.press;
}
//--------------------------Получить показания давления----------------------------
uint16_t getPressure(void)
{
#if ESP_ENABLE
  return getPressure(getMainSens());
#else
  return sens.press;
#endif
}
//-------------------------Получить показания влажности----------------------------
uint8_t getHumidity(uint8_t data)
{
  if (data >= SHOW_TEMP_ESP) return extSens.hum;
  return sens.hum;
}
//-------------------------Получить показания влажности----------------------------
uint8_t getHumidity(void)
{
#if ESP_ENABLE
  return getHumidity(getMainSens());
#else
  return sens.hum;
#endif
}
//------------------------------------Звук смены часа------------------------------------
void hourSound(void) //звук смены часа
{
  if (checkHourStrart(mainSettings.timeHour[0], mainSettings.timeHour[1])) {
    if ((mainTask == MAIN_PROGRAM) || (mainTask == SLEEP_PROGRAM)) { //если в режиме часов или спим
#if PLAYER_TYPE
      uint8_t temp = mainSettings.hourSound;
      if (!(temp & 0x03)) {
        if (mainSettings.baseSound) temp |= 0x02;
        else temp = 0x01;
      }
      playerStop(); //сброс воспроизведения плеера
      if (temp & 0x01) playerSetTrackNow(PLAYER_HOUR_SOUND, PLAYER_GENERAL_FOLDER); //звук смены часа
      if (temp & 0x02) speakTime(temp & 0x01); //воспроизвести время
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
      if (temp & 0x80) { //воспроизвести температуру
#if ESP_ENABLE
        if (getTemperature(getHourSens()) <= 990) speakTemp(SPEAK_TEMP_HOUR); //воспроизвести целую температуру
#else
        if (getTemperature() <= 990) speakTemp(SPEAK_TEMP_HOUR); //воспроизвести целую температуру
#endif
      }
#endif
#else
      melodyPlay(SOUND_HOUR, SOUND_LINK(general_sound), REPLAY_ONCE); //звук смены часа
#endif
    }
  }
}
//---------------------Установка яркости от времени суток-----------------------------
void changeBright(void) //установка яркости от времени суток
{
  indi.sleepMode = SLEEP_DISABLE; //сбросили флаг режима сна индикаторов

#if LIGHT_SENS_ENABLE || ESP_ENABLE
  if (mainSettings.timeBright[TIME_NIGHT] != mainSettings.timeBright[TIME_DAY])
#endif
    light_state = (checkHourStrart(mainSettings.timeBright[TIME_NIGHT], mainSettings.timeBright[TIME_DAY])) ? 2 : 0;
#if !LIGHT_SENS_ENABLE && ESP_ENABLE
  else light_state = device.light;
#endif

  switch (light_state) {
    case 0: //дневной режим
#if ((SECS_DOT != 3) || !DOTS_PORT_ENABLE) && (SECS_DOT != 4)
      dot.menuBright = dot.maxBright = mainSettings.dotBright[TIME_DAY]; //установка максимальной яркости точек
#else
      dot.menuBright = dot.maxBright = 1; //установка максимальной яркости точек
#endif
#if BACKL_TYPE
      backl.menuBright = backl.maxBright = mainSettings.backlBright[TIME_DAY]; //установка максимальной яркости подсветки
#endif
      indi.maxBright = mainSettings.indiBright[TIME_DAY]; //установка максимальной яркости индикаторов
      if (mainSettings.timeSleep[TIME_DAY]) indi.sleepMode = SLEEP_DAY; //установили флаг режима сна индикаторов
      break;
#if LIGHT_SENS_ENABLE || ESP_ENABLE
    case 1: //промежуточный режим
#if ((SECS_DOT != 3) || !DOTS_PORT_ENABLE) && (SECS_DOT != 4)
      dot.maxBright = dot.menuBright = getMidBright(mainSettings.dotBright[TIME_NIGHT], mainSettings.dotBright[TIME_DAY]); //установка максимальной яркости точек
#else
      dot.menuBright = dot.maxBright = 1; //установка максимальной яркости точек
#endif
#if BACKL_TYPE
      backl.menuBright = getMidBright(mainSettings.backlBright[TIME_NIGHT], mainSettings.backlBright[TIME_DAY]); //установка максимальной яркости подсветки
#endif
      indi.maxBright = getMidBright(mainSettings.indiBright[TIME_NIGHT], mainSettings.indiBright[TIME_DAY]); //установка максимальной яркости индикаторов
      if (mainSettings.timeSleep[TIME_DAY]) indi.sleepMode = SLEEP_DAY; //установили флаг режима сна индикаторов
      break;
#endif
    default: //ночной режим
      dot.maxBright = mainSettings.dotBright[TIME_NIGHT]; //установка максимальной яркости точек
      dot.menuBright = (dot.maxBright) ? dot.maxBright : 10; //установка максимальной яркости точек в меню
#if BACKL_TYPE
      backl.maxBright = mainSettings.backlBright[TIME_NIGHT]; //установка максимальной яркости подсветки
      backl.menuBright = (backl.maxBright) ? backl.maxBright : 10; //установка максимальной яркости подсветки в меню
#endif
      indi.maxBright = mainSettings.indiBright[TIME_NIGHT]; //установка максимальной яркости индикаторов
      if (mainSettings.timeSleep[TIME_NIGHT]) indi.sleepMode = SLEEP_NIGHT; //установили флаг режима сна индикаторов
      break;
  }

  if (changeBrightState) { //если разрешено менять яркость
    if (mainTask == MAIN_PROGRAM) { //если основной режим
      switch (dotGetMode()) { //мигание точек
        case DOT_OFF: dotSetBright(0); break; //точки выключены
        case DOT_STATIC: dotSetBright(dot.maxBright); break; //точки включены
        default:
#if (SECS_DOT != 3) && (SECS_DOT != 4)
          if (!dot.maxBright) dotSetBright(0); //если яркость не установлена
          else if (dotGetBright()) dotSetBright(dot.maxBright); //установка яркости точек
#endif
#if SECS_DOT == 4
          if (!dot.maxBright) decatronDisable(); //отключение декатрона
#endif
#if DOTS_PORT_ENABLE
          if (!dot.maxBright) indiClrDots(); //очистка разделителных точек
#endif
          break;
      }

#if (SECS_DOT == 0) || (SECS_DOT == 1) || (SECS_DOT == 2)
      if (dot.maxBright) { //пересчитываем шаги
#if (SECS_DOT == 0) || (SECS_DOT == 1)
        dot.brightStep = setBrightStep(dot.maxBright * 2, DOT_PULS_STEP_TIME, DOT_PULS_TIME); //расчёт шага яркости точки
        dot.brightTime = setBrightTime(dot.maxBright * 2, DOT_PULS_STEP_TIME, DOT_PULS_TIME); //расчёт периода шага яркости точки
#endif
#if SECS_DOT == 2
        dot.brightTurnStep = setBrightStep(dot.maxBright * 2, DOT_PULS_TURN_STEP_TIME, DOT_PULS_TURN_TIME); //расчёт шага яркости точки
        dot.brightTurnTime = setBrightTime(dot.maxBright * 2, DOT_PULS_TURN_STEP_TIME, DOT_PULS_TURN_TIME); //расчёт периода шага яркости точки
#endif
      }
#endif
    }
#if SECS_DOT < 2
    else if (dotGetBright()) dotSetBright(dot.menuBright); //установка яркости точек в меню
#elif SECS_DOT == 2
    else if (dotGetBright()) neonDotSetBright(dot.menuBright); //установка яркости точек в меню
#endif

#if BACKL_TYPE
    if (fastSettings.backlMode & 0x80) { //если подсветка заблокирована
#if BACKL_TYPE == 3
      switch (changeBrightState) { //режим управления яркостью
        case CHANGE_STATIC_BACKL: if (fastSettings.backlMode & 0x7F) wsBacklSetLedBright(backl.maxBright); break; //устанавливаем максимальную яркость
        case CHANGE_DYNAMIC_BACKL: wsBacklSetOnLedBright(backl.maxBright); break; //устанавливаем максимальную яркость
        default: wsBacklSetOnLedBright(backl.menuBright); break; //установка яркости подсветки в меню
      }
#else
      if (ledBacklGetBright()) ledBacklSetBright(backl.menuBright); //установили яркость если она была включена
#endif
    }
    else { //иначе устанавливаем яркость
#if BACKL_TYPE == 3
      if (backl.maxBright) {
        switch (fastSettings.backlMode) {
          case BACKL_OFF: wsBacklClearLeds(); break; //выключили светодиоды
          case BACKL_STATIC:
            wsBacklSetLedBright(backl.maxBright); //устанавливаем максимальную яркость
            wsBacklSetLedHue(fastSettings.backlColor, WHITE_ON); //устанавливаем статичный цвет
            break;
          case BACKL_SMOOTH_COLOR_CHANGE:
          case BACKL_RAINBOW:
          case BACKL_CONFETTI:
            wsBacklSetLedBright(backl.maxBright); //устанавливаем максимальную яркость
            break;
        }
      }
      else wsBacklClearLeds(); //выключили светодиоды
#else
      switch (fastSettings.backlMode) {
        case BACKL_OFF: ledBacklSetBright(0); break; //если посветка выключена
        case BACKL_STATIC: ledBacklSetBright(backl.maxBright); break; //если посветка статичная, устанавливаем яркость
        case BACKL_PULS: if (!backl.maxBright) ledBacklSetBright(0); break; //иначе посветка выключена
      }
#endif
      if (backl.maxBright) {
        backl.minBright = (backl.maxBright > (BACKL_MIN_BRIGHT + 10)) ? BACKL_MIN_BRIGHT : 0;
        uint8_t backlNowBright = (backl.maxBright > BACKL_MIN_BRIGHT) ? (backl.maxBright - BACKL_MIN_BRIGHT) : backl.maxBright;

        backl.mode_2_time = setBrightTime((uint16_t)backlNowBright * 2, BACKL_MODE_2_STEP_TIME, BACKL_MODE_2_TIME); //расчёт периода шага яркости
        backl.mode_2_step = setBrightStep((uint16_t)backlNowBright * 2, BACKL_MODE_2_STEP_TIME, BACKL_MODE_2_TIME); //расчёт шага яркости

#if BACKL_TYPE == 3
        backl.mode_4_step = ceil((float)backl.maxBright / (float)BACKL_MODE_4_TAIL / (float)BACKL_MODE_4_FADING); //расчёт шага яркости
        if (!backl.mode_4_step) backl.mode_4_step = 1; //если шаг слишком мал
        backl.mode_8_time = setBrightTime((uint16_t)backlNowBright * LEDS_NUM, BACKL_MODE_8_STEP_TIME, BACKL_MODE_8_TIME); //расчёт периода шага яркости
        backl.mode_8_step = setBrightStep((uint16_t)backlNowBright * LEDS_NUM, BACKL_MODE_8_STEP_TIME, BACKL_MODE_8_TIME); //расчёт шага яркости
#endif
      }
    }
#endif
#if BURN_BRIGHT
    if (changeBrightState != CHANGE_INDI_BLOCK) indiSetBright(indi.maxBright); //установка общей яркости индикаторов
#else
    indiSetBright(indi.maxBright); //установка общей яркости индикаторов
#endif
#if INDI_SYMB_TYPE
    indiSetSymbBright(indi.maxBright); //установка яркости индикатора символов
#endif
  }
}
//----------------------------------Анимация подсветки---------------------------------
void backlEffect(void) //анимация подсветки
{
#if BACKL_TYPE == 3
  if (backl.maxBright) { //если подсветка не выключена
    if (!_timer_ms[TMR_BACKL]) { //если время пришло
      switch (fastSettings.backlMode) {
        case BACKL_OFF: //подсветка выключена
        case BACKL_STATIC: //статичный режим
          return; //выходим
        case BACKL_PULS:
        case BACKL_PULS_COLOR: { //дыхание подсветки
            _timer_ms[TMR_BACKL] = backl.mode_2_time; //установили таймер
            if (backl.drive) { //если светодиоды в режиме разгорания
              if (wsBacklIncLedBright(backl.mode_2_step, backl.maxBright)) backl.drive = 0; //прибавили шаг яркости
            }
            else { //иначе светодиоды в режиме затухания
              if (wsBacklDecLedBright(backl.mode_2_step, backl.minBright)) { //уменьшаем яркость
                backl.drive = 1;
                if (fastSettings.backlMode == BACKL_PULS_COLOR) backl.color += BACKL_MODE_3_COLOR; //меняем цвет
                else backl.color = fastSettings.backlColor; //иначе статичный цвет
                wsBacklSetLedHue(backl.color, WHITE_ON); //установили цвет
                _timer_ms[TMR_BACKL] = BACKL_MODE_2_PAUSE; //установили таймер
              }
            }
          }
          break;
        case BACKL_RUNNING_FIRE:
        case BACKL_RUNNING_FIRE_COLOR:
        case BACKL_RUNNING_FIRE_RAINBOW:
        case BACKL_RUNNING_FIRE_CONFETTI: { //бегущий огонь
            _timer_ms[TMR_BACKL] = BACKL_MODE_4_TIME / LEDS_NUM / BACKL_MODE_4_FADING; //установили таймер
            if (backl.steps) { //если есть шаги затухания
              wsBacklDecLedsBright(backl.position - 1, backl.mode_4_step); //уменьшаем яркость
              backl.steps--; //уменьшаем шаги затухания
            }
            else { //иначе двигаем голову
              if (backl.drive) { //если направление вправо
                if (backl.position > 0) backl.position--; else backl.drive = 0; //едем влево
              }
              else { //иначе напрвление влево
                if (backl.position < (LEDS_NUM + 1)) backl.position++; else backl.drive = 1; //едем вправо
              }
              wsBacklSetLedBright(backl.position - 1, backl.maxBright); //установили яркость
              backl.steps = BACKL_MODE_4_FADING; //установили шаги затухания
            }
            if (fastSettings.backlMode == BACKL_RUNNING_FIRE) {
              backl.color = fastSettings.backlColor; //статичный цвет
              wsBacklSetLedHue(backl.color, WHITE_ON); //установили цвет
            }
          }
          break;
        case BACKL_WAVE:
        case BACKL_WAVE_COLOR:
        case BACKL_WAVE_RAINBOW:
        case BACKL_WAVE_CONFETTI: { //волна
            _timer_ms[TMR_BACKL] = backl.mode_8_time; //установили таймер
            switch (backl.steps) { //в зависимости от текущего шага анимации
              case 0:
              case 2:
                if (wsBacklIncLedBright(backl.position, backl.mode_8_step, backl.maxBright)) { //прибавили шаг яркости
                  backl.drive = 1; //установили флаг завершения анимации
                }
                break;
              default:
                if (wsBacklDecLedBright(backl.position, backl.mode_8_step, backl.minBright)) { //убавили шаг яркости
                  backl.drive = 1; //установили флаг завершения анимации
                }
                break;
            }
            if (backl.drive) { //если анимация завершена
              backl.drive = 0; //сбросили флаг завершения анимации
              switch (backl.steps) {
                case 0:
                case 1:
                  if (backl.position < (LEDS_NUM - 1)) backl.position++; //сменили позицию
                  else { //иначе меняем режим анимации
                    if (backl.steps < 1) { //если был режим разгорания
                      backl.steps = 1; //перешли в затухание
                      backl.position = 0; //сбросили позицию
                    }
                    else { //иначе режим затухания
                      backl.steps = 2; //перешли в разгорание
                      backl.position = (LEDS_NUM - 1); //сбросили позицию
                    }
                  }
                  break;
                default:
                  if (backl.position > 0) backl.position--; //сменили позицию
                  else { //иначе меняем режим анимации
                    if (backl.steps < 3) { //если был режим разгорания
                      backl.steps = 3; //перешли в затухание
                      backl.position = (LEDS_NUM - 1); //сбросили позицию
                    }
                    else { //иначе режим затухания
                      backl.steps = 0; //перешли в разгорание
                      backl.position = 0; //сбросили позицию
                    }
                  }
                  break;
              }
            }
            if (fastSettings.backlMode == BACKL_WAVE) { //если режим статичного цвета
              backl.color = fastSettings.backlColor; //статичный цвет
              wsBacklSetLedHue(backl.color, WHITE_ON); //установили цвет
            }
          }
          break;
      }
    }
    if (!_timer_ms[TMR_COLOR]) { //если время пришло
      switch (fastSettings.backlMode) {
        case BACKL_RUNNING_FIRE_RAINBOW:
        case BACKL_WAVE_RAINBOW:
        case BACKL_RAINBOW: { //радуга
            _timer_ms[TMR_COLOR] = BACKL_MODE_13_TIME; //установили таймер
            backl.color += BACKL_MODE_13_STEP; //прибавили шаг
            for (uint8_t f = 0; f < LEDS_NUM; f++) wsBacklSetLedHue(f, backl.color + (f * BACKL_MODE_13_STEP), WHITE_OFF); //установили цвет
          }
          break;
        case BACKL_RUNNING_FIRE_CONFETTI:
        case BACKL_WAVE_CONFETTI:
        case BACKL_CONFETTI: { //рандомный цвет
            _timer_ms[TMR_COLOR] = BACKL_MODE_14_TIME; //установили таймер
            wsBacklSetLedHue(random(0, LEDS_NUM), random(0, 256), WHITE_ON); //установили цвет
          }
          break;
        case BACKL_RUNNING_FIRE_COLOR:
        case BACKL_WAVE_COLOR:
        case BACKL_SMOOTH_COLOR_CHANGE: { //плавная смена цвета
            _timer_ms[TMR_COLOR] = BACKL_MODE_12_TIME; //установили таймер
            backl.color += BACKL_MODE_12_COLOR;
            wsBacklSetLedHue(backl.color, WHITE_OFF); //установили цвет
          }
          break;
      }
    }
  }
#elif BACKL_TYPE != 3
  if (backl.maxBright && fastSettings.backlMode == BACKL_PULS) {
    if (!_timer_ms[TMR_BACKL]) {
      _timer_ms[TMR_BACKL] = backl.mode_2_time;
      if (backl.drive) {
        if (ledBacklDecBright(backl.mode_2_step, backl.minBright)) {
          _timer_ms[TMR_BACKL] = BACKL_MODE_2_PAUSE;
          backl.drive = 0;
        }
      }
      else if (ledBacklIncBright(backl.mode_2_step, backl.maxBright)) backl.drive = 1;
    }
  }
#endif
}
//-------------------------------Анимации точек------------------------------------
void dotEffect(void) //анимации точек
{
  if (mainTask == MAIN_PROGRAM) { //если основная программа
    if (!dot.update && !_timer_ms[TMR_DOT]) { //если пришло время
      if (!dot.maxBright || (animShow >= ANIM_MAIN)) { //если яркость выключена или запущена сторонняя анимация
        dot.update = 1; //сбросили флаг секунд
        return; //выходим
      }

      switch (dotGetMode()) { //режим точек
        case DOT_MAIN_BLINK:
          if (!dot.drive) dotSetBright(dot.maxBright); //включаем точки
          else dotSetBright(0); //выключаем точки
          dot.drive = !dot.drive; //сменили направление
          dot.update = 1; //сбросили флаг обновления точек
          break;
        case DOT_MAIN_DOOBLE_BLINK:
          if (dot.count & 0x01) dotSetBright(0); //выключаем точки
          else dotSetBright(dot.maxBright); //включаем точки

          if (++dot.count > 3) { //сместили шаг
            dot.count = 0;  //сбрасываем счетчик
            dot.update = 1; //сбросили флаг обновления точек
          }
          else _timer_ms[TMR_DOT] = DOT_MAIN_DOOBLE_TIME; //установили таймер
          break;
#if (SECS_DOT != 3) && (SECS_DOT != 4)
        case DOT_MAIN_PULS:
          if (!dot.drive) {
            if (dotIncBright(dot.brightStep, dot.maxBright)) dot.drive = 1; //сменили направление
          }
          else {
            if (dotDecBright(dot.brightStep, 0)) {
              dot.drive = 0; //сменили направление
              dot.update = 1; //сбросили флаг обновления точек
              return; //выходим
            }
          }
          _timer_ms[TMR_DOT] = dot.brightTime; //установили таймер
          break;
#endif
#if SECS_DOT == 2
        case DOT_MAIN_TURN_BLINK:
          neonDotSetBright(dot.maxBright); //установка яркости неоновых точек
          if (!dot.drive) neonDotSet(DOT_LEFT); //установка неоновой точки
          else neonDotSet(DOT_RIGHT); //установка неоновой точки
          dot.drive = !dot.drive; //сменили направление
          dot.update = 1; //сбросили флаг обновления точек
          break;
        case DOT_MAIN_TURN_PULS:
          switch (dot.count) {
            case 0: if (dotIncBright(dot.brightTurnStep, dot.maxBright, DOT_LEFT)) dot.count = 1; break; //сменили направление
            case 1: if (dotDecBright(dot.brightTurnStep, 0, DOT_LEFT)) dot.count = 2; break; //сменили направление
            case 2: if (dotIncBright(dot.brightTurnStep, dot.maxBright, DOT_RIGHT)) dot.count = 3; break; //сменили направление
            default:
              if (dotDecBright(dot.brightTurnStep, 0, DOT_RIGHT)) {
                dot.count = 0; //сменили направление
                dot.update = 1; //сбросили флаг обновления точек
                return; //выходим
              }
              break;
          }
          _timer_ms[TMR_DOT] = dot.brightTurnTime; //установили таймер
          break;
#endif
#if SECS_DOT == 4
        case DOT_DECATRON_METR:
          if (RTC.s & 0x01) decatronSetLine(29, 1); //установка линии декатрона
          else decatronSetLine(14, 16); //установка линии декатрона
          dot.update = 1; //сбросили флаг обновления точек
          break;
        case DOT_DECATRON_STEPS:
          dot.count = RTC.s >> 1;
          dot.steps = dot.count + (RTC.s & 0x01);
          if (dot.steps == 30) dot.steps = 0;
          decatronSetLine(dot.count, dot.steps); //установка линии декатрона
          dot.update = 1; //сбросили флаг обновления точек
          break;
        case DOT_DECATRON_TIMER:
          if (!RTC.s) decatronDisable(); //отключение декатрона
          else if (RTC.s <= 30) decatronSetLine(0, RTC.s - 1); //установка линии декатрона
          else decatronSetLine(RTC.s - 30, 0); //установка линии декатрона
          dot.update = 1; //сбросили флаг обновления точек
          break;
        case DOT_DECATRON_SWAY: {
            static const uint8_t _time[] = {100, 75, 50, 25, 25, 25, 30, 40, 60, 70, 100};
            if (RTC.s & 0x01) decatronSetLine(19 - dot.count, 22 - dot.count); //установка линии декатрона
            else decatronSetLine(dot.count + 8, dot.count + 11); //установка линии декатрона
            if (dot.count < 11) { //если анимация не завершена
              _timer_ms[TMR_DOT] = _time[dot.count]; //установили таймер
              dot.count++; //сместили шаг
            }
            else {
              dot.count = 0; //сбросили шаги
              dot.update = 1; //иначе сбросили флаг обновления точек
            }
          }
          break;
#endif
#if DOTS_PORT_ENABLE
        case DOT_BLINK:
          if (!dot.drive) {
            indiSetDotsMain(DOT_ALL); //установка разделительных точек
            dot.drive = 1; //сменили направление
#if DOT_BLINK_TIME
            _timer_ms[TMR_DOT] = DOT_BLINK_TIME; //установили таймер
#else
            dot.update = 1; //сбросили флаг секунд
#endif
          }
          else {
            indiClrDots(); //очистка разделительных точек
            dot.drive = 0; //сменили направление
            dot.update = 1; //сбросили флаг секунд
          }
          break;
        case DOT_RUNNING: //бегущая
          indiClrDots(); //очистка разделителных точек
          indiSetDots(dot.count, 1); //установка разделительных точек
          if (dot.drive) {
            if (dot.count > 0) dot.count--; //сместили точку
            else {
              dot.drive = 0; //сменили направление
              dot.update = 1; //сбросили флаг обновления точек
              return; //выходим
            }
          }
          else {
            if (dot.count < (DOTS_ALL - 1)) dot.count++; //сместили точку
            else {
              dot.drive = 1; //сменили направление
              dot.update = 1; //сбросили флаг обновления точек
              return; //выходим
            }
          }
          _timer_ms[TMR_DOT] = (DOT_RUNNING_TIME / DOTS_ALL); //установили таймер
          break;
        case DOT_SNAKE: //змейка
          indiClrDots(); //очистка разделителных точек
          indiSetDots(dot.count - (DOTS_ALL - 1), DOTS_ALL); //установка разделительных точек
          if (dot.drive) {
            if (dot.count > 0) dot.count--; //убавили шаг
            else {
              dot.drive = 0; //сменили направление
              dot.update = 1; //сбросили флаг обновления точек
              return; //выходим
            }
          }
          else {
            if (dot.count < ((DOTS_ALL * 2) - 2)) dot.count++; //прибавили шаг
            else {
              dot.drive = 1; //сменили направление
              dot.update = 1; //сбросили флаг обновления точек
              return; //выходим
            }
          }
          _timer_ms[TMR_DOT] = (DOT_SNAKE_TIME / (DOTS_ALL * 2)); //установили таймер
          break;
        case DOT_RUBBER_BAND: //резинка
          indiClrDots(); //очистка разделителных точек
          if (dot.drive) { //если режим убывания
            if (dot.steps < (DOTS_ALL - 1)) dot.steps++; //сместили точку
            else {
              if (dot.count) { //если еще есть точки
                dot.count--; //убавили шаг
                dot.steps = dot.count; //установили точку
              }
              else {
                dot.steps = 0; //сбросили точку
                dot.drive = 0; //сменили направление
                dot.update = 1; //сбросили флаг обновления точек
                return; //выходим
              }
            }
            indiSetDots(dot.steps, 1); //установка разделительных точек
            indiSetDots(0, dot.count); //установка разделительных точек
          }
          else { //иначе режим заполнения
            indiSetDots(dot.steps, 1); //установка разделительных точек
            indiSetDots(DOTS_ALL - dot.count, dot.count); //установка разделительных точек
            if (dot.steps < ((DOTS_ALL - 1) - dot.count)) dot.steps++; //сместили точку
            else {
              if (dot.count < (DOTS_ALL - 1)) { //если еще есть точки
                dot.count++; //прибавили шаг
                dot.steps = 0; //сбросили точку
              }
              else {
                dot.steps = dot.count; //установили точку
                dot.drive = 1; //сменили направление
                dot.update = 1; //сбросили флаг обновления точек
                return; //выходим
              }
            }
          }
          _timer_ms[TMR_DOT] = (DOT_RUBBER_BAND_TIME / (DOTS_ALL * ((DOTS_ALL / 2) + 0.5))); //установили таймер
          break;
#if (DOTS_NUM > 4) || (DOTS_TYPE == 2)
        case DOT_TURN_BLINK: //мигание одной точкой по очереди
#if DOT_TURN_TIME
          if (dot.count < ((1000 / DOT_TURN_TIME) - 1)) {
            dot.count++; //прибавили шаг
            _timer_ms[TMR_DOT] = DOT_TURN_TIME; //установили таймер
          }
          else {
            dot.count = 0; //сбросили счетчик
            dot.update = 1; //сбросили флаг секунд
          }
#else
          dot.update = 1; //сбросили флаг секунд
#endif
          indiClrDots(); //очистка разделительных точек
          switch (dot.drive) {
            case 0: indiSetDotsMain(DOT_LEFT); dot.drive = 1; break; //включаем левую точку
            case 1: indiSetDotsMain(DOT_RIGHT); dot.drive = 0; break; //включаем правую точку
          }
          break;
#endif
#if (DOTS_NUM > 4) && (DOTS_TYPE == 2)
        case DOT_DUAL_TURN_BLINK: //мигание двумя точками по очереди
#if DOT_DUAL_TURN_TIME
          if (dot.count < ((1000 / DOT_DUAL_TURN_TIME) - 1)) {
            dot.count++; //прибавили шаг
            _timer_ms[TMR_DOT] = DOT_DUAL_TURN_TIME; //установили таймер
          }
          else {
            dot.count = 0; //сбросили счетчик
            dot.update = 1; //сбросили флаг секунд
          }
#else
          dot.update = 1; //сбросили флаг секунд
#endif
          switch (dot.drive) {
            case 0: indiSetDotL(2); indiSetDotR(3); indiClrDotR(1); indiClrDotL(4); dot.drive = 1; break; //включаем левую точку
            case 1: indiSetDotR(1); indiSetDotL(4); indiClrDotL(2); indiClrDotR(3); dot.drive = 0; break; //включаем правую точку
          }
          break;
#endif
#endif
        default: dot.update = 1; break; //сбросили флаг обновления точек
      }
    }
  }
}
//--------------------------------Мигание точек------------------------------------
void dotFlash(void) //мигание точек
{
#if (SECS_DOT != 3) && (SECS_DOT != 4) && DOTS_PORT_ENABLE
  static boolean state; //текущее состояние точек

  if (mainTask == MAIN_PROGRAM) { //если основная программа
    if (!dot.maxBright || (animShow >= ANIM_MAIN)) { //если яркость выключена или запущена сторонняя анимация
      return; //выходим
    }

    if (dotGetMode() >= DOT_BLINK) {
      switch (fastSettings.neonDotMode) { //режим точек
        case DOT_EXT_OFF: dotSetBright(0); break; //точки выключены
        case DOT_EXT_STATIC: dotSetBright(dot.maxBright); break; //точки включены
        case DOT_EXT_BLINK:
          if (!state) dotSetBright(dot.maxBright); //включаем точки
          else dotSetBright(0); //выключаем точки
          state = !state; //сменили направление
          break;
        case DOT_EXT_TURN_BLINK:
          neonDotSetBright(dot.maxBright); //установка яркости неоновых точек
          if (!state) neonDotSet(DOT_LEFT); //установка неоновой точки
          else neonDotSet(DOT_RIGHT); //установка неоновой точки
          state = !state; //сменили направление
          break;
      }
    }
  }
#endif
}
//---------------------------Получить анимацию точек-------------------------------
uint8_t dotGetMode(void) //получить анимацию точек
{
#if ALARM_TYPE
  switch (alarms.now) {
    case ALARM_ENABLE: if (extendedSettings.alarmDotOn != DOT_EFFECT_NUM) return extendedSettings.alarmDotOn; break;
    case ALARM_WAIT: if (extendedSettings.alarmDotWait != DOT_EFFECT_NUM) return extendedSettings.alarmDotWait; break;
  }
#endif
  return fastSettings.dotMode;
}
//-----------------------------Установить разделяющую точку-----------------------------------
void setDotTemp(boolean set) {
#if DOTS_PORT_ENABLE && !SHOW_TEMP_DOT_DIV
  if (!set) indiClrDots(); //выключаем разделительные точки
  else {
#if (DOTS_TYPE == 1) || ((DOTS_DIV == 1) && (DOTS_TYPE == 2))
    indiSetDotR(1); //включаем разделительную точку
#else
    indiSetDotL(2); //включаем разделительную точку
#endif
  }
#elif SECS_DOT == 2
  if (!set) neonDotSet(DOT_NULL); //выключаем разделительной точки
  else {
    neonDotSetBright(dot.menuBright); //установка яркости неоновых точек
    neonDotSet(DOT_LEFT); //включаем неоновую точку
  }
#elif SECS_DOT == 4
  if (!set) decatronDisable(); //отключение декатрона
  else decatronSetDot(15); //установка позиции декатрона
#else
  if (!set) dotSetBright(0); //выключаем точки
  else dotSetBright(dot.menuBright); //включаем точки
#endif
}
//-----------------------------Установить разделяющую точку-----------------------------------
void setDotDate(boolean set) {
#if DOTS_PORT_ENABLE && !SHOW_DATE_DOT_DIV
#if SHOW_DATE_TYPE > 1
#if DOTS_NUM > 4
  if (!set) indiClrDots(); //выключаем разделительные точки
  else {
#if (DOTS_TYPE == 1) || ((DOTS_DIV == 1) && (DOTS_TYPE == 2))
    indiSetDotR(1); //включаем разделительную точку
    indiSetDotR(3); //включаем разделительную точку
#else
    indiSetDotL(2); //включаем разделительную точку
    indiSetDotL(4); //включаем разделительную точку
#endif
  }
#elif SECS_DOT != 3
  dotSetBright(dot.menuBright); //включаем точки
#endif
#else
  if (!set) indiClrDots(); //выключаем разделительные точки
  else {
#if (DOTS_TYPE == 1) || ((DOTS_DIV == 1) && (DOTS_TYPE == 2))
    indiSetDotR(1); //включаем разделительную точку
#else
    indiSetDotL(2); //включаем разделительную точку
#endif
  }
#endif
#elif SECS_DOT == 2
#if (SHOW_DATE_TYPE > 1) && (LAMP_NUM > 4)
  if (!set) dotSetBright(0); //выключаем точки
  else dotSetBright(dot.menuBright); //включаем точки
#else
  if (!set) neonDotSet(DOT_NULL); //выключаем разделительной точки
  else {
    neonDotSetBright(dot.menuBright); //установка яркости неоновых точек
    neonDotSet(DOT_LEFT); //установка разделительной точки
  }
#endif
#elif SECS_DOT == 4
  if (!set) decatronDisable(); //отключение декатрона
  else decatronSetDot(0); //установка позиции декатрона
#else
  if (!set) dotSetBright(0); //выключаем точки
  else dotSetBright(dot.menuBright); //включаем точки
#endif
}
//-----------------------------Сброс анимации точек--------------------------------
void dotReset(uint8_t state) //сброс анимации точек
{
  static uint8_t mode; //предыдущий режим точек

  if ((state != ANIM_RESET_CHANGE) || (mode != dotGetMode())) { //если нужно сбросить анимацию точек
#if DOTS_PORT_ENABLE
    indiClrDots(); //выключаем разделительные точки
#endif
#if (SECS_DOT != 3) || !DOTS_PORT_ENABLE
    dotSetBright(0); //выключаем секундные точки
#endif
    _timer_ms[TMR_DOT] = 0; //сбросили таймер
    if (dotGetMode() > DOT_STATIC) { //если анимация точек включена
      dot.update = 1; //установли флаг обновления точек
      dot.drive = 0; //сбросили флаг направления яркости точек
      dot.count = 0; //сбросили счетчик вспышек точек
#if DOTS_PORT_ENABLE
      dot.steps = 0; //сбросили шаги точек
#endif
    }
  }
  mode = dotGetMode(); //запоминаем анимацию точек
}
//----------------------------Сброс анимации секунд--------------------------------
void secsReset(void) //сброс анимации секунд
{
#if LAMP_NUM > 4
  anim.flipSeconds = 0; //сбрасываем флаг анимации секунд
#endif
  indi.update = 0; //устанавливаем флаг обновления индикаторов
}
//------------------------------Сброс режима сна------------------------------------
void sleepReset(void) //сброс режима сна
{
  _timer_sec[TMR_SLEEP] = mainSettings.timeSleep[indi.sleepMode - SLEEP_NIGHT]; //установли время ожидания режима сна
  if (indi.sleepMode == SLEEP_DAY) _timer_sec[TMR_SLEEP] *= 60; //если режим сна день
}
//----------------------------Режим сна индикаторов---------------------------------
uint8_t sleepIndi(void) //режим сна индикаторов
{
  indiClr(); //очистка индикаторов
  backlAnimDisable(); //запретили эффекты подсветки
  changeBrightDisable(CHANGE_DISABLE); //запретить смену яркости
#if BACKL_TYPE == 3
  wsBacklClearLeds(); //выключили светодиоды
#elif BACKL_TYPE
  ledBacklSetBright(0); //выключили светодиоды
#endif
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
#if RADIO_SLEEP_ENABLE == 1
  if (indi.sleepMode != SLEEP_DAY) radioShutdown(); //выключить радиоприемник
#elif !RADIO_SLEEP_ENABLE
  radioShutdown(); //выключить радиоприемник
#endif
#endif
  while (!buttonState()) { //если не нажата кнопка
    dataUpdate(); //обработка данных

#if ESP_ENABLE
    if (busCheck() & ~(0x01 << BUS_COMMAND_WAIT)) return MAIN_PROGRAM; //выходим
#endif

    if (!indi.update) { //если пришло время обновить индикаторы
      indi.update = 1; //сбрасываем флаг

#if ALARM_TYPE
      if (alarms.now == ALARM_WARN) return ALARM_PROGRAM; //тревога будильника
#endif
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
      if ((timer.mode == 2) && !timer.count) return WARN_PROGRAM; //тревога таймера
#endif
      if (!indi.sleepMode) return MAIN_PROGRAM; //выход в режим часов
    }
  }
  return MAIN_PROGRAM; //выход в режим часов
}
//------------------------------------Имитация глюков------------------------------------
void glitchIndi(void) //имитация глюков
{
  if (mainSettings.glitchMode) { //если глюки включены
    if (!_timer_sec[TMR_GLITCH] && (RTC.s >= GLITCH_PHASE_MIN) && (RTC.s < GLITCH_PHASE_MAX)) { //если пришло время
      uint8_t indiSave = 0; //текущая цифра в индикаторе
      uint8_t glitchCounter = random(GLITCH_NUM_MIN, GLITCH_NUM_MAX); //максимальное количество глюков
      uint8_t glitchIndic = random(0, LAMP_NUM); //номер индикатора
      _timer_ms[TMR_ANIM] = 0; //сбрасываем таймер
      while (!buttonState()) { //если не нажата кнопка
        systemTask(); //обработка данных
#if LAMP_NUM > 4
        flipSecs(); //анимация секунд
#endif
        if (!_timer_ms[TMR_ANIM]) { //если таймер истек
          if (indiGet(glitchIndic) != INDI_NULL) { //если индикатр включен
            indiSave = indiGet(glitchIndic); //сохраняем текущую цифру в индикаторе
            indiClr(glitchIndic); //выключаем индикатор
          }
          else indiSet(indiSave, glitchIndic); //включаем индикатор
          _timer_ms[TMR_ANIM] = random(1, 6) * GLITCH_TIME; //перезапускаем таймер глюка
          if (!glitchCounter--) break; //выходим если закончились глюки
        }
      }
      _timer_sec[TMR_GLITCH] = random(GLITCH_MIN_TIME, GLITCH_MAX_TIME); //находим рандомное время появления глюка
      indiSet(indiSave, glitchIndic); //восстанавливаем состояние индикатора
    }
  }
}
//----------------------------Антиотравление индикаторов-------------------------------
void burnIndi(uint8_t mode, boolean demo) //антиотравление индикаторов
{
  uint8_t indi = 0; //номер индикатора

  if (mode != BURN_SINGLE_TIME) { //если режим без отображения времени
#if DOTS_PORT_ENABLE
#if BURN_DOTS
    if (!demo) indiSetDots(0, DOTS_ALL); //установка разделительных точек
#else
    indiClrDots(); //выключаем разделительные точки
#endif
#endif
#if ((SECS_DOT != 3) || !DOTS_PORT_ENABLE) && (SECS_DOT != 4)
    dotSetBright(0); //выключаем секундные точки
#endif
  }

  _timer_ms[TMR_MS] = 0; //сбрасываем таймер

  while (1) {
    if (mode == BURN_SINGLE) indiClr(); //очистка индикаторов
    for (uint8_t loops = (demo) ? 1 : BURN_LOOPS; loops; loops--) {
      if (mode == BURN_SINGLE_TIME) {
        indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
        indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
#if LAMP_NUM > 4
        indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
#endif
      }
      for (uint8_t digit = 0; digit < 10; digit++) {
        switch (mode) {
          case BURN_ALL:
            for (indi = 0; indi < LAMP_NUM; indi++) indiPrintNum(cathodeMask[digit], indi); //отрисовываем цифру
            break;
          case BURN_SINGLE:
          case BURN_SINGLE_TIME:
            indiPrintNum(cathodeMask[digit], indi); //отрисовываем цифру
            break;
        }
        for (_timer_ms[TMR_MS] = BURN_TIME; _timer_ms[TMR_MS] && !buttonState();) dataUpdate(); //ждем
      }
    }
    if (mode == BURN_ALL || (++indi >= LAMP_NUM)) return;
  }
}
//-------------------------------Анимация секунд-----------------------------------
#if LAMP_NUM > 4
void flipSecs(void) //анимация секунд
{
  switch (fastSettings.secsMode) {
    case SECS_BRIGHT: //плавное угасание и появление
      if (animShow == ANIM_SECS) { //если сменились секунды
        animShow = ANIM_NULL; //сбрасываем флаг анимации цифр
        _timer_ms[TMR_MS] = 0; //сбрасываем таймер
        anim.timeBright = SECONDS_ANIM_1_TIME / indi.maxBright; //расчёт шага яркости режима 2
        anim.flipSeconds = (RTC.s) ? (RTC.s - 1) : 59; //предыдущая секунда
        anim.flipBuffer[0] = 0; //сбросили флаг
        anim.flipBuffer[1] = indi.maxBright; //установили максимальную яркость
        anim.flipBuffer[2] = anim.flipSeconds % 10; //старые секунды
        anim.flipBuffer[3] = anim.flipSeconds / 10; //старые секунды
        anim.flipSeconds = 0; //сбросили разряды анимации
        if (anim.flipBuffer[2] != (RTC.s % 10)) anim.flipSeconds = 5;
        if (anim.flipBuffer[3] != (RTC.s / 10)) anim.flipSeconds = 4;
      }
      if (anim.flipSeconds && !_timer_ms[TMR_MS]) { //если анимация активна и пришло время
        _timer_ms[TMR_MS] = anim.timeBright; //установили таймер
        if (!anim.flipBuffer[0]) { //если режим уменьшения яркости
          if (anim.flipBuffer[1]) {
            anim.flipBuffer[1]--;
            indiSetBright(anim.flipBuffer[1], anim.flipSeconds, 6); //уменьшение яркости
          }
          else {
            anim.flipBuffer[0] = 1; //перешли к разгоранию
            indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
          }
        }
        else { //иначе режим увеличения яркости
          if (anim.flipBuffer[1] < indi.maxBright) {
            anim.flipBuffer[1]++;
            indiSetBright(anim.flipBuffer[1], anim.flipSeconds, 6); //увеличение яркости
          }
          else anim.flipSeconds = 0; //сбросили разряды анимации
        }
      }
      break;
    case SECS_ORDER_OF_NUMBERS: //перемотка по порядку числа
    case SECS_ORDER_OF_CATHODES: //перемотка по порядку катодов в лампе
      if (animShow == ANIM_SECS) { //если сменились секунды
        animShow = ANIM_NULL; //сбрасываем флаг анимации цифр
        _timer_ms[TMR_MS] = 0; //сбрасываем таймер
        anim.flipSeconds = (RTC.s) ? (RTC.s - 1) : 59; //предыдущая секунда
        anim.flipBuffer[0] = anim.flipSeconds % 10; //старые секунды
        anim.flipBuffer[1] = anim.flipSeconds / 10; //старые секунды
        anim.flipBuffer[2] = RTC.s % 10; //новые секунды
        anim.flipBuffer[3] = RTC.s / 10; //новые секунды
        anim.flipSeconds = 0x03; //устанавливаем флаги анимации

        if (fastSettings.secsMode == SECS_ORDER_OF_CATHODES) {
          for (uint8_t i = 0; i < 4; i++) {
            for (uint8_t c = 0; c < 10; c++) {
              if (cathodeMask[c] == anim.flipBuffer[i]) {
                anim.flipBuffer[i] = c;
                break;
              }
            }
          }
        }
      }
      if (anim.flipSeconds && !_timer_ms[TMR_MS]) { //если анимация активна и пришло время
        _timer_ms[TMR_MS] = (fastSettings.secsMode == SECS_ORDER_OF_NUMBERS) ? SECONDS_ANIM_2_TIME : SECONDS_ANIM_3_TIME; //установили таймер
        for (uint8_t i = 0; i < 2; i++) { //перебираем все цифры
          if (anim.flipBuffer[i] != anim.flipBuffer[i + 2]) { //если не достигли конца анимации разряда
            if (--anim.flipBuffer[i] > 9) anim.flipBuffer[i] = 9; //меняем цифру разряда
          }
          else anim.flipSeconds &= ~(0x01 << i); //иначе завершаем анимацию для разряда
          indiPrintNum((fastSettings.secsMode == SECS_ORDER_OF_NUMBERS) ? anim.flipBuffer[i] : cathodeMask[anim.flipBuffer[i]], (LAMP_NUM - 1) - i); //вывод секунд
        }
      }
      break;
    default:
      if (animShow == ANIM_SECS) { //если сменились секунды
        animShow = ANIM_NULL; //сбрасываем флаг анимации цифр
        indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
      }
      break;
  }
}
#endif
//----------------------Обновить буфер анимации текущего времени--------------------
void animUpdateTime(void) //обновить буфер анимации текущего времени
{
  animPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
  animPrintNum(RTC.m, 2, 2, 0); //вывод минут
#if LAMP_NUM > 4
  animPrintNum(RTC.s, 4, 2, 0); //вывод секунд
#endif
}
//----------------------------------Анимация цифр-----------------------------------
void animIndi(uint8_t mode, uint8_t type) //анимация цифр
{
  switch (mode) {
    case 0: if (type == FLIP_NORMAL) animPrintBuff(0, 6, LAMP_NUM); animShow = ANIM_NULL; return; //без анимации
    case 1: if (type == FLIP_DEMO) return; else mode = pgm_read_byte(&_anim_set[random(0, sizeof(_anim_set))]); break; //случайный режим
  }

  mode -= 2; //установили режим
  flipIndi(mode, type); //перешли в отрисовку анимации
  if (mode == FLIP_BRIGHT) indiSetBright(indi.maxBright); //возвращаем максимальную яркость

  animPrintBuff(0, 6, LAMP_NUM); //отрисовали буфер
  animShow = ANIM_NULL; //сбрасываем флаг анимации
}
//----------------------------------Анимация цифр-----------------------------------
void flipIndi(uint8_t mode, uint8_t type) //анимация цифр
{
  uint8_t changeBuffer[6]; //буфер анимаций
  uint8_t changeIndi = 0; //флаги разрядов
  uint8_t changeCnt = 0; //счетчик шагов
  uint8_t changeNum = 0; //счетчик разрядов

  if (type != FLIP_NORMAL) animUpdateTime(); //обновили буфер анимации текущего времени
  if (type == FLIP_DEMO) { //если режим демонстрации
    indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
    indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
#if LAMP_NUM > 4
    indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
#endif
  }

  for (uint8_t i = 0; i < LAMP_NUM; i++) anim.flipBuffer[i] = indiGet(i);

  switch (mode) {
    case FLIP_BRIGHT:
    case FLIP_TRAIN:
    case FLIP_GATES:
      if (mode != FLIP_BRIGHT) changeIndi = (mode != FLIP_TRAIN) ? 1 : FLIP_MODE_5_STEP; //перешли к второму этапу
      else anim.timeBright = FLIP_MODE_2_TIME / indi.maxBright; //расчёт шага яркости
      for (uint8_t i = 0; i < LAMP_NUM; i++) {
        if (anim.flipBuffer[i] != INDI_NULL) {
          changeCnt = indi.maxBright; //установили максимальную яркость
          changeIndi = 0; //перешли к второму этапу
          break; //продолжаем
        }
      }
      break;
    case FLIP_ORDER_OF_NUMBERS:
    case FLIP_ORDER_OF_CATHODES:
    case FLIP_SLOT_MACHINE:
      for (uint8_t i = 0; i < LAMP_NUM; i++) {
        anim.flipBuffer[i] = indiDecodeNum(anim.flipBuffer[i]);
        changeBuffer[i] = indiDecodeNum(anim.flipBuffer[i + 6]);
        if (type == FLIP_DEMO) anim.flipBuffer[i]--;
        else if ((anim.flipBuffer[i] != changeBuffer[i]) || (mode == FLIP_SLOT_MACHINE)) {
          if (changeBuffer[i] == 10) changeBuffer[i] = (anim.flipBuffer[i]) ? (anim.flipBuffer[i] - 1) : 9;
          if (anim.flipBuffer[i] == 10) anim.flipBuffer[i] = (changeBuffer[i]) ? (changeBuffer[i] - 1) : 9;
        }
        if (mode == FLIP_ORDER_OF_CATHODES) {
          for (uint8_t i = 0; i < LAMP_NUM; i++) {
            for (uint8_t c = 0; c < 10; c++) {
              if (cathodeMask[c] == anim.flipBuffer[i]) {
                anim.flipBuffer[i] = c;
                break;
              }
            }
            for (uint8_t c = 0; c < 10; c++) {
              if (cathodeMask[c] == changeBuffer[i]) {
                changeBuffer[i] = c;
                break;
              }
            }
          }
        }
      }
      break;
  }

  _timer_ms[TMR_MS] = FLIP_TIMEOUT; //устанавливаем таймер
  _timer_ms[TMR_ANIM] = 0; //сбрасываем таймер

  while (!buttonState()) {
    dataUpdate(); //обработка данных

#if ESP_ENABLE
    if (busCheck() & ~(0x01 << BUS_COMMAND_WAIT)) return; //обновление шины
#endif

    if (type != FLIP_NORMAL) { //если анимация времени
      if (!indi.update) { //если пришло время обновить индикаторы
        indi.update = 1; //сбрасываем флаг
        animUpdateTime(); //обновляем буфер анимации текущего времени
        switch (mode) { //режим анимации перелистывания
          case FLIP_RUBBER_BAND: if (changeCnt) animPrintBuff(LAMP_NUM - changeNum, (LAMP_NUM + 6) - changeNum, changeNum); break; //вывод часов
          case FLIP_HIGHLIGHTS:
          case FLIP_EVAPORATION:
            if (changeCnt || (mode == FLIP_HIGHLIGHTS)) for (uint8_t f = 0; f < changeNum; f++) indiSet(anim.flipBuffer[6 + changeBuffer[f]], changeBuffer[f]); //вывод часов
            break;
          case FLIP_SLOT_MACHINE:
            animPrintBuff(0, 6, changeNum); //вывод часов
            for (uint8_t f = changeNum; f < LAMP_NUM; f++) changeBuffer[f] = indiDecodeNum(anim.flipBuffer[f + 6]);
            break;
        }
      }
    }

    if (!_timer_ms[TMR_MS]) break; //выходим если тайм-аут
    if (!_timer_ms[TMR_ANIM]) { //если таймер истек
      switch (mode) { //режим анимации перелистывания
        case FLIP_BRIGHT: { //плавное угасание и появление
            if (!changeIndi) { //если режим уменьшения яркости
              if (changeCnt) {
                changeCnt--;
                if (type != FLIP_DEMO) animBright(changeCnt);
                else indiSetBright(changeCnt); //уменьшение яркости
              }
              else {
                animPrintBuff(0, 6, LAMP_NUM); //вывод буфера
                changeIndi = 1; //перешли к разгоранию
              }
            }
            else { //иначе режим увеличения яркости
              if (changeCnt < indi.maxBright) {
                changeCnt++;
                if (type != FLIP_DEMO) animBright(changeCnt);
                else indiSetBright(changeCnt); //увеличение яркости
              }
              else return; //выходим
            }
            _timer_ms[TMR_ANIM] = anim.timeBright; //устанавливаем таймер
          }
          break;
        case FLIP_ORDER_OF_NUMBERS:  //перемотка по порядку числа
        case FLIP_ORDER_OF_CATHODES: { //перемотка по порядку катодов в лампе
            changeIndi = LAMP_NUM; //загрузили буфер
            for (uint8_t i = 0; i < LAMP_NUM; i++) {
              if (anim.flipBuffer[i] != changeBuffer[i]) { //если не достигли конца анимации разряда
                if (--anim.flipBuffer[i] > 9) anim.flipBuffer[i] = 9; //меняем цифру разряда
                indiPrintNum((mode != FLIP_ORDER_OF_CATHODES) ? anim.flipBuffer[i] : cathodeMask[anim.flipBuffer[i]], i);
              }
              else {
                if (anim.flipBuffer[i + 6] == INDI_NULL) indiClr(i); //очистка индикатора
                changeIndi--; //иначе завершаем анимацию для разряда
              }
            }
            if (!changeIndi) return; //выходим
            _timer_ms[TMR_ANIM] = (mode != FLIP_ORDER_OF_CATHODES) ? FLIP_MODE_3_TIME : FLIP_MODE_4_TIME; //устанавливаем таймер
          }
          break;
        case FLIP_TRAIN: { //поезд
            if (changeIndi < (LAMP_NUM + FLIP_MODE_5_STEP - 1)) {
              indiClr(); //очистка индикатора
              animPrintBuff(changeIndi + 1, 0, LAMP_NUM);
              animPrintBuff(changeIndi - (LAMP_NUM + FLIP_MODE_5_STEP - 1), 6, LAMP_NUM);
              changeIndi++;
              _timer_ms[TMR_ANIM] = FLIP_MODE_5_TIME; //устанавливаем таймер
            }
            else return; //выходим
          }
          break;
        case FLIP_RUBBER_BAND: { //резинка
            if (changeCnt < 2) {
              if (changeNum < LAMP_NUM) {
                switch (changeCnt) {
                  case 0:
                    for (uint8_t b = changeNum + 1; b > 0; b--) {
                      if ((b - 1) == (changeNum - changeIndi)) indiSet(anim.flipBuffer[(LAMP_NUM - 1) - changeNum], LAMP_NUM - b); //вывод часов
                      else indiClr(LAMP_NUM - b); //очистка индикатора
                    }
                    if (changeIndi++ >= changeNum) {
                      changeIndi = 0; //сбрасываем позицию индикатора
                      changeNum++; //прибавляем цикл
                    }
                    break;
                  case 1:
                    for (uint8_t b = 0; b < (LAMP_NUM - changeNum); b++) {
                      if (b == changeIndi) indiSet(anim.flipBuffer[((LAMP_NUM + 6) - 1) - changeNum], b); //вывод часов
                      else indiClr(b); //очистка индикатора
                    }
                    if (changeIndi++ >= (LAMP_NUM - 1) - changeNum) {
                      changeIndi = 0; //сбрасываем позицию индикатора
                      changeNum++; //прибавляем цикл
                    }
                    break;
                }
                if (anim.flipBuffer[((LAMP_NUM - 1) - changeNum) + (changeCnt * 6)] != INDI_NULL) _timer_ms[TMR_ANIM] = FLIP_MODE_6_TIME; //устанавливаем таймер
              }
              else {
                changeCnt++; //прибавляем цикл
                changeNum = 0; //сбросили счетчик
              }
            }
            else return; //выходим
          }
          break;
        case FLIP_GATES: { //ворота
            if (changeIndi < 2) {
              if (changeNum < ((LAMP_NUM / 2) + 1)) {
                indiClr(); //очистка индикатора
                if (!changeIndi) {
                  animPrintBuff(-changeNum, 0, (LAMP_NUM / 2));
                  animPrintBuff(changeNum + (LAMP_NUM / 2), (LAMP_NUM / 2), (LAMP_NUM / 2));
                }
                else {
                  animPrintBuff(changeNum - (LAMP_NUM / 2), 6, (LAMP_NUM / 2));
                  animPrintBuff(LAMP_NUM - changeNum, 6 + (LAMP_NUM / 2), (LAMP_NUM / 2));
                }
                changeNum++; //прибавляем цикл
                _timer_ms[TMR_ANIM] = FLIP_MODE_7_TIME; //устанавливаем таймер
              }
              else {
                changeIndi++; //прибавляем цикл
                changeNum = 0; //сбросили счетчик
              }
            }
            else return; //выходим
          }
          break;
        case FLIP_WAVE: { //волна
            if (changeCnt < 2) {
              if (changeNum < LAMP_NUM) {
                switch (changeCnt) {
                  case 0: indiClr((LAMP_NUM - 1) - changeNum); break; //очистка индикатора
                  case 1: animPrintBuff((LAMP_NUM - 1) - changeNum, (LAMP_NUM + 5) - changeNum, changeNum + 1); break; //вывод часов
                }
                if (anim.flipBuffer[changeNum + (changeCnt * 6)] != INDI_NULL) _timer_ms[TMR_ANIM] = FLIP_MODE_8_TIME; //устанавливаем таймер
                changeNum++; //прибавляем цикл
              }
              else {
                changeCnt++; //прибавляем цикл
                changeNum = 0; //сбросили счетчик
              }
            }
            else return; //выходим
          }
          break;
        case FLIP_HIGHLIGHTS: { //блики
            if (changeNum < LAMP_NUM) {
              if (changeCnt < 2) {
                switch (changeCnt) {
                  case 0:
                    changeIndi = random(0, LAMP_NUM);
                    for (uint8_t b = 0; b < changeNum; b++) {
                      while (changeBuffer[b] == changeIndi) {
                        changeIndi = random(0, LAMP_NUM);
                        b = 0;
                      }
                    }
                    changeBuffer[changeNum] = changeIndi;
                    indiClr(changeIndi); //очистка индикатора
                    break;
                  case 1:
                    indiSet(anim.flipBuffer[6 + changeIndi], changeIndi); //вывод часов
                    changeNum++; //прибавляем цикл
                    break; //вывод часов
                }
                changeCnt++; //прибавляем цикл
                if (anim.flipBuffer[changeIndi + (changeCnt * 6)] != INDI_NULL) _timer_ms[TMR_ANIM] = FLIP_MODE_9_TIME; //устанавливаем таймер
              }
              else changeCnt = 0; //сбросили счетчик
            }
            else return; //выходим
          }
          break;
        case FLIP_EVAPORATION: { //испарение
            if (changeCnt < 2) {
              changeIndi = random(0, LAMP_NUM);
              if (changeNum < LAMP_NUM) {
                for (uint8_t b = 0; b < changeNum; b++) {
                  while (changeBuffer[b] == changeIndi) {
                    changeIndi = random(0, LAMP_NUM);
                    b = 0;
                  }
                }
                changeBuffer[changeNum] = changeIndi;
                switch (changeCnt) {
                  case 0: indiClr(changeIndi); break; //очистка индикатора
                  case 1:
                    indiSet(anim.flipBuffer[6 + changeIndi], changeIndi); //вывод часов
                    break;
                }
                changeNum++; //прибавляем цикл
                if (anim.flipBuffer[changeIndi + (changeCnt * 6)] != INDI_NULL) _timer_ms[TMR_ANIM] = FLIP_MODE_10_TIME; //устанавливаем таймер
              }
              else {
                changeCnt++; //прибавляем цикл
                changeNum = 0; //сбросили счетчик
              }
            }
            else return; //выходим
          }
          break;

        case FLIP_SLOT_MACHINE: { //игровой автомат
            if (changeNum < LAMP_NUM) {
              for (uint8_t b = changeNum; b < LAMP_NUM; b++) {
                if (--anim.flipBuffer[b] > 9) anim.flipBuffer[b] = 9; //меняем цифру разряда
                indiPrintNum(anim.flipBuffer[b], b); //выводим разряд
              }
              if (anim.flipBuffer[changeNum] == changeBuffer[changeNum]) { //если разряд совпал
                if (anim.flipBuffer[changeNum + 6] == INDI_NULL) indiClr(changeNum); //очистка индикатора
                changeIndi += FLIP_MODE_11_STEP; //добавили шаг
                changeNum++; //завершаем анимацию для разряда
              }
              _timer_ms[TMR_ANIM] = (uint16_t)FLIP_MODE_11_TIME + changeIndi; //устанавливаем таймер
            }
            else return; //выходим
          }
          break;
        default: return; //неизвестная анимация
      }
    }
  }
}
//------------------Проверка модуля часов реального времени-------------------------
void checkRealTimeClock(void) //проверка модуля часов реального времени
{
#if DS3231_ENABLE
  if (!rtcDisable32K()) return; //отключение вывода 32K
#endif

#if SQW_PORT_ENABLE
#if DS3231_ENABLE
  if (!rtcSetSQW()) return; //установка SQW на 1Гц
#endif
  EICRA = (0x01 << ISC01); //настраиваем внешнее прерывание по спаду импульса на INT0
  EIFR |= (0x01 << INTF0); //сбрасываем флаг прерывания INT0

  for (_timer_ms[TMR_MS] = SQW_TEST_TIME; !(EIFR & (0x01 << INTF0)) && _timer_ms[TMR_MS];) systemTask(); //ждем сигнала от SQW
  tick_sec = 0; //сбросили счетчик секунд
#endif

#if DS3231_ENABLE
  if (!rtcGetTime(RTC_CLEAR_OSF)) { //считываем время из RTC
    rtcWriteAging(debugSettings.aging); //восстанавливаем коррекцию хода
    rtcSendTime(); //отправляем последнее сохраненное время в RTC
  }
#if ESP_ENABLE
  else device.status |= (0x01 << STATUS_UPDATE_TIME_SET); //установили статус актуального времени
#endif
#endif

#if SQW_PORT_ENABLE
  if (EIFR & (0x01 << INTF0)) { //если был сигнал с SQW
    EIFR |= (0x01 << INTF0); //сбрасываем флаг прерывания INT0
    EIMSK = (0x01 << INT0); //разрешаем внешнее прерывание INT0
  }
  else SET_ERROR(SQW_LONG_ERROR); //иначе выдаем ошибку
#endif
}
//-----------------------------Проверка ошибок-------------------------------------
void checkErrors(void) //проверка ошибок
{
  uint16_t _error_reg = EEPROM_ReadByte(EEPROM_BLOCK_ERROR) | ((uint16_t)EEPROM_ReadByte(EEPROM_BLOCK_EXT_ERROR) << 8); //прочитали регистр ошибок
#if ESP_ENABLE
  device.failure = _error_reg; //скопировали ошибки
#endif
  if (_error_reg) { //если есть ошибка
#if FLIP_ANIM_START == 1
    animShow = ANIM_MAIN; //установили флаг анимации
#elif FLIP_ANIM_START > 1
    animShow = (ANIM_OTHER + FLIP_ANIM_START); //установили флаг анимации
#endif
    for (uint8_t i = 0; i < 13; i++) { //проверяем весь регистр
      if (_error_reg & (0x01 << i)) { //если стоит флаг ошибки
        indiPrintNum(i + 1, 0, 4, 0); //вывод ошибки
#if PLAYER_TYPE
        playerSetTrack(PLAYER_ERROR_SOUND, PLAYER_GENERAL_FOLDER); //воспроизводим трек ошибки
        playerSpeakNumber(i + 1); //воспроизводим номер ошибки
#else
        uint8_t _sound_bit = ((i + 1) & 0x0F) | 0xF0; //указатель на бит ошибки
        melodyStop(); //остановка воспроизведения мелодии
#endif
        for (_timer_ms[TMR_MS] = ERROR_SHOW_TIME; _timer_ms[TMR_MS];) {
          dataUpdate(); //обработка данных
          if (buttonState()) { //если нажата кнопка
#if FLIP_ANIM_START
            animShow = ANIM_NULL; //сбросили флаг анимации
#endif
            break;
          }
#if !PLAYER_TYPE
          if (!_timer_ms[TMR_PLAYER] && (_sound_bit != 0x0F)) { //если звук не играет
            buzzPulse(ERROR_SOUND_FREQ, (_sound_bit & 0x01) ? ERROR_SOUND_HIGH_TIME : ERROR_SOUND_LOW_TIME); //воспроизводим звук
            _timer_ms[TMR_PLAYER] = (_sound_bit & 0x01) ? ERROR_SOUND_HIGH_PAUSE : ERROR_SOUND_LOW_PAUSE; //установили таймер
            _sound_bit >>= 1; //сместили указатель
          }
#endif
        }
      }
    }
    updateByte(0x00, EEPROM_BLOCK_ERROR, EEPROM_BLOCK_CRC_ERROR); //сбросили ошибки
    updateByte(0x00, EEPROM_BLOCK_EXT_ERROR, EEPROM_BLOCK_CRC_EXT_ERROR); //сбросили ошибки
  }
}
//---------------------------Проверка системы---------------------------------------
void testSystem(void) //проверка системы
{
  indiPrintNum(CONVERT_NUM(FIRMWARE_VERSION), 0); //отрисовываем версию прошивки
#if PLAYER_TYPE
  playerSetTrackNow(PLAYER_FIRMWARE_SOUND, PLAYER_GENERAL_FOLDER);
  playerSpeakNumber(CONVERT_CHAR(FIRMWARE_VERSION[0]));
  playerSpeakNumber(CONVERT_CHAR(FIRMWARE_VERSION[2]));
  playerSpeakNumber(CONVERT_CHAR(FIRMWARE_VERSION[4]));
#endif
  for (_timer_ms[TMR_MS] = TEST_FIRMWARE_TIME; _timer_ms[TMR_MS] && !buttonState();) systemTask(); //ждем

#if PLAYER_TYPE
  playerSetTrackNow(PLAYER_TEST_SOUND, PLAYER_GENERAL_FOLDER); //звук тестирования динамика
#else
  melodyPlay(SOUND_TEST_SPEAKER, SOUND_LINK(general_sound), REPLAY_ONCE); //сигнал тестирования динамика
#endif

#if (BACKL_TYPE != 3) && BACKL_TYPE
  ledBacklSetBright(TEST_BACKL_BRIGHT); //устанавливаем максимальную яркость
#endif
  indiSetBright(TEST_INDI_BRIGHT); //установка яркости индикаторов
#if ((SECS_DOT != 3) || !DOTS_PORT_ENABLE) && (SECS_DOT != 4)
  dotSetBright(TEST_DOT_BRIGHT); //установка яркости точек
#endif

#if DOTS_PORT_ENABLE
#if DOTS_TYPE == 2
  indiSetDots(0, DOTS_NUM * 2); //установка разделительных точек
#else
  indiSetDots(0, DOTS_NUM); //установка разделительных точек
#endif
#endif

  while (1) {
#if INDI_SYMB_TYPE
    indiClr(); //очистка индикаторов
    for (uint8_t symb = 0; symb < 10; symb++) {
      indiSetSymb(ID(symb)); //установка индикатора символов
      for (_timer_ms[TMR_MS] = TEST_LAMP_TIME; _timer_ms[TMR_MS];) { //ждем
        dataUpdate(); //обработка данных
        if (buttonState()) return; //выходим если нажата кнопка
      }
    }
    indiClrSymb(); //очистка индикатора символов
#endif
    for (uint8_t indi = 0; indi < LAMP_NUM; indi++) {
      indiClr(); //очистка индикаторов
#if BACKL_TYPE == 3
      wsBacklSetLedBright(0); //выключаем светодиоды
      wsBacklSetLedBright(indi, TEST_BACKL_BRIGHT); //включаем светодиод
#endif
      for (uint8_t digit = 0; digit < 10; digit++) {
        indiPrintNum(digit, indi); //отрисовываем цифру
#if BACKL_TYPE == 3
        wsBacklSetLedHue(indi, digit * 25, WHITE_OFF); //устанавливаем статичный цвет
#endif
        for (_timer_ms[TMR_MS] = TEST_LAMP_TIME; _timer_ms[TMR_MS];) { //ждем
          dataUpdate(); //обработка данных
          if (buttonState()) return; //выходим если нажата кнопка
        }
      }
    }
  }
}
//-----------------------------Проверка пароля------------------------------------
boolean checkPass(void) //проверка пароля
{
  boolean blink_data = 0; //мигание сигментами
  uint8_t cur_indi = 0; //текущий индикатор
  uint8_t time_out = 0; //таймер авто выхода
  uint8_t attempts_pass = 0; //попытки ввода пароля
  uint8_t entry_pass[] = {0, 0, 0, 0}; //введеный пароль

  dotSetBright(0); //выключаем точки
  indiSetBright(30); //устанавливаем максимальную яркость индикаторов

#if PLAYER_TYPE
  playerSetTrack(PLAYER_DEBUG_SOUND, PLAYER_GENERAL_FOLDER);
#endif

  while (1) {
    dataUpdate(); //обработка данных

    if (!indi.update) {
      indi.update = 1; //сбросили флаг
      if (++time_out >= DEBUG_TIMEOUT) { //если время вышло
#if FLIP_ANIM_START == 1
        animShow = ANIM_MAIN; //установили флаг анимации
#elif FLIP_ANIM_START > 1
        animShow = (ANIM_OTHER + FLIP_ANIM_START); //установили флаг анимации
#endif
        return 0; //выходим
      }
    }

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = DEBUG_PASS_BLINK_TIME; //устанавливаем таймер

      for (uint8_t i = 0; i < 4; i++) indiPrintNum(entry_pass[i], (LAMP_NUM / 2 - 2) + i); //вывод пароля
      if (blink_data) indiClr(cur_indi + (LAMP_NUM / 2 - 2)); //очистка индикатора

      blink_data = !blink_data; //мигаем индикатором
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        if (entry_pass[cur_indi] > 0) entry_pass[cur_indi]--; else entry_pass[cur_indi] = 9;
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        if (entry_pass[cur_indi] < 9) entry_pass[cur_indi]++; else entry_pass[cur_indi] = 0;
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        if (cur_indi < 3) cur_indi++; else cur_indi = 0; //переключаем разряды
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        if (((uint16_t)entry_pass[3] + ((uint16_t)entry_pass[2] * 10) + ((uint16_t)entry_pass[1] * 100) + ((uint16_t)entry_pass[0] * 1000)) == DEBUG_PASS) return 1; //если пароль совпал
        for (uint8_t i = 0; i < 4; i++) entry_pass[i] = 0; //сбросили введеный пароль
        cur_indi = 0; //сбросили текущий индикатор
#if PLAYER_TYPE
        playerSetTrack(PLAYER_PASS_SOUND, PLAYER_GENERAL_FOLDER); //сигнал ошибки ввода пароля
#else
        melodyPlay(SOUND_PASS_ERROR, SOUND_LINK(general_sound), REPLAY_ONCE); //сигнал ошибки ввода пароля
#endif
        if (++attempts_pass >= DEBUG_PASS_ATTEMPTS) return 0; //выходим если превышено количество попыток ввода пароля
        break;
    }
  }
  return 0;
}
//-----------------------------Отладка------------------------------------
void debugMenu(void) //отладка
{
  boolean cur_set = 0; //режим настройки
  boolean cur_reset = 0; //сброс настройки
  boolean cur_update = 0; //обновление индикаторов
  uint8_t cur_mode = 0; //текущий режим
#if IR_PORT_ENABLE
  uint8_t cur_button = 0; //текущая кнопка пульта
#endif
#if LIGHT_SENS_ENABLE
  uint8_t temp_min = 255;
  uint8_t temp_max = 0;
#endif

#if PLAYER_TYPE
  playerSetTrackNow(PLAYER_DEBUG_MENU_START, PLAYER_MENU_FOLDER); //воспроизводим название пункта отладки
#endif

  dotSetBright(0); //выключаем точки
  indiSetBright(30); //устанавливаем максимальную яркость индикаторов

  //настройки
  while (1) {
    dataUpdate(); //обработка данных

#if LIGHT_SENS_ENABLE || IR_PORT_ENABLE
    if (cur_set) {
      switch (cur_mode) {
#if IR_PORT_ENABLE
        case DEB_IR_BUTTONS: //програмирование кнопок
          if (irGetDebugStatus()) { //если управление ИК заблокировано и пришла новая команда
            indiClr(3); //очистка индикатора
            indiPrintNum((uint8_t)irGetCommand(), 0, 3, 0); //выводим код кнопки пульта
            debugSettings.irButtons[cur_button] = irGetCommand(); //записываем команду в массив
            _timer_ms[TMR_MS] = DEBUG_IR_BUTTONS_TIME; //установили таймер
          }
          else if (!_timer_ms[TMR_MS]) {
            cur_update = 0; //обновление экрана
            _timer_ms[TMR_MS] = DEBUG_IR_BUTTONS_TIME; //установили таймер
          }
          break;
#endif
#if LIGHT_SENS_ENABLE
        case DEB_LIGHT_SENS: //калибровка датчика освещения
          if (!_timer_ms[TMR_MS]) {
            if (temp_min > light_adc) temp_min = light_adc;
            if (temp_max < light_adc) temp_max = light_adc;
            analogState |= 0x01; //установили флаг обновления АЦП сенсора яркости
            _timer_ms[TMR_MS] = DEBUG_LIGHT_SENS_TIME; //установили таймер
            cur_update = 0; //обновление экрана
          }
          break;
#endif
      }
    }
#endif

    if (!cur_update) {
      cur_update = 1; //сбрасываем флаг

      indiClr(); //очистка индикаторов
      switch (cur_set) {
        case 0:
          indiPrintNum(cur_mode + 1, (LAMP_NUM / 2 - 1), 2, 0); //вывод режима
          break;
        case 1:
#if LAMP_NUM > 4
          indiPrintNum(cur_mode + 1, 5); //режим
#endif
          switch (cur_mode) {
#if DS3231_ENABLE
            case DEB_AGING_CORRECT: indiPrintNum(debugSettings.aging + 128, 0); break; //выводим коррекцию DS3231
#endif
            case DEB_TIME_CORRECT: indiPrintNum(debugSettings.timePeriod, 0); break; //выводим коррекцию внутреннего таймера
#if GEN_ENABLE
            case DEB_DEFAULT_MIN_PWM: indiPrintNum(debugSettings.min_pwm, 0); break; //выводим минимальный шим
            case DEB_DEFAULT_MAX_PWM: indiPrintNum(debugSettings.max_pwm, 0); break; //выводим максимальный шим
#if GEN_FEEDBACK == 1
            case DEB_HV_ADC: indiPrintNum(hv_treshold, 0); break; //выводим корекцию напряжения
#endif
#endif
#if IR_PORT_ENABLE
            case DEB_IR_BUTTONS: //програмирование кнопок
              indiPrintNum((debugSettings.irButtons[cur_button]) ? 1 : 0, 0); //выводим значение записи в ячейке кнопки пульта
              indiPrintNum(cur_button + 1, 2, 2, 0); //выводим номер кнопки пульта
              break;
#endif
#if LIGHT_SENS_ENABLE
            case DEB_LIGHT_SENS: //калибровка датчика освещения
              indiPrintNum(light_adc, 1, 3); //выводим значение АЦП датчика освещения
              break;
#endif
            case DEB_RESET: indiPrintNum(cur_reset, 0, 2, 0); break; //сброс настроек отладки
          }
          break;
      }
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (cur_set) {
          case 0:
            if (cur_mode > 0) cur_mode--;
            else cur_mode = DEB_MAX_ITEMS - 1;
#if PLAYER_TYPE
            playerSetTrackNow(PLAYER_DEBUG_MENU_START + cur_mode, PLAYER_MENU_FOLDER); //воспроизводим название пункта отладки
#endif
            break;
          case 1:
            switch (cur_mode) {
#if DS3231_ENABLE
              case DEB_AGING_CORRECT: if (debugSettings.aging > -127) debugSettings.aging--; else debugSettings.aging = 127; break; //коррекция хода
#endif
              case DEB_TIME_CORRECT: if (debugSettings.timePeriod > US_PERIOD_MIN) debugSettings.timePeriod--; else debugSettings.timePeriod = US_PERIOD_MAX; break; //коррекция хода
#if GEN_ENABLE
              case DEB_DEFAULT_MIN_PWM: //коррекция минимального значения шим
                if (debugSettings.min_pwm > 100) debugSettings.min_pwm -= 5; //минимальное значение шим
                indiChangeCoef(); //обновление коэффициента линейного регулирования
                break;
              case DEB_DEFAULT_MAX_PWM: //коррекция максимального значения шим
                if (debugSettings.max_pwm > 150) debugSettings.max_pwm -= 5; //максимальное значение шим
                indiChangeCoef(); //обновление коэффициента линейного регулирования
                break;
#if GEN_FEEDBACK == 1
              case DEB_HV_ADC: //коррекция значения ацп преобразователя
                if (debugSettings.hvCorrect > -30) debugSettings.hvCorrect--; //значение ацп преобразователя
                updateTresholdADC(); //обновление предела удержания напряжения
                break;
#endif
#endif
#if IR_PORT_ENABLE
              case DEB_IR_BUTTONS: //програмирование кнопок
                if (cur_button) cur_button--;
                break;
#endif
              case DEB_RESET: cur_reset = 0; break; //сброс настроек отладки
            }
            break;
        }
        cur_update = 0; //обновление экрана
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (cur_set) {
          case 0:
            if (cur_mode < (DEB_MAX_ITEMS - 1)) cur_mode++;
            else cur_mode = 0;
#if PLAYER_TYPE
            playerSetTrackNow(PLAYER_DEBUG_MENU_START + cur_mode, PLAYER_MENU_FOLDER); //воспроизводим название пункта отладки
#endif
            break;
          case 1:
            switch (cur_mode) {
#if DS3231_ENABLE
              case DEB_AGING_CORRECT: if (debugSettings.aging < 127) debugSettings.aging++; else debugSettings.aging = -127; break; //коррекция хода
#endif
              case DEB_TIME_CORRECT: if (debugSettings.timePeriod < US_PERIOD_MAX) debugSettings.timePeriod++; else debugSettings.timePeriod = US_PERIOD_MIN; break; //коррекция хода
#if GEN_ENABLE
              case DEB_DEFAULT_MIN_PWM: //коррекция минимального значения шим
                if (debugSettings.min_pwm < 190) debugSettings.min_pwm += 5; //минимальное значение шим
                indiChangeCoef(); //обновление коэффициента линейного регулирования
                break;
              case DEB_DEFAULT_MAX_PWM: //коррекция максимального значения шим
                if (debugSettings.max_pwm < 200) debugSettings.max_pwm += 5; //максимальное значение шим
                indiChangeCoef(); //обновление коэффициента линейного регулирования
                break;
#if GEN_FEEDBACK == 1
              case DEB_HV_ADC: //коррекция значения ацп преобразователя
                if (debugSettings.hvCorrect < 30) debugSettings.hvCorrect++; //значение ацп преобразователя
                updateTresholdADC(); //обновление предела удержания напряжения
                break;
#endif
#endif
#if IR_PORT_ENABLE
              case DEB_IR_BUTTONS: //програмирование кнопок
                if (cur_button < (KEY_MAX_ITEMS - 2)) cur_button++;
                break;
#endif
              case DEB_RESET: cur_reset = 1; break; //сброс настроек отладки
            }
            break;
        }
        cur_update = 0; //обновление экрана
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        cur_set = !cur_set; //сменили сотояние подрежима меню

        if (cur_set) { //если в режиме настройки
          switch (cur_mode) {
#if DS3231_ENABLE
            case DEB_AGING_CORRECT: if (!rtcReadAging(&debugSettings.aging)) cur_set = 0; break; //чтение коррекции хода
#endif
            case DEB_TIME_CORRECT: break; //коррекция хода
#if GEN_ENABLE
            case DEB_DEFAULT_MIN_PWM: indiSetBright(1); break; //минимальное значение шим
            case DEB_DEFAULT_MAX_PWM: break; //максимальное значение шим
#if GEN_FEEDBACK == 1
            case DEB_HV_ADC: break; //коррекция значения ацп преобразователя
#endif
#endif
#if IR_PORT_ENABLE
            case DEB_IR_BUTTONS: //програмирование кнопок
              _timer_ms[TMR_MS] = 0; //сбросили таймер
              cur_button = 0; //сбросили номер текущей кнопки
              irSetDebugMode(); //перевести IR приемник в режим отладки
              break;
#endif
#if LIGHT_SENS_ENABLE
            case DEB_LIGHT_SENS: //калибровка датчика освещения
              _timer_ms[TMR_MS] = 0; //сбросили таймер
              temp_min = 255; //установили минимальное значение
              temp_max = 0; //установили максимальное значение
              break;
#endif
            case DEB_RESET: cur_reset = 0; break; //сброс настроек отладки
            default: cur_set = 0; break; //запретили войти в пункт меню
          }
        }
        else { //иначе режим выбора пункта меню
          switch (cur_mode) {
            case DEB_AGING_CORRECT: rtcWriteAging(debugSettings.aging); break; //запись коррекции хода
#if IR_PORT_ENABLE
            case DEB_IR_BUTTONS: //програмирование кнопок
              irResetStatus(); //сбросить статус IR приемника
              break;
#endif
#if LIGHT_SENS_ENABLE
            case DEB_LIGHT_SENS: lightSensZoneUpdate(temp_min, temp_max); break; //обновление зон сенсора яркости освещения
#endif
            case DEB_RESET: //сброс настроек отладки
              if (cur_reset) { //подтверждение
                cur_mode = 0; //перешли на первый пункт меню
#if DS3231_ENABLE
                debugSettings.aging = 0; //коррекции хода модуля часов
#endif
                debugSettings.timePeriod = US_PERIOD; //коррекция хода внутреннего осцилятора
#if GEN_ENABLE
                debugSettings.min_pwm = DEFAULT_MIN_PWM; //минимальное значение шим
                debugSettings.max_pwm = DEFAULT_MAX_PWM; //максимальное значение шим
                indiChangeCoef(); //обновление коэффициента линейного регулирования
#if GEN_FEEDBACK == 1
                debugSettings.hvCorrect = 0; //коррекция напряжения преобразователя
                updateTresholdADC(); //обновление предела удержания напряжения
#endif
#endif
#if IR_PORT_ENABLE
                for (uint8_t i = 0; i < (KEY_MAX_ITEMS - 1); i++) debugSettings.irButtons[i] = 0; //сбрасываем значение ячеек кнопок пульта
#endif
#if DS3231_ENABLE
                rtcWriteAging(debugSettings.aging); //запись коррекции хода
#endif
#if LIGHT_SENS_ENABLE
                lightSensZoneUpdate(LIGHT_SENS_START_MIN, LIGHT_SENS_START_MAX); //обновление зон сенсора яркости освещения
#endif
#if PLAYER_TYPE
                playerSetTrack(PLAYER_RESET_SOUND, PLAYER_GENERAL_FOLDER);
#else
                melodyPlay(SOUND_RESET_SETTINGS, SOUND_LINK(general_sound), REPLAY_ONCE); //сигнал сброса настроек отладки
#endif
              }
              break;
          }
          indiSetBright(30); //устанавливаем максимальную яркость индикаторов
        }
        dotSetBright((cur_set) ? 250 : 0); //включаем точки
        cur_update = 0; //обновление экрана
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        if (!cur_set) { //если не в режиме настройки
          updateData((uint8_t*)&debugSettings, sizeof(debugSettings), EEPROM_BLOCK_SETTINGS_DEBUG, EEPROM_BLOCK_CRC_DEBUG); //записываем настройки отладки в память
          return; //выходим
        }
#if IR_PORT_ENABLE
        else if (cur_mode == DEB_IR_BUTTONS) { //если програмирование кнопок
          debugSettings.irButtons[cur_button] = 0;
          cur_update = 0; //обновление экрана
        }
#endif
        break;
    }
  }
}
//----------------------------Настройки времени----------------------------------
uint8_t settings_time(void) //настройки времени
{
  boolean time_update = 0; //флаг изменения времени
  boolean blink_data = 0; //мигание сигментами
  uint8_t cur_mode = 0; //текущий режим
  uint8_t time_out = 0; //таймаут автовыхода

  dotSetBright(dot.menuBright); //включаем точки

  _timer_ms[TMR_MS] = 0; //сбросили таймер

#if BACKL_TYPE == 3
  backlAnimDisable(); //запретили эффекты подсветки
  wsBacklSetLedBright(backl.menuBright); //установили максимальную яркость
#endif

#if PLAYER_TYPE
  if (mainSettings.baseSound) playerSetTrackNow(PLAYER_TIME_SET_SOUND, PLAYER_GENERAL_FOLDER); //воспроизводим название меню
#endif

#if INDI_SYMB_TYPE
  indiSetSymb(SYMB_MENU); //установка индикатора символов
#endif

  while (1) {
    dataUpdate(); //обработка данных

#if ESP_ENABLE
    if (busCheck()) return MAIN_PROGRAM;
#endif

    if (!indi.update) {
      indi.update = 1;
      if (++time_out >= SETTINGS_TIMEOUT) {
#if DS3231_ENABLE
        rtcSendTime(); //отправить время в RTC
#endif
#if ESP_ENABLE
        device.status |= (0x01 << STATUS_UPDATE_TIME_SET); //установили статус актуального времени
#endif
        animShow = ANIM_MAIN; //установили флаг анимации
        return MAIN_PROGRAM;
      }
    }

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

      indiClr(); //очистка индикаторов
#if LAMP_NUM > 4
      indiPrintNum(cur_mode + 1, 5); //режим
#endif
      switch (cur_mode) {
        case 0:
        case 1:
          indiPrintMenuData(blink_data, cur_mode, RTC.h, 0, RTC.m, 2); //вывод часов/минут
          break;
        case 2:
        case 3:
          indiPrintMenuData(blink_data, cur_mode & 0x01, RTC.DD, 0, RTC.MM, 2); //вывод даты/месяца
          break;
        case 4:
          if (!blink_data) indiPrintNum(RTC.YY, 0); //вывод года
          break;
      }
#if BACKL_TYPE == 3
      wsBacklSetMultipleHue((cur_mode & 0x01) * 2, (cur_mode != 4) ? 2 : 4, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); //подсветка активных разрядов
#endif
      blink_data = !blink_data; //мигание сигментами
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (cur_mode) {
          //настройка времени
          case 0: if (RTC.h > 0) RTC.h--; else RTC.h = 23; RTC.s = 0; time_update = 1; break; //часы
          case 1: if (RTC.m > 0) RTC.m--; else RTC.m = 59; RTC.s = 0; time_update = 1; break; //минуты

          //настройка даты
          case 2: if (RTC.DD > 1) RTC.DD--; else RTC.DD = maxDays(); break; //день
          case 3: //месяц
            if (RTC.MM > 1) RTC.MM--;
            else RTC.MM = 12;
            if (RTC.DD > maxDays()) RTC.DD = maxDays();
            break;

          //настройка года
          case 4: if (RTC.YY > 2000) RTC.YY--; else RTC.YY = 2099; break; //год
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (cur_mode) {
          //настройка времени
          case 0: if (RTC.h < 23) RTC.h++; else RTC.h = 0; RTC.s = 0; time_update = 1; break; //часы
          case 1: if (RTC.m < 59) RTC.m++; else RTC.m = 0; RTC.s = 0; time_update = 1; break; //минуты

          //настройка даты
          case 2: if (RTC.DD < maxDays()) RTC.DD++; else RTC.DD = 1; break; //день
          case 3: //месяц
            if (RTC.MM < 12) RTC.MM++;
            else RTC.MM = 1;
            if (RTC.DD > maxDays()) RTC.DD = maxDays();
            break;

          //настройка года
          case 4: if (RTC.YY < 2099) RTC.YY++; else RTC.YY = 2000; break; //год
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        if (cur_mode < 4) cur_mode++; else cur_mode = 0;
        if (cur_mode != 4) dotSetBright(dot.menuBright); //включаем точки
        else dotSetBright(0); //выключаем точки
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        if (cur_mode < 2 && time_update) RTC.s = 0; //сбрасываем секунды
#if DS3231_ENABLE
        rtcSendTime(); //отправить время в RTC
#endif
#if ESP_ENABLE
        device.status |= (0x01 << STATUS_UPDATE_TIME_SET); //установили статус актуального времени
#endif
        return MAIN_PROGRAM;
    }
  }
  return INIT_PROGRAM;
}
//-----------------------------Настройка будильника------------------------------------
uint8_t settings_singleAlarm(void) //настройка будильника
{
  boolean cur_indi = 0; //текущий индикатор
  boolean blink_data = 0; //мигание сигментами
  uint8_t alarm[ALARM_MAX_ARR]; //массив данных о будильнике
  uint8_t cur_mode = 0; //текущий режим
  uint8_t cur_day = 1; //текущий день недели
  uint8_t time_out = 0; //таймаут автовыхода

  dotSetBright(dot.menuBright); //включаем точки

  alarmReset(); //сброс будильника
  alarmReadBlock(1, alarm); //читаем блок данных

  _timer_ms[TMR_MS] = 0; //сбросили таймер

#if BACKL_TYPE == 3
  backlAnimDisable(); //запретили эффекты подсветки
  wsBacklSetLedBright(backl.menuBright); //установили максимальную яркость
#endif

#if PLAYER_TYPE
  if (mainSettings.baseSound) playerSetTrackNow(PLAYER_ALARM_SET_SOUND, PLAYER_GENERAL_FOLDER); //воспроизводим название меню
#endif

#if INDI_SYMB_TYPE
  indiSetSymb(SYMB_MENU); //установка индикатора символов
#endif

  while (1) {
    dataUpdate(); //обработка данных

#if ESP_ENABLE
    if (busCheck()) {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
      if ((cur_mode == 3) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
#if PLAYER_TYPE
      playerStop(); //сброс воспроизведения мелодии
#else
      melodyStop(); //сброс воспроизведения мелодии
#endif
      return MAIN_PROGRAM; //выходим
    }
#endif

#if PLAYER_TYPE
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
    if (cur_mode == 3 && !alarm[ALARM_RADIO] && !playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarm[ALARM_SOUND], PLAYER_ALARM_FOLDER); //воспроизводим мелодию будильника
#else
    if (cur_mode == 3 && !playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarm[ALARM_SOUND], PLAYER_ALARM_FOLDER); //воспроизводим мелодию будильника
#endif
#endif

    if (!indi.update) {
      indi.update = 1;
      if (++time_out >= SETTINGS_TIMEOUT) {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
        if ((cur_mode == 3) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
#if PLAYER_TYPE
        playerStop(); //сброс воспроизведения мелодии
#else
        melodyStop(); //сброс воспроизведения мелодии
#endif
        animShow = ANIM_MAIN; //установили флаг анимации
        return MAIN_PROGRAM; //выходим по тайм-ауту
      }
    }

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

      indiClr(); //очистка индикаторов
#if LAMP_NUM > 4
      indiPrintNum(cur_mode + 1, 5); //режим
#endif
      switch (cur_mode) {
        case 0:
          indiPrintMenuData(blink_data, cur_indi, alarm[ALARM_HOURS], 0, alarm[ALARM_MINS], 2); //вывод часов/минут
          break;
        case 1:
        case 2:
          if (!blink_data || cur_mode != 1) indiPrintNum(alarm[ALARM_MODE], 0); //вывод режима
          if (alarm[ALARM_MODE] > 3) {
            if (!blink_data || cur_mode != 2 || cur_indi) indiPrintNum(cur_day, 2); //вывод дня недели
            if (!blink_data || cur_mode != 2 || !cur_indi) indiPrintNum((alarm[ALARM_DAYS] >> cur_day) & 0x01, 3); //вывод установки
          }
          break;
        case 3:
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
#if PLAYER_TYPE
          indiPrintMenuData(blink_data, cur_indi, alarm[ALARM_VOLUME], 0, alarm[ALARM_SOUND] + !alarm[ALARM_RADIO], 2); //вывод громкости мелодии/номера мелодии
#else
          if (alarm[ALARM_RADIO]) indiPrintMenuData(blink_data, cur_indi, alarm[ALARM_VOLUME], 0, alarm[ALARM_SOUND], 2); //вывод громкости радио/номера радиостанции
          else if (!blink_data) indiPrintNum(alarm[ALARM_SOUND] + 1, 2, 2, 0); //вывод номера мелодии
#endif
#else
#if PLAYER_TYPE
          indiPrintMenuData(blink_data, cur_indi, alarm[ALARM_VOLUME], 0, alarm[ALARM_SOUND] + 1, 2); //вывод громкости мелодии/номера мелодии
#else
          if (!blink_data) indiPrintNum(alarm[ALARM_SOUND] + 1, 2, 2, 0); //вывод номера мелодии
#endif
#endif
          break;
      }
#if BACKL_TYPE == 3
      switch (cur_mode) {
        case 1: wsBacklSetMultipleHue(0, 1, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
        case 2: wsBacklSetMultipleHue((cur_indi) ? 3 : 2, 1, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
#if !PLAYER_TYPE
        case 3:
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
          if (alarm[ALARM_RADIO]) wsBacklSetMultipleHue(cur_indi * 2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); //подсветка активных разрядов
          else wsBacklSetMultipleHue(2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2);  //подсветка активных разрядов
#else
          wsBacklSetMultipleHue(2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2);  //подсветка активных разрядов
#endif
          break;
#endif
        default: wsBacklSetMultipleHue(cur_indi * 2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
      }
#endif
      blink_data = !blink_data; //мигание сигментами
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (cur_mode) {
          //настройка времени будильника
          case 0:
            switch (cur_indi) {
              case 0: if (alarm[ALARM_HOURS] > 0) alarm[ALARM_HOURS]--; else alarm[ALARM_HOURS] = 23; break; //часы
              case 1: if (alarm[ALARM_MINS] > 0) alarm[ALARM_MINS]--; else alarm[ALARM_MINS] = 59; break; //минуты
            }
            break;
          //настройка режима будильника
          case 1: if (alarm[ALARM_MODE] > 0) alarm[ALARM_MODE]--; else alarm[ALARM_MODE] = 4; break; //режим
          case 2:
            switch (cur_indi) {
              case 0: if (cur_day > 1) cur_day--; else cur_day = 7; break; //день недели
              case 1: alarm[ALARM_DAYS] &= ~(0x01 << cur_day); break; //установка
            }
            break;
          //настройка мелодии будильника
          case 3:
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] > 0) alarm[ALARM_VOLUME]--; else alarm[ALARM_VOLUME] = MAIN_MAX_VOL; //громкость
                if (alarm[ALARM_RADIO]) setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //устанавливаем громкость
                else playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
                break;
              case 1:
                if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = (alarm[ALARM_RADIO]) ? 9 : (PLAYER_ALARM_MAX - 1); //мелодия
                if (alarm[ALARM_RADIO]) setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
                break;
            }
#else
            if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = (alarm[ALARM_RADIO]) ? 9 : (SOUND_MAX(alarm_sound) - 1); //мелодия
            if (alarm[ALARM_RADIO]) setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
            else melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#else
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] > 0) alarm[ALARM_VOLUME]--; else alarm[ALARM_VOLUME] = MAIN_MAX_VOL; //громкость
                playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
                break;
              case 1:
                if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = PLAYER_ALARM_MAX - 1; //мелодия
                break;
            }
#else
            if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = SOUND_MAX(alarm_sound) - 1; //мелодия
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#endif
            break;
        }
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (cur_mode) {
          //настройка времени будильника
          case 0:
            switch (cur_indi) {
              case 0: if (alarm[ALARM_HOURS] < 23) alarm[ALARM_HOURS]++; else alarm[ALARM_HOURS] = 0; break; //часы
              case 1: if (alarm[ALARM_MINS] < 59) alarm[ALARM_MINS]++; else alarm[ALARM_MINS] = 0; break; //минуты
            }
            break;
          //настройка режима будильника
          case 1: if (alarm[ALARM_MODE] < 4) alarm[ALARM_MODE]++; else alarm[ALARM_MODE] = 0; break; //режим
          case 2:
            switch (cur_indi) {
              case 0: if (cur_day < 7) cur_day++; else cur_day = 1; break; //день недели
              case 1: alarm[ALARM_DAYS] |= (0x01 << cur_day); break; //установка
            }
            break;
          //настройка мелодии будильника
          case 3:
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] < MAIN_MAX_VOL) alarm[ALARM_VOLUME]++; else alarm[ALARM_VOLUME] = 0; //громкость
                if (alarm[ALARM_RADIO]) setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //устанавливаем громкость
                else playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
                break;
              case 1:
                if (alarm[ALARM_SOUND] < ((alarm[ALARM_RADIO]) ? 9 : (PLAYER_ALARM_MAX - 1))) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
                if (alarm[ALARM_RADIO]) setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
                break;
            }
#else
            if (alarm[ALARM_SOUND] < ((alarm[ALARM_RADIO]) ? 9 : (SOUND_MAX(alarm_sound) - 1))) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
            if (alarm[ALARM_RADIO]) setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
            else melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#else
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] < MAIN_MAX_VOL) alarm[ALARM_VOLUME]++; else alarm[ALARM_VOLUME] = 0; //громкость
                playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
                break;
              case 1:
                if (alarm[ALARM_SOUND] < (PLAYER_ALARM_MAX - 1)) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
                break;
            }
#else
            if (alarm[ALARM_SOUND] < (SOUND_MAX(alarm_sound) - 1)) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#endif
            break;
        }
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        cur_indi = !cur_indi;
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case LEFT_KEY_HOLD: //удержание левой кнопки
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
        if ((cur_mode == 3) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
        if (cur_mode > 0) cur_mode--; else cur_mode = 3; //переключение пунктов

        if ((cur_mode == 2) && (alarm[ALARM_MODE] < 4)) cur_mode = 1; //если нет дней недели
        if (cur_mode == 3) { //если режим настройки мелодии
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
          if (alarm[ALARM_RADIO] && (getPowerStatusRDA() != RDA_ERROR)) { //если режим радиобудильника
            setPowerRDA(RDA_ON); //включаем радио
            setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //устанавливаем громкость
            setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
          }
          else { //иначе обычный режим
            alarm[ALARM_RADIO] = 0; //обычный режим
            setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
            playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
#else
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
          }
#else
#if PLAYER_TYPE
          playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
#else
          melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#endif
        }
        dotSetBright((cur_mode) ? 0 : dot.menuBright); //включаем точки

        cur_indi = 0; //сбрасываем текущий индикатор
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case RIGHT_KEY_HOLD: //удержание правой кнопки
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
        if ((cur_mode == 3) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
        if (cur_mode < 3) cur_mode++; else cur_mode = 0; //переключение пунктов

        if ((cur_mode == 2) && (alarm[ALARM_MODE] < 4)) cur_mode = 3; //если нет дней недели
        if (cur_mode == 3) { //если режим настройки мелодии
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
          if (alarm[ALARM_RADIO] && (getPowerStatusRDA() != RDA_ERROR)) { //если режим радиобудильника
            setPowerRDA(RDA_ON); //включаем радио
            setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //устанавливаем громкость
            setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
          }
          else { //иначе обычный режим
            alarm[ALARM_RADIO] = 0; //обычный режим
            setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
            playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
#else
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
          }
#else
#if PLAYER_TYPE
          playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
#else
          melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#endif
        }
        dotSetBright((cur_mode) ? 0 : dot.menuBright); //включаем точки

        cur_indi = 0; //сбрасываем текущий индикатор
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
        if ((cur_mode == 3) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
#if ESP_ENABLE
        device.status |= (0x01 << STATUS_UPDATE_ALARM_SET);
#endif
        alarm[ALARM_STATUS] = 255; //установили статус изменения будильника
        alarmWriteBlock(1, alarm); //записать блок основных данных будильника и выйти
        return MAIN_PROGRAM;

#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
      case ADD_KEY_HOLD: //удержание дополнительной кнопки
        if (cur_mode == 3) {
          if (alarm[ALARM_RADIO]) { //если режим радиобудильника
            alarm[ALARM_RADIO] = 0;
            alarm[ALARM_SOUND] = 0;
            setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
            playerSetVolNow(alarm[ALARM_VOLUME]); //установка громкости
#endif
          }
          else if (getPowerStatusRDA() != RDA_ERROR) { //иначе если радиоприемник доступен
            alarm[ALARM_RADIO] = 1;
            alarm[ALARM_SOUND] = 0;
            setPowerRDA(RDA_ON); //включаем радио
            setVolumeRDA(alarm[ALARM_VOLUME]); //устанавливаем громкость
            setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
          }
          blink_data = 0; //сбрасываем флаги
          _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        }
        break;
#endif
    }
  }
  return INIT_PROGRAM;
}
//-----------------------------Настройка будильников------------------------------------
uint8_t settings_multiAlarm(void) //настройка будильников
{
  boolean cur_indi = 0; //текущий индикатор
  boolean blink_data = 0; //мигание сигментами
  uint8_t alarm[ALARM_MAX_ARR]; //массив данных о будильнике
  uint8_t cur_mode = 0; //текущий режим
  uint8_t cur_day = 1; //текущий день недели
  uint8_t time_out = 0; //таймаут автовыхода
  uint8_t cur_alarm = alarms.num > 0;

  alarmReset(); //сброс будильника
  alarmReadBlock(cur_alarm, alarm); //читаем блок данных

  _timer_ms[TMR_MS] = 0; //сбросили таймер

#if BACKL_TYPE == 3
  backlAnimDisable(); //запретили эффекты подсветки
  wsBacklSetLedBright(backl.menuBright); //установили максимальную яркость
#endif

#if PLAYER_TYPE
  if (mainSettings.baseSound) playerSetTrackNow(PLAYER_ALARM_SET_SOUND, PLAYER_GENERAL_FOLDER); //воспроизводим название меню
#endif

#if INDI_SYMB_TYPE
  indiSetSymb(SYMB_MENU); //установка индикатора символов
#endif

  while (1) {
    dataUpdate(); //обработка данных

#if ESP_ENABLE
    if (busCheck()) {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
      if ((cur_mode == 4) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
#if PLAYER_TYPE
      playerStop(); //сброс воспроизведения мелодии
#else
      melodyStop(); //сброс воспроизведения мелодии
#endif
      return MAIN_PROGRAM; //выходим
    }
#endif

#if PLAYER_TYPE
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
    if (cur_mode == 4 && !alarm[ALARM_RADIO] && !playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarm[ALARM_SOUND], PLAYER_ALARM_FOLDER); //воспроизводим мелодию будильника
#else
    if (cur_mode == 4 && !playerPlaybackStatus()) playerSetTrack(PLAYER_ALARM_START + alarm[ALARM_SOUND], PLAYER_ALARM_FOLDER); //воспроизводим мелодию будильника
#endif
#endif

    if (!indi.update) {
      indi.update = 1;
      if (++time_out >= SETTINGS_TIMEOUT) {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
        if ((cur_mode == 4) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
#if PLAYER_TYPE
        playerStop(); //сброс воспроизведения мелодии
#else
        melodyStop(); //сброс воспроизведения мелодии
#endif
        animShow = ANIM_MAIN; //установили флаг анимации
        return MAIN_PROGRAM; //выходим по тайм-ауту
      }
    }

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

      indiClr(); //очистка индикаторов
#if LAMP_NUM > 4
      indiPrintNum(cur_mode + 1, 5); //режим
#endif
      switch (cur_mode) {
        case 0:
          if (!blink_data) indiPrintNum(cur_alarm, 0, 2, 0); //вывод номера будильника
          if (cur_alarm) indiPrintNum(alarm[ALARM_MODE], 3); //вывод режима
          break;
        case 1:
          indiPrintMenuData(blink_data, cur_indi, alarm[ALARM_HOURS], 0, alarm[ALARM_MINS], 2); //вывод часов/минут
          break;
        case 2:
        case 3:
          if (!blink_data || cur_mode != 1) indiPrintNum(alarm[ALARM_MODE], 0); //вывод режима
          if (alarm[ALARM_MODE] > 3) {
            if (!blink_data || cur_mode != 2 || cur_indi) indiPrintNum(cur_day, 2); //вывод дня недели
            if (!blink_data || cur_mode != 2 || !cur_indi) indiPrintNum((alarm[ALARM_DAYS] >> cur_day) & 0x01, 3); //вывод установки
          }
          break;
        case 4:
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
#if PLAYER_TYPE
          indiPrintMenuData(blink_data, cur_indi, alarm[ALARM_VOLUME], 0, alarm[ALARM_SOUND] + !alarm[ALARM_RADIO], 2); //вывод громкости мелодии/номера мелодии
#else
          if (alarm[ALARM_RADIO]) indiPrintMenuData(blink_data, cur_indi, alarm[ALARM_VOLUME], 0, alarm[ALARM_SOUND], 2); //вывод громкости радио/номера радиостанции
          else if (!blink_data) indiPrintNum(alarm[ALARM_SOUND] + 1, 2, 2, 0); //вывод номера мелодии
#endif
#else
#if PLAYER_TYPE
          indiPrintMenuData(blink_data, cur_indi, alarm[ALARM_VOLUME], 0, alarm[ALARM_SOUND] + 1, 2); //вывод громкости мелодии/номера мелодии
#else
          if (!blink_data) indiPrintNum(alarm[ALARM_SOUND] + 1, 2, 2, 0); //вывод номера мелодии
#endif
#endif
          break;
      }
#if BACKL_TYPE == 3
      switch (cur_mode) {
        case 0: wsBacklSetMultipleHue(0, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
        case 2: wsBacklSetMultipleHue(0, 1, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
        case 3: wsBacklSetMultipleHue((cur_indi) ? 3 : 2, 1, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
#if !PLAYER_TYPE
        case 4:
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
          if (alarm[ALARM_RADIO]) wsBacklSetMultipleHue(cur_indi * 2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); //подсветка активных разрядов
          else wsBacklSetMultipleHue(2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2);  //подсветка активных разрядов
#else
          wsBacklSetMultipleHue(2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2);  //подсветка активных разрядов
#endif
          break;
#endif
        default: wsBacklSetMultipleHue(cur_indi * 2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
      }
#endif
      blink_data = !blink_data; //мигание сигментами
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (cur_mode) {
          case 0: if (cur_alarm > (alarms.num > 0)) cur_alarm--; else cur_alarm = alarms.num; alarmReadBlock(cur_alarm, alarm); break; //будильник

          //настройка времени будильника
          case 1:
            switch (cur_indi) {
              case 0: if (alarm[ALARM_HOURS] > 0) alarm[ALARM_HOURS]--; else alarm[ALARM_HOURS] = 23; break; //часы
              case 1: if (alarm[ALARM_MINS] > 0) alarm[ALARM_MINS]--; else alarm[ALARM_MINS] = 59; break; //минуты
            }
            break;

          //настройка режима будильника
          case 2: if (alarm[ALARM_MODE] > 0) alarm[ALARM_MODE]--; else alarm[ALARM_MODE] = 4; break; //режим
          case 3:
            switch (cur_indi) {
              case 0: if (cur_day > 1) cur_day--; else cur_day = 7; break; //день недели
              case 1: alarm[ALARM_DAYS] &= ~(0x01 << cur_day); break; //установка
            }
            break;

          //настройка мелодии будильника
          case 4:
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] > 0) alarm[ALARM_VOLUME]--; else alarm[ALARM_VOLUME] = MAIN_MAX_VOL; //громкость
                if (alarm[ALARM_RADIO]) setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //устанавливаем громкость
                else playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
                break;
              case 1:
                if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = (alarm[ALARM_RADIO]) ? 9 : (PLAYER_ALARM_MAX - 1); //мелодия
                if (alarm[ALARM_RADIO]) setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
                break;
            }
#else
            if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = (alarm[ALARM_RADIO]) ? 9 : (SOUND_MAX(alarm_sound) - 1); //мелодия
            if (alarm[ALARM_RADIO]) setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
            else melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#else
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] > 0) alarm[ALARM_VOLUME]--; else alarm[ALARM_VOLUME] = MAIN_MAX_VOL; //громкость
                playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
                break;
              case 1:
                if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = PLAYER_ALARM_MAX - 1; //мелодия
                break;
            }
#else
            if (alarm[ALARM_SOUND] > 0) alarm[ALARM_SOUND]--; else alarm[ALARM_SOUND] = SOUND_MAX(alarm_sound) - 1; //мелодия
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#endif
            break;
        }
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (cur_mode) {
          case 0: if (cur_alarm < alarms.num) cur_alarm++; else cur_alarm = (alarms.num > 0); alarmReadBlock(cur_alarm, alarm); break; //будильник

          //настройка времени будильника
          case 1:
            switch (cur_indi) {
              case 0: if (alarm[ALARM_HOURS] < 23) alarm[ALARM_HOURS]++; else alarm[ALARM_HOURS] = 0; break; //часы
              case 1: if (alarm[ALARM_MINS] < 59) alarm[ALARM_MINS]++; else alarm[ALARM_MINS] = 0; break; //минуты
            }
            break;

          //настройка режима будильника
          case 2: if (alarm[ALARM_MODE] < 4) alarm[ALARM_MODE]++; else alarm[ALARM_MODE] = 0; break; //режим
          case 3:
            switch (cur_indi) {
              case 0: if (cur_day < 7) cur_day++; else cur_day = 1; break; //день недели
              case 1: alarm[ALARM_DAYS] |= (0x01 << cur_day); break; //установка
            }
            break;

          //настройка мелодии будильника
          case 4:
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] < MAIN_MAX_VOL) alarm[ALARM_VOLUME]++; else alarm[ALARM_VOLUME] = 0; //громкость
                if (alarm[ALARM_RADIO]) setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //устанавливаем громкость
                else playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
                break;
              case 1:
                if (alarm[ALARM_SOUND] < ((alarm[ALARM_RADIO]) ? 9 : (PLAYER_ALARM_MAX - 1))) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
                if (alarm[ALARM_RADIO]) setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
                break;
            }
#else
            if (alarm[ALARM_SOUND] < ((alarm[ALARM_RADIO]) ? 9 : (SOUND_MAX(alarm_sound) - 1))) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
            if (alarm[ALARM_RADIO]) setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
            else melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#else
#if PLAYER_TYPE
            switch (cur_indi) {
              case 0:
                if (alarm[ALARM_VOLUME] < MAIN_MAX_VOL) alarm[ALARM_VOLUME]++; else alarm[ALARM_VOLUME] = 0; //громкость
                playerSetVol((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
                break;
              case 1:
                if (alarm[ALARM_SOUND] < (PLAYER_ALARM_MAX - 1)) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
                break;
            }
#else
            if (alarm[ALARM_SOUND] < (SOUND_MAX(alarm_sound) - 1)) alarm[ALARM_SOUND]++; else alarm[ALARM_SOUND] = 0; //мелодия
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#endif
            break;
        }
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        if (!cur_mode && cur_alarm) {
          cur_mode = 1;
          cur_indi = 0;
          dotSetBright(dot.menuBright); //включаем точки
        }
        else cur_indi = !cur_indi;
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case LEFT_KEY_HOLD: //удержание левой кнопки
        if (!cur_mode) {
          if (cur_alarm) { //если есть будильники в памяти
            alarmRemove(cur_alarm); //удалить текущий будильник
            dotSetBright(dot.menuBright); //включаем точки
            for (_timer_ms[TMR_MS] = 500; _timer_ms[TMR_MS];) systemTask(); //обработка данных
            dotSetBright(0); //выключаем точки
            if (cur_alarm > (alarms.num > 0)) cur_alarm--; //убавляем номер текущего будильника
            else cur_alarm = (alarms.num > 0);
            alarmReadBlock(cur_alarm, alarm); //читаем блок данных
#if ESP_ENABLE
            device.status |= (0x01 << STATUS_UPDATE_ALARM_SET);
#endif
          }
        }
        else {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
          if ((cur_mode == 4) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
          if (cur_mode > 1) cur_mode--; else cur_mode = 4; //переключение пунктов

          if ((cur_mode == 3) && (alarm[ALARM_MODE] < 4)) cur_mode = 2; //если нет дней недели
          if (cur_mode == 4) { //если режим настройки мелодии
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
            if (alarm[ALARM_RADIO] && (getPowerStatusRDA() != RDA_ERROR)) { //если режим радиобудильника
              setPowerRDA(RDA_ON); //включаем радио
              setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //устанавливаем громкость
              setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
            }
            else { //иначе обычный режим
              alarm[ALARM_RADIO] = 0; //обычный режим
              setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
              playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
#else
              melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
            }
#else
#if PLAYER_TYPE
            playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
#else
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#endif
          }
          dotSetBright((cur_mode == 1) ? 0 : dot.menuBright); //включаем точки
        }

        cur_indi = 0;
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case RIGHT_KEY_HOLD: //удержание правой кнопки
        if (!cur_mode) {
          alarmCreate(); //создать новый будильник
          dotSetBright(dot.menuBright); //включаем точки
          for (_timer_ms[TMR_MS] = 500; _timer_ms[TMR_MS];) systemTask(); //обработка данных
          dotSetBright(0); //выключаем точки
          cur_alarm = alarms.num;
          alarmReadBlock(cur_alarm, alarm); //читаем блок данных
#if ESP_ENABLE
          device.status |= (0x01 << STATUS_UPDATE_ALARM_SET);
#endif
        }
        else {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
          if ((cur_mode == 4) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
          if (cur_mode < 4) cur_mode++; else cur_mode = 1; //переключение пунктов

          if ((cur_mode == 3) && (alarm[ALARM_MODE] < 4)) cur_mode = 4; //если нет дней недели
          if (cur_mode == 4) { //если режим настройки мелодии
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
            if (alarm[ALARM_RADIO] && (getPowerStatusRDA() != RDA_ERROR)) { //если режим радиобудильника
              setPowerRDA(RDA_ON); //включаем радио
              setVolumeRDA((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //устанавливаем громкость
              setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
            }
            else { //иначе обычный режим
              alarm[ALARM_RADIO] = 0; //обычный режим
              setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
              playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
#else
              melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
            }
#else
#if PLAYER_TYPE
            playerSetVolNow((alarm[ALARM_VOLUME]) ? alarm[ALARM_VOLUME] : ALARM_AUTO_VOL_MAX); //установка громкости
#else
            melodyPlay(alarm[ALARM_SOUND], SOUND_LINK(alarm_sound), REPLAY_CYCLE); //воспроизводим мелодию
#endif
#endif
          }
          dotSetBright((cur_mode == 1) ? 0 : dot.menuBright); //включаем точки
        }


        cur_indi = 0;
        blink_data = 0; //сбрасываем флаги
        _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        if (!cur_mode) return MAIN_PROGRAM; //выход
        else {
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
          if ((cur_mode == 4) && alarm[ALARM_RADIO]) radioPowerRet(); //вернуть питание радиоприемника
#endif
#if ESP_ENABLE
          device.status |= (0x01 << STATUS_UPDATE_ALARM_SET);
#endif
          alarm[ALARM_STATUS] = 255; //установили статус изменения будильника
          alarmWriteBlock(cur_alarm, alarm); //записать блок основных данных будильника
          dotSetBright(0); //выключаем точки
          cur_mode = 0; //выбор будильника
          blink_data = 0; //сбрасываем флаги
          _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        }
        break;

#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
      case ADD_KEY_HOLD: //удержание дополнительной кнопки
        if (cur_mode == 4) {
          if (alarm[ALARM_RADIO]) { //если режим радиобудильника
            alarm[ALARM_RADIO] = 0;
            alarm[ALARM_SOUND] = 0;
            setPowerRDA(RDA_OFF); //выключаем радио
#if PLAYER_TYPE
            playerSetVolNow(alarm[ALARM_VOLUME]); //установка громкости
#endif
          }
          else if (getPowerStatusRDA() != RDA_ERROR) { //иначе если радиоприемник доступен
            alarm[ALARM_RADIO] = 1;
            alarm[ALARM_SOUND] = 0;
            setPowerRDA(RDA_ON); //включаем радио
            setVolumeRDA(alarm[ALARM_VOLUME]); //устанавливаем громкость
            setFreqRDA(radioSettings.stationsSave[alarm[ALARM_SOUND]]); //устанавливаем частоту
          }
          blink_data = 0; //сбрасываем флаги
          _timer_ms[TMR_MS] = time_out = 0; //сбрасываем таймеры
        }
        break;
#endif
    }
  }
  return INIT_PROGRAM;
}
//-----------------------------Настроки основные------------------------------------
uint8_t settings_main(void) //настроки основные
{
  boolean set = 0; //режим настройки
  boolean cur_indi = 0; //текущий индикатор
  boolean blink_data = 0; //мигание сигментами
  uint8_t cur_mode = 0; //текущий режим
  uint8_t anim_demo = 0; //флаг демонстрации анимации
  uint8_t time_out = 0; //таймаут автовыхода

  _timer_ms[TMR_MS] = 0; //сбросили таймер

#if BACKL_TYPE == 3
  backlAnimDisable(); //запретили эффекты подсветки
  wsBacklSetLedBright(backl.menuBright); //установили максимальную яркость
#endif

#if PLAYER_TYPE
  if (mainSettings.baseSound) playerSetTrackNow(PLAYER_MAIN_MENU_START, PLAYER_MENU_FOLDER); //воспроизводим название меню
#endif

#if INDI_SYMB_TYPE
  indiSetSymb(SYMB_MENU); //установка индикатора символов
#endif

  while (1) {
    dataUpdate(); //обработка данных

#if ESP_ENABLE
    if (busCheck()) return MAIN_PROGRAM;
#endif

    if (!indi.update) { //если установлен флаг
      indi.update = 1; //сбрасываем флаг
      if (++time_out >= SETTINGS_TIMEOUT) {
        setUpdateMemory(CELL(MEM_UPDATE_MAIN_SET)); //записываем основные настройки в память
        animShow = ANIM_MAIN; //установили флаг анимации
        return MAIN_PROGRAM;
      }
    }

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

      indiClr(); //очистка индикаторов
      if (!set) {
        indiPrintNum(cur_mode + 1, (LAMP_NUM / 2 - 1), 2, 0); //вывод режима
#if BACKL_TYPE == 3
        wsBacklSetMultipleHue((LAMP_NUM / 2 - 1), 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); //подсветка активных разрядов
#endif
      }
      else {
        if (anim_demo == 1) { //если нужно отобразить демонстрацию эффекта
          anim_demo = 0; //сбросили флаг демонстрации
#if BACKL_TYPE == 3
          wsBacklSetLedHue(BACKL_MENU_COLOR_1, WHITE_ON); //подсветка активных разрядов
#endif
          switch (cur_mode) {
            case SET_AUTO_SHOW: animIndi(mainSettings.autoShowFlip, FLIP_DEMO); break; //демонстрация анимации показа температуры
            case SET_BURN_MODE:
              burnIndi(mainSettings.burnMode, BURN_DEMO); //демонстрация антиотравления индикаторов
              dotSetBright(dot.menuBright); //включаем точки
              break;
          }
          _timer_ms[TMR_MS] = blink_data = 0; //сбрасываем флаги
        }
        else {
          if (anim_demo > 1) { //если нужно отобразить анимацию
            anim_demo = 1; //перешли в режим анимации
            _timer_ms[TMR_MS] = SETTINGS_WAIT_TIME; //устанавливаем таймер
          }
#if LAMP_NUM > 4
          indiPrintNum(cur_mode + 1, 4, 2); //режим
#endif
          switch (cur_mode) {
#if PLAYER_TYPE
            case SET_TIME_FORMAT: //вывод формата времени
              indiPrintMenuData(blink_data, cur_indi, (mainSettings.timeFormat) ? 12 : 24, 0, mainSettings.glitchMode, 3); //вывод формата времени/режима глюков
              break;
            case SET_GLITCH_MODE: //вывод озвучки
              indiPrintMenuData(blink_data, cur_indi, mainSettings.volumeSound, 0, mainSettings.voiceSound, 3); //вывод громкости озвучки/голоса озвучки
              break;
            case SET_BTN_SOUND: //вывод озвучки
              indiPrintMenuData(blink_data, cur_indi, (mainSettings.hourSound & 0x03) + ((mainSettings.hourSound & 0x80) ? 10 : 0), 0, mainSettings.baseSound, 3); //вывод озвучки смены часа/действий
              break;
#else
            case SET_TIME_FORMAT: if (!blink_data) indiPrintNum((mainSettings.timeFormat) ? 12 : 24, 0); break; //вывод формата времени
            case SET_GLITCH_MODE: if (!blink_data) indiPrintNum(mainSettings.glitchMode, 3); break; //вывод глюков
            case SET_BTN_SOUND: if (!blink_data) indiPrintNum(mainSettings.baseSound, 3); break; //звук кнопок или озвучка
#endif
            case SET_HOUR_TIME:
              indiPrintMenuData(blink_data, cur_indi, mainSettings.timeHour[TIME_NIGHT], 0, mainSettings.timeHour[TIME_DAY], 2); //вывод часа начала звукового оповещения нового часа/окончания звукового оповещения нового часа
              break;
            case SET_BRIGHT_TIME:
              indiPrintMenuData(blink_data, cur_indi, mainSettings.timeBright[TIME_NIGHT], 0, mainSettings.timeBright[TIME_DAY], 2); //вывод часа начала ночной посветки/окончания ночной посветки
              break;
            case SET_INDI_BRIGHT:
              indiPrintMenuData(blink_data, cur_indi, mainSettings.indiBright[TIME_NIGHT], 0, mainSettings.indiBright[TIME_DAY], 2); //вывод яркости индикаторов ночь/день
              break;
#if BACKL_TYPE
            case SET_BACKL_BRIGHT:
              indiPrintMenuData(blink_data, cur_indi, mainSettings.backlBright[TIME_NIGHT] / 10, 0, mainSettings.backlBright[TIME_DAY] / 10, 2); //вывод яркости подсветки ночь/день
              break;
#endif
            case SET_DOT_BRIGHT:
#if ((SECS_DOT != 3) || !DOTS_PORT_ENABLE) && (SECS_DOT != 4)
              indiPrintMenuData(blink_data, cur_indi, mainSettings.dotBright[TIME_NIGHT] / 10, 0, mainSettings.dotBright[TIME_DAY] / 10, 2); //вывод яркости точек ночь/день
#else
              if (!blink_data) indiPrintNum((boolean)mainSettings.dotBright[TIME_NIGHT], 3); //вывод яркости ночь
#endif
              break;
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
            case SET_CORRECT_SENS: {
                if (!blink_data) {
#if ESP_ENABLE
                  uint16_t temperature = getTemperature((extendedSettings.tempCorrectSensor == 2) ? SHOW_TEMP_ESP : SHOW_TEMP); //буфер температуры
#else
                  uint16_t temperature = getTemperature(); //буфер температуры
#endif
                  if (temperature > 990) indiPrintNum(0, 0); //вывод ошибки
                  else indiPrintNum(temperature, 1, 2, 0); //вывод температуры
                }
              }
              break;
#endif
            case SET_AUTO_SHOW:
              indiPrintMenuData(blink_data, cur_indi, mainSettings.autoShowTime, 0, mainSettings.autoShowFlip, 2); //вывод времени автопоказа/анимации автопоказа
              break;
            case SET_BURN_MODE:
              indiPrintMenuData(blink_data, cur_indi, mainSettings.burnTime, 0, mainSettings.burnMode, 3); //вывод анимации антиотравления/анимации секунд
              break;
            case SET_SLEEP_TIME:
              indiPrintMenuData(blink_data, cur_indi, mainSettings.timeSleep[TIME_NIGHT], 0, mainSettings.timeSleep[TIME_DAY], 2); //вывод времени сна ночь/ночь
              break;
          }

#if BACKL_TYPE == 3
          switch (cur_mode) {
#if PLAYER_TYPE
            case SET_TIME_FORMAT:
            case SET_GLITCH_MODE:
            case SET_BTN_SOUND:
              wsBacklSetMultipleHue((cur_indi) ? 3 : 0, (cur_indi) ? 1 : 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
#endif
#if ((SECS_DOT == 3) && DOTS_PORT_ENABLE) || (SECS_DOT == 4)
            case SET_DOT_BRIGHT:
#endif
#if !PLAYER_TYPE
            case SET_GLITCH_MODE:
            case SET_BTN_SOUND:
#endif
#if ((SECS_DOT == 3) && DOTS_PORT_ENABLE) || (SECS_DOT == 4) || !PLAYER_TYPE
              wsBacklSetMultipleHue(3, 1, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
#endif
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
            case SET_CORRECT_SENS: wsBacklSetMultipleHue(0, 3, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
#endif
            case SET_BURN_MODE: wsBacklSetMultipleHue((cur_indi) ? 3 : 0, (cur_indi) ? 1 : 3, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
            default: wsBacklSetMultipleHue(cur_indi * 2, 2, BACKL_MENU_COLOR_1, BACKL_MENU_COLOR_2); break; //подсветка активных разрядов
          }
#endif
          blink_data = !blink_data; //мигание сигментами
        }
      }
    }

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (set) {
          case 0:
            if (cur_mode > 0) cur_mode--;
            else cur_mode = SET_MAX_ITEMS - 1;
#if PLAYER_TYPE
            if (mainSettings.baseSound) playerSetTrackNow(PLAYER_MAIN_MENU_START + cur_mode, PLAYER_MENU_FOLDER);
#endif
            break;
          case 1:
            switch (cur_mode) {
              case SET_TIME_FORMAT:
#if PLAYER_TYPE
                switch (cur_indi) {
                  case 0: mainSettings.timeFormat = 0; break; //формат времени
                  case 1: mainSettings.glitchMode = 0; break; //глюки
                }
#else
                mainSettings.timeFormat = 0; //формат времени
#endif
                break;
              case SET_GLITCH_MODE: //озвучка
#if PLAYER_TYPE
                switch (cur_indi) {
                  case 0: if (mainSettings.volumeSound > MAIN_MIN_VOL) mainSettings.volumeSound--; playerSetVolNow(mainSettings.volumeSound); playerSetTrackNow(PLAYER_TEST_VOL_SOUND, PLAYER_GENERAL_FOLDER); break; //установили громкость
                  case 1: //голос озвучки
                    if (mainSettings.voiceSound > 0) {
                      mainSettings.voiceSound--;
                      playerSetVoice(mainSettings.voiceSound);
                      playerSetTrackNow(PLAYER_VOICE_SOUND, PLAYER_GENERAL_FOLDER);
                    }
                    break;
                }
#else
                mainSettings.glitchMode = 0; //глюки
#endif
                break;

              case SET_BTN_SOUND: //звук кнопок
#if PLAYER_TYPE
                switch (cur_indi) {
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
                  case 0: if (mainSettings.hourSound & 0x80) mainSettings.hourSound &= ~0x80; else mainSettings.hourSound |= 0x80; break; //установили озвучку темепературы
#endif
                  case 1: mainSettings.baseSound = 0; break; //выключили озвучку действий
                }
#else
                if (!mainSettings.baseSound) buzzPulse(KNOCK_SOUND_FREQ, KNOCK_SOUND_TIME); //щелчок пищалкой
                if (mainSettings.baseSound > 0) mainSettings.baseSound--; //звук кнопок
#endif
                break;
              case SET_HOUR_TIME: //время звука смены часа
                switch (cur_indi) {
                  case 0: if (mainSettings.timeHour[TIME_NIGHT] > 0) mainSettings.timeHour[TIME_NIGHT]--; else mainSettings.timeHour[TIME_NIGHT] = 23; break;
                  case 1: if (mainSettings.timeHour[TIME_DAY] > 0) mainSettings.timeHour[TIME_DAY]--; else mainSettings.timeHour[TIME_DAY] = 23; break;
                }
                break;
              case SET_BRIGHT_TIME: //время смены подсветки
                switch (cur_indi) {
                  case 0: if (mainSettings.timeBright[TIME_NIGHT] > 0) mainSettings.timeBright[TIME_NIGHT]--; else mainSettings.timeBright[TIME_NIGHT] = 23; break;
                  case 1: if (mainSettings.timeBright[TIME_DAY] > 0) mainSettings.timeBright[TIME_DAY]--; else mainSettings.timeBright[TIME_DAY] = 23; break;
                }
                break;
              case SET_INDI_BRIGHT: //яркость индикаторов
                switch (cur_indi) {
                  case 0: if (mainSettings.indiBright[TIME_NIGHT] > INDI_MIN_BRIGHT) mainSettings.indiBright[TIME_NIGHT]--; break;
                  case 1: if (mainSettings.indiBright[TIME_DAY] > INDI_MIN_BRIGHT) mainSettings.indiBright[TIME_DAY]--; break;
                }
                indiSetBright(mainSettings.indiBright[cur_indi]); //установка общей яркости индикаторов
                break;
#if BACKL_TYPE
              case SET_BACKL_BRIGHT: //яркость подсветки
                switch (cur_indi) {
                  case 0: if (mainSettings.backlBright[TIME_NIGHT] > 0) mainSettings.backlBright[TIME_NIGHT] -= 10; break;
                  case 1: if (mainSettings.backlBright[TIME_DAY] > 10) mainSettings.backlBright[TIME_DAY] -= 10; break;
                }
#if BACKL_TYPE == 3
                wsBacklSetLedBright(mainSettings.backlBright[cur_indi]); //устанавливаем максимальную яркость
#else
                ledBacklSetBright(mainSettings.backlBright[cur_indi]); //если посветка статичная, устанавливаем яркость
#endif
                break;
#endif
              case SET_DOT_BRIGHT: //яркость точек
#if ((SECS_DOT != 3) || !DOTS_PORT_ENABLE) && (SECS_DOT != 4)
                switch (cur_indi) {
                  case 0: if (mainSettings.dotBright[TIME_NIGHT] > 0) mainSettings.dotBright[TIME_NIGHT] -= 10; break;
                  case 1: if (mainSettings.dotBright[TIME_DAY] > 10) mainSettings.dotBright[TIME_DAY] -= 10; break;
                }
                dotSetBright(mainSettings.dotBright[cur_indi]); //включаем точки
#else
                mainSettings.dotBright[TIME_NIGHT] = 0;
#endif
                break;
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
              case SET_CORRECT_SENS: //настройка коррекции температуры
                if (mainSettings.tempCorrect > -127) mainSettings.tempCorrect--; else mainSettings.tempCorrect = 127;
                break;
#endif
              case SET_AUTO_SHOW: //автопоказ температуры
                switch (cur_indi) {
                  case 0:
                    if (mainSettings.autoShowTime > 0) mainSettings.autoShowTime--; else mainSettings.autoShowTime = 15;
                    _timer_sec[TMR_SHOW] = getPhaseTime(mainSettings.autoShowTime, AUTO_SHOW_PHASE); //установка таймера показа температуры
                    break;
                  case 1:
                    if (mainSettings.autoShowFlip > 0) mainSettings.autoShowFlip--; else mainSettings.autoShowFlip = (FLIP_EFFECT_NUM + 1); //устанавливаем анимацию автопоказа температуры
                    if (mainSettings.autoShowFlip > 1) anim_demo = 2; //установили флаг демонстрации анимации
                    break;
                }
                break;
              case SET_BURN_MODE: //анимация антиотравления индикаторов
                switch (cur_indi) {
                  case 0:
                    if (mainSettings.burnTime > 10) mainSettings.burnTime -= 5; else mainSettings.burnTime = 180;
                    _timer_sec[TMR_BURN] = getPhaseTime(mainSettings.burnTime, BURN_PHASE); //установка таймера антиотравления
                    break;
                  case 1:
                    if (mainSettings.burnMode) mainSettings.burnMode--; else mainSettings.burnMode = (BURN_EFFECT_NUM - 1);
                    anim_demo = 2; //установили флаг демонстрации анимации
                    break;
                }
                break;
              case SET_SLEEP_TIME: //время ухода в сон
                switch (cur_indi) {
                  case 0: if (mainSettings.timeSleep[TIME_NIGHT] > 0) mainSettings.timeSleep[TIME_NIGHT] -= 5; else mainSettings.timeSleep[TIME_NIGHT] = 30; break;
                  case 1: if (mainSettings.timeSleep[TIME_DAY] > 0) mainSettings.timeSleep[TIME_DAY] -= 15; else mainSettings.timeSleep[TIME_DAY] = 90; break;
                }
                break;
            }
            break;
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (set) {
          case 0:
            if (cur_mode < (SET_MAX_ITEMS - 1)) cur_mode++;
            else cur_mode = 0;
#if PLAYER_TYPE
            if (mainSettings.baseSound) playerSetTrackNow(PLAYER_MAIN_MENU_START + cur_mode, PLAYER_MENU_FOLDER);
#endif
            break;
          case 1:
            switch (cur_mode) {
              case SET_TIME_FORMAT:
#if PLAYER_TYPE
                switch (cur_indi) {
                  case 0: mainSettings.timeFormat = 1; break; //формат времени
                  case 1: mainSettings.glitchMode = 1; break; //глюки
                }
#else
                mainSettings.timeFormat = 1; //формат времени
#endif
                break;
              case SET_GLITCH_MODE: //глюки
#if PLAYER_TYPE
                switch (cur_indi) {
                  case 0: if (mainSettings.volumeSound < MAIN_MAX_VOL) mainSettings.volumeSound++; playerSetVolNow(mainSettings.volumeSound); playerSetTrackNow(PLAYER_TEST_VOL_SOUND, PLAYER_GENERAL_FOLDER); break; //установили громкость
                  case 1: //голос озвучки
                    if (mainSettings.voiceSound < (PLAYER_VOICE_MAX - 1)) {
                      mainSettings.voiceSound++;
                      playerSetVoice(mainSettings.voiceSound);
                      playerSetTrackNow(PLAYER_VOICE_SOUND, PLAYER_GENERAL_FOLDER);
                    }
                    break;
                }
#else
                mainSettings.glitchMode = 1; //глюки
#endif
                break;
              case SET_BTN_SOUND: //звук кнопок
#if PLAYER_TYPE
                switch (cur_indi) {
                  case 0: if ((mainSettings.hourSound & 0x7F) < 3) mainSettings.hourSound++; else mainSettings.hourSound = 0; break; //установили тип озвучки часа
                  case 1: mainSettings.baseSound = 1; break; //включили озвучку действий
                }
#else
                if (!mainSettings.baseSound) buzzPulse(KNOCK_SOUND_FREQ, KNOCK_SOUND_TIME); //щелчок пищалкой
                if (mainSettings.baseSound < 2) mainSettings.baseSound++; //звук кнопок
#endif
                break;
              case SET_HOUR_TIME: //время звука смены часа
                switch (cur_indi) {
                  case 0: if (mainSettings.timeHour[TIME_NIGHT] < 23) mainSettings.timeHour[TIME_NIGHT]++; else mainSettings.timeHour[TIME_NIGHT] = 0; break;
                  case 1: if (mainSettings.timeHour[TIME_DAY] < 23) mainSettings.timeHour[TIME_DAY]++; else mainSettings.timeHour[TIME_DAY] = 0; break;
                }
                break;
              case SET_BRIGHT_TIME: //время смены подсветки
                switch (cur_indi) {
                  case 0: if (mainSettings.timeBright[TIME_NIGHT] < 23) mainSettings.timeBright[TIME_NIGHT]++; else mainSettings.timeBright[TIME_NIGHT] = 0; break;
                  case 1: if (mainSettings.timeBright[TIME_DAY] < 23) mainSettings.timeBright[TIME_DAY]++; else mainSettings.timeBright[TIME_DAY] = 0; break;
                }
                break;
              case SET_INDI_BRIGHT: //яркость индикаторов
                switch (cur_indi) {
                  case 0: if (mainSettings.indiBright[TIME_NIGHT] < 30) mainSettings.indiBright[TIME_NIGHT]++; break;
                  case 1: if (mainSettings.indiBright[TIME_DAY] < 30) mainSettings.indiBright[TIME_DAY]++; break;
                }
                indiSetBright(mainSettings.indiBright[cur_indi]); //установка общей яркости индикаторов
                break;
#if BACKL_TYPE
              case SET_BACKL_BRIGHT: //яркость подсветки
                switch (cur_indi) {
                  case 0: if (mainSettings.backlBright[TIME_NIGHT] < 250) mainSettings.backlBright[TIME_NIGHT] += 10; break;
                  case 1: if (mainSettings.backlBright[TIME_DAY] < 250) mainSettings.backlBright[TIME_DAY] += 10; break;
                }
#if BACKL_TYPE == 3
                wsBacklSetLedBright(mainSettings.backlBright[cur_indi]); //устанавливаем максимальную яркость
#else
                ledBacklSetBright(mainSettings.backlBright[cur_indi]); //если посветка статичная, устанавливаем яркость
#endif
                break;
#endif
              case SET_DOT_BRIGHT: //яркость точек
#if ((SECS_DOT != 3) || !DOTS_PORT_ENABLE) && (SECS_DOT != 4)
                switch (cur_indi) {
                  case 0: if (mainSettings.dotBright[TIME_NIGHT] < 250) mainSettings.dotBright[TIME_NIGHT] += 10; break;
                  case 1: if (mainSettings.dotBright[TIME_DAY] < 250) mainSettings.dotBright[TIME_DAY] += 10; break;
                }
                dotSetBright(mainSettings.dotBright[cur_indi]); //включаем точки
#else
                mainSettings.dotBright[TIME_NIGHT] = 1;
#endif
                break;
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
              case SET_CORRECT_SENS: //настройка коррекции температуры
                if (mainSettings.tempCorrect < 127) mainSettings.tempCorrect++; else mainSettings.tempCorrect = -127;
                break;
#endif
              case SET_AUTO_SHOW: //автопоказ
                switch (cur_indi) {
                  case 0:
                    if (mainSettings.autoShowTime < 15) mainSettings.autoShowTime++; else mainSettings.autoShowTime = 0;
                    _timer_sec[TMR_SHOW] = getPhaseTime(mainSettings.autoShowTime, AUTO_SHOW_PHASE); //установка таймера показа температуры
                    break;
                  case 1:
                    if (mainSettings.autoShowFlip < (FLIP_EFFECT_NUM + 1)) mainSettings.autoShowFlip++; else mainSettings.autoShowFlip = 0; //устанавливаем анимацию автопоказа температуры
                    if (mainSettings.autoShowFlip > 1) anim_demo = 2; //установили флаг демонстрации анимации
                    break;
                }
                break;
              case SET_BURN_MODE: //анимация антиотравления индикаторов
                switch (cur_indi) {
                  case 0:
                    if (mainSettings.burnTime < 180) mainSettings.burnTime += 5; else mainSettings.burnTime = 10;
                    _timer_sec[TMR_BURN] = getPhaseTime(mainSettings.burnTime, BURN_PHASE); //установка таймера антиотравления
                    break;
                  case 1:
                    if (mainSettings.burnMode < (BURN_EFFECT_NUM - 1)) mainSettings.burnMode++; else mainSettings.burnMode = 0;
                    anim_demo = 2; //установили флаг демонстрации анимации
                    break;
                }
                break;
              case SET_SLEEP_TIME: //время ухода в сон
                switch (cur_indi) {
                  case 0: if (mainSettings.timeSleep[TIME_NIGHT] < 30) mainSettings.timeSleep[TIME_NIGHT] += 5; else mainSettings.timeSleep[TIME_NIGHT] = 0; break;
                  case 1: if (mainSettings.timeSleep[TIME_DAY] < 90) mainSettings.timeSleep[TIME_DAY] += 15; else mainSettings.timeSleep[TIME_DAY] = 0; break;
                }
                break;
            }
            break;
        }
        _timer_ms[TMR_MS] = time_out = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_PRESS: //клик средней кнопкой
        set = !set;
        if (set) {
          switch (cur_mode) {
            case SET_INDI_BRIGHT: indiSetBright(mainSettings.indiBright[TIME_NIGHT]); break; //установка общей яркости индикаторов
            case SET_BACKL_BRIGHT: //яркость подсветки
#if BACKL_TYPE == 3
              wsBacklSetLedBright(mainSettings.backlBright[TIME_NIGHT]); //устанавливаем максимальную яркость
#elif BACKL_TYPE
              backlAnimDisable(); //запретили эффекты подсветки
              ledBacklSetBright(mainSettings.backlBright[TIME_NIGHT]); //если посветка статичная, устанавливаем яркость
#else
              set = 0; //заблокировали пункт меню
#endif
              break;
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
#if ((SECS_DOT != 3) && DOTS_PORT_ENABLE) || ESP_ENABLE
            case SET_CORRECT_SENS: //настройка коррекции температуры
#if ESP_ENABLE
              if (!extendedSettings.tempCorrectSensor) set = 0; //заблокировали пункт меню
#if (SECS_DOT != 3) && DOTS_PORT_ENABLE
              else
#endif
#endif
#if (SECS_DOT != 3) && DOTS_PORT_ENABLE
#if (DOTS_TYPE == 1) || ((DOTS_DIV == 1) && (DOTS_TYPE == 2))
                indiSetDotR(1); //включаем разделительную точку
#else
                indiSetDotL(2); //включаем разделительную точку
#endif
#endif
              break;
#endif
#endif
          }
          if (set) {
#if PLAYER_TYPE
            if (mainSettings.baseSound) playerSetTrackNow((PLAYER_MAIN_MENU_OTHER + TIME_NIGHT) + (cur_mode * 2), PLAYER_MENU_FOLDER);
#endif
            changeBrightDisable(CHANGE_DISABLE); //запретить смену яркости
            dotSetBright((cur_mode != SET_DOT_BRIGHT) ? dot.menuBright : mainSettings.dotBright[TIME_NIGHT]); //включаем точки
          }
        }
        else {
#if BACKL_TYPE == 3
          wsBacklSetLedBright(backl.menuBright); //устанавливаем максимальную яркость
#elif BACKL_TYPE
          backlAnimEnable(); //разрешили эффекты подсветки
#endif
          changeBrightEnable(); //разрешить смену яркости
          changeBright(); //установка яркости от времени суток
          dotSetBright(0); //выключаем точки
#if (SECS_DOT != 3) && DOTS_PORT_ENABLE
          indiClrDots(); //выключаем разделительные точки
#endif
        }
        cur_indi = TIME_NIGHT;
        _timer_ms[TMR_MS] = time_out = anim_demo = blink_data = 0; //сбрасываем флаги
        break;

      case LEFT_KEY_HOLD: //удержание левой кнопки
        if (set) {
          cur_indi = TIME_NIGHT;
          switch (cur_mode) {
            case SET_INDI_BRIGHT: indiSetBright(mainSettings.indiBright[TIME_NIGHT]); break; //установка ночной яркости индикаторов
            case SET_BACKL_BRIGHT: //яркость подсветки
#if BACKL_TYPE == 3
              wsBacklSetLedBright(mainSettings.backlBright[TIME_NIGHT]); //установка ночной яркости подсветки
#elif BACKL_TYPE
              ledBacklSetBright(mainSettings.backlBright[TIME_NIGHT]); //установка ночной яркости подсветки
#endif
              break;
            case SET_DOT_BRIGHT: dotSetBright(mainSettings.dotBright[TIME_NIGHT]); break; //установка ночной яркости точек
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
            case SET_CORRECT_SENS: mainSettings.tempCorrect = 0; break; //сброс коррекции температуры
#endif
          }
#if PLAYER_TYPE
          if (mainSettings.baseSound) playerSetTrackNow((PLAYER_MAIN_MENU_OTHER + TIME_NIGHT) + (cur_mode * 2), PLAYER_MENU_FOLDER);
#endif
        }
        _timer_ms[TMR_MS] = time_out = anim_demo = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_HOLD: //удержание правой кнопки
        if (set) {
          cur_indi = TIME_DAY;
          switch (cur_mode) {
#if !PLAYER_TYPE && (BACKL_TYPE == 3)
            case SET_TIME_FORMAT: cur_indi = TIME_NIGHT; break; //возврат к формату времени
#endif
            case SET_INDI_BRIGHT: indiSetBright(mainSettings.indiBright[TIME_DAY]); break; //установка дневной яркости индикаторов
            case SET_BACKL_BRIGHT: //яркость подсветки
#if BACKL_TYPE == 3
              wsBacklSetLedBright(mainSettings.backlBright[TIME_DAY]); //установка дневной яркости подсветки
#elif BACKL_TYPE
              ledBacklSetBright(mainSettings.backlBright[TIME_DAY]); //установка дневной яркости подсветки
#endif
              break;
            case SET_DOT_BRIGHT: dotSetBright(mainSettings.dotBright[TIME_DAY]); break; //установка дневной яркости точек
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
            case SET_CORRECT_SENS: mainSettings.tempCorrect = 0; break; //сброс коррекции температуры
#endif
          }
#if PLAYER_TYPE
          if (mainSettings.baseSound) playerSetTrackNow((PLAYER_MAIN_MENU_OTHER + cur_indi) + (cur_mode * 2), PLAYER_MENU_FOLDER);
#endif
        }
        _timer_ms[TMR_MS] = time_out = anim_demo = blink_data = 0; //сбрасываем флаги
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        setUpdateMemory(CELL(MEM_UPDATE_MAIN_SET)); //записываем основные настройки в память
        return MAIN_PROGRAM;
    }
  }
  return INIT_PROGRAM;
}
//-------------------------Включить питание радиоприемника------------------------------
void radioPowerOn(void) //включить питание радиоприемника
{
  setPowerRDA(RDA_ON); //включаем радио
  setVolumeRDA(radioSettings.volume); //устанавливаем громкость
  setFreqRDA(radioSettings.stationsFreq); //устанавливаем частоту
}
//------------------------Выключить питание радиоприемника------------------------------
void radioPowerOff(void) //выключить питание радиоприемника
{
  if (getPowerStatusRDA() == RDA_ON) { //если радио включено
    setPowerRDA(RDA_OFF); //выключаем радио
  }
}
//-----------------------------Включить радиоприемник-----------------------------------
void radioStartup(void) //включить радиоприемник
{
  radio.powerState = RDA_ON; //установили флаг питания радио
  radioPowerOn(); //включить питание радиоприемника
}
//-----------------------------Выключить радиоприемник----------------------------------
void radioShutdown(void) //выключить радиоприемник
{
  radio.powerState = RDA_OFF; //сбросили флаг питания радио
  radioPowerOff(); //выключить питание радиоприемника
}
//--------------------------Вернуть питание радиоприемника------------------------------
void radioPowerRet(void) //вернуть питание радиоприемника
{
  if (radio.powerState == RDA_ON) { //если радио было включено
    radioPowerOn(); //включить питание радиоприемника
  }
  else radioPowerOff(); // иначе выключить питание радиоприемника
}
//------------------------Переключить питание радиоприемника-----------------------------
void radioPowerSwitch(void) //переключить питание радиоприемника
{
  if (getPowerStatusRDA() != RDA_ERROR) { //если радиоприемник доступен
    if (getPowerStatusRDA() == RDA_OFF) radioStartup(); //включить радиоприемник
    else radioShutdown(); //выключить радиоприемник
  }
}
//--------------------------Поиск радиостанции в памяти---------------------------------
void radioSearchStation(void) //поиск радиостанции в памяти
{
  setUpdateMemory(CELL(MEM_UPDATE_RADIO_SET)); //записываем настройки радио в память
  for (uint8_t i = 0; i < RADIO_MAX_STATIONS; i++) { //ищем среди всех ячеек
    if (radioSettings.stationsSave[i] == radioSettings.stationsFreq) { //если частота совпадает с радиостанцией
      radioSettings.stationNum = i; //установили номер радиостанции
      return; //выходим
    }
  }
  radioSettings.stationNum |= 0x80; //установили номер ячейки за пределом видимости
}
//-----------------------Переключить радиостанцию в памяти------------------------------
void radioSwitchStation(boolean _sta) //переключить радиостанцию в памяти
{
  setUpdateMemory(CELL(MEM_UPDATE_RADIO_SET)); //записываем настройки радио в память
  if (radioSettings.stationNum & 0x80) { //если установлен флаг ячейки
    radioSettings.stationNum &= 0x7F; //сбросили флаг
    radioSettings.stationsFreq = radioSettings.stationsSave[radioSettings.stationNum]; //прочитали частоту
    setFreqRDA(radioSettings.stationsFreq); //установили частоту
    return; //выходим
  }
  for (uint8_t i = 0; i < RADIO_MAX_STATIONS; i++) { //ищем среди всех ячеек
    if (_sta) { //ищем вперед
      if (radioSettings.stationNum < (RADIO_MAX_STATIONS - 1)) radioSettings.stationNum++; else radioSettings.stationNum = 0; //переключаем станцию
    }
    else { //ищем назад
      if (radioSettings.stationNum > 0) radioSettings.stationNum--; else radioSettings.stationNum = (RADIO_MAX_STATIONS - 1); //переключаем станцию
    }
    if (radioSettings.stationsSave[radioSettings.stationNum]) { //если в памяти записана частота
      radioSettings.stationsFreq = radioSettings.stationsSave[radioSettings.stationNum]; //прочитали частоту
      setFreqRDA(radioSettings.stationsFreq); //установили частоту
      return; //выходим
    }
  }
}
//------------------------Остановка автопоиска радиостанции-----------------------------
void radioSeekStop(void) //остановка автопоиска радиостанции
{
  if (radio.seekRun) { //если идет поиск
    radio.seekRun = 0; //выключили поиск
    stopSeekRDA(); //остановили поиск радио
    clrSeekCompleteStatusRDA(); //очищаем флаг окончания поиска
    setFreqRDA(radioSettings.stationsFreq); //устанавливаем частоту
    setMuteRDA(RDA_MUTE_OFF); //выключаем приглушение звука
    radioSearchStation(); //поиск радиостанции в памяти
  }
}
//------------------------Автопоиск радиостанций вверх-----------------------------
void radioSeekUp(void) //автопоиск радиостанций
{
  if (radioSettings.stationsFreq < RADIO_MAX_FREQ) { //если не достигли предела поиска
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
    radio.seekAnim = 0; //сбросили анимацию поиска
#endif
    radio.seekRun = 2; //включили поиск
    radio.seekFreq = RADIO_MAX_FREQ; //установили максимальную частоту
    radioSettings.stationNum |= 0x80; //установили номер ячейки за пределом видимости
    setMuteRDA(RDA_MUTE_ON); //включаем приглушение звука
    startSeekRDA(RDA_SEEK_UP); //начинаем поиск вверх
    dotSetBright(0); //выключаем точки
#if (SECS_DOT != 3) && DOTS_PORT_ENABLE
    indiClrDots(); //очистка разделителных точек
#endif
  }
}
//------------------------Автопоиск радиостанций вниз-----------------------------
void radioSeekDown(void) //автопоиск радиостанций
{
  if (radioSettings.stationsFreq > RADIO_MIN_FREQ) { //если не достигли предела поиска
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
    radio.seekAnim = ((LAMP_NUM + 1) * 2); //сбросили анимацию поиска
#endif
    radio.seekRun = 1; //включили поиск
    radio.seekFreq = RADIO_MIN_FREQ; //установили минимальную частоту
    radioSettings.stationNum |= 0x80; //установили номер ячейки за пределом видимости
    setMuteRDA(RDA_MUTE_ON); //включаем приглушение звука
    startSeekRDA(RDA_SEEK_DOWN); //начинаем поиск вниз
    dotSetBright(0); //выключаем точки
#if (SECS_DOT != 3) && DOTS_PORT_ENABLE
    indiClrDots(); //очистка разделителных точек
#endif
  }
}
//-----------------------------Быстрые настройки радио-----------------------------------
uint8_t radioFastSettings(void) //быстрые настройки радио
{
  if (btn.state) { //если радио включено и нажата кнопка
    uint8_t _state = btn.state; //буфер кнопки

#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
    if (_state == PWR_KEY_PRESS) { //управление питанием
      radioPowerSwitch(); //переключить питание радиоприемника
      buttonState(); //сбросили состояние кнопки
      return 2; //выходим
    }
#endif
    if (radio.powerState) { //если питание радио включено
#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
      if (_state <= ADD_KEY_HOLD) { //если нажата кнопка из стандартного набора
        if (mainTask == RADIO_PROGRAM) { //если текущая подпрограмма радио
#endif
          if (_state == SET_KEY_PRESS) _state = KEY_MAX_ITEMS; //установили режим просмотра громкости
          else if (_state != ADD_KEY_PRESS) return 0; //иначе выходим
#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
        }
        else return 0; //иначе выходим
      }
      else if (mainTask != RADIO_PROGRAM) { //если текущая подпрограмма не радио
        mainTask = RADIO_PROGRAM; //подмена текущей программы
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
        backlAnimDisable(); //запретили эффекты подсветки
#if RADIO_BACKL_TYPE == 1
        changeBrightDisable(CHANGE_STATIC_BACKL); //разрешить смену яркости статичной подсветки
        wsBacklSetLedBright((fastSettings.backlMode & 0x7F) ? backl.maxBright : 0); //установили яркость в зависимости от режима подсветки
#else
        wsBacklSetLedBright(backl.menuBright); //установили максимальную яркость
#endif
#endif
      }
#endif

#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
      wsBacklSetMultipleHue(((LAMP_NUM / 2) - 1), 2, RADIO_BACKL_COLOR_1, RADIO_BACKL_COLOR_2);
#endif

      dotSetBright(0); //выключаем точки
#if (SECS_DOT != 3) && DOTS_PORT_ENABLE
      indiClrDots(); //очистка разделителных точек
#endif

#if INDI_SYMB_TYPE
      indiSetSymb(SYMB_MENU); //установка индикатора символов
#endif

      buttonState(); //сбросили состояние кнопки
      radioSeekStop(); //остановка автопоиска радиостанции

      while (1) {
        dataUpdate(); //обработка данных

        switch (_state) {
          case KEY_NULL:
          case KEY_MAX_ITEMS:
            break;
          case SET_KEY_PRESS: return 2; //выходим
          case RIGHT_KEY_PRESS: //прибавить громкость
#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
          case VOL_UP_KEY_PRESS: //прибавить громкость
#endif
            if (radioSettings.volume < MAIN_MAX_VOL) {
              setUpdateMemory(CELL(MEM_UPDATE_RADIO_SET));
              setVolumeRDA(++radioSettings.volume); //прибавитиь громкость
            }
            break;
          case LEFT_KEY_PRESS: //убавить громкость
#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
          case VOL_DOWN_KEY_PRESS: //убавить громкость
#endif
            if (radioSettings.volume > MAIN_MIN_VOL) {
              setUpdateMemory(CELL(MEM_UPDATE_RADIO_SET));
              setVolumeRDA(--radioSettings.volume); //убавить громкость
            }
            break;
          case ADD_KEY_PRESS: //следующая станция
#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
          case STATION_UP_KEY_PRESS: //следующая станция
#endif
            radioSwitchStation(1); //переключить радиостанцию в памяти
            break;
#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
          case STATION_DOWN_KEY_PRESS: //предыдущая станция
            radioSwitchStation(0); //переключить радиостанцию в памяти
            break;
#if IR_EXT_BTN_ENABLE == 2
          case STATION_CELL_0_PRESS: //клик кнопки станции 0
          case STATION_CELL_1_PRESS: //клик кнопки станции 1
          case STATION_CELL_2_PRESS: //клик кнопки станции 2
          case STATION_CELL_3_PRESS: //клик кнопки станции 3
          case STATION_CELL_4_PRESS: //клик кнопки станции 4
          case STATION_CELL_5_PRESS: //клик кнопки станции 5
          case STATION_CELL_6_PRESS: //клик кнопки станции 6
          case STATION_CELL_7_PRESS: //клик кнопки станции 7
          case STATION_CELL_8_PRESS: //клик кнопки станции 8
          case STATION_CELL_9_PRESS: //клик кнопки станции 9
            if (radioSettings.stationsSave[_state - STATION_CELL_0_PRESS]) { //если в памяти записана частота
              radioSettings.stationNum = _state - STATION_CELL_0_PRESS; //установили номер ячейки
              radioSettings.stationsFreq = radioSettings.stationsSave[radioSettings.stationNum]; //прочитали частоту
              setFreqRDA(radioSettings.stationsFreq); //установили частоту
              setUpdateMemory(CELL(MEM_UPDATE_RADIO_SET));
            }
            break;
#endif
#endif
          default: return 1; //выходим
        }

        switch (_state) {
          case RIGHT_KEY_PRESS:
          case LEFT_KEY_PRESS:
          case KEY_MAX_ITEMS:
#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
          case VOL_UP_KEY_PRESS:
          case VOL_DOWN_KEY_PRESS:
            _timer_ms[TMR_MS] = ((_state == VOL_UP_KEY_PRESS) || (_state == VOL_DOWN_KEY_PRESS)) ? RADIO_FAST_TIME : RADIO_VOL_TIME; //устанавливаем таймер
#else
            _timer_ms[TMR_MS] = RADIO_VOL_TIME; //устанавливаем таймер
#endif
            indiClr(); //очистка индикаторов
            indiPrintNum(radioSettings.volume, ((LAMP_NUM / 2) - 1), 2, 0); //вывод настройки
            break;
          case ADD_KEY_PRESS:
#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
          case STATION_UP_KEY_PRESS:
          case STATION_DOWN_KEY_PRESS:
#endif
            _timer_ms[TMR_MS] = RADIO_FAST_TIME; //устанавливаем таймер
            indiClr(); //очистка индикаторов
            indiPrintNum(radioSettings.stationNum, ((LAMP_NUM / 2) - 1), 2, 0); //вывод настройки
            break;
#if RADIO_ENABLE && IR_PORT_ENABLE && (IR_EXT_BTN_ENABLE == 2)
          case STATION_CELL_0_PRESS: //клик кнопки станции 0
          case STATION_CELL_1_PRESS: //клик кнопки станции 1
          case STATION_CELL_2_PRESS: //клик кнопки станции 2
          case STATION_CELL_3_PRESS: //клик кнопки станции 3
          case STATION_CELL_4_PRESS: //клик кнопки станции 4
          case STATION_CELL_5_PRESS: //клик кнопки станции 5
          case STATION_CELL_6_PRESS: //клик кнопки станции 6
          case STATION_CELL_7_PRESS: //клик кнопки станции 7
          case STATION_CELL_8_PRESS: //клик кнопки станции 8
          case STATION_CELL_9_PRESS: //клик кнопки станции 9
            _timer_ms[TMR_MS] = RADIO_FAST_TIME; //устанавливаем таймер
            indiClr(); //очистка индикаторов
            indiPrintNum(_state - STATION_CELL_0_PRESS, ((LAMP_NUM / 2) - 1), 2, 0); //вывод настройки
            break;
#endif
        }

        _state = buttonState(); //прочитали кнопку

        if (!_timer_ms[TMR_MS]) return 1; //выходим
      }
    }
  }
  return 0;
}
//------------------------------Меню настроек радио-------------------------------------
boolean radioMenuSettings(void) //меню настроек радио
{
  boolean _state = 0; //флаг бездействия
  uint8_t _station = radioSettings.stationNum & 0x7F; //текущий номер радиостанции
  _timer_ms[TMR_MS] = 0; //сбросили таймер

  dotSetBright(0); //выключаем точки
#if (SECS_DOT != 3) && DOTS_PORT_ENABLE
  indiClrDots(); //очистка разделителных точек
#endif

#if INDI_SYMB_TYPE
  indiSetSymb(SYMB_MENU); //установка индикатора символов
#endif

  while (1) {
    dataUpdate(); //обработка данных

    if (!_timer_ms[TMR_MS]) { //если таймер истек
      indiClr(); //очистка индикаторов
      _timer_ms[TMR_MS] = RADIO_STATION_TIME; //устанавливаем таймер
      if (_state) return 1; //выходим по бездействию
      indiPrintNum((boolean)radioSettings.stationsSave[_station], ((LAMP_NUM / 2) - 2)); //вывод настройки
      indiPrintNum(_station, (LAMP_NUM / 2), 2, 0); //вывод настройки
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
      wsBacklSetMultipleHue((LAMP_NUM / 2), 2, RADIO_BACKL_COLOR_1, RADIO_BACKL_COLOR_2);
      wsBacklSetLedHue(((LAMP_NUM / 2) - 2), RADIO_BACKL_COLOR_1, WHITE_ON);
#endif
      _state = 1; //установили флаг бездействия
    }

    switch (buttonState()) {
      case RIGHT_KEY_PRESS: //клик правой кнопкой
        if (_station < (RADIO_MAX_STATIONS - 1)) _station++; else _station = 0;
        _state = 0;
        _timer_ms[TMR_MS] = 0; //сбросили таймер
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
        if (_station > 0) _station--; else _station = (RADIO_MAX_STATIONS - 1);
        _state = 0;
        _timer_ms[TMR_MS] = 0; //сбросили таймер
        break;

      case ADD_KEY_PRESS: //клик дополнительной кнопкой
        radioSettings.stationsSave[_station] = radioSettings.stationsFreq; //сохранили радиостанцию
        radioSettings.stationNum = _station; //установили номер радиостанции
        setUpdateMemory(CELL(MEM_UPDATE_RADIO_SET));
        return 0; //выходим

      case SET_KEY_PRESS: //клик средней кнопкой
        return 1; //выходим

      case ADD_KEY_HOLD: //удержание дополнительной кнопкой
        radioSettings.stationsSave[_station] = 0; //сбросили радиостанцию
        setUpdateMemory(CELL(MEM_UPDATE_RADIO_SET));
        _state = 0;
        _timer_ms[TMR_MS] = 0; //сбросили таймер
        break;
    }
  }
}
//---------------------------------Радиоприемник----------------------------------------
uint8_t radioMenu(void) //радиоприемник
{
  if (getPowerStatusRDA() != RDA_ERROR) { //если радиоприемник доступен
    uint8_t time_out = 0; //таймаут автовыхода

#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
    backlAnimDisable(); //запретили эффекты подсветки
#if RADIO_BACKL_TYPE == 1
    changeBrightDisable(CHANGE_STATIC_BACKL); //разрешить смену яркости статичной подсветки
    wsBacklSetLedBright((fastSettings.backlMode & 0x7F) ? backl.maxBright : 0); //установили яркость в зависимости от режима подсветки
#else
    wsBacklSetLedBright(backl.menuBright); //установили максимальную яркость
#endif
#endif

    if (getPowerStatusRDA() == RDA_OFF) { //если радио выключено
#if PLAYER_TYPE
      if (mainSettings.baseSound) playerSetTrackNow(PLAYER_RADIO_SOUND, PLAYER_GENERAL_FOLDER);
      playerSetMute(PLAYER_MUTE_ON); //включаем приглушение звука плеера
      radio.powerState = RDA_OFF; //сбросили флаг питания радио
#else
      radioStartup(); //включить радиоприемник
#endif
    }

    radioSearchStation(); //поиск радиостанции в памяти

    _timer_ms[TMR_MS] = 0; //сбросили таймер

    while (1) {
      dataUpdate(); //обработка данных

#if ESP_ENABLE
      if (busCheck() & ~(0x01 << BUS_COMMAND_WAIT)) { //обновились настройки
        radioSeekStop(); //остановка автопоиска радиостанции
        return RADIO_PROGRAM;
      }
#endif

      if (!indi.update) { //если прошла секунда
        indi.update = 1; //сбросили флаг секунды

#if ALARM_TYPE
        if (alarms.now == ALARM_WARN) { //тревога будильника
          radioSeekStop(); //остановка автопоиска радиостанции
          return ALARM_PROGRAM;
        }
#endif
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
        if ((timer.mode == 2) && !timer.count) { //тревога таймера
          radioSeekStop(); //остановка автопоиска радиостанции
          return WARN_PROGRAM;
        }
#endif
        if (++time_out >= RADIO_TIMEOUT) { //если время вышло
          radioSeekStop(); //остановка автопоиска радиостанции
          animShow = ANIM_MAIN; //установили флаг анимации
          return MAIN_PROGRAM; //выходим по тайм-ауту
        }
      }

      if (!_timer_ms[TMR_MS]) { //если таймер истек
        _timer_ms[TMR_MS] = RADIO_UPDATE_TIME; //устанавливаем таймер

        if (!radio.seekRun) { //если не идет поиск
#if (RADIO_STATUS_DOT_TYPE != 3)
#if ((RADIO_STATUS_DOT_TYPE == 1) && (SECS_DOT != 3)) || !DOTS_PORT_ENABLE
#if SECS_DOT == 4
          if (getStationStatusRDA()) decatronSetLine(28, 2); //установка позиции декатрона
          else decatronDisable(); //отключение декатрона
#elif SECS_DOT == 2
          neonDotSetBright(dot.menuBright); //установка яркости неоновых точек
          if (getStationStatusRDA()) neonDotSet(DOT_RIGHT); //включаем разделительную точку
          else neonDotSet(DOT_NULL); //выключаем разделительную точку
#elif SECS_DOT < 2
          dotSetBright((getStationStatusRDA()) ? dot.menuBright : 0); //управление точками в зависимости от устойчивости сигнала
#endif
#else
          indiClrDots(); //очистка разделителных точек
#if DOTS_TYPE == 1
          if (getStationStatusRDA()) indiSetDotR(3); //установка разделительной точки
#else
#if DOTS_SHIFT
          if (getStationStatusRDA()) indiSetDotL(4); //установка разделительной точки
#if DOTS_TYPE == 2
          if (getStationStatusRDA()) indiSetDotR(3); //установка разделительной точки
#endif
#else
          if (getStationStatusRDA()) indiSetDotL(radioSettings.stationsFreq < 1000); //установка разделительной точки
#endif
#endif
#endif
#endif
        }
        else { //иначе идет автопоиск
          if (getSeekCompleteStatusRDA()) { //если поиск завершился
            clrSeekCompleteStatusRDA(); //очищаем флаг окончания поиска
            radio.seekFreq = getFreqRDA(); //прочитали частоту
          }
          switch (radio.seekRun) {
            case 1:
              if (radioSettings.stationsFreq > radio.seekFreq) radioSettings.stationsFreq--; else radio.seekRun = 0;
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
              if (radio.seekAnim > 0) radio.seekAnim--; else radio.seekAnim = ((LAMP_NUM + 1) * 2);
#endif
              break;
            case 2:
              if (radioSettings.stationsFreq < radio.seekFreq) radioSettings.stationsFreq++; else radio.seekRun = 0;
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
              if (radio.seekAnim < ((LAMP_NUM + 1) * 2)) radio.seekAnim++; else radio.seekAnim = 0;
#endif
              break;
          }
          if (!radio.seekRun) {
            setMuteRDA(RDA_MUTE_OFF); //выключаем приглушение звука
            radioSettings.stationsFreq = radio.seekFreq; //прочитали частоту
            radioSearchStation(); //поиск радиостанции в памяти
          }
          else _timer_ms[TMR_MS] = RADIO_ANIM_TIME; //устанавливаем таймер
        }

#if DOTS_PORT_ENABLE
#if (DOTS_TYPE == 1) || ((DOTS_DIV == 1) && (DOTS_TYPE == 2))
        indiSetDotR(2); //включаем разделительную точку
#else
        indiSetDotL(3); //включаем разделительную точку
#endif
#endif

#if INDI_SYMB_TYPE && (RADIO_STATUS_DOT_TYPE == 3)
        if (getStationStatusRDA()) indiSetSymb(SYMB_RADIO); //установка индикатора символов
        else indiClrSymb(); //очистка индикатора символов
#endif

        indiClr(); //очистка индикаторов
        indiPrintNum(radioSettings.stationsFreq, 0, 4); //текущаяя частота
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
        if (!radio.seekRun) { //если не идет поиск
          boolean freq_backl = (radioSettings.stationsFreq >= 1000);
          wsBacklSetMultipleHue((freq_backl) ? 0 : 1, (freq_backl) ? 3 : 2, RADIO_BACKL_COLOR_1, RADIO_BACKL_COLOR_2);
          wsBacklSetLedHue(3, RADIO_BACKL_COLOR_3, WHITE_ON);
        }
        else wsBacklSetMultipleHue((radio.seekAnim >> 1) - 1, 1, RADIO_BACKL_COLOR_1, RADIO_BACKL_COLOR_2); //иначе анимация
#endif
#if LAMP_NUM > 4
        if (radioSettings.stationNum < RADIO_MAX_STATIONS) {
          indiPrintNum(radioSettings.stationNum, 5); //номер станции
#if (BACKL_TYPE == 3) && RADIO_BACKL_TYPE
          wsBacklSetLedHue(5, RADIO_BACKL_COLOR_3, WHITE_ON);
#endif
        }
#endif
      }

#if PLAYER_TYPE
      if (!radio.powerState) { //если питание выключено
        if (!playerPlaybackStatus()) { //если все команды отправлены
          radioStartup(); //включить радиоприемник
        }
      }
#endif

      switch (radioFastSettings()) { //быстрые настройки радио
        case 1: //клик
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;
        case 2: return MAIN_PROGRAM; //выходим
      }

      switch (buttonState()) {
        case RIGHT_KEY_PRESS: //клик правой кнопкой
          radioSeekStop(); //остановка автопоиска радиостанции
          if (radioSettings.stationsFreq < RADIO_MAX_FREQ) radioSettings.stationsFreq++; else radioSettings.stationsFreq = RADIO_MIN_FREQ; //переключаем частоту
          setFreqRDA(radioSettings.stationsFreq); //установили частоту
          radioSearchStation(); //поиск радиостанции в памяти
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;

        case LEFT_KEY_PRESS: //клик левой кнопкой
          radioSeekStop(); //остановка автопоиска радиостанции
          if (radioSettings.stationsFreq > RADIO_MIN_FREQ) radioSettings.stationsFreq--; else radioSettings.stationsFreq = RADIO_MAX_FREQ; //переключаем частоту
          setFreqRDA(radioSettings.stationsFreq); //установили частоту
          radioSearchStation(); //поиск радиостанции в памяти
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;

        case RIGHT_KEY_HOLD: //удержание правой кнопки
          if (!radio.seekRun) radioSeekUp(); //автопоиск радиостанций
          else radioSeekStop(); //остановка автопоиска радиостанции
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = RADIO_ANIM_TIME; //устанавливаем таймер
          break;

        case LEFT_KEY_HOLD: //удержание левой кнопки
          if (!radio.seekRun) radioSeekDown(); //автопоиск радиостанций
          else radioSeekStop(); //остановка автопоиска радиостанции
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = RADIO_ANIM_TIME; //устанавливаем таймер
          break;

        case ADD_KEY_HOLD: //удержание дополнительной кнопки
          if (!radio.seekRun) { //если не идет поиск
            if (!radioMenuSettings()) { //настройки радио
#if !PLAYER_TYPE
              buzzPulse(RADIO_SAVE_SOUND_FREQ, RADIO_SAVE_SOUND_TIME); //сигнал успешной записи радиостанции в память
#endif
            }
          }
          time_out = 0; //сбросили таймер
          _timer_ms[TMR_MS] = 0; //сбросили таймер
          break;

        case SET_KEY_HOLD: //удержание средней кнопк
          radioSeekStop(); //остановка автопоиска радиостанции
          radioShutdown(); //выключить радиоприемник
          return MAIN_PROGRAM; //выходим
      }
    }
    return INIT_PROGRAM;
  }
  return MAIN_PROGRAM;
}
//--------------------------------Обработка таймера----------------------------------------
void timerUpdate(void) //обработка таймера
{
  switch (timer.mode) {
    case 1: if (timer.count != 65535) timer.count++; break;
    case 2: if (timer.count) timer.count--; break;
  }
}
//--------------------------------Тревога таймера----------------------------------------
uint8_t timerWarn(void) //тревога таймера
{
  boolean blink_data = 0; //флаг мигания индикаторами

#if PLAYER_TYPE
  playerStop(); //сброс позиции мелодии
#else
  melodyPlay(SOUND_TIMER_WARN, SOUND_LINK(general_sound), REPLAY_CYCLE); //звук окончания таймера
#endif
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
  radioPowerOff(); //выключить питание радиоприемника
#endif
#if (BACKL_TYPE == 3) && TIMER_WARN_BACKL_TYPE
  backlAnimDisable(); //запретили эффекты подсветки
#if TIMER_WARN_BACKL_TYPE == 1
  changeBrightDisable(CHANGE_DYNAMIC_BACKL); //разрешить смену яркости динамичной подсветки
#endif
  wsBacklSetLedHue(TIMER_WARN_COLOR, WHITE_ON); //установили цвет
#endif
  while (!buttonState()) { //ждем
    dataUpdate(); //обработка данных
#if ESP_ENABLE
    if (!timer.mode) { //если таймер отключен
#if PLAYER_TYPE
      playerStop(); //сброс воспроизведения плеера
#else
      melodyStop(); //сброс воспроизведения мелодии
#endif
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
      radioPowerRet(); //вернуть питание радиоприемника
#endif
      return MAIN_PROGRAM; //выходим
    }
#endif
#if PLAYER_TYPE
    if (!playerPlaybackStatus()) playerSetTrack(PLAYER_TIMER_WARN_SOUND, PLAYER_GENERAL_FOLDER);
#endif
    if (!_timer_ms[TMR_ANIM]) {
      _timer_ms[TMR_ANIM] = TIMER_BLINK_TIME;
      switch (blink_data) {
        case 0: indiClr(); break; //очищаем индикаторы
        case 1: indiPrintNum(0, 0, LAMP_NUM, 0); break; //вывод минут/часов/секунд
      }
      dotSetBright((blink_data) ? dot.menuBright : 0); //установили точки
#if (BACKL_TYPE == 3) && TIMER_WARN_BACKL_TYPE
#if TIMER_WARN_BACKL_TYPE == 1
      wsBacklSetLedBright((blink_data) ? backl.maxBright : 0); //установили яркость
#else
      wsBacklSetLedBright((blink_data) ? backl.menuBright : 0); //установили яркость
#endif
#endif
      blink_data = !blink_data; //мигаем временем
    }
  }
  timer.mode = 0; //деактивируем таймер
  timer.count = timer.time; //сбрасываем таймер
#if RADIO_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE || ESP_ENABLE)
  radioPowerRet(); //вернуть питание радиоприемника
#endif
  return TIMER_PROGRAM;
}
//----------------------------Настройки таймера----------------------------------
void timerSettings(void) //настройки таймера
{
  boolean mode = 0; //текущий режим
  boolean blink_data = 0; //флаг мигания индикаторами
  uint8_t mins = 99; //буфер минут
  uint8_t secs = 59; //буфер секунд

  if (timer.time < 6000) { //если в диапазоне настройки
    mins = timer.time / 60; //установили минуты
    secs = timer.time % 60; //установили секунды
  }

#if PLAYER_TYPE
  if (mainSettings.baseSound) playerSetTrackNow(PLAYER_TIMER_SET_SOUND, PLAYER_GENERAL_FOLDER);
#endif

#if INDI_SYMB_TYPE
  indiSetSymb(SYMB_MENU); //установка индикатора символов
#endif

  dotSetBright(0); //выключаем точки
  while (1) {
    dataUpdate(); //обработка данных

    if (!_timer_ms[TMR_MS]) { //если прошло пол секунды
      _timer_ms[TMR_MS] = SETTINGS_BLINK_TIME; //устанавливаем таймер

      indiClr(); //очистка индикаторов
#if LAMP_NUM > 4
      indiPrintNum(2, 5); //вывод режима
#endif
      indiPrintMenuData(blink_data, mode, mins, 0, secs, 2); //вывод минут/секунд

#if (BACKL_TYPE == 3) && TIMER_BACKL_TYPE
      wsBacklSetMultipleHue(mode * 2, 2, TIMER_MENU_COLOR_1, TIMER_MENU_COLOR_2);
#endif
      blink_data = !blink_data;
    }

    switch (buttonState()) {
      case SET_KEY_PRESS: //клик средней кнопкой
        mode = !mode; //переключаем режим
        _timer_ms[TMR_MS] = blink_data = 0; //сбрасываем флаги
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        switch (mode) {
          case 0: if (mins < 99) mins++; else mins = 0; break; //прибавляем минуты
          case 1: if (secs < 59) secs++; else secs = 0; break; //прибавляем секунды
        }
        _timer_ms[TMR_MS] = blink_data = 0; //сбрасываем флаги
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
        switch (mode) {
          case 0: if (mins > 0) mins--; else mins = 99; break; //убавляем минуты
          case 1: if (secs > 0) secs--; else secs = 59; break; //убавляем секунды
        }
        _timer_ms[TMR_MS] = blink_data = 0; //сбрасываем флаги
        break;

      case ADD_KEY_HOLD: //удержание дополнительной кнопки
        return; //выходим

      case SET_KEY_HOLD: //удержание средней кнопки
        if (!mins && !secs) timer.time = TIMER_TIME; //устанавливаем значение по умолчанию
        else timer.time = (mins * 60) + secs; //установили настроенное время
        timer.count = timer.time; //сбрасываем таймер
        return; //выходим
    }
  }
}
//--------------------------------Таймер-секундомер----------------------------------------
uint8_t timerStopwatch(void) //таймер-секундомер
{
  uint8_t mode = 0; //текущий режим
  uint8_t time_out = 0; //таймаут автовыхода
#if LAMP_NUM > 4
  static uint8_t millis_cnt; //счетчик миллисекунд
#endif

  if (timer.mode) mode = (timer.mode & 0x7F) - 1; //если таймер был запущен
  else { //иначе таймер выключен
    timer.count = 0; //сбрасываем таймер
#if LAMP_NUM > 4
    millis_cnt = 0; //сбрасываем счетчик миллисекунд
#endif
  }

#if PLAYER_TYPE
  if (mainSettings.baseSound && (!timer.mode || (timer.mode > 2))) playerSetTrackNow((mode) ? PLAYER_TIMER_SOUND : PLAYER_STOPWATCH_SOUND, PLAYER_GENERAL_FOLDER);
#endif

#if (BACKL_TYPE == 3) && TIMER_BACKL_TYPE
  backlAnimDisable(); //запретили эффекты подсветки
#if TIMER_BACKL_TYPE == 1
  changeBrightDisable(CHANGE_STATIC_BACKL); //разрешить смену яркости статичной подсветки
  wsBacklSetLedBright((fastSettings.backlMode & 0x7F) ? backl.maxBright : 0); //установили яркость в зависимости от режима подсветки
#else
  wsBacklSetLedBright(backl.menuBright); //установили максимальную яркость
#endif
#endif

  while (1) {
    dataUpdate(); //обработка данных

#if ESP_ENABLE
    if (busCheck() & ~(0x01 << BUS_COMMAND_WAIT)) {
      if (!timer.mode || (timer.mode > 2)) return MAIN_PROGRAM; //выходим
      else return TIMER_PROGRAM; //выходим
    }
#endif

    if ((timer.mode == 2) && !timer.count) return WARN_PROGRAM; //тревога таймера

    if (!indi.update) {
      indi.update = 1; //сбрасываем флаг

      if (!timer.mode || (timer.mode > 2)) {
        if (++time_out >= TIMER_TIMEOUT) {
          animShow = ANIM_MAIN; //установили флаг анимации
          return MAIN_PROGRAM;
        }
      }

      if (!timer.count) dotSetBright(0); //выключили точки
      else if (timer.mode > 2 || !timer.mode) dotSetBright(dot.menuBright); //установили точки
      else dotSetBright((dotGetBright()) ? 0 : dot.menuBright); //установили точки

      uint8_t hour = timer.count / 3600; //часы
      uint8_t mins = (timer.count / 60) % 60; //минуты
      uint8_t secs = timer.count % 60; //секунды

      indiClr(); //очистка индикаторов
#if INDI_SYMB_TYPE
      indiClrSymb(); //очистка индикатора символов
#endif
#if LAMP_NUM > 4
      if (timer.mode) {
        if (timer.mode < 3) millis_cnt = 0; //сбрасываем счетчик миллисекунд
        indiPrintNum((timer.count < 3600) ? ((mode) ? (100 - millis_cnt) : millis_cnt) : secs, 4, 2, 0); //вывод милиекунд/секунд
      }
      else indiPrintNum(mode + 1, 5); //вывод режима
#endif

      indiPrintNum((timer.count < 3600) ? mins : hour, 0, 2, 0); //вывод минут/часов
      indiPrintNum((timer.count < 3600) ? secs : mins, 2, 2, 0); //вывод секунд/минут

#if (BACKL_TYPE == 3) && TIMER_BACKL_TYPE
      switch (timer.mode) {
        case 0: wsBacklSetLedHue(TIMER_STOP_COLOR, WHITE_ON); break; //установили цвет остановки
        case 1: wsBacklSetLedHue(TIMER_RUN_COLOR_1, WHITE_ON); break; //установили цвет секундомера
        case 2: wsBacklSetLedHue(TIMER_RUN_COLOR_2, WHITE_ON); break; //установили цвет таймера
        default: wsBacklSetLedHue(TIMER_PAUSE_COLOR, WHITE_ON); break; //установили цвет паузы
      }
#endif
    }

#if LAMP_NUM > 4
    switch (timer.mode) {
      case 1: //секундомер
      case 2: //таймер
        if (!_timer_ms[TMR_MS]) {
          _timer_ms[TMR_MS] = 10;
          if (timer.count < 3600) {
            millis_cnt += 1;
            indiPrintNum((mode) ? (100 - millis_cnt) : millis_cnt, 4, 2, 0); //вывод милиекунд
          }
        }
        break;
    }
#endif

    switch (buttonState()) {
      case SET_KEY_PRESS: //клик средней кнопкой
        if (mode && !timer.mode) { //если режим таймера и таймер/секундомер не запущен
          timerSettings(); //настройки таймера
          time_out = 0; //сбрасываем таймер автовыхода
          indi.update = 0; //обновление экрана
        }
        break;

      case SET_KEY_HOLD: //удержание средней кнопки
        if (timer.mode == 1) timer.mode |= 0x80; //приостановка секундомера
        return MAIN_PROGRAM; //выходим

      case RIGHT_KEY_PRESS: //клик правой кнопкой
      case RIGHT_KEY_HOLD: //удержание правой кнопки
#if PLAYER_TYPE
        if (mainSettings.baseSound) playerSetTrackNow(PLAYER_TIMER_SOUND, PLAYER_GENERAL_FOLDER);
#endif
        mode = 1; //переключаем режим
        timer.mode = 0; //деактивируем таймер
        timer.count = timer.time; //сбрасываем таймер
        time_out = 0; //сбрасываем таймер автовыхода
        indi.update = 0; //обновление экрана
        break;

      case LEFT_KEY_PRESS: //клик левой кнопкой
      case LEFT_KEY_HOLD: //удержание левой кнопки
#if PLAYER_TYPE
        if (mainSettings.baseSound) playerSetTrackNow(PLAYER_STOPWATCH_SOUND, PLAYER_GENERAL_FOLDER);
#endif
        mode = 0; //переключаем режим
        timer.mode = 0; //деактивируем таймер
        timer.count = 0; //сбрасываем секундомер
        time_out = 0; //сбрасываем таймер автовыхода
        indi.update = 0; //обновление экрана
        break;

      case ADD_KEY_PRESS: //клик дополнительной кнопкой
        if (!timer.mode) {
#if LAMP_NUM > 4
          millis_cnt = 0; //сбрасываем счетчик миллисекунд
#endif
          timer.mode = mode + 1;
        }
        else timer.mode ^= 0x80; //приостановка таймера/секундомера
        time_out = 0; //сбрасываем таймер автовыхода
        indi.update = 0; //обновление экрана
        break;

      case ADD_KEY_HOLD: //удержание дополнительной кнопки
        timer.mode = 0; //деактивируем таймер
        switch (mode & 0x01) {
          case 0: timer.count = 0; break; //сбрасываем секундомер
          case 1: timer.count = timer.time; break; //сбрасываем таймер
        }
        time_out = 0; //сбрасываем таймер автовыхода
        indi.update = 0; //обновление экрана
        break;
    }
  }
  return INIT_PROGRAM;
}
//----------------------------Воспроизвести температуру--------------------------------------
void speakTemp(boolean mode) //воспроизвести температуру
{
#if ESP_ENABLE
  uint8_t _sens = (!mode) ? getMainSens() : getHourSens();
  uint16_t _int = getTemperature(_sens) / 10;
  uint16_t _dec = getTemperature(_sens) % 10;
#else
  uint16_t _int = getTemperature() / 10;
  uint16_t _dec = getTemperature() % 10;
#endif

  if (!mode) playerSetTrackNow(PLAYER_TEMP_SOUND, PLAYER_GENERAL_FOLDER);
  else playerSetTrack(PLAYER_TEMP_SOUND, PLAYER_GENERAL_FOLDER);
#if ESP_ENABLE
  if (getTemperatureSign(_sens)) playerSetTrack(PLAYER_SENS_TEMP_OTHER, PLAYER_END_NUMBERS_FOLDER);
#elif SENS_PORT_ENABLE
  if (getTemperatureSign()) playerSetTrack(PLAYER_SENS_TEMP_OTHER, PLAYER_END_NUMBERS_FOLDER);
#endif
  if (_dec && !mode) {
    playerSpeakNumber(_int, OTHER_NUM);
    playerSetTrack(PLAYER_SENS_CEIL_START + (boolean)playerGetSpeak(_int), PLAYER_END_NUMBERS_FOLDER);
    playerSpeakNumber(_dec, OTHER_NUM);
    playerSetTrack(PLAYER_SENS_DEC_START + (boolean)playerGetSpeak(_dec), PLAYER_END_NUMBERS_FOLDER);
    playerSetTrack(PLAYER_SENS_TEMP_START + 1, PLAYER_END_NUMBERS_FOLDER);
  }
  else {
    playerSpeakNumber(_int);
    playerSetTrack(PLAYER_SENS_TEMP_START + playerGetSpeak(_int), PLAYER_END_NUMBERS_FOLDER);
  }
}
//------------------------------Воспроизвести влажность---------------------------------------
void speakHum(uint8_t hum) //воспроизвести влажность
{
  playerSetTrackNow(PLAYER_HUM_SOUND, PLAYER_GENERAL_FOLDER);
  playerSpeakNumber(hum);
  playerSetTrack(PLAYER_SENS_HUM_START + playerGetSpeak(hum), PLAYER_END_NUMBERS_FOLDER);
}
//-------------------------------Воспроизвести давление---------------------------------------
void speakPress(uint16_t press) //воспроизвести давление
{
  playerSetTrackNow(PLAYER_PRESS_SOUND, PLAYER_GENERAL_FOLDER);
  playerSpeakNumber(press);
  playerSetTrack(PLAYER_SENS_PRESS_START + playerGetSpeak(press), PLAYER_END_NUMBERS_FOLDER);
  playerSetTrack(PLAYER_SENS_PRESS_OTHER, PLAYER_END_NUMBERS_FOLDER);
}
//--------------------------------Показать температуру----------------------------------------
uint8_t showTemp(void) //показать температуру
{
  uint8_t mode = 0; //текущий режим

  uint16_t temperature = getTemperature(); //буфер температуры
  uint16_t pressure = getPressure(); //буфер давления
  uint8_t humidity = getHumidity(); //буфер влажности

  if (temperature > 990) return MAIN_PROGRAM; //выходим

#if (BACKL_TYPE == 3) && SHOW_TEMP_BACKL_TYPE
  backlAnimDisable(); //запретили эффекты подсветки
#if SHOW_TEMP_BACKL_TYPE == 1
  changeBrightDisable(CHANGE_STATIC_BACKL); //разрешить смену яркости статичной подсветки
  wsBacklSetLedBright((fastSettings.backlMode & 0x7F) ? backl.maxBright : 0); //установили яркость в зависимости от режима подсветки
#else
  wsBacklSetLedBright(backl.menuBright); //установили максимальную яркость
#endif
#endif

  setDotTemp(1); //установить точку температуры

#if (ESP_ENABLE || SENS_PORT_ENABLE) && !INDI_SYMB_TYPE
  boolean dot = 0; //флаг мигания точками
  boolean sign = getTemperatureSign(); //знак температуры
  _timer_ms[TMR_ANIM] = SHOW_TEMP_SIGN_TIME; //устанавливаем таймер
#endif

#if PLAYER_TYPE
  if (mainSettings.baseSound) speakTemp(SPEAK_TEMP_MAIN); //воспроизвести температуру
#endif

  for (_timer_ms[TMR_MS] = SHOW_TEMP_TIME; _timer_ms[TMR_MS];) {
    dataUpdate(); //обработка данных

#if (ESP_ENABLE || SENS_PORT_ENABLE) && !INDI_SYMB_TYPE
    if (!mode && sign) { //если температура отрицательная
      if (!_timer_ms[TMR_ANIM]) { //если пришло время
        _timer_ms[TMR_ANIM] = SHOW_TEMP_SIGN_TIME; //устанавливаем таймер
        setDotTemp(dot); //инвертировать точку температуры
        dot = !dot; //инвертировали точки
      }
    }
#endif

    if (!indi.update) {
      indi.update = 1; //сбрасываем флаг
      indiClr(); //очистка индикаторов
#if (LAMP_NUM > 4) && MENU_SHOW_NUMBER
      indiPrintNum(mode + 1, 5); //режим
#endif
      switch (mode) {
        case 0:
          indiPrintNum(temperature, 1, 2, 0);
#if INDI_SYMB_TYPE
          indiSetSymb(getTemperatureSign() ? SYMB_NEGATIVE : SYMB_POSITIVE); //установка индикатора символов
#endif
#if (BACKL_TYPE == 3) && SHOW_TEMP_BACKL_TYPE
          wsBacklSetLedHue(SHOW_TEMP_COLOR_T, WHITE_ON); //установили цвет температуры
#endif
          break;
        case 1:
          indiPrintNum(humidity, 0, 4);
#if INDI_SYMB_TYPE
          indiSetSymb(SYMB_HUMIDITY); //установка индикатора символов
#endif
#if (BACKL_TYPE == 3) && SHOW_TEMP_BACKL_TYPE
          wsBacklSetLedHue(SHOW_TEMP_COLOR_H, WHITE_ON); //установили цвет влажности
#endif
          break;
        case 2:
          indiPrintNum(pressure, 0, 4);
#if INDI_SYMB_TYPE
          indiSetSymb(SYMB_PRESSURE); //установка индикатора символов
#endif
#if (BACKL_TYPE == 3) && SHOW_TEMP_BACKL_TYPE
          wsBacklSetLedHue(SHOW_TEMP_COLOR_P, WHITE_ON); //установили цвет давления
#endif
          break;
      }
    }

    switch (buttonState()) {
      case LEFT_KEY_PRESS: //клик левой кнопкой
        if (++mode > 2) mode = 0;
        switch (mode) {
          case 1:
            if (!humidity) {
              if (!pressure) mode = 0;
              else mode = 2;
            }
            break;
          case 2: if (!pressure) mode = 0; break;
        }
        if (!mode) { //если режим отображения температуры
          setDotTemp(1); //установить точку температуры
#if (ESP_ENABLE || SENS_PORT_ENABLE) && !INDI_SYMB_TYPE
          dot = 0; //установили флаг мигания точками
          _timer_ms[TMR_ANIM] = SHOW_TEMP_SIGN_TIME; //устанавливаем таймер
#endif
        }
        else { //иначе давление или влажность
          setDotTemp(0); //очистить точку температуры
        }
#if PLAYER_TYPE
        if (mainSettings.baseSound) {
          switch (mode) {
            case 0: speakTemp(SPEAK_TEMP_MAIN); break; //воспроизвести температуру
            case 1: speakHum(humidity); break; //воспроизвести влажность
            case 2: speakPress(pressure); break; //воспроизвести давление
          }
        }
#endif
        _timer_ms[TMR_MS] = SHOW_TEMP_TIME;
        indi.update = 0; //обновление экрана
        break;

      case RIGHT_KEY_PRESS: //клик правой кнопкой
      case SET_KEY_PRESS: //клик средней кнопкой
        return MAIN_PROGRAM; //выходим
    }
  }
  animShow = ANIM_MAIN; //установили флаг анимации
  return MAIN_PROGRAM; //выходим
}
//-------------------------------Воспроизвести время--------------------------------
void speakTime(boolean mode) //воспроизвести время
{
  if (!mode) playerSetTrackNow(PLAYER_TIME_NOW_SOUND, PLAYER_GENERAL_FOLDER);
  else playerSetTrack(PLAYER_TIME_NOW_SOUND, PLAYER_GENERAL_FOLDER);
  playerSpeakNumber(RTC.h);
  playerSetTrack(PLAYER_TIME_HOUR_START + playerGetSpeak(RTC.h), PLAYER_END_NUMBERS_FOLDER);
  if (RTC.m) {
    playerSpeakNumber(RTC.m, OTHER_NUM);
    playerSetTrack(PLAYER_TIME_MINS_START + playerGetSpeak(RTC.m), PLAYER_END_NUMBERS_FOLDER);
  }
}
//----------------------------------Показать дату-----------------------------------
uint8_t showDate(void) //показать дату
{
#if (SHOW_DATE_TYPE < 2) || (LAMP_NUM < 6)
  uint8_t mode = 0; //текущий режим
#endif

  setDotDate(1); //включили разделительную точку

#if (BACKL_TYPE == 3) && SHOW_DATE_BACKL_TYPE
  backlAnimDisable(); //запретили эффекты подсветки
#if SHOW_DATE_BACKL_TYPE == 1
  changeBrightDisable(CHANGE_STATIC_BACKL); //разрешить смену яркости статичной подсветки
  wsBacklSetLedBright((fastSettings.backlMode & 0x7F) ? backl.maxBright : 0); //установили яркость в зависимости от режима подсветки
#else
  wsBacklSetLedBright(backl.menuBright); //установили максимальную яркость
#endif
#endif

#if PLAYER_TYPE
  if (mainSettings.baseSound) speakTime(0); //воспроизвести время
#endif

  for (_timer_ms[TMR_MS] = SHOW_DATE_TIME; _timer_ms[TMR_MS];) {
    dataUpdate(); //обработка данных

    if (!indi.update) {
      indi.update = 1; //сбрасываем флаг
      indiClr(); //очистка индикаторов
#if (SHOW_DATE_TYPE > 1) && (LAMP_NUM > 4)
#if SHOW_DATE_TYPE == 3
      indiPrintNum(RTC.MM, 0, 2, 0); //вывод месяца
      indiPrintNum(RTC.DD, 2, 2, 0); //вывод даты
#else
      indiPrintNum(RTC.DD, 0, 2, 0); //вывод даты
      indiPrintNum(RTC.MM, 2, 2, 0); //вывод месяца
#endif
      indiPrintNum(RTC.YY - 2000, 4, 2, 0); //вывод года
#if (BACKL_TYPE == 3) && SHOW_DATE_BACKL_TYPE
      wsBacklSetMultipleHue(0, 4, SHOW_DATE_BACKL_DM, SHOW_DATE_BACKL_YY);
#endif
#else
#if (LAMP_NUM > 4) && MENU_SHOW_NUMBER && !SHOW_DATE_WEEK
      indiPrintNum(mode + 1, 5); //режим
#endif
      switch (mode) {
        case 0:
#if SHOW_DATE_TYPE == 1
          indiPrintNum(RTC.MM, 0, 2, 0); //вывод месяца
          indiPrintNum(RTC.DD, 2, 2, 0); //вывод даты
#else
          indiPrintNum(RTC.DD, 0, 2, 0); //вывод даты
          indiPrintNum(RTC.MM, 2, 2, 0); //вывод месяца
#endif
#if (LAMP_NUM > 4) && SHOW_DATE_WEEK
          indiPrintNum(RTC.DW, 5); //день недели
#endif
#if (BACKL_TYPE == 3) && SHOW_DATE_BACKL_TYPE
          wsBacklSetMultipleHue(0, 4, SHOW_DATE_BACKL_DM, SHOW_DATE_BACKL_NN);
#if SHOW_DATE_WEEK
          wsBacklSetLedHue(5, SHOW_DATE_BACKL_DW, WHITE_ON);
#endif
#endif
          break;
        case 1:
          indiPrintNum(RTC.YY, 0); //вывод года
#if (BACKL_TYPE == 3) && SHOW_DATE_BACKL_TYPE
          wsBacklSetMultipleHue(0, 4, SHOW_DATE_BACKL_YY, SHOW_DATE_BACKL_NN);
#endif
          break;
      }
#endif
    }

    switch (buttonState()) {
      case RIGHT_KEY_PRESS: //клик правой кнопкой
#if (SHOW_DATE_TYPE < 2) || (LAMP_NUM < 6)
        if (++mode > 1) mode = 0;
        switch (mode) {
          case 0: //дата
            setDotDate(1); //включили разделительную точку
            break;
          case 1: //год
            setDotDate(0); //включили разделительную точку
            break;
        }
        _timer_ms[TMR_MS] = SHOW_DATE_TIME;
        indi.update = 0; //обновление экрана
        break;
#endif

      case LEFT_KEY_PRESS: //клик левой кнопкой
      case SET_KEY_PRESS: //клик средней кнопкой
        return MAIN_PROGRAM; //выходим
    }
  }
  animShow = ANIM_MAIN; //установили флаг анимации
  return MAIN_PROGRAM; //выходим
}
//------------------------------Анимация автоматического показа--------------------------------------
uint8_t autoShowAnimMode(void) //анимация автоматического показа
{
  return (mainSettings.autoShowFlip) ? mainSettings.autoShowFlip : fastSettings.flipMode;
}
//--------------------------------Меню автоматического показа----------------------------------------
void autoShowMenu(void) //меню автоматического показа
{
#if (BACKL_TYPE == 3) && AUTO_SHOW_BACKL_TYPE
  boolean state = 0; //состояние подсветки
#endif
  uint8_t show_mode = 0; //текущий режим

#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
  uint16_t temperature = 0; //буфер температуры
  uint16_t pressure = 0; //буфер давления
  uint8_t humidity = 0; //буфер влажности
#endif

  for (uint8_t mode = 0; mode < sizeof(extendedSettings.autoShowModes); mode++) {
#if DOTS_PORT_ENABLE
    indiClrDots(); //выключаем разделительные точки
#endif
#if (SECS_DOT != 3) || !DOTS_PORT_ENABLE
    dotSetBright(0); //выключаем секундные точки
#endif
#if INDI_SYMB_TYPE
    indiClrSymb(); //очистка индикатора символов
#endif
    animClearBuff(); //очистка буфера анимации
    show_mode = extendedSettings.autoShowModes[mode];
    switch (show_mode) {
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
      case SHOW_TEMP: //режим отображения температуры
#if LAMP_NUM > 4
      case SHOW_TEMP_HUM: //режим отображения температуры и влажности
#endif
#if ESP_ENABLE
      case SHOW_TEMP_ESP:
#if LAMP_NUM > 4
      case SHOW_TEMP_HUM_ESP:
#endif
        temperature = getTemperature(show_mode);
#if LAMP_NUM > 4
        humidity = getHumidity(show_mode);
#endif
#else
        temperature = getTemperature();
#if LAMP_NUM > 4
        humidity = getHumidity();
#endif
#endif
        if (temperature > 990) continue; //возвращаемся назад
        animPrintNum(temperature, 1, 2, 0); //вывод температуры
#if LAMP_NUM > 4
        if (humidity && (show_mode != SHOW_TEMP) && (show_mode != SHOW_TEMP_ESP)) animPrintNum(humidity, 4, 2); //вывод влажности
#endif
        animIndi(autoShowAnimMode(), FLIP_NORMAL); //анимация цифр
        setDotTemp(1); //установить точку температуры
#if INDI_SYMB_TYPE
#if ESP_ENABLE
        indiSetSymb(getTemperatureSign(show_mode) ? SYMB_NEGATIVE : SYMB_POSITIVE); //установка индикатора символов
#else
        indiSetSymb(getTemperatureSign() ? SYMB_NEGATIVE : SYMB_POSITIVE); //установка индикатора символов
#endif
#endif
#if (BACKL_TYPE == 3) && AUTO_SHOW_BACKL_TYPE
#if LAMP_NUM > 4
        if (humidity && (show_mode != SHOW_TEMP) && (show_mode != SHOW_TEMP_ESP)) { //если режим отображения температуры и влажности
          wsBacklSetMultipleHue(4, 2, SHOW_TEMP_COLOR_H, SHOW_TEMP_COLOR_T); //установили цвет температуры и влажности
          wsBacklSetLedHue(3, SHOW_TEMP_COLOR_P, WHITE_ON); //установили цвет пустого сегмента
        }
        else wsBacklSetLedHue(SHOW_TEMP_COLOR_T, WHITE_ON); //установили цвет температуры
#else
        wsBacklSetLedHue(SHOW_TEMP_COLOR_T, WHITE_ON); //установили цвет температуры
#endif
#endif
        break;

      case SHOW_HUM: //режим отображения влажности
#if ESP_ENABLE
      case SHOW_HUM_ESP:
        humidity = getHumidity(show_mode);
#else
        humidity = getHumidity();
#endif
        if (!humidity) continue; //возвращаемся назад
        animPrintNum(humidity, 0, 4); //вывод влажности

        animIndi(autoShowAnimMode(), FLIP_NORMAL); //анимация цифр
#if INDI_SYMB_TYPE
        indiSetSymb(SYMB_HUMIDITY); //установка индикатора символов
#endif
#if (BACKL_TYPE == 3) && AUTO_SHOW_BACKL_TYPE
        wsBacklSetLedHue(SHOW_TEMP_COLOR_H, WHITE_ON); //установили цвет влажности
#endif
        break;

      case SHOW_PRESS: //режим отображения давления
#if ESP_ENABLE
      case SHOW_PRESS_ESP:
        pressure = getPressure(show_mode);
#else
        pressure = getPressure();
#endif
        if (!pressure) continue; //возвращаемся назад
        animPrintNum(pressure, 0, 4); //вывод давления

        animIndi(autoShowAnimMode(), FLIP_NORMAL); //анимация цифр
#if INDI_SYMB_TYPE
        indiSetSymb(SYMB_PRESSURE); //установка индикатора символов
#endif
#if (BACKL_TYPE == 3) && AUTO_SHOW_BACKL_TYPE
        wsBacklSetLedHue(SHOW_TEMP_COLOR_P, WHITE_ON); //установили цвет давления
#endif
        break;
#endif

      case SHOW_DATE: //режим отображения даты
#if (SHOW_DATE_TYPE == 1) || (SHOW_DATE_TYPE == 3)
        animPrintNum(RTC.MM, 0, 2, 0); //вывод месяца
        animPrintNum(RTC.DD, 2, 2, 0); //вывод даты
#else
        animPrintNum(RTC.DD, 0, 2, 0); //вывод даты
        animPrintNum(RTC.MM, 2, 2, 0); //вывод месяца
#endif
#if SHOW_DATE_WEEK
        animPrintNum(RTC.DW, 5); //день недели
#endif
        animIndi(autoShowAnimMode(), FLIP_NORMAL); //анимация цифр

        setDotDate(1); //включили разделительную точку

#if (BACKL_TYPE == 3) && SHOW_DATE_BACKL_TYPE
        wsBacklSetMultipleHue(0, 4, SHOW_DATE_BACKL_DM, SHOW_DATE_BACKL_NN);
#if SHOW_DATE_WEEK
        wsBacklSetLedHue(5, SHOW_DATE_BACKL_DW, WHITE_ON);
#endif
#endif
        break;

      case SHOW_YEAR: //режим отображения года
        animPrintNum(RTC.YY, 0); //вывод года
        animIndi(autoShowAnimMode(), FLIP_NORMAL); //анимация цифр
#if (BACKL_TYPE == 3) && SHOW_DATE_BACKL_TYPE
        wsBacklSetMultipleHue(0, 4, SHOW_DATE_BACKL_YY, SHOW_DATE_BACKL_NN);
#endif
        break;

#if LAMP_NUM > 4
      case SHOW_DATE_YEAR: //режим отображения даты и года
#if (SHOW_DATE_TYPE == 1) || (SHOW_DATE_TYPE == 3)
        animPrintNum(RTC.MM, 0, 2, 0); //вывод месяца
        animPrintNum(RTC.DD, 2, 2, 0); //вывод даты
#else
        animPrintNum(RTC.DD, 0, 2, 0); //вывод даты
        animPrintNum(RTC.MM, 2, 2, 0); //вывод месяца
#endif
        animPrintNum(RTC.YY - 2000, 4, 2, 0); //вывод года
        animIndi(autoShowAnimMode(), FLIP_NORMAL); //анимация цифр

        setDotDate(1); //включили разделительную точку

#if (BACKL_TYPE == 3) && AUTO_SHOW_BACKL_TYPE
        wsBacklSetMultipleHue(0, 4, SHOW_DATE_BACKL_DM, SHOW_DATE_BACKL_YY);
#endif
        break;
#endif
      default: continue; //иначе неизвестный режим
    }

#if (BACKL_TYPE == 3) && AUTO_SHOW_BACKL_TYPE
    if (!state) { //если первый запуск
      state = 1; //установили флаг подсветки
      backlAnimDisable(); //запретили эффекты подсветки
#if AUTO_SHOW_BACKL_TYPE == 1
      changeBrightDisable(CHANGE_STATIC_BACKL); //разрешить смену яркости статичной подсветки
      wsBacklSetLedBright((fastSettings.backlMode & 0x7F) ? backl.maxBright : 0); //установили яркость в зависимости от режима подсветки
#else
      wsBacklSetLedBright(backl.menuBright); //установили максимальную яркость
#endif
    }
#endif

#if AUTO_SHOW_DATE_BLINK
    boolean blink = 0; //флаг мигания индикаторами
#endif
#if (ESP_ENABLE || SENS_PORT_ENABLE) && !INDI_SYMB_TYPE
    boolean dot = 0; //флаг мигания точками
    boolean sign = 0; //знак температуры

    switch (show_mode) {
      case SHOW_TEMP: //режим отображения температуры
#if LAMP_NUM > 4
      case SHOW_TEMP_HUM: //режим отображения температуры и влажности
#endif
#if ESP_ENABLE
      case SHOW_TEMP_ESP:
#if LAMP_NUM > 4
      case SHOW_TEMP_HUM_ESP:
#endif
#endif
#if ESP_ENABLE
        if (getTemperatureSign(show_mode)) sign = 1;
#else
        if (getTemperatureSign()) sign = 1;
#endif
        _timer_ms[TMR_ANIM] = SHOW_TEMP_SIGN_TIME; //устанавливаем таймер
        break;
    }
#endif
    _timer_ms[TMR_MS] = (uint16_t)extendedSettings.autoShowTimes[mode] * 1000; //устанавливаем таймер
    while (_timer_ms[TMR_MS]) { //если таймер истек
      dataUpdate(); //обработка данных

#if ESP_ENABLE
      if (busCheck() & ~(0x01 << BUS_COMMAND_WAIT)) return; //обновление шины
#endif

#if (ESP_ENABLE || SENS_PORT_ENABLE) && !INDI_SYMB_TYPE
      if (sign) { //если температура отрицательная
        if (!_timer_ms[TMR_ANIM]) { //если пришло время
          _timer_ms[TMR_ANIM] = SHOW_TEMP_SIGN_TIME; //устанавливаем таймер
          setDotTemp(dot); //инвертировать точку температуры
          dot = !dot; //инвертировали точки
        }
      }
#endif
#if AUTO_SHOW_DATE_BLINK
      if (show_mode <= SHOW_DATE_YEAR) { //если режим отображения даты или года
        if (!_timer_ms[TMR_ANIM]) { //если пришло время
          _timer_ms[TMR_ANIM] = AUTO_SHOW_DATE_TIME; //установили время
          if (blink) indiClr(); //очистили индикаторы
          else animPrintBuff(0, 6, LAMP_NUM); //отрисовали предыдущий буфер
          blink = !blink; //изменили состояние
        }
      }
#endif
      if (buttonState()) return; //возврат если нажата кнопка
    }
#if AUTO_SHOW_DATE_BLINK
    if (blink) animPrintBuff(0, 6, LAMP_NUM); //отрисовали предыдущий буфер
#endif
  }
  animShow = (mainSettings.autoShowFlip) ? (ANIM_OTHER + mainSettings.autoShowFlip) : ANIM_MAIN; //установили флаг анимации
}
//-------------------------Сменить режим анимации минут быстрых настроек----------------------------
void changeFastSetFlip(void) //сменить режим анимации минут быстрых настроек
{
  if (++fastSettings.flipMode > (FLIP_EFFECT_NUM + 1)) fastSettings.flipMode = 0;
}
//-------------------------Сменить режим анимации секунд быстрых настроек----------------------------
void changeFastSetSecs(void) //сменить режим анимации секунд быстрых настроек
{
  if (++fastSettings.secsMode >= SECS_EFFECT_NUM) fastSettings.secsMode = 0;
}
//-------------------------Сменить режим анимации точек быстрых настроек----------------------------
void changeFastSetDot(void) //сменить режим анимации точек быстрых настроек
{
#if BTN_EASY_MAIN_MODE
  if (!dot.maxBright) return; //выходим если точки выключены
#endif
  if (++fastSettings.dotMode >= DOT_EFFECT_NUM) fastSettings.dotMode = 0;
#if BTN_EASY_MAIN_MODE
  dotReset(ANIM_RESET_CHANGE); //сброс анимации точек
  if (dotGetMode() == DOT_STATIC) dotSetBright(dot.maxBright); //точки включены
#endif
}
//-------------------------Сменить режим анимации подсветки быстрых настроек----------------------------
void changeFastSetBackl(void) //сменить режим анимации подсветки быстрых настроек
{
#if BTN_EASY_MAIN_MODE && (BACKL_TYPE == 3)
  if (!backl.maxBright) return; //выходим если точки выключены

  switch (fastSettings.backlMode) {
    case BACKL_STATIC:
    case BACKL_PULS:
    case BACKL_RUNNING_FIRE:
    case BACKL_WAVE:
      if (fastSettings.backlColor < 253) {
        fastSettings.backlColor -= fastSettings.backlColor % 50;
        if (fastSettings.backlColor < 200) fastSettings.backlColor += 50;
        else fastSettings.backlColor = 253;
      }
      else fastSettings.backlColor++;

      if (fastSettings.backlColor) { //если не начальный цвет
        wsBacklSetLedHue(fastSettings.backlColor, WHITE_ON); //устанавливаем статичный цвет
        return; //выходим
      }
      break;
  }
#endif

  if (++fastSettings.backlMode >= BACKL_EFFECT_NUM) fastSettings.backlMode = 0; //переключили режим подсветки
  switch (fastSettings.backlMode) {
#if BACKL_TYPE == 3
    case BACKL_OFF:
      wsBacklClearLeds(); //выключили светодиоды
      break;
    case BACKL_STATIC:
      wsBacklSetLedBright(backl.maxBright); //устанавливаем максимальную яркость
      wsBacklSetLedHue(fastSettings.backlColor, WHITE_ON); //устанавливаем статичный цвет
      break;
    case BACKL_PULS:
      wsBacklSetLedBright(backl.maxBright ? backl.minBright : 0); //устанавливаем минимальную яркость
      wsBacklSetLedHue(fastSettings.backlColor, WHITE_ON); //устанавливаем статичный цвет
      break;
    case BACKL_RUNNING_FIRE:
      wsBacklSetLedBright(0); //устанавливаем минимальную яркость
      wsBacklSetLedHue(fastSettings.backlColor, WHITE_ON); //устанавливаем статичный цвет
      break;
    case BACKL_WAVE:
      wsBacklSetLedBright(backl.maxBright ? backl.minBright : 0); //устанавливаем минимальную яркость
      wsBacklSetLedHue(fastSettings.backlColor, WHITE_ON); //устанавливаем статичный цвет
      break;
    case BACKL_SMOOTH_COLOR_CHANGE:
      wsBacklSetLedBright(backl.maxBright); //устанавливаем максимальную яркость
      break;
#else
    case BACKL_OFF: ledBacklSetBright(0); break; //выключаем подсветку
    case BACKL_STATIC: ledBacklSetBright(backl.maxBright); break; //включаем подсветку
    case BACKL_PULS: ledBacklSetBright(backl.maxBright ? backl.minBright : 0); break; //выключаем подсветку
#endif
  }
}
//-------------------------Сменить цвет режима анимации подсвеетки быстрых настроек----------------------------
void changeFastSetColor(void) //сменить цвет режима анимации подсвеетки быстрых настроек
{
  if (fastSettings.backlColor < 250) fastSettings.backlColor += 10;
  else if (fastSettings.backlColor == 250) fastSettings.backlColor = 253;
  else fastSettings.backlColor++;
  wsBacklSetLedHue(fastSettings.backlColor, WHITE_ON); //устанавливаем статичный цвет
}
//-------------------------------Получить значение быстрых настроек---------------------------------
uint8_t getFastSetData(uint8_t pos) //получить значение быстрых настроек
{
  switch (pos) {
    case FAST_FLIP_MODE: return fastSettings.flipMode; //вывод режима смены минут
#if LAMP_NUM > 4
    case FAST_SECS_MODE: return fastSettings.secsMode; //вывод режима смены секунд
#endif
    case FAST_DOT_MODE: return fastSettings.dotMode; //вывод режима секундных точек
#if BACKL_TYPE
    case FAST_BACKL_MODE: return fastSettings.backlMode; //вывод режима подсветки
#endif
#if BACKL_TYPE == 3
    case FAST_BACKL_COLOR: return (fastSettings.backlColor >= 253) ? (fastSettings.backlColor - 227) : (fastSettings.backlColor / 10); //вывод цвета подсветки
#endif
  }
  return 0;
}
//----------------------------------Переключение быстрых настроек-----------------------------------
uint8_t fastSetSwitch(void) //переключение быстрых настроек
{
  uint8_t show = 1; //флаг запуска анимации
  uint8_t mode = FAST_FLIP_MODE; //режим быстрой настройки

  while (1) {
    if (show) {
      switch (show) {
        case 1:
#if PLAYER_TYPE
          if (mainSettings.baseSound) playerSetTrackNow(PLAYER_FAST_MENU_START + mode, PLAYER_MENU_FOLDER);
#endif
          animClearBuff(); //очистка буфера анимации
          animPrintNum(getFastSetData(mode), (LAMP_NUM / 2 - 1), 2, 0); //вывод информации
          animIndi(FLIP_GATES + 2, FLIP_NORMAL); //анимация цифр
          break;
        default:
          switch (mode) {
            case FAST_FLIP_MODE:
              changeFastSetFlip(); //сменить режим анимации минут быстрых настроек
              break;
#if LAMP_NUM > 4
            case FAST_SECS_MODE:
              changeFastSetSecs(); //сменить режим анимации секунд быстрых настроек
              break;
#endif
            case FAST_DOT_MODE:
              changeFastSetDot(); //сменить режим анимации точек быстрых настроек
              break;
#if BACKL_TYPE
            case FAST_BACKL_MODE:
              changeFastSetBackl(); //сменить режим анимации подсветки быстрых настроек
              break;
#endif
#if BACKL_TYPE == 3
            case FAST_BACKL_COLOR:
              changeFastSetColor(); //сменить цвет режима анимации подсвеетки быстрых настроек
              break;
#endif
          }
          indiClr(); //очистка индикаторов
          indiPrintNum(getFastSetData(mode), (LAMP_NUM / 2 - 1), 2, 0); //вывод информации
          break;
      }
      show = 0;
      _timer_ms[TMR_MS] = FAST_SHOW_TIME;
    }

    dataUpdate(); //обработка данных

#if ESP_ENABLE
    if (busCheck()) return MAIN_PROGRAM;
#endif

    if (!_timer_ms[TMR_MS]) break; //выходим

    switch (buttonState()) {
      case RIGHT_KEY_PRESS: //клик правой кнопкой
        if (mode != FAST_DOT_MODE) {
          show = 1; //запустить анимацию
          mode = FAST_DOT_MODE; //демострация текущего режима работы
        }
        else show = 2; //изменить и отобразить данные
        break;

#if BACKL_TYPE
      case LEFT_KEY_PRESS: //клик левой кнопкой
#if BACKL_TYPE == 3
        if ((mode != FAST_BACKL_MODE) && (mode != FAST_BACKL_COLOR)) {
#else
        if (mode != FAST_BACKL_MODE) {
#endif
          show = 1; //запустить анимацию
          mode = FAST_BACKL_MODE; //демострация текущего режима работы
        }
        else show = 2; //изменить и отобразить данные
        break;

#if BACKL_TYPE == 3
      case LEFT_KEY_HOLD: //удержание левой кнопки
        if (mode == FAST_BACKL_MODE) {
          switch (fastSettings.backlMode) {
            case BACKL_STATIC:
            case BACKL_PULS:
            case BACKL_RUNNING_FIRE:
            case BACKL_WAVE:
              show = 1; //запустить анимацию
              mode = FAST_BACKL_COLOR;
              break;
          }
        }
        break;
#endif
#endif

      case SET_KEY_PRESS: //клик средней кнопкой
#if (LAMP_NUM > 4) && !BTN_ADD_TYPE
        if ((mode != FAST_FLIP_MODE) && (mode != FAST_SECS_MODE)) {
#else
        if (mode != FAST_FLIP_MODE) {
#endif
          show = 1; //запустить анимацию
          mode = FAST_FLIP_MODE; //демострация текущего режима работы
        }
        else show = 2; //изменить и отобразить данные
        break;

#if (LAMP_NUM > 4)
#if BTN_ADD_TYPE
      case ADD_KEY_PRESS: //клик доп кнопкой
#else
      case SET_KEY_HOLD: //удержание правой кнопки
#endif
        if (mode != FAST_SECS_MODE) {
          show = 1; //запустить анимацию
          mode = FAST_SECS_MODE; //демострация текущего режима работы
        }
#if BTN_ADD_TYPE
        else show = 2; //изменить и отобразить данные
#endif
        break;
#endif
    }
  }
  
  if (mode == FAST_FLIP_MODE) animShow = ANIM_DEMO; //демонстрация анимации цифр
  setUpdateMemory(CELL(MEM_UPDATE_FAST_SET)); //записываем настройки в память
  
  return MAIN_PROGRAM; //выходим
}
//-----------------------------Главный экран------------------------------------------------
uint8_t mainScreen(void) //главный экран
{
  if (animShow < ANIM_MAIN) animShow = ANIM_NULL; //сбрасываем флаг анимации цифр
  else if (animShow == ANIM_DEMO) animIndi(fastSettings.flipMode, FLIP_DEMO); //демонстрация анимации цифр
  else if (animShow >= ANIM_OTHER) animIndi(animShow - ANIM_OTHER, FLIP_TIME); //анимация цифр

  if (indi.sleepMode) { //если активен режим сна
    if (!changeAnimState) sleepReset(); //установли время ожидания режима сна
    else if (_timer_sec[TMR_SLEEP] < RESET_TIME_SLEEP) _timer_sec[TMR_SLEEP] = RESET_TIME_SLEEP; //установли минимальное время ожидания режима сна
  }

  if (_timer_sec[TMR_GLITCH] < RESET_TIME_GLITCH) _timer_sec[TMR_GLITCH] = RESET_TIME_GLITCH; //если время вышло то устанавливаем минимальное время
  if (_timer_sec[TMR_BURN] < RESET_TIME_BURN) _timer_sec[TMR_BURN] = RESET_TIME_BURN; //если время вышло то устанавливаем минимальное время
  if (_timer_sec[TMR_SHOW] < RESET_TIME_SHOW) _timer_sec[TMR_SHOW] = RESET_TIME_SHOW; //если время вышло то устанавливаем минимальное время

  changeAnimState = ANIM_RESET_ALL; //сбрасываем флаг изменения ремов анимации

  for (;;) { //основной цикл
    dataUpdate(); //обработка данных

#if ESP_ENABLE
    if (busCheck() & ~(0x01 << BUS_COMMAND_WAIT)) { //обновление шины
      if (!changeAnimState) changeAnimState = ANIM_RESET_CHANGE; //установили тип сброса анимации
      return MAIN_PROGRAM; //перезапуск основной программы
    }
#endif

#if RADIO_ENABLE && IR_PORT_ENABLE && IR_EXT_BTN_ENABLE
    if (radioFastSettings() == 1) return MAIN_PROGRAM; //перезапуск основной программы
#endif

    if (!indi.update) { //если пришло время обновить индикаторы
#if ALARM_TYPE
#if INDI_SYMB_TYPE
      if (alarms.now != ALARM_DISABLE) indiSetSymb(SYMB_ALARM); //установка индикатора символов
      else indiClrSymb(); //очистка индикатора символов
#endif
      if (alarms.now == ALARM_WARN) return ALARM_PROGRAM; //тревога будильника
#endif
#if TIMER_ENABLE && (BTN_ADD_TYPE || IR_PORT_ENABLE)
      if ((timer.mode == 2) && !timer.count) return WARN_PROGRAM; //тревога таймера
#endif

      if (indi.sleepMode != SLEEP_NIGHT) { //если режим сна не ночной
        if (!_timer_sec[TMR_BURN]) { //если пришло время отобразить анимацию антиотравления
#if BURN_BRIGHT
          changeBrightDisable(CHANGE_INDI_BLOCK); //запретить смену яркости индикаторов
          indiSetBright(BURN_BRIGHT); //установка общей яркости индикаторов
#endif
          if (mainSettings.burnMode != BURN_SINGLE_TIME) mainTask = SLEEP_PROGRAM; //подмена текущей программы
          burnIndi(mainSettings.burnMode, BURN_NORMAL); //антиотравление индикаторов
          _timer_sec[TMR_BURN] = getPhaseTime(mainSettings.burnTime, BURN_PHASE); //установка таймера антиотравления
          if (mainSettings.burnMode != BURN_SINGLE_TIME) changeAnimState = ANIM_RESET_DOT; //установили тип сброса анимации
          else changeAnimState = ANIM_RESET_CHANGE; //установили тип сброса анимации
          return MAIN_PROGRAM; //перезапуск основной программы
        }

        if (mainSettings.autoShowTime && !_timer_sec[TMR_SHOW]) { //если пришло время отобразить температуру
          mainTask = SLEEP_PROGRAM; //подмена текущей программы
          autoShowMenu(); //автоматическое отображение данных
          _timer_sec[TMR_SHOW] = getPhaseTime(mainSettings.autoShowTime, AUTO_SHOW_PHASE); //установка таймера показа температуры
          changeAnimState = ANIM_RESET_DOT; //установили тип сброса анимации
          return MAIN_PROGRAM; //перезапуск основной программы
        }

        if (animShow >= ANIM_MINS) animIndi(fastSettings.flipMode, FLIP_TIME); //анимация минут
      }
      else animShow = ANIM_NULL; //сбрасываем флаг анимации
      if (indi.sleepMode && !_timer_sec[TMR_SLEEP]) return SLEEP_PROGRAM; //режим сна индикаторов

      indiPrintNum((mainSettings.timeFormat) ? get_12h(RTC.h) : RTC.h, 0, 2, 0); //вывод часов
      indiPrintNum(RTC.m, 2, 2, 0); //вывод минут
#if LAMP_NUM > 4
      if (animShow != ANIM_SECS) indiPrintNum(RTC.s, 4, 2, 0); //вывод секунд
#endif
      indi.update = 1; //сбрасываем флаг

      glitchIndi(); //имитация глюков
    }

#if LAMP_NUM > 4
    flipSecs(); //анимация секунд
#endif

    //+++++++++++++++++++++  опрос кнопок  +++++++++++++++++++++++++++
    switch (buttonState()) {
#if BTN_EASY_MAIN_MODE //упрощенный режим
#if BACKL_TYPE
      case LEFT_KEY_PRESS: //клик левой кнопкой
        if (indi.sleepMode) sleepReset(); //сброс режима сна
        changeFastSetBackl(); //сменить режим анимации подсветки быстрых настроек
        setUpdateMemory(CELL(MEM_UPDATE_FAST_SET)); //записываем настройки в память
        break;
#endif

#if ALARM_TYPE
      case LEFT_KEY_HOLD: //удержание левой кнопки
        return ALARM_SET_PROGRAM; //настройка будильника
#endif

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        if (indi.sleepMode) sleepReset(); //сброс режима сна
        changeFastSetDot(); //сменить режим анимации точек быстрых настроек
        setUpdateMemory(CELL(MEM_UPDATE_FAST_SET)); //записываем настройки в память
        break;

      case RIGHT_KEY_HOLD: //удержание правой кнопки
        return CLOCK_SET_PROGRAM; //иначе настройки времени

      case SET_KEY_PRESS: //клик средней кнопкой
        changeFastSetFlip(); //сменить режим анимации минут быстрых настроек
        setUpdateMemory(CELL(MEM_UPDATE_FAST_SET)); //записываем настройки в память
        animShow = ANIM_DEMO; //демонстрация анимации цифр
        return MAIN_PROGRAM; //перезапуск основной программы

      case SET_KEY_HOLD: //удержание средней кнопки
#if ALARM_TYPE
        if (alarms.now == ALARM_WAIT) { //если будильник в режиме ожидания
          alarmDisable(); //отключение будильника
          break;
        }
#endif
#if LAMP_NUM > 4
        changeFastSetSecs(); //сменить режим анимации секунд быстрых настроек
        setUpdateMemory(CELL(MEM_UPDATE_FAST_SET)); //записываем настройки в память
        changeAnimState = ANIM_RESET_CHANGE; //установили тип сброса анимации
        return MAIN_PROGRAM; //перезапуск основной программы
#else
        break;
#endif
#else //обычный режим
#if (DS3231_ENABLE == 2) || SENS_AHT_ENABLE || SENS_SHT_ENABLE || SENS_BME_ENABLE || SENS_PORT_ENABLE || ESP_ENABLE
      case LEFT_KEY_PRESS: //клик левой кнопкой
        return TEMP_PROGRAM; //показать температуру
#endif

#if ALARM_TYPE
      case LEFT_KEY_HOLD: //удержание левой кнопки
        return ALARM_SET_PROGRAM; //настройка будильника
#endif

      case RIGHT_KEY_PRESS: //клик правой кнопкой
        return DATE_PROGRAM; //показать дату

      case RIGHT_KEY_HOLD: //удержание правой кнопки
        return CLOCK_SET_PROGRAM; //иначе настройки времени

      case SET_KEY_PRESS: //клик средней кнопкой
        return FAST_SET_PROGRAM; //переключение настроек

      case SET_KEY_HOLD: //удержание средней кнопки
#if ALARM_TYPE
        if (alarms.now == ALARM_WAIT) { //если будильник в режиме ожидания
          alarmDisable(); //отключение будильника
          break;
        }
#endif
        return MAIN_SET_PROGRAM; //настроки основные
#endif

#if BTN_ADD_TYPE || IR_PORT_ENABLE
#if TIMER_ENABLE
      case ADD_KEY_PRESS: //клик дополнительной кнопкой
        return TIMER_PROGRAM; //таймер-секундомер
#elif RADIO_ENABLE
      case ADD_KEY_PRESS: //клик дополнительной кнопкой
        return RADIO_PROGRAM; //радиоприемник
#endif
      case ADD_KEY_HOLD: //удержание дополнительной кнопки
#if ALARM_TYPE
        if (alarms.now == ALARM_WAIT) { //если будильник в режиме ожидания
          alarmDisable(); //отключение будильника
          break;
        }
#endif
#if RADIO_ENABLE && TIMER_ENABLE
        return RADIO_PROGRAM; //радиоприемник
#else
        break;
#endif
#endif
    }
  }
  return INIT_PROGRAM;
}
