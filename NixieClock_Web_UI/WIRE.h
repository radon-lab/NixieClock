#define TWI_SCL_STRCH_LIMIT 0    //максимальное время ожидания освобождения SCL линии(мкс)
#define TWI_SDA_STRCH_LIMIT 20   //количество попыток освободить SDA
#define TWI_BUS_POLLING_LIMIT 20 //максимальное время ожидания освобождения шины(мкс)

#define TWI_NACK HIGH
#define TWI_ACK LOW

#define TWI_OK -1
#define TWI_BUSY -2

#define TWI_READ 0x01
#define TWI_WRITE 0x00

#include "pins_arduino.h"
#include "wiring_private.h"

#ifndef FCPU80
#define FCPU80 80000000UL
#endif

#if F_CPU == FCPU80
#define TWI_CLOCK_STRETCH_MULTIPLIER 3
#define TWI_CLOCK_DCOUNT 19 //100KHz
#else
#define TWI_CLOCK_STRETCH_MULTIPLIER 6
#define TWI_CLOCK_DCOUNT 32 //100KHz
#endif

#define SDA_LOW()  (GPES = (0x01 << TWI_SDA_PIN))
#define SDA_HIGH() (GPEC = (0x01 << TWI_SDA_PIN))
#define SDA_READ() ((GPI & (0x01 << TWI_SDA_PIN)) != 0)
#define SCL_LOW()  (GPES = (0x01 << TWI_SCL_PIN))
#define SCL_HIGH() (GPEC = (0x01 << TWI_SCL_PIN))
#define SCL_READ() ((GPI & (0x01 << TWI_SCL_PIN)) != 0)

boolean twi_beginTransmission(uint8_t addr, boolean rw = 0x00); //запуск передачи

boolean twi_state = false;
boolean twi_collision = false;

