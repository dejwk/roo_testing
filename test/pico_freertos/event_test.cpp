#include <pthread.h>

#include <atomic>
#include <cstdlib>
#include <thread>

extern "C" {
#include "wait_for_event.h"
}

namespace {
void Check(bool condition) {
  if (!condition) std::abort();
}

struct Wait {
  event* value;
  std::atomic<bool> ready{false};
};

// Announces entry before blocking; pending deferred cancellation is delivered
// by the condition wait after it has acquired the mutex.
void* Block(void* pointer) {
  Wait& wait = *static_cast<Wait*>(pointer);
  wait.ready.store(true, std::memory_order_release);
  event_wait(wait.value);
  return nullptr;
}
}  // namespace

// Verifies cancellation unlocks the event before signal/delete, including the
// cancel-before-wait race. The original port deadlocks in event_signal().
int main() {
  for (int repeat = 0; repeat < 100; ++repeat) {
    Wait wait{event_create()};
    Check(wait.value != nullptr);
    pthread_t thread;
    Check(pthread_create(&thread, nullptr, Block, &wait) == 0);
    while (!wait.ready.load(std::memory_order_acquire))
      std::this_thread::yield();
    Check(pthread_cancel(thread) == 0);
    void* result;
    Check(pthread_join(thread, &result) == 0);
    Check(result == PTHREAD_CANCELED);
    event_signal(wait.value);
    Check(event_wait(wait.value));
    Check(!event_wait_timed(wait.value, 1));
    event_signal(wait.value);
    Check(event_wait_timed(wait.value, 1));
    event_delete(wait.value);
  }
}
