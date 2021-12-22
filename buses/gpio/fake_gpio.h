#pragma once

#include <stdint.h>

#include <cmath>
#include <cstdlib>
#include <memory>
#include <vector>

class FakeGpioPin {
 public:
  enum DigitalLevel { kDigitalLow = 0, kDigitalHigh = 1, kDigitalUndef = -1 };

  FakeGpioPin() : last_written_(std::nanf("")) {}
  virtual ~FakeGpioPin() {}

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

  bool isDigitalLow() const {
    return digitalRead() == kDigitalLow;
  }

  bool isDigitalHigh() const {
    return digitalRead() == kDigitalHigh;
  }

  void write(float voltage) {
    last_written_ = voltage;
    onWrite(voltage);
  }

  virtual void onWrite(float voltage) {}

  float last_written() { return last_written_; }

 private:
  float last_written_;
};

class FakeGpioInterface {
 public:
  FakeGpioInterface();

  void attach(uint8_t pin, FakeGpioPin* fake) {
    attach(pin, std::unique_ptr<FakeGpioPin>(fake));
  }
  void attach(uint8_t pin, std::unique_ptr<FakeGpioPin> fake);
  FakeGpioPin& get(uint8_t pin) const;

 private:
  mutable std::vector<std::unique_ptr<FakeGpioPin>> pins_;
};

FakeGpioInterface* getGpioInterface();
