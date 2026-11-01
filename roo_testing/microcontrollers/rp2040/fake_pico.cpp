#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

#include "glog/logging.h"
#include "roo_testing/system/timer.h"

using roo_testing_transducers::DigitalLevel;
using roo_testing_transducers::DigitalLevelFromVoltage;
using roo_testing_transducers::kDigitalHigh;
using roo_testing_transducers::kDigitalLow;
using roo_testing_transducers::kDigitalUndef;
using roo_testing_transducers::VoltageFromDigitalLevel;
using roo_testing_transducers::VoltageSignal;

namespace {

/// Applies an override that was validated before storing it in pin state.
bool ApplyOverride(bool value, uint control) {
  switch (control) {
    case GPIO_OVERRIDE_NORMAL:
      return value;
    case GPIO_OVERRIDE_INVERT:
      return !value;
    case GPIO_OVERRIDE_LOW:
      return false;
    case GPIO_OVERRIDE_HIGH:
      return true;
    default:
      LOG(FATAL) << "Invalid Pico GPIO override";
  }
  std::abort();
}

/// Separates the software output latch from an electrically enabled output.
std::optional<bool> StrongLevel(const FakePicoBoard::PinState& state) {
  if (state.function != GPIO_FUNC_SIO ||
      !ApplyOverride(state.direction_out, state.output_enable_override)) {
    return std::nullopt;
  }
  return ApplyOverride(state.output_latch, state.output_override);
}

/// Checks SDK override values before any state or external signal changes.
void CheckOverride(uint value) {
  CHECK_LE(value, static_cast<uint>(GPIO_OVERRIDE_HIGH))
      << "Invalid Pico GPIO override";
}

/// Maps physical pin groups, including UART1 on both GP4 and GP8 groups.
uint UartIndex(uint pin) {
  constexpr uint8_t indices[] = {0, 1, 1, 0, 0, 1, 1, 0};
  FakePicoBoard::CheckPin(pin);
  return indices[pin / 4];
}

/// Implements the pinned SDK divisor calculation at the default peri clock.
uint ActualBaudrate(uint requested) {
  CHECK_NE(requested, 0u) << "Invalid Pico UART baudrate";
  constexpr uint64_t clock = 125000000;
  const uint64_t divisor = 8 * clock / requested + 1;
  const uint64_t integer = divisor >> 7;
  if (integer == 0) return clock / 16;
  if (integer >= 65535) return 4 * clock / (64 * 65535);
  return 4 * clock / (64 * integer + ((divisor & 0x7f) >> 1));
}

/// Shares the existing ESP32 console device implementation without SDK policy.
ConsoleUartDevice& HostConsole() {
  static ConsoleUartDevice console;
  return console;
}

/// Uses the RP2040 SPI0/SPI1 eight-pin group alternation.
uint SpiIndex(uint pin) {
  FakePicoBoard::CheckPin(pin);
  return (pin / 8) % 2;
}

/// Matches the pinned SDK prescale/postdivide search at nominal clk_peri.
uint SpiBaudrate(uint requested) {
  constexpr uint64_t clock = 125000000;
  CHECK(requested != 0 && requested <= clock) << "Invalid Pico SPI baudrate";
  uint prescale;
  for (prescale = 2; prescale <= 254; prescale += 2) {
    if (clock < prescale * 256 * uint64_t{requested}) break;
  }
  CHECK_LE(prescale, 254u) << "Invalid Pico SPI baudrate: too low";
  uint postdiv;
  for (postdiv = 256; postdiv > 1; --postdiv) {
    if (clock / (prescale * (postdiv - 1)) > requested) break;
  }
  return clock / (prescale * postdiv);
}

}  // namespace

FakePicoBoard& FakePico() {
  static FakePicoBoard board;
  return board;
}

void FakePicoBoard::CheckPin(uint pin) {
  CHECK_LT(pin, kGpioCount) << "Invalid Pico GPIO pin";
}

void FakePicoBoard::CheckMask(uint64_t mask) {
  CHECK_EQ(mask & ~static_cast<uint64_t>(kGpioMask), uint64_t{0})
      << "Invalid Pico GPIO mask";
}

