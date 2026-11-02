// Destination-owned kernel access through FreeRTOS's supported additions hook.
// Called by native main only after the scheduler and tick thread have stopped.
void RooFreeRtosDeleteFinalTask(void) {
  TaskHandle_t final_task = xTaskGetCurrentTaskHandle();
  if (final_task != NULL) {
    vTaskDelete(final_task);
    pxCurrentTCB = NULL;
  }
}
