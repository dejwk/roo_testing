#include "roo_testing/host/scheduler_safe_host_lock.h"

#include <gtest/gtest.h>
#include <pthread.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <mutex>
#include <thread>

namespace {

volatile sig_atomic_t signal_count = 0;

void CountSignal(int) { ++signal_count; }

bool IsBlocked(int signal) {
  sigset_t mask;
  EXPECT_EQ(pthread_sigmask(SIG_SETMASK, nullptr, &mask), 0);
  return sigismember(&mask, signal) == 1;
}

void WaitFor(const std::atomic<bool>& value) {
  for (int i = 0; i < 1000 && !value.load(std::memory_order_acquire); ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

}  // namespace

// Verifies the guard blocks signals before waiting for the host mutex.
TEST(SchedulerSafeHostLockTest, BlocksSignalsBeforeAcquiringMutex) {
  struct sigaction action = {};
  action.sa_handler = CountSignal;
  ASSERT_EQ(sigemptyset(&action.sa_mask), 0);
  struct sigaction previous_action = {};
  ASSERT_EQ(sigaction(SIGUSR1, &action, &previous_action), 0);

  std::mutex mutex;
  mutex.lock();
  std::atomic<bool> attempting_lock = false;
  std::atomic<bool> holds_lock = false;
  std::atomic<bool> release_lock = false;
  signal_count = 0;

  std::thread worker([&] {
    attempting_lock.store(true, std::memory_order_release);
    {
      roo_testing::SchedulerSafeHostLock lock(mutex);
      holds_lock.store(true, std::memory_order_release);
      while (!release_lock.load(std::memory_order_acquire)) {
        std::this_thread::yield();
      }
    }
  });

  WaitFor(attempting_lock);
  ASSERT_TRUE(attempting_lock.load(std::memory_order_acquire));
  ASSERT_EQ(pthread_kill(worker.native_handle(), SIGUSR1), 0);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_EQ(signal_count, 0);

  mutex.unlock();
  WaitFor(holds_lock);
  ASSERT_TRUE(holds_lock.load(std::memory_order_acquire));
  EXPECT_EQ(signal_count, 0);

  release_lock.store(true, std::memory_order_release);
  worker.join();
  EXPECT_EQ(signal_count, 1);
  EXPECT_EQ(sigaction(SIGUSR1, &previous_action, nullptr), 0);
}

// Verifies the guard restores the caller's exact signal mask after unlocking.
TEST(SchedulerSafeHostLockTest, RestoresCallerSignalMask) {
  sigset_t original_mask;
  ASSERT_EQ(pthread_sigmask(SIG_SETMASK, nullptr, &original_mask), 0);
  sigset_t requested_mask = original_mask;
  ASSERT_EQ(sigaddset(&requested_mask, SIGUSR1), 0);
  ASSERT_EQ(pthread_sigmask(SIG_SETMASK, &requested_mask, nullptr), 0);

  std::mutex mutex;
  {
    roo_testing::SchedulerSafeHostLock lock(mutex);
    EXPECT_TRUE(IsBlocked(SIGUSR1));
    EXPECT_TRUE(IsBlocked(SIGALRM));
  }

  EXPECT_TRUE(IsBlocked(SIGUSR1));
  EXPECT_EQ(IsBlocked(SIGALRM), sigismember(&original_mask, SIGALRM) == 1);
  EXPECT_EQ(pthread_sigmask(SIG_SETMASK, &original_mask, nullptr), 0);
}
