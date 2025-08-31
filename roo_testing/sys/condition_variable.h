#pragma once

#include <chrono>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "roo_testing/sys/mutex.h"

static constexpr int kMaxWaitingThreads = 8;

namespace roo_testing {

enum class cv_status { no_timeout, timeout };

class condition_variable {
 public:
  typedef std::chrono::system_clock sysclock;

  condition_variable() noexcept;

  condition_variable(const condition_variable&) = delete;
  condition_variable& operator=(const condition_variable&) = delete;

  void wait(unique_lock<mutex>& lock) noexcept;

  template <class Predicate>
  void wait(unique_lock<mutex>& lock, Predicate pred) {
    while (!pred()) wait(lock);
  }

  void notify_one() noexcept;

  void notify_all() noexcept;

  template <typename Duration>
  cv_status wait_until(
      unique_lock<mutex>& lock,
      const std::chrono::time_point<sysclock, Duration>& duration) {
    return wait_until_impl(lock, duration);
  }

  template <typename Clock, typename Duration>
  cv_status wait_until(unique_lock<mutex>& lock,
                       const std::chrono::time_point<Clock, Duration>& when) {
    const typename Clock::time_point clock_now = Clock::now();
    const sysclock::time_point sys_now = sysclock::now();
    return wait_until_impl(lock, sys_now + (when - clock_now));
  }

  template <typename Clock, typename Duration, typename Predicate>
  bool wait_until(unique_lock<mutex>& lock,
                  const std::chrono::time_point<Clock, Duration>& when,
                  Predicate p) {
    while (!p())
      if (wait_until(lock, when) == cv_status::timeout) return p();
    return true;
  }

  template <typename Rep, typename Period>
  cv_status wait_for(unique_lock<mutex>& lock,
                     const std::chrono::duration<Rep, Period>& duration) {
    return wait_until(lock, sysclock::now() + duration);
  }

  template <typename Rep, typename Period, typename Predicate>
  bool wait_for(unique_lock<mutex>& lock,
                const std::chrono::duration<Rep, Period>& duration,
                Predicate p) {
    return wait_until(lock, sysclock::now() + duration, std::move(p));
  }

 private:
  cv_status wait_until_impl(
      unique_lock<mutex>& lock,
      const std::chrono::time_point<sysclock, std::chrono::nanoseconds>&
          when) noexcept;

  TaskHandle_t tasks_waiting_[kMaxWaitingThreads];
};

}  // namespace roo_testing