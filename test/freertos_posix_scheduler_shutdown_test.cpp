#include <algorithm>
#include <cstdio>
#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr size_t kGuardDepth = 32 * 1024 / sizeof(StackType_t);
constexpr StackType_t kGuardValue = 0x5a;
// Host libraries can reserve static TLS inside each pthread stack allocation.
thread_local volatile char host_tls[8 * 1024];
std::vector<StackType_t> idle_storage;
std::vector<StackType_t> timer_storage;

// Keep a writable guard below each real pthread stack so cancellation overflow
// is reported deterministically instead of corrupting unrelated global objects.
void AddGuard(std::vector<StackType_t>& storage, StackType_t** stack,
              uint32_t depth) {
  storage.assign(kGuardDepth + depth, kGuardValue);
  *stack = storage.data() + kGuardDepth;
}

bool GuardIntact(const std::vector<StackType_t>& storage) {
  return storage.size() > kGuardDepth &&
         std::all_of(storage.begin(), storage.begin() + kGuardDepth,
                     [](StackType_t value) { return value == kGuardValue; });
}

void StopScheduler(void*) {
  // Let the idle and timer tasks reach their blocking host calls before they
  // are cancelled and joined by the scheduler's main thread.
  host_tls[0] = 1;
  vTaskDelay(5);
  vTaskEndScheduler();
}

}  // namespace

extern "C" void __real_vApplicationGetIdleTaskMemory(StaticTask_t**,
                                                     StackType_t**, uint32_t*);
extern "C" void __real_vApplicationGetTimerTaskMemory(StaticTask_t**,
                                                      StackType_t**, uint32_t*);

extern "C" void __wrap_vApplicationGetIdleTaskMemory(StaticTask_t** tcb,
                                                     StackType_t** stack,
                                                     uint32_t* depth) {
  __real_vApplicationGetIdleTaskMemory(tcb, stack, depth);
  AddGuard(idle_storage, stack, *depth);
}

extern "C" void __wrap_vApplicationGetTimerTaskMemory(StaticTask_t** tcb,
                                                      StackType_t** stack,
                                                      uint32_t* depth) {
  __real_vApplicationGetTimerTaskMemory(tcb, stack, depth);
  AddGuard(timer_storage, stack, *depth);
}

// Verifies that cancelling scheduler-owned pthreads preserves memory below
// their stacks, including when sanitizer instrumentation increases stack use.
int main() {
  if (xTaskCreate(StopScheduler, "shutdown", 64 * 1024, nullptr,
                  tskIDLE_PRIORITY + 1, nullptr) != pdPASS) {
    return 1;
  }
  vTaskStartScheduler();
  if (!GuardIntact(idle_storage) || !GuardIntact(timer_storage)) {
    std::fprintf(stderr, "Scheduler shutdown overflowed a task stack\n");
    return 1;
  }
  return 0;
}
