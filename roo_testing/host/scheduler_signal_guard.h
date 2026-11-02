#pragma once
#if defined(ROO_TESTING_SOC_RP2040) && defined(ROO_TESTING_RUNTIME_FREERTOS)
#include <pthread.h>
#include <signal.h>

#include <cstdlib>
#endif

namespace roo_testing {
/// Keeps a short single-core Pico state operation indivisible across task
/// ticks. This guard has no host-thread exclusion. Wiring is configured before
/// startup, and callbacks inside a guarded operation must be nonblocking.
class SchedulerSignalGuard {
 public:
  /// Masks scheduler signals until the short operation finishes.
  SchedulerSignalGuard() {
#if defined(ROO_TESTING_SOC_RP2040) && defined(ROO_TESTING_RUNTIME_FREERTOS)
    sigset_t signals;
    if (sigfillset(&signals) != 0 ||
        pthread_sigmask(SIG_BLOCK, &signals, &previous_) != 0)
      std::abort();
#endif
  }

  /// Restores the signal mask after all state and callbacks are consistent.
  ~SchedulerSignalGuard() {
#if defined(ROO_TESTING_SOC_RP2040) && defined(ROO_TESTING_RUNTIME_FREERTOS)
    if (pthread_sigmask(SIG_SETMASK, &previous_, nullptr) != 0) std::abort();
#endif
  }

  /// A signal mask has one lexical owner.
  SchedulerSignalGuard(const SchedulerSignalGuard&) = delete;
  /// A signal mask cannot be assigned.
  SchedulerSignalGuard& operator=(const SchedulerSignalGuard&) = delete;

 private:
#if defined(ROO_TESTING_SOC_RP2040) && defined(ROO_TESTING_RUNTIME_FREERTOS)
  sigset_t previous_;
#endif
};
}  // namespace roo_testing
