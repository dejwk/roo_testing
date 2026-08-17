#include <cstdlib>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main();

namespace {

void RunApp(void*) {
  app_main();
  // On hardware the main task is deleted when app_main returns. For a finite
  // host example, successful return should terminate the process cleanly.
  std::exit(EXIT_SUCCESS);
}

}  // namespace

int main() {
  TaskHandle_t task = nullptr;
  if (xTaskCreate(RunApp, "main", 64 * 1024, nullptr,
                  tskIDLE_PRIORITY + 1, &task) != pdPASS) {
    return EXIT_FAILURE;
  }
  vTaskStartScheduler();
  return EXIT_FAILURE;
}
