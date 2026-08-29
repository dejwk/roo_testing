#include <atomic>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gtest/gtest.h"
#include "roo_testing/system/timer.h"

extern "C" void app_main();

namespace {

std::atomic<bool> completed{false};

void RunExample(void*) {
  app_main();
  completed.store(true, std::memory_order_release);
  vTaskDelete(nullptr);
}

// Verifies the example unblocks its creating task after timer-task completion.
TEST(EspTimerExampleTest, CompletesAfterManualTimeAdvancement) {
  completed.store(false, std::memory_order_release);
  TaskHandle_t task = nullptr;
  ASSERT_EQ(pdPASS, xTaskCreate(RunExample, "example", 2048, nullptr,
                                tskIDLE_PRIORITY + 1, &task));
  taskYIELD();

  system_time_delay_micros(50000);
  system_time_delay_micros(0);
  taskYIELD();

  EXPECT_TRUE(completed.load(std::memory_order_acquire));
}

}  // namespace
