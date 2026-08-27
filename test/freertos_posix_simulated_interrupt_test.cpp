#include <cerrno>
#include <chrono>
#include <csignal>
#include <thread>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gtest/gtest.h"
#include "roo_testing_port.h"

namespace {

enum class DispatchMode : sig_atomic_t {
  kRecordContext,
  kRerequest,
  kWakeTask,
};

volatile sig_atomic_t dispatch_mode =
    static_cast<sig_atomic_t>(DispatchMode::kRecordContext);
volatile sig_atomic_t handler_count = 0;
volatile sig_atomic_t handler_observed_isr = 0;
volatile sig_atomic_t handler_continued_before_task = 0;
volatile sig_atomic_t awakened_task_ran = 0;
volatile sig_atomic_t awakened_task_observed_isr = 1;
TaskHandle_t task_to_wake = nullptr;

void SimulatedInterruptDispatcher() {
  handler_count = handler_count + 1;
  handler_observed_isr = xPortInIsrContext() == pdTRUE ? 1 : -1;
  if (dispatch_mode != static_cast<sig_atomic_t>(DispatchMode::kWakeTask)) {
    if (dispatch_mode == static_cast<sig_atomic_t>(DispatchMode::kRerequest) &&
        handler_count == 1) {
      vPortRequestSimulatedInterrupt();
    }
    return;
  }

  BaseType_t higher_priority_task_woken = pdFALSE;
  vTaskNotifyGiveFromISR(task_to_wake, &higher_priority_task_woken);
  portYIELD_FROM_ISR(higher_priority_task_woken);
  handler_continued_before_task = awakened_task_ran == 0 ? 1 : -1;
}

bool ResetDispatcher(DispatchMode mode) {
  portENTER_CRITICAL(nullptr);
  dispatch_mode = static_cast<sig_atomic_t>(mode);
  handler_count = 0;
  handler_observed_isr = 0;
  handler_continued_before_task = 0;
  awakened_task_ran = 0;
  awakened_task_observed_isr = 1;
  task_to_wake = nullptr;
  const bool installed =
      xPortInstallSimulatedInterruptDispatcher(SimulatedInterruptDispatcher);
  portEXIT_CRITICAL(nullptr);
  return installed;
}

bool SpinUntil(volatile sig_atomic_t* value, sig_atomic_t expected) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (*value != expected && std::chrono::steady_clock::now() < deadline) {
  }
  return *value == expected;
}

void DelayedInterruptRequest() {
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  vPortRequestSimulatedInterrupt();
}

std::thread StartDelayedInterruptRequester() {
  // Native helper threads must inherit the port's blocked signal mask so the
  // process-directed scheduler tick remains confined to FreeRTOS pthreads.
  portENTER_CRITICAL(nullptr);
  std::thread requester(DelayedInterruptRequest);
  portEXIT_CRITICAL(nullptr);
  return requester;
}

struct WakeTaskState {
  TaskHandle_t test_task;
};

void AwakenedTask(void* arg) {
  WakeTaskState* state = static_cast<WakeTaskState*>(arg);
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  awakened_task_observed_isr = xPortInIsrContext() == pdTRUE ? 1 : 0;
  awakened_task_ran = 1;
  xTaskNotifyGive(state->test_task);
  vTaskSuspend(nullptr);
}

}  // namespace

// Verifies a native interrupt request preempts a task that makes no FreeRTOS
// calls and preserves the interrupted pthread's errno value.
TEST(FreeRtosPosixSimulatedInterrupt, InterruptsCpuBusyTask) {
  ASSERT_TRUE(ResetDispatcher(DispatchMode::kRecordContext));
  constexpr int kErrnoSentinel = EDOM;
  errno = kErrnoSentinel;

  std::thread requester = StartDelayedInterruptRequester();
  const bool handled = SpinUntil(&handler_count, 1);
  const int errno_after_interrupt = errno;
  requester.join();

  EXPECT_TRUE(handled);
  EXPECT_EQ(handler_observed_isr, 1);
  EXPECT_EQ(xPortInIsrContext(), pdFALSE);
  EXPECT_EQ(errno_after_interrupt, kErrnoSentinel);
}

// Verifies the peripheral signal remains pending while a task has interrupts
// masked and is delivered after the outer critical section exits.
TEST(FreeRtosPosixSimulatedInterrupt, CriticalSectionDefersInterrupt) {
  ASSERT_TRUE(ResetDispatcher(DispatchMode::kRecordContext));

  portENTER_CRITICAL(nullptr);
  std::thread requester = StartDelayedInterruptRequester();
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  EXPECT_EQ(handler_count, 0);
  portEXIT_CRITICAL(nullptr);

  const bool handled = SpinUntil(&handler_count, 1);
  requester.join();
  EXPECT_TRUE(handled);
  EXPECT_EQ(handler_observed_isr, 1);
}

// Verifies a request issued by the signal-side dispatcher is drained without
// losing work or recursively entering another dispatcher frame.
TEST(FreeRtosPosixSimulatedInterrupt, DispatcherCanRerequest) {
  ASSERT_TRUE(ResetDispatcher(DispatchMode::kRerequest));

  std::thread requester = StartDelayedInterruptRequester();
  const bool handled_twice = SpinUntil(&handler_count, 2);
  requester.join();

  EXPECT_TRUE(handled_twice);
  EXPECT_EQ(handler_count, 2);
  EXPECT_EQ(handler_observed_isr, 1);
}

// Verifies a FromISR wakeup switches tasks only after the dispatcher finishes
// and that the awakened task resumes outside ISR context.
TEST(FreeRtosPosixSimulatedInterrupt, YieldsAtInterruptExit) {
  ASSERT_TRUE(ResetDispatcher(DispatchMode::kWakeTask));

  WakeTaskState state;
  state.test_task = xTaskGetCurrentTaskHandle();
  TaskHandle_t awakened_task = nullptr;
  ASSERT_EQ(xTaskCreate(AwakenedTask, "irq_awakened", 2048, &state,
                        uxTaskPriorityGet(nullptr) + 1, &awakened_task),
            pdPASS);
  ASSERT_NE(awakened_task, nullptr);

  portENTER_CRITICAL(nullptr);
  task_to_wake = awakened_task;
  portEXIT_CRITICAL(nullptr);

  std::thread requester = StartDelayedInterruptRequester();
  const uint32_t notifications = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000));
  requester.join();

  portENTER_CRITICAL(nullptr);
  task_to_wake = nullptr;
  portEXIT_CRITICAL(nullptr);

  EXPECT_EQ(notifications, 1U);
  EXPECT_EQ(handler_count, 1);
  EXPECT_EQ(handler_continued_before_task, 1);
  EXPECT_EQ(awakened_task_ran, 1);
  EXPECT_EQ(awakened_task_observed_isr, 0);
  EXPECT_EQ(xPortInIsrContext(), pdFALSE);

  vTaskDelete(awakened_task);
}
