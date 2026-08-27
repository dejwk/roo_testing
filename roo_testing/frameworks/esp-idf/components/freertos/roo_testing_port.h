#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Dispatcher invoked in POSIX-signal and FreeRTOS ISR context.
///
/// The function and all state it accesses must have static lifetime. It must be
/// signal-safe, nonblocking, nonthrowing, and must not lock or allocate. Port
/// requests coalesce, so the dispatcher must drain authoritative source state
/// maintained by the higher-level interrupt controller.
typedef void (*RooTestingInterruptDispatcher)(void);

/// Installs the process-wide dispatcher once or confirms the same installation.
///
/// Replacement and uninstallation are not supported because a signal handler
/// may already have loaded the installed function. Returns false for a null or
/// different dispatcher.
bool xPortInstallSimulatedInterruptDispatcher(
    RooTestingInterruptDispatcher dispatcher);

/// Requests delivery of an emulated interrupt to the running FreeRTOS task.
///
/// This function may be called by a native host thread or by the installed
/// dispatcher. Multiple requests can coalesce; the dispatcher must retain and
/// drain its own source-pending state.
void vPortRequestSimulatedInterrupt(void);

#ifdef __cplusplus
}
#endif
