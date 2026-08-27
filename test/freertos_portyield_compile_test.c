#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void CompilePortYieldFromIsrForms(BaseType_t execute_no_arg) {
  if (execute_no_arg != pdFALSE) {
    portYIELD_FROM_ISR();
  }
  portYIELD_FROM_ISR(pdFALSE);
}
