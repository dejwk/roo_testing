#include "esp_err.h"
#include "hal/assert.h"
#include "roo_testing/devices/microcontroller/esp32/fake_esp32.h"

extern "C" {

esp_err_t i2cWrite(uint8_t i2c_num, uint16_t address, const uint8_t* buff,
                   size_t size, uint32_t timeOutMillis) {
  return FakeEsp32().i2c(i2c_num).write(address, buff, size, timeOutMillis);
}

esp_err_t i2cRead(uint8_t i2c_num, uint16_t address, uint8_t* buff, size_t size,
                  uint32_t timeOutMillis, size_t* readCount) {
  esp_err_t ret =
      FakeEsp32().i2c(i2c_num).read(address, buff, size, timeOutMillis);
  if (ret == ESP_OK) {
    *readCount = size;
  } else {
    *readCount = 0;
  }
  return ret;
}
}
