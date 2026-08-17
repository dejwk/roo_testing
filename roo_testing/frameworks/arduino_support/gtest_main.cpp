#include <cstdlib>

#include "Arduino.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gtest/gtest.h"

namespace {
void RunTests(void*) { std::exit(RUN_ALL_TESTS()); }
}  // namespace

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  initArduino();
  TaskHandle_t task = nullptr;
  if (xTaskCreate(RunTests, "gtest", 64 * 1024, nullptr,
                  tskIDLE_PRIORITY + 1, &task) != pdPASS) {
    return 1;
  }
  vTaskStartScheduler();
  return 1;
}
