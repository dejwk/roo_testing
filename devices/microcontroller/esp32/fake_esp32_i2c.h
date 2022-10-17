#pragma once

#include <inttypes.h>
#include <stddef.h>
#include <string>

#include "roo_testing/buses/i2c/fake_i2c.h"

class FakeEsp32Board;

class Esp32I2c {
 public:
  Esp32I2c(FakeEsp32Board& board, const std::string& name, uint8_t sda_signal, uint8_t sdl_signal);

  const std::string name() const { return name_; }

  void resetTxFifo() {
    txfifo_idx_ = 0;
  }

  void writeTxFifo(uint8_t data);

  void start();
  void write();
  void read();
  void stop();
  void end();

  int32_t write(uint16_t address, const uint8_t* buf, size_t size, uint32_t timeOutMillis);
  int32_t read(uint16_t address, uint8_t* buf, size_t size, uint32_t timeOutMillis);

 private:
  FakeI2cDevice* resolveOut(uint16_t address);

  FakeEsp32Board& board_;
  std::string name_;
  uint8_t sda_signal_;
  uint8_t scl_signal_;
  uint8_t ram_[32];
  int txfifo_idx_;  
};
