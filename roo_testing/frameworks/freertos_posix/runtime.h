#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/// Reports a running application task, excluding native threads and ISR frames.
bool RooFreeRtosTaskContext(void);
/// Anchors kernel ticks to shared uptime before the native ticker starts.
void RooFreeRtosRuntimeStarted(void);
/// Disables runtime services after the ticker stops.
void RooFreeRtosRuntimeStopped(void);
/// Consumes a bounded batch of ticks elapsed on the shared clock.
uint32_t RooFreeRtosElapsedTicks(void);
/// Signals expired high-resolution waits from scheduler interrupt context.
long RooFreeRtosPollWaiters(void);
/// Unlinks a deleted task's stack-owned wait before its pthread is reclaimed.
void RooFreeRtosTaskDeleted(void* task);
/// Advances an idle manual clock to the earliest supported wake deadline.
void RooFreeRtosIdle(void);
/// Parks the current task until shared uptime reaches the absolute deadline.
void RooFreeRtosWaitUntil(int64_t deadline_us);
/// Deletes other application tasks before returning through scheduler shutdown.
void RooFreeRtosEndScheduler(void);
/// Reclaims the final caller task after native main regains control.
void RooFreeRtosDeleteFinalTask(void);
/// Queries the next finite kernel wait, including a tick-counter rollover.
uint32_t RooFreeRtosKernelIdleTicks(void);
/// Reports native port task context; valid before and after scheduler startup.
bool RooFreeRtosPortIsTask(void);
/// Reports whether the current native context is a scheduler signal handler.
bool RooFreeRtosPortIsIsr(void);
/// Requests a pending shared-clock update on the selected kernel task.
void RooFreeRtosPortRequestTick(void);
#ifdef __cplusplus
}
#endif
