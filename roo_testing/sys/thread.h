#pragma once

#include <chrono>

#include "roo_testing/sys/internal/callable.h"

namespace roo_testing {

class thread {
 public:
  class id {
   public:
    id() : id_(nullptr) {}
    id(void* id) : id_(id) {}
    bool operator==(const id& other) const { return id_ == other.id_; }

   private:
    friend class thread;

    void* id_;
  };
  thread() noexcept = default;

  thread(const thread&) = delete;

  thread(thread&& other) noexcept { swap(other); }

  template <typename Callable, typename... Args,
            typename = internal::RequireNotSame<Callable, thread>>
  explicit thread(Callable&& callable, Args&&... args) {
    static_assert(std::is_invocable<typename std::decay<Callable>::type,
                                    typename std::decay<Args>::type...>::value,
                  "std::thread argument must be invocable");

    start(internal::MakeDynamicCallableWithArgs(
        std::forward<Callable>(callable), std::forward<Args>(args)...));
  }

  ~thread();

  thread& operator=(const thread&) = delete;

  thread& operator=(thread&& other) noexcept;

  void swap(thread& other) noexcept { std::swap(id_, other.id_); }

  bool joinable() const noexcept { return !(id_ == id()); }

  void join();

  void detach();

  thread::id get_id() const noexcept { return id_; }

 private:
  void start(std::unique_ptr<internal::VirtualCallable>);

  id id_;
};

namespace this_thread {

thread::id get_id() noexcept;

void yield() noexcept;

namespace internal {
void sleep_ns(std::chrono::nanoseconds);
}

template <typename Rep, typename Period>
inline void sleep_for(const std::chrono::duration<Rep, Period>& duration) {
  if (duration <= duration.zero()) return;
  internal::sleep_ns(
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration));
}

template <typename Clock, typename Duration>
inline void sleep_until(const std::chrono::time_point<Clock, Duration>& when) {
  auto now = Clock::now();
  if (Clock::is_steady) {
    if (now < when) sleep_for(when - now);
    return;
  }
  while (now < when) {
    sleep_for(when - now);
    now = Clock::now();
  }
}
}  // namespace this_thread

}  // namespace roo_testing