#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

TaskHandle_t waiting_task;

void NotifyWaitingTask(void*) {
  printf("esp_timer example: callback dispatched; notifying app task\n");
  xTaskNotifyGive(waiting_task);
}

}  // namespace

extern "C" void app_main() {
  printf("esp_timer example: starting one-shot timer for 50 ms\n");
  waiting_task = xTaskGetCurrentTaskHandle();
  const esp_timer_create_args_t timer_args = {
      .callback = NotifyWaitingTask,
      .arg = nullptr,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "example",
      .skip_unhandled_events = false,
  };
  esp_timer_handle_t timer = nullptr;
  ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer));
  ESP_ERROR_CHECK(esp_timer_start_once(timer, 50000));

  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  printf("esp_timer example: notification received; deleting timer\n");
  ESP_ERROR_CHECK(esp_timer_delete(timer));
}
#include <stdio.h>
