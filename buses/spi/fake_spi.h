#pragma once

#include <stdint.h>

#include <memory>
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

class SimpleFakeSpiDevice {
 public:
  SimpleFakeSpiDevice(uint8_t cs) : cs_(new FakeGpioPin()) {
    getGpioInterface()->attach(cs, cs_);
  }

  virtual ~SimpleFakeSpiDevice() {}

  bool isSelected() const { return cs_->isDigitalLow(); }

  virtual void transfer(uint32_t clk, SpiDataMode mode, SpiBitOrder order,
                        uint8_t* buf, uint16_t bit_count) = 0;

 private:
  FakeGpioPin* cs_;
};

class FakeSpiInterface {
 public:
  FakeSpiInterface() {}

  FakeSpiInterface(std::initializer_list<SimpleFakeSpiDevice*> devices);

  void addDevice(SimpleFakeSpiDevice* device) {
    addDevice(std::unique_ptr<SimpleFakeSpiDevice>(device));
  }
  void addDevice(std::unique_ptr<SimpleFakeSpiDevice> device);

  int device_count() const { return devices_.size(); }

  SimpleFakeSpiDevice& device(int pos) { return *devices_[pos]; }

  const SimpleFakeSpiDevice& device(int pos) const { return *devices_[pos]; }

 private:
  std::vector<std::unique_ptr<SimpleFakeSpiDevice>> devices_;
};

void attachSpiInterface(uint8_t spi_num, FakeSpiInterface* spi);
FakeSpiInterface* getSpiInterface(uint8_t spi_num);

extern "C" {

void spiFakeTransfer(uint8_t spi_num, uint32_t clk, SpiDataMode mode,
                     SpiBitOrder order, uint8_t* buf, uint16_t bit_count);
}