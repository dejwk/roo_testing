#include "Wire.h"

#include "gtest/gtest.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

namespace {

class RegisterDevice : public FakeI2cDevice {
 public:
  RegisterDevice() : FakeI2cDevice("wire-master-test", 0x42) {}

  Result write(const uint8_t *buffer, uint16_t size, bool, uint16_t) override {
    if (size != 1) return I2C_ERROR_DEV;
    value_ = buffer[0];
    return I2C_ERROR_OK;
  }

  Result read(uint8_t *buffer, uint16_t size, bool, uint16_t) override {
    if (size != 1) return I2C_ERROR_DEV;
    buffer[0] = value_;
    return I2C_ERROR_OK;
  }

 private:
  uint8_t value_ = 0;
};

}  // namespace

TEST(WireMasterTest, StartsAndStopsWithUnsupportedSlavePathLinked) {
  RegisterDevice device;
  FakeEsp32().attachI2cDevice(device, 21, 22);

  TwoWire wire(0);
  ASSERT_TRUE(wire.begin(21, 22, 100000));
  EXPECT_EQ(wire.getClock(), 100000U);

  wire.beginTransmission(0x42);
  ASSERT_EQ(wire.write(0xA5), 1U);
  ASSERT_EQ(wire.endTransmission(), 0);
  ASSERT_EQ(wire.requestFrom(0x42, 1), 1U);
  EXPECT_EQ(wire.read(), 0xA5);

  EXPECT_TRUE(wire.end());
}
