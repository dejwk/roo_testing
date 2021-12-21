#include "fake_spi.h"

FakeSpiInterface::FakeSpiInterface(
    std::initializer_list<FakeSpiDevice*> devices) {
  for (auto device : devices) {
    addDevice(device);
  }
}

void FakeSpiInterface::addDevice(std::unique_ptr<FakeSpiDevice> device) {
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

void spiFakeTransfer(uint8_t spi_num, uint8_t* buf, uint16_t bit_count) {
//   FakeSpiDevice* device = getDevice(i2c_num, address);
//   if (device == nullptr) {
//     return FakeI2cDevice::I2C_ERROR_DEV;
//   } else {
//     return device->write(buff, size, sendStop, timeOutMillis);
//   }
}

}