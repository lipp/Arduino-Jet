
#ifndef SERIAL_CLIENT
#define SERIAL_CLIENT

#include "Client.h"

struct SerialClient : public Client {
  SerialClient(HardwareSerial &serial) : _serial(serial){};

  size_t write(const uint8_t *buf, size_t size) {
    return _serial.write(buf, size);
  };

  int available() { return _serial.available(); };

  int read() { return _serial.read(); };

  void waitHandshake(int delayMs = 1000) {
    while (1) {
      if (_serial.available()) {
        _serial.read();
        return;
      } else {
        delay(delayMs);
      }
    }
  };

  virtual int connect(IPAddress ip, uint16_t port) { return 0; };
  virtual int connect(const char *host, uint16_t port) { return 0; };
  virtual size_t write(uint8_t) { return 0; };
  virtual int read(uint8_t *buf, size_t size) { return 0; };
  virtual int peek() { return 0; };
  virtual void flush(){};
  virtual void stop(){};
  virtual uint8_t connected() { return 0; };
  virtual operator bool() { return true; };

private:
  HardwareSerial &_serial;
};

#endif
