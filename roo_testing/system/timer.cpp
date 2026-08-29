#include "timer.h"

#include <errno.h>
#include <pthread.h>
#include <time.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <limits>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "glog/logging.h"
#include "roo_testing/host/scheduler_safe_host_lock.h"

extern "C" volatile sig_atomic_t system_time_auto_sync_mode
    __attribute__((weak));
volatile sig_atomic_t system_time_auto_sync_mode = 1;

namespace {

constexpr int64_t kNanosPerMicrosecond = 1000;
constexpr int64_t kMaxTimeAheadNs = 100000;
constexpr int64_t kMinAlarmUptimeMicros =
    std::numeric_limits<int64_t>::min() / kNanosPerMicrosecond;
constexpr int64_t kMaxAlarmUptimeMicros =
    std::numeric_limits<int64_t>::max() / kNanosPerMicrosecond;

enum class InitializationState : uint8_t {
  kUninitialized,
  kInitializing,
  kManualReady,
  kAutoReady,
};

enum class AlarmServiceState : uint8_t {
  kRunning,
  kClosing,
  kStopped,
};

/// Publishes a monotonic uptime without locking on the passive-read path.
class AtomicUptimePublication {
 public:
  constexpr AtomicUptimePublication() = default;

  /// Returns the published uptime, catching it up to host time in auto mode.
  int64_t read() {
    InitializationState state = state_.load(std::memory_order_acquire);
    if (state == InitializationState::kUninitialized) {
      initialize();
      state = state_.load(std::memory_order_acquire);
    }
    if (state == InitializationState::kAutoReady) {
      publishAtLeast(HostUptimeNs());
    }
    return uptime_ns_.load(std::memory_order_relaxed);
  }

  /// Waits for task/native initialization and returns the selected mode.
  bool isAutoSyncEnabled() {
    InitializationState state = state_.load(std::memory_order_acquire);
    if (state == InitializationState::kUninitialized) {
      initialize();
      state = state_.load(std::memory_order_acquire);
    }
    while (state == InitializationState::kInitializing) {
      std::this_thread::yield();
      state = state_.load(std::memory_order_acquire);
    }
    return state == InitializationState::kAutoReady;
  }

  /// Adds `delta_ns` after checking that the resulting uptime is representable.
  void add(int64_t delta_ns) {
    CHECK_GE(delta_ns, 0);
    int64_t current = uptime_ns_.load(std::memory_order_relaxed);
    while (true) {
      CHECK_LE(delta_ns, std::numeric_limits<int64_t>::max() - current);
      if (uptime_ns_.compare_exchange_weak(current, current + delta_ns,
                                           std::memory_order_release,
                                           std::memory_order_relaxed)) {
        return;
      }
    }
  }

  /// Raises the published uptime floor to `candidate_ns`.
  void publishAtLeastForMutation(int64_t candidate_ns) {
    publishAtLeast(candidate_ns);
  }

  /// Returns the host-derived uptime under the immutable auto-sync mapping.
  int64_t HostUptimeNs() const {
    const int64_t now_ns = MonotonicNowNs();
    if (now_ns <= host_origin_ns_) return 0;
    return now_ns - host_origin_ns_;
  }

  /// Maps an uptime deadline to the host monotonic clock when representable.
  bool HostDeadlineNs(int64_t uptime_ns, int64_t* deadline_ns) const {
    if (uptime_ns > std::numeric_limits<int64_t>::max() - host_origin_ns_) {
      return false;
    }
    *deadline_ns = host_origin_ns_ + uptime_ns;
    return *deadline_ns > 0;
  }

