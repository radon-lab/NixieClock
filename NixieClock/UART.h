volatile uint8_t uartData; //буфер UART
volatile uint8_t uartByte; //текущий байт UART
volatile uint8_t uartBit; //текущий бит UART

#define SOFT_UART_TIME (uint8_t)((1e9 / PLAYER_UART_SPEED) / 500) //период фрейма UART

//-----------------------------------Инициализация UART------------------------------------
void uartInit(void) //инициализация uart
{
  UBRR0 = (F_CPU / (8UL * PLAYER_UART_SPEED)) - 1; //устанавливаем битрейт
  UCSR0A = (0x01 << U2X0); //устанавливаем удвоение скорости
  UCSR0B = (0x01 << TXEN0); //разрешаем передачу
  UCSR0C = ((0x01 << UCSZ01) | (0x01 << UCSZ00)); //устанавливаем длинну посылки 8 бит
}
//-----------------------------------Отключение UART------------------------------------
void uartDisable(void) //отключение uart
{
  UCSR0B = 0; //выключаем UART
}
//-------------------------------------Статус UART--------------------------------------
inline boolean uartStatus(void)
{
#if PLAYER_UART_MODE
  return (boolean)!(TIMSK2 & (0x01 << OCIE2B));
#else
  return (boolean)!(UCSR0B & (0x01 << UDRIE0));
#endif
}
//-------------------------------Отправка данных в UART---------------------------------
void uartSendData(void)
{
  playerGenCRC(command.transferBuffer);
#if PLAYER_UART_MODE
  uartData = command.transferBuffer[0];
  uartByte = 0;
  uartBit = 0;
  OCR2B = 0; //устанавливаем COMB в начало
  TIFR2 |= (0x01 << OCF2B); //сбросили флаг прерывания
  TIMSK2 |= (0x01 << OCIE2B); //запускаем таймер
#else
  uartByte = 0;
  UCSR0B |= (0x01 << UDRIE0); //разрешаем прерывание по завершению передачи
#endif
  _timer_ms[TMR_PLAYER] = PLAYER_BUSY_ERROR + ((command.transferBuffer[_COMMAND] != PLAYER_CMD_RESET) ? PLAYER_COMMAND_WAIT : PLAYER_RESET_WAIT);
}

#if PLAYER_TYPE == 1
#if PLAYER_UART_MODE
//---------------------------------Софтовая обработка UART----------------------------------
ISR(TIMER2_COMPB_vect)
{
  OCR2B += SOFT_UART_TIME;
  switch (uartBit++) {
    case 0: DF_RX_CLEAR; break;
    case 9: DF_RX_SET; break;
    case 10:
      if (++uartByte >= sizeof(command.transferBuffer)) TIMSK2 &= ~(0x01 << OCIE2B); //выключаем таймер
      else {
        uartData = command.transferBuffer[uartByte];
        uartBit = 0;
      }
      break;
    default:
      if (uartData & 0x01) DF_RX_SET;
      else DF_RX_CLEAR;
      uartData >>= 1;
      break;
  }
}
#else
//--------------------------------Хардверная обработка UART----------------------------------
ISR(USART_UDRE_vect)
{
  if (uartByte >= sizeof(command.transferBuffer)) UCSR0B &= ~(0x01 << UDRIE0); //выключаем прерывания передачи
  else UDR0 = command.transferBuffer[uartByte++]; //записываем байт в буфер UART
}
#endif
#endif
