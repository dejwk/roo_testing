#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "hardware/gpio.h"
#include "roo_testing/buses/gpio/fake_gpio.h"

/// Owns RP2040 GPIO configuration and borrowed voltage-device connections.
/// SDK access and fixture configuration are single-threaded. Attach devices
/// before configuring their pins and retain them through all GPIO activity.
class FakePicoBoard {
 public:
  /// Keeps the process-wide board topology unique.
  FakePicoBoard(const FakePicoBoard&) = delete;

  /// Prevents replacing the process-wide board state.
  FakePicoBoard& operator=(const FakePicoBoard&) = delete;

  static constexpr uint kGpioCount = 30;
  static constexpr uint32_t kGpioMask = (uint32_t{1} << kGpioCount) - 1;

  /// Captures a pin's software latch, mux, and supported pad controls.
  struct PinState {
    gpio_function_t function = GPIO_FUNC_NULL;
    bool direction_out = false;
    bool output_latch = false;
    bool pull_up = false;
    bool pull_down = true;
    bool input_enabled = true;
    uint input_override = GPIO_OVERRIDE_NORMAL;
    uint output_override = GPIO_OVERRIDE_NORMAL;
    uint output_enable_override = GPIO_OVERRIDE_NORMAL;
  };

  /// Connects a borrowed external source; active output drive is a conflict.
  void attachInput(uint pin,
                   const roo_testing_transducers::VoltageSource& source);

  /// Connects a borrowed observer of strong output voltage assignments.
  void attachOutput(uint pin, roo_testing_transducers::VoltageSink& sink);

  /// Connects borrowed bidirectional I/O; NaN input denotes external high
  /// impedance.
  void attach(uint pin, roo_testing_transducers::VoltageIO& io);

  /// Returns a checked, read-only snapshot of one pin's configuration.
  PinState state(uint pin) const;

  /// Returns the active strong output signal, or no value for high impedance.
  std::optional<roo_testing_transducers::VoltageSignal> driveSignal(
      uint pin) const;

  /// Samples the resolved pad voltage; NaN denotes an undriven floating pad.
  /// External sources override weak pulls. Opposing strong drivers fail.
  float padVoltage(uint pin) const;

  /// Reads the digital input after input enable and input override.
  /// Unforced floating or intermediate voltages fail instead of guessing a bit.
  bool read(uint pin) const;

  /// Reads all digital inputs into the low 30 bits.
  uint32_t readAll() const;

  /// Initializes selected pins as SIO inputs with a low output latch.
  /// Pull configuration is preserved, matching gpio_init().
  void init(uint32_t mask);

  /// Selects SIO or NULL, resets overrides, and enables pad input.
  void setFunction(uint32_t mask, gpio_function_t function);

  /// Updates selected raw SIO output latches without requiring output
  /// direction.
  void put(uint32_t mask, uint32_t value);

  /// Toggles selected raw SIO output latches.
  void toggle(uint32_t mask);

  /// Updates selected SIO directions, independent of the output-enable
  /// override.
  void setDirection(uint32_t mask, uint32_t value);

  /// Sets weak pulls; the unsupported two-pull bus keeper fails explicitly.
  void setPulls(uint pin, bool up, bool down);

  /// Enables or disables the pad input before applying the input override.
  void setInputEnabled(uint pin, bool enabled);

  /// Changes the input override to a checked SDK override value.
  void setInputOverride(uint pin, uint value);

  /// Changes the SIO output-level override to a checked SDK override value.
  void setOutputOverride(uint pin, uint value);

  /// Changes the SIO output-enable override to a checked SDK override value.
  void setOutputEnableOverride(uint pin, uint value);

  /// Rejects an invalid GPIO number, including converted negative values.
  static void CheckPin(uint pin);

  /// Rejects masks that name a nonexistent RP2040 GPIO.
  static void CheckMask(uint64_t mask);

 private:
  friend FakePicoBoard& FakePico();

  FakePicoBoard() : gpio_(kGpioCount) {}

  enum class Attachment : uint8_t { kNone, kInput, kOutput, kIo };

  void checkAttachment(uint pin) const;

  void publish(uint32_t mask);

  template <typename Update>
  void updatePins(uint32_t mask, Update update);

  std::array<PinState, kGpioCount> pins_{};
  std::array<Attachment, kGpioCount> attachments_{};
  std::array<bool, kGpioCount> configured_{};
  FakeGpioInterface gpio_;
};

/// Returns the lazily constructed process-wide Pico board without hardware I/O.
FakePicoBoard& FakePico();