 private:
  /// Claims initialization and release-publishes the immutable host origin.
  void initialize() {
    InitializationState expected = InitializationState::kUninitialized;
    if (!state_.compare_exchange_strong(
            expected, InitializationState::kInitializing,
            std::memory_order_acq_rel, std::memory_order_acquire)) {
      return;
    }

    const bool auto_sync = system_time_auto_sync_mode != 0;
    host_origin_ns_ =
        MonotonicNowNs() - uptime_ns_.load(std::memory_order_relaxed);
    state_.store(auto_sync ? InitializationState::kAutoReady
                           : InitializationState::kManualReady,
                 std::memory_order_release);
  }

 public:
  /// Samples the host's steady monotonic clock as signed nanoseconds.
  static int64_t MonotonicNowNs() {
    timespec now;
    CHECK_EQ(clock_gettime(CLOCK_MONOTONIC, &now), 0);
    CHECK_LE(now.tv_sec, std::numeric_limits<int64_t>::max() / 1000000000LL);
    return static_cast<int64_t>(now.tv_sec) * 1000000000LL + now.tv_nsec;
  }

 private:
  /// Atomically raises the uptime floor to `candidate_ns`.
  void publishAtLeast(int64_t candidate_ns) {
    int64_t current = uptime_ns_.load(std::memory_order_relaxed);
    while (candidate_ns > current &&
           !uptime_ns_.compare_exchange_weak(current, candidate_ns,
                                             std::memory_order_release,
                                             std::memory_order_relaxed)) {
    }
  }

  std::atomic<int64_t> uptime_ns_{0};
  std::atomic<InitializationState> state_{InitializationState::kUninitialized};
  int64_t host_origin_ns_ = 0;
};

static_assert(std::atomic<int64_t>::is_always_lock_free);
static_assert(std::atomic<InitializationState>::is_always_lock_free);

// Its constexpr constructor makes this namespace-scope publication
// constant-initialized under the repository's C++17 toolchain.
AtomicUptimePublication kPublication;
std::mutex kMutationMutex;
int64_t kReservedUptimeNs = 0;
pthread_mutex_t kAlarmWakeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t kAlarmWakeCondition;
pthread_once_t kAlarmWakeConditionOnce = PTHREAD_ONCE_INIT;
std::atomic<uint64_t> kAlarmWakeGeneration{0};

struct AlarmRecord {
  SystemTimeAlarmId id;
  std::function<void()> callback;
};

using AlarmQueue = std::multimap<int64_t, AlarmRecord>;

AlarmQueue kAlarms;
std::unordered_map<SystemTimeAlarmId, AlarmQueue::iterator> kAlarmIndex;
SystemTimeAlarmId kNextAlarmId = 1;
bool kDeliveryActive = false;
bool kAlarmWorkerStarted = false;
pthread_t kAlarmWorker;
AlarmServiceState kAlarmServiceState = AlarmServiceState::kRunning;

/// Initializes the alarm wait condition against the host monotonic clock.
void InitializeAlarmWakeCondition() {
  pthread_condattr_t attributes;
  CHECK_EQ(pthread_condattr_init(&attributes), 0);
  CHECK_EQ(pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC), 0);
  CHECK_EQ(pthread_cond_init(&kAlarmWakeCondition, &attributes), 0);
  CHECK_EQ(pthread_condattr_destroy(&attributes), 0);
}

/// Wakes the worker after changing alarm state or delivery ownership.
void NotifyAlarmWorker() {
  kAlarmWakeGeneration.fetch_add(1, std::memory_order_release);
  CHECK_EQ(pthread_once(&kAlarmWakeConditionOnce, InitializeAlarmWakeCondition),
           0);
  CHECK_EQ(pthread_cond_broadcast(&kAlarmWakeCondition), 0);
}

/// Converts microseconds to nanoseconds without overflowing the signed uptime.
int64_t MicrosToNanos(uint64_t micros) {
  CHECK_LE(micros, static_cast<uint64_t>(std::numeric_limits<int64_t>::max() /
                                         kNanosPerMicrosecond));
  return static_cast<int64_t>(micros) * kNanosPerMicrosecond;
}

