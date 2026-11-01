#include "hardware/i2c.h"

#include <array>
#include <memory>
#include <vector>

#include "gtest/gtest.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

namespace {
/// Records transport details and can fail after writing a staged response.
class Device : public FakeI2cDevice {
 public:
  /// Creates a recorder for one address and controller.
  Device(uint8_t address, uint index)
      : FakeI2cDevice("record", address), index_(index) {}
  /// Captures transmitted bytes, STOP intent, and timeout.
  Result write(const uint8_t* data, uint16_t size, bool stop,
               uint16_t timeout) override {
    record(stop, timeout);
    bytes.assign(data, data + size);
    return result;
  }
  /// Writes a deterministic reply into the adapter staging buffer.
  Result read(uint8_t* data, uint16_t size, bool stop,
              uint16_t timeout) override {
    record(stop, timeout);
    for (uint i = 0; i < size; ++i) data[i] = i;
    return result;
  }
  /// Records the previous repeated-START intent during active delivery.
  void record(bool stop, uint16_t timeout) {
    EXPECT_TRUE(FakePico().i2cState(index_).busy);
    stops.push_back(stop);
    timeouts.push_back(timeout);
    restarts.push_back(FakePico().i2cState(index_).restart_on_next);
  }
  /// Clears observations without changing immutable fixture wiring.
  void clear() {
    stops.clear();
    timeouts.clear();
    restarts.clear();
    bytes.clear();
    result = I2C_ERROR_OK;
  }
  std::vector<bool> stops;
  std::vector<bool> restarts;
  std::vector<uint16_t> timeouts;
  std::vector<uint8_t> bytes;
  Result result = I2C_ERROR_OK;

 private:
  uint index_;
};

/// Keeps borrowed attachments alive and covers each legal SDA/SCL pair.
struct Wiring {
  Wiring() {
    for (uint pair = 0; pair < devices.size(); ++pair) {
      devices[pair] = std::make_unique<Device>(0x30 + pair, pair % 2);
      FakePico().attachI2cDevice(*devices[pair], pair * 2, pair * 2 + 1);
    }
  }
  std::array<std::unique_ptr<Device>, 15> devices;
};
Wiring& Wires() {
  static Wiring wiring;
  return wiring;
}

void Select(uint pair) {
  for (uint pin = 0; pin < 30; ++pin) gpio_set_function(pin, GPIO_FUNC_NULL);
  gpio_set_function(pair * 2, GPIO_FUNC_I2C);
  gpio_set_function(pair * 2 + 1, GPIO_FUNC_I2C);
}
class PicoI2cTest : public testing::Test {
 protected:
  void SetUp() override {
    for (std::unique_ptr<Device>& device : Wires().devices) device->clear();
    Select(0);
    i2c_init(i2c0, 100000);
    i2c_init(i2c1, 400000);
  }
};

// Verifies all physical SDA/SCL pairs select the correct controller/address.
TEST_F(PicoI2cTest, AllRoutesAndRepeatedStart) {
  for (uint pair = 0; pair < 15; ++pair) {
    Select(pair);
    i2c_inst_t* i2c = i2c_get_instance(pair % 2);
    Device& device = *Wires().devices[pair];
    const uint8_t reg = 7;
    uint8_t response[2];
    EXPECT_EQ(i2c_write_blocking(i2c, device.address(), &reg, 1, true), 1);
    EXPECT_TRUE(FakePico().i2cState(pair % 2).restart_on_next);
    EXPECT_EQ(i2c_read_blocking(i2c, device.address(), response, 2, false), 2);
    EXPECT_EQ(device.stops, std::vector<bool>({false, true}));
    EXPECT_EQ(device.restarts, std::vector<bool>({false, true}));
    EXPECT_FALSE(FakePico().i2cState(pair % 2).restart_on_next);
    EXPECT_EQ(response[1], 1);
  }
}

// Verifies timeout rounding/saturation, expired deadlines, error mapping, and
// read staging, without falsely modeling physical bus/clock-stretch timing.
TEST_F(PicoI2cTest, TimeoutsAndErrors) {
  Device& device = *Wires().devices[0];
  uint8_t response = 99;
  const uint64_t before = time_us_64();
  EXPECT_EQ(i2c_read_timeout_us(i2c0, 0x30, &response, 1, false, 1), 1);
  EXPECT_EQ(device.timeouts.back(), 1);
  EXPECT_EQ(i2c_read_timeout_us(i2c0, 0x30, &response, 1, false, 1001), 1);
  EXPECT_EQ(device.timeouts.back(), 2);
  EXPECT_EQ(i2c_read_timeout_us(i2c0, 0x30, &response, 1, false, UINT32_MAX),
            1);
  EXPECT_EQ(device.timeouts.back(), UINT16_MAX);
  const size_t calls = device.stops.size();
  EXPECT_EQ(i2c_read_timeout_us(i2c0, 0x30, &response, 1, false, 0),
            PICO_ERROR_TIMEOUT);
  EXPECT_EQ(i2c_read_blocking_until(i2c0, 0x30, &response, 1, false,
                                    get_absolute_time()),
            PICO_ERROR_TIMEOUT);
  EXPECT_EQ(device.stops.size(), calls);
  device.result = FakeI2cDevice::I2C_ERROR_ACK;
  response = 99;
  EXPECT_EQ(i2c_read_blocking(i2c0, 0x30, &response, 1, false),
            PICO_ERROR_GENERIC);
  EXPECT_EQ(response, 99);
  device.result = FakeI2cDevice::I2C_ERROR_TIMEOUT;
  EXPECT_EQ(i2c_read_blocking(i2c0, 0x30, &response, 1, false),
            PICO_ERROR_TIMEOUT);
  EXPECT_EQ(time_us_64(), before);
}

// Verifies oversize/invalid transfers, unconfigured/ambiguous muxes, invalid
// handles and unsupported slave operations cannot silently reach a device.
TEST_F(PicoI2cTest, InvalidRequestsAndRouting) {
  uint8_t byte = 99;
  EXPECT_EQ(i2c_read_blocking(i2c0, 0x30, &byte, 65536, false),
            PICO_ERROR_INVALID_ARG);
  EXPECT_EQ(i2c_read_blocking(i2c0, 0x30, &byte, 0, false),
            PICO_ERROR_INVALID_ARG);
  EXPECT_EQ(i2c_read_blocking(i2c0, 0x30, nullptr, 1, false),
            PICO_ERROR_INVALID_ARG);
  EXPECT_EQ(i2c_read_blocking(i2c0, 0x80, &byte, 1, false),
            PICO_ERROR_INVALID_ARG);
  EXPECT_EQ(i2c_read_blocking(i2c0, 0x70, &byte, 1, false), PICO_ERROR_GENERIC);
  gpio_set_function(4, GPIO_FUNC_I2C);
  EXPECT_EQ(i2c_read_blocking(i2c0, 0x30, &byte, 1, false),
            PICO_ERROR_INVALID_STATE);
  EXPECT_TRUE(Wires().devices[0]->stops.empty());
  EXPECT_EQ(byte, 99);
  EXPECT_DEATH(i2c_get_index(nullptr), "instance");
  EXPECT_DEATH(i2c_get_instance(2), "index");
  EXPECT_DEATH(i2c_set_slave_mode(i2c0, true, 0x30), "slave mode");
  EXPECT_DEATH(FakePico().padVoltage(0), "I2C pin voltage");
}
}  // namespace
