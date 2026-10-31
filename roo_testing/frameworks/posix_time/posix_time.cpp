
#include <errno.h>
#include <sys/time.h>
#include <time.h>

#include <limits>

#include "roo_testing/system/wall_clock.h"

extern "C" int __real_clock_gettime(clockid_t, struct timespec*);
extern "C" int __real_clock_getres(clockid_t, struct timespec*);

namespace {
/// Converts normalized seconds and a fractional microsecond count with checks.
bool ToMicros(time_t seconds, int64_t fraction, int64_t& result) {
  const __int128 value = static_cast<__int128>(seconds) * 1000000 + fraction;
  if (value < INT64_MIN || value > INT64_MAX) return false;
  result = static_cast<int64_t>(value);
  return true;
}

/// Splits negative epochs using floor, keeping a nonnegative fraction.
bool SplitMicros(int64_t value, time_t& seconds, long& micros) {
  int64_t whole = value / 1000000;
  micros = value % 1000000;
  if (micros < 0) {
    --whole;
    micros += 1000000;
  }
  if (whole < std::numeric_limits<time_t>::min() ||
      whole > std::numeric_limits<time_t>::max())
    return false;
  seconds = static_cast<time_t>(whole);
  return true;
}

int Overflow() {
  errno = EOVERFLOW;
  return -1;
}
int Invalid() {
  errno = EINVAL;
  return -1;
}
int Unsupported() {
  errno = ENOTSUP;
  return -1;
}
}  // namespace

extern "C" int __wrap_gettimeofday(struct timeval* tv, void*) {
  if (tv == nullptr) return 0;
  int64_t value;
  time_t seconds;
  long micros;
  if (!roo_testing::system::GetUnixTimeMicros(value) ||
      !SplitMicros(value, seconds, micros))
    return Overflow();
  tv->tv_sec = seconds;
  tv->tv_usec = micros;
  return 0;
}

extern "C" int __wrap_settimeofday(const struct timeval* tv,
                                   const struct timezone*) {
  if (tv == nullptr) return 0;
  if (tv->tv_usec < 0 || tv->tv_usec >= 1000000) return Invalid();
  int64_t value;
  if (!ToMicros(tv->tv_sec, tv->tv_usec, value) ||
      !roo_testing::system::SetUnixTimeMicros(value))
    return Overflow();
  return 0;
}

extern "C" time_t __wrap_time(time_t* out) {
  struct timeval now;
  if (__wrap_gettimeofday(&now, nullptr) != 0) return static_cast<time_t>(-1);
  if (out != nullptr) *out = now.tv_sec;
  return now.tv_sec;
}

extern "C" int __wrap_clock_gettime(clockid_t id, struct timespec* ts) {
  if (id != CLOCK_REALTIME) return __real_clock_gettime(id, ts);
  if (ts == nullptr) return Invalid();
  struct timeval now;
  if (__wrap_gettimeofday(&now, nullptr) != 0) return -1;
  ts->tv_sec = now.tv_sec;
  ts->tv_nsec = now.tv_usec * 1000;
  return 0;
}

extern "C" int __wrap_clock_getres(clockid_t id, struct timespec* ts) {
  if (id != CLOCK_REALTIME) return __real_clock_getres(id, ts);
  if (ts != nullptr) {
    ts->tv_sec = 0;
    ts->tv_nsec = 1000;
  }
  return 0;
}

extern "C" int __wrap_clock_settime(clockid_t id, const struct timespec* ts) {
  if (id != CLOCK_REALTIME) return Unsupported();
  if (ts == nullptr || ts->tv_nsec < 0 || ts->tv_nsec >= 1000000000)
    return Invalid();
  int64_t value;
  if (!ToMicros(ts->tv_sec, ts->tv_nsec / 1000, value) ||
      !roo_testing::system::SetUnixTimeMicros(value))
    return Overflow();
  return 0;
}

extern "C" int __wrap_adjtime(const struct timeval*, struct timeval*) {
  return Unsupported();
}
