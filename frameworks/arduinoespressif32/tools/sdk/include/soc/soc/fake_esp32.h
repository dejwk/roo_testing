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
  uint16_t pin_for_signal(uint8_t signal) const {
    return signal_to_pin_[signal];
  }

 private:
  uint8_t signal_to_pin_[256];
  std::map<uint8_t, uint8_t> pin_to_signals_;
};

class Esp32OutMatrix {
 public:
  Esp32OutMatrix();

  void assign(uint32_t pin, uint32_t function, bool out_inv, bool oen_inv);
  uint16_t signal_for_pin(uint8_t pin) const { return pin_to_signal_[pin]; }

 private:
  uint16_t pin_to_signal_[40];
  std::map<uint8_t, uint8_t> signal_to_pins_;
};

struct spi_def_t;

// struct SpiConnector {
//   enum Role { kClk, kMosi, kMiso } role;
//   FakeSpiDevice* device;
// }

struct SpiPins {
  int8_t clk;
  int8_t miso;
  int8_t mosi;
};

class FakeEsp32Board;

class Esp32SpiInterface : public FakeSpiInterface {
 public:
  Esp32SpiInterface(volatile spi_def_t& spi, const std::string& name,
                    uint8_t clk_signal, uint8_t miso_signal, uint8_t mosi_signal,
                    FakeEsp32Board* esp32);

  uint32_t clkHz() const override;
  SpiDataMode dataMode() const override;
  SpiBitOrder bitOrder() const override;

 private:
  friend void spiFakeTransferOnDevice(int8_t spi_num);

  void transfer();

  volatile spi_def_t& spi_;

  uint8_t clk_signal_;
  uint8_t miso_signal_;
  uint8_t mosi_signal_;

  FakeEsp32Board* esp32_;
};

class FakeEsp32Board {
 public:
  FakeGpioInterface gpio;

  Esp32InMatrix in_matrix;
  Esp32OutMatrix out_matrix;

  FakeI2cInterface i2c[2];

  void attachSpiDevice(SimpleFakeSpiDevice& dev, int8_t clk, int8_t miso, int8_t mosi);

  const std::map<SimpleFakeSpiDevice*, SpiPins>& spi_devices() const {
    return spi_devices_to_pins_;
  }

  const std::string& fs_root() const {
    return fs_root_;
  }

  void set_fs_root(std::string fs_root) {
    fs_root_ = std::move(fs_root);
  }

 private:
  friend FakeEsp32Board& FakeEsp32();
  friend void spiFakeTransferOnDevice(int8_t spi_num);

  FakeEsp32Board();

  Esp32SpiInterface spi[4];

  std::map<SimpleFakeSpiDevice*, SpiPins> spi_devices_to_pins_;
  std::string fs_root_;
};

FakeEsp32Board& FakeEsp32();
