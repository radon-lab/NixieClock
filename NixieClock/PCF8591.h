#define PCF8591_ADDR 0x48       //адрес датчика

#define PCF8591_SINGLE_ENDED_INPUT      0x00
#define PCF8591_OUTPUT_MASK             0b01000000
#define PCF8591_INCR_FLAG               0x04

#define LIGTH_VALUE   (((uint8_t *)&analogInputChannelsPCF)[0]) // значени от 0 до 255, где большее значение - выше яркость
#define TERMISTOR_VALUE   (((uint8_t *)&analogInputChannelsPCF)[1]) // значени от 0 до 255
#if (BTN_ADD_TYPE == 3)
#define ADD_CHK       (((uint8_t *)&analogInputChannelsPCF)[2] < 0x80) // при нажатой кнопке значение ны выходе 207, при отжатой - 0-1, поэтому сраниваем со значение в диапазоне от 1-207
#endif

#define INPUT_VOLTAGE_VALUE   (((uint8_t *)&analogInputChannelsPCF)[3]) // значени от 0 до 255

// Структура данных для хранения последних прочитанных значений
struct AnalogInputChannelsPCF {
  uint8_t ports[4];
} analogInputChannelsPCF;

//-------------------------------------- Проверка подключения чтение PCF8591 ----------------------------
bool isConnectedPCF() //проверка подключения чтение PCF8591
{
  if (wireBeginTransmission(PCF8591_ADDR, false)) return false;
  wireEnd(); //конец передачи

  return 1;
}
//-------------------------------------- Чтение значений 4-х портов PCF8591 -----------------------------
bool readAnalogInputChannelsPCFImpl(void) //чтение аналоговых портов ввода
{
  uint8_t control = PCF8591_OUTPUT_MASK | PCF8591_INCR_FLAG | PCF8591_SINGLE_ENDED_INPUT;

  if (wireBeginTransmission(PCF8591_ADDR)) return 0;
  wireWrite(control);
  wireEnd();
  if (wireRequestFrom(PCF8591_ADDR, (uint8_t) 5)) {
    return 0;
  }

  wireRead();
  for (uint8_t i = 3; i >= 0; i++) {
    if (i == 0) {
      ((uint8_t *)&analogInputChannelsPCF)[i] = wireReadEndByte();
    } else {
      ((uint8_t *)&analogInputChannelsPCF)[i] = wireRead();
    }
  }

  return 1;
}
bool readAnalogInputChannelsPCF(void) { //чтение аналоговых портов ввода
  bool res = readAnalogInputChannelsPCFImpl();

  return res;
}
//--------------------------------------  запись значения аналогового вывода PCF8591 --------------------
bool writeAnalogPCF(uint8_t value) { // запись значения в аналоговый портов вывода
  uint8_t control = PCF8591_OUTPUT_MASK;

  if (wireBeginTransmission(PCF8591_ADDR)) return 0;
  wireWrite(control);
  wireWrite(value);
  wireEnd();

  return 1;
}
