#include "fake_esp32.h"

#include <limits.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <random>
#include <set>
#include <thread>

#include "glog/logging.h"

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

FakeEsp32Board* CreateEsp32Board() {
  google::InitGoogleLogging("ESP32");
  FLAGS_logtostderr = false;
  return new FakeEsp32Board();
}

FakeEsp32Board& FakeEsp32() {
  static FakeEsp32Board* esp32 = CreateEsp32Board();
  return *esp32;
}

namespace {
roo_testing_transducers::wifi::Environment* empty_env() {
  static roo_testing_transducers::wifi::Environment env;
  return &env;
}
}

FakeEsp32Board::FakeEsp32Board()
    : gpio(40),
      in_matrix(),
      out_matrix(),
      nvs(default_nvs_file()),
      reg_(*this),
      uart_{Esp32UartInterface("uart0", /* U0TXD_OUT_IDX*/ 14,
                               /* U0RXD_IN_IDX*/ 14, this),
            Esp32UartInterface("uart1", /* U0TXD_OUT_IDX*/ 17,
                               /* U0RXD_IN_IDX*/ 17, this),
            Esp32UartInterface("uart2", /* U0TXD_OUT_IDX*/ 198,
                               /* U0RXD_IN_IDX*/ 198, this)},
      // i2c{FakeI2cInterface("i2c0"), FakeI2cInterface("i2c1")},
      adc_{
        Esp32Adc(*this, {36, 37, 38, 39, 32, 33, 34, 35, -1, -1}),
        Esp32Adc(*this, {4, 0, 2, 15, 13, 12, 14, 27, 25, 26})
      },
      i2c_{
        Esp32I2c(*this, "i2c0", /*I2CEXT0_SDA_*_IDX*/ 30, /*I2CEXT0_SCL_*_IDX8*/ 29),
        Esp32I2c(*this, "i2c1", /*I2CEXT1_SDA_*_IDX*/ 96, /*I2CEXT1_SCL_*_IDX8*/ 95),
      },
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
      time_([this]() { flush(); }),
      wifi_env_(empty_env()) {
  FLAGS_alsologtostderr = true;
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

void FakeEsp32Board::attachI2cDevice(FakeI2cDevice& dev, int8_t sda,
                                      int8_t scl) {
  i2c_devices_to_pins_[&dev] = I2cPins{
      .sda = sda,
      .scl = scl,
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

void FakeEsp32Board::attachOneWireBus(FakeOneWireInterface& dev, int8_t pin) {
  onewire_buses_[pin] = &dev;
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

const char* GetVfsRoot() {
  return FakeEsp32().fs_root().c_str();
}