//--------------------------------------------------------------------
void twi_delay(int16_t value)
{
  if (value < 1) value = TWI_CLOCK_DCOUNT;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

  uint16_t reg = 0;

  for (uint16_t i = 0; i < value; i++) reg = GPI;

  (void)reg;

#pragma GCC diagnostic pop
}
//--------------------------------------------------------------------
boolean clockStretch(void)
{
#if TWI_SCL_STRCH_LIMIT
  int32_t pollCounter = (TWI_SCL_STRCH_LIMIT * TWI_CLOCK_STRETCH_MULTIPLIER);

  while (SCL_READ() == LOW) {
    if (pollCounter-- < 0) return false;
  }
#else
  while (SCL_READ() == LOW) yield();
#endif

  return true;
}
//--------------------------------------------------------------------
boolean dataStretch(void)
{
  int8_t pollCounter = TWI_SDA_STRCH_LIMIT;

  while (SDA_READ() == LOW) {
    if (pollCounter-- < 0) return false;

    SCL_HIGH();
    if (clockStretch() != true) return false;
    twi_delay(TWI_CLOCK_DCOUNT - 3);

    SCL_LOW();
    twi_delay(TWI_CLOCK_DCOUNT - 3);
  }

  return true;
}
//--------------------------------------------------------------------
int8_t checkBus(void)
{
  int32_t pollCounter = (TWI_BUS_POLLING_LIMIT * TWI_CLOCK_STRETCH_MULTIPLIER);

  while ((SDA_READ() == HIGH) && (SCL_READ() == HIGH)) {
    if (pollCounter-- < 0) return TWI_OK; //bus released!!!
  }

  return TWI_BUSY; //error handler, bus busy!!!
}
//--------------------------------------------------------------------
void twi_init(void)
{
  SCL_HIGH();
  SDA_HIGH();
  delay(20);
  twi_state = false;
}
//--------------------------------------------------------------------
int8_t twi_write_stop(void)
{
  if (twi_state == true) {
    SCL_LOW();
    SDA_HIGH();
    twi_delay(TWI_CLOCK_DCOUNT - 3);
    if (dataStretch() != true) return TWI_BUSY;
    SDA_LOW();
    twi_delay(TWI_CLOCK_DCOUNT - 3);

    SCL_HIGH();
    if (clockStretch() != true) return TWI_BUSY;
    twi_delay(TWI_CLOCK_DCOUNT - 5);

    SDA_HIGH();

    twi_state = false;
  }
  return TWI_OK;
}
//--------------------------------------------------------------------
int8_t twi_write_start(void)
{
  if (twi_write_stop() != TWI_OK) return TWI_BUSY;
  SCL_HIGH();
  SDA_HIGH();
  twi_delay(TWI_CLOCK_DCOUNT - 18);

  if (checkBus() != TWI_OK) {
    twi_state = false;
    return TWI_BUSY;
  }

  SDA_LOW();
  twi_delay(TWI_CLOCK_DCOUNT - 1);
  SCL_LOW();

  twi_state = true;

  return TWI_OK;
}
//--------------------------------------------------------------------
int8_t twi_write_bit(boolean _bit)
{
  SCL_LOW();
  twi_delay(TWI_CLOCK_DCOUNT - 5);

  switch (_bit) {
    case HIGH: SDA_HIGH(); break;
    case LOW: SDA_LOW(); break;
  }
  twi_delay(1);

  SCL_HIGH();
  if (clockStretch() == false) return TWI_BUSY;
  twi_delay(TWI_CLOCK_DCOUNT - 5);
  SCL_LOW();
  SDA_LOW();

  return TWI_OK;
}
//--------------------------------------------------------------------
int8_t twi_read_bit(void)
{
  boolean rxBit = 0;

  SCL_LOW();
  SDA_HIGH();
  twi_delay(TWI_CLOCK_DCOUNT - 3);

  SCL_HIGH();
  if (clockStretch() == false) return TWI_BUSY;

  rxBit = SDA_READ();
  twi_delay(TWI_CLOCK_DCOUNT - 6);
  SCL_LOW();
  SDA_LOW();

  return rxBit;
}
//--------------------------------------------------------------------
boolean twi_write_byte(uint8_t _byte)
{
  for (uint8_t i = 0; i < 8; i++) {
    if (twi_write_bit(_byte & 0x80) != TWI_OK) return TWI_NACK;
    _byte <<= 1;
  }

  if (twi_read_bit() == TWI_ACK) return TWI_ACK;
  return TWI_NACK;
}
//--------------------------------------------------------------------
uint8_t twi_read_byte(boolean _answer)
{
  uint8_t _byte = 0;

  for (uint8_t i = 0; i < 8; i++) {
    _byte <<= 1;
    switch (twi_read_bit()) {
      case HIGH: _byte |= 0x01; break;
      case TWI_BUSY: twi_collision = true; return 0;
    }
  }

  if (twi_write_bit(_answer) != TWI_OK) {
    twi_collision = true;
    return 0;
  }

  twi_collision = false;
  return _byte;
}
//--------------------------------------------------------------------
boolean twi_beginTransmission(uint8_t addr, boolean rw) //запуск передачи
{
  if (twi_write_start() != TWI_OK) return TWI_NACK; //запуск шины wire
  if (twi_write_byte((addr << 0x01) | rw) != TWI_ACK) {
    twi_write_stop(); //остановили шину
    return TWI_NACK; //отправка устройству адреса с битом read/write
  }
  return TWI_ACK;
}
//--------------------------------------------------------------------
boolean twi_requestFrom(uint8_t addr, uint8_t reg) //запрос чтения данных
{
  if (twi_beginTransmission(addr) != TWI_ACK) return TWI_NACK; //отправка устройству адреса с битом write
  twi_write_byte(reg); //отправка устройству начального адреса чтения
  if (twi_beginTransmission(addr, 0x01) != TWI_ACK) return TWI_NACK; //отправка устройству адреса с битом read

  return TWI_ACK;
}
//--------------------------------------------------------------------
boolean twi_requestFrom(uint8_t addr, uint8_t cmd, uint8_t arg) //запрос чтения данных
{
  if (twi_beginTransmission(addr) != TWI_ACK) return TWI_NACK; //отправка устройству адреса с битом write
  twi_write_byte(cmd); //отправка устройству начального адреса чтения
  twi_write_byte(arg); //отправка устройству начального адреса чтения
  if (twi_beginTransmission(addr, 0x01) != TWI_ACK) return TWI_NACK; //отправка устройству адреса с битом read

  return TWI_ACK;
}
//--------------------------------------------------------------------
boolean twi_requestFrom(uint8_t addr, uint8_t cmd, uint8_t arg, uint8_t reg) //запрос чтения данных
{
  if (twi_beginTransmission(addr) != TWI_ACK) return TWI_NACK; //отправка устройству адреса с битом write
  twi_write_byte(cmd); //отправка устройству начального адреса чтения
  twi_write_byte(arg); //отправка устройству начального адреса чтения
  twi_write_byte(reg); //отправка устройству начального адреса чтения
  if (twi_beginTransmission(addr, 0x01) != TWI_ACK) return TWI_NACK; //отправка устройству адреса с битом read

  return TWI_ACK;
}
