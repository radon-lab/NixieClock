#define IR_COMMAND_TIME 26
#define IR_REPEAT_TIME 10
#define IR_DATA_TIME 5

boolean irDisable;
volatile uint8_t irTime;
volatile uint8_t irCommand;

enum {
  IR_IDLE,
  IR_READ,
  IR_REPEAT
};

//------------------------------Инициализация IR приемника------------------------------
void irInit(void)
{
  IR_INIT;
  DECODE_PCMSK(IR_PIN) |= (0x01 << IR_BIT);
  PCICR |= (0x01 << DECODE_PCIE(IR_PIN));
}
#if IR_PORT_ENABLE
//-----------------------------Счетчик времени IR приемника-----------------------------
ISR(TIMER2_OVF_vect)
{
  if (!++irTime) TIMSK2 &= ~(0x01 << TOIE2); //выключаем таймер
}
//----------------------------Обработка сигнала IR приемника----------------------------
ISR(PCINT2_vect)
{
  static uint8_t _bit;
  static uint8_t _byte;
  static uint8_t _data;
  static uint8_t _state;
  static uint8_t _command[4];

  if (!IR_CHK) {
    if (!irTime) {
      _state = IR_IDLE;
      TIMSK2 |= (0x01 << TOIE2); //включаем таймер
    }
    else {
      switch (_state) {
        case IR_IDLE:
          if (irTime > IR_COMMAND_TIME) { //режим чтения
            _bit = 0;
            _byte = 0;
            _state = IR_READ;
          }
          else if (irTime > IR_REPEAT_TIME) _state = IR_REPEAT; //режим повтора
          break;
        case IR_READ:
          _data >>= 0x01;
          _data |= (irTime > IR_DATA_TIME) ? 0x80 : 0x00;

          if (++_bit >= 8) {
            _command[_byte] = _data;
            _bit = 0;

            if (++_byte >= 4) {
              TIMSK2 &= ~(0x01 << TOIE2); //выключаем таймер
              if (_command[2] == (_command[3] ^ 0xFF)) irCommand = _command[2];
              _timer_ms[TMR_IR] = IR_HOLD_TIME;
              _state = IR_IDLE;
            }
          }
          break;
        case IR_REPEAT:
          TIMSK2 &= ~(0x01 << TOIE2); //выключаем таймер
#if IR_HOLD_ENABLE
          if (irTime < IR_DATA_TIME && !_timer_ms[TMR_IR] && (_command[2] == (_command[3] ^ 0xFF))) irCommand = _command[2];
#endif
          _state = IR_IDLE;
          break;
      }
    }
  }
  irTime = 0;
}
#endif
