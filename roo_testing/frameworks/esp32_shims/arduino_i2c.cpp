#include "esp32-hal-i2c.h"
#include "esp32-hal-i2c-slave.h"
#include "esp32-hal-periman.h"
#include "soc/gpio_sig_map.h"

#include <array>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

namespace {
struct I2cState {
  bool initialized = false;
  int8_t sda = -1;
  int8_t scl = -1;
  uint32_t frequency = 100000;
};

struct I2cSignals {
  uint8_t sda_out;
  uint8_t sda_in;
  uint8_t scl_out;
  uint8_t scl_in;
};

constexpr std::array<I2cSignals, SOC_I2C_NUM> kI2cSignals = {{
    {I2CEXT0_SDA_OUT_IDX, I2CEXT0_SDA_IN_IDX, I2CEXT0_SCL_OUT_IDX,
     I2CEXT0_SCL_IN_IDX},
#if SOC_I2C_NUM > 1
    {I2CEXT1_SDA_OUT_IDX, I2CEXT1_SDA_IN_IDX, I2CEXT1_SCL_OUT_IDX,
     I2CEXT1_SCL_IN_IDX},
#endif
}};

static_assert(SOC_I2C_NUM <= 2,
              "add the selected SoC's additional I2C signal mappings");

std::array<I2cState, SOC_I2C_NUM> buses;
bool valid(uint8_t bus) { return bus < buses.size(); }
bool validPin(int8_t pin) { return pin >= 0 && pin < SOC_GPIO_PIN_COUNT; }

void routePins(uint8_t bus, int8_t sda, int8_t scl) {
  const I2cSignals &signals = kI2cSignals[bus];
  FakeEsp32().out_matrix.assign(sda, signals.sda_out, false, false);
  FakeEsp32().in_matrix.assign(sda, signals.sda_in, false);
  FakeEsp32().out_matrix.assign(scl, signals.scl_out, false, false);
  FakeEsp32().in_matrix.assign(scl, signals.scl_in, false);
}

void unroutePins(uint8_t bus, int8_t sda, int8_t scl) {
  const I2cSignals &signals = kI2cSignals[bus];
  FakeEsp32().out_matrix.assign(sda, kMatrixDetachOutSig, false, false);
  FakeEsp32().in_matrix.assign(kMatrixDetachInUndefPin, signals.sda_in, false);
  FakeEsp32().out_matrix.assign(scl, kMatrixDetachOutSig, false, false);
  FakeEsp32().in_matrix.assign(kMatrixDetachInUndefPin, signals.scl_in, false);
}
}  // namespace

extern "C" {

esp_err_t i2cInit(uint8_t bus, int8_t sda, int8_t scl, uint32_t frequency) {
  if (!valid(bus) || !validPin(sda) || !validPin(scl)) {
    return ESP_ERR_INVALID_ARG;
  }
  if (frequency == 0) frequency = 100000;
  if (frequency > 1000000) frequency = 1000000;
  if (!perimanSetPinBus(sda, ESP32_BUS_TYPE_I2C_MASTER_SDA, &buses[bus],
                        bus, -1)) {
    return ESP_ERR_INVALID_STATE;
  }
  if (!perimanSetPinBus(scl, ESP32_BUS_TYPE_I2C_MASTER_SCL, &buses[bus],
                        bus, -1)) {
    perimanClearPinBus(sda);
    return ESP_ERR_INVALID_STATE;
  }
  buses[bus] = {true, sda, scl, frequency};
  routePins(bus, sda, scl);
  return ESP_OK;
}

esp_err_t i2cDeinit(uint8_t bus) {
  if (!valid(bus)) return ESP_ERR_INVALID_ARG;
  if (buses[bus].initialized) {
    unroutePins(bus, buses[bus].sda, buses[bus].scl);
  }
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

#if SOC_I2C_SUPPORT_SLAVE
// Slave-mode events have never been modeled by roo_testing. Arduino's Wire
// implementation keeps the master and slave methods in the same object file,
// so exact-signature unsupported stubs are still required for master-only
// applications to link.
esp_err_t i2cSlaveAttachCallbacks(
    uint8_t, i2c_slave_request_cb_t, i2c_slave_receive_cb_t, void *) {
  return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t i2cSlaveInit(uint8_t, int, int, uint16_t, uint32_t, size_t,
                       size_t) {
  return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t i2cSlaveDeinit(uint8_t) { return ESP_ERR_NOT_SUPPORTED; }

size_t i2cSlaveWrite(uint8_t, const uint8_t *, uint32_t, uint32_t) {
  return 0;
}
#endif

}  // extern "C"
