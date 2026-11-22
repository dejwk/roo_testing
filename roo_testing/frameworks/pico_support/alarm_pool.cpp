#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <utility>

#include "glog/logging.h"
#include "pico/time.h"

namespace {

constexpr uint64_t kMaxDeadline = INT64_MAX / 1000;

struct AlarmEntry {
  alarm_id_t id = 0;
  uint64_t deadline = 0;
  alarm_callback_t callback = nullptr;
  void* data = nullptr;
};

}  // namespace

struct alarm_pool {
  uint channel;
  AlarmEntry* entries;
  uint capacity;
  alarm_id_t next_id;
  std::unique_ptr<AlarmEntry[]> owned_entries;
};

namespace {

std::array<alarm_pool_t*, NUM_ALARMS> pools{};
int rp2040_timer;

#if !PICO_TIME_DEFAULT_ALARM_POOL_DISABLED
/// Provides the reserved default pool, including during global initialization.
alarm_pool_t& DefaultPool() {
  static std::array<AlarmEntry, PICO_TIME_DEFAULT_ALARM_POOL_MAX_TIMERS>
      entries;
  static alarm_pool_t pool{PICO_TIME_DEFAULT_ALARM_POOL_HARDWARE_ALARM_NUM,
                           entries.data(), entries.size(), 1, nullptr};
  return pool;
}
#endif

/// Finds the earliest live entry, preserving insertion order for equal times.
AlarmEntry* NextEntry(alarm_pool_t& pool) {
  AlarmEntry* next = nullptr;
  for (uint i = 0; i < pool.capacity; ++i) {
    AlarmEntry& entry = pool.entries[i];
    if (entry.id == 0) continue;
    if (next == nullptr || entry.deadline < next->deadline ||
        (entry.deadline == next->deadline && entry.id < next->id)) {
      next = &entry;
    }
  }
  return next;
}

/// Arms the hardware compare for the next pool deadline.
void ArmNext(alarm_pool_t& pool) {
  const AlarmEntry* next = NextEntry(pool);
  if (next == nullptr) {
    hardware_alarm_cancel(pool.channel);
  } else if (hardware_alarm_set_target(pool.channel,
                                       from_us_since_boot(next->deadline))) {
    hardware_alarm_force_irq(pool.channel);
  }
}

/// Finds an entry by its current ID, ignoring cancelled or expired handles.
AlarmEntry* FindEntry(alarm_pool_t& pool, alarm_id_t id) {
  if (id <= 0) return nullptr;
  AlarmEntry* end = pool.entries + pool.capacity;
  AlarmEntry* entry =
      std::find_if(pool.entries, end,
                   [id](const AlarmEntry& item) { return item.id == id; });
  return entry == end ? nullptr : entry;
}

/// Converts a signed repeat interval to its unsigned magnitude.
uint64_t Magnitude(int64_t delay) {
  return delay < 0 ? uint64_t{0} - static_cast<uint64_t>(delay)
                   : static_cast<uint64_t>(delay);
}

/// Dispatches due callbacks in timer IRQ context and rearms the next compare.
void DispatchAlarmPool(uint channel) {
  alarm_pool_t& pool = *pools[channel];
  for (;;) {
    AlarmEntry* entry = NextEntry(pool);
    if (entry == nullptr || entry->deadline > time_us_64()) break;
    const alarm_id_t id = entry->id;
    const int64_t delay = entry->callback(id, entry->data);
    if (entry->id != id) continue;  // The callback cancelled its own entry.
    const uint64_t base = delay < 0 ? entry->deadline : time_us_64();
    if (delay == 0 || Magnitude(delay) > kMaxDeadline - base) {
      entry->id = 0;
    } else {
      entry->deadline = base + Magnitude(delay);
    }
  }
  ArmNext(pool);
}

/// Connects the bounded software pool to its claimed hardware compare.
void ConnectPool(alarm_pool_t& pool) {
  pools[pool.channel] = &pool;
  hardware_alarm_set_callback(pool.channel, DispatchAlarmPool);
}

/// Allocates a custom pool after its caller claims a hardware compare.
alarm_pool_t* NewPool(uint channel, uint capacity) {
  auto entries = std::make_unique<AlarmEntry[]>(capacity);
  alarm_pool_t* pool =
      new alarm_pool_t{channel, entries.get(), capacity, 1, std::move(entries)};
  ConnectPool(*pool);
  return pool;
}

/// Adapts the SDK repeating-timer callback to a signed interval.
int64_t RepeatAlarm(alarm_id_t, void* data) {
  repeating_timer_t& timer = *static_cast<repeating_timer_t*>(data);
  return timer.callback(&timer) ? timer.delay_us : 0;
}

}  // namespace

