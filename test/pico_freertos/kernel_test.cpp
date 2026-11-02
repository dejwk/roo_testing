#include <atomic>
#include <chrono>
#include <cstdio>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

extern "C" void RooFreeRtosDeleteFinalTask();
namespace {
std::atomic<int> result{1};
SemaphoreHandle_t ready;
int counter = 0;
std::atomic<bool> preempted{false};

void DelayedWorker(void*) {
  vTaskDelay(2);
  preempted.store(true);
  vTaskSuspend(nullptr);
}

void Worker(void*) {
  ++counter;
  xSemaphoreGive(ready);
  vTaskSuspend(nullptr);
}

// Verifies a higher-priority task preempts the caller and can be reclaimed.
void Test(void*) {
  ready = xSemaphoreCreateBinary();
  TaskHandle_t worker = nullptr;
  configASSERT(xTaskCreate(Worker, "worker", 8192, nullptr, 3, &worker) ==
               pdPASS);
  configASSERT(counter == 1);
  configASSERT(xSemaphoreTake(ready, 100) == pdTRUE);
  vTaskDelete(worker);
  vSemaphoreDelete(ready);
  configASSERT(xTaskCreate(DelayedWorker, "preempt", 8192, nullptr, 3,
                           &worker) == pdPASS);
  const auto timeout =
      std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (!preempted.load()) {
    configASSERT(std::chrono::steady_clock::now() < timeout);
  }
  vTaskDelete(worker);
  const TickType_t before = xTaskGetTickCount();
  vTaskDelay(2);
  configASSERT(xTaskGetTickCount() - before >= 2);
  result.store(0);
  vTaskEndScheduler();
}
}  // namespace

int main() {
  TaskHandle_t task = nullptr;
  if (xTaskCreate(Test, "test", 8192, nullptr, 1, &task) != pdPASS) return 1;
  vTaskStartScheduler();
  RooFreeRtosDeleteFinalTask();
  std::puts("Pico FreeRTOS kernel prototype passed");
  return result.load();
}
