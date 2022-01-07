#pragma once

#include <stdint.h>

#include <cmath>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "roo_testing/transducers/voltage/voltage.h"

class FakeGpioPin {
 public:
  FakeGpioPin() : last_written_(std::nanf("")) {}

  virtual ~FakeGpioPin() {}

  virtual const std::string& name() const = 0;

  virtual float read() const {
    if (std::isnan(last_written_)) {
      // The value has never been written; assume floating.
      return (float)rand() * 5.0 / RAND_MAX;
    }
    // Otherwise, default to returning last written value. This is normal
    // behavior for microcontroller pins in the output mode.
    return last_written_;
  }

  roo_testing_transducers::DigitalLevel digitalRead() const {
    return roo_testing_transducers::DigitalLevelFromVoltage(read());
  }

  bool isDigitalLow() const {
    return digitalRead() == roo_testing_transducers::kDigitalLow;
  }

  bool isDigitalHigh() const {
    return digitalRead() == roo_testing_transducers::kDigitalHigh;
  }

  void write(float voltage) {
    last_written_ = voltage;
    onWrite(voltage);
  }

  void digitalWrite(roo_testing_transducers::DigitalLevel level) {
    write(level == roo_testing_transducers::kDigitalLow    ? 0.0
          : level == roo_testing_transducers::kDigitalHigh ? 3.3
                                                           : 1.5);
  }

  void digitalWriteHigh() {
    digitalWrite(roo_testing_transducers::kDigitalHigh);
  }

  void digitalWriteLow() { digitalWrite(roo_testing_transducers::kDigitalLow); }

  virtual void onWrite(float voltage) {}

  float last_written() { return last_written_; }

 private:
  float last_written_;
};

class FakeGpioInterface {
 public:
  FakeGpioInterface();

  // Attaches a voltage sink. Does not take ownership.
  void attachOutput(uint8_t pin, roo_testing_transducers::VoltageSink& voltage);

  // Attaches a voltage sink. Takes ownership.
  void attachOutput(
      uint8_t pin,
      std::unique_ptr<roo_testing_transducers::VoltageSink> voltage);

  // Attaches a voltage source. Does not take ownership.
  void attachInput(uint8_t pin,
                   const roo_testing_transducers::VoltageSource& voltage);

  // Attaches a voltage source. Takes ownership.
  void attachInput(
      uint8_t pin,
      std::unique_ptr<const roo_testing_transducers::VoltageSource> voltage);

  FakeGpioPin& get(uint8_t pin) const;

  // Attaches a bi-directional voltage device. Does not take ownership.
  void attach(uint8_t pin, roo_testing_transducers::VoltageIO& fake);

  // Attaches a bi-directional voltage device. Takes ownership.
  void attach(uint8_t pin,
              std::unique_ptr<roo_testing_transducers::VoltageIO> fake);

  void detach(uint8_t pin);

 private:
  void attachInternal(uint8_t pin, FakeGpioPin* fake);

  mutable std::vector<std::unique_ptr<FakeGpioPin>> pins_;
};

FakeGpioInterface* getGpioInterface();
