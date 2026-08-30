#include <cstdlib>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

void FailIfAppMainReturnExitsProcess() { std::_Exit(EXIT_FAILURE); }

void VerifySchedulerContinues(void*) {
  // Bypass the failure-only atexit handler. Reaching this task proves that
  // app_main returned and the ESP-IDF scheduler continued running.
  std::_Exit(EXIT_SUCCESS);
}

}  // namespace

extern "C" void app_main() {
  std::atexit(FailIfAppMainReturnExitsProcess);
  if (xTaskCreate(VerifySchedulerContinues, "verify", configMINIMAL_STACK_SIZE,
                  nullptr, tskIDLE_PRIORITY, nullptr) != pdPASS) {
    std::_Exit(EXIT_FAILURE);
  }
}