void FakePicoBoard::checkAttachment(uint pin) const {
  checkBusesIdle();
  CheckPin(pin);
  CHECK(attachments_[pin] == Attachment::kNone)
      << "Pico GPIO attachment conflict";
  CHECK(!configured_[pin])
      << "Attach Pico GPIO devices before configuring the pin";
}

void FakePicoBoard::attachInput(
    uint pin, const roo_testing_transducers::VoltageSource& source) {
  checkAttachment(pin);
  gpio_.attachInput(pin, source);
  attachments_[pin] = Attachment::kInput;
}

void FakePicoBoard::attachOutput(uint pin,
                                 roo_testing_transducers::VoltageSink& sink) {
  checkAttachment(pin);
  gpio_.attachOutput(pin, sink);
  attachments_[pin] = Attachment::kOutput;
}

void FakePicoBoard::attach(uint pin, roo_testing_transducers::VoltageIO& io) {
  checkAttachment(pin);
  gpio_.attach(pin, io);
  attachments_[pin] = Attachment::kIo;
}

FakePicoBoard::PinState FakePicoBoard::state(uint pin) const {
  CheckPin(pin);
  return pins_[pin];
}

std::optional<VoltageSignal> FakePicoBoard::driveSignal(uint pin) const {
  CheckPin(pin);
  CHECK(pins_[pin].function != GPIO_FUNC_I2C &&
        attachments_[pin] != Attachment::kI2c)
      << "Unimplemented: Pico I2C pin voltage";
  CHECK(pins_[pin].function != GPIO_FUNC_SPI &&
        attachments_[pin] != Attachment::kSpi)
      << "Unimplemented: Pico SPI pin voltage";
  CHECK(pins_[pin].function != GPIO_FUNC_UART &&
        attachments_[pin] != Attachment::kUart)
      << "Unimplemented: Pico UART pin voltage";
  return pinDrive(pin, pins_[pin]);
}

float FakePicoBoard::padVoltage(uint pin) const {
  CheckPin(pin);
  CHECK(pins_[pin].function != GPIO_FUNC_I2C &&
        attachments_[pin] != Attachment::kI2c)
      << "Unimplemented: Pico I2C pin voltage";
  CHECK(pins_[pin].function != GPIO_FUNC_SPI &&
        attachments_[pin] != Attachment::kSpi)
      << "Unimplemented: Pico SPI pin voltage";
  CHECK(pins_[pin].function != GPIO_FUNC_UART &&
        attachments_[pin] != Attachment::kUart)
      << "Unimplemented: Pico UART pin voltage";
  float external = std::nanf("");
  if (attachments_[pin] == Attachment::kInput ||
      attachments_[pin] == Attachment::kIo) {
    external = gpio_.get(pin).read();
    CHECK(std::isfinite(external) || std::isnan(external))
        << "Invalid Pico GPIO source voltage";
  }
  const std::optional<VoltageSignal> driven = pinDrive(pin, pins_[pin]);
  if (driven.has_value()) {
    const float voltage =
        driven->voltageAtUptimeMicros(system_time_get_micros());
    const DigitalLevel level = DigitalLevelFromVoltage(voltage);
    if (std::isfinite(external)) {
      CHECK(DigitalLevelFromVoltage(external) == level)
          << "Pico GPIO strong-driver contention";
    }
    return voltage;
  }
  if (std::isfinite(external)) return external;
  if (pins_[pin].pull_up) return VoltageFromDigitalLevel(kDigitalHigh);
  if (pins_[pin].pull_down) return VoltageFromDigitalLevel(kDigitalLow);
  return std::nanf("");
}

bool FakePicoBoard::read(uint pin) const {
  CheckPin(pin);
  const PinState& config = pins_[pin];
  if (config.input_override == GPIO_OVERRIDE_LOW) return false;
  if (config.input_override == GPIO_OVERRIDE_HIGH) return true;
  if (!config.input_enabled) return ApplyOverride(false, config.input_override);
  const DigitalLevel level = DigitalLevelFromVoltage(padVoltage(pin));
  CHECK(level != kDigitalUndef) << "Pico GPIO input is floating or undefined";
  return ApplyOverride(level == kDigitalHigh, config.input_override);
}

uint32_t FakePicoBoard::readAll() const {
  uint32_t value = 0;
  for (uint pin = 0; pin < kGpioCount; ++pin) {
    if (read(pin)) value |= uint32_t{1} << pin;
  }
  return value;
}

