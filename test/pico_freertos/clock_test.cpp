#include <atomic>
#include <cstdio>

#include "FreeRTOS.h"
#include "queue.h"
#include "roo_testing/system/timer.h"
#include "runtime.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

namespace {
std::atomic<int> result{1};
SemaphoreHandle_t done;
TaskHandle_t parent;
int64_t origin;
int64_t completed[2];

void WaitWorker(void* arg) {
  const intptr_t index = reinterpret_cast<intptr_t>(arg);
  RooFreeRtosWaitUntil(origin + (index + 1) * 10000);
  completed[index] = system_time_get_micros();
  xSemaphoreGive(done);
  vTaskSuspend(nullptr);
}

void DeletedWaiter(void*) {
  system_time_delay_micros(3600000000ULL);
  configASSERT(false);
}

void Timer(TimerHandle_t) { xSemaphoreGive(done); }

// Verifies overlapping waits, untouched task notifications, kernel timeouts,
// software timers, deletion-safe parking, long gaps, and tick rollover.
void Test(void*) {
  const bool manual = !system_time_is_auto_sync_enabled();
  parent = xTaskGetCurrentTaskHandle();
  done = xSemaphoreCreateCounting(2, 0);
  origin = system_time_get_micros();
  TaskHandle_t workers[2];
  for (intptr_t i = 0; i < 2; ++i) {
    configASSERT(xTaskCreate(WaitWorker, "wait", 8192,
                             reinterpret_cast<void*>(i), 3,
                             &workers[i]) == pdPASS);
  }
  configASSERT(xSemaphoreTake(done, portMAX_DELAY) == pdTRUE);
  configASSERT(xSemaphoreTake(done, portMAX_DELAY) == pdTRUE);
  for (int i = 0; i < 2; ++i) {
    configASSERT(completed[i] >= origin + (i + 1) * 10000);
    if (manual) configASSERT(completed[i] == origin + (i + 1) * 10000);
    vTaskDelete(workers[i]);
  }
  if (manual) configASSERT(system_time_get_micros() == origin + 20000);

  xTaskNotify(parent, 0x123, eSetValueWithOverwrite);
  system_time_delay_micros(50);
  configASSERT(ulTaskNotifyTake(pdTRUE, 0) == 0x123);
  std::atomic<bool> alarm_fired{false};
  ScheduleSystemTimeAlarm(system_time_get_micros() + 75,
                          [&] { alarm_fired = true; });
  system_time_delay_micros(100);
  const int64_t alarm_timeout = system_time_get_micros() + 1000000;
  while (!alarm_fired.load() && system_time_get_micros() < alarm_timeout) {
    system_time_delay_micros(1000);
  }
  configASSERT(alarm_fired.load());

  QueueHandle_t queue = xQueueCreate(1, sizeof(int));
  int value;
  const TickType_t before = xTaskGetTickCount();
  configASSERT(xQueueReceive(queue, &value, 5) == pdFALSE);
  configASSERT(xTaskGetTickCount() - before >= 5);
  vQueueDelete(queue);
  TimerHandle_t timer = xTimerCreate("test", 3, pdFALSE, nullptr, Timer);
  configASSERT(timer != nullptr && xTimerStart(timer, 0) == pdPASS);
  configASSERT(xSemaphoreTake(done, 100) == pdTRUE);
  configASSERT(xTimerDelete(timer, portMAX_DELAY) == pdPASS);

  TaskHandle_t deleted;
  configASSERT(xTaskCreate(DeletedWaiter, "deleted", 8192, nullptr, 3,
                           &deleted) == pdPASS);
  vTaskDelete(deleted);
  system_time_delay_micros(25);
  if (manual) {
    const TickType_t start_tick = xTaskGetTickCount();
    vTaskDelay(UINT32_MAX - 15);
    configASSERT(static_cast<TickType_t>(xTaskGetTickCount() - start_tick) ==
                 UINT32_MAX - 15);
  }
  vSemaphoreDelete(done);
  // Leave one blocked application task for runner shutdown to reclaim.
  configASSERT(xTaskCreate(DeletedWaiter, "shutdown", 8192, nullptr, 3,
                           &deleted) == pdPASS);
  result.store(0);
  RooFreeRtosEndScheduler();
}
}  // namespace

int main() {
  TaskHandle_t task;
  configASSERT(xTaskCreate(Test, "clock_test", 16384, nullptr, 1, &task) ==
               pdPASS);
  vTaskStartScheduler();
  RooFreeRtosDeleteFinalTask();
  while (!TryBeginSystemTimeServiceShutdownForHost()) {
  }
  FinishSystemTimeServiceShutdownForHost();
  std::puts("Pico FreeRTOS shared clock tests passed");
  return result.load();
}
