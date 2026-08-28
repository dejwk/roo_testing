/*
 * FreeRTOS Kernel V10.5.1 (ESP-IDF SMP modified)
 * Copyright (C) 2020 Cambridge Consultants Ltd.
 *
 * SPDX-FileCopyrightText: 2020 Cambridge Consultants Ltd
 *
 * SPDX-License-Identifier: MIT
 *
 * SPDX-FileContributor: 2023-2025 Espressif Systems (Shanghai) CO LTD
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

/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the Posix port.
 *
 * Each task has a pthread which eases use of standard debuggers
 * (allowing backtraces of tasks etc). Threads for tasks that are not
 * running are blocked in sigwait().
 *
 * Task switch is done by resuming the thread for the next task by
 * signaling the condition variable and then waiting on a condition variable
 * with the current thread.
 *
 * The timer interrupt uses SIGALRM and care is taken to ensure that
 * the signal handler runs only on the thread for the current task.
 *
 * Use of part of the standard C library requires care as some
 * functions can take pthread mutexes internally which can result in
 * deadlocks as the FreeRTOS kernel can switch tasks while they're
 * holding a pthread mutex.
 *
 * stdio (printf() and friends) should be called from a single task
 * only or serialized with a FreeRTOS primitive such as a binary
 * semaphore or mutex.
 *----------------------------------------------------------*/

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/times.h>
#include <time.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "roo_testing_port.h"
#include "task.h"
#include "timers.h"
#include "utils/wait_for_event.h"
/*-----------------------------------------------------------*/

#define SIG_RESUME SIGUSR1
#define SIG_SIMULATED_INTERRUPT SIGUSR2

typedef struct THREAD
{
    pthread_t pthread;
    TaskFunction_t pxCode;
    void *pvParams;
    BaseType_t xDying;
    struct event *ev;
} Thread_t;

/*
 * The additional per-thread data is stored at the beginning of the
 * task's stack.
 */
static inline Thread_t *prvGetThreadFromTask(TaskHandle_t xTask)
{
    StackType_t *pxTopOfStack = *(StackType_t **)xTask;

    return (Thread_t *)(pxTopOfStack + 1);
}

/*-----------------------------------------------------------*/

static pthread_once_t hSigSetupThread = PTHREAD_ONCE_INIT;
static sigset_t xAllSignals;
static sigset_t xSchedulerOriginalSignalMask;
static pthread_t hMainThread = ( pthread_t )NULL;
static pthread_t hRunningThread = ( pthread_t )NULL;
/*
 * These values are accessed both by task code and by asynchronous signal
 * handlers. volatile sig_atomic_t is the C representation that guarantees
 * indivisible reads and writes in both contexts without requiring a lock.
 *
 * uxCriticalNesting is the logical critical-section/ISR exclusion depth for
 * the active FreeRTOS context. Direct interrupt masking does not change it.
 * uxInterruptNesting is only the active ISR-frame depth; an ordinary task
 * critical section therefore changes the former without changing the latter.
 */
static volatile sig_atomic_t uxCriticalNesting;
static volatile sig_atomic_t uxInterruptNesting;
/* A boolean request consumed after the outermost signal-side ISR returns. */
static volatile sig_atomic_t xYieldPending;
static volatile sig_atomic_t xSimulatedInterruptPending;
static RooTestingInterruptDispatcher pxSimulatedInterruptDispatcher;

_Static_assert( __atomic_always_lock_free( sizeof( hRunningThread ), 0 ),
                "simulated interrupt pthread publication must be lock-free" );
_Static_assert( __atomic_always_lock_free( sizeof( xSimulatedInterruptPending ), 0 ),
                "simulated interrupt pending state must be lock-free" );
_Static_assert( __atomic_always_lock_free( sizeof( pxSimulatedInterruptDispatcher ), 0 ),
                "simulated interrupt dispatcher publication must be lock-free" );
/*-----------------------------------------------------------*/

static BaseType_t xSchedulerEnd = pdFALSE;
/*-----------------------------------------------------------*/

