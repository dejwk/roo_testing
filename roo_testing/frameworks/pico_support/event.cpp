#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>

#include "glog/logging.h"
#include "hardware/sync.h"
#include "pico/time.h"
#include "roo_testing/frameworks/pico_support/hardware_alarm.h"
#include "roo_testing/system/timer.h"

namespace {

using roo_testing::pico::kNoHardwareAlarm;

constexpr uint64_t kMaxClockMicros = INT64_MAX / 1000;
constexpr uint64_t kMaxHostWaitMicros = 24 * 60 * 60 * uint64_t{1000000};

// ARM WFE consumes one latched event. Multiple SEVs before a wait coalesce.
struct EventLatch {
  std::mutex mutex;
  std::condition_variable changed;
  bool pending = false;
};

/// Returns the core's processor event latch even during global initialization.
EventLatch& CoreEvent() {
  static EventLatch event;
  return event;
}

/// Tests and consumes the event under the same lock used to signal it.
bool ConsumeEvent() {
  EventLatch& event = CoreEvent();
  std::lock_guard<std::mutex> lock(event.mutex);
  const bool was_pending = event.pending;
  event.pending = false;
  return was_pending;
}

/// Blocks until a host signal or the next emulated deadline.
void WaitOnHost(uint64_t next, uint64_t now) {
  EventLatch& event = CoreEvent();
  std::unique_lock<std::mutex> lock(event.mutex);
  if (next == kNoHardwareAlarm) {
    event.changed.wait(lock, [&event] { return event.pending; });
  } else if (next > now) {
    // Keep the host's absolute timeout representable for far-future deadlines.
    const std::chrono::microseconds delay(
        std::min(next - now, kMaxHostWaitMicros));
    event.changed.wait_for(lock, delay, [&event] { return event.pending; });
  }
}

/// Waits for an event or deadline while dispatching IRQs on the Pico core.
bool WaitForEvent(uint64_t deadline) {
  for (;;) {
    roo_testing::pico::PollTimerInterrupts();
    const uint64_t now = time_us_64();
    if (now >= deadline) return true;
    if (ConsumeEvent()) return false;

    const uint64_t next =
        std::min(deadline, roo_testing::pico::NextHardwareAlarm());
    if (system_time_is_auto_sync_enabled()) {
      WaitOnHost(next, now);
      continue;
    }

    // In manual time, visit each shared device deadline in case it signals
    // SEV. Advancing straight to the timeout would hide that early wakeup.
    const int64_t shared = GetNextSystemTimeAlarmDeadline();
    const uint64_t step =
        shared == INT64_MAX
            ? next
            : std::min(next,
                       static_cast<uint64_t>(std::max<int64_t>(0, shared)));
    if (step == kNoHardwareAlarm) {
      WaitOnHost(step, now);
    } else if (step > now) {
      system_time_delay_micros(step - now);
    } else {
      ProcessSystemTimeAlarms();
    }
  }
}

}  // namespace

extern "C" {

void __sev() {
  EventLatch& event = CoreEvent();
  {
    std::lock_guard<std::mutex> lock(event.mutex);
    event.pending = true;
  }
  event.changed.notify_one();
}

void __wfe() {
  CHECK_EQ(__get_current_exception(), 0u) << "Cannot wait in a Pico interrupt";
  WaitForEvent(kNoHardwareAlarm);
}

bool best_effort_wfe_or_timeout(absolute_time_t target) {
  if (__get_current_exception() != 0) return time_reached(target);
  if (is_at_the_end_of_time(target)) return WaitForEvent(kNoHardwareAlarm);
  const uint64_t deadline = to_us_since_boot(target);
  CHECK_LE(deadline, kMaxClockMicros)
      << "Pico deadline exceeds the emulated clock range";
  return WaitForEvent(deadline);
}

}  // extern "C"
