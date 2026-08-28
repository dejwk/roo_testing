#include "roo_testing/host/scheduler_safe_host_lock.h"

#include <pthread.h>
#include <signal.h>

#include "glog/logging.h"

namespace roo_testing {

SchedulerSafeHostLock::SchedulerSafeHostLock(std::mutex& mutex)
    : mutex_(mutex) {
  sigset_t all_maskable_signals;
  CHECK_EQ(sigfillset(&all_maskable_signals), 0);
  CHECK_EQ(pthread_sigmask(SIG_BLOCK, &all_maskable_signals, &previous_mask_),
           0);
  mutex_.lock();
}

SchedulerSafeHostLock::~SchedulerSafeHostLock() {
  mutex_.unlock();
  CHECK_EQ(pthread_sigmask(SIG_SETMASK, &previous_mask_, nullptr), 0);
}

}  // namespace roo_testing
