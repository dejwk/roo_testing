#pragma once

#include <stdint.h>

#include <map>
#include <memory>

class FakeGpioPin {
 public:
  FakeGpioPin() {}
  virtual ~FakeGpioPin() {}

  virtual float read() const = 0;
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

class SimpleFakeGpioPin : public FakeGpioPin {
 public:
  SimpleFakeGpioPin() : voltage_(0.0) {}

  float read() const override { return voltage_; }
  void write(float voltage) override { voltage_ = voltage; }

 private:
  float voltage_;
};
