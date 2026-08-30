#include <cstdlib>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "roo_testing/frameworks/esp32_shims/esp_timer_host.h"

extern "C" void app_main();

namespace {

void RunApp(void*) {
  if (roo_testing::esp32_shims::InitializeEspTimerForHost() != ESP_OK) {
    std::exit(EXIT_FAILURE);
  }
  app_main();
  // Match ESP-IDF: returning from app_main deletes only the main task. The
  // scheduler and peripheral emulation continue running until the host process
  // is closed.
  vTaskDelete(nullptr);
}

}  // namespace

int main() {
  TaskHandle_t task = nullptr;
  if (xTaskCreate(RunApp, "main", 64 * 1024, nullptr, tskIDLE_PRIORITY + 1,
                  &task) != pdPASS) {
    return EXIT_FAILURE;
  }
  vTaskStartScheduler();
  return EXIT_FAILURE;
}
