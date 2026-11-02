#include "roo_testing/frameworks/freertos_posix/runner.h"

#include <atomic>
#include <thread>

#include "FreeRTOS.h"
#include "roo_testing/system/timer.h"
#include "runtime.h"
#include "task.h"

namespace roo_testing {
namespace freertos_posix {
namespace {
struct Context {
  int (*body)(void*);
  void* argument;
  std::atomic<int> result{1};
};

void RunBody(void* pointer) {
  Context& context = *static_cast<Context*>(pointer);
  context.result.store(context.body(context.argument),
                       std::memory_order_release);
  RooFreeRtosEndScheduler();
}
}  // namespace

int Run(int (*body)(void*), void* argument) {
  static bool started = false;
  configASSERT(!started && body != nullptr);
  started = true;
  system_time_is_auto_sync_enabled();
  Context context{body, argument};
  TaskHandle_t task = nullptr;
  if (xTaskCreate(RunBody, "pico_main", 64 * 1024, &context,
                  tskIDLE_PRIORITY + 1, &task) != pdPASS)
    return 1;
  vTaskStartScheduler();
  RooFreeRtosDeleteFinalTask();
  while (!TryBeginSystemTimeServiceShutdownForHost()) std::this_thread::yield();
  FinishSystemTimeServiceShutdownForHost();
  return context.result.load(std::memory_order_acquire);
}
}  // namespace freertos_posix
}  // namespace roo_testing
