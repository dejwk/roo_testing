#include <atomic>
#include <cstdint>
#include <limits>
#include <mutex>

#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "esp_timer.h"
#include "glog/logging.h"
#include "roo_testing/frameworks/esp_idf_support/esp_interrupts.h"
#include "roo_testing/system/timer.h"
#include "soc/interrupts.h"

extern "C" {
#include "components/esp_timer/private_include/esp_timer_impl.h"
}

#if defined(CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD)
#error "The host esp_timer backend supports TASK dispatch only."
#endif

namespace {

constexpr uint64_t kDisarmedCompare = std::numeric_limits<uint64_t>::max();

std::mutex compare_mutex;
SystemTimeAlarmId scheduled_alarm = 0;
uint64_t requested_compare = kDisarmedCompare;
std::atomic<uint64_t> compare_generation{0};
bool initialized = false;
intr_handle_t interrupt_handle = nullptr;
std::atomic<uint64_t> pending_generation{0};
std::atomic<bool> pending_status{false};
std::atomic<intr_handler_t> upper_handler{nullptr};

/// Publishes a compare expiry only when it still belongs to the current arm.
void DeliverCompare(uint64_t generation) {
  bool raise_interrupt = false;
  {
    std::lock_guard<std::mutex> lock(compare_mutex);
    if (!initialized ||
        generation != compare_generation.load(std::memory_order_relaxed)) {
      return;
    }
    scheduled_alarm = 0;
    pending_generation.store(generation, std::memory_order_relaxed);
    pending_status.store(true, std::memory_order_release);
    raise_interrupt = true;
  }
  if (raise_interrupt) {
    roo_testing::esp_idf::raiseInterruptSource(ETS_TG0_LACT_LEVEL_INTR_SOURCE);
  }
}

/// Crosses the emulated hardware boundary before entering the common handler.
void TimerInterrupt(void*) {
  if (!pending_status.exchange(false, std::memory_order_acq_rel)) return;
  const uint64_t generation =
      pending_generation.load(std::memory_order_acquire);
  if (generation != compare_generation.load(std::memory_order_acquire)) return;
  intr_handler_t handler = upper_handler.load(std::memory_order_acquire);
  if (handler != nullptr) handler(nullptr);
}

/// Advances the nonreused generation used to discard claimed stale alarms.
uint64_t NextGeneration() {
  const uint64_t next =
      compare_generation.fetch_add(1, std::memory_order_relaxed) + 1;
  CHECK_NE(next, static_cast<uint64_t>(0));
  return next;
}

}  // namespace

extern "C" {

esp_err_t esp_timer_impl_early_init(void) { return ESP_OK; }

esp_err_t esp_timer_impl_init(intr_handler_t alarm_handler) {
  if (alarm_handler == nullptr) return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(compare_mutex);
  if (initialized) return ESP_ERR_INVALID_STATE;

  intr_handle_t handle = nullptr;
  esp_err_t err =
      esp_intr_alloc(ETS_TG0_LACT_LEVEL_INTR_SOURCE, ESP_INTR_FLAG_INTRDISABLED,
                     TimerInterrupt, nullptr, &handle);
  if (err != ESP_OK) return err;
  err = esp_intr_enable(handle);
  if (err != ESP_OK) {
    esp_intr_free(handle);
    return err;
  }

  interrupt_handle = handle;
  requested_compare = kDisarmedCompare;
  scheduled_alarm = 0;
  pending_status.store(false, std::memory_order_release);
  upper_handler.store(alarm_handler, std::memory_order_release);
  initialized = true;
  return ESP_OK;
}

void esp_timer_impl_deinit(void) {
  SystemTimeAlarmId alarm = 0;
  intr_handle_t handle = nullptr;
  {
    std::lock_guard<std::mutex> lock(compare_mutex);
    if (!initialized) return;
    NextGeneration();
    initialized = false;
    requested_compare = kDisarmedCompare;
    pending_status.store(false, std::memory_order_release);
    upper_handler.store(nullptr, std::memory_order_release);
    alarm = scheduled_alarm;
    scheduled_alarm = 0;
    handle = interrupt_handle;
    interrupt_handle = nullptr;
  }
  CancelSystemTimeAlarm(alarm);
  if (handle != nullptr) {
    esp_intr_disable(handle);
    CHECK_EQ(esp_intr_free(handle), ESP_OK);
  }
}

void esp_timer_impl_set_alarm_id(uint64_t timestamp, unsigned alarm_id) {
  CHECK_EQ(alarm_id, 0U);
  SystemTimeAlarmId old_alarm = 0;
  uint64_t generation;
  {
    std::lock_guard<std::mutex> lock(compare_mutex);
    CHECK(initialized);
    generation = NextGeneration();
    requested_compare = timestamp;
    pending_status.store(false, std::memory_order_release);
    old_alarm = scheduled_alarm;
    scheduled_alarm = 0;
  }
  CancelSystemTimeAlarm(old_alarm);
  if (timestamp == kDisarmedCompare ||
      timestamp > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
    return;
  }

  const SystemTimeAlarmId new_alarm =
      ScheduleSystemTimeAlarm(static_cast<int64_t>(timestamp),
                              [generation] { DeliverCompare(generation); });
  bool keep_alarm = false;
  {
    std::lock_guard<std::mutex> lock(compare_mutex);
    if (initialized &&
        generation == compare_generation.load(std::memory_order_relaxed) &&
        pending_generation.load(std::memory_order_acquire) != generation) {
      scheduled_alarm = new_alarm;
      keep_alarm = true;
    }
  }
  if (!keep_alarm) CancelSystemTimeAlarm(new_alarm);
}

void esp_timer_impl_advance(int64_t time_us) {
  CHECK_GE(time_us, 0);
  system_time_delay_micros(static_cast<uint64_t>(time_us));
}

int64_t esp_timer_impl_get_time(void) { return system_time_get_micros(); }

int64_t esp_timer_get_time(void) { return esp_timer_impl_get_time(); }

uint64_t esp_timer_impl_get_counter_reg(void) {
  return static_cast<uint64_t>(system_time_get_micros());
}

uint64_t esp_timer_impl_get_alarm_reg(void) {
  std::lock_guard<std::mutex> lock(compare_mutex);
  return requested_compare;
}

}  // extern "C"

namespace roo_testing::esp32_shims {

bool IsEspTimerImplInitializedForHost() {
  std::lock_guard<std::mutex> lock(compare_mutex);
  return initialized;
}

}  // namespace roo_testing::esp32_shims
