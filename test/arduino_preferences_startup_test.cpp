#include "Preferences.h"
#include "gtest/gtest.h"

TEST(ArduinoPreferencesStartupTest, DefaultPartitionIsReadyBeforeTestsRun) {
  Preferences preferences;

  ASSERT_TRUE(preferences.begin("startup_test"));
  EXPECT_EQ(preferences.putUInt("answer", 42), sizeof(uint32_t));
  EXPECT_EQ(preferences.getUInt("answer"), 42u);
  EXPECT_TRUE(preferences.clear());
  preferences.end();
}
