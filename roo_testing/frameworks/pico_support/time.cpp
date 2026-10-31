#include "pico/time.h"

#include <cstdint>
#include <cstdlib>
#include <limits>

#include "glog/logging.h"
#include "roo_testing/system/timer.h"

namespace {

constexpr uint64_t kMaxClockMicros =
    static_cast<uint64_t>(std::numeric_limits<int64_t>::max() / 1000);

/// Rejects deadlines the shared signed-nanosecond clock cannot represent.
uint64_t CheckedDeadline(absolute_time_t target) {
  const uint64_t target_us = to_us_since_boot(target);
  CHECK_LE(target_us, kMaxClockMicros)
      << "Pico deadline exceeds the emulated clock range";
  return target_us;
}

/// Fails unsupported hardware operations instead of reporting completion.
[[noreturn]] void UnsupportedAlarm(const char* operation) {
  LOG(FATAL) << "Unimplemented: Pico hardware alarm " << operation;
  std::abort();
}

}  // namespace

extern "C" {

const absolute_time_t ABSOLUTE_TIME_INITIALIZED_VAR(nil_time, 0);
const absolute_time_t ABSOLUTE_TIME_INITIALIZED_VAR(at_the_end_of_time,
                                                    INT64_MAX);

uint64_t time_us_64() {
  return static_cast<uint64_t>(system_time_get_micros());
}

uint32_t time_us_32() { return static_cast<uint32_t>(time_us_64()); }

bool time_reached(absolute_time_t target) {
  return time_us_64() >= to_us_since_boot(target);
}

void sleep_us(uint64_t us) {
  CHECK_LE(us, kMaxClockMicros)
      << "Pico duration exceeds the emulated clock range";
  if (us == 0) return;
  system_time_delay_micros(us);
}

void sleep_ms(uint32_t ms) { sleep_us(static_cast<uint64_t>(ms) * 1000); }

void sleep_until(absolute_time_t target) {
  const uint64_t target_us = CheckedDeadline(target);
  const uint64_t now_us = time_us_64();
  if (target_us > now_us) sleep_us(target_us - now_us);
}

void busy_wait_us(uint64_t us) { system_time_busy_wait_micros(us); }

void busy_wait_us_32(uint32_t us) { busy_wait_us(us); }

void busy_wait_ms(uint32_t ms) {
  busy_wait_us(static_cast<uint64_t>(ms) * 1000);
}

void busy_wait_until(absolute_time_t target) {
  const uint64_t target_us = CheckedDeadline(target);
  uint64_t now_us = time_us_64();
  if (target_us <= now_us) return;
  // In manual mode the test driver owns progression. Occupy only the sampled
  // host duration; re-reading manual uptime until target would never finish.
  if (!system_time_is_auto_sync_enabled()) {
    busy_wait_us(target_us - now_us);
    return;
  }
  // Explicit time advances can leave the published floor slightly ahead of
  // host time. Recheck auto uptime rather than returning before the target.
  do {
    busy_wait_us(target_us - now_us);
    now_us = time_us_64();
  } while (now_us < target_us);
}

bool best_effort_wfe_or_timeout(absolute_time_t target) {
  // No processor-event source is modeled. Waiting to the deadline is the
  // supported fallback; SDK alarms and interrupt callbacks remain unavailable.
  sleep_until(target);
  return time_reached(target);
}

void hardware_alarm_claim(uint) { UnsupportedAlarm("claim"); }

void hardware_alarm_unclaim(uint) { UnsupportedAlarm("unclaim"); }

int hardware_alarm_claim_unused(bool required) {
  if (required) UnsupportedAlarm("claim unused");
  return -1;
}

void hardware_alarm_set_callback(uint, hardware_alarm_callback_t) {
  UnsupportedAlarm("callback");
}

bool hardware_alarm_set_target(uint, absolute_time_t) {
  UnsupportedAlarm("target");
}

void hardware_alarm_cancel(uint) { UnsupportedAlarm("cancel"); }

void hardware_alarm_force_irq(uint) { UnsupportedAlarm("force IRQ"); }

}  // extern "C"
