#define US_PERIOD (uint16_t)(((uint16_t)FREQ_TICK + 1) * 16.0) //период тика таймера в мкс
#define US_PERIOD_MIN (uint16_t)(US_PERIOD - (US_PERIOD % 100) - 400) //минимальный период тика таймера
#define US_PERIOD_MAX (uint16_t)(US_PERIOD - (US_PERIOD % 100) + 400) //максимальный период тика таймера

#define MS_PERIOD (uint8_t)(US_PERIOD / 1000) //период тика таймера в целых мс

#define LIGHT_MAX (uint8_t)(FREQ_TICK - INDI_DEAD_TIME) //расчет максимального шага яркости
#define DOT_LIGHT_MAX (uint8_t)(CONSTRAIN(((uint16_t)LIGHT_MAX - 2) + (LIGHT_MAX >> 5), 100, 255)) //расчет максимального шага яркости для точек
#define INDI_LIGHT_MAX (uint16_t)(((uint16_t)LIGHT_MAX * 8) + (LIGHT_MAX >> 1)) //расчет максимального шага яркости для индикаторов

#define R_COEF(low, high) (((float)(low) + (float)(high)) / (float)(low)) //коэффициент делителя напряжения
#define HV_ADC(vcc) (uint16_t)((1023.0 / (float)(vcc)) * ((float)GEN_HV_VCC / (float)R_COEF(GEN_HV_R_LOW, GEN_HV_R_HIGH))) //значение ацп удержания напряжения
#define GET_VCC(ref, adc) (float)(((ref) * 1023.0) / (float)(adc)) //расчет напряжения питания

#define RESET_BOOTLOADER __asm__ __volatile__ ("JMP 0x7E00") //загрузчик
#define RESET_SYSTEM __asm__ __volatile__ ("JMP 0x0000") //перезагрузка
#define RESET_WDT __asm__ __volatile__ ("WDR") //сброс WDT

//перечисления кнопок
enum {
  KEY_NULL, //кнопка не нажата
  LEFT_KEY_PRESS, //клик левой кнопкой
  LEFT_KEY_HOLD, //удержание левой кнопки
  RIGHT_KEY_PRESS, //клик правой кнопкой
  RIGHT_KEY_HOLD, //удержание правой кнопки
  SET_KEY_PRESS, //клик средней кнопкой
  SET_KEY_HOLD, //удержание средней кнопки
  ADD_KEY_PRESS, //клик дополнительной кнопкой
  ADD_KEY_HOLD, //удержание дополнительной кнопки
#if IR_EXT_BTN_ENABLE
#if IR_PORT_ENABLE && RADIO_ENABLE
  PWR_KEY_PRESS, //клик кнопки питания
  VOL_UP_KEY_PRESS, //клик кнопки громкости вверх
  VOL_DOWN_KEY_PRESS, //клик кнопки громкости вниз
  STATION_UP_KEY_PRESS, //клик кнопки станции вверх
  STATION_DOWN_KEY_PRESS, //клик кнопки станции вниз
#if IR_EXT_BTN_ENABLE == 2
  STATION_CELL_0_PRESS, //клик кнопки станции 0
  STATION_CELL_1_PRESS, //клик кнопки станции 1
  STATION_CELL_2_PRESS, //клик кнопки станции 2
  STATION_CELL_3_PRESS, //клик кнопки станции 3
  STATION_CELL_4_PRESS, //клик кнопки станции 4
  STATION_CELL_5_PRESS, //клик кнопки станции 5
  STATION_CELL_6_PRESS, //клик кнопки станции 6
  STATION_CELL_7_PRESS, //клик кнопки станции 7
  STATION_CELL_8_PRESS, //клик кнопки станции 8
  STATION_CELL_9_PRESS, //клик кнопки станции 9
#endif
#endif
#endif
  KEY_MAX_ITEMS //максимум кнопок
};

