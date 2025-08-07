#pragma once

#include <chrono>
#include <functional>

class EmulatedTime {
 public:
  EmulatedTime()
      : rt_clock_(),
        rt_start_time_(rt_clock_.now()),
        emu_uptime_(0) {}

 public:
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
};

EmulatedTime& SystemTimer();