/// Validates the entire prospective change before publishing state or signals.
/// Signal callbacks observe all selected state changes, without a board lock.
template <typename Update>
void FakePicoBoard::updatePins(uint32_t mask, Update update) {
  checkBusesIdle();
  CheckMask(mask);
  std::array<PinState, kGpioCount> next = pins_;
  uint32_t changed_drive = 0;
  for (uint pin = 0; pin < kGpioCount; ++pin) {
    if ((mask & (uint32_t{1} << pin)) == 0) continue;
    update(next[pin], pin);
    CHECK(!pinDrive(pin, next[pin]).has_value() ||
          attachments_[pin] != Attachment::kInput)
        << "Pico GPIO output conflicts with an attached input source";
    CHECK(attachments_[pin] != Attachment::kUart ||
          !pinDrive(pin, next[pin]).has_value())
        << "Unsupported Pico SIO output on a UART attachment";
    CHECK(attachments_[pin] != Attachment::kSpi ||
          !pinDrive(pin, next[pin]).has_value())
        << "Unsupported Pico SIO output on a SPI attachment";
    CHECK(attachments_[pin] != Attachment::kI2c ||
          !pinDrive(pin, next[pin]).has_value())
        << "Unsupported Pico SIO output on an I2C attachment";
    if (pinDrive(pin, next[pin]) != pinDrive(pin, pins_[pin])) {
      changed_drive |= uint32_t{1} << pin;
    }
  }
  pins_ = next;
  for (uint pin = 0; pin < kGpioCount; ++pin) {
    if ((mask & (uint32_t{1} << pin)) != 0) configured_[pin] = true;
  }
  publish(changed_drive);
}

void FakePicoBoard::publish(uint32_t mask) {
  for (uint pin = 0; pin < kGpioCount; ++pin) {
    if ((mask & (uint32_t{1} << pin)) == 0) continue;
    if (pins_[pin].function == GPIO_FUNC_I2C ||
        attachments_[pin] == Attachment::kI2c ||
        pins_[pin].function == GPIO_FUNC_SPI ||
        attachments_[pin] == Attachment::kSpi ||
        pins_[pin].function == GPIO_FUNC_UART ||
        attachments_[pin] == Attachment::kUart) {
      continue;
    }
    const std::optional<VoltageSignal> signal = driveSignal(pin);
    if (signal.has_value()) gpio_.get(pin).write(*signal);
  }
}

void FakePicoBoard::init(uint32_t mask) {
  updatePins(mask, [](PinState& state, uint) {
    state.function = GPIO_FUNC_SIO;
    state.direction_out = false;
    state.output_latch = false;
    state.input_enabled = true;
    state.input_override = GPIO_OVERRIDE_NORMAL;
    state.output_override = GPIO_OVERRIDE_NORMAL;
    state.output_enable_override = GPIO_OVERRIDE_NORMAL;
  });
}

void FakePicoBoard::setFunction(uint32_t mask, gpio_function_t function) {
  CHECK(function == GPIO_FUNC_SIO || function == GPIO_FUNC_NULL ||
        function == GPIO_FUNC_UART || function == GPIO_FUNC_SPI ||
        function == GPIO_FUNC_I2C || function == GPIO_FUNC_PWM)
      << "Unsupported Pico GPIO function";
  updatePins(mask, [this, function](PinState& state, uint pin) {
    if (function == GPIO_FUNC_I2C) {
      CHECK(attachments_[pin] == Attachment::kNone ||
            attachments_[pin] == Attachment::kI2c)
          << "Pico I2C conflicts with another attachment";
    }
    if (function == GPIO_FUNC_SPI) {
      CHECK_NE(pin % 4, 1u) << "Unsupported Pico GPIO function: SPI CSn";
      CHECK(attachments_[pin] == Attachment::kNone ||
            attachments_[pin] == Attachment::kSpi)
          << "Pico SPI conflicts with a GPIO attachment";
    }
    if (function == GPIO_FUNC_UART) {
      CHECK_LT(pin % 4, 2u) << "Unsupported Pico GPIO function: UART CTS/RTS";
      CHECK(attachments_[pin] == Attachment::kNone ||
            attachments_[pin] == Attachment::kUart)
          << "Pico UART conflicts with a GPIO attachment";
    }
    state.function = function;
    state.input_enabled = true;
    state.input_override = GPIO_OVERRIDE_NORMAL;
    state.output_override = GPIO_OVERRIDE_NORMAL;
    state.output_enable_override = GPIO_OVERRIDE_NORMAL;
  });
}

