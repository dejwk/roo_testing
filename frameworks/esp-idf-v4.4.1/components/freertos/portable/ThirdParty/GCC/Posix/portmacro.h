/*
 * FreeRTOS Kernel V10.4.6
 * Copyright 2020 Cambridge Consultants Ltd.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */


#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>
#include <stdbool.h>

#include "soc/spinlock.h"

// #include "hal/cpu_hal.h"

/*-----------------------------------------------------------
 * Port specific definitions.
 *
 * The settings in this file configure FreeRTOS correctly for the
 * given hardware and compiler.
 *
 * These settings should not be altered.
 *-----------------------------------------------------------
 */

/* Type definitions. */
#define portCHAR		char
#define portFLOAT		float
#define portDOUBLE		double
#define portLONG		long
#define portSHORT		short
#define portSTACK_TYPE	unsigned long
#define portBASE_TYPE	long
#define portPOINTER_SIZE_TYPE intptr_t

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

typedef unsigned long TickType_t;
#define portMAX_DELAY ( TickType_t ) ULONG_MAX

#define portTICK_TYPE_IS_ATOMIC 1

/*-----------------------------------------------------------*/

/* Architecture specifics. */
#define portSTACK_GROWTH			( -1 )
#define portHAS_STACK_OVERFLOW_CHECKING	( 1 )
#define portTICK_PERIOD_MS			( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portTICK_RATE_MICROSECONDS	( ( portTickType ) 1000000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT			8
/*-----------------------------------------------------------*/

/* Scheduler utilities. */
extern void vPortYield( void );

bool xPortCanYield(void);

#define portYIELD() vPortYield()

#define portEND_SWITCHING_ISR( xSwitchRequired ) if( xSwitchRequired != pdFALSE ) vPortYield()
// #define portYIELD_FROM_ISR( x ) portEND_SWITCHING_ISR( x )
#define portYIELD_FROM_ISR() portEND_SWITCHING_ISR(true)
/*-----------------------------------------------------------*/

typedef spinlock_t                          portMUX_TYPE;               /**< Spinlock type used by FreeRTOS critical sections */
#define portMUX_INITIALIZER_UNLOCKED        SPINLOCK_INITIALIZER        /**< Spinlock initializer */
#define portMUX_FREE_VAL                    SPINLOCK_FREE               /**< Spinlock is free. [refactor-todo] check if this is still required */
#define portMUX_NO_TIMEOUT                  SPINLOCK_WAIT_FOREVER       /**< When passed for 'timeout_cycles', spin forever if necessary. [refactor-todo] check if this is still required */
#define portMUX_TRY_LOCK                    SPINLOCK_NO_WAIT            /**< Try to acquire the spinlock a single time only. [refactor-todo] check if this is still required */
#define portMUX_INITIALIZE(mux)             spinlock_initialize(mux)    /*< Initialize a spinlock to its unlocked state */

/* Critical section management. */
extern void vPortDisableInterrupts( void );
extern void vPortEnableInterrupts( void );
#define portSET_INTERRUPT_MASK()        ( vPortDisableInterrupts() )
#define portCLEAR_INTERRUPT_MASK()      ( vPortEnableInterrupts() )

extern portBASE_TYPE xPortSetInterruptMask( void );
extern void vPortClearInterruptMask( portBASE_TYPE xMask );

extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );
#define portSET_INTERRUPT_MASK_FROM_ISR()		xPortSetInterruptMask()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)	vPortClearInterruptMask(x)
#define portDISABLE_INTERRUPTS()				portSET_INTERRUPT_MASK()
#define portENABLE_INTERRUPTS()					portCLEAR_INTERRUPT_MASK()
#define portENTER_CRITICAL(mux)					vPortEnterCritical()
#define portEXIT_CRITICAL(mux)						vPortExitCritical()

#define portTRY_ENTER_CRITICAL_ISR(mux, timeout)    xPortEnterCritical()
#define portENTER_CRITICAL_ISR(mux)                 vPortEnterCritical()
#define portEXIT_CRITICAL_ISR(mux)                  vPortExitCritical()

#define portENTER_CRITICAL_SAFE(mux)                 vPortEnterCritical()
#define portEXIT_CRITICAL_SAFE(mux)                  vPortExitCritical()

/*-----------------------------------------------------------*/

extern void vPortThreadDying( void *pxTaskToDelete, volatile BaseType_t *pxPendYield );
extern void vPortCancelThread( void *pxTaskToDelete );
#define portPRE_TASK_DELETE_HOOK( pvTaskToDelete, pxPendYield ) vPortThreadDying( ( pvTaskToDelete ), ( pxPendYield ) )
#define portCLEAN_UP_TCB( pxTCB )	vPortCancelThread( pxTCB )
/*-----------------------------------------------------------*/

#define portTASK_FUNCTION_PROTO( vFunction, pvParameters ) void vFunction( void *pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters ) void vFunction( void *pvParameters )
/*-----------------------------------------------------------*/

/*
 * Tasks run in their own pthreads and context switches between them
 * are always a full memory barrier. ISRs are emulated as signals
 * which also imply a full memory barrier.
 *
 * Thus, only a compilier barrier is needed to prevent the compiler
 * reordering.
 */
#define portMEMORY_BARRIER() __asm volatile( "" ::: "memory" )

extern unsigned long ulPortGetRunTime( void );
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() /* no-op */
#define portGET_RUN_TIME_COUNTER_VALUE()         ulPortGetRunTime()

#define portMAX_DELAY ( TickType_t ) 0xffffffffUL

#define _lock_t SemaphoreHandle_t

#define portMUX_TYPE spinlock_t

// ----------------------- System --------------------------

static inline BaseType_t xPortGetCoreID(void)
{
    return 1;
}

#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */
