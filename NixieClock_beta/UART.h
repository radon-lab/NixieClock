#define RECEIVE_BUFFER_SIZE 64
volatile uint8_t _RECEIVE_BUFFER[RECEIVE_BUFFER_SIZE];
volatile uint8_t _RECEIVE_BUFFER_END;
volatile uint8_t _RECEIVE_BUFFER_START;

#define TRANSFER_BUFFER_SIZE 32
volatile uint8_t _TRANSFER_BUFFER[TRANSFER_BUFFER_SIZE];
volatile uint8_t _TRANSFER_BUFFER_END;
volatile uint8_t _TRANSFER_BUFFER_START;

#define _TIME_OUT 2 //таймаут приёма 8мс

#define _DELAY_START _delay = 0; OCR0B = TCNT0 - 1; TIMSK0 |= (0x01 << OCIE0B)
#define _DELAY_STOP  TIMSK0 &= ~(0x01 << OCIE0B)
volatile uint8_t _delay = 0;
//----------------------------------Инициализация UART----------------------------------
void dataChannelInit(uint32_t baudrate) //инициализация UART
{
  UBRR0 = (F_CPU / (8UL * baudrate)) - 1; //устанавливаем битрейт
  UCSR0A = (0x01 << U2X0); //устанавливаем удвоение скорости
  UCSR0B = ((0x01 << TXEN0) | (0x01 << RXEN0) | (0x01 << RXCIE0));
  UCSR0C = ((0x01 << UCSZ01) | (0x01 << UCSZ00));

  _RECEIVE_BUFFER_END = _RECEIVE_BUFFER_START = 0;
  _TRANSFER_BUFFER_END = _TRANSFER_BUFFER_START = 0;
}
ISR(TIMER0_COMPB_vect) {
  switch (++_delay) {
    case _TIME_OUT: _DELAY_STOP; break;
  }
}
//----------------------------------Выключение UART----------------------------------
void dataChannelEnd() //выключение UART
{
  UCSR0B = 0; //выключаем UART
}
//----------------------------------Прерывание принятого байта----------------------------------
ISR(USART_RX_vect) //прерывание принятого байта
{
  uint8_t data, head;

  data = UDR0; //читаем байт из регистра
  if (_RECEIVE_BUFFER_END + 1 == RECEIVE_BUFFER_SIZE) head = 0; //если указатель переполнен устанавливаем в 0
  else head = _RECEIVE_BUFFER_END + 1; //иначе прибавляем значение

  if (head != _RECEIVE_BUFFER_START) { //если буфер не переполнен
    _RECEIVE_BUFFER[_RECEIVE_BUFFER_END] = data; //записываем байт
    _RECEIVE_BUFFER_END = head; //сдвигаем конец
    _DELAY_START; //перезапускаем таймер
  }
}
//----------------------------------Чтение байта из буфера приёма----------------------------------
uint8_t readData() //чтение байта из буфера приёма
{
  if (_RECEIVE_BUFFER_END == _RECEIVE_BUFFER_START) return 0; //если буфер пуст, возвращаем 0

  uint8_t c = _RECEIVE_BUFFER[_RECEIVE_BUFFER_START]; //считываем байт
  if (++_RECEIVE_BUFFER_START >= RECEIVE_BUFFER_SIZE) _RECEIVE_BUFFER_START = 0; //сдвигаем начало
  return c;
}
//----------------------------------Доступно байт для приёма----------------------------------
uint8_t availableData() //доступно байт для приёма
{
  if (_RECEIVE_BUFFER_END == _RECEIVE_BUFFER_START || (TIMSK0 & (1 << 2))) return 0; //если идет приём пакета или буфер пуст, возвращаем 0
  return ((uint16_t)(RECEIVE_BUFFER_SIZE + _RECEIVE_BUFFER_END - _RECEIVE_BUFFER_START) % RECEIVE_BUFFER_SIZE); //иначе возвращаем доступное количество байт
}
//----------------------------------Очистить буфер приёма----------------------------------
void clearBuffer() //очистить буфер приёма
{
  _RECEIVE_BUFFER_START = _RECEIVE_BUFFER_END; //очищаем оглавление
}
//----------------------------------Прерывание пустого буфера передачи----------------------------------
ISR(USART_UDRE_vect) //прерывание пустого буфера передачи
{
  UDR0 = _TRANSFER_BUFFER[_TRANSFER_BUFFER_START]; //записываем байт в буфер UART
  if (++_TRANSFER_BUFFER_START >= _TRANSFER_BUFFER_END) { //если весь пакет передан
    _TRANSFER_BUFFER_START = _TRANSFER_BUFFER_END = 0; //сбрасываем указатели передачи
    UCSR0B &= ~ (0x01 << UDRIE0); //выключаем прерывания передачи
  }
}
//----------------------------------Отправка команды----------------------------------
void sendCommand(uint8_t command) //отправка команды
{
  if (!_TRANSFER_BUFFER_END) { //отправляем только после опустошения буфера
    _TRANSFER_BUFFER_END = 1; //указываем длинну пакета
    _TRANSFER_BUFFER[0] = command; //записываем команду
    UCSR0B |= (0x01 << UDRIE0); //разрешаем прерывание по завершению передачи
  }
}
//-------------------------Отправка информации----------------------------------------------------
void sendData(uint8_t command, uint8_t *data, uint32_t size_l) //отправка информации
{
  if (!_TRANSFER_BUFFER_END) { //отправляем только после опустошения буфера
    _TRANSFER_BUFFER[0] = command;
    
    uint16_t crc = 0;
    for (uint8_t i = 0; i < size_l; i++) {
      crc += (uint16_t)data[i] * (i + 2); //расчет контрольной суммы(максимум 21 байт данных)
      _TRANSFER_BUFFER[i + 1] = data[i];
    }
    for (uint8_t i = 0; i < 2; i++) _TRANSFER_BUFFER[i + size_l + 1] = *((char*)&crc + i);

    _TRANSFER_BUFFER_END = size_l + 3; //указываем длинну пакета
    UCSR0B |= (0x01 << UDRIE0); //разрешаем прерывание по завершению передачи
  }
}
