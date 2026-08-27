#pragma once

#include <stdint.h>

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "roo_testing/transducers/voltage/voltage.h"

/// Emulates one GPIO pin and retains the most recently written signal.
class FakeGpioPin {
 public:
  /// Creates an unwritten GPIO pin.
  FakeGpioPin() = default;

  /// Destroys the GPIO pin.
  virtual ~FakeGpioPin() = default;

  /// Returns the pin's descriptive name.
  virtual const std::string& name() const = 0;

  /// Returns the instantaneous voltage at current emulator uptime.
  virtual float read() const;

  /// Returns the instantaneous voltage at an explicit emulator uptime.
  virtual float readAtUptimeMicros(int64_t uptime_us) const;

  /// Returns the digital classification of the current instantaneous voltage.
  roo_testing_transducers::DigitalLevel digitalRead() const;

  /// Returns whether the current instantaneous voltage is digital low.
  bool isDigitalLow() const;

  /// Returns whether the current instantaneous voltage is digital high.
  bool isDigitalHigh() const;

  /// Retains and forwards a complete voltage signal.
  void write(const roo_testing_transducers::VoltageSignal& signal);

  /// Retains and forwards a constant voltage signal.
  void write(float voltage);

  /// Retains and forwards a constant digital voltage.
  void digitalWrite(roo_testing_transducers::DigitalLevel level);

  /// Retains and forwards a digital-high voltage.
  void digitalWriteHigh();

  /// Retains and forwards a digital-low voltage.
  void digitalWriteLow();

  /// Returns a copy of the last written signal, if any.
  std::optional<roo_testing_transducers::VoltageSignal> lastSignal() const;

  /// Returns the current local DC voltage of the last signal, or NaN.
  float last_written() const;

 protected:
  /// Forwards a newly committed signal to an attached device.
  virtual void onWrite(const roo_testing_transducers::VoltageSignal& signal) {}

 private:
  mutable std::mutex mutex_;
  std::optional<roo_testing_transducers::VoltageSignal> last_signal_;
};

/// Owns the GPIO-pin topology for one fake microcontroller.
class FakeGpioInterface {
 public:
  /// Creates an interface with the supplied number of addressable pins.
  FakeGpioInterface(int size) : size_(size) {}

  /// Attaches a voltage sink without taking ownership.
  void attachOutput(int pin, roo_testing_transducers::VoltageSink& voltage);

  /// Attaches a voltage sink and takes ownership.
  void attachOutput(
      int pin, std::unique_ptr<roo_testing_transducers::VoltageSink> voltage);

  /// Attaches a voltage source without taking ownership.
  void attachInput(int pin,
                   const roo_testing_transducers::VoltageSource& voltage);

  /// Attaches a voltage source and takes ownership.
  void attachInput(
      int pin,
      std::unique_ptr<const roo_testing_transducers::VoltageSource> voltage);

  /// Returns the emulated pin, creating an unattached output when necessary.
  FakeGpioPin& get(int pin) const;

  /// Attaches a bi-directional voltage device without taking ownership.
  void attach(int pin, roo_testing_transducers::VoltageIO& fake);

  /// Attaches a bi-directional voltage device and takes ownership.
  void attach(int pin,
              std::unique_ptr<roo_testing_transducers::VoltageIO> fake);

  /// Detaches the device currently assigned to a pin.
  void detach(int pin);

 private:
  int size_;
  void attachInternal(int pin, FakeGpioPin* fake);

  mutable std::vector<std::unique_ptr<FakeGpioPin>> pins_;
};
