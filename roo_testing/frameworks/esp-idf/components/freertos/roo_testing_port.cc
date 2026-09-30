// Host glue for ESP-IDF's upstream Linux FreeRTOS port.
//
// port_idf.c in ESP-IDF also owns the process main().  roo_testing already has
// Arduino and GoogleTest mains, so this file provides only the IDF callbacks
// that the kernel needs.  The scheduler and pthread implementation continue to
// come directly from FreeRTOS-Kernel/portable/linux/port.c.

#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <limits>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "roo_testing/host_event/host_event_endpoint.h"

namespace {

// pthread cancellation runs signal handlers and forced unwinding on the task's
// stack. PTHREAD_STACK_MIN only permits thread creation; it is too small for
// that shutdown path, especially with sanitizer instrumentation.
constexpr size_t kSchedulerStackDepth = 64 * 1024 / sizeof(StackType_t);

}  // namespace

extern "C" {

void* pvPortMalloc(size_t size) { return std::malloc(size); }

void vPortFree(void* ptr) { std::free(ptr); }

size_t xPortGetFreeHeapSize(void) {
  // The host process heap is intentionally not capped like ESP32 SRAM.
  return std::numeric_limits<size_t>::max();
}

size_t xPortGetMinimumEverFreeHeapSize(void) {
  return std::numeric_limits<size_t>::max();
}

bool xPortCheckValidListMem(const void* ptr) { return ptr != nullptr; }

bool xPortCheckValidTCBMem(const void* ptr) { return ptr != nullptr; }

bool xPortcheckValidStackMem(const void* ptr) { return ptr != nullptr; }

void esp_vApplicationIdleHook(void) {
  // Match ESP-IDF's Linux port: avoid spinning a host CPU while idle, while
  // returning often enough for the kernel to reclaim deleted tasks.
  usleep(15000);
}

void esp_vApplicationTickHook(void) {
  roo_testing::internal::hostEventTickHook();
}

#if configUSE_TICK_HOOK
void vApplicationTickHook(void) { esp_vApplicationTickHook(); }
#endif

void vPortYieldOtherCore(BaseType_t core_id) { (void)core_id; }

#if configSUPPORT_STATIC_ALLOCATION
void vApplicationGetIdleTaskMemory(StaticTask_t** tcb, StackType_t** stack,
                                   uint32_t* stack_size) {
  static StaticTask_t idle_tcb;
  static StackType_t idle_stack[configMINIMAL_STACK_SIZE > kSchedulerStackDepth
                                    ? configMINIMAL_STACK_SIZE
                                    : kSchedulerStackDepth];
  *tcb = &idle_tcb;
  *stack = idle_stack;
  *stack_size = sizeof(idle_stack) / sizeof(idle_stack[0]);
}
#endif

#if configSUPPORT_STATIC_ALLOCATION && configUSE_TIMERS
void vApplicationGetTimerTaskMemory(StaticTask_t** tcb, StackType_t** stack,
                                    uint32_t* stack_size) {
  static StaticTask_t timer_tcb;
  static StackType_t
      timer_stack[configTIMER_TASK_STACK_DEPTH > kSchedulerStackDepth
                      ? configTIMER_TASK_STACK_DEPTH
                      : kSchedulerStackDepth];
  *tcb = &timer_tcb;
  *stack = timer_stack;
  *stack_size = sizeof(timer_stack) / sizeof(timer_stack[0]);
}
#endif

void __attribute__((weak)) vApplicationStackOverflowHook(TaskHandle_t task,
                                                         char* task_name) {
  (void)task;
  std::fprintf(stderr, "FreeRTOS stack overflow in task %s\n",
               task_name == nullptr ? "<unknown>" : task_name);
  std::abort();
}

}  // extern "C"
