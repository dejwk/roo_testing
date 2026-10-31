#include <gtest/gtest.h>

#include "SntpClock.ino"

// Verifies the actual teaching sketch traverses cold boot and asynchronous
// sync.
TEST(SntpClock, BoundedSketch) {
  testing::internal::CaptureStdout();
  setup();
  loop();
  const std::string output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find("Before sync (seconds since epoch):"),
            std::string::npos);
  EXPECT_NE(output.find("Synchronized UTC:"), std::string::npos);
  EXPECT_EQ(output.find("timed out"), std::string::npos);
}
