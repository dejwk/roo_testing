#pragma once

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "roo_testing/buses/gpio/fake_gpio.h"

enum SpiDataMode {
  kSpiMode0 = 0,
  kSpiMode1 = 1,
  kSpiMode2 = 2,
  kSpiMode3 = 3,
};

enum SpiBitOrder {
  kSpiLsbFirst = 0,
  kSpiMsbFirst = 1,
};

class FakeSpiInterface {
 public:
  FakeSpiInterface(const std::string& name) : name_(name) {}
  virtual ~FakeSpiInterface() {}

  const std::string& name() const { return name_; }

  virtual uint32_t clkHz() const = 0;
  virtual SpiDataMode dataMode() const = 0;
  virtual SpiBitOrder bitOrder() const = 0;

 private:
  const std::string name_;
};

class SimpleFakeSpiDevice {
 public:
  SimpleFakeSpiDevice(const std::string& name, uint8_t cs)
      : name_(name), cs_pin_(cs), cs_(name + ":CS") {
    getGpioInterface()->attach(cs, cs_);
  }

  virtual ~SimpleFakeSpiDevice() { getGpioInterface()->detach(cs_pin_); }

  const std::string& name() const { return name_; }

  bool isSelected() const { return cs_.isDigitalLow(); }

  virtual void transfer(const FakeSpiInterface& spi, uint8_t* buf,
                        uint16_t bit_count) = 0;

  virtual void flush() {}

 private:
  const std::string name_;
  uint8_t cs_pin_;
  SimpleFakeGpioPin cs_;
};
