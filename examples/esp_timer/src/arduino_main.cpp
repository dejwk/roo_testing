#include <Arduino.h>

#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

TaskHandle_t waiting_task;
esp_timer_handle_t timer;
bool completed = false;

void NotifyWaitingTask(void*) {
  Serial.println("esp_timer example: callback dispatched; notifying loop task");
  xTaskNotifyGive(waiting_task);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println("esp_timer example: starting one-shot timer for 50 ms");
  waiting_task = xTaskGetCurrentTaskHandle();
  const esp_timer_create_args_t timer_args = {
      .callback = NotifyWaitingTask,
      .arg = nullptr,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "example",
      .skip_unhandled_events = false,
  };
  ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer));
  ESP_ERROR_CHECK(esp_timer_start_once(timer, 50000));
}

void loop() {
  if (!completed) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    Serial.println("esp_timer example: notification received; deleting timer");
    ESP_ERROR_CHECK(esp_timer_delete(timer));
    completed = true;
  }
  delay(1000);
}
