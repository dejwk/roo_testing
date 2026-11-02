// Destination-owned access through FreeRTOS's supported kernel additions hook.
void RooFreeRtosDeleteFinalTask(void) {
  TaskHandle_t final_task = xTaskGetCurrentTaskHandle();
  if (final_task != NULL) {
    vTaskDelete(final_task);
    pxCurrentTCB = NULL;
  }
}

// The idle hook runs only when no application task at a higher priority is
// ready. Unsigned subtraction handles a finite deadline in the overflow delayed
// list.
uint32_t RooFreeRtosKernelIdleTicks(void) {
  if (!listLIST_IS_EMPTY(pxDelayedTaskList)) {
    return (uint32_t)(xNextTaskUnblockTime - xTickCount);
  }
  if (!listLIST_IS_EMPTY(pxOverflowDelayedTaskList)) {
    return (
        uint32_t)(listGET_ITEM_VALUE_OF_HEAD_ENTRY(pxOverflowDelayedTaskList) -
                  xTickCount);
  }
  return UINT32_MAX;
}
