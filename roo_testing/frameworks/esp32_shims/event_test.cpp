#include "esp_event.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "gtest/gtest.h"

ESP_EVENT_DEFINE_BASE(TEST_EVENT);

namespace {

constexpr int32_t kEventId = 1;

struct Capture {
  int calls = 0;
  int value = 0;
  TaskHandle_t handler_task = nullptr;
  SemaphoreHandle_t done = nullptr;
};

void CaptureEvent(void* arg, esp_event_base_t, int32_t, void* data) {
  auto* capture = static_cast<Capture*>(arg);
  ++capture->calls;
  capture->value = *static_cast<int*>(data);
  capture->handler_task = xTaskGetCurrentTaskHandle();
  if (capture->done != nullptr) xSemaphoreGive(capture->done);
}

esp_event_loop_args_t ManualLoopArgs(int queue_size) {
  return {
      .queue_size = queue_size,
      .task_name = nullptr,
      .task_priority = 0,
      .task_stack_size = 0,
      .task_core_id = 0,
  };
}

TEST(EspEventTest, ManualLoopQueuesACopyUntilRun) {
  esp_event_loop_handle_t loop = nullptr;
  auto args = ManualLoopArgs(2);
  ASSERT_EQ(esp_event_loop_create(&args, &loop), ESP_OK);

  Capture capture;
  ASSERT_EQ(esp_event_handler_register_with(loop, TEST_EVENT, kEventId,
                                            CaptureEvent, &capture),
            ESP_OK);

  int payload = 7;
  ASSERT_EQ(esp_event_post_to(loop, TEST_EVENT, kEventId, &payload,
                              sizeof(payload), 0),
            ESP_OK);
  payload = 19;
  EXPECT_EQ(capture.calls, 0);

  EXPECT_EQ(esp_event_loop_run(loop, 0), ESP_OK);
  EXPECT_EQ(capture.calls, 1);
  EXPECT_EQ(capture.value, 7);
  EXPECT_EQ(esp_event_loop_delete(loop), ESP_OK);
}

TEST(EspEventTest, ManualLoopHonorsQueueCapacity) {
  esp_event_loop_handle_t loop = nullptr;
  auto args = ManualLoopArgs(1);
  ASSERT_EQ(esp_event_loop_create(&args, &loop), ESP_OK);

  EXPECT_EQ(esp_event_post_to(loop, TEST_EVENT, kEventId, nullptr, 0, 0),
            ESP_OK);
  EXPECT_EQ(esp_event_post_to(loop, TEST_EVENT, kEventId, nullptr, 0, 0),
            ESP_ERR_TIMEOUT);

  EXPECT_EQ(esp_event_loop_run(loop, 0), ESP_OK);
  EXPECT_EQ(esp_event_loop_delete(loop), ESP_OK);
}

TEST(EspEventTest, DefaultLoopDispatchesOnItsDedicatedTask) {
  ASSERT_EQ(esp_event_loop_create_default(), ESP_OK);

  Capture capture;
  capture.done = xSemaphoreCreateBinary();
  ASSERT_NE(capture.done, nullptr);
  ASSERT_EQ(esp_event_handler_register(TEST_EVENT, kEventId, CaptureEvent,
                                       &capture),
            ESP_OK);

  const TaskHandle_t posting_task = xTaskGetCurrentTaskHandle();
  int payload = 23;
  ASSERT_EQ(esp_event_post(TEST_EVENT, kEventId, &payload, sizeof(payload), 0),
            ESP_OK);
  ASSERT_EQ(xSemaphoreTake(capture.done, pdMS_TO_TICKS(1000)), pdTRUE);

  EXPECT_EQ(capture.calls, 1);
  EXPECT_EQ(capture.value, 23);
  EXPECT_NE(capture.handler_task, posting_task);

  EXPECT_EQ(esp_event_handler_unregister(TEST_EVENT, kEventId, CaptureEvent),
            ESP_OK);
  EXPECT_EQ(esp_event_loop_delete_default(), ESP_OK);
  vSemaphoreDelete(capture.done);
}

}  // namespace
