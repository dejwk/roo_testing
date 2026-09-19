// Synchronous modern IDF master API over the pin-routed emulated I2C devices.
#include <array>
#include <climits>
#include <mutex>
#include <new>
#include <set>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "soc/gpio_sig_map.h"

/// Host-owned bus routing and registered devices behind the opaque IDF handle.
struct i2c_master_bus_t {
  int port;
  gpio_num_t sda;
  gpio_num_t scl;
  std::set<i2c_master_dev_handle_t> devices;
};

/// Addressed peripheral registration owned by an emulated master bus.
struct i2c_master_dev_t {
  i2c_master_bus_handle_t bus;
  uint16_t address;
};

namespace {
struct Registry {
  std::mutex mutex;
  std::array<i2c_master_bus_handle_t, SOC_I2C_NUM> buses{};
};

// Shares bus ownership and serializes complete transfers, including repeated
// START.
Registry& GetRegistry() {
  static Registry value;
  return value;
}

struct Signals {
  uint8_t sda_out, sda_in, scl_out, scl_in;
};

constexpr std::array<Signals, SOC_I2C_NUM> kSignals = {{
    {I2CEXT0_SDA_OUT_IDX, I2CEXT0_SDA_IN_IDX, I2CEXT0_SCL_OUT_IDX,
     I2CEXT0_SCL_IN_IDX},
#if SOC_I2C_NUM > 1
    {I2CEXT1_SDA_OUT_IDX, I2CEXT1_SDA_IN_IDX, I2CEXT1_SCL_OUT_IDX,
     I2CEXT1_SCL_IN_IDX},
#endif
}};

static_assert(SOC_I2C_NUM <= 2, "Add the selected SoC's I2C matrix signals");
// Tests registry membership without dereferencing an external handle.
bool IsValidBus(i2c_master_bus_handle_t bus) {
  if (bus == nullptr) return false;
  for (i2c_master_bus_handle_t current : GetRegistry().buses) {
    if (current == bus) return true;
  }
  return false;
}

// Checks membership before dereferencing possibly invalid external handles.
bool IsValidDevice(i2c_master_dev_handle_t device) {
  if (device == nullptr) return false;
  for (i2c_master_bus_handle_t bus : GetRegistry().buses) {
    if (bus != nullptr && bus->devices.count(device) != 0) return true;
  }
  return false;
}

bool IsValidPin(gpio_num_t pin) {
  return pin >= 0 && pin < SOC_GPIO_PIN_COUNT && GPIO_IS_VALID_OUTPUT_GPIO(pin);
}

// The emulated device protocol stores timeout milliseconds in 16 bits.
uint16_t TimeoutValue(int timeout) {
  return timeout < 0 || timeout > UINT16_MAX ? UINT16_MAX : timeout;
}

// Converts emulator device errors into the public modern IDF error contract.
esp_err_t ResultCode(int32_t result) {
  switch (result) {
    case FakeI2cDevice::I2C_ERROR_OK:
      return ESP_OK;
    case ESP_ERR_NOT_FOUND:
    case FakeI2cDevice::I2C_ERROR_DEV:
    case FakeI2cDevice::I2C_ERROR_ACK:
      return ESP_ERR_INVALID_RESPONSE;
    case FakeI2cDevice::I2C_ERROR_TIMEOUT:
    case FakeI2cDevice::I2C_ERROR_BUSY:
      return ESP_ERR_TIMEOUT;
    case FakeI2cDevice::I2C_ERROR_MEMORY:
      return ESP_ERR_NO_MEM;
    default:
      return ESP_FAIL;
  }
}
}  // namespace
extern "C" {
esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t* config,
                             i2c_master_bus_handle_t* result) {
  if (config == nullptr || result == nullptr ||
      !IsValidPin(config->sda_io_num) || !IsValidPin(config->scl_io_num) ||
      config->sda_io_num == config->scl_io_num) {
    return ESP_ERR_INVALID_ARG;
  }
  *result = nullptr;
  // Background queues and power-domain behavior are not modeled.
  if (config->trans_queue_depth != 0 || config->flags.allow_pd != 0) {
    return ESP_ERR_NOT_SUPPORTED;
  }
  Registry& r = GetRegistry();
  std::lock_guard<std::mutex> lock(r.mutex);
  int port = config->i2c_port;
  if (port == -1) {
    for (size_t i = 0; i < r.buses.size(); ++i) {
      if (r.buses[i] == nullptr) {
        port = i;
        break;
      }
    }
    if (port == -1) return ESP_ERR_NOT_FOUND;
  }
  if (port < 0 || static_cast<size_t>(port) >= r.buses.size()) {
    return ESP_ERR_INVALID_ARG;
  }
  if (r.buses[port] != nullptr) return ESP_ERR_NOT_FOUND;
  auto* bus = new (std::nothrow)
      i2c_master_bus_t{port, config->sda_io_num, config->scl_io_num, {}};
  if (bus == nullptr) return ESP_ERR_NO_MEM;
  const Signals& sig = kSignals[port];
  FakeEsp32().out_matrix.assign(bus->sda, sig.sda_out, false, false);
  FakeEsp32().in_matrix.assign(bus->sda, sig.sda_in, false);
  FakeEsp32().out_matrix.assign(bus->scl, sig.scl_out, false, false);
  FakeEsp32().in_matrix.assign(bus->scl, sig.scl_in, false);
  r.buses[port] = bus;
  *result = bus;
  return ESP_OK;
}

