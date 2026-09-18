#include <vector>

#include "driver/i2c_master.h"
#include "gtest/gtest.h"
#include "roo_testing/buses/i2c/fake_i2c.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

namespace {

/// Records transfer boundaries and injects emulated peripheral errors.
class RecordingDevice : public FakeI2cDevice {
 public:
  /// Creates a peripheral at unshifted address 0x68.
  RecordingDevice() : FakeI2cDevice("modern IDF I2C", 0x68) {}

  /// Records the write's STOP and timeout settings, returning the scripted
  /// error.
  Result write(const uint8_t* data, uint16_t size, bool stop,
               uint16_t timeout) override {
    sent.assign(data, data + size);
    write_stop = stop;
    timeout_ms = timeout;
    return result;
  }

  /// Returns a fixed byte pattern and records the read's STOP setting.
  Result read(uint8_t* data, uint16_t size, bool stop,
              uint16_t timeout) override {
    ++reads;
    read_stop = stop;
    timeout_ms = timeout;
    for (uint16_t i = 0; i < size; ++i) data[i] = 0x5a;
    return result;
  }

  Result result = I2C_ERROR_OK;
  std::vector<uint8_t> sent;
  bool write_stop = true;
  bool read_stop = false;
  uint16_t timeout_ms = 0;
  int reads = 0;
};

// Verifies pin routing, repeated START, errors, and owned-handle cleanup.
TEST(IdfI2c, TransactionsAndLifecycle) {
  // Attachments live for the process lifetime.
  static RecordingDevice peripheral;
  FakeEsp32().attachI2cDevice(peripheral, 18, 19);
  i2c_master_bus_config_t config{};
  config.i2c_port = 0;
  config.sda_io_num = GPIO_NUM_18;
  config.scl_io_num = GPIO_NUM_19;
  config.clk_source = I2C_CLK_SRC_DEFAULT;
  i2c_master_bus_handle_t bus = nullptr;
  ASSERT_EQ(ESP_OK, i2c_new_master_bus(&config, &bus));
  i2c_master_bus_handle_t resolved = nullptr;
  ASSERT_EQ(ESP_OK, i2c_master_get_bus_handle(0, &resolved));
  EXPECT_EQ(bus, resolved);
  EXPECT_EQ(ESP_ERR_NOT_FOUND, i2c_new_master_bus(&config, &resolved));
  EXPECT_EQ(ESP_OK, i2c_master_probe(bus, 0x68, 10));
  EXPECT_EQ(ESP_ERR_NOT_FOUND, i2c_master_probe(bus, 0x69, 10));

  i2c_device_config_t device_config{};
  device_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  device_config.device_address = 0x68;
  device_config.scl_speed_hz = 100000;
  i2c_master_dev_handle_t device = nullptr;
  ASSERT_EQ(ESP_OK, i2c_master_bus_add_device(bus, &device_config, &device));
  EXPECT_EQ(ESP_ERR_INVALID_STATE, i2c_del_master_bus(bus));
  const uint8_t tx[] = {0, 1};
  uint8_t rx[3]{};
  ASSERT_EQ(ESP_OK, i2c_master_transmit_receive(device, tx, 2, rx, 3, 25));
  EXPECT_EQ((std::vector<uint8_t>{0, 1}), peripheral.sent);
  EXPECT_FALSE(peripheral.write_stop);
  EXPECT_TRUE(peripheral.read_stop);
  EXPECT_EQ(25, peripheral.timeout_ms);
  EXPECT_EQ(0x5a, rx[2]);
  ASSERT_EQ(ESP_OK, i2c_master_transmit(device, tx, 2, 10));
  EXPECT_TRUE(peripheral.write_stop);
  ASSERT_EQ(ESP_OK, i2c_master_receive(device, rx, 3, 10));

  peripheral.result = FakeI2cDevice::I2C_ERROR_ACK;
  const int reads_before_failure = peripheral.reads;
  EXPECT_EQ(ESP_ERR_INVALID_RESPONSE,
            i2c_master_transmit_receive(device, tx, 2, rx, 3, 25));
  EXPECT_EQ(reads_before_failure, peripheral.reads);
  peripheral.result = FakeI2cDevice::I2C_ERROR_TIMEOUT;
  EXPECT_EQ(ESP_ERR_TIMEOUT, i2c_master_receive(device, rx, 3, 10));
  peripheral.result = FakeI2cDevice::I2C_ERROR_OK;
  EXPECT_EQ(ESP_OK, i2c_master_receive(device, rx, 3, 10));
  EXPECT_EQ(ESP_ERR_INVALID_ARG, i2c_master_transmit(device, nullptr, 1, 10));

  device_config.device_address = 0x69;
  i2c_master_dev_handle_t missing = nullptr;
  ASSERT_EQ(ESP_OK, i2c_master_bus_add_device(bus, &device_config, &missing));
  EXPECT_EQ(ESP_ERR_INVALID_RESPONSE, i2c_master_transmit(missing, tx, 2, 10));
  EXPECT_EQ(ESP_OK, i2c_master_bus_rm_device(missing));
  EXPECT_EQ(ESP_OK, i2c_master_bus_rm_device(device));
  EXPECT_EQ(ESP_OK, i2c_del_master_bus(bus));
  EXPECT_EQ(ESP_ERR_NOT_FOUND, i2c_master_get_bus_handle(0, &resolved));
  EXPECT_EQ(nullptr, resolved);

  // Selecting other pins must not resolve the previously attached device.
  config.sda_io_num = GPIO_NUM_21;
  config.scl_io_num = GPIO_NUM_22;
  ASSERT_EQ(ESP_OK, i2c_new_master_bus(&config, &bus));
  EXPECT_EQ(ESP_ERR_NOT_FOUND, i2c_master_probe(bus, 0x68, 10));
  EXPECT_EQ(ESP_OK, i2c_del_master_bus(bus));
}

// Verifies automatic port allocation, invalid configuration, and port reuse.
TEST(IdfI2c, AllocationAndArgumentValidation) {
  i2c_master_bus_config_t config{};
  config.i2c_port = -1;
  config.sda_io_num = GPIO_NUM_18;
  config.scl_io_num = GPIO_NUM_19;
  config.clk_source = I2C_CLK_SRC_DEFAULT;
  i2c_master_bus_handle_t first = nullptr, second = nullptr, extra = nullptr;
  EXPECT_EQ(ESP_ERR_INVALID_ARG, i2c_new_master_bus(nullptr, &first));
  ASSERT_EQ(ESP_OK, i2c_new_master_bus(&config, &first));
  config.sda_io_num = GPIO_NUM_21;
  config.scl_io_num = GPIO_NUM_22;
  ASSERT_EQ(ESP_OK, i2c_new_master_bus(&config, &second));
  EXPECT_NE(first, second);
  EXPECT_EQ(ESP_ERR_NOT_FOUND, i2c_new_master_bus(&config, &extra));
  EXPECT_EQ(ESP_ERR_INVALID_ARG, i2c_master_get_bus_handle(2, &extra));
  i2c_device_config_t device_config{};
  device_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  device_config.device_address = 0x80;
  device_config.scl_speed_hz = 100000;
  i2c_master_dev_handle_t device = nullptr;
  EXPECT_EQ(ESP_ERR_INVALID_ARG,
            i2c_master_bus_add_device(first, &device_config, &device));
  EXPECT_EQ(ESP_OK, i2c_del_master_bus(first));
  EXPECT_EQ(ESP_OK, i2c_del_master_bus(second));
  config.i2c_port = 0;
  config.scl_io_num = config.sda_io_num;
  EXPECT_EQ(ESP_ERR_INVALID_ARG, i2c_new_master_bus(&config, &extra));
}
}  // namespace
