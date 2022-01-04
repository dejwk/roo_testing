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
  enum DigitalLevel { kDigitalLow = 0, kDigitalHigh = 1, kDigitalUndef = -1 };

  FakeGpioPin() : last_written_(std::nanf("")) {}

  virtual ~FakeGpioPin() {}

  virtual std::string name() const { return "<unnamed>"; }

  virtual float read() const {
    if (std::isnan(last_written_)) {
      // The value has never been written; assume floating.
      return (float)rand() * 5.0 / RAND_MAX;
    }
    // Otherwise, default to returning last written value. This is normal
    // behavior for microcontroller pins in the output mode.
    return last_written_;
  }

  DigitalLevel digitalRead() const {
    float voltage = read();
    return voltage <= 0.8   ? kDigitalLow
           : voltage >= 2.0 ? kDigitalHigh
                            : kDigitalUndef;
  }

  bool isDigitalLow() const { return digitalRead() == kDigitalLow; }

  bool isDigitalHigh() const { return digitalRead() == kDigitalHigh; }

  void write(float voltage) {
    last_written_ = voltage;
    onWrite(voltage);
  }

  void digitalWrite(DigitalLevel level) {
    write(level == kDigitalLow ? 0.0 : level == kDigitalHigh ? 3.3 : 1.5);
  }

  void digitalWriteHigh() { digitalWrite(kDigitalHigh); }

  void digitalWriteLow() { digitalWrite(kDigitalLow); }

  virtual void onWrite(float voltage) {}

  float last_written() { return last_written_; }

 private:
  float last_written_;
};

class SimpleFakeGpioPin : public FakeGpioPin {
 public:
  SimpleFakeGpioPin(const std::string& name) : name_(name) {}

  std::string name() const override { return name_; }

 private:
  std::string name_;
};

class FakeGpioInterface {
 public:
  FakeGpioInterface();
  ~FakeGpioInterface();

  void attach(uint8_t pin, FakeGpioPin* fake);

  void detach(uint8_t pin);

  // Does not take ownership.
  void attachInput(uint8_t pin, const roo_testing_transducers::Voltage& voltage);

  // Takes ownership.
  void attachInput(uint8_t pin,
                   std::unique_ptr<const roo_testing_transducers::Voltage> voltage);

  FakeGpioPin& get(uint8_t pin) const;

 private:
  void attachInternal(uint8_t pin, FakeGpioPin* fake, bool owned);

  struct Pin {
    Pin() : ptr(nullptr), owned(false) {}

    FakeGpioPin* ptr;
    bool owned;
  };

  mutable std::vector<Pin> pins_;
};

FakeGpioInterface* getGpioInterface();