esp_err_t i2c_master_get_bus_handle(i2c_port_num_t port,
                                    i2c_master_bus_handle_t* result) {
  if (result == nullptr || port < 0 ||
      static_cast<size_t>(port) >= GetRegistry().buses.size()) {
    return ESP_ERR_INVALID_ARG;
  }
  std::lock_guard<std::mutex> lock(GetRegistry().mutex);
  *result = GetRegistry().buses[port];
  return *result != nullptr ? ESP_OK : ESP_ERR_NOT_FOUND;
}

esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus,
                                    const i2c_device_config_t* config,
                                    i2c_master_dev_handle_t* result) {
  if (config == nullptr || result == nullptr || config->scl_speed_hz == 0) {
    return ESP_ERR_INVALID_ARG;
  }
  *result = nullptr;
  if (config->dev_addr_length != I2C_ADDR_BIT_LEN_7 ||
      config->flags.disable_ack_check != 0) {
    return ESP_ERR_NOT_SUPPORTED;
  }
  if (config->device_address > 0x7f) return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(GetRegistry().mutex);
  if (!IsValidBus(bus)) return ESP_ERR_INVALID_ARG;
  auto* device =
      new (std::nothrow) i2c_master_dev_t{bus, config->device_address};
  if (device == nullptr) return ESP_ERR_NO_MEM;
  bus->devices.insert(device);
  *result = device;
  return ESP_OK;
}

esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t device) {
  std::lock_guard<std::mutex> lock(GetRegistry().mutex);
  if (!IsValidDevice(device)) return ESP_ERR_INVALID_ARG;
  device->bus->devices.erase(device);
  delete device;
  return ESP_OK;
}

esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t bus) {
  std::lock_guard<std::mutex> lock(GetRegistry().mutex);
  if (!IsValidBus(bus)) return ESP_ERR_INVALID_ARG;
  if (!bus->devices.empty()) return ESP_ERR_INVALID_STATE;
  const Signals& sig = kSignals[bus->port];
  FakeEsp32().out_matrix.assign(bus->sda, kMatrixDetachOutSig, false, false);
  FakeEsp32().out_matrix.assign(bus->scl, kMatrixDetachOutSig, false, false);
  FakeEsp32().in_matrix.assign(kMatrixDetachInUndefPin, sig.sda_in, false);
  FakeEsp32().in_matrix.assign(kMatrixDetachInUndefPin, sig.scl_in, false);
  GetRegistry().buses[bus->port] = nullptr;
  delete bus;
  return ESP_OK;
}

esp_err_t i2c_master_transmit(i2c_master_dev_handle_t device,
                              const uint8_t* data, size_t size, int timeout) {
  if (data == nullptr || size == 0 || size > UINT16_MAX) {
    return ESP_ERR_INVALID_ARG;
  }
  std::lock_guard<std::mutex> lock(GetRegistry().mutex);
  if (!IsValidDevice(device)) return ESP_ERR_INVALID_ARG;
  return ResultCode(
      FakeEsp32()
          .i2c(device->bus->port)
          .write(device->address, data, size, TimeoutValue(timeout)));
}

esp_err_t i2c_master_receive(i2c_master_dev_handle_t device, uint8_t* data,
                             size_t size, int timeout) {
  if (data == nullptr || size == 0 || size > UINT16_MAX) {
    return ESP_ERR_INVALID_ARG;
  }
  std::lock_guard<std::mutex> lock(GetRegistry().mutex);
  if (!IsValidDevice(device)) return ESP_ERR_INVALID_ARG;
  return ResultCode(
      FakeEsp32()
          .i2c(device->bus->port)
          .read(device->address, data, size, TimeoutValue(timeout)));
}

esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t device,
                                      const uint8_t* tx, size_t tx_size,
                                      uint8_t* rx, size_t rx_size,
                                      int timeout) {
  if (tx == nullptr || rx == nullptr || tx_size == 0 || rx_size == 0 ||
      tx_size > UINT16_MAX || rx_size > UINT16_MAX) {
    return ESP_ERR_INVALID_ARG;
  }
  std::lock_guard<std::mutex> lock(GetRegistry().mutex);
  if (!IsValidDevice(device)) return ESP_ERR_INVALID_ARG;
  Esp32I2c& controller = FakeEsp32().i2c(device->bus->port);
  const esp_err_t result = ResultCode(controller.write(
      device->address, tx, tx_size, TimeoutValue(timeout), false));
  if (result != ESP_OK) return result;
  return ResultCode(
      controller.read(device->address, rx, rx_size, TimeoutValue(timeout)));
}

esp_err_t i2c_master_probe(i2c_master_bus_handle_t bus, uint16_t address, int) {
  if (address > 0x7f) return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(GetRegistry().mutex);
  if (!IsValidBus(bus)) return ESP_ERR_INVALID_ARG;
  return FakeEsp32().i2c(bus->port).probe(address) ? ESP_OK : ESP_ERR_NOT_FOUND;
}
}
