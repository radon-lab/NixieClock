#define IR_COMMAND_TIME 26 //сингнал начала команды
#define IR_REPEAT_TIME 10  //сигнал начала повтора команды
#define IR_DATA_TIME 5     //сигнал бита команды

boolean irDisable; //флаг запрета работы с ИК пультом
volatile uint8_t irTime; //время сигнала от ИК приемника
volatile uint8_t irCommand; //текущая команда от пульта

enum {
  IR_IDLE, //ожидание команды
  IR_READ, //чтение данных
  IR_REPEAT //повтор последней команды
};

//------------------------------Инициализация IR приемника------------------------------
void irInit(void)
{
  IR_INIT; //инициализация ИК приемника
  DECODE_PCMSK(IR_PIN) |= (0x01 << IR_BIT); //настраиваем маску прерывания порта
  PCICR |= (0x01 << DECODE_PCIE(IR_PIN)); //разрешаем прерывание порта
}
#if IR_PORT_ENABLE
//-----------------------------Счетчик времени IR приемника-----------------------------
ISR(TIMER2_OVF_vect)
{
  if (!++irTime) TIMSK2 &= ~(0x01 << TOIE2); //если таймаут то выключаем таймер
}
//----------------------------Обработка сигнала IR приемника----------------------------
ISR(PCINT2_vect)
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
          if (irTime > IR_COMMAND_TIME) { //если режим чтения
            _bit = 0; //сбросили текущий бит
            _byte = 0; //сбросили текущий байт
            _state = IR_READ; //перешли в режим чтения данных
          }
          else if (irTime > IR_REPEAT_TIME) _state = IR_REPEAT; //иначе режим повтора
          break;
        case IR_READ:
          _data >>= 0x01; //сдвинули байт
          _data |= (irTime > IR_DATA_TIME) ? 0x80 : 0x00; //записали бит

          if (++_bit >= 8) { //если все биты приняты
            _command[_byte] = _data; //копируем байт в буфер
            _bit = 0; //сбросили текущий бит

            if (++_byte >= 4) { //если все байты приняты
              TIMSK2 &= ~(0x01 << TOIE2); //выключаем таймер
              if (_command[2] == (_command[3] ^ 0xFF)) irCommand = _command[2]; //проверяем контрольную сумму команды
              _timer_ms[TMR_IR] = IR_HOLD_TIME; //устанавливаем таймер повтора
              _state = IR_IDLE; //переходим в режим ожидания команды
            }
          }
          break;
        case IR_REPEAT:
          TIMSK2 &= ~(0x01 << TOIE2); //выключаем таймер
#if IR_HOLD_ENABLE
          if (irTime < IR_DATA_TIME && !_timer_ms[TMR_IR] && (_command[2] == (_command[3] ^ 0xFF))) irCommand = _command[2]; //если таймер истек и контрольная сумма совпала
#endif
          _state = IR_IDLE; //переходим в режим ожидания команды
          break;
      }
    }
  }
  irTime = 0; //сбросили время сигнала
}
#endif
