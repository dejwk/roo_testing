// Host glue for ESP-IDF's upstream Linux FreeRTOS port.
//
// port_idf.c in ESP-IDF also owns the process main().  roo_testing already has
// Arduino and GoogleTest mains, so this file provides only the IDF callbacks
// that the kernel needs.  The scheduler and pthread implementation continue to
// come directly from FreeRTOS-Kernel/portable/linux/port.c.

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "roo_testing/host_event/host_event_endpoint.h"

extern "C" {

BaseType_t xPortCheckIfInISR(void) {
  // The POSIX port represents interrupts as signals. roo_testing does not
  // currently emulate an interrupt context exposed to application code.
  return pdFALSE;
}

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

void vPortSetStackWatchpoint(void* stack_start) { (void)stack_start; }

#if configSUPPORT_STATIC_ALLOCATION
void vApplicationGetIdleTaskMemory(StaticTask_t** tcb, StackType_t** stack,
                                   uint32_t* stack_size) {
  static StaticTask_t idle_tcb;
  static StackType_t idle_stack[configMINIMAL_STACK_SIZE];
  *tcb = &idle_tcb;
  *stack = idle_stack;
  *stack_size = configMINIMAL_STACK_SIZE;
}
#endif

#if configSUPPORT_STATIC_ALLOCATION && configUSE_TIMERS
void vApplicationGetTimerTaskMemory(StaticTask_t** tcb, StackType_t** stack,
                                    uint32_t* stack_size) {
  static StaticTask_t timer_tcb;
  static StackType_t timer_stack[configTIMER_TASK_STACK_DEPTH];
  *tcb = &timer_tcb;
  *stack = timer_stack;
  *stack_size = configTIMER_TASK_STACK_DEPTH;
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
