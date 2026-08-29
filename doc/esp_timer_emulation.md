# ESP-IDF esp_timer emulation

Status: Proposed

## Objective

Run the vendored ESP-IDF `esp_timer` service against roo_testing uptime with
faithful task-dispatched one-shot and periodic callbacks.

## Motivation

ESP-IDF components use `esp_timer` for deferred work, polling, watchdogs, and
legacy ETS timers. The current host shim exposes time reads and reports
initialization success, but timer creation and delivery are absent. Components
that rely on callbacks therefore cannot run correctly.

The vendored common service already defines public handle lifetime, ordering,
periodic catch-up, skipped-event policy, callback context, and diagnostics.
Reusing it avoids creating a second subtly different implementation.

## Background

[`esp_timer.c`](../roo_testing/frameworks/esp-idf/components/esp_timer/src/esp_timer.c)
is the target-independent policy layer. It owns timer handles and ordered lists,
creates the high-priority timer task, executes `ESP_TIMER_TASK` callbacks, and
implements the public lifecycle and query APIs. Its only hardware dependency is
the private
[`esp_timer_impl_*` interface](../roo_testing/frameworks/esp-idf/components/esp_timer/private_include/esp_timer_impl.h).

On classic ESP32, the vendored
[LAC backend](../roo_testing/frameworks/esp-idf/components/esp_timer/src/esp_timer_impl_lac.c)
maps that interface to a free-running counter, one physical compare, and the
`ETS_TG0_LACT_LEVEL_INTR_SOURCE` interrupt. The common layer keeps the earliest
timer at the head of its list and programs only that compare. The lower ISR
clears hardware status and invokes the common alarm handler. With task dispatch,
that handler wakes the dedicated timer task, which drains all timers due at the
current time.

The current roo_testing configuration defines timer task affinity, priority,
and stack size but does not define
`CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD`. Consequently the public header
exposes only `ESP_TIMER_TASK`, and the common layer maintains one active list.

[`idf_core.cpp`](../roo_testing/frameworks/esp32_shims/idf_core.cpp) currently
implements `esp_timer_get_time()` as a roo_testing clock read, returns success
from early init, init, and deinit, and reports no next alarm. It does not compile
the vendored common service or implement create, start, restart, stop, delete,
periodic, query, or dump operations.

Hardware startup consumes `ESP_SYSTEM_INIT_FN` linker sections. The current host
runners do not, so compiling the vendored init records alone would not initialize
the service before application code.

The [emulated-time clock](emulated_time_clock.md) provides the counter. The
[emulated-time alarm service](emulated_time_alarms.md) provides a dynamically
retargetable host deadline, and the
[emulated interrupt controller](emulated_interrupts.md) provides recognizable
FreeRTOS ISR delivery even while a task is CPU-busy.

## Requirements

1. Public timer creation, start, restart, stop, deletion, activity, period,
   expiry, next-alarm, wake-alarm, and dump behavior matches the vendored common
   ESP-IDF implementation.
2. One-shot and periodic callbacks configured for `ESP_TIMER_TASK` run on the
   dedicated ESP-IDF timer task, never on the native alarm worker or explicit
   time-pump owner.
3. Timers use the same monotonic microsecond uptime as `esp_timer_get_time()`,
   Arduino clock APIs, and the generic alarm service.
4. Manual-time tests deliver timers deterministically after explicit time
   advancement and FreeRTOS scheduling. Auto-sync processes deliver them without
   application polling, including while the selected application task is
   CPU-busy.
5. Equal-deadline ordering, callback-visible stop/restart, periodic catch-up,
   `skip_unhandled_events`, deferred deletion, and minimum-period behavior remain
   owned by the vendored common layer.
6. Rearming, stopping, or deleting the earliest timer makes an obsolete host
   deadline harmless even when cancellation races a callback already claimed by
   the generic alarm service.