extern "C" {

void alarm_pool_init_default() {
#if !PICO_TIME_DEFAULT_ALARM_POOL_DISABLED
  alarm_pool_t& pool = DefaultPool();
  if (pools[pool.channel] == nullptr) ConnectPool(pool);
#endif
}

void runtime_init_default_alarm_pool() { alarm_pool_init_default(); }

#if !PICO_TIME_DEFAULT_ALARM_POOL_DISABLED
alarm_pool_t* alarm_pool_get_default() {
  alarm_pool_init_default();
  return &DefaultPool();
}
#endif

alarm_pool_timer_t* alarm_pool_timer_for_timer_num(uint timer_num) {
  CHECK_EQ(timer_num, 0u);
  return &rp2040_timer;
}

alarm_pool_timer_t* alarm_pool_get_default_timer() { return &rp2040_timer; }

alarm_pool_t* alarm_pool_create_on_timer(alarm_pool_timer_t* timer,
                                         uint channel, uint capacity) {
  CHECK_EQ(timer, &rp2040_timer);
  CHECK_GT(capacity, 0u);
  CHECK_LE(capacity, 255u);
  hardware_alarm_claim(channel);
  return NewPool(channel, capacity);
}

alarm_pool_t* alarm_pool_create_on_timer_with_unused_hardware_alarm(
    alarm_pool_timer_t* timer, uint capacity) {
  CHECK_EQ(timer, &rp2040_timer);
  CHECK_GT(capacity, 0u);
  CHECK_LE(capacity, 255u);
  return NewPool(static_cast<uint>(hardware_alarm_claim_unused(true)),
                 capacity);
}

uint alarm_pool_timer_alarm_num(alarm_pool_t* pool) { return pool->channel; }

uint alarm_pool_core_num(alarm_pool_t*) { return 0; }

void alarm_pool_destroy(alarm_pool_t* pool) {
  CHECK_EQ(__get_current_exception(), 0u);
  CHECK(pool->owned_entries != nullptr) << "Cannot destroy the default pool";
  hardware_alarm_unclaim(pool->channel);
  pools[pool->channel] = nullptr;
  delete pool;
}

alarm_id_t alarm_pool_add_alarm_at(alarm_pool_t* pool, absolute_time_t when,
                                   alarm_callback_t callback, void* data,
                                   bool fire_if_past) {
  if (!fire_if_past && to_us_since_boot(when) < time_us_64()) return 0;
  return alarm_pool_add_alarm_at_force_in_context(pool, when, callback, data);
}

alarm_id_t alarm_pool_add_alarm_at_force_in_context(alarm_pool_t* pool,
                                                    absolute_time_t when,
                                                    alarm_callback_t callback,
                                                    void* data) {
  CHECK(callback != nullptr);
  const uint64_t deadline = to_us_since_boot(when);
  CHECK_LE(deadline, kMaxDeadline);
  AlarmEntry* end = pool->entries + pool->capacity;
  AlarmEntry* entry = std::find_if(
      pool->entries, end, [](const AlarmEntry& item) { return item.id == 0; });
  if (entry == end) return -1;
  CHECK_LT(pool->next_id, INT32_MAX);
  const alarm_id_t id = pool->next_id++;
  *entry = {id, deadline, callback, data};
  ArmNext(*pool);
  return id;
}

bool alarm_pool_cancel_alarm(alarm_pool_t* pool, alarm_id_t id) {
  AlarmEntry* entry = FindEntry(*pool, id);
  if (entry == nullptr) return false;
  entry->id = 0;
  ArmNext(*pool);
  return true;
}

bool alarm_pool_add_repeating_timer_us(alarm_pool_t* pool, int64_t delay,
                                       repeating_timer_callback_t callback,
                                       void* data, repeating_timer_t* out) {
  CHECK(callback != nullptr);
  if (delay == 0) delay = 1;
  out->pool = pool;
  out->callback = callback;
  out->delay_us = delay;
  out->user_data = data;
  out->alarm_id = alarm_pool_add_alarm_at_force_in_context(
      pool, make_timeout_time_us(Magnitude(delay)), RepeatAlarm, out);
  return out->alarm_id > 0;
}

bool cancel_repeating_timer(repeating_timer_t* timer) {
  if (timer->alarm_id <= 0) return false;
  const bool cancelled = alarm_pool_cancel_alarm(timer->pool, timer->alarm_id);
  timer->alarm_id = 0;
  return cancelled;
}

int64_t alarm_pool_remaining_alarm_time_us(alarm_pool_t* pool, alarm_id_t id) {
  const AlarmEntry* entry = FindEntry(*pool, id);
  if (entry == nullptr) return -1;
  return static_cast<int64_t>(entry->deadline) -
         static_cast<int64_t>(time_us_64());
}

int32_t alarm_pool_remaining_alarm_time_ms(alarm_pool_t* pool, alarm_id_t id) {
  const int64_t us = alarm_pool_remaining_alarm_time_us(pool, id);
  if (us < 0) return -1;
  return static_cast<int32_t>(std::min<int64_t>(us / 1000, INT32_MAX));
}

#if !PICO_TIME_DEFAULT_ALARM_POOL_DISABLED
int64_t remaining_alarm_time_us(alarm_id_t id) {
  return alarm_pool_remaining_alarm_time_us(alarm_pool_get_default(), id);
}

int32_t remaining_alarm_time_ms(alarm_id_t id) {
  return alarm_pool_remaining_alarm_time_ms(alarm_pool_get_default(), id);
}
#endif

}  // extern "C"
