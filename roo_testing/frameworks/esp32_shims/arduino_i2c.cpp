#include "esp32-hal-i2c.h"
#include "esp32-hal-periman.h"

#include <array>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

namespace {
struct I2cState {
  bool initialized = false;
  int8_t sda = -1;
  int8_t scl = -1;
  uint32_t frequency = 100000;
};
std::array<I2cState, SOC_I2C_NUM> buses;
bool valid(uint8_t bus) { return bus < buses.size(); }
}  // namespace

extern "C" {

esp_err_t i2cInit(uint8_t bus, int8_t sda, int8_t scl, uint32_t frequency) {
  if (!valid(bus) || sda < 0 || scl < 0) return ESP_ERR_INVALID_ARG;
  buses[bus] = {true, sda, scl, frequency};
  perimanSetPinBus(sda, ESP32_BUS_TYPE_I2C_MASTER_SDA, &buses[bus], bus, -1);
  perimanSetPinBus(scl, ESP32_BUS_TYPE_I2C_MASTER_SCL, &buses[bus], bus, -1);
  return ESP_OK;
}

esp_err_t i2cDeinit(uint8_t bus) {
  if (!valid(bus)) return ESP_ERR_INVALID_ARG;
  if (buses[bus].sda >= 0) perimanClearPinBus(buses[bus].sda);
  if (buses[bus].scl >= 0) perimanClearPinBus(buses[bus].scl);
  buses[bus] = {};
  return ESP_OK;
}

esp_err_t i2cSetClock(uint8_t bus, uint32_t frequency) {
  if (!valid(bus) || !buses[bus].initialized || frequency == 0) return ESP_ERR_INVALID_ARG;
  buses[bus].frequency = frequency;
  return ESP_OK;
}

esp_err_t i2cGetClock(uint8_t bus, uint32_t *frequency) {
  if (!valid(bus) || !frequency || !buses[bus].initialized) return ESP_ERR_INVALID_ARG;
  *frequency = buses[bus].frequency;
  return ESP_OK;
}

esp_err_t i2cWrite(uint8_t bus, uint16_t address, const uint8_t *data,
                   size_t size, uint32_t timeout_ms) {
  if (!valid(bus) || !buses[bus].initialized || (!data && size)) return ESP_ERR_INVALID_ARG;
  return static_cast<esp_err_t>(FakeEsp32().i2c(bus).write(address, data, size, timeout_ms));
}

esp_err_t i2cRead(uint8_t bus, uint16_t address, uint8_t *data, size_t size,
                  uint32_t timeout_ms, size_t *read_count) {
  if (read_count) *read_count = 0;
  if (!valid(bus) || !buses[bus].initialized || (!data && size)) return ESP_ERR_INVALID_ARG;
  esp_err_t result = static_cast<esp_err_t>(FakeEsp32().i2c(bus).read(address, data, size, timeout_ms));
  if (result == ESP_OK && read_count) *read_count = size;
  return result;
}

esp_err_t i2cWriteReadNonStop(uint8_t bus, uint16_t address,
                              const uint8_t *write_data, size_t write_size,
                              uint8_t *read_data, size_t read_size,
                              uint32_t timeout_ms, size_t *read_count) {
  esp_err_t result = i2cWrite(bus, address, write_data, write_size, timeout_ms);
  return result == ESP_OK
             ? i2cRead(bus, address, read_data, read_size, timeout_ms, read_count)
             : result;
}

bool i2cIsInit(uint8_t bus) { return valid(bus) && buses[bus].initialized; }
void *i2cBusHandle(uint8_t bus) { return i2cIsInit(bus) ? &buses[bus] : nullptr; }

}  // extern "C"
