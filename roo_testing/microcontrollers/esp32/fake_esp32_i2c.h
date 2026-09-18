#pragma once

#include <inttypes.h>
#include <stddef.h>

#include <string>

#include "roo_testing/buses/i2c/fake_i2c.h"

class FakeEsp32Board;

/// Resolves attached I2C peripherals through the emulated GPIO output matrix.
class Esp32I2c {
 public:
  /// Binds a controller to its board and SDA/SCL matrix signals.
  Esp32I2c(FakeEsp32Board& board, const std::string& name, uint8_t sda_signal,
           uint8_t sdl_signal);

  /// Returns the controller diagnostic name.
  const std::string name() const { return name_; }

  /// Discards queued emulated transmit FIFO bytes.
  void resetTxFifo() { txfifo_idx_ = 0; }

  /// Appends one byte to the emulated transmit FIFO.
  void writeTxFifo(uint8_t data);

  /// Placeholder for register-level operations; currently performs no action.
  void start();

  /// Placeholder for register-level operations; currently performs no action.
  void write();

  /// Placeholder for register-level operations; currently performs no action.
  void read();

  /// Placeholder for register-level operations; currently performs no action.
  void stop();

  /// Placeholder for register-level operations; currently performs no action.
  void end();

  /// Writes to the device selected by the output matrix, preserving STOP
  /// intent.
  int32_t write(uint16_t address, const uint8_t* buf, size_t size,
                uint32_t timeOutMillis, bool sendStop = true);

  /// Reads from the device selected by the output matrix, preserving STOP
  /// intent.
  int32_t read(uint16_t address, uint8_t* buf, size_t size,
               uint32_t timeOutMillis, bool sendStop = true);

  /// Returns whether exactly one addressed device is connected through the
  /// pins.
  bool probe(uint16_t address);

 private:
  FakeI2cDevice* resolveOut(uint16_t address);

  FakeEsp32Board& board_;
  std::string name_;
  uint8_t sda_signal_;
  uint8_t scl_signal_;
  uint8_t ram_[32];
  int txfifo_idx_;
};
