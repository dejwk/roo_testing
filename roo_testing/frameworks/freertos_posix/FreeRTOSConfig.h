#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "runtime.h"
#define portCHECK_IF_IN_ISR() RooFreeRtosPortIsIsr()

#define configNUMBER_OF_CORES 1
#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configCPU_CLOCK_HZ 125000000UL
#define configTICK_RATE_HZ 1000
#define configMAX_PRIORITIES 8
#define configMINIMAL_STACK_SIZE 8192
#define configMAX_TASK_NAME_LEN 16
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1
#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 1
#define configUSE_COUNTING_SEMAPHORES 1
#define configUSE_QUEUE_SETS 1
#define configQUEUE_REGISTRY_SIZE 8
#define configUSE_TASK_NOTIFICATIONS 1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES 4
#define configSUPPORT_STATIC_ALLOCATION 1
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configUSE_TIMERS 1
#define configTIMER_TASK_PRIORITY 2
#define configTIMER_QUEUE_LENGTH 16
#define configTIMER_TASK_STACK_DEPTH 8192
#define configUSE_TRACE_FACILITY 1
#define configUSE_STATS_FORMATTING_FUNCTIONS 0
#define configUSE_APPLICATION_TASK_TAG 1
#define configUSE_IDLE_HOOK 1
#define configUSE_TICK_HOOK 0
#define configUSE_TICKLESS_IDLE 2
#define portSUPPRESS_TICKS_AND_SLEEP(ticks) ((void)(ticks))
#define configUSE_MALLOC_FAILED_HOOK 1
#define configCHECK_FOR_STACK_OVERFLOW 0
#define configUSE_NEWLIB_REENTRANT 0
#define configUSE_CO_ROUTINES 0
#define configSTACK_DEPTH_TYPE uint32_t
#define configKERNEL_PROVIDED_STATIC_MEMORY 1
#define configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H 1
#define INCLUDE_vTaskDelete 1
#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskDelayUntil 1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_uxTaskPriorityGet 1
#define INCLUDE_vTaskPrioritySet 1
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define INCLUDE_xTaskGetIdleTaskHandle 1
#define INCLUDE_eTaskGetState 1
#define INCLUDE_xTimerPendFunctionCall 1
#define INCLUDE_xTaskAbortDelay 1
#define INCLUDE_xSemaphoreGetMutexHolder 1
#define configASSERT(x)                                                 \
  do {                                                                  \
    if (!(x)) {                                                         \
      fprintf(stderr, "FreeRTOS assertion: %s (%s:%d)\n", #x, __FILE__, \
              __LINE__);                                                \
      abort();                                                          \
    }                                                                   \
  } while (0)