void FakePicoBoard::put(uint32_t mask, uint32_t value) {
  updatePins(mask, [value](PinState& state, uint pin) {
    state.output_latch = (value & (uint32_t{1} << pin)) != 0;
  });
}

void FakePicoBoard::toggle(uint32_t mask) {
  updatePins(mask, [](PinState& state, uint) {
    state.output_latch = !state.output_latch;
  });
}

void FakePicoBoard::setDirection(uint32_t mask, uint32_t value) {
  updatePins(mask, [value](PinState& state, uint pin) {
    state.direction_out = (value & (uint32_t{1} << pin)) != 0;
  });
}

void FakePicoBoard::setPulls(uint pin, bool up, bool down) {
  CheckPin(pin);
  CHECK(!up || !down) << "Unimplemented: Pico GPIO bus keeper";
  updatePins(uint32_t{1} << pin, [up, down](PinState& state, uint) {
    state.pull_up = up;
    state.pull_down = down;
  });
}

void FakePicoBoard::setInputEnabled(uint pin, bool enabled) {
  CheckPin(pin);
  updatePins(uint32_t{1} << pin, [enabled](PinState& state, uint) {
    state.input_enabled = enabled;
  });
}

void FakePicoBoard::setInputOverride(uint pin, uint value) {
  CheckPin(pin);
  CheckOverride(value);
  updatePins(uint32_t{1} << pin,
             [value](PinState& state, uint) { state.input_override = value; });
}

void FakePicoBoard::setOutputOverride(uint pin, uint value) {
  CheckPin(pin);
  CheckOverride(value);
  CHECK(value == GPIO_OVERRIDE_NORMAL || pins_[pin].function == GPIO_FUNC_SIO)
      << "Unsupported Pico GPIO override on a non-SIO pin";
  updatePins(uint32_t{1} << pin,
             [value](PinState& state, uint) { state.output_override = value; });
}

void FakePicoBoard::setOutputEnableOverride(uint pin, uint value) {
  CheckPin(pin);
  CheckOverride(value);
  CHECK(value == GPIO_OVERRIDE_NORMAL || pins_[pin].function == GPIO_FUNC_SIO)
      << "Unsupported Pico GPIO override on a non-SIO pin";
  updatePins(uint32_t{1} << pin, [value](PinState& state, uint) {
    state.output_enable_override = value;
  });
}

void FakePicoBoard::attachUartDevice(FakeUartDevice& device, int tx, int rx) {
  CHECK(tx >= 0 || rx >= 0) << "Pico UART needs a TX or RX pin";
  CHECK_GE(tx, -1) << "Invalid Pico UART TX pin";
  CHECK_GE(rx, -1) << "Invalid Pico UART RX pin";
  if (tx >= 0) {
    checkAttachment(tx);
    CHECK_EQ(tx % 4, 0) << "Invalid Pico UART TX pin";
  }
  if (rx >= 0) {
    checkAttachment(rx);
    CHECK_EQ(rx % 4, 1) << "Invalid Pico UART RX pin";
  }
  if (tx >= 0 && rx >= 0) {
    CHECK_EQ(UartIndex(tx), UartIndex(rx)) << "Pico UART controller mismatch";
  }
  for (const UartConnection& connection : uart_connections_) {
    CHECK(connection.device != &device) << "Pico UART device already attached";
  }
  uart_connections_.push_back({&device, tx, rx});
  if (tx >= 0) attachments_[tx] = Attachment::kUart;
  if (rx >= 0) attachments_[rx] = Attachment::kUart;
}

FakePicoBoard::UartState FakePicoBoard::uartState(uint index) const {
  CHECK_LT(index, uarts_.size()) << "Invalid Pico UART index";
  return uarts_[index];
}

uint FakePicoBoard::initUart(uint index, uint baudrate) {
  uartState(index);
  const uint actual = ActualBaudrate(baudrate);
  uarts_[index] = {true, actual, false};
  return actual;
}

void FakePicoBoard::deinitUart(uint index) {
  uartState(index);
  uarts_[index].enabled = false;
}

