#pragma once

#include <memory>
#include <vector>

#include <stdint.h>

#include "roo_testing/buses/gpio/fake_gpio.h"

class FakeSpiDevice {
 public:
  FakeSpiDevice(const FakeGpioPin& cs) : cs_(cs) {}
  virtual ~FakeSpiDevice() {}

  virtual void transfer(uint8_t* buf, uint16_t size) = 0;

 private:
  const FakeGpioPin& cs_;
};

class FakeSpiInterface {
 public:
  FakeSpiInterface() {}

  FakeSpiInterface(std::initializer_list<FakeSpiDevice*> devices);

  void addDevice(FakeSpiDevice* device) {
    addDevice(std::unique_ptr<FakeSpiDevice>(device));
  }
  void addDevice(std::unique_ptr<FakeSpiDevice> device);

 private:
  std::vector<std::unique_ptr<FakeSpiDevice>> devices_;
};

void attachSpiInterface(uint8_t spi_num, FakeSpiInterface* spi);
FakeSpiInterface* getSpiInterface(uint8_t spi_num);
