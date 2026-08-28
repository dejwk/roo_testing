#pragma once

#include <signal.h>

#include <mutex>

namespace roo_testing {

/// Holds a host mutex while preventing FreeRTOS signal-driven task handoff.
///
/// This guard is for short roo_testing host-state critical sections shared by
/// native pthreads and FreeRTOS task pthreads. It does not replace FreeRTOS or
/// framework synchronization primitives.
class SchedulerSafeHostLock {
 public:
  /// Blocks maskable signals, then acquires `mutex`.
  explicit SchedulerSafeHostLock(std::mutex& mutex);

  /// Releases the mutex before restoring the caller's signal mask.
  ~SchedulerSafeHostLock();

  SchedulerSafeHostLock(const SchedulerSafeHostLock&) = delete;
  SchedulerSafeHostLock& operator=(const SchedulerSafeHostLock&) = delete;

 private:
  std::mutex& mutex_;
  sigset_t previous_mask_;
};

}  // namespace roo_testing
