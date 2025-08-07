#include "fake_esp32_time.h"

#include <thread>

void EmulatedTime::sync() const {
  auto rt = rt_clock_.now() - rt_start_time_;
  if (rt > emu_uptime_ && rt - emu_uptime_ > kMaxTimeLag) {
    emu_uptime_ = rt;
  } else if (rt < emu_uptime_ && emu_uptime_ - rt > kMaxTimeAhead) {
    // Note: using standard sleep, not FreeRTOS sleep, to pause all FreeRTOS
    // threads. Otherwise, FreeRTOS scheduler would try to schedule another
    // FreeRTOS thread to run.
    std::this_thread::sleep_for(emu_uptime_ - rt - kMaxTimeAhead);
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
