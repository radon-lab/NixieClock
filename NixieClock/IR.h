#define IR_COMMAND_START_TIME 26 //сингнал начала команды
#define IR_REPEAT_START_TIME 10 //сигнал начала повтора команды
#define IR_DATA_TIME 5 //сигнал бита команды

#define IR_READY 0x01 //флаг новой команды
#define IR_DISABLE 0x02 //флаг запрета чтения команды

volatile uint8_t irTime; //время сигнала от ИК приемника
volatile uint8_t irState; //текущее состояние ИК приемника
volatile uint16_t irCommand; //текущая команда от пульта

enum {
  IR_IDLE, //ожидание команды
  IR_READ, //чтение данных
  IR_REPEAT //повтор последней команды
};

//------------------------------Инициализация IR приемника------------------------------
void irInit(void) //инициализация IR приемника
{
  IR_INIT; //инициализация ИК приемника
  DECODE_PCMSK(IR_PIN) |= (0x01 << IR_BIT); //настраиваем маску прерывания порта
  PCICR |= (0x01 << DECODE_PCIE(IR_PIN)); //разрешаем прерывание порта
}
//-------------------------------Проверка принятой команды-------------------------------
void irCheckCommand(uint8_t* _command) //проверка принятой команды
{
  if (_command[0] != (_command[1] ^ 0xFF)) return; //проверяем контрольную сумму адреса
  if (_command[2] != (_command[3] ^ 0xFF)) return; //проверяем контрольную сумму команды
  irState |= IR_READY; //установили флаг готовности
  irCommand = ((uint16_t)_command[0] << 8) | _command[2]; //скопировали команду
}
#if IR_PORT_ENABLE
//-----------------------------Счетчик времени IR приемника-----------------------------
ISR(TIMER2_OVF_vect) //счетчик времени IR приемника
{
  if (!++irTime) TIMSK2 &= ~(0x01 << TOIE2); //если таймаут то выключаем таймер
}
//----------------------------Обработка сигнала IR приемника----------------------------
#if IR_PIN < 8
ISR(PCINT2_vect) //обработка сигнала IR приемника
#else
ISR(PCINT0_vect) //обработка сигнала IR приемника
#endif
{
  static uint8_t _bit; //текущий бит
  static uint8_t _byte; //текущий байт
  static uint8_t _data; //буфер приема данных
  static uint8_t _state; //текущее состояние
  static uint8_t _command[4]; //массив команды

  if (!IR_CHK) { //если на пине низкий уровень
    if (!irTime) { //если начало передачи
      _state = IR_IDLE; //возвращаемся в ожидание команды
      TIMSK2 |= (0x01 << TOIE2); //включаем таймер
    }
    else {
      switch (_state) {
        case IR_IDLE:
          if (irTime > IR_COMMAND_START_TIME) { //если режим чтения
            _bit = 0; //сбросили текущий бит
            _byte = 0; //сбросили текущий байт
            _state = IR_READ; //перешли в режим чтения данных
          }
          else if (irTime > IR_REPEAT_START_TIME) _state = IR_REPEAT; //иначе режим повтора
          break;
        case IR_READ:
          _data >>= 0x01; //сдвинули байт
          _data |= (irTime > IR_DATA_TIME) ? 0x80 : 0x00; //записали бит

          if (++_bit >= 8) { //если все биты приняты
            _command[_byte] = _data; //копируем байт в буфер
            _bit = 0; //сбросили текущий бит

            if (++_byte >= 4) { //если все байты приняты
              TIMSK2 &= ~(0x01 << TOIE2); //выключаем таймер
              irCheckCommand(_command); //проверяем контрольную сумму команды
#if IR_REPEAT_ENABLE
              _timer_ms[TMR_IR] = (IR_HOLD_TIME + IR_REPEAT_WAIT_TIME); //устанавливаем таймер удержания
#endif
              _state = IR_IDLE; //переходим в режим ожидания команды
            }
          }
          break;
        case IR_REPEAT:
          TIMSK2 &= ~(0x01 << TOIE2); //выключаем таймер
#if IR_REPEAT_ENABLE
          if (_timer_ms[TMR_IR] && _timer_ms[TMR_IR] < IR_REPEAT_TIME) {
            if (irTime < IR_DATA_TIME) { //если таймер истек
              irCheckCommand(_command); //проверяем контрольную сумму команды
              _timer_ms[TMR_IR] = (IR_REPEAT_WAIT_TIME + IR_REPEAT_TIME); //устанавливаем таймер повтора
            }
          }
#endif
          _state = IR_IDLE; //переходим в режим ожидания команды
          break;
      }
    }
  }
  irTime = 0; //сбросили время сигнала
}
#endif
