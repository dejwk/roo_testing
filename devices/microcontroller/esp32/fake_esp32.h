#pragma once

#include <map>

#include "fake_esp32_adc.h"
#include "fake_esp32_nvs.h"
#include "fake_esp32_spi.h"
#include "fake_esp32_reg.h"
#include "fake_esp32_time.h"
#include "fake_esp32_uart.h"
#include "fake_esp32_wifi.h"

#include "fake_esp32_spi_struct.h"

#include "roo_testing/buses/gpio/fake_gpio.h"
#include "roo_testing/buses/i2c/fake_i2c.h"
#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/buses/uart/fake_uart.h"
#include "roo_testing/transducers/wifi/wifi.h"

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

struct UartPins {
  int8_t tx;
  int8_t rx;
};

struct SpiPins {
  int8_t clk;
  int8_t miso;
  int8_t mosi;
};

class FakeEsp32Board {
 public:
  FakeGpioInterface gpio;

  Esp32InMatrix in_matrix;
  Esp32OutMatrix out_matrix;

  FakeI2cInterface i2c[2];
  Esp32WifiAdapter wifi;

  Nvs nvs;

  EmulatedTime& time() { return time_; }
  const EmulatedTime& time() const { return time_; }

  void attachUartDevice(FakeUartDevice& dev, int8_t tx, int8_t rx);

  void attachSpiDevice(SimpleFakeSpiDevice& dev, int8_t clk, int8_t miso,
                       int8_t mosi);

  const std::map<FakeUartDevice*, UartPins>& uart_devices() const {
    return uart_devices_to_pins_;
  }

  const std::map<SimpleFakeSpiDevice*, SpiPins>& spi_devices() const {
    return spi_devices_to_pins_;
  }

  void flush();

  const std::string& fs_root() const { return fs_root_; }

  void set_fs_root(std::string fs_root) { fs_root_ = std::move(fs_root); }

  Esp32UartInterface uart(int idx) {
    return uart_[idx];
  }

  EspReg& reg() { return reg_; }
  Esp32Adc& adc(int idx) { return adc_[idx]; }

 private:
  friend FakeEsp32Board& FakeEsp32();
  friend void spiFakeTransferOnDevice(int8_t spi_num);

  FakeEsp32Board();

  EspReg reg_;

  Esp32UartInterface uart_[3];
  Esp32SpiInterface spi[4];
  Esp32Adc adc_[2];

  std::map<SimpleFakeSpiDevice*, SpiPins> spi_devices_to_pins_;
  std::map<FakeUartDevice*, UartPins> uart_devices_to_pins_;

  std::string fs_root_;
  EmulatedTime time_;
};

FakeEsp32Board& FakeEsp32();
