#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>

#define BOOTLOADER_ADDR 120
#define BOOTLOADER_EEPROM 1023

#define BOOTLOADER_PAGE_BUFF 128
#define BOOTLOADER_PAGE_SIZE 64
#define BOOTLOADER_PAGE_ALL 252

#define RESET_SYSTEM __asm__ __volatile__ ("JMP 0x0000") //перезагрузка
#define RESET_WDT __asm__ __volatile__ ("WDR") //сброс WDT

#define BOOTLOADER_OK 0xAA
#define BOOTLOADER_START 0xBB
#define BOOTLOADER_FLASH 0xCC

#define LED_DDR  DDRB
#define LED_PORT PORTB
#define LED_PIN  PINB
#define LED      PINB5

int main(void) __attribute__ ((naked)) __attribute__ ((section (".init9")));
void ledFlash(uint8_t count);
uint8_t EEPROM_ReadByte(uint16_t addr);
void EEPROM_WriteByte(uint16_t addr, uint8_t data);

volatile uint16_t ram_page_address = 0;
uint8_t ram_page_buff[BOOTLOADER_PAGE_BUFF];

int main(void) {
  cli(); //запрещаем прерывания

  uint8_t ram_page = 0;
  uint8_t ram_page_cnt = 0;
  uint8_t ram_page_crc = 0;

  uint8_t status = 0;
  uint8_t twi_buffer = 0;

  RESET_WDT; //сбрасываем таймер WDT
  status = MCUSR; //запомнили статус
  MCUSR = 0; //сбросли флаги перезагрузки
  WDTCSR = (0x01 << WDCE) | (0x01 << WDE); //разрешаем изменения WDT
  WDTCSR = (0x01 << WDE) | (0x01 << WDP3) | (0x01 << WDP0); //устанавливаем таймер WDT на 8сек

  PORTC = 0x00; //отключаем подтяжку SDA и SCL
  DDRC = 0x00; //устанавливаем SDA и SCL как входы

  LED_DDR |= (0x01 << LED);
  ledFlash(2);

  if (status & (0x01 << EXTRF)) status = BOOTLOADER_START; //если нажата кнопка ресет начинаем загрузку
  else { //иначе проверяем флаги в памяти
    status = EEPROM_ReadByte(BOOTLOADER_EEPROM); //чтение байта из EEPROM
    if (status != BOOTLOADER_FLASH) { //если память не была измененена
      if (status != BOOTLOADER_OK) EEPROM_WriteByte(BOOTLOADER_EEPROM, BOOTLOADER_OK); //запись байта в EEPROM
      if (status != BOOTLOADER_START) RESET_SYSTEM; //переход к основной программе
    }
  }

  ledFlash(4);

  TWAR = (BOOTLOADER_ADDR << 0x01); //установили адрес шины
  TWCR = (0x01 << TWEN) | (0x01 << TWINT) | (0x01 << TWEA); //включили шину

  for (;;) {
    if (TWCR & (0x01 << TWINT)) {
      twi_buffer = (TWSR & 0xF8); //прочитали статус шины

      if (twi_buffer == 0x00) { //ошибка шины
        TWCR = (0x01 << TWSTO) | (0x01 << TWEN) | (0x01 << TWINT) | (0x01 << TWEA); //отправляем команду стоп и устанавливаем флаг выполнить задачу
      }
      else if (twi_buffer == 0x60) { //принят SLA+W - передан ACK
        ram_page_cnt = 0;
        ram_page_crc = 0;
      }
      else if (twi_buffer == 0x80) { //принят байт данных - передан ACK
        if (ram_page < BOOTLOADER_PAGE_ALL) {
          if (ram_page_cnt < BOOTLOADER_PAGE_BUFF) {
            twi_buffer = TWDR;
            ram_page_buff[ram_page_cnt] = twi_buffer;
            uint8_t cnt = 8;
            while (cnt) { //считаем для всех бит
              ram_page_crc = ((ram_page_crc ^ twi_buffer) & 0x01) ? (ram_page_crc >> 0x01) ^ 0x8C : (ram_page_crc >> 0x01); //рассчитываем значение
              twi_buffer >>= 0x01; //сдвигаем буфер
              cnt--;
            }
            ram_page_cnt++;
          }
          else {
            if (TWDR == ram_page_crc) {
              if (status != BOOTLOADER_FLASH) { //если память не была измененена
                status = BOOTLOADER_FLASH; //устанавливаем статус
                EEPROM_WriteByte(BOOTLOADER_EEPROM, BOOTLOADER_FLASH); //запись байта в EEPROM
              }
              while (EECR & (0x01 << EEPE)); //ждем окончания записи
			  
			  ram_page_address = (uint16_t)ram_page * BOOTLOADER_PAGE_BUFF;

              //запись флеш памяти
              asm volatile(
                "clr  r17                    \n\t" //page_word_count
                "lds  r30,ram_page_address   \n\t" //Address of FLASH location (in bytes)
                "lds  r31,ram_page_address+1 \n\t"
                "ldi  r28,lo8(ram_page_buff) \n\t" //Start of buffer array in RAM
                "ldi  r29,hi8(ram_page_buff) \n\t"
                "ldi  r24,0x80               \n\t" //Length of data to be written (in bytes)
                "ldi  r25,0x00               \n\t"
                "length_loop:                \n\t" //Main loop, repeat for number of words in block
                "cpi  r17,0x00               \n\t" //If page_word_count=0 then erase page
                "brne no_page_erase          \n\t"
                "wait_spm1:                  \n\t"
                "lds  r16,%0                 \n\t" //Wait for previous spm to complete
                "andi r16,1                  \n\t"
                "cpi  r16,1                  \n\t"
                "breq wait_spm1              \n\t"
                "ldi  r16,0x03               \n\t" //Erase page pointed to by Z
                "sts  %0,r16                 \n\t"
                "spm                         \n\t"
                "wait_spm2:                  \n\t"
                "lds  r16,%0                 \n\t" //Wait for previous spm to complete
                "andi r16,1                  \n\t"
                "cpi  r16,1                  \n\t"
                "breq wait_spm2              \n\t"

                "ldi  r16,0x11               \n\t" //Re-enable RWW section
                "sts  %0,r16                 \n\t"
                "spm                         \n\t"
                "no_page_erase:              \n\t"
                "ld r0,Y+                    \n\t" //Write 2 bytes into page buffer
                "ld r1,Y+                    \n\t"

                "wait_spm3:                  \n\t"
                "lds  r16,%0                 \n\t" //Wait for previous spm to complete
                "andi r16,1                  \n\t"
                "cpi  r16,1                  \n\t"
                "breq wait_spm3              \n\t"
                "ldi  r16,0x01               \n\t" //Load r0,r1 into FLASH page buffer
                "sts  %0,r16                 \n\t"
                "spm                         \n\t"

                "inc  r17                    \n\t" //page_word_count++
                "cpi r17,%1                  \n\t"
                "brlo same_page              \n\t" //Still same page in FLASH
                "write_page:                 \n\t"
                "clr  r17                    \n\t" //New page, write current one first
                "wait_spm4:                  \n\t"
                "lds  r16,%0                 \n\t" //Wait for previous spm to complete
                "andi r16,1                  \n\t"
                "cpi  r16,1                  \n\t"
                "breq wait_spm4              \n\t"
                "ldi  r16,0x05               \n\t" //Write page pointed to by Z
                "sts  %0,r16                 \n\t"
                "spm                         \n\t"
                "wait_spm5:                  \n\t"
                "lds  r16,%0                 \n\t" //Wait for previous spm to complete
                "andi r16,1                  \n\t"
                "cpi  r16,1                  \n\t"
                "breq wait_spm5              \n\t"
                "ldi  r16,0x11               \n\t" //Re-enable RWW section
                "sts  %0,r16                 \n\t"
                "spm                         \n\t"
                "same_page:                  \n\t"
                "adiw r30,2                  \n\t" //Next word in FLASH
                "sbiw r24,2                  \n\t" //length-2
                "breq final_write            \n\t" //Finished
                "rjmp length_loop            \n\t"
                "final_write:                \n\t"
                "cpi  r17,0                  \n\t"
                "breq block_done             \n\t"
                "adiw r24,2                  \n\t" //length+2, fool above check on length after short page write
                "rjmp write_page             \n\t"
                "block_done:                 \n\t"
                "clr  __zero_reg__           \n\t" //restore zero register
                : "=m" (SPMCSR)
                : "M" (BOOTLOADER_PAGE_SIZE)
                : "r0", "r16", "r17", "r24", "r25", "r28", "r29", "r30", "r31"
              );
              
              ram_page++; //сместили указатель страницы

              if (ram_page >= BOOTLOADER_PAGE_ALL) { //если страницы закончились
                ram_page = 0xFF; //конец памяти
                EEPROM_WriteByte(BOOTLOADER_EEPROM, BOOTLOADER_OK); //запись байта в EEPROM
              }
            }
          }
        }
        else if (TWDR == 0xFF) {
          RESET_SYSTEM; //переход к основной программе
        }
      }
      else if (twi_buffer == 0xA8) { //принят SLA+R - передан ACK
        TWDR = ram_page;
      }
      TWCR |= (0x01 << TWINT); //сбросили флаг прерывания
      RESET_WDT; //сбрасываем таймер WDT
    }
  }
  return 0;
}
//--------------------------Запись байта в EEPROM-----------------------------------
void EEPROM_WriteByte(uint16_t addr, uint8_t data) //запись байта в EEPROM
{
  while (EECR & (0x01 << EEPE)); //ждем окончания записи
  EEAR = addr; //загружаем адрес
  EEDR = data; //загружаем байт данных
  EECR |= 1 << EEMPE; //разрешаем запись
  EECR |= 1 << EEPE; //сбрасываем флаг записи
}
//-------------------------Чтение байта из EEPROM-----------------------------------
uint8_t EEPROM_ReadByte(uint16_t addr) //чтение байта из EEPROM
{
  while (EECR & (0x01 << EEPE)); //ждем окончания записи
  EEAR = addr; //загружаем адрес
  EECR |= (0x01 << EERE); //запускаем чтение
  return EEDR; //возвращаем результат
}
//----------------------------------------------------------------------------------
void ledFlash(uint8_t count)
{
  while (count--) {
    LED_PORT ^= (0x01 << LED);
    _delay_ms(100);
  }
}