uint FakePicoBoard::setUartBaudrate(uint index, uint baudrate) {
  CHECK(uartState(index).enabled) << "Pico UART is disabled";
  const uint actual = ActualBaudrate(baudrate);
  uarts_[index].baudrate = actual;
  return actual;
}

void FakePicoBoard::setUartTranslateCrLf(uint index, bool enabled) {
  CHECK(uartState(index).enabled) << "Pico UART is disabled";
  uarts_[index].translate_crlf = enabled;
}

FakeUartDevice& FakePicoBoard::uartDevice(uint index, bool transmit) const {
  CHECK(uartState(index).enabled) << "Pico UART is disabled";
  FakeUartDevice* device = nullptr;
  uint active = 0;
  // Count every selected physical route, including unwired alternate pins.
  // Otherwise an invalid second route could silently pick a fixture endpoint.
  for (uint pin = 0; pin < kGpioCount; ++pin) {
    if (pins_[pin].function != GPIO_FUNC_UART || UartIndex(pin) != index ||
        pin % 4 != (transmit ? 0u : 1u)) {
      continue;
    }
    ++active;
    if (!transmit) {
      CHECK(pins_[pin].input_enabled &&
            pins_[pin].input_override == GPIO_OVERRIDE_NORMAL)
          << "Unsupported Pico UART RX pad configuration";
    }
    for (const UartConnection& connection : uart_connections_) {
      if (static_cast<int>(pin) == (transmit ? connection.tx : connection.rx)) {
        device = connection.device;
      }
    }
    if (pin == (transmit ? 0u : 1u) && attachments_[pin] == Attachment::kNone) {
      device = &HostConsole();
    }
  }
  CHECK_EQ(active, 1u) << "Pico UART needs exactly one active pin route";
  CHECK(device != nullptr) << "Pico UART route has no attached device";
  return *device;
}

void FakePicoBoard::checkBusesIdle() const {
  CHECK(!publishing_pwm_) << "Pico reconfiguration during PWM publication";
  for (const I2cState& i2c : i2cs_) {
    CHECK(!i2c.busy) << "Pico reconfiguration during I2C transfer";
  }
  for (const SpiState& spi : spis_) {
    CHECK(!spi.busy) << "Pico reconfiguration during SPI transfer";
  }
}

void FakePicoBoard::attachSpiDevice(SimpleFakeSpiDevice& device, int clk,
                                    int miso, int mosi) {
  checkBusesIdle();
  CheckPin(clk);
  CheckPin(mosi);
  CHECK_GE(miso, -1) << "Invalid Pico SPI RX pin";
  CHECK_EQ(clk % 4, 2) << "Invalid Pico SPI SCK pin";
  CHECK_EQ(mosi % 4, 3) << "Invalid Pico SPI TX pin";
  CHECK_EQ(SpiIndex(clk), SpiIndex(mosi)) << "Pico SPI controller mismatch";
  if (miso >= 0) {
    CheckPin(miso);
    CHECK_EQ(miso % 4, 0) << "Invalid Pico SPI RX pin";
    CHECK_EQ(SpiIndex(clk), SpiIndex(miso)) << "Pico SPI controller mismatch";
  }
  for (int pin : {clk, miso, mosi}) {
    if (pin < 0) continue;
    CHECK(attachments_[pin] == Attachment::kNone ||
          attachments_[pin] == Attachment::kSpi)
        << "Pico SPI attachment conflict";
    CHECK(!configured_[pin])
        << "Attach Pico SPI devices before configuring pins";
  }
  for (const SpiConnection& connection : spi_connections_) {
    CHECK(connection.device != &device) << "Pico SPI device already attached";
  }
  spi_connections_.push_back({&device, clk, miso, mosi});
  for (int pin : {clk, miso, mosi}) {
    if (pin >= 0) attachments_[pin] = Attachment::kSpi;
  }
}

FakePicoBoard::SpiState FakePicoBoard::spiState(uint index) const {
  CHECK_LT(index, spis_.size()) << "Invalid Pico SPI index";
  return spis_[index];
}

uint FakePicoBoard::initSpi(uint index, uint baudrate) {
  checkBusesIdle();
  spiState(index);
  const uint actual = SpiBaudrate(baudrate);
  spis_[index] = {true, false, actual, 8, kSpiMode0};
  return actual;
}

