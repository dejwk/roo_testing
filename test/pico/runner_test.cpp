#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <memory>

#include "gtest/gtest.h"
#include "roo_testing/system/timer.h"

namespace {

std::weak_ptr<int> kPendingCapture;
std::atomic<bool> kCaptureDestroyed{false};
bool kShutdownCheckRegistered = false;

/// Records capture destruction so a post-main check can observe runner cleanup.
void DeleteCapture(int* value) {
  delete value;
  kCaptureDestroyed.store(true, std::memory_order_release);
}

/// Runs before static destruction, after the test runner has returned from
/// main.
void VerifyShutdownAfterMain() {
  if (!kPendingCapture.expired() ||
      !kCaptureDestroyed.load(std::memory_order_acquire) ||
      TryBeginSystemTimeServiceShutdownForHost()) {
    std::fputs("Pico runner failed to close/join the shared time service\n",
               stderr);
    std::_Exit(99);
  }
}

/// Leaves a far-future capture for the runner to discard after joining its
/// worker.
bool RegisterShutdownCheck() {
  if (kShutdownCheckRegistered) return true;
  std::shared_ptr<int> capture(new int(0), DeleteCapture);
  kPendingCapture = capture;
  ScheduleSystemTimeAlarm(INT64_MAX / 1000 - 1, [capture] {
    if (*capture == 0) std::abort();
  });
  if (std::atexit(VerifyShutdownAfterMain) != 0) return false;
  kShutdownCheckRegistered = true;
  return true;
}

// Verifies shutdown discards captures and joins the auto worker or manual
// no-worker path.
TEST(PicoRunnerTest, CleansPendingWorkAfterTests) {
  ASSERT_EQ(system_time_is_auto_sync_enabled(), PICO_TEST_AUTO != 0);
  ASSERT_TRUE(RegisterShutdownCheck());
  EXPECT_FALSE(kPendingCapture.expired());
}

// Verifies a failing test still shuts down cleanly and returns a nonzero
// result. This case is run separately by verify_pico.py with the exact expected
// exit code.
TEST(PicoRunnerTest, DISABLED_PropagatesFailure) {
  ASSERT_TRUE(RegisterShutdownCheck());
  ADD_FAILURE() << "Intentional Pico runner result propagation check";
}

}  // namespace
