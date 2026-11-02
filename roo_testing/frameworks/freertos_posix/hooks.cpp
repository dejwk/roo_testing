#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <limits>

#include "FreeRTOS.h"
#include "roo_testing/system/timer.h"
#include "runtime.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

namespace {
constexpr int64_t kTickMicros = 1000000 / configTICK_RATE_HZ;
struct Waiter {
  TaskHandle_t task;
  int64_t deadline;
  SemaphoreHandle_t event;
  Waiter* next;
};
struct Resource {
  StaticSemaphore_t storage;
  SemaphoreHandle_t handle = nullptr;
  TaskHandle_t reservation_owner = nullptr;
};
Resource resources[4];
Waiter* waiters = nullptr;
std::atomic<bool> active{false};
int64_t tick_origin_us = 0;
uint64_t processed_ticks = 0;

// Signal masking serializes the active FreeRTOS task and its scheduler ISR.
void RemoveWaiter(Waiter* waiter) {
  Waiter** current = &waiters;
  while (*current != nullptr) {
    if (*current == waiter) {
      *current = waiter->next;
      return;
    }
    current = &(*current)->next;
  }
}
}  // namespace

extern "C" {
bool RooFreeRtosTaskContext() {
  return active.load(std::memory_order_acquire) && RooFreeRtosPortIsTask() &&
         !RooFreeRtosPortIsIsr();
}

void RooFreeRtosRuntimeStarted() {
  tick_origin_us = system_time_get_micros();
  processed_ticks = 0;
  active.store(true, std::memory_order_release);
}

void RooFreeRtosRuntimeStopped() {
  active.store(false, std::memory_order_release);
}

uint32_t RooFreeRtosElapsedTicks() {
  const uint64_t expected =
      (system_time_get_micros() - tick_origin_us) / kTickMicros;
  configASSERT(expected >= processed_ticks);
  const uint32_t count = static_cast<uint32_t>(
      std::min<uint64_t>(expected - processed_ticks, 128));
  processed_ticks += count;
  return count;
}

long RooFreeRtosPollWaiters() {
  BaseType_t higher_priority = pdFALSE;
  const int64_t now = system_time_get_micros();
  for (Waiter* waiter = waiters; waiter != nullptr; waiter = waiter->next) {
    if (waiter->deadline <= now) {
      xSemaphoreGiveFromISR(waiter->event, &higher_priority);
    }
  }
  return higher_priority;
}

void RooFreeRtosTaskDeleted(void* task) {
  // The caller is an active task or native main after the scheduler has
  // stopped.
  portENTER_CRITICAL();
  for (Resource& resource : resources) {
    // FreeRTOS does not release an owner's mutex on task deletion. Require
    // transactions to finish before deletion, rather than leaving a dead bus.
    configASSERT(resource.handle == nullptr ||
                 xSemaphoreGetMutexHolder(resource.handle) != task);
  }
  Waiter** current = &waiters;
  while (*current != nullptr) {
    if ((*current)->task == task) {
      *current = (*current)->next;
    } else {
      current = &(*current)->next;
    }
  }
  portEXIT_CRITICAL();
}

void RooFreeRtosWaitUntil(int64_t deadline_us) {
  configASSERT(RooFreeRtosTaskContext());
  configASSERT(deadline_us <= std::numeric_limits<int64_t>::max() / 1000);
  StaticSemaphore_t storage;
  Waiter waiter{xTaskGetCurrentTaskHandle(), deadline_us,
                xSemaphoreCreateBinaryStatic(&storage), nullptr};
  portENTER_CRITICAL();
  waiter.next = waiters;
  waiters = &waiter;
  portEXIT_CRITICAL();
  // A deadline can expire before take(); the binary semaphore retains that
  // wake.
  while (system_time_get_micros() < deadline_us) {
    xSemaphoreTake(waiter.event, portMAX_DELAY);
  }
  portENTER_CRITICAL();
  RemoveWaiter(&waiter);
  portEXIT_CRITICAL();
  vSemaphoreDelete(waiter.event);
}

void RooFreeRtosIdle() {
  if (system_time_is_auto_sync_enabled()) {
    usleep(1000);
    return;
  }
  int64_t deadline = GetNextSystemTimeAlarmDeadline();
  portENTER_CRITICAL();
  const uint32_t ticks = RooFreeRtosKernelIdleTicks();
  if (ticks != portMAX_DELAY) {
    deadline =
        std::min(deadline, tick_origin_us +
                               static_cast<int64_t>(processed_ticks + ticks) *
                                   kTickMicros);
  }
  for (Waiter* waiter = waiters; waiter != nullptr; waiter = waiter->next) {
    deadline = std::min(deadline, waiter->deadline);
  }
  const int64_t now = system_time_get_micros();
  if (deadline != std::numeric_limits<int64_t>::max() && deadline > now) {
    // Skip only ticks before the next kernel wake and before counter rollover.
    // The final tick still goes through xTaskIncrementTick and wakes tasks.
    const uint64_t target_ticks = (deadline - tick_origin_us) / kTickMicros;
    const uint64_t available = target_ticks - processed_ticks;
    const uint64_t until_wrap = portMAX_DELAY - xTaskGetTickCount();
    const uint32_t skip = static_cast<uint32_t>(std::min(
        available,
        std::min<uint64_t>(ticks == portMAX_DELAY ? until_wrap : ticks - 1,
                           until_wrap == 0 ? 0 : until_wrap - 1)));
    if (skip != 0) {
      vTaskStepTick(skip);
      processed_ticks += skip;
    }
    // Stop at the rollover boundary if it precedes the actual wake deadline.
    const int64_t wrap_deadline =
        tick_origin_us +
        static_cast<int64_t>(processed_ticks + 1) * kTickMicros;
    if (until_wrap <= available) deadline = std::min(deadline, wrap_deadline);
  }
  // Publish a skipped kernel interval and its clock atomically with respect to
  // the ticker signal; otherwise an ISR could observe ticks ahead of uptime.
  if (deadline != std::numeric_limits<int64_t>::max() && deadline > now) {
    system_time_lag_ns(static_cast<uint64_t>(deadline - now) * 1000);
    ProcessSystemTimeAlarms();
  }
  portEXIT_CRITICAL();
  if (deadline == std::numeric_limits<int64_t>::max()) {
    usleep(1000);
  } else {
    RooFreeRtosPortRequestTick();
  }
}

void vApplicationIdleHook() { RooFreeRtosIdle(); }

void vApplicationMallocFailedHook() { std::abort(); }

bool roo_testing_runtime_delay_micros(uint64_t us) {
  if (!RooFreeRtosTaskContext()) return false;
  const int64_t now = system_time_get_micros();
  configASSERT(us <= static_cast<uint64_t>(
                         std::numeric_limits<int64_t>::max() / 1000 - now));
  RooFreeRtosWaitUntil(now + static_cast<int64_t>(us));
  return true;
}

bool roo_testing_runtime_sync() {
  if (!RooFreeRtosTaskContext()) return false;
  taskYIELD();
  return true;
}

void roo_testing_runtime_time_advanced() {
  if (active.load(std::memory_order_acquire)) RooFreeRtosPortRequestTick();
}

void RooFreeRtosLockResource(uint32_t resource) {
  if (!RooFreeRtosTaskContext()) return;
  configASSERT(resource < 4);
  Resource& state = resources[resource];
  portENTER_CRITICAL();
  if (state.handle == nullptr) {
    state.handle = xSemaphoreCreateRecursiveMutexStatic(&state.storage);
  }
  portEXIT_CRITICAL();
  configASSERT(xSemaphoreTakeRecursive(state.handle, portMAX_DELAY) == pdTRUE);
}

void RooFreeRtosUnlockResource(uint32_t resource) {
  if (!RooFreeRtosTaskContext()) return;
  configASSERT(resource < 4);
  configASSERT(xSemaphoreGiveRecursive(resources[resource].handle) == pdTRUE);
}

void RooFreeRtosReserveResource(uint32_t resource) {
  if (!RooFreeRtosTaskContext()) return;
  RooFreeRtosLockResource(resource);
  Resource& state = resources[resource];
  if (state.reservation_owner == xTaskGetCurrentTaskHandle()) {
    RooFreeRtosUnlockResource(resource);
  } else {
    state.reservation_owner = xTaskGetCurrentTaskHandle();
  }
}

void RooFreeRtosReleaseResource(uint32_t resource) {
  if (!RooFreeRtosTaskContext()) return;
  configASSERT(resource < 4);
  Resource& state = resources[resource];
  if (state.reservation_owner == nullptr) return;
  configASSERT(state.reservation_owner == xTaskGetCurrentTaskHandle());
  state.reservation_owner = nullptr;
  RooFreeRtosUnlockResource(resource);
}

void RooFreeRtosEndScheduler() {
  // User tasks can still be blocked in SDK waits when a test/sketch exits.
  // Delete them while the scheduler can safely unlink their wait records.
  const UBaseType_t capacity = uxTaskGetNumberOfTasks();
  TaskStatus_t* tasks =
      static_cast<TaskStatus_t*>(pvPortMalloc(sizeof(TaskStatus_t) * capacity));
  configASSERT(tasks != nullptr);
  const UBaseType_t count = uxTaskGetSystemState(tasks, capacity, nullptr);
  const TaskHandle_t self = xTaskGetCurrentTaskHandle();
  const TaskHandle_t idle = xTaskGetIdleTaskHandle();
  const TaskHandle_t timer = xTimerGetTimerDaemonTaskHandle();
  for (UBaseType_t i = 0; i < count; ++i) {
    TaskHandle_t task = tasks[i].xHandle;
    if (task != self && task != idle && task != timer) vTaskDelete(task);
  }
  vPortFree(tasks);
  vTaskEndScheduler();
}
}
