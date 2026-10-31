#include "roo_testing/frameworks/posix_time/host_time.h"

#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

namespace roo_testing::system {
bool GetHostUnixTimeMicros(int64_t& unix_us) {
  struct timespec now;
  // The kernel read remains independent of whether C time is interposed.
  if (syscall(SYS_clock_gettime, CLOCK_REALTIME, &now) != 0) return false;
  const __int128 value =
      static_cast<__int128>(now.tv_sec) * 1000000 + now.tv_nsec / 1000;
  if (value < INT64_MIN || value > INT64_MAX) return false;
  unix_us = static_cast<int64_t>(value);
  return true;
}
}  // namespace roo_testing::system
