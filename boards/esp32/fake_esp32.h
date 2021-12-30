#pragma once

#include <map>

#include "roo_testing/buses/gpio/fake_gpio.h"
#include "roo_testing/buses/i2c/fake_i2c.h"
#include "roo_testing/buses/spi/fake_spi.h"

static constexpr uint16_t kMatrixDetachOutSig = 0x100;

// Fake pin # that acts as always low when attached to an input.
static constexpr uint8_t kMatrixDetachInLowPin = 0x30;

// Fake pin # that acts as always high when attached to an input.
static constexpr uint8_t kMatrixDetachInHighPin = 0x38;

// Fake pin # that acts as undefined (low-impedance) when attached to an input.
static constexpr uint8_t kMatrixDetachInUndefPin = 0x34;

class Esp32InMatrix {
 public:
  Esp32InMatrix();

  void assign(uint32_t pin, uint32_t function, bool inv);

 private:
  uint8_t signal_to_pin_[256];
  std::map<uint8_t, uint8_t> pin_to_signals_;
};

class Esp32OutMatrix {
 public:
  Esp32OutMatrix();

  void assign(uint32_t pin, uint32_t function, bool out_inv, bool oen_inv);

 private:
  uint16_t pin_to_signal_[40];
  std::map<uint8_t, uint8_t> signal_to_pins_;
};

class FakeEsp32Board {
 public:
  FakeGpioInterface gpio;

  Esp32InMatrix in_matrix;
  Esp32OutMatrix out_matrix;

  FakeI2cInterface i2c[2];
  FakeSpiInterface spi[4];

  FakeSpiInterface& fspi;  // References spi[1].
  FakeSpiInterface& hspi;  // References spi[2].
  FakeSpiInterface& vspi;  // References spi[3].

 private:
  friend FakeEsp32Board& FakeEsp32();

  FakeEsp32Board();
};

FakeEsp32Board& FakeEsp32();
