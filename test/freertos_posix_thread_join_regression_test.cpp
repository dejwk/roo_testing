#include <atomic>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "gtest/gtest.h"

namespace {

struct JoinLikeState {
  SemaphoreHandle_t finished;
  std::atomic<int> ran_count{0};
};

struct DestructCounter {
  ~DestructCounter() = default;
};

void JoinLikeTask(void* arg) {
  auto* state = static_cast<JoinLikeState*>(arg);
  std::string payload = "join-regression";
  DestructCounter counter;
  (void)payload;
  (void)counter;
  state->ran_count.fetch_add(1, std::memory_order_relaxed);
  xSemaphoreGive(state->finished);
  vTaskSuspend(nullptr);
}

}  // namespace

TEST(FreeRtosPosixRegression, CreateAndJoinLikeRooThreadsSimple) {
  constexpr int kIterations = 20;
  for (int i = 0; i < kIterations; ++i) {
    JoinLikeState state;
    StaticSemaphore_t finished_storage;
    state.finished = xSemaphoreCreateBinaryStatic(&finished_storage);
    ASSERT_NE(state.finished, nullptr);

    TaskHandle_t task = nullptr;
    BaseType_t create_result =
        xTaskCreate(JoinLikeTask, "join_regression",
                    2048 / sizeof(StackType_t), &state,
                    tskIDLE_PRIORITY + 1, &task);
    ASSERT_EQ(create_result, pdPASS);
    ASSERT_NE(task, nullptr);

    BaseType_t wait_result = xSemaphoreTake(state.finished, pdMS_TO_TICKS(2000));
    ASSERT_EQ(wait_result, pdTRUE);
    EXPECT_EQ(state.ran_count.load(std::memory_order_relaxed), 1);

    vTaskDelete(task);
    vSemaphoreDelete(state.finished);
  }
}