void FakePicoBoard::deinitSpi(uint index) {
  checkBusesIdle();
  spiState(index);
  spis_[index].enabled = false;
}

uint FakePicoBoard::setSpiBaudrate(uint index, uint baudrate) {
  checkBusesIdle();
  CHECK(spiState(index).enabled) << "Pico SPI is disabled";
  const uint actual = SpiBaudrate(baudrate);
  spis_[index].baudrate = actual;
  return actual;
}

void FakePicoBoard::setSpiFormat(uint index, uint data_bits, spi_cpol_t cpol,
                                 spi_cpha_t cpha, spi_order_t order) {
  checkBusesIdle();
  CHECK(spiState(index).enabled) << "Pico SPI is disabled";
  CHECK(data_bits == 8 || data_bits == 16)
      << "Unimplemented: Pico SPI frame width";
  CHECK(order == SPI_MSB_FIRST) << "Unimplemented: Pico SPI LSB-first order";
  CHECK(cpol == SPI_CPOL_0 || cpol == SPI_CPOL_1)
      << "Invalid Pico SPI polarity";
  CHECK(cpha == SPI_CPHA_0 || cpha == SPI_CPHA_1) << "Invalid Pico SPI phase";
  spis_[index].data_bits = data_bits;
  spis_[index].mode = static_cast<SpiDataMode>(2 * cpol + cpha);
}

int FakePicoBoard::prepareSpi(uint index, bool receive, SpiRoute& route) const {
  for (const I2cState& i2c : i2cs_) {
    if (i2c.busy) return PICO_ERROR_RESOURCE_IN_USE;
  }
  const SpiState config = spiState(index);
  if (!config.enabled) return PICO_ERROR_INVALID_STATE;
  for (const SpiState& spi : spis_) {
    if (spi.busy) return PICO_ERROR_RESOURCE_IN_USE;
  }
  int pins[4] = {-1, -1, -1, -1};
  for (uint pin = 0; pin < kGpioCount; ++pin) {
    if (pins_[pin].function != GPIO_FUNC_SPI || SpiIndex(pin) != index) {
      continue;
    }
    const uint role = pin % 4;
    if (pins[role] >= 0) return PICO_ERROR_INVALID_STATE;
    pins[role] = pin;
  }
  if (pins[2] < 0 || pins[3] < 0 || (receive && pins[0] < 0)) {
    return PICO_ERROR_INVALID_STATE;
  }
  if (receive && (!pins_[pins[0]].input_enabled ||
                  pins_[pins[0]].input_override != GPIO_OVERRIDE_NORMAL)) {
    return PICO_ERROR_INVALID_STATE;
  }
  bool wired = false;
  SpiRoute next;
  for (const SpiConnection& connection : spi_connections_) {
    if (connection.clk != pins[2] || connection.mosi != pins[3]) continue;
    wired = true;
    if (!connection.device->isSelected()) continue;
    next.devices.push_back(connection.device);
    if (receive && connection.miso == pins[0]) {
      if (next.receiver != nullptr) return PICO_ERROR_RESOURCE_IN_USE;
      next.receiver = connection.device;
    }
  }
  if (!wired) return PICO_ERROR_INVALID_STATE;
  if (receive && next.receiver == nullptr) {
    const PinState& input = pins_[pins[0]];
    if (!input.pull_up && !input.pull_down) return PICO_ERROR_INVALID_STATE;
    next.idle_high = input.pull_up;
  }
  route = std::move(next);
  return PICO_OK;
}

void FakePicoBoard::beginSpi(uint index) {
  checkBusesIdle();
  CHECK(spiState(index).enabled) << "Pico SPI is disabled";
  spis_[index].busy = true;
}

void FakePicoBoard::endSpi(uint index) {
  CHECK(spiState(index).busy) << "Pico SPI is not busy";
  spis_[index].busy = false;
}

