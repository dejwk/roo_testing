#pragma once

#include <map>
#include <memory>

#include <stdint.h>

class FakeGpioPin {
 public:
  FakeGpioPin() {}
  virtual ~FakeGpioPin() {}

  virtual float read() = 0;
  virtual void write(float voltage) = 0;
};

class FakeGpioInterface {
 public:
  FakeGpioInterface();

  void attach(uint8_t pin, FakeGpioPin* fake) {
    attach(pin, std::unique_ptr<FakeGpioPin>(fake));
  }
  void attach(uint8_t pin, std::unique_ptr<FakeGpioPin> fake);
  FakeGpioPin* get(uint8_t pin) const;

 private:
  std::map<uint8_t, std::unique_ptr<FakeGpioPin>> pins_;
  std::unique_ptr<FakeGpioPin> default_;
};

FakeGpioInterface* getGpioInterface();
