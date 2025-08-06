#include "fake_esp32_time.h"

#include <limits.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <random>
#include <set>

#include "roo_testing/sys/thread.h"

#include "glog/logging.h"

void EmulatedTime::sync() const {
  auto rt = rt_clock_.now() - rt_start_time_;
  if (rt > emu_uptime_ && rt - emu_uptime_ > kMaxTimeLag) {
    emu_uptime_ = rt;
  } else if (rt < emu_uptime_ && emu_uptime_ - rt > kMaxTimeAhead) {
    roo_testing::this_thread::sleep_for(emu_uptime_ - rt - kMaxTimeAhead);
    // Adjust the emulated time in case we overslept.
    rt = rt_clock_.now() - rt_start_time_;
    if (rt > emu_uptime_) {
      emu_uptime_ = rt;
    }
  }
}

void EmulatedTime::lag(std::chrono::nanoseconds lag) { emu_uptime_ += lag; }

int64_t EmulatedTime::getTimeMicros() const {
  sync();
  return std::chrono::duration_cast<std::chrono::microseconds>(emu_uptime_)
      .count();
}

void EmulatedTime::delayMicros(uint64_t delay) {
  lag(std::chrono::microseconds(delay));
  sync();
}
