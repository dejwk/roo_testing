# Emulated-time alarms

Status: Proposed

## Objective

Provide deterministic, cancellable one-shot callbacks at absolute roo_testing
uptimes without a background thread or implicit advancement of the emulated
clock.

## Motivation

Emulated peripherals need to finish asynchronous work when fake time reaches a
deadline. Implementing a private timer queue in each shim would duplicate
ordering, cancellation, concurrency, and callback-lifetime logic. Advancing the
global clock from a blocking peripheral call is also incorrect: it changes time
for every task and interacts badly with wall-clock auto-sync.

A shared alarm primitive lets peripherals describe completion deadlines while
leaving time progress under the existing system clock and test driver.

## Background

[`timer.cpp`](../roo_testing/system/timer.cpp) owns roo_testing's intended
monotonic uptime. Its `EmulatedTime` can follow host elapsed time or, with
auto-sync disabled, be advanced explicitly by tests. Arduino
`micros()`/`millis()` and ESP-IDF `esp_timer_get_time()` use this same clock.
The current timer has no callback queue, uses `high_resolution_clock` even
though C++ does not guarantee that clock is steady, and does not check all
unsigned lag/delay conversions for overflow.

ESP-IDF has several timer facilities rather than one universal alarm API. The
closest generic API is [`esp_timer`](../roo_testing/frameworks/esp-idf/components/esp_timer/include/esp_timer.h),
which provides one-shot and periodic software timers. Its [vendored
implementation](../roo_testing/frameworks/esp-idf/components/esp_timer/src/esp_timer.c)
keeps armed timers ordered by absolute alarm time and separates task and ISR
dispatch. Its lower-level [SYSTIMER
implementation](../roo_testing/frameworks/esp-idf/components/esp_timer/src/esp_timer_impl_systimer.c)
allocates a hardware interrupt whose ISR clears the alarm and invokes
`timer_alarm_handler`. That upper handler runs `ESP_TIMER_ISR` callbacks
directly; when no ISR timer was processed, it uses a `FromISR` notification to
wake the dedicated timer task for `ESP_TIMER_TASK` callbacks. ESP-IDF components
use the service for work including [touch
filtering](../roo_testing/frameworks/esp-idf/components/driver/touch_sensor/esp32/touch_sensor.c),
[task-watchdog
expiry](../roo_testing/frameworks/esp-idf/components/esp_system/task_wdt/task_wdt_impl_esp_timer.c),
[Ethernet link
polling](../roo_testing/frameworks/esp-idf/components/esp_eth/src/esp_eth.c),
and [Bluetooth sleep
timing](../roo_testing/frameworks/esp-idf/components/bt/controller/esp32/bt.c).
roo_testing currently shims its clock read, makes initialization harmless, and
reports no upcoming alarm, but does not implement timer creation, start,
restart, stop, or deletion in
[`idf_core.cpp`](../roo_testing/frameworks/esp32_shims/idf_core.cpp).

Hardware LEDC does not itself use `esp_timer` for fade completion; its
peripheral ISR advances fade segments and releases the channel semaphore. The
shared queue proposed here is therefore host-emulator infrastructure, not a
claim that ESP-IDF implements LEDC over its software-timer service.

Arduino hardware timers and ESP-IDF GPTimer are separate counter peripherals.
The Arduino shim retains counter, callback, and alarm configuration in
[`arduino_timer.cpp`](../roo_testing/frameworks/esp32_shims/arduino_timer.cpp),
but never dispatches the callback. The host FreeRTOS kernel also has software
timers, but its Linux tick is driven by wall-clock `ITIMER_REAL`; it is not a
deterministic substitute for an alarm tied to manually driven roo_testing
uptime.

In this design, an *alarm* is an internal C++ one-shot record associated with
an absolute system uptime and an *alarm handler*. The handler is infrastructure
for driver adapters, not a public peripheral callback. A consumer that models
hardware delivery can materialize peripheral status and use the separate
[emulated interrupt controller](emulated_interrupts.md) to run the registered
framework handler in ISR context.

