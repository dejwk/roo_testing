#include "fake_esp32.h"

#include <limits.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <random>
#include <set>
#include <thread>

#include "glog/logging.h"

FakeEsp32Board& FakeEsp32() {
  static FakeEsp32Board esp32;
  return esp32;
}

FakeI2cInterface* getI2cInterface(uint8_t i2c_num) {
  return &FakeEsp32().i2c[i2c_num];
}

Esp32InMatrix::Esp32InMatrix() {
  std::fill(&signal_to_pin_[0], &signal_to_pin_[256], kMatrixDetachInUndefPin);
}

Esp32OutMatrix::Esp32OutMatrix() {
  std::fill(&pin_to_signal_[0], &pin_to_signal_[40], kMatrixDetachInUndefPin);
}

namespace {

std::string default_fs_root_path() {
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) != nullptr) {
    return std::string(cwd) + "/fs_root";
  } else {
    LOG(ERROR) << "getcwd() failed";
    return "";
  }
}

std::string default_nvs_file() {
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) != nullptr) {
    return std::string(cwd) + "/nvs.txt";
  } else {
    LOG(ERROR) << "getcwd() failed";
    return "";
  }
}

}  // namespace

FakeEsp32Board::FakeEsp32Board()
    : gpio(40),
      in_matrix(),
      out_matrix(),
      uart_{Esp32UartInterface("uart0", /* U0TXD_OUT_IDX*/ 14,
                               /* U0RXD_IN_IDX*/ 14, this),
            Esp32UartInterface("uart1", /* U0TXD_OUT_IDX*/ 17,
                               /* U0RXD_IN_IDX*/ 17, this),
            Esp32UartInterface("uart2", /* U0TXD_OUT_IDX*/ 198,
                               /* U0RXD_IN_IDX*/ 198, this)},
      i2c{FakeI2cInterface("i2c0"), FakeI2cInterface("i2c1")},
      adc_{
        Esp32Adc(*this, {36, 37, 38, 39, 32, 33, 34, 35, -1, -1}),
        Esp32Adc(*this, {4, 0, 2, 15, 13, 12, 14, 27, 25, 26})
      },
      wifi(),
      nvs(default_nvs_file()),
      spi{Esp32SpiInterface(SPI0, "spi0(internal)", /*SPICLK_OUT_IDX*/ 0,
                            /*SPIQ_OUT_IDX*/ 1, /*SPID_IN_IDX*/ 2, this),
          Esp32SpiInterface(SPI1, "spi1(FSPI)", /*SPICLK_OUT_IDX*/ 0,
                            /*SPIQ_OUT_IDX*/ 1,
                            /*SPID_IN_IDX*/ 2, this),
          Esp32SpiInterface(SPI2, "spi2(HSPI)", /*HSPICLK_OUT_IDX*/ 8,
                            /*HSPIQ_OUT_IDX*/ 9,
                            /*HSPID_IN_IDX*/ 10, this),
          Esp32SpiInterface(SPI3, "spi3(VSPI)", /*VSPICLK_OUT_IDX*/ 63,
                            /*VSPIQ_OUT_IDX*/ 64,
                            /*VSPID_IN_IDX*/ 65, this)},
      fs_root_(default_fs_root_path()),
      time_([this]() { flush(); }) {
  FLAGS_stderrthreshold = google::WARNING;
  attachUartDevice(*(new ConsoleUartDevice()), 1, 3);
  out_matrix.assign(1, 14, false, false);
}

void FakeEsp32Board::attachUartDevice(FakeUartDevice& dev, int8_t tx,
                                      int8_t rx) {
  uart_devices_to_pins_[&dev] = UartPins{
      .tx = tx,
      .rx = rx,
  };
}

void FakeEsp32Board::attachSpiDevice(SimpleFakeSpiDevice& dev, int8_t clk,
                                     int8_t miso, int8_t mosi) {
  spi_devices_to_pins_[&dev] = SpiPins{
      .clk = clk,
      .miso = miso,
      .mosi = mosi,
  };
}

void FakeEsp32Board::flush() {
  for (auto& dev : spi_devices_to_pins_) {
    dev.first->flush();
  }
}

void Esp32InMatrix::assign(uint32_t gpio, uint32_t signal_idx, bool inv) {
  uint8_t old_pin = signal_to_pin_[signal_idx];
  signal_to_pin_[signal_idx] = gpio;
  pin_to_signals_.erase(old_pin);
  if (gpio != kMatrixDetachInUndefPin) {
    pin_to_signals_[gpio] = signal_idx;
  }
}

void Esp32OutMatrix::assign(uint32_t gpio, uint32_t signal_idx, bool out_inv,
                            bool oen_inv) {
  uint16_t old_signal = pin_to_signal_[gpio];
  signal_to_pins_.erase(old_signal);
  pin_to_signal_[gpio] = signal_idx;
  if (gpio == kMatrixDetachOutSig) {
    signal_to_pins_.erase(signal_idx);
  } else {
    signal_to_pins_[signal_idx] = gpio;
  }
}

// Declared in Arduino.h
long random(long min, long max) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_int_distribution<long> distrib(min, max);
  return distrib(gen);
}

// Declared in Arduino.h
long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

namespace roo_testing_transducers {

Clock* getDefaultSystemClock() { return &FakeEsp32().time(); }

}  // namespace roo_testing_transducers
