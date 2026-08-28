#include <gtest/gtest.h>

#include "roo_testing/system/timer.h"

namespace {

const bool kModeFromStaticInitialization = system_time_is_auto_sync_enabled();

}  // namespace

// Verifies the default auto-sync mode is selected before test initialization.
TEST(TimeModeAutoStaticInitializationTest, UsesAutoSyncBeforeMain) {
  EXPECT_TRUE(kModeFromStaticInitialization);
  EXPECT_TRUE(system_time_is_auto_sync_enabled());
}
