#include <gtest/gtest.h>

#include "roo_testing/system/timer.h"

namespace {

const bool kModeFromStaticInitialization = system_time_is_auto_sync_enabled();

}  // namespace

// Verifies manual mode is selected before test initialization.
TEST(TimeModeManualStaticInitializationTest, UsesManualTimeBeforeMain) {
  EXPECT_FALSE(kModeFromStaticInitialization);
  EXPECT_FALSE(system_time_is_auto_sync_enabled());
}
