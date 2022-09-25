#include "fake_esp32.h"

#include <limits.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <random>
#include <set>
#include <thread>

#include "esp32-hal-spi.h"
#include "glog/logging.h"
#include "soc/spi_struct.h"

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

Esp32SpiInterface::Esp32SpiInterface(volatile SpiDevType& spi,
                                     const std::string& name,
                                     uint8_t clk_signal, uint8_t miso_signal,
                                     uint8_t mosi_signal, FakeEsp32Board* esp32)
    : FakeSpiInterface(name),
      spi_(spi),
      clk_signal_(clk_signal),
      miso_signal_(miso_signal),
      mosi_signal_(mosi_signal),
      esp32_(esp32) {}

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
      i2c{FakeI2cInterface("i2c0"), FakeI2cInterface("i2c1")},
      wifi(),
      nvs(default_nvs_file()),
      spi{Esp32SpiInterface(SPI0, "spi0(internal)", /*SPICLK_OUT_IDX*/ 0,
                            /*SPIQ_OUT_IDX*/ 1, /*SPID_IN_IDX*/ 2, this),
          Esp32SpiInterface(SPI1, "spi1(FSPI)", /*SPICLK_OUT_IDX*/ 0, /*SPIQ_OUT_IDX*/ 1,
                            /*SPID_IN_IDX*/ 2, this),
          Esp32SpiInterface(SPI2, "spi2(HSPI)", /*HSPICLK_OUT_IDX*/ 8, /*HSPIQ_OUT_IDX*/ 9,
                            /*HSPID_IN_IDX*/ 10, this),
          Esp32SpiInterface(SPI3, "spi3(VSPI)", /*VSPICLK_OUT_IDX*/ 63, /*VSPIQ_OUT_IDX*/ 64,
                            /*VSPID_IN_IDX*/ 65, this)},
      fs_root_(default_fs_root_path()),
      time_([this]() { flush(); }) {
  FLAGS_stderrthreshold = google::WARNING;
  google::InitGoogleLogging("ESP32_emulator");
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

extern "C" {

void gpioFakeWrite(uint8_t pin, float voltage) {
  FakeEsp32().gpio.get(pin).write(voltage);
}

float gpioFakeRead(uint8_t pin) { return FakeEsp32().gpio.get(pin).read(); }
}

}  // namespace roo_testing_transducers