## Requirements

1. A caller can schedule a one-shot callback at any signed 64-bit system
   uptime and cancel it by opaque identifier.
2. Scheduling, cancellation, clock access, and dispatch are safe from multiple
   host threads and FreeRTOS tasks.
3. An alarm never runs inline from scheduling, including when its deadline is
   already due.
4. Due alarms run in deadline order; alarms at one deadline run in registration
   order.
5. An owning explicit fake-time delay crosses deadlines chronologically before
   returning. If another drain owns delivery, advancement does not nest or move
   uptime backward, and the owner later dispatches the due handlers.
6. Alarm dispatch occurs only at documented pump points. Auto-sync does not
   create a background callback thread.
7. Alarm handlers are non-throwing by contract. Handler invocation and
   destruction of captured state occur outside the timer mutex, handler
   re-entry does not create a nested dispatch stack, and an escaping host
   exception cannot strand drain ownership.
8. Cancellation prevents an unclaimed callback. Cancelling executing,
   completed, or unknown work is a no-op.
9. An alarm scheduled or made due by a callback is not stranded after the
   active drain relinquishes ownership.
10. The core primitive imposes no ESP-IDF ISR, task affinity, priority, or
    periodic-timer semantics on its consumers.
11. Tests can cancel their own pending alarms and restore auto-sync without a
    global reset that invalidates unrelated registrations.
12. Host-time observation and explicit clock mutation preserve monotonic uptime;
    numeric conversions and additions fail before overflow.

### Out of scope

- Implementing the public `esp_timer`, GPTimer, or Arduino hardware-timer APIs.
  Those are adapters and follow-on work.
- FreeRTOS software timers, task delays, or `roo_scheduler` jobs.
- Implementing interrupt delivery itself, which belongs to the separate
  [emulated interrupt design](emulated_interrupts.md). An alarm consumer may
  use that implemented service.
- A background dispatcher that fires solely because wall time passed.
- A built-in periodic-alarm API. Periodic adapters reschedule one-shot alarms
  according to their own missed-period policy.
- RTC, Unix-time, calendar, or deep-sleep wake alarms.
- Scheduling periodic waveform edges; voltage signals remain analytical.

## Design Overview

Extend `EmulatedTime` with an ordered collection of value-owned alarm records
and an ID index for cancellation. Scheduling inserts work but never invokes it.
`system_time_delay_micros()` and explicit `ProcessSystemTimeAlarms()` calls are
the only generic dispatch points.

One timer mutex protects uptime, host-time origin, auto-sync state, alarm
records, ID allocation, and drain ownership. A drainer claims one due alarm,
releases the mutex, invokes it, and then rescans. This makes callback re-entry
and cancellation safe without nested delivery.