/// Advances the published uptime while holding the scheduler-safe mutation
/// lock.
void AddUptimeNs(int64_t delta_ns) {
  {
    roo_testing::SchedulerSafeHostLock lock(kMutationMutex);
    CHECK(kAlarmServiceState == AlarmServiceState::kRunning);
    kPublication.add(delta_ns);
    kReservedUptimeNs = std::max(kReservedUptimeNs, kPublication.read());
  }
  NotifyAlarmWorker();
}

/// Reserves a monotonic target for an explicit delay before it can be pumped.
int64_t ReserveDelayedUptimeNs(int64_t duration_ns) {
  roo_testing::SchedulerSafeHostLock lock(kMutationMutex);
  CHECK(kAlarmServiceState == AlarmServiceState::kRunning);
  const int64_t current_ns = kPublication.read();
  const int64_t base_ns = std::max(current_ns, kReservedUptimeNs);
  CHECK_LE(duration_ns, std::numeric_limits<int64_t>::max() - base_ns);
  kReservedUptimeNs = base_ns + duration_ns;
  return kReservedUptimeNs;
}

/// Converts an exactly representable alarm uptime to nanoseconds.
int64_t AlarmMicrosToNanos(int64_t micros) {
  CHECK_GE(micros, kMinAlarmUptimeMicros);
  CHECK_LE(micros, kMaxAlarmUptimeMicros);
  return micros * kNanosPerMicrosecond;
}

/// Clears delivery ownership after a callback violates its non-throwing
/// contract.
void ReleaseDeliveryOwnership() {
  {
    roo_testing::SchedulerSafeHostLock lock(kMutationMutex);
    kDeliveryActive = false;
  }
  NotifyAlarmWorker();
}

/// Releases delivery ownership if a callback unwinds past the delivery loop.
class DeliveryOwnershipGuard {
 public:
  ~DeliveryOwnershipGuard() {
    if (active_) ReleaseDeliveryOwnership();
  }

  void dismiss() { active_ = false; }

 private:
  bool active_ = true;
};

void DeliverAlarmsThrough(int64_t limit_ns, bool advance_time);

/// Waits until queue state changes or an auto-sync deadline reaches host time.
void WaitForAlarmWorkerWake(uint64_t observed_generation,
                            bool wait_for_deadline,
                            int64_t deadline_uptime_us) {
  CHECK_EQ(pthread_mutex_lock(&kAlarmWakeMutex), 0);
  if (kAlarmWakeGeneration.load(std::memory_order_acquire) ==
      observed_generation) {
    int64_t host_deadline_ns;
    if (wait_for_deadline &&
        kPublication.HostDeadlineNs(AlarmMicrosToNanos(deadline_uptime_us),
                                    &host_deadline_ns)) {
      const timespec deadline = {host_deadline_ns / 1000000000LL,
                                 host_deadline_ns % 1000000000LL};
      const int result = pthread_cond_timedwait(&kAlarmWakeCondition,
                                                &kAlarmWakeMutex, &deadline);
      CHECK(result == 0 || result == ETIMEDOUT);
    } else {
      CHECK_EQ(pthread_cond_wait(&kAlarmWakeCondition, &kAlarmWakeMutex), 0);
    }
  }
  CHECK_EQ(pthread_mutex_unlock(&kAlarmWakeMutex), 0);
}

/// Delivers auto-sync alarms from a native thread independent of FreeRTOS.
void* RunAlarmWorker(void*) {
  while (true) {
    uint64_t generation;
    int64_t deadline_uptime_us = 0;
    bool deliver_due_work = false;
    bool wait_for_deadline = false;
    {
      roo_testing::SchedulerSafeHostLock lock(kMutationMutex);
      if (kAlarmServiceState != AlarmServiceState::kRunning) return nullptr;
      generation = kAlarmWakeGeneration.load(std::memory_order_acquire);
      if (!kAlarms.empty()) {
        deadline_uptime_us = kAlarms.begin()->first;
        deliver_due_work =
            AlarmMicrosToNanos(deadline_uptime_us) <= kPublication.read() &&
            !kDeliveryActive;
        wait_for_deadline = !deliver_due_work && !kDeliveryActive;
      }
    }

    if (deliver_due_work) {
      DeliverAlarmsThrough(kPublication.read(), false);
      continue;
    }
    WaitForAlarmWorkerWake(generation, wait_for_deadline, deadline_uptime_us);
  }
}

