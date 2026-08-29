#include <cstdlib>

#include "Arduino.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "roo_testing/frameworks/esp32_shims/esp_timer_host.h"

namespace {
void RunSketch(void*) {
  if (roo_testing::esp32_shims::InitializeEspTimerForHost() != ESP_OK) {
    std::abort();
  }
  setup();
  for (;;) {
    loop();
    taskYIELD();
  }
}
}  // namespace

int main() {
  initArduino();
  TaskHandle_t task = nullptr;
  if (xTaskCreate(RunSketch, "loopTask", 64 * 1024, nullptr,
                  tskIDLE_PRIORITY + 1, &task) != pdPASS) {
    return 1;
  }
  vTaskStartScheduler();
  return 1;
}
