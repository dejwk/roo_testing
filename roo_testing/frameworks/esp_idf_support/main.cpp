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
  // On hardware the main task is deleted when app_main returns. For a finite
  // host example, successful return should terminate the process cleanly.
  std::exit(roo_testing::esp32_shims::ShutdownEspTimerForHost() == ESP_OK
                ? EXIT_SUCCESS
                : EXIT_FAILURE);
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
