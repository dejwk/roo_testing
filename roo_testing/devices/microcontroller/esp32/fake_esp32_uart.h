#pragma once

#include "roo_testing/buses/uart/fake_uart.h"

class FakeEsp32Board;

class Esp32UartInterface : public FakeUartInterface {
 public:
  Esp32UartInterface(const std::string& name,
                     uint8_t tx_signal, uint8_t rx_signal,
                     FakeEsp32Board* esp32);

  size_t write(const uint8_t* buf, size_t size);
  size_t read(uint8_t* buf, size_t size);
  size_t availableForRead();
  size_t availableForWrite();

 private:
  uint8_t tx_signal_;
  uint8_t rx_signal_;

  FakeEsp32Board* esp32_;
};
