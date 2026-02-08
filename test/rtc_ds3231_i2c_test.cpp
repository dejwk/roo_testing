#include <gtest/gtest.h>

#include "roo_testing/devices/clock/ds3231/ds3231.h"

namespace {

uint8_t DecToBcd(int value) {
  return static_cast<uint8_t>(((value / 10) << 4) | (value % 10));
}

int BcdToDec(uint8_t value) { return ((value >> 4) * 10) + (value & 0x0F); }

}  // namespace

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