void FakePicoBoard::attachI2cDevice(FakeI2cDevice& device, int sda, int scl) {
  checkBusesIdle();
  CheckPin(sda);
  CheckPin(scl);
  CHECK_EQ(sda % 2, 0) << "Invalid Pico I2C SDA pin";
  CHECK_EQ(scl % 2, 1) << "Invalid Pico I2C SCL pin";
  CHECK_EQ((sda / 2) % 2, (scl / 2) % 2) << "Pico I2C controller mismatch";
  CHECK(device.address() >= 8 && device.address() <= 0x77)
      << "Invalid Pico I2C address";
  for (int pin : {sda, scl}) {
    CHECK(attachments_[pin] == Attachment::kNone ||
          attachments_[pin] == Attachment::kI2c)
        << "Pico I2C attachment conflict";
    CHECK(!configured_[pin])
        << "Attach Pico I2C devices before configuring pins";
  }
  for (const I2cConnection& connection : i2c_connections_) {
    CHECK(connection.device != &device) << "Pico I2C device already attached";
    CHECK(connection.sda != sda || connection.scl != scl ||
          connection.device->address() != device.address())
        << "Duplicate Pico I2C address";
  }
  i2c_connections_.push_back({&device, sda, scl});
  attachments_[sda] = Attachment::kI2c;
  attachments_[scl] = Attachment::kI2c;
}

FakePicoBoard::I2cState FakePicoBoard::i2cState(uint index) const {
  CHECK_LT(index, i2cs_.size()) << "Invalid Pico I2C index";
  return i2cs_[index];
}

uint FakePicoBoard::initI2c(uint index, uint baudrate) {
  checkBusesIdle();
  i2cState(index);
  CHECK(baudrate != 0 && baudrate <= 1000000) << "Invalid Pico I2C baudrate";
  const uint64_t period = (125000000 + uint64_t{baudrate} / 2) / baudrate;
  const uint64_t low = period * 3 / 5;
  const uint64_t high = period - low;
  CHECK(low >= 8 && low <= 65535 && high >= 8 && high <= 65535)
      << "Invalid Pico I2C divider";
  const uint actual = 125000000 / period;
  i2cs_[index] = {true, false, false, actual};
  return actual;
}

void FakePicoBoard::deinitI2c(uint index) {
  checkBusesIdle();
  i2cState(index);
  i2cs_[index] = {};
}

uint FakePicoBoard::setI2cBaudrate(uint index, uint baudrate) {
  CHECK(i2cState(index).enabled) << "Pico I2C is disabled";
  CHECK(!i2cState(index).restart_on_next) << "Pico I2C bus is held";
  return initI2c(index, baudrate);
}

int FakePicoBoard::prepareI2c(uint index, uint8_t address,
                              FakeI2cDevice*& device) const {
  if (!i2cState(index).enabled) return PICO_ERROR_INVALID_STATE;
  for (const I2cState& state : i2cs_) {
    if (state.busy) return PICO_ERROR_RESOURCE_IN_USE;
  }
  for (const SpiState& state : spis_) {
    if (state.busy) return PICO_ERROR_RESOURCE_IN_USE;
  }
  int sda = -1;
  int scl = -1;
  for (uint pin = 0; pin < kGpioCount; ++pin) {
    if (pins_[pin].function != GPIO_FUNC_I2C || (pin / 2) % 2 != index) {
      continue;
    }
    int& selected = pin % 2 == 0 ? sda : scl;
    if (selected >= 0) return PICO_ERROR_INVALID_STATE;
    if (!pins_[pin].input_enabled ||
        pins_[pin].input_override != GPIO_OVERRIDE_NORMAL) {
      return PICO_ERROR_INVALID_STATE;
    }
    selected = pin;
  }
  if (sda < 0 || scl < 0) return PICO_ERROR_INVALID_STATE;
  device = nullptr;
  for (const I2cConnection& connection : i2c_connections_) {
    if (connection.sda != sda || connection.scl != scl ||
        connection.device->address() != address)
      continue;
    if (device != nullptr) return PICO_ERROR_RESOURCE_IN_USE;
    device = connection.device;
  }
  return device != nullptr ? PICO_OK : PICO_ERROR_GENERIC;
}

void FakePicoBoard::beginI2c(uint index) {
  checkBusesIdle();
  CHECK(i2cState(index).enabled);
  i2cs_[index].busy = true;
}

void FakePicoBoard::endI2c(uint index, bool restart) {
  CHECK(i2cState(index).busy);
  i2cs_[index].busy = false;
  i2cs_[index].restart_on_next = restart;
}

