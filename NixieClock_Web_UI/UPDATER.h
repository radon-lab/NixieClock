#define BOOTLOADER_ADDRESS 120 //адрес загрузчика(только 120)
#define BOOTLOADER_PAGE_BUFF 128 //размер страницы(только 128)

struct pageData {
  uint8_t pos;
  uint8_t size;
  uint8_t type;
  uint16_t addr;
} page;

enum {
  UPDATER_IDLE,
  UPDATER_ERROR,
  UPDATER_TIMEOUT,
  UPDATER_NO_FILE,
  UPDATER_NOT_HEX,
  UPDATER_UPL_ABORT,
  UPDETER_LOAD,
  UPDATER_WRITE,
  UPDATER_CHECK,
  UPDATER_END
};
uint8_t updater_buffer[BOOTLOADER_PAGE_BUFF];
uint8_t updater_state = UPDATER_IDLE;
uint8_t updater_page_crc = 0;
uint8_t updater_page_cnt = 0;
uint32_t updater_timer = 0;

File firmwareFile;

//---------------------------------------------------------------------------------
uint8_t hexToInt(uint8_t data) {
  if ((data >= '0' && data <= '9') || (data >= 'A' && data <= 'F')) return data - ((data <= '9') ? '0' : '7');
  return 0xFF;
}
//---------------------------------------------------------------------------------
uint8_t getIntData(uint8_t data_h, uint8_t data_l) {
  return hexToInt(data_l) | (hexToInt(data_h) << 4);
}
//---------------------------------------------------------------------------------
void updateCRC(uint8_t* crc, uint8_t data) //сверка контрольной суммы
{
  for (uint8_t i = 8; i; i--) { //считаем для всех бит
    *crc = ((*crc ^ data) & 0x01) ? (*crc >> 0x01) ^ 0x8C : (*crc >> 0x01); //рассчитываем значение
    data >>= 0x01; //сдвигаем буфер
  }
}
//---------------------------------------------------------------------------------
uint8_t getFileData(void) {
  if (firmwareFile.available()) {
    if (page.pos >= page.size) {
      page.type = 1;
      while (firmwareFile.available()) {
        if (firmwareFile.read() == ':') {
          page.size = getIntData(firmwareFile.read(), firmwareFile.read());
          page.addr = ((uint16_t)getIntData(firmwareFile.read(), firmwareFile.read()) << 8) | getIntData(firmwareFile.read(), firmwareFile.read());
          page.type = getIntData(firmwareFile.read(), firmwareFile.read());
          page.pos = 0;
          break;
        }
      }
    }
    if (page.addr >= 0x7E00) {
      page.size = 0;
      page.pos = 0;
    }
    else if (page.type == 0) {
      page.pos++;
      return getIntData(firmwareFile.read(), firmwareFile.read());
    }
  }
  return 0xFF;
}
//---------------------------------------------------------------------------------
boolean updaterFlash(void) {
  return (updater_state > UPDATER_UPL_ABORT);
}
//---------------------------------------------------------------------------------
boolean updaterState(void) {
  return (updater_state > UPDATER_TIMEOUT);
}
//---------------------------------------------------------------------------------
uint8_t updaterStatus(void) {
  return updater_state;
}
//---------------------------------------------------------------------------------
uint8_t updaterProgress(void) {
  return updater_page_cnt;
}
//---------------------------------------------------------------------------------
void updaterSetStatus(uint8_t status) {
  if (!updaterFlash()) {
    if ((status >= UPDATER_NO_FILE) && (status <= UPDATER_UPL_ABORT)) updater_state = status;
  }
}
//---------------------------------------------------------------------------------
void updaterSetIdle(void) {
  if ((updater_state >= UPDATER_NO_FILE) && (updater_state <= UPDATER_UPL_ABORT)) updater_state = UPDATER_IDLE;
}
//---------------------------------------------------------------------------------
void updaterStart(void) {
  if (!updaterState()) {
    firmwareFile = LittleFS.open("/update/firmware.hex", "r");
    if (firmwareFile && (firmwareFile.size() != 0)) {
      page.pos = 0;
      page.size = 0;
      updater_page_cnt = 0;
      updater_timer = millis();
      updater_state = UPDETER_LOAD;
      Serial.println F("Updater start programming");
    }
    else {
      updater_state = UPDATER_NO_FILE; //установили флаг ошибки файла
      LittleFS.remove("/update/firmware.hex"); //удаляем файл
      Serial.println F("Updater error opening file");
    }
  }
}
//---------------------------------------------------------------------------------
void updaterRun(void) {
  if ((millis() - updater_timer) >= 10000) {
    updater_state = UPDATER_TIMEOUT;
    LittleFS.remove("/update/firmware.hex"); //удаляем файл
    Serial.println("Updater timeout write page " + String(updater_page_cnt));
  }
  switch (updater_state) {
    case UPDETER_LOAD:
      updater_page_crc = 0;
      for (uint8_t cnt = 0; cnt < sizeof(updater_buffer); cnt++) {
        updater_buffer[cnt] = getFileData();
        updateCRC(&updater_page_crc, updater_buffer[cnt]);
      }
      Serial.println("Updater load page " + String(updater_page_cnt) + " success");
      updater_state = UPDATER_CHECK;
      break;
    case UPDATER_WRITE:
      if (!twi_beginTransmission(BOOTLOADER_ADDRESS)) { //начинаем передачу
        for (uint8_t cnt = 0; cnt < sizeof(updater_buffer); cnt++) twi_write_byte(updater_buffer[cnt]);
        twi_write_byte(updater_page_crc);
        twi_write_stop(); //остановили шину
        updater_timer = millis();
        updater_state = UPDATER_CHECK;
        Serial.println("Updater write page " + String(updater_page_cnt) + " success");
      }
      break;
    case UPDATER_CHECK: {
        if (!twi_beginTransmission(BOOTLOADER_ADDRESS, 1)) { //начинаем передачу
          uint8_t temp_page = twi_read_byte(TWI_NACK);
          if (!twi_error()) { //если передача была успешной
            twi_write_stop(); //остановили шину
            if (temp_page == 0xFF) {
              updater_page_cnt = temp_page;
              updater_state = UPDATER_END;
            }
            else if ((temp_page - updater_page_cnt) == 1) {
              updater_page_cnt = temp_page;
              updater_state = UPDETER_LOAD;
              Serial.println("Updater new page at " + String(updater_page_cnt));
            }
            else if ((temp_page - updater_page_cnt) == 0) updater_state = UPDATER_WRITE;
            else {
              updater_state = UPDATER_ERROR;
              LittleFS.remove("/update/firmware.hex"); //удаляем файл
              Serial.println("Updater error, page at " + String(updater_page_cnt));
            }
          }
        }
      }
      break;

    case UPDATER_END:
      if (!twi_beginTransmission(BOOTLOADER_ADDRESS)) { //начинаем передачу
        twi_write_byte(0xFF);
        if (!twi_error()) { //если передача была успешной
          twi_write_stop(); //остановили шину
          updater_state = UPDATER_IDLE;
          firmwareFile.close();
          LittleFS.remove("/update/firmware.hex"); //удаляем файл
          Serial.println F("Updater end, reboot...");
          ESP.reset(); //перезагрузка
        }
      }
      break;
  }
}
