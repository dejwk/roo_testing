// Include Pico first to exercise its macros against subsequent host headers.
// clang-format off
#include "pico.h"
#include <condition_variable>
#include <mutex>
// clang-format on

// Verifies C++ synchronization headers and their pthread initializers remain
// usable after Pico defines its compiler attributes.
int main() {
  std::mutex mutex;
  std::condition_variable ready;
  std::unique_lock<std::mutex> lock(mutex);
  ready.wait(lock, [] { return true; });
  return lock.owns_lock() ? 0 : 1;
}