7. Initialization creates exactly one timer task and one interrupt registration;
   partial failure rolls back both. Deinitialization rejects active public timers
   according to upstream rules and leaves no live host compare or interrupt.
8. Timer expiry crosses the hardware boundary: the neutral host deadline marks
   compare status and raises the LAC interrupt source; only the emulated ISR calls
   the common alarm handler.
9. The current build supports task dispatch only. Unsupported ISR dispatch and
   ETM behavior fail explicitly instead of silently degrading to another context.
10. Callbacks and query results preserve upstream error codes and do not gain
    roo_testing-specific public lifetime or reset semantics.

### Out of scope

- Enabling `CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD` or
  `ESP_TIMER_ISR` callbacks.
- ESP Timer Event Task Matrix integration.
- Light sleep, RTC correction, deep-sleep wake, or power-management behavior.
- Emulating LAC or SYSTIMER registers for arbitrary driver access.
- Hard-real-time callback latency or minimum-period fidelity below what the host
  scheduler can sustain.
- Replacing the vendored public service with a C++ reimplementation.

## Design Overview

The implementation retains the upstream policy/backend boundary:

```text
public esp_timer API
        |
vendored esp_timer.c: handles, ordered list, periodic policy, timer task
        |
host esp_timer_impl backend: one requested compare and interrupt status
        |
generic uptime alarm -> ESP32 LAC source -> emulated ISR -> common handler
```

The design introduces three backend concepts:

| Term | Meaning and lifetime |
| --- | --- |
| Requested compare | The latest absolute microsecond timestamp supplied by the common layer; `UINT64_MAX` means disarmed. |
| Compare generation | A nonreused value identifying one requested compare, used to reject callbacks from older requests. |
| Pending status | Lock-free ISR-visible state saying that the current generation reached its compare and may invoke the common alarm handler once. |

The backend owns one generic `SystemTimeAlarmId`, not one per public timer. The
common layer already orders public timers and programs only its head, matching
the single hardware compare. A due generic callback validates its generation,
publishes pending status, and raises `ETS_TG0_LACT_LEVEL_INTR_SOURCE`. The lower
emulated ISR consumes pending status and invokes the common handler. That handler
wakes the upstream timer task, which applies every public timer policy.

| Requirements | Design element |
| --- | --- |
| 1, 5, 10 | Unmodified vendored common service |
| 2, 8 | LAC source assertion followed by upstream ISR and timer task |
| 3–4 | Shared clock and generic alarm service |
| 6 | Compare generations plus ISR-visible pending status |
| 7 | Transactional backend initialization and upstream deinitialization |
| 9 | Current TASK-only configuration and explicit ETM rejection |

## Design Details

### Vendored common service

The host build compiles `esp_timer.c`, `esp_timer_init.c`, and
`esp_timer_impl_common.c` from the vendored ESP-IDF component. It does not copy
their algorithms into shim code. The build supplies a new host backend for the
SoC-specific functions and removes the overlapping placeholder definitions from
`idf_core.cpp`.

The common service continues to own:

- argument and initialization validation;
- allocation and lifetime of `esp_timer_handle_t`;
- sorted insertion and FIFO order among equal timestamps;
- one-shot, periodic, restart, stop, and active state;
- periodic catch-up and `skip_unhandled_events` advancement;
- callback invocation outside the timer-list critical section;
- deferred deletion through the timer task;
- next-alarm, wake-alarm, period, expiry, and dump queries; and
- profiling fields when the existing configuration enables them.

The host backend reports the existing common minimum of 50 microseconds. This is
a compatibility floor, not a host latency promise. Periods below it are clamped
by the upstream implementation.

### Counter and clock adjustment

`esp_timer_impl_get_time()` and public `esp_timer_get_time()` return
`system_time_get_micros()`. This is the complete ISR-visible counter path and
inherits the clock design's monotonic, nonblocking contract.

