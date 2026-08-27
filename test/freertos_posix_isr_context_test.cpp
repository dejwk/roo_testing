#include <csignal>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gtest/gtest.h"

namespace {

volatile sig_atomic_t tick_observed_isr_context = 0;
TaskHandle_t tick_wakeup_task = nullptr;

struct WakeupState {
  TaskHandle_t test_task;
  volatile sig_atomic_t resumed_in_isr = 1;
};

void IsrWakeupTask(void* arg) {
  WakeupState* state = static_cast<WakeupState*>(arg);
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  state->resumed_in_isr = xPortInIsrContext() == pdTRUE ? 1 : 0;
  xTaskNotifyGive(state->test_task);
  vTaskSuspend(nullptr);
}

void CompileCppPortYieldFromIsrForms(bool execute_no_arg) {
  if (execute_no_arg) {
    portYIELD_FROM_ISR();
  }
  portYIELD_FROM_ISR(pdFALSE);
}

}  // namespace

extern "C" void __real_vApplicationTickHook(void);

extern "C" void __wrap_vApplicationTickHook(void) {
  TaskHandle_t task = tick_wakeup_task;
  if (task != nullptr) {
    tick_observed_isr_context =
        xPortInIsrContext() == pdTRUE ? sig_atomic_t{1} : sig_atomic_t{-1};
    BaseType_t higher_priority_task_woken = pdFALSE;
    vTaskNotifyGiveFromISR(task, &higher_priority_task_woken);
  }
  __real_vApplicationTickHook();
}

extern "C" void CompilePortYieldFromIsrForms(BaseType_t execute_no_arg);

// Verifies the signal tick is recognized as an ISR and that its context does
// not leak into a task selected while the signal-handler frame is suspended.
TEST(FreeRtosPosixIsrContext, TickIsAnIsrAcrossTaskSwitch) {
  CompilePortYieldFromIsrForms(pdFALSE);
  CompileCppPortYieldFromIsrForms(false);

  WakeupState state;
  state.test_task = xTaskGetCurrentTaskHandle();
  TaskHandle_t wakeup_task = nullptr;
  ASSERT_EQ(xTaskCreate(IsrWakeupTask, "isr_wakeup", 2048, &state,
                        uxTaskPriorityGet(nullptr) + 1, &wakeup_task),
            pdPASS);
  ASSERT_NE(wakeup_task, nullptr);

  portENTER_CRITICAL(nullptr);
  tick_observed_isr_context = 0;
  tick_wakeup_task = wakeup_task;
  portEXIT_CRITICAL(nullptr);

  const uint32_t notification_count =
      ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000));

  portENTER_CRITICAL(nullptr);
  tick_wakeup_task = nullptr;
  portEXIT_CRITICAL(nullptr);

  EXPECT_EQ(notification_count, 1U);
  EXPECT_EQ(tick_observed_isr_context, 1);
  EXPECT_EQ(state.resumed_in_isr, pdFALSE);
  EXPECT_EQ(xPortInIsrContext(), pdFALSE);

  vTaskDelete(wakeup_task);
}
