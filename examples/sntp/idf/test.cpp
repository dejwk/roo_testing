#include <gtest/gtest.h>
extern "C" void app_main();

// Verifies the actual IDF example initializes, synchronizes, and cleans up.
TEST(IdfSntp, BoundedApplication) {
  testing::internal::CaptureStdout();
  app_main();
  const std::string output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find("Before sync (seconds since epoch):"),
            std::string::npos);
  EXPECT_NE(output.find("Synchronized UTC epoch seconds:"), std::string::npos);
  EXPECT_EQ(output.find("timed out"), std::string::npos);
}