For internal inspection, `esp_timer_impl_get_counter_reg()` returns the same
microsecond counter and `esp_timer_impl_get_alarm_reg()` returns the requested
compare. These host values model behavior, not LAC tick encoding.

`esp_timer_impl_advance()` accepts only nonnegative microsecond adjustments and
publishes them through checked emulated-time advancement. The private set entry
point accepts a value no earlier than current uptime and advances to it. A
negative adjustment or backward set terminates via `CHECK`; silently moving a
monotonic clock backward would corrupt both public timer ordering and generic
alarms. Host light-sleep emulation is outside scope and introduces no caller that
requires backward correction.

### Compare programming and cancellation

`esp_timer_impl_set_alarm_id()` accepts only alarm ID zero in the current
TASK-only configuration. Each call increments the compare generation, clears
pending status, extracts the previous generic alarm ID under the backend's
scheduler-safe host lock, then cancels that alarm after unlocking.

`UINT64_MAX` leaves the compare disarmed. A timestamp later than the generic
alarm service's representable maximum is retained as the requested compare but
creates no host registration: monotonic uptime cannot reach it. Every other
timestamp registers one generic alarm at the same absolute microsecond uptime.

Registration occurs without the backend lock held. After it returns, the caller
installs the resulting ID only when its generation is still current and the due
callback has not already completed. Otherwise it cancels that ID. The due
callback takes the backend lock to validate the generation and clear the stored
ID before publishing pending status. This handshake covers already-due alarms,
concurrent rearming, and cancellation without holding one service lock while
calling another.

Cancellation cannot stop a generic callback already claimed for delivery. The
generation check prevents such a callback from publishing status after a newer
compare request. Once status is published, a concurrent stop can clear it before
the ISR consumes it. An ISR already consuming the old status is a legitimate
hardware-like race with stop; upstream list locking determines whether any
public timer remains due.

Generation exhaustion is fatal and generations are never reused. Backend state
has process lifetime, so a late claimed callback never dereferences destroyed
storage.

### Interrupt delivery

`esp_timer_impl_init()` stores the common alarm handler and allocates
`ETS_TG0_LACT_LEVEL_INTR_SOURCE` through the implemented `esp_intr_alloc()` host
adapter. It registers a small lower ISR rather than the common handler directly.
The lower ISR atomically consumes pending status and calls the common handler
only for a current due compare.

The neutral generic-alarm callback never calls the common handler. After
publishing pending status and releasing the backend lock, it calls
`raiseInterruptSource(ETS_TG0_LACT_LEVEL_INTR_SOURCE)`. This preserves the public
ISR boundary and allows the interrupt controller to preempt CPU-busy task code.

Level-source coalescing is sufficient. The common timer task drains every public
timer due at its observed uptime, and any new earliest deadline reprograms the
single compare. A stale or duplicate source assertion finds no pending status
and performs no upper call.

### Initialization and deinitialization

`esp_timer_impl_early_init()` is idempotent and returns `ESP_OK`; the shared clock
needs no peripheral initialization. The vendored `esp_timer_early_init()` retains
its upstream call sequence.

The common `esp_timer_init()` creates the timer task before calling backend init.
Backend init rejects repetition, allocates its interrupt disabled, initializes
compare state, then enables delivery. Failure frees any partial interrupt state
and returns the allocator error; the common layer deletes the timer task on that
failure.

Because host runners do not execute ESP-IDF init linker sections, their selected
FreeRTOS task calls an internal `InitializeEspTimerForHost()` before application
or test code. The helper makes early initialization idempotent and treats an
already initialized backend as success. Initialization runs in a scheduled task
because the host interrupt allocator rejects native and pre-scheduler callers.

The common `esp_timer_deinit()` first rejects active timers under its existing
rules. Backend deinit increments the generation, clears pending status, extracts
and cancels the generic alarm after unlocking, disables and frees the interrupt,
and clears the stored upper handler. Static backend storage remains valid for a
claimed stale callback, which fails its generation check. The common layer then
deletes the timer task.

