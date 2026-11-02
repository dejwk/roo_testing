#include <chrono>

#include "gtest/gtest.h"
#include "roo_testing/sys/condition_variable.h"
#include "roo_testing/sys/thread.h"

// Verifies the legacy Roo emulation adapter uses the selected kernel, retains
// native stack sizes above 64 KiB, and joins a thread after a CV handoff.
TEST(PicoFreeRtosSys, ThreadAndConditionVariable) {
  roo_testing::thread::attributes attributes;
  EXPECT_GE(attributes.stack_size(), 65536u);
  attributes.set_stack_size(128 * 1024);
  EXPECT_EQ(attributes.stack_size(), 128 * 1024u);
  roo_testing::mutex mutex;
  roo_testing::condition_variable cv;
  bool ready = false;
  roo_testing::unique_lock<roo_testing::mutex> lock(mutex);
  roo_testing::thread worker(attributes, [&] {
    roo_testing::unique_lock<roo_testing::mutex> child_lock(mutex);
    ready = true;
    cv.notify_one();
  });
  cv.wait(lock, [&ready] { return ready; });
  EXPECT_TRUE(ready);
  lock.unlock();
  worker.join();
}
