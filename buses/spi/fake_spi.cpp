#include "fake_spi.h"

#include <iostream>
#include <map>

#include <glog/logging.h>

FakeSpiInterface::FakeSpiInterface(
    std::initializer_list<SimpleFakeSpiDevice*> devices) {
  for (auto device : devices) {
    attach(device);
  }
}

void FakeSpiInterface::attach(std::unique_ptr<SimpleFakeSpiDevice> device) {
  devices_.push_back(std::move(device));
}

extern "C" {

void spiFakeTransfer(uint8_t spi_num, uint32_t clk, SpiDataMode mode,
                     SpiBitOrder order, uint8_t* buf, uint16_t bit_count) {
  FakeSpiInterface* spi = getSpiInterface(spi_num);
  if (spi == nullptr) {
    LOG(WARNING) << "SPI interface #" << spi_num << " is not attached";
    return;
  }
  SimpleFakeSpiDevice* selected_dev = nullptr;
  for (int i = 0; i < spi->device_count(); i++) {
    SimpleFakeSpiDevice& dev = spi->device(i);
    if (dev.isSelected()) {
      if (selected_dev == nullptr) {
        selected_dev = &dev;
      } else {
        LOG(WARNING) << "SPI interface #" << spi_num << ": bus conflict" << "\n";
        return;
      }
    }
  }
  if (selected_dev == nullptr) {
    LOG(WARNING) << "SPI interface #" << spi_num << ": no device selected";
    return;
  }
  selected_dev->transfer(clk, mode, order, buf, bit_count);
}

}
