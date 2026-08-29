#include <atomic>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gtest/gtest.h"
#include "roo_testing/system/timer.h"

namespace {

std::atomic<int> kTestResult{1};

void RunTests(void*) {
  kTestResult.store(RUN_ALL_TESTS(), std::memory_order_release);
  while (!TryBeginSystemTimeServiceShutdownForHost()) {
    vTaskDelay(1);
  }
  vTaskEndScheduler();
}

}  // namespace

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  TaskHandle_t task = nullptr;
  if (xTaskCreate(RunTests, "gtest", 64 * 1024, nullptr, tskIDLE_PRIORITY + 1,
                  &task) != pdPASS) {
    return 1;
  }
  vTaskStartScheduler();
  const int test_result = kTestResult.load(std::memory_order_acquire);
  FinishSystemTimeServiceShutdownForHost();
  return test_result;
}
