#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

#include <cmath>
#include <cstdlib>

#include "glog/logging.h"

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
  const std::optional<bool> level = StrongLevel(pins_[pin]);
  if (!level.has_value()) return std::nullopt;
  return VoltageSignal::Constant(
      VoltageFromDigitalLevel(*level ? kDigitalHigh : kDigitalLow));
}

float FakePicoBoard::padVoltage(uint pin) const {
  CheckPin(pin);
  float external = std::nanf("");
  if (attachments_[pin] == Attachment::kInput ||
      attachments_[pin] == Attachment::kIo) {
    external = gpio_.get(pin).read();
    CHECK(std::isfinite(external) || std::isnan(external))
        << "Invalid Pico GPIO source voltage";
  }
  const std::optional<bool> driven = StrongLevel(pins_[pin]);
  if (driven.has_value()) {
    const DigitalLevel level = *driven ? kDigitalHigh : kDigitalLow;
    if (std::isfinite(external)) {
      CHECK(DigitalLevelFromVoltage(external) == level)
          << "Pico GPIO strong-driver contention";
    }
    return VoltageFromDigitalLevel(level);
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
  CheckMask(mask);
  std::array<PinState, kGpioCount> next = pins_;
  uint32_t changed_drive = 0;
  for (uint pin = 0; pin < kGpioCount; ++pin) {
    if ((mask & (uint32_t{1} << pin)) == 0) continue;
    update(next[pin], pin);
    CHECK(!StrongLevel(next[pin]).has_value() ||
          attachments_[pin] != Attachment::kInput)
        << "Pico GPIO output conflicts with an attached input source";
    if (StrongLevel(next[pin]) != StrongLevel(pins_[pin])) {
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
  CHECK(function == GPIO_FUNC_SIO || function == GPIO_FUNC_NULL)
      << "Unsupported Pico GPIO function";
  updatePins(mask, [function](PinState& state, uint) {
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
