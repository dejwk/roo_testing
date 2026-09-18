#include <gtest/gtest.h>

#include "roo_testing/devices/clock/ds3231/ds3231.h"

namespace {

uint8_t DecToBcd(int value) {
  return static_cast<uint8_t>(((value / 10) << 4) | (value % 10));
}

int BcdToDec(uint8_t value) { return ((value >> 4) * 10) + (value & 0x0F); }

}  // namespace

// Verifies time registers survive a complete RTC write/read transaction.
TEST(RtcDs3231I2cExampleTest, StoresAndReadsTime) {
  FakeDs3231 rtc;

  uint8_t set_time[] = {
      0x00,        DecToBcd(40), DecToBcd(55), DecToBcd(13),
      DecToBcd(1), DecToBcd(21), DecToBcd(2),  DecToBcd(22),
  };
  ASSERT_EQ(rtc.write(set_time, sizeof(set_time), true, 1000),
            FakeI2cDevice::I2C_ERROR_OK);

  uint8_t reg = 0x00;
  ASSERT_EQ(rtc.write(&reg, 1, true, 1000), FakeI2cDevice::I2C_ERROR_OK);

  uint8_t read_buf[7] = {};
  ASSERT_EQ(rtc.read(read_buf, sizeof(read_buf), true, 1000),
            FakeI2cDevice::I2C_ERROR_OK);

  int sec = BcdToDec(read_buf[0]);
  EXPECT_GE(sec, 40);
  EXPECT_LT(sec, 60);
  EXPECT_EQ(BcdToDec(read_buf[1]), 55);
  EXPECT_EQ(BcdToDec(read_buf[2]), 13);
  EXPECT_EQ(BcdToDec(read_buf[4]), 21);
  EXPECT_EQ(BcdToDec(read_buf[5] & 0x7F), 2);
  EXPECT_EQ(BcdToDec(read_buf[6]), 22);
}

// Verifies nonzero register selection and sequential read pointer advancement.
TEST(RtcDs3231I2cExampleTest, ReadsSelectedRegisters) {
  FakeDs3231 rtc;
  const uint8_t alarm[] = {0x07, 0x12, 0x34};
  ASSERT_EQ(FakeI2cDevice::I2C_ERROR_OK, rtc.write(alarm, 3, true, 100));
  const uint8_t reg = 0x07;
  ASSERT_EQ(FakeI2cDevice::I2C_ERROR_OK, rtc.write(&reg, 1, false, 100));
  uint8_t value = 0;
  ASSERT_EQ(FakeI2cDevice::I2C_ERROR_OK, rtc.read(&value, 1, true, 100));
  EXPECT_EQ(0x12, value);
  ASSERT_EQ(FakeI2cDevice::I2C_ERROR_OK, rtc.read(&value, 1, true, 100));
  EXPECT_EQ(0x34, value);
}

// Verifies both temperature bytes fit the model and register addressing wraps.
TEST(RtcDs3231I2cExampleTest, TemperatureAndAddressWrap) {
  FakeDs3231 rtc;
  const uint8_t reg = 0x11;
  ASSERT_EQ(FakeI2cDevice::I2C_ERROR_OK, rtc.write(&reg, 1, false, 100));
  uint8_t temperature[2]{};
  ASSERT_EQ(FakeI2cDevice::I2C_ERROR_OK, rtc.read(temperature, 2, true, 100));
  EXPECT_EQ(25, temperature[0]);
  EXPECT_EQ(0, temperature[1]);
  uint8_t seconds = 0;
  ASSERT_EQ(FakeI2cDevice::I2C_ERROR_OK, rtc.read(&seconds, 1, true, 100));
  EXPECT_LE(BcdToDec(seconds), 59);
}