std::optional<VoltageSignal> FakePicoBoard::pinDrive(
    uint pin, const PinState& state) const {
  if (state.function == GPIO_FUNC_PWM) {
    const PwmState& pwm = pwms_[(pin / 2) % 8];
    const uint channel = pin % 2;
    if (!pwm.enabled) {
      const bool high =
          (pwm.counter < pwm.levels[channel]) != pwm.inverted[channel];
      return VoltageSignal::Constant(high ? 3.3f : 0.0f);
    }
    const double period = uint32_t{pwm.wrap} + 1;
    const double duty = std::min<double>(pwm.levels[channel] / period, 1.0);
    return VoltageSignal::Square(
        0, 3.3f, 125000000.0 * 16 / pwm.divider16 / period,
        roo_testing_transducers::ConstantDuty{duty}, pwm.origin_us,
        -static_cast<double>(pwm.counter) / period, pwm.inverted[channel]);
  }
  const std::optional<bool> level = StrongLevel(state);
  if (!level.has_value()) return std::nullopt;
  return VoltageSignal::Constant(*level ? 3.3f : 0.0f);
}

void FakePicoBoard::initAdc() {
  checkBusesIdle();
  adc_enabled_ = true;
  adc_input_ = 0;
}

void FakePicoBoard::initAdcGpio(uint pin) {
  CheckPin(pin);
  CHECK(pin >= 26 && pin <= 29) << "Invalid Pico ADC GPIO";
  CHECK(attachments_[pin] == Attachment::kNone ||
        attachments_[pin] == Attachment::kInput ||
        attachments_[pin] == Attachment::kIo)
      << "Pico ADC attachment conflict";
  updatePins(uint32_t{1} << pin, [](PinState& state, uint) {
    state.function = GPIO_FUNC_NULL;
    state.direction_out = false;
    state.input_enabled = false;
    state.pull_up = false;
    state.pull_down = false;
    state.input_override = GPIO_OVERRIDE_NORMAL;
    state.output_override = GPIO_OVERRIDE_NORMAL;
    state.output_enable_override = GPIO_OVERRIDE_NORMAL;
  });
  adc_pins_[pin - 26] = true;
}

void FakePicoBoard::selectAdc(uint input) {
  checkBusesIdle();
  CHECK(adc_enabled_) << "Pico ADC is not initialized";
  CHECK_LT(input, 4u) << "Unimplemented: Pico ADC internal channel";
  adc_input_ = input;
}

uint FakePicoBoard::adcInput() const {
  CHECK(adc_enabled_) << "Pico ADC is not initialized";
  return adc_input_;
}

uint16_t FakePicoBoard::readAdc() const {
  const uint input = adcInput();
  const uint pin = 26 + input;
  CHECK(adc_pins_[input] && pins_[pin].function == GPIO_FUNC_NULL &&
        !pins_[pin].input_enabled)
      << "Configure Pico ADC GPIO before conversion";
  const float voltage = padVoltage(pin);
  CHECK(std::isfinite(voltage)) << "Invalid Pico ADC voltage";
  const double value = std::clamp<double>(voltage / 3.3f, 0.0, 1.0) * 4096;
  return static_cast<uint16_t>(std::min<double>(std::floor(value), 4095));
}

FakePicoBoard::PwmState FakePicoBoard::pwmState(uint slice) const {
  CHECK_LT(slice, pwms_.size()) << "Invalid Pico PWM slice";
  return pwms_[slice];
}

uint16_t FakePicoBoard::pwmCounter(uint slice) const {
  const PwmState state = pwmState(slice);
  if (!state.enabled) return state.counter;
  const long double elapsed = system_time_get_micros() - state.origin_us;
  const long double ticks = elapsed * 125.0L * 16 / state.divider16;
  return static_cast<uint16_t>(
      std::fmod(ticks + state.counter, uint32_t{state.wrap} + 1));
}

void FakePicoBoard::setPwmState(uint slice, const PwmState& state) {
  checkBusesIdle();
  pwmState(slice);
  CHECK_GE(state.divider16, 16u) << "Invalid Pico PWM divider";
  CHECK_LE(state.divider16, 4095u) << "Invalid Pico PWM divider";
  pwms_[slice] = state;
  publishing_pwm_ = true;
  for (uint pin = 0; pin < kGpioCount; ++pin) {
    if (pins_[pin].function == GPIO_FUNC_PWM && (pin / 2) % 8 == slice) {
      publish(uint32_t{1} << pin);
    }
  }
  publishing_pwm_ = false;
}