Applications and tests delete their timers before explicit deinitialization.
Before generic alarm shutdown, the selected runner task calls the restricted
`ShutdownEspTimerForHost()`. It is a no-op after prior successful public
deinitialization; otherwise it calls the common deinitializer. Active timers
produce `ESP_ERR_INVALID_STATE`, which the runner reports as a lifecycle failure
instead of closing the alarm service under a timer that can rearm. On success,
the timer task, backend compare, and interrupt are gone before the generic alarm
service enters closing.

### Task callback behavior

The emulated ISR calls the unmodified common alarm handler. In the current build
it uses `vTaskNotifyGiveFromISR()` and requests a yield when necessary. The timer
task wakes, drains due timers in upstream order, and invokes callbacks with
`xPortInIsrContext() == false` at `CONFIG_ESP_TIMER_TASK_PRIORITY` on the
configured core.

Callbacks may stop, restart, delete, or create timers exactly where upstream
permits those operations. Periodic callbacks retain the upstream catch-up rule:
without `skip_unhandled_events`, missed expirations remain scheduled in period
steps and can produce consecutive callbacks; with the flag, the timer advances
past accumulated missed events according to the vendored algorithm.

### ISR dispatch boundary

`CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD` remains undefined. Enabling it
would make the common ISR callback path call `esp_timer_impl_set_alarm_id()` from
emulated ISR context. The generic dynamic alarm API allocates and locks and is
therefore not valid for that rearm.

The build fails rather than compiling the host backend with ISR dispatch enabled.
A later design must add a fixed, allocation-free compare ingress before changing
this configuration. It must also cover `esp_timer_isr_dispatch_need_yield()`, two
cached compare heads, and coincident TASK/ISR delivery.

## Proposed API

No roo_testing-specific public API is added. The supported surface remains the
vendored [`esp_timer.h`](../roo_testing/frameworks/esp-idf/components/esp_timer/include/esp_timer.h):

- `esp_timer_early_init()`, `esp_timer_init()`, and `esp_timer_deinit()`;
- `esp_timer_create()`, `esp_timer_start_once()`,
  `esp_timer_start_periodic()`, `esp_timer_restart()`, `esp_timer_stop()`, and
  `esp_timer_delete()`;
- `esp_timer_get_time()`, `esp_timer_get_next_alarm()`,
  `esp_timer_get_next_alarm_for_wake_up()`, `esp_timer_get_period()`,
  `esp_timer_get_expiry_time()`, and `esp_timer_is_active()`; and
- `esp_timer_dump()` with the current profiling configuration.

The upstream return codes and argument contracts remain authoritative.
`ESP_TIMER_ISR` is absent from the configured public enum.
`esp_timer_new_etm_alarm_event()` returns `ESP_ERR_NOT_SUPPORTED` without
producing a handle because the host has no ETM fabric.

Two restricted functions bridge the host's missing startup-section execution
and required shutdown order:

```cpp
esp_err_t InitializeEspTimerForHost();
esp_err_t ShutdownEspTimerForHost();
```

They are not declared by `esp_timer.h` and are unavailable to applications.

During the internal-backend phase, the existing public stubs remain unchanged
and the backend target has restricted visibility. The public APIs become
available only when the vendored common service replaces all overlapping stubs
in one change; there is no partially functional public timer interval.

## Implementation Plan

Authoring reference: follow this repository's
[C++ code-authoring guidance](../.github/instructions/embedded-cpp-code-authoring.instructions.md).

