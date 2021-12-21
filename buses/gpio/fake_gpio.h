#pragma once

#include <stdint.h>

#include <vector>
#include <cmath>
#include <cstdlib>
#include <memory>

class FakeGpioPin {
 public:
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