/// Starts the auto-sync worker while the caller holds the mutation lock.
void StartAlarmWorkerUnderMutationLock() {
  CHECK_EQ(pthread_once(&kAlarmWakeConditionOnce, InitializeAlarmWakeCondition),
           0);
  CHECK_EQ(pthread_create(&kAlarmWorker, nullptr, RunAlarmWorker, nullptr), 0);
  kAlarmWorkerStarted = true;
}

/// Claims and invokes work due while advancing no later than `limit_ns`.
void DeliverAlarmsThrough(int64_t limit_ns, bool advance_time) {
  {
    roo_testing::SchedulerSafeHostLock lock(kMutationMutex);
    if (kAlarmServiceState != AlarmServiceState::kRunning) return;
    if (kDeliveryActive) {
      if (advance_time) kPublication.publishAtLeastForMutation(limit_ns);
      return;
    }
    kDeliveryActive = true;
  }
  DeliveryOwnershipGuard ownership;

  while (true) {
    std::function<void()> callback;
    bool no_due_work = false;
    {
      roo_testing::SchedulerSafeHostLock lock(kMutationMutex);
      int64_t current_ns = kPublication.read();
      if (advance_time && current_ns < limit_ns) {
        const AlarmQueue::iterator first = kAlarms.begin();
        if (first != kAlarms.end()) {
          const int64_t deadline_ns = AlarmMicrosToNanos(first->first);
          if (deadline_ns > current_ns && deadline_ns <= limit_ns) {
            kPublication.publishAtLeastForMutation(deadline_ns);
          } else {
            kPublication.publishAtLeastForMutation(limit_ns);
          }
        } else {
          kPublication.publishAtLeastForMutation(limit_ns);
        }
        current_ns = kPublication.read();
      }

      const AlarmQueue::iterator first = kAlarms.begin();
      if (first == kAlarms.end() ||
          AlarmMicrosToNanos(first->first) > current_ns) {
        no_due_work = true;
      } else {
        callback = std::move(first->second.callback);
        kAlarmIndex.erase(first->second.id);
        kAlarms.erase(first);
      }
    }

    if (no_due_work) {
      ReleaseDeliveryOwnership();
      ownership.dismiss();
      return;
    }
    NotifyAlarmWorker();
    callback();
  }
}

/// Paces a task/native caller when explicit progression is ahead of host time.
void PaceAutoSync() {
  if (!kPublication.isAutoSyncEnabled()) return;

  const int64_t current_ns = kPublication.read();
  const int64_t host_ns = kPublication.HostUptimeNs();
  if (current_ns > host_ns + kMaxTimeAheadNs) {
    std::this_thread::sleep_for(
        std::chrono::nanoseconds(current_ns - host_ns - kMaxTimeAheadNs));
  }
  kPublication.read();
}

}  // namespace

SystemTimeAlarmId ScheduleSystemTimeAlarm(int64_t deadline_uptime_us,
                                          std::function<void()> callback) {
  CHECK(callback);
  AlarmMicrosToNanos(deadline_uptime_us);
  const bool auto_sync = kPublication.isAutoSyncEnabled();

  SystemTimeAlarmId id;
  {
    roo_testing::SchedulerSafeHostLock lock(kMutationMutex);
    CHECK(kAlarmServiceState == AlarmServiceState::kRunning);
    CHECK_NE(kNextAlarmId, static_cast<SystemTimeAlarmId>(0));
    id = kNextAlarmId++;
    AlarmQueue::iterator record = kAlarms.emplace(
        deadline_uptime_us, AlarmRecord{id, std::move(callback)});
    kAlarmIndex.emplace(id, record);
    if (auto_sync && !kAlarmWorkerStarted) {
      StartAlarmWorkerUnderMutationLock();
    }
  }
  NotifyAlarmWorker();
  return id;
}