static void prvSetupSignalsAndSchedulerPolicy( void );
static void prvSetupTimerInterrupt( void );
static void *prvWaitForStart( void * pvParams );
static void prvSwitchThread( Thread_t * xThreadToResume,
                             Thread_t *xThreadToSuspend );
static void prvSuspendSelf( Thread_t * thread);
static void prvResumeThread( Thread_t * xThreadId );
static void vPortSystemTickHandler( int sig );
static void vPortSimulatedInterruptHandler( int sig );
static void vPortStartFirstTask( void );
static void prvEnterInterruptContext( void );
static void prvExitInterruptContext( void );
static void prvYieldPendingFromInterruptExit( void );
static void prvKickSimulatedInterrupt( void );
/*-----------------------------------------------------------*/

static void prvFatalError( const char *pcCall, int iErrno )
{
    fprintf( stderr, "%s: %s\n", pcCall, strerror( iErrno ) );
    abort();
}

/*
 * See header file for description.
 */
StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack,
                                    StackType_t *pxEndOfStack,
                                    TaskFunction_t pxCode,
                                    void *pvParameters )
{
    Thread_t *thread;
    pthread_attr_t xThreadAttributes;
    size_t ulStackSize;
    int iRet;

    (void)pthread_once( &hSigSetupThread, prvSetupSignalsAndSchedulerPolicy );

    /*
     * Store the additional thread data at the start of the stack.
     */
    thread = (Thread_t *)(pxTopOfStack + 1) - 1;
    pxTopOfStack = (StackType_t *)thread - 1;
    ulStackSize = (pxTopOfStack + 1 - pxEndOfStack) * sizeof(*pxTopOfStack);

    thread->pxCode = pxCode;
    thread->pvParams = pvParameters;
    thread->xDying = pdFALSE;

    pthread_attr_init( &xThreadAttributes );
    pthread_attr_setstack( &xThreadAttributes, pxEndOfStack, ulStackSize );

    thread->ev = event_create();

    vPortEnterCritical();

    iRet = pthread_create( &thread->pthread, &xThreadAttributes,
                           prvWaitForStart, thread );
    if ( iRet )
    {
        prvFatalError( "pthread_create", iRet );
    }

    vPortExitCritical();

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

void vPortStartFirstTask( void )
{
    Thread_t *pxFirstThread = prvGetThreadFromTask( xTaskGetCurrentTaskHandle() );

    /* Start the first task. */
    prvResumeThread( pxFirstThread );
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
BaseType_t xPortStartScheduler( void )
{
    int iSignal;
    sigset_t xSignals;

    hMainThread = pthread_self();

    /* Start the timer that generates the tick ISR(SIGALRM).
       Interrupts are disabled here already. */
    prvSetupTimerInterrupt();

    /* Start the first task. */
    vPortStartFirstTask();

    /* Wait until signaled by vPortEndScheduler(). */
    sigemptyset( &xSignals );
    sigaddset( &xSignals, SIG_RESUME );

    while ( !xSchedulerEnd )
    {
        sigwait( &xSignals, &iSignal );
    }

    /* Cancel the Idle task and free its resources */
#if ( INCLUDE_xTaskGetIdleTaskHandle == 1 )
    vPortCancelThread( xTaskGetIdleTaskHandle() );
#endif

#if ( configUSE_TIMERS == 1 )
    /* Cancel the Timer task and free its resources */
    vPortCancelThread( xTimerGetTimerDaemonTaskHandle() );
#endif /* configUSE_TIMERS */

    /* Restore original signal mask. */
    (void)pthread_sigmask( SIG_SETMASK, &xSchedulerOriginalSignalMask,  NULL );

    return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    struct itimerval itimer;
    struct sigaction sigignore;
    Thread_t *xCurrentThread;

    /* Stop the timer and ignore any pending SIGALRMs that would end
     * up running on the main thread when it is resumed. */
    itimer.it_value.tv_sec = 0;
    itimer.it_value.tv_usec = 0;

    itimer.it_interval.tv_sec = 0;
    itimer.it_interval.tv_usec = 0;
    (void)setitimer( ITIMER_REAL, &itimer, NULL );

    sigignore.sa_flags = 0;
    sigignore.sa_handler = SIG_IGN;
    sigemptyset( &sigignore.sa_mask );
    sigaction( SIGALRM, &sigignore, NULL );

    __atomic_store_n( &xSimulatedInterruptPending, 0, __ATOMIC_RELEASE );
    __atomic_store_n( &hRunningThread, ( pthread_t )NULL, __ATOMIC_RELEASE );
    sigaction( SIG_SIMULATED_INTERRUPT, &sigignore, NULL );

    /* Signal the scheduler to exit its loop. */
    xSchedulerEnd = pdTRUE;
    (void)pthread_kill( hMainThread, SIG_RESUME );

    xCurrentThread = prvGetThreadFromTask( xTaskGetCurrentTaskHandle() );
    prvSuspendSelf(xCurrentThread);
}
/*-----------------------------------------------------------*/

void vPortEnterCritical( void )
{
    if ( uxCriticalNesting == 0 )
    {
        vPortDisableInterrupts();
    }
    uxCriticalNesting++;
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
    if ( uxCriticalNesting > 0 )
    {
        uxCriticalNesting--;
    }

    /* Critical section nesting count must always be >= 0. */
    configASSERT( uxCriticalNesting >= 0 );

    /* If we have reached 0 then re-enable the interrupts. */
    if( uxCriticalNesting == 0 )
    {
        vPortEnableInterrupts();
    }
}
/*-----------------------------------------------------------*/

BaseType_t xPortIsFreeRtosTask( void )
{
    TaskHandle_t xTask = xTaskGetCurrentTaskHandle();

    if( xTask == NULL )
    {
        return pdFALSE;
    }

    return pthread_equal( pthread_self(), prvGetThreadFromTask( xTask )->pthread )
           ? pdTRUE : pdFALSE;
}
/*-----------------------------------------------------------*/

bool xPortCanYield( void )
{
    return uxCriticalNesting == 0;
}
/*-----------------------------------------------------------*/

BaseType_t xPortCheckIfInISR( void )
{
    return ( uxInterruptNesting == 0 ) ? pdFALSE : pdTRUE;
}
/*-----------------------------------------------------------*/

void vPortYieldFromISR( void )
{
    Thread_t *xThreadToSuspend;
    Thread_t *xThreadToResume;

    if( uxInterruptNesting > 0 )
    {
        xYieldPending = pdTRUE;
        return;
    }

    xThreadToSuspend = prvGetThreadFromTask( xTaskGetCurrentTaskHandle() );

    vTaskSwitchContext();

    xThreadToResume = prvGetThreadFromTask( xTaskGetCurrentTaskHandle() );

    prvSwitchThread( xThreadToResume, xThreadToSuspend );
}
/*-----------------------------------------------------------*/

void vPortYield( void )
{
    vPortEnterCritical();

    vPortYieldFromISR();

    vPortExitCritical();
}
/*-----------------------------------------------------------*/

void vPortDisableInterrupts( void )
{
    pthread_sigmask( SIG_BLOCK, &xAllSignals, NULL );
}
/*-----------------------------------------------------------*/

void vPortEnableInterrupts( void )
{
    pthread_sigmask( SIG_UNBLOCK, &xAllSignals, NULL );
}
/*-----------------------------------------------------------*/

BaseType_t xPortSetInterruptMask( void )
{
    /* Interrupts are always disabled inside ISRs (signals
       handlers). */
    return pdTRUE;
}
/*-----------------------------------------------------------*/

void vPortClearInterruptMask( BaseType_t xMask )
{
}
/*-----------------------------------------------------------*/

static uint64_t prvGetTimeNs(void)
{
    struct timespec t;

    clock_gettime(CLOCK_MONOTONIC, &t);

    return t.tv_sec * 1000000000ull + t.tv_nsec;
}

static uint64_t prvStartTimeNs;
/* commented as part of the code below in vPortSystemTickHandler,
 * to adjust timing according to full demo requirements */
/* static uint64_t prvTickCount; */

static void prvEnterInterruptContext( void )
{
    /* Signals are masked while their handler runs. Keep the existing
     * critical-nesting accounting separate from the ISR-context query. */
    uxCriticalNesting++;
    uxInterruptNesting++;
}
/*-----------------------------------------------------------*/

static void prvExitInterruptContext( void )
{
    configASSERT( uxInterruptNesting > 0 );
    configASSERT( uxCriticalNesting > 0 );
    uxInterruptNesting--;
    uxCriticalNesting--;
}
/*-----------------------------------------------------------*/

static void prvYieldPendingFromInterruptExit( void )
{
    if( ( uxInterruptNesting == 0 ) && ( xYieldPending != pdFALSE ) )
    {
        xYieldPending = pdFALSE;
        vPortYieldFromISR();
    }
}
/*-----------------------------------------------------------*/

static void prvKickSimulatedInterrupt( void )
{
    pthread_t hThread;

    if( __atomic_load_n( &xSimulatedInterruptPending, __ATOMIC_ACQUIRE ) == 0 )
    {
        return;
    }

    hThread = __atomic_load_n( &hRunningThread, __ATOMIC_ACQUIRE );
    if( hThread != ( pthread_t )NULL )
    {
        ( void ) pthread_kill( hThread, SIG_SIMULATED_INTERRUPT );
    }
}
/*-----------------------------------------------------------*/

bool xPortInstallSimulatedInterruptDispatcher( RooTestingInterruptDispatcher dispatcher )
{
    RooTestingInterruptDispatcher expected = NULL;

    if( dispatcher == NULL )
    {
        return false;
    }

    if( __atomic_compare_exchange_n( &pxSimulatedInterruptDispatcher, &expected,
                                     dispatcher, false, __ATOMIC_RELEASE,
                                     __ATOMIC_ACQUIRE ) )
    {
        prvKickSimulatedInterrupt();
        return true;
    }

    return expected == dispatcher;
}
/*-----------------------------------------------------------*/

void vPortRequestSimulatedInterrupt( void )
{
    __atomic_store_n( &xSimulatedInterruptPending, 1, __ATOMIC_RELEASE );
    prvKickSimulatedInterrupt();
}
/*-----------------------------------------------------------*/

/*
 * Setup the systick timer to generate the tick interrupts at the required
 * frequency.
 */
void prvSetupTimerInterrupt( void )
{
    struct itimerval itimer;
    int iRet;

    /* Initialise the structure with the current timer information. */
    iRet = getitimer( ITIMER_REAL, &itimer );
    if ( iRet )
    {
        prvFatalError( "getitimer", errno );
    }

    /* Set the interval between timer events. */
    itimer.it_interval.tv_sec = 0;
    itimer.it_interval.tv_usec = portTICK_RATE_MICROSECONDS;

    /* Set the current count-down. */
    itimer.it_value.tv_sec = 0;
    itimer.it_value.tv_usec = portTICK_RATE_MICROSECONDS;

    /* Set-up the timer interrupt. */
    iRet = setitimer( ITIMER_REAL, &itimer, NULL );
    if ( iRet )
    {
        prvFatalError( "setitimer", errno );
    }

    prvStartTimeNs = prvGetTimeNs();
}
/*-----------------------------------------------------------*/

static void vPortSystemTickHandler( int sig )
{
    const int iSavedErrno = errno;
    Thread_t *pxThreadToSuspend;
    Thread_t *pxThreadToResume;
    /* uint64_t xExpectedTicks; */

    prvEnterInterruptContext();

#if ( configUSE_PREEMPTION == 1 )
    pxThreadToSuspend = prvGetThreadFromTask( xTaskGetCurrentTaskHandle() );
#endif

    /* Tick Increment, accounting for any lost signals or drift in
     * the timer. */
/*
 *      Comment code to adjust timing according to full demo requirements
 *      xExpectedTicks = (prvGetTimeNs() - prvStartTimeNs)
 *        / (portTICK_RATE_MICROSECONDS * 1000);
 * do { */
        xTaskIncrementTick();
/*        prvTickCount++;
 *    } while (prvTickCount < xExpectedTicks);
*/

#if ( configUSE_PREEMPTION == 1 )
    /* Select Next Task. */
    xYieldPending = pdFALSE;
    vTaskSwitchContext();

    pxThreadToResume = prvGetThreadFromTask( xTaskGetCurrentTaskHandle() );

    prvSwitchThread(pxThreadToResume, pxThreadToSuspend);
#endif

    prvExitInterruptContext();
    prvYieldPendingFromInterruptExit();
    errno = iSavedErrno;
}
/*-----------------------------------------------------------*/

static void vPortSimulatedInterruptHandler( int sig )
{
    const int iSavedErrno = errno;
    RooTestingInterruptDispatcher dispatcher;

    ( void ) sig;
    dispatcher = __atomic_load_n( &pxSimulatedInterruptDispatcher,
                                  __ATOMIC_ACQUIRE );
    if( dispatcher == NULL )
    {
        errno = iSavedErrno;
        return;
    }

    if( __atomic_exchange_n( &xSimulatedInterruptPending, 0,
                             __ATOMIC_ACQ_REL ) == 0 )
    {
        errno = iSavedErrno;
        return;
    }

    prvEnterInterruptContext();
    do
    {
        dispatcher();
    } while( __atomic_exchange_n( &xSimulatedInterruptPending, 0,
                                  __ATOMIC_ACQ_REL ) != 0 );
    prvExitInterruptContext();
    prvYieldPendingFromInterruptExit();
    errno = iSavedErrno;
}
/*-----------------------------------------------------------*/

void vPortThreadDying( void *pxTaskToDelete, volatile BaseType_t *pxPendYield )
{
    Thread_t *pxThread = prvGetThreadFromTask( pxTaskToDelete );

    pxThread->xDying = pdTRUE;
}

void vPortCancelThread( void *pxTaskToDelete )
{
    #if ( CONFIG_FREERTOS_TASK_PRE_DELETION_HOOK )
        /* Call the user defined task pre-deletion hook before canceling the thread */
        extern void vTaskPreDeletionHook( void * pxTCB );
        vTaskPreDeletionHook( pxTaskToDelete );
    #endif /* CONFIG_FREERTOS_TASK_PRE_DELETION_HOOK */

    Thread_t *pxThreadToCancel = prvGetThreadFromTask( pxTaskToDelete );

    /*
     * The thread has already been suspended so it can be safely cancelled.
     */
    pthread_cancel( pxThreadToCancel->pthread );
    pthread_join( pxThreadToCancel->pthread, NULL );
    event_delete( pxThreadToCancel->ev );
}
/*-----------------------------------------------------------*/

static void *prvWaitForStart( void * pvParams )
{
    Thread_t *pxThread = pvParams;

    prvSuspendSelf(pxThread);

    /* Resumed for the first time, unblocks all signals. */
    uxCriticalNesting = 0;
    uxInterruptNesting = 0;
    xYieldPending = pdFALSE;
    vPortEnableInterrupts();

    /* Call the task's entry point. */
    pxThread->pxCode( pxThread->pvParams );

    /* A function that implements a task must not exit or attempt to return to
    * its caller as there is nothing to return to. If a task wants to exit it
    * should instead call vTaskDelete( NULL ). Artificially force an assert()
    * to be triggered if configASSERT() is defined, so application writers can
        * catch the error. */
    configASSERT( pdFALSE );

    return NULL;
}
/*-----------------------------------------------------------*/

static void prvSwitchThread( Thread_t *pxThreadToResume,
                             Thread_t *pxThreadToSuspend )
{
    sig_atomic_t uxSavedCriticalNesting;
    sig_atomic_t uxSavedInterruptNesting;

    if ( pxThreadToSuspend != pxThreadToResume )
    {
        /*
         * Switch tasks.
         *
         * The critical section nesting is per-task, so save it on the
         * stack of the current (suspending thread), restoring it when
         * we switch back to this task.
         */
        uxSavedCriticalNesting = uxCriticalNesting;
        uxSavedInterruptNesting = uxInterruptNesting;

        prvResumeThread( pxThreadToResume );
        if ( pxThreadToSuspend->xDying )
        {
            pthread_exit( NULL );
        }
        prvSuspendSelf( pxThreadToSuspend );

        uxCriticalNesting = uxSavedCriticalNesting;
        uxInterruptNesting = uxSavedInterruptNesting;
    }
}
/*-----------------------------------------------------------*/

static void prvSuspendSelf( Thread_t *thread )
{
    /*
     * Suspend this thread by waiting for a pthread_cond_signal event.
     *
     * A suspended thread must not handle signals (interrupts) so
     * all signals must be blocked by calling this from:
     *
     * - Inside a critical section (vPortEnterCritical() /
     *   vPortExitCritical()).
     *
     * - From a signal handler that has all signals masked.
     *
     * - A thread with all signals blocked with pthread_sigmask().
        */
    event_wait(thread->ev);
}

/*-----------------------------------------------------------*/

static void prvResumeThread( Thread_t *xThreadId )
{
    __atomic_store_n( &hRunningThread, xThreadId->pthread, __ATOMIC_RELEASE );
    if ( pthread_self() != xThreadId->pthread )
    {
        event_signal(xThreadId->ev);
    }
    prvKickSimulatedInterrupt();
}
/*-----------------------------------------------------------*/

static void prvSetupSignalsAndSchedulerPolicy( void )
{
    struct sigaction sigresume, sigtick, siginterrupt;
    int iRet;

    hMainThread = pthread_self();

    /* Initialise common signal masks. */
    sigfillset( &xAllSignals );
    /* Don't block SIGINT so this can be used to break into GDB while
     * in a critical section. */
    sigdelset( &xAllSignals, SIGINT );

    /*
     * Block all signals in this thread so all new threads
     * inherits this mask.
     *
     * When a thread is resumed for the first time, all signals
     * will be unblocked.
     */
    (void)pthread_sigmask( SIG_SETMASK, &xAllSignals,
                           &xSchedulerOriginalSignalMask );

    /* SIG_RESUME is only used with sigwait() so doesn't need a
       handler. */
    sigresume.sa_flags = 0;
    sigresume.sa_handler = SIG_IGN;
    sigfillset( &sigresume.sa_mask );

    /* The scheduler tick is an implementation detail and must not surface as
     * EINTR from restartable host system calls used by emulated tasks. */
    sigtick.sa_flags = SA_RESTART;
    sigtick.sa_handler = vPortSystemTickHandler;
    sigfillset( &sigtick.sa_mask );

    /* Peripheral interrupts share the tick's restart and masking behavior but
     * drain source state through the installed roo_testing dispatcher. */
    siginterrupt.sa_flags = SA_RESTART;
    siginterrupt.sa_handler = vPortSimulatedInterruptHandler;
    sigfillset( &siginterrupt.sa_mask );

    iRet = sigaction( SIG_RESUME, &sigresume, NULL );
    if ( iRet )
    {
        prvFatalError( "sigaction", errno );
    }

    iRet = sigaction( SIGALRM, &sigtick, NULL );
    if ( iRet )
    {
        prvFatalError( "sigaction", errno );
    }

    iRet = sigaction( SIG_SIMULATED_INTERRUPT, &siginterrupt, NULL );
    if ( iRet )
    {
        prvFatalError( "sigaction", errno );
    }
}
/*-----------------------------------------------------------*/

unsigned long ulPortGetRunTime( void )
{
    struct tms xTimes;

    times( &xTimes );

    return ( unsigned long ) xTimes.tms_utime;
}
/*-----------------------------------------------------------*/

void vPortSetStackWatchpoint( void *pxStackStart )
{
    (void) pxStackStart;
}
/*-----------------------------------------------------------*/
