#include <cstdint>

#include "gtest/gtest.h"
#include "roo_testing/buses/i2c/fake_i2c.h"

namespace {

TEST(LegacyI2cInterface, ProvidesTwoStableClassicEsp32Controllers) {
  FakeI2cInterface* i2c0 = getI2cInterface(0);
  FakeI2cInterface* i2c1 = getI2cInterface(1);

  ASSERT_NE(i2c0, nullptr);
  ASSERT_NE(i2c1, nullptr);
  EXPECT_NE(i2c0, i2c1);
  EXPECT_EQ(i2c0->name(), "i2c0");
  EXPECT_EQ(i2c1->name(), "i2c1");
  EXPECT_EQ(getI2cInterface(0), i2c0);
  EXPECT_EQ(getI2cInterface(1), i2c1);
  EXPECT_EQ(getI2cInterface(2), nullptr);
  EXPECT_EQ(getI2cInterface(UINT8_MAX), nullptr);
}

}  // namespace
