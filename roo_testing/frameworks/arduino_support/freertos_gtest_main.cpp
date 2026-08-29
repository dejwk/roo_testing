#include <atomic>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gtest/gtest.h"
#include "roo_testing/frameworks/esp32_shims/esp_timer_host.h"
#include "roo_testing/system/timer.h"

namespace {

std::atomic<int> kTestResult{1};

void RunTests(void*) {
  int result = roo_testing::esp32_shims::InitializeEspTimerForHost();
  if (result == ESP_OK) result = RUN_ALL_TESTS();
  if (roo_testing::esp32_shims::ShutdownEspTimerForHost() != ESP_OK) result = 1;
  kTestResult.store(result, std::memory_order_release);
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