The completed [clock](emulated_time_clock.md#implementation-plan),
[alarm](emulated_time_alarms.md#implementation-plan), and interrupt
[controller and ESP-IDF adapter](emulated_interrupts.md#implementation-plan) are
prerequisites.

### Phase 1: Host counter, compare, and interrupt backend

Add a restricted host `esp_timer_impl` target with counter reads, one TASK
compare, generation-safe alarm registration, pending status, LAC source
delivery, initialization rollback, and deinitialization. Keep the public stubs
in `idf_core.cpp` during this phase.

Proposed commit: `Emulate the ESP timer hardware boundary on host uptime`

Validation: direct backend tests cover early reads, arm, rearm, disarm, already-
due delivery, stale claimed callbacks, pending-clear races, equal and extreme
timestamps, repeated init, allocator failure, deinit with a claimed callback,
and manual versus auto-sync delivery. The handler must run only through an
emulated ISR.

### Phase 2: Vendored task-dispatched esp_timer service

Compile the vendored common, init, and implementation-common sources with the
host backend. Remove overlapping timer stubs from `idf_core.cpp`, add the ETM
not-supported implementation, add the restricted lifecycle helpers, and wire
the ESP-IDF runner and both FreeRTOS GTest runners to initialize before user code
and deinitialize before generic alarm shutdown. Update BUILD dependencies and
public API documentation. Add focused conformance targets in both clock modes.

Proposed commit: `Run the vendored ESP timer service on the host backend`

Validation: cover initialization errors; create validation; one-shot start,
stop, restart, delete, and self-rearm; equal-deadline FIFO order; periodic
catch-up and skipped events; the 50-microsecond floor; every query; dump output;
timer-task callback context; manual advancement; autonomous CPU-busy delivery;
automatic runner initialization; active-timer shutdown rejection; prior public
deinitialization; and clean ordered shutdown. Compile-time checks reject
ISR-dispatch configuration.

### Phase 3: Legacy ETS timer integration

Compile the vendored
[`ets_timer_legacy.c`](../roo_testing/frameworks/esp-idf/components/esp_timer/src/ets_timer_legacy.c)
adapter and document support for its `ets_timer_*` and `os_timer_*` aliases. It
is the selected integration consumer because its entire behavior is expressed
over `esp_timer`, it uses `ESP_TIMER_TASK`, and it introduces no unrelated
peripheral model.

Proposed commit: `Run legacy ETS timers on emulated esp_timer`

Validation: exercise `ets_timer_setfn()`, one-shot and periodic microsecond and
millisecond arming, disarm, deletion after disarm, and all `os_timer_*` aliases
in manual and auto-sync binaries. Confirm that the adapter never calls the
generic alarm API directly and that every callback runs on the ESP timer task.

## Testing Plan

Backend tests validate the hardware boundary independently of upstream policy.
Public conformance tests then exercise the unmodified common service in separate
manual and auto-sync processes. FreeRTOS integration verifies ISR-to-task
handoff, callback context, priority, and CPU-busy preemption. Legacy ETS adapter
tests prove that a real vendored compatibility layer uses the same path.

Wall-time cases assert no early callback and eventual delivery, not a hard
latency threshold. Manual cases use relative deadlines from observed uptime and
yield the scheduler after advancement when task callback completion is expected.
Every test deletes its handles and leaves the service quiescent. After deferred
deletion in manual mode, it pumps current-time alarms and yields the timer task
before runner shutdown.

## Caveats

Timestamp precision is one microsecond, but delivery latency includes the host
condition wait, pthread scheduling, emulated interrupt ingress, and timer-task
scheduling. Periods at the 50-microsecond floor can overwhelm a general-purpose
host even though the common service accepts them.

The backend models the counter and compare contract, not readable LAC register
encoding. Code that accesses timer-group registers directly is outside the
`esp_timer` API and remains unsupported.

Deinitialization cannot revoke a common callback already running on the timer
task; upstream requires callers to quiesce their timer usage. Generation checks
only protect the lower host compare from stale generic-alarm callbacks.

A finite host application must stop or delete active timers before returning.
Unlike hardware, returning from a host runner ends the process and therefore
performs ordered service shutdown; an active timer is reported as a lifecycle
failure.

A custom host runner that bypasses the supplied ESP-IDF and GTest mains must call
the restricted lifecycle helpers from its selected FreeRTOS task. It cannot rely
on the target linker section being executed automatically.

### Rejected Alternatives

#### Add an arbitrary second integration component

Phase 3 does not bring up another component merely as a smoke test. The software
Task Watchdog requests `ESP_TIMER_ISR`, which is outside this design. Touch
filtering and Ethernet link polling use task dispatch, but compiling and
validating either also requires its peripheral or driver lifecycle; that work
belongs to the corresponding component design. Phase 2 already covers the same
public timer semantics they consume, while the selected legacy ETS adapter adds
integration coverage without unrelated infrastructure.

#### Reimplement the public service in C++

That would duplicate upstream ordering, callback-visible mutations, periodic
catch-up, skipped-event policy, deferred deletion, and diagnostics. Keeping the
vendored policy layer makes behavior track imported ESP-IDF updates.

#### Give each public timer a generic alarm

The upstream layer already maintains an ordered list and hardware programs only
its earliest entry. Per-handle host alarms would duplicate ordering, complicate
periodic races, and bypass the common timer task.

#### Invoke the common handler from the native alarm callback

The common handler is an ISR and uses `FromISR` APIs. Direct invocation would
give it the wrong context and fail to preempt CPU-busy FreeRTOS code through the
implemented interrupt path.

#### Enable ISR dispatch in the first implementation

ISR callbacks can rearm the hardware compare while still in ISR context. The
dynamic generic alarm API is intentionally task/native-only, so enabling this
mode without a fixed compare ingress would violate both designs.

#### Emulate timerfd-backed hardware separately

The generic alarm service already provides manual and auto-sync deadline
delivery. A second native deadline worker would duplicate clock mapping,
retargeting, cancellation, and shutdown behavior.

## Future Work

### ISR-dispatched timers

Enable `CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD` only after adding a
fixed, allocation-free compare ingress. Until then, `ESP_TIMER_ISR` is absent
from the configured public enum. Applications retain full TASK-dispatched timer
behavior but cannot run a callback directly in the timer interrupt, use
`FromISR` operations there, or request an immediate post-ISR yield; callback
latency includes notification and scheduling of the high-priority timer task.

The follow-on design must cover:

- two statically allocated, always-lock-free compare slots for the common
  layer's TASK and ISR deadline heads;
- bounded `esp_timer_impl_set_alarm_id()` updates from task or emulated ISR
  context, without a host mutex, allocation, waiting, or capture destruction;
- atomic selection of the earlier slot and nonreused generations that make
  stale expiry and cancellation races harmless;
- an async-signal-safe control wake when ISR code moves the earliest deadline.
  The current pthread condition variable cannot be signalled from the POSIX
  signal frame. The follow-on design must either add an eventfd or pipe bridge,
  migrate the waiter transport, or select another demonstrably signal-safe
  mechanism without introducing deadline polling;
- upper-handler sequencing for ISR callbacks, accumulated
  `esp_timer_isr_dispatch_need_yield()` requests, TASK notification, and a
  follow-up interrupt when coincident TASK and ISR heads cannot both complete in
  one interrupt; and
- shutdown and reconfiguration that quiesce both compare slots before freeing
  the interrupt or closing the alarm waiter.

Validation must prove ISR context, legal `FromISR` use, requested yields,
self-rearm and stop, simultaneous TASK/ISR deadlines, earlier ISR-side
retargeting, cancellation racing expiry, and CPU-busy preemption. Instrumented
tests must also confirm that the ISR path never locks, allocates, waits, destroys
captures, or calls pthread condition-variable operations.

### Other extensions

- Add ETM alarm-event emulation if a host ETM model is introduced.
- Integrate light-sleep clock correction after the emulator has an RTC and
  power-management design.
