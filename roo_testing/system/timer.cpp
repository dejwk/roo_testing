#include "timer.h"

#include <chrono>
#include <functional>
#include <thread>

static constexpr auto kMaxTimeAhead = std::chrono::nanoseconds(100000);
static constexpr auto kMaxTimeLag = std::chrono::nanoseconds(50);

class EmulatedTime {
 public:
  EmulatedTime()
      : rt_clock_(), rt_start_time_(rt_clock_.now()), emu_uptime_(0) {
    set_auto_sync(true);
  }

 public:
  void set_auto_sync(bool auto_sync) {
    auto_sync_ = auto_sync;
    if (auto_sync_) {
      // Reset the offset so that it agrees with the current value of the
      // emulated time.
      rt_start_time_ = (rt_clock_.now() - emu_uptime_);
    }
  }

  // Keep the emulated time within [-kMaxTimeAheadNanos, +kMaxTimeLagNanos] of
  // the real time. If the emulator is ahead (emulated time > real time), sleep
  // to make up the difference. If the emulator is behind (emulated_time <
  // real_time), adjust the emulated time.
  void sync() const;

  // Adds the given lag to the emulated time.
  // Does not call sync(), to avoid the risk that the emulated time gets lagged
  // by more than expected.
  void lag(std::chrono::nanoseconds lag);

  int64_t getTimeMicros() const;

  void delayMicros(uint64_t delay);

 private:
  std::chrono::high_resolution_clock rt_clock_;
  std::chrono::high_resolution_clock::time_point rt_start_time_;
  mutable std::chrono::high_resolution_clock::duration emu_uptime_;
  bool auto_sync_;
};

EmulatedTime& SystemTimer() {
  static EmulatedTime timer;
  return timer;
}

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
  if (auto_sync_) sync();
  return std::chrono::duration_cast<std::chrono::microseconds>(emu_uptime_)
      .count();
}

void EmulatedTime::delayMicros(uint64_t delay) {
  lag(std::chrono::microseconds(delay));
  if (auto_sync_) sync();
}

extern "C" {

void system_time_sync() { SystemTimer().sync(); }

void system_time_lag_ns(uint64_t ns) {
  SystemTimer().lag(std::chrono::nanoseconds(ns));
}

int64_t system_time_get_micros() { return SystemTimer().getTimeMicros(); }

void system_time_delay_micros(uint64_t us) { SystemTimer().delayMicros(us); }

void system_time_set_auto_sync(bool auto_sync) {
  SystemTimer().set_auto_sync(auto_sync);
}

}