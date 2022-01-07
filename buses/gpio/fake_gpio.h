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

class SimpleFakeGpioPin : public FakeGpioPin {
 public:
  SimpleFakeGpioPin() : name_("<unnamed>") {}
  SimpleFakeGpioPin(const std::string& name) : name_(name) {}

  std::string name() const override { return name_; }

 private:
  std::string name_;
};

class SimpleFakeGpioOutput : public SimpleFakeGpioPin {
 public:
  SimpleFakeGpioOutput(
      std::function<void(roo_testing_transducers::DigitalLevel)> trigger)
      : SimpleFakeGpioPin(), trigger_(trigger) {}

  SimpleFakeGpioOutput(
      const std::string& name,
      std::function<void(roo_testing_transducers::DigitalLevel)> trigger)
      : SimpleFakeGpioPin(name), trigger_(trigger) {}

  void onWrite(float voltage) override {
    trigger_(roo_testing_transducers::DigitalLevelFromVoltage(voltage));
  }

 private:
  std::function<void(roo_testing_transducers::DigitalLevel)> trigger_;
};

class SimpleFakeGpioAnalogOutput : public SimpleFakeGpioPin {
 public:
  SimpleFakeGpioAnalogOutput(std::function<void(float)> trigger)
      : SimpleFakeGpioPin(), trigger_(trigger) {}

  SimpleFakeGpioAnalogOutput(const std::string& name,
                             std::function<void(float)> trigger)
      : SimpleFakeGpioPin(name), trigger_(trigger) {}

  void onWrite(float voltage) override { trigger_(voltage); }

 private:
  std::function<void(float)> trigger_;
};

class FakeGpioInterface {
 public:
  FakeGpioInterface();
  ~FakeGpioInterface();

  void attach(uint8_t pin, FakeGpioPin& fake);
  void attach(uint8_t pin, std::unique_ptr<FakeGpioPin> fake);

  void detach(uint8_t pin);

  // Does not take ownership.
  void attachInput(uint8_t pin,
                   const roo_testing_transducers::VoltageSource& voltage);

  // Takes ownership.
  void attachInput(
      uint8_t pin,
      std::unique_ptr<const roo_testing_transducers::VoltageSource> voltage);

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