Consumers adapt this neutral deadline mechanism to their own behavior. The
[LEDC adapter](ledc_voltage_emulation.md#alarm-service-integration) uses it to
materialize fade completion and enqueue final GPIO publication followed by a
post-publication interrupt continuation. That continuation publishes ISR state
and raises the LEDC source; the LEDC ISR releases the channel gate and invokes
the registered callback. A future `esp_timer` shim can add handle and periodic
semantics around a timer-source interrupt, while Arduino timer or GPTimer
adapters can translate counter values and raise their own logical sources.

| Requirements | Design element |
| --- | --- |
| 1, 3-4, 8 | Ordered alarm records plus an ID index |
| 2 | One timer mutex around all shared clock and alarm state |
| 5-6, 12 | Checked monotonic clock updates and explicit pump points |
| 7, 9 | Scope-guarded, invoke-outside-lock non-nesting drain |
| 10 | Neutral one-shot callback contract with consumer adapters |
| 11 | Per-alarm teardown without a global alarm reset |

## Design Details

### Alarm storage and identity

Each record owns:

- an absolute `deadline_uptime_us`;
- a nonzero `SystemTimeAlarmId`;
- a `std::function<void()>` callback; and
- implicit registration order from its position among equal keys.

Store records in a `std::multimap<int64_t, AlarmRecord>`. The standard preserves
insertion order among equivalent keys, which supplies deterministic FIFO order
at one deadline. An `unordered_map<SystemTimeAlarmId, iterator>` makes
cancellation independent of queue length.

IDs increase monotonically, reserving zero as invalid. On wrap, allocation
skips zero and any live ID. Exhausting all nonzero IDs is a fatal invariant
violation; it would require more simultaneously live alarms than the process
can represent.

The callback owns its captures according to normal `std::function` rules.
Borrowed state must outlive any handler execution that may already have been
claimed; cancellation alone cannot establish that. Deletable adapters use
shared or deferred lifetime, or external synchronization that excludes a
concurrent claim.

### Scheduling and cancellation

Scheduling `CHECK`s that the callback is non-empty, allocates an ID, and inserts
the record while holding the timer mutex. An already-due deadline is queued
normally and waits for a pump point; scheduling never calls user code.

Cancellation removes an unclaimed record from both containers. Move the
callback out while locked and destroy it after unlock so captured-object
destructors cannot re-enter the timer under its mutex. Once a drainer has
claimed a record, cancellation is a no-op.

An alarm ID identifies one registration only. Rescheduling is expressed as
cancellation followed by a new schedule and therefore produces a new ID.
Peripheral adapters use their own generation tokens when stale callbacks must
also be harmless after a race with cancellation.

### Time advancement and pump points

`ScheduleSystemTimeAlarm()` and `CancelSystemTimeAlarm()` never readjust
uptime. Plain clock reads, `system_time_sync()`, and `system_time_lag_ns()` also
retain their current non-dispatching behavior.

Replace the host elapsed-time source with `std::chrono::steady_clock`, and never
assign an observed value below current uptime. Both
`system_time_delay_micros()` and `system_time_lag_ns()` perform checked unit
conversion and duration addition; an unrepresentable mutation fails a `CHECK`
before changing state.

`system_time_delay_micros()` remains the dispatching explicit-advance
operation. It walks through intervening alarm deadlines. At each boundary it
sets uptime, drains every alarm then due, and continues to the final target.
Alarms registered for the same deadline by a callback join the active drain
after previously registered work. The lower-level `system_time_lag_ns()`
retains its current non-dispatching behavior; work it makes due waits for a
later delay or explicit pump.

`ProcessSystemTimeAlarms()` first observes current uptime using the normal
auto-sync behavior and drains everything due at that observed time. It does not
add time itself. If wall-clock synchronization moved uptime past several
deadlines, callbacks still run in deadline/registration order but observe the
current, possibly later, uptime.

There is no generic background dispatcher. With auto-sync enabled, due work
runs at the next explicit pump point. With auto-sync disabled, a test or
another task must advance time and pump it. A consumer such as an LEDC channel
wait may call `ProcessSystemTimeAlarms()` at a documented retry boundary, but
that is an integration decision, not an additional kind of pump.

### Dispatch and re-entry

Only one drainer owns callback dispatch at a time. It performs this loop:

1. Coordinate drain ownership and timer state under the timer mutex. If
   auto-sync requires host sleeping, release the mutex, sleep, then reacquire
   and re-evaluate instead of using the old snapshot.
2. Synchronize or advance to the applicable limit and remove the earliest due
   record from both containers, marking it claimed.
3. Release the mutex and invoke the callback.
4. Destroy the callback outside the mutex, then lock and rescan.
5. When no due work remains, clear drain ownership and confirm the queue state
   in the same critical section.

A scope guard owns the drain marker for the entire loop. On normal completion,
step 5 clears ownership and disarms the guard in the same critical section, so
the guard cannot later clear a new drainer's ownership. On abnormal exit, the
guard clears ownership under the mutex. Alarm handlers must not throw. If a
handler nevertheless throws in a host build with exceptions enabled, its local
`std::function` is destroyed outside the mutex, the guard releases drain
ownership, and the exception propagates. Remaining alarms stay queued for a
later pump; the drainer does not catch the exception and continue invoking
unrelated handlers.

A recursive or concurrent pump that finds an active drainer records that a
rescan is needed and returns without invoking callbacks. The owner processes
newly due work after the current callback returns. Scheduling or cancellation
from a callback is therefore safe, and callback delivery never nests.

A recursive or concurrent explicit delay may still move uptime to its checked
target, but it cannot dispatch a nested handler. The owning drain resumes from
the later of current uptime and its original target, then processes skipped
deadlines in order at that possibly later uptime. No pump ever moves uptime
backward.

Consequently, a recursive or concurrent pump can return while due handlers are
still pending with the active drainer. The ordinary before-return drain
guarantee applies only to the caller that owns drain delivery.

Wall-clock sleeping for auto-sync never holds the timer mutex. After waking,
the drainer reacquires the mutex and re-evaluates uptime, drain ownership, and
the earliest alarm rather than relying on a stale snapshot.

### Consumer adapters

The core alarm callback executes on whichever host thread or FreeRTOS task
pumps time. It must not directly invoke a public callback that promises a task
or ISR context. Consumers add that dispatch layer after releasing alarm and
peripheral locks:

- The [LEDC adapter](ledc_voltage_emulation.md#alarm-service-integration)
  materializes generation-checked completion state and appends an ordered pair:
  final GPIO publication, then an internal completion finalizer. The finalizer
  publishes the ISR mailbox/status and raises the LEDC source only after the
  GPIO and sink calls have returned. Its emulated ISR releases the channel
  gate, invokes the registered callback, and requests any `FromISR` yield.
- A future `esp_timer` adapter owns timer handles and implements one-shot,
  periodic, restart, stop, delete, activity, expiry, and next-alarm queries.
  Its alarm asserts the timer source. The timer ISR processes `ESP_TIMER_ISR`
  callbacks and notifies an emulated timer task for `ESP_TIMER_TASK` callbacks,
  matching the real lower-ISR/upper-dispatch split.
  Its next-alarm queries inspect only its own handles, never unrelated records
  in the shared queue, and the wake-up query excludes timers configured with
  `skip_unhandled_events`.
- Arduino hardware timer and GPTimer adapters convert counter alarms to uptime
  deadlines. Start, stop, writes, frequency changes, autoreload configuration,
  and counter reload values cancel or replace their current internal alarm;
  expiry materializes counter status and raises the timer's interrupt source.
- Other peripheral shims can model conversion, transmission, scan, DMA, or
  timeout completion without advancing global time from the initiating call.

Adapters must not use this queue for high-frequency carrier edges. They retain
analytical state where observation does not require a discrete completion
event.

## Proposed API

The C++ API is added to [`timer.h`](../roo_testing/system/timer.h) outside its
`extern "C"` block. The `<functional>` include and declarations are guarded by
`__cplusplus`, so the existing C clock API remains usable.

```cpp
using SystemTimeAlarmId = uint64_t;

SystemTimeAlarmId ScheduleSystemTimeAlarm(
    int64_t deadline_uptime_us, std::function<void()> callback);
void CancelSystemTimeAlarm(SystemTimeAlarmId id);
void ProcessSystemTimeAlarms();
```

`SystemTimeAlarmId{0}` is invalid. Cancellation has no return value because the
caller cannot safely infer whether a callback already began executing from a
boolean result. An empty callback is a programmer error and fails `CHECK`
without allocating an ID.

No public ESP-IDF or Arduino timer API is added in this design.

## Implementation Plan

Authoring references: follow this repository's
[design-authoring guidance](../.github/instructions/embedded-design-doc-authoring.instructions.md),
[C++ code-authoring guidance](../.github/instructions/embedded-cpp-code-authoring.instructions.md),
and adjacent system-timer/test conventions.

### Phase 1: Deterministic one-shot uptime alarms

Add the C++ alarm API and synchronized timer state in `system/timer.h/.cpp`,
update BUILD dependencies, and add isolated ordering, cancellation, checked
clock-mutation, auto-sync, concurrency, and re-entry tests. Migrate
`EmulatedTime`'s existing clock fields under the same mutex, replace its host
elapsed-time source with `steady_clock`, and check lag/delay arithmetic as part
of this change.

Proposed commit: `Add deterministic system-time alarms`

Validation: run the system timer/alarm tests under auto-sync disabled and
enabled cases. Verify that scheduling never changes uptime and
`ProcessSystemTimeAlarms()` adds no time beyond normal auto-sync observation;
with auto-sync disabled, processing leaves uptime unchanged and an isolated
explicit delay advances by its checked requested amount. Verify that callbacks
and capture destruction run outside the mutex, test-owned pending alarms are
cancelled, and auto-sync is restored at teardown.

## Testing Plan

Tests cover future, due, and past deadlines; chronological and same-deadline
ordering; cancellation before claim and during an earlier callback;
cancellation of unknown IDs and already-completed alarms; callback-created
alarms; non-dispatching schedule and clock reads; explicit delays across several
deadlines; wall-clock auto-sync; checked large delays and lags; concurrent
schedule/cancel/pump; recursive pumping; and lock-free callback and capture
destruction. Builds with exceptions enabled also verify that a throwing test
handler releases drain ownership and leaves unclaimed work for the next pump.

Alarm unit tests run in an isolated test process because an explicit global
pump can legitimately dispatch any due consumer. Tests capture a starting
uptime and use relative deadlines rather than resetting or assuming zero. Every
test retains and cancels the IDs it creates and restores timer auto-sync. Tests
do not expose a global reset operation that could invalidate another consumer's
alarms.

## Caveats

Core alarm handlers do not themselves emulate interrupt context and may observe
a later uptime when wall-clock sync passes their deadline before a pump.
Consumer adapters must not assume that handler execution time equals the
registered deadline. They can request ISR delivery through the interrupt
controller after publishing authoritative peripheral state, completing any
externally observable effect that precedes the hardware interrupt, and
releasing locks. If external publication is already being drained, an ordered
post-publication continuation performs the source assertion later without
nested delivery.

An alarm callback that blocks also blocks the task performing the pump. Other
FreeRTOS tasks may continue, but this primitive does not create an independent
execution context.

Cancellation does not wait for an already-claimed handler. Consumer adapters
must use generation checks and shared or deferred lifetime for deletable
handles; an alarm handler must never capture a raw handle that cancellation can
free concurrently.

### Rejected Alternatives

#### Keep the queue private to LEDC

LEDC is only the first consumer. A private queue would duplicate the same
mechanism for `esp_timer`, Arduino hardware timers, GPTimer, and future
peripheral completions while obscuring ownership of global clock behavior.

#### Use FreeRTOS software timers

The host FreeRTOS tick follows wall time and requires the scheduler. It cannot
deterministically fire when a test advances roo_testing uptime with auto-sync
disabled, and it would introduce a second timebase for emulated peripherals.

#### Use `roo_scheduler`

`roo_scheduler` models application-owned cooperative jobs with priorities and
explicit execution. System alarms are lower-level global clock events with
different ownership, cancellation, and callback-context requirements.

#### Run a background timer thread

A background worker would make manually driven tests depend on host scheduling
and complicate global-time synchronization. Explicit pump points preserve
deterministic ordering; adapters can introduce a task when their public API
requires one.

#### Dispatch alarms from every clock read

Clock reads occur in logging, sampling, and validation paths. Making them
execute arbitrary callbacks would create surprising re-entry and turn passive
observation into a mutation point.

#### Add periodic alarms to the core

Periodic APIs differ on drift, missed periods, callback overruns, stop/restart,
and sleep handling. Rescheduling absolute one-shots in an adapter keeps those
policies with the API that promises them.

## Future Work

- Implement an `esp_timer` adapter over the alarm queue when a roo consumer
  needs timer handles or periodic callbacks.
- Complete Arduino hardware-timer alarm delivery and add a direct GPTimer shim
  when required.
- Use alarms for additional asynchronous peripheral completions when their
  observable timing matters to a test.
- Route consumers through the implemented interrupt controller or a dedicated
  task only when their public callback contract requires that context.
