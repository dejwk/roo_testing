#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "roo_testing/interrupts/interrupt_delivery.h"
#include "roo_testing_port.h"

namespace roo_testing::interrupt_delivery {

bool CanRegister() {
  return xTaskGetSchedulerState() == taskSCHEDULER_RUNNING &&
         xPortIsFreeRtosTask() == pdTRUE && xPortInIsrContext() == pdFALSE;
}

bool InstallDispatcher(void (*dispatch)()) {
  return xPortInstallSimulatedInterruptDispatcher(dispatch);
}

uint32_t Mask() {
  portENTER_CRITICAL(nullptr);
  return 0;
}

void Restore(uint32_t) { portEXIT_CRITICAL(nullptr); }

bool CanDispatch() { return true; }

void RequestDispatch() noexcept { vPortRequestSimulatedInterrupt(); }

}  // namespace roo_testing::interrupt_delivery
