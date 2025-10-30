#pragma once

#include <stdint.h>

#include <string>

class FakeEsp32Board;

class Esp32UartInterface {
 public:
  Esp32UartInterface(int idx, const std::string& name, uint8_t tx_signal,
                     uint8_t rx_signal, FakeEsp32Board* esp32);

  size_t write(const uint8_t* buf, size_t size);
  size_t read(uint8_t* buf, size_t size);
  size_t availableForRead();
  size_t availableForWrite();

  void notifyDataAvailable();

  uint8_t tx_signal() const { return tx_signal_; }
  uint8_t rx_signal() const { return rx_signal_; }

 private:
  int idx_;

  uint8_t tx_signal_;
  uint8_t rx_signal_;

  FakeEsp32Board* esp32_;
};
