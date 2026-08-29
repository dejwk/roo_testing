#include <gtest/gtest.h>

#include "esp32-hal-ledc.h"

namespace {

bool callback_invoked = false;

void FadeCallback() { callback_invoked = true; }

TEST(ArduinoLedcTest, UnsupportedFadesDoNotChangeOutputOrInvokeCallbacks) {
  ASSERT_TRUE(ledcAttachChannel(23, 1000, 8, 0));
  ASSERT_TRUE(ledcWrite(23, 37));

  EXPECT_FALSE(ledcFade(23, 1, 100, 10));
  EXPECT_FALSE(ledcFadeWithInterrupt(23, 1, 100, 10, FadeCallback));
  EXPECT_FALSE(ledcFadeWithInterruptArg(
      23, 1, 100, 10, [](void* value) { *static_cast<bool*>(value) = true; },
      &callback_invoked));
  EXPECT_EQ(37U, ledcRead(23));
  EXPECT_FALSE(callback_invoked);
}

}  // namespace
