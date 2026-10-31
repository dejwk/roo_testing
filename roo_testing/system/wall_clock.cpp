#include "roo_testing/system/wall_clock.h"

#include <mutex>

#include "roo_testing/host/scheduler_safe_host_lock.h"
#include "roo_testing/system/timer.h"

namespace roo_testing::system {
namespace {
std::mutex mutex;
int64_t reference_unix_us = 0;
int64_t reference_uptime_us = 0;
}  // namespace

bool SetUnixTimeMicros(int64_t unix_us) {
  SchedulerSafeHostLock lock(mutex);
  reference_unix_us = unix_us;
  reference_uptime_us = system_time_get_micros();
  return true;
}

bool GetUnixTimeMicros(int64_t& unix_us) {
  SchedulerSafeHostLock lock(mutex);
  const int64_t now = system_time_get_micros();
  int64_t elapsed;
  int64_t result;
  if (__builtin_sub_overflow(now, reference_uptime_us, &elapsed) ||
      __builtin_add_overflow(reference_unix_us, elapsed, &result))
    return false;
  unix_us = result;
  return true;
}

void ResetToPowerOnMapping() {
  SchedulerSafeHostLock lock(mutex);
  reference_unix_us = 0;
  reference_uptime_us = 0;
}
}  // namespace roo_testing::system
