#pragma once

// GP Logger module

#include <Print.h>
class DataLogger : public Print {
  public:
    void start(uint16_t n = 64) {
      stop();
      clear();
      size = n;
      buffer = new char [size];
    }

    ~DataLogger() {
      stop();
    }

    void stop(void) {
      if (buffer) {
        delete [] buffer;
        buffer = nullptr;
      }
    }

    virtual size_t write(uint8_t n) {
      writeBuff(n);
      return 1;
    }

    String read(void) {
      String s;
      if (!len) return s;
      s.reserve(len + 1);
      if (newline) {
        newline = 0;
        s += '\n';
      }
      for (uint16_t i = 0; i < len; i++) {
        char c = readBuff(i);
        if ((i == (len - 2)) && (c == '\r')) {
          if (autoclear) newline = 1;
          break;
        }
        if (c == '\r') continue;
        s += c;
      }
      if (autoclear) clear();
      return s;
    }

    void clear(void) {
      len = head = 0;
    }

    void autoClear(boolean set) {
      autoclear = set;
    }

    boolean available(void) {
      return (buffer && len);
    }

    boolean state(void) {
      return buffer;
    }

    uint16_t length(void) {
      return len;
    }

  private:
    void writeBuff(uint8_t n) {
      if (len < size) len++;
      buffer[head] = n;
      if (++head >= size) head = 0;
    }
    char readBuff(uint16_t num) {
      return buffer[(len < size) ? num : ((head + num) % size)];
    }

    char* buffer = nullptr;

    boolean newline = 0;
    boolean autoclear = 1;
    uint16_t size = 0;
    uint16_t len = 0;
    uint16_t head = 0;
};
