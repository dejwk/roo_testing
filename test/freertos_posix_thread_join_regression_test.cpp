#include <atomic>
#include <cerrno>
#include <string>

#include <unistd.h>

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

struct DelayedPipeWriteState {
  int descriptor;
  TickType_t delay;
  SemaphoreHandle_t finished;
  ssize_t result = -1;
  int error = 0;
};

void DelayedPipeWriteTask(void* arg) {
  auto* state = static_cast<DelayedPipeWriteState*>(arg);
  vTaskDelay(state->delay);

  constexpr char kMarker = 'R';
  errno = 0;
  state->result = write(state->descriptor, &kMarker, sizeof(kMarker));
  state->error = errno;
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

TEST(FreeRtosPosixRegression, TickRestartsHostReadAndPreservesErrno) {
  int descriptors[2] = {-1, -1};
  ASSERT_EQ(pipe(descriptors), 0);

  StaticSemaphore_t finished_storage;
  DelayedPipeWriteState state;
  state.descriptor = descriptors[1];
  state.delay = pdMS_TO_TICKS(30);
  state.finished = xSemaphoreCreateBinaryStatic(&finished_storage);
  ASSERT_NE(state.finished, nullptr);

  TaskHandle_t writer = nullptr;
  ASSERT_EQ(xTaskCreate(DelayedPipeWriteTask, "delayed_pipe_write",
                        2048 / sizeof(StackType_t), &state,
                        tskIDLE_PRIORITY + 1, &writer),
            pdPASS);
  ASSERT_NE(writer, nullptr);

  constexpr int kErrnoSentinel = E2BIG;
  const TickType_t start_tick = xTaskGetTickCount();
  errno = kErrnoSentinel;
  char marker = 0;
  const ssize_t read_result = read(descriptors[0], &marker, sizeof(marker));
  const int error_after_read = errno;
  const TickType_t elapsed_ticks = xTaskGetTickCount() - start_tick;

  ASSERT_EQ(xSemaphoreTake(state.finished, pdMS_TO_TICKS(2000)), pdTRUE);
  EXPECT_EQ(state.result, 1) << "write errno: " << state.error;
  EXPECT_EQ(read_result, 1) << "read errno: " << error_after_read;
  EXPECT_EQ(marker, 'R');
  // The writer may start its delay one tick before this task records
  // start_tick, so assert that the read spanned many ticks rather than an
  // exact scheduler boundary.
  EXPECT_GE(elapsed_ticks, pdMS_TO_TICKS(20));
  EXPECT_EQ(error_after_read, kErrnoSentinel);

  vTaskDelete(writer);
  vSemaphoreDelete(state.finished);
  close(descriptors[0]);
  close(descriptors[1]);
}