struct Settings_5 {
  uint16_t irButtons[KEY_MAX_ITEMS - 1]; //коды кнопок пульта
  uint16_t timePeriod = US_PERIOD; //коррекция хода внутреннего осцилятора
  uint8_t min_pwm = DEFAULT_MIN_PWM; //минимальный шим
  uint8_t max_pwm = DEFAULT_MAX_PWM; //максимальный шим
  uint8_t light_zone[2][3]; //зоны яркости датчика освещения
  int8_t hvCorrect; //коррекция напряжения
  int8_t aging; //коррекция регистра старения
} debugSettings;

uint8_t pwm_coef; //коэффициент линейного регулирования
uint16_t hv_treshold = HV_ADC(5); //буфер сравнения напряжения

uint8_t light_adc; //значение АЦП сенсора яркости освещения
uint8_t light_state = 2; //состояние сенсора яркости освещения
boolean light_update = 0; //флаг обновления яркости

//-----------------------------Прерывание от RTC--------------------------------
#if SQW_PORT_ENABLE
ISR(INT0_vect) //внешнее прерывание на пине INT0 - считаем секунды с RTC
{
  tick_sec++; //прибавляем секунду
}
#endif
//---------------------------Первичный запуск WDT-------------------------------
inline void startEnableWDT(void) //первичный запуск WDT
{
  cli(); //запрещаем прерывания
  RESET_WDT; //сбрасываем таймер WDT
  MCUSR &= ~(0x01 << WDRF); //сбросли флаг перезагрузки WDT
  WDTCSR = (0x01 << WDCE) | (0x01 << WDE); //разрешаем изменения WDT
  WDTCSR = (0x01 << WDE) | (0x01 << WDP3) | (0x01 << WDP0); //устанавливаем таймер WDT на 8сек
}
//---------------------------Основной запуск WDT--------------------------------
inline void mainEnableWDT(void) //основной запуск WDT
{
  RESET_WDT; //сбрасываем таймер WDT
  WDTCSR = (0x01 << WDCE) | (0x01 << WDE); //разрешаем изменения WDT
  WDTCSR = (0x01 << WDE) | (0x01 << WDP2) | (0x01 << WDP1) | (0x01 << WDP0); //устанавливаем таймер WDT на 2сек
}
//------------------------------Проверка переполнения тиков---------------------------------------
inline void tickCheck(void) //проверка переполнения тиков
{
  if (!++tick_ms) { //если превышено количество тиков
    SET_ERROR(TICK_OVF_ERROR); //устанавливаем ошибку переполнения тиков времени
    SET_ERROR(RESET_ERROR); //устанавливаем ошибку аварийной перезагрузки
    RESET_SYSTEM; //перезагрузка
  }
}
//------------------------------Проверка переполнения стека---------------------------------------
inline void stackCheck(void) //проверка переполнения стека
{
  if (!(SPH & 0xFC)) { //если стек переполнен
    SET_ERROR(STACK_OVF_ERROR); //устанавливаем ошибку переполнения стека
    SET_ERROR(RESET_ERROR); //устанавливаем ошибку аварийной перезагрузки
    RESET_SYSTEM; //перезагрузка
  }
}
//----------------------------Проверка состояния преобразователя----------------------------------
void converterCheck(void) //проверка состояния преобразователя
{
#if GEN_FEEDBACK
  if (((TCCR1A & 0x4F) != (0x01 << WGM10)) ||
#else
#if CONV_PIN == 9
  if (((TCCR1A & 0xCF) != ((0x01 << WGM10) | (0x01 << COM1A1))) ||
#elif CONV_PIN == 10
  if (((TCCR1A & 0x3F) != ((0x01 << WGM10) | (0x01 << COM1B1))) ||
#endif
#endif
#if GEN_SPEED_X2
      (TCCR1B != ((0x01 << CS10) | (0x01 << WGM12))))
#else
      (TCCR1B != (0x01 << CS10)))
#endif
  {
    TCCR1A = TCCR1B = 0; //выключаем шим
    CONV_DISABLE; //выключаем преобразователь
    SET_ERROR(CONVERTER_ERROR); //устанавливаем ошибку сбоя работы преобразователя
    SET_ERROR(RESET_ERROR); //устанавливаем ошибку аварийной перезагрузки
    RESET_SYSTEM; //перезагрузка
  }
#if CONV_PIN == 9
  if (OCR1A > 200) { //если вышли за предел
    OCR1A = 200; //установили максимум
    SET_ERROR(PWM_OVF_ERROR); //устанавливаем ошибку переполнения заполнения шим преобразователя
  }
#elif CONV_PIN == 10
  if (OCR1B > 200) { //если вышли за предел
    OCR1B = 200; //установили максимум
    SET_ERROR(PWM_OVF_ERROR); //устанавливаем ошибку переполнения заполнения шим преобразователя
  }
#endif
}
//---------------------------------Инициализация периферии ядра----------------------------------------
void coreInit(void) //инициализация периферии ядра
{
  cli(); //запрещаем прерывания глобально

#if INDI_PORT_TYPE
  DDRB |= 0x04; //установили D10 как выход

  SPSR = (0x01 << SPI2X); //включили удвоение скорости
  SPCR = (0x01 << SPE) | (0x01 << MSTR); //запустили SPI в режиме ведущего
#endif

  EIMSK = 0; //запретили прерывания INT0/INT1

#if !IR_PORT_ENABLE
  PCICR = 0; //запретили прерывания PCINT0/PCINT1/PCINT2
#endif

#if PLAYER_TYPE == 2
#if BUZZ_PIN == 9
  OCR1A = 128; //выключаем dac
#elif BUZZ_PIN == 10
  OCR1B = 128; //выключаем dac
#endif
#endif
#if !SECS_DOT
#if SECL_PIN == 9
  OCR1A = 0; //выключаем точки
#elif SECL_PIN == 10
  OCR1B = 0; //выключаем точки
#endif
#endif

  TIMSK1 = 0; //отключаем прерывания
  TCCR1A = (0x01 << WGM10); //режим коррекции фазы шим
#if GEN_SPEED_X2
  TCCR1B = (0x01 << CS10) | (0x01 << WGM12);  //задаем частоту 62 кГц
#else
  TCCR1B = (0x01 << CS10);  //задаем частоту 31 кГц
#endif

#if PLAYER_TYPE == 2
#if BUZZ_PIN == 9
  TCCR1A |= (0x01 << COM1A1); //подключаем D9
#elif BUZZ_PIN == 10
  TCCR1A |= (0x01 << COM1B1); //подключаем D10
#endif
#endif
#if !SECS_DOT
#if SECL_PIN == 9
  TCCR1A |= (0x01 << COM1A1); //подключаем D9
#elif SECL_PIN == 10
  TCCR1A |= (0x01 << COM1B1); //подключаем D10
#endif
#endif

#if GEN_ENABLE
#if CONV_PIN == 9
  OCR1A = CONSTRAIN(debugSettings.min_pwm, 100, 200); //устанавливаем первичное значение шим
  TCCR1A |= (0x01 << COM1A1); //подключаем D9
#elif CONV_PIN == 10
  OCR1B = CONSTRAIN(debugSettings.min_pwm, 100, 200); //устанавливаем первичное значение шим
  TCCR1A |= (0x01 << COM1B1); //подключаем D10
#endif
#endif

  OCR2A = 0; //выключаем подсветку
  OCR2B = 0; //сбрасывем бузер

  TIMSK2 = 0; //отключаем прерывания
#if (BACKL_TYPE != 1)
  TCCR2A = 0; //отключаем OCR2A и OCR2B
#else
  TCCR2A = (0x01 << COM2A1 | 0x01 << WGM20 | 0x01 << WGM21); //подключаем D11
#endif
  TCCR2B = (0x01 << CS21); //пределитель 8

  sei(); //разрешаем прерывания глобально
}
