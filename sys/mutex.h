#pragma once

#include <chrono>
#include <cstdint>
#include <utility>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace roo_testing {

class mutex {
 public:
  mutex() noexcept;
  ~mutex() = default;

  mutex(const mutex&) = delete;
  mutex& operator=(const mutex&) = delete;

  void lock();
  bool try_lock();

  void unlock();

 private:
  StaticSemaphore_t mutex_;
};

template <typename Mutex>
class lock_guard {
 public:
  typedef Mutex mutex_type;

  explicit lock_guard(mutex_type& mutex) : mutex_(mutex) { mutex_.lock(); }

  ~lock_guard() { mutex_.unlock(); }

  lock_guard(const lock_guard&) = delete;
  lock_guard& operator=(const lock_guard&) = delete;

 private:
  mutex_type& mutex_;
};

// Do not lock the associated mutex.
struct defer_lock_t {
  explicit defer_lock_t() = default;
};

// Try to lock the associated mutex without blocking.
struct try_to_lock_t {
  explicit try_to_lock_t() = default;
};

// Assume the calling thread already holds a non-shared lock (i.e., a lock
// acquired by lock, try_lock, try_lock_for, or try_lock_until) on the mutex.
struct adopt_lock_t {
  explicit adopt_lock_t() = default;
};

// Tags for the unique_lock constructor.

inline constexpr defer_lock_t defer_lock{};
inline constexpr try_to_lock_t try_to_lock{};
inline constexpr adopt_lock_t adopt_lock{};

namespace internal {
void checkLockOwned(bool owns);
void checkLockUnowned(const void* lock, bool owns);
}  // namespace internal

template <typename Mutex>
class unique_lock {
 public:
  typedef Mutex mutex_type;

  unique_lock() noexcept : lock_(nullptr), owns_(false) {}

  explicit unique_lock(mutex_type& mutex) : lock_(&mutex), owns_(false) {
    lock();
  }

  unique_lock(mutex_type& mutex, defer_lock_t) noexcept
      : lock_(&mutex), owns_(false) {}

  unique_lock(mutex_type& mutex, try_to_lock_t)
      : lock_(&mutex), owns_(lock_->try_lock()) {}

  unique_lock(mutex_type& mutex, adopt_lock_t) noexcept
      : lock_(&mutex), owns_(true) {}

  template <typename Clock, typename Duration>
  unique_lock(mutex_type& mutex,
              const std::chrono::time_point<Clock, Duration>& tp)
      : lock_(&mutex), owns_(lock_->try_lock_until(tp)) {}

  template <typename Rep, typename Period>
  unique_lock(mutex_type& mutex,
              const std::chrono::duration<Rep, Period>& duration)
      : lock_(&mutex), owns_(lock_->try_lock_for(duration)) {}

  ~unique_lock() {
    if (owns_) unlock();
  }

  unique_lock(const unique_lock&) = delete;
  unique_lock& operator=(const unique_lock&) = delete;

  unique_lock(unique_lock&& lock) noexcept
      : lock_(lock.lock_), owns_(lock.owns_) {
    lock.lock_ = 0;
    lock.owns_ = false;
  }

  unique_lock& operator=(unique_lock&& lock) noexcept {
    if (owns_) unlock();

    unique_lock(std::move(lock)).swap(*this);

    lock.lock_ = 0;
    lock.owns_ = false;

    return *this;
  }

  void lock() {
    internal::checkLockUnowned(lock_, owns_);
    lock_->lock();
    owns_ = true;
  }

  bool try_lock() {
    internal::checkLockUnowned(lock_, owns_);
    owns_ = lock_->try_lock();
    return owns_;
  }

  template <typename Clock, typename Duration>
  bool try_lock_until(const std::chrono::time_point<Clock, Duration>& tp) {
    internal::checkLockUnowned(lock_, owns_);
    owns_ = lock_->try_lock_until(tp);
    return owns_;
  }

  template <typename Rep, typename Period>
  bool try_lock_for(const std::chrono::duration<Rep, Period>& duration) {
    internal::checkLockUnowned(lock_, owns_);
    owns_ = lock_->try_lock_for(duration);
    return owns_;
  }

  void unlock() {
    internal::checkLockOwned(owns_);
    if (lock_ != nullptr) {
      lock_->unlock();
      owns_ = false;
    }
  }

  void swap(unique_lock& other) noexcept {
    std::swap(lock_, other.lock_);
    std::swap(owns_, other.owns_);
  }

  mutex_type* release() noexcept {
    mutex_type* ret = lock_;
    lock_ = nullptr;
    owns_ = false;
    return ret;
  }

  bool owns_lock() const noexcept { return owns_; }

  explicit operator bool() const noexcept { return owns_lock(); }

  mutex_type* mutex() const noexcept { return lock_; }

 private:
  mutex_type* lock_;
  bool owns_;
};

}  // namespace roo_testing