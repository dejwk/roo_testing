#include "Wire.h"

#include "gtest/gtest.h"

TEST(WireMasterTest, StartsAndStopsWithUnsupportedSlavePathLinked) {
  TwoWire wire(0);
  ASSERT_TRUE(wire.begin(21, 22, 100000));
  EXPECT_EQ(wire.getClock(), 100000U);
  EXPECT_TRUE(wire.end());
}
