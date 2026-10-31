#include "fake_esp32.h"

#include <limits.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <random>
#include <set>

#include "glog/logging.h"
#include "nvs_path.h"

Esp32InMatrix::Esp32InMatrix() {
  std::fill(&signal_to_pin_[0], &signal_to_pin_[256], kMatrixDetachInUndefPin);
}

Esp32OutMatrix::Esp32OutMatrix() {
  std::fill(&pin_to_signal_[0], &pin_to_signal_[SOC_GPIO_PIN_COUNT],
            kMatrixDetachOutSig);
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

std::string resolve_fs_root_path(std::string path) {
  if (path.empty() || path.front() == '/') {
    return path;
  }
  const char* workspace = std::getenv("BUILD_WORKSPACE_DIRECTORY");
  if (workspace == nullptr || workspace[0] == '\0') {
    return path;
  }
  return std::string(workspace) + "/" + path;
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

FakeEsp32Board::FakeEsp32Board()
    : gpio(SOC_GPIO_PIN_COUNT),
      in_matrix(),
      out_matrix(),
      nvs(roo_testing::internal::DefaultNvsPath()),
      reg_(*this),
      uart_{{
          Esp32UartInterface(0, "uart0", roo_testing::soc::kUartTxSignals[0],
                             roo_testing::soc::kUartRxSignals[0], this),
          Esp32UartInterface(1, "uart1", roo_testing::soc::kUartTxSignals[1],
                             roo_testing::soc::kUartRxSignals[1], this),
#if SOC_UART_NUM > 2
          Esp32UartInterface(2, "uart2", roo_testing::soc::kUartTxSignals[2],
                             roo_testing::soc::kUartRxSignals[2], this),
#endif
      }},
      adc_{Esp32Adc(*this, {roo_testing::soc::kAdcPins[0].begin(),
                            roo_testing::soc::kAdcPins[0].end()}),
           Esp32Adc(*this, {roo_testing::soc::kAdcPins[1].begin(),
                            roo_testing::soc::kAdcPins[1].end()})},
      i2c_{{
          Esp32I2c(*this, "i2c0", roo_testing::soc::kI2cSdaSignals[0],
                   roo_testing::soc::kI2cSclSignals[0]),
#if SOC_I2C_NUM > 1
          Esp32I2c(*this, "i2c1", roo_testing::soc::kI2cSdaSignals[1],
                   roo_testing::soc::kI2cSclSignals[1]),
#endif
      }},
      spi{
          Esp32SpiInterface(SPI0, "spi0(internal)",
                            roo_testing::soc::kSpiClockSignals[0],
                            roo_testing::soc::kSpiMisoSignals[0],
                            roo_testing::soc::kSpiMosiSignals[0], this),
          Esp32SpiInterface(SPI1, "spi1(internal)",
                            roo_testing::soc::kSpiClockSignals[0],
                            roo_testing::soc::kSpiMisoSignals[0],
                            roo_testing::soc::kSpiMosiSignals[0], this),
          Esp32SpiInterface(SPI2, "spi2", roo_testing::soc::kSpiClockSignals[1],
                            roo_testing::soc::kSpiMisoSignals[1],
                            roo_testing::soc::kSpiMosiSignals[1], this),
#if SOC_SPI_PERIPH_NUM > 2
          Esp32SpiInterface(SPI3, "spi3", roo_testing::soc::kSpiClockSignals[2],
                            roo_testing::soc::kSpiMisoSignals[2],
                            roo_testing::soc::kSpiMosiSignals[2], this),
#else
          Esp32SpiInterface(SPI3, "unsupported spi3", 0, 0, 0, this),
#endif
      },
      fs_root_(default_fs_root_path()),
      // time_([this]() { flush(); }),
      wifi_env_(
          std::make_shared<roo_testing_transducers::wifi::Environment>()) {
  FLAGS_alsologtostderr = true;
  FLAGS_stderrthreshold = google::WARNING;
  attachUartDevice(*(new ConsoleUartDevice()), roo_testing::soc::kConsoleTx,
                   roo_testing::soc::kConsoleRx);
  out_matrix.assign(roo_testing::soc::kConsoleTx,
                    roo_testing::soc::kUartTxSignals[0], false, false);
}

void FakeEsp32Board::attachUartDevice(FakeUartDevice& dev, int8_t tx,
                                      int8_t rx) {
  uart_devices_to_pins_[&dev] = UartPins{
      .tx = tx,
      .rx = rx,
  };
  dev.setDataAvailableFn([this](const FakeUartDevice* source) {
    notifyUartDataAvailable(source);
  });
}

void FakeEsp32Board::notifyUartDataAvailable(const FakeUartDevice* source) {
  const auto& itr =
      uart_devices_to_pins_.find(const_cast<FakeUartDevice*>(source));
  if (itr == uart_devices_to_pins_.end()) {
    return;
  }
  int rx_pin = itr->second.rx;
  if (rx_pin < 0) {
    return;
  }
  for (Esp32UartInterface& uart : uart_) {
    if (in_matrix.pin_for_signal(uart.rx_signal()) == rx_pin) {
      uart.notifyDataAvailable();
    }
  }
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

void FakeEsp32Board::attachEspNowDevice(FakeEspNowDevice& dev) {
  esp_now().attachDevice(dev);
}

void FakeEsp32Board::attachOneWireBus(FakeOneWireInterface& dev, int8_t pin) {
  onewire_buses_[pin] = &dev;
}

void FakeEsp32Board::flush() {
  for (auto& dev : spi_devices_to_pins_) {
    dev.first->flush();
  }
}

void FakeEsp32Board::set_fs_root(std::string fs_root) {
  fs_root_ = resolve_fs_root_path(std::move(fs_root));
}

void Esp32InMatrix::assign(uint32_t gpio, uint32_t signal_idx, bool inv) {
  if (signal_idx >= 256) return;
  if (!roo_testing::soc::IsValidPin(gpio) && gpio != kMatrixDetachInLowPin &&
      gpio != kMatrixDetachInHighPin && gpio != kMatrixDetachInUndefPin) {
    return;
  }
  uint8_t old_pin = signal_to_pin_[signal_idx];
  signal_to_pin_[signal_idx] = gpio;
  pin_to_signals_.erase(old_pin);
  if (gpio != kMatrixDetachInUndefPin) {
    pin_to_signals_[gpio] = signal_idx;
  }
}

void Esp32OutMatrix::assign(uint32_t gpio, uint32_t signal_idx, bool out_inv,
                            bool oen_inv) {
  if (!roo_testing::soc::IsOutputPin(gpio)) return;
  uint16_t old_signal = pin_to_signal_[gpio];
  signal_to_pins_.erase(old_signal);
  pin_to_signal_[gpio] = signal_idx;
  if (signal_idx == kMatrixDetachOutSig) {
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

// Clock* getDefaultSystemClock() { return &FakeEsp32().time(); }

}  // namespace roo_testing_transducers

const char* GetVfsRoot() { return FakeEsp32().fs_root().c_str(); }
