#include "fake_spi.h"

#include <iostream>
#include <map>

FakeSpiInterface::FakeSpiInterface(
    std::initializer_list<SimpleFakeSpiDevice*> devices) {
  for (auto device : devices) {
    addDevice(device);
  }
}

void FakeSpiInterface::addDevice(std::unique_ptr<SimpleFakeSpiDevice> device) {
  devices_.push_back(std::move(device));
}

std::map<uint8_t, std::unique_ptr<FakeSpiInterface>>* spiMap() {
  static std::map<uint8_t, std::unique_ptr<FakeSpiInterface>> spiMap;
  return &spiMap;
}

FakeSpiInterface* getSpiInterface(uint8_t spi_num) {
  auto* map = spiMap();
  auto i = map->find(spi_num);
  return (i == map->end() ? nullptr : i->second.get());
}

void attachSpiInterface(uint8_t spi_num, FakeSpiInterface* spi) {
  spiMap()->emplace(spi_num, spi);
}

extern "C" {

void spiFakeTransfer(uint8_t spi_num, uint32_t clk, SpiDataMode mode,
                     SpiBitOrder order, uint8_t* buf, uint16_t bit_count) {
  FakeSpiInterface* spi = getSpiInterface(spi_num);
  if (spi == nullptr) {
    std::cerr << "SPI interface #" << spi_num << " is not attached" << "\n";
    return;
  }
  SimpleFakeSpiDevice* selected_dev = nullptr;
  for (int i = 0; i < spi->device_count(); i++) {
    SimpleFakeSpiDevice& dev = spi->device(i);
    if (dev.isSelected()) {
      if (selected_dev == nullptr) {
        selected_dev = &dev;
      } else {
        std::cerr << "SPI interface #" << spi_num << ": bus conflict" << "\n";
        return;
      }
    }
  }
  if (selected_dev == nullptr) {
    std::cerr << "SPI interface #" << spi_num << ": no device selected" << "\n";
    return;
  }
  selected_dev->transfer(clk, mode, order, buf, bit_count);
}

}