void CancelSystemTimeAlarm(SystemTimeAlarmId id) {
  if (id == 0) return;

  std::function<void()> discarded_callback;
  {
    roo_testing::SchedulerSafeHostLock lock(kMutationMutex);
    const std::unordered_map<SystemTimeAlarmId, AlarmQueue::iterator>::iterator
        found = kAlarmIndex.find(id);
    if (found == kAlarmIndex.end()) return;
    discarded_callback = std::move(found->second->second.callback);
    kAlarms.erase(found->second);
    kAlarmIndex.erase(found);
  }
  NotifyAlarmWorker();
}

void ProcessSystemTimeAlarms() {
  {
    roo_testing::SchedulerSafeHostLock lock(kMutationMutex);
    if (kAlarmServiceState != AlarmServiceState::kRunning) return;
  }
  DeliverAlarmsThrough(kPublication.read(), false);
}

bool TryBeginSystemTimeServiceShutdownForHost() {
  {
    roo_testing::SchedulerSafeHostLock lock(kMutationMutex);
    if (kDeliveryActive) return false;
    if (kAlarmServiceState != AlarmServiceState::kRunning) return false;
    kAlarmServiceState = AlarmServiceState::kClosing;
  }
  NotifyAlarmWorker();
  return true;
}

void FinishSystemTimeServiceShutdownForHost() {
  pthread_t worker;
  bool join_worker = false;
  {
    roo_testing::SchedulerSafeHostLock lock(kMutationMutex);
    CHECK(kAlarmServiceState == AlarmServiceState::kClosing);
    if (kAlarmWorkerStarted) {
      worker = kAlarmWorker;
      join_worker = true;
    }
  }
  if (join_worker) CHECK_EQ(pthread_join(worker, nullptr), 0);

  std::vector<std::function<void()>> discarded_callbacks;
  {
    roo_testing::SchedulerSafeHostLock lock(kMutationMutex);
    for (AlarmQueue::iterator alarm = kAlarms.begin(); alarm != kAlarms.end();
         ++alarm) {
      discarded_callbacks.push_back(std::move(alarm->second.callback));
    }
    kAlarms.clear();
    kAlarmIndex.clear();
    kAlarmServiceState = AlarmServiceState::kStopped;
  }
}

extern "C" {

void system_time_sync() { PaceAutoSync(); }

void system_time_lag_ns(uint64_t ns) {
  kPublication.isAutoSyncEnabled();
  CHECK_LE(ns, static_cast<uint64_t>(std::numeric_limits<int64_t>::max()));
  AddUptimeNs(static_cast<int64_t>(ns));
}

int64_t system_time_get_micros() {
  return kPublication.read() / kNanosPerMicrosecond;
}

bool system_time_is_auto_sync_enabled() {
  return kPublication.isAutoSyncEnabled();
}

void system_time_delay_micros(uint64_t micros) {
  kPublication.isAutoSyncEnabled();
  DeliverAlarmsThrough(ReserveDelayedUptimeNs(MicrosToNanos(micros)), true);
  PaceAutoSync();
}

void system_time_busy_wait_micros(uint64_t micros) {
  const int64_t duration_ns = MicrosToNanos(micros);
  const int64_t start_ns = AtomicUptimePublication::MonotonicNowNs();
  CHECK_LE(duration_ns, std::numeric_limits<int64_t>::max() - start_ns);
  const int64_t deadline_ns = start_ns + duration_ns;
  while (AtomicUptimePublication::MonotonicNowNs() < deadline_ns) {
  }
}

}  // extern "C"
