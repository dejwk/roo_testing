#include "timer.h"

#include <time.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <limits>
#include <mutex>
#include <thread>

#include "glog/logging.h"
#include "roo_testing/host/scheduler_safe_host_lock.h"

extern "C" volatile sig_atomic_t system_time_auto_sync_mode
    __attribute__((weak));
volatile sig_atomic_t system_time_auto_sync_mode = 1;

namespace {

constexpr int64_t kNanosPerMicrosecond = 1000;
constexpr int64_t kMaxTimeAheadNs = 100000;

enum class InitializationState : uint8_t {
  kUninitialized,
  kInitializing,
  kManualReady,
  kAutoReady,
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
      publishAtLeast(hostUptimeNs());
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

  /// Returns the host-derived uptime under the immutable auto-sync mapping.
  int64_t hostUptimeNs() const {
    const int64_t now_ns = monotonicNowNs();
    if (now_ns <= host_origin_ns_) return 0;
    return now_ns - host_origin_ns_;
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
        monotonicNowNs() - uptime_ns_.load(std::memory_order_relaxed);
    state_.store(auto_sync ? InitializationState::kAutoReady
                           : InitializationState::kManualReady,
                 std::memory_order_release);
  }

 public:
  /// Samples the host's steady monotonic clock as signed nanoseconds.
  static int64_t monotonicNowNs() {
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

/// Converts microseconds to nanoseconds without overflowing the signed uptime.
int64_t microsToNanos(uint64_t micros) {
  CHECK_LE(micros, static_cast<uint64_t>(std::numeric_limits<int64_t>::max() /
                                         kNanosPerMicrosecond));
  return static_cast<int64_t>(micros) * kNanosPerMicrosecond;
}

/// Advances the published uptime while holding the scheduler-safe mutation
/// lock.
void addUptimeNs(int64_t delta_ns) {
  roo_testing::SchedulerSafeHostLock lock(kMutationMutex);
  kPublication.add(delta_ns);
}

/// Paces a task/native caller when explicit progression is ahead of host time.
void paceAutoSync() {
  if (!kPublication.isAutoSyncEnabled()) return;

  const int64_t current_ns = kPublication.read();
  const int64_t host_ns = kPublication.hostUptimeNs();
  if (current_ns > host_ns + kMaxTimeAheadNs) {
    std::this_thread::sleep_for(
        std::chrono::nanoseconds(current_ns - host_ns - kMaxTimeAheadNs));
  }
  kPublication.read();
}

}  // namespace

extern "C" {

void system_time_sync() { paceAutoSync(); }

void system_time_lag_ns(uint64_t ns) {
  kPublication.isAutoSyncEnabled();
  CHECK_LE(ns, static_cast<uint64_t>(std::numeric_limits<int64_t>::max()));
  addUptimeNs(static_cast<int64_t>(ns));
}

int64_t system_time_get_micros() {
  return kPublication.read() / kNanosPerMicrosecond;
}

bool system_time_is_auto_sync_enabled() {
  return kPublication.isAutoSyncEnabled();
}

void system_time_delay_micros(uint64_t micros) {
  kPublication.isAutoSyncEnabled();
  addUptimeNs(microsToNanos(micros));
  paceAutoSync();
}

void system_time_busy_wait_micros(uint64_t micros) {
  const int64_t duration_ns = microsToNanos(micros);
  const int64_t start_ns = AtomicUptimePublication::monotonicNowNs();
  CHECK_LE(duration_ns, std::numeric_limits<int64_t>::max() - start_ns);
  const int64_t deadline_ns = start_ns + duration_ns;
  while (AtomicUptimePublication::monotonicNowNs() < deadline_ns) {
  }
}

}  // extern "C"
