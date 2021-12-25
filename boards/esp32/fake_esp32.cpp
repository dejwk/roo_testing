#include "roo_testing/boards/esp32/fake_esp32.h"

FakeEsp32Board& FakeEsp32() {
  static FakeEsp32Board esp32;
  return esp32;
}

FakeGpioInterface* getGpioInterface() {
  return &FakeEsp32().gpio;
}

FakeI2cInterface* getI2cInterface(uint8_t i2c_num) {
  return &FakeEsp32().i2c[i2c_num];
}

FakeSpiInterface* getSpiInterface(uint8_t spi_num) {
  return &FakeEsp32().spi[spi_num];
}
