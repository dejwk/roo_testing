#include "Arduino.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
void RunSketch(void*) {
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
