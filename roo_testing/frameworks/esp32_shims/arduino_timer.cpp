#include "esp32-hal-timer.h"

#include <algorithm>
#include <mutex>

#include "roo_testing/system/timer.h"

struct timer_struct_t {
  std::mutex mutex;
  uint32_t frequency;
  uint64_t value;
  int64_t started_at;
  bool running;
  void (*callback)(void *);
  void *callback_arg;
  bool callback_has_arg;
  uint64_t alarm;
  bool autoreload;
  uint64_t reload_count;
};

namespace {
uint64_t readLocked(hw_timer_t *timer) {
  uint64_t value = timer->value;
  if (timer->running) {
    const uint64_t elapsed_us = std::max<int64_t>(0, system_time_get_micros() - timer->started_at);
    value += elapsed_us * timer->frequency / 1000000ULL;
  }
  return value;
}
}  // namespace

extern "C" {

hw_timer_t *timerBegin(uint32_t frequency) {
  if (frequency == 0) return nullptr;
  return new timer_struct_t{{}, frequency, 0, system_time_get_micros(), true,
                            nullptr, nullptr, false, 0, false, 0};
}

void timerEnd(hw_timer_t *timer) { delete timer; }

void timerStart(hw_timer_t *timer) {
  if (!timer) return;
  std::lock_guard<std::mutex> lock(timer->mutex);
  if (!timer->running) {
    timer->started_at = system_time_get_micros();
    timer->running = true;
  }
}

void timerStop(hw_timer_t *timer) {
  if (!timer) return;
  std::lock_guard<std::mutex> lock(timer->mutex);
  if (timer->running) {
    timer->value = readLocked(timer);
    timer->running = false;
  }
}

void timerRestart(hw_timer_t *timer) { timerWrite(timer, 0); }

void timerWrite(hw_timer_t *timer, uint64_t value) {
  if (!timer) return;
  std::lock_guard<std::mutex> lock(timer->mutex);
  timer->value = value;
  timer->started_at = system_time_get_micros();
}

uint64_t timerRead(hw_timer_t *timer) {
  if (!timer) return 0;
  std::lock_guard<std::mutex> lock(timer->mutex);
  return readLocked(timer);
}

uint64_t timerReadMicros(hw_timer_t *timer) {
  return timer && timer->frequency ? timerRead(timer) * 1000000ULL / timer->frequency : 0;
}
uint64_t timerReadMillis(hw_timer_t *timer) { return timerReadMicros(timer) / 1000ULL; }
double timerReadSeconds(hw_timer_t *timer) { return timerReadMicros(timer) / 1000000.0; }
uint32_t timerGetFrequency(hw_timer_t *timer) { return timer ? timer->frequency : 0; }

void timerAttachInterrupt(hw_timer_t *timer, void (*callback)(void)) {
  if (!timer) return;
  timer->callback = reinterpret_cast<void (*)(void *)>(callback);
  timer->callback_arg = nullptr;
  timer->callback_has_arg = false;
}

void timerAttachInterruptArg(hw_timer_t *timer, void (*callback)(void *), void *arg) {
  if (!timer) return;
  timer->callback = callback;
  timer->callback_arg = arg;
  timer->callback_has_arg = true;
}

void timerDetachInterrupt(hw_timer_t *timer) {
  if (timer) timer->callback = nullptr;
}

void timerAlarm(hw_timer_t *timer, uint64_t alarm_value, bool autoreload,
                uint64_t reload_count) {
  if (!timer) return;
  timer->alarm = alarm_value;
  timer->autoreload = autoreload;
  timer->reload_count = reload_count;
}

}  // extern "C"
