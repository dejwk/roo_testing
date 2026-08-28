# Emulated-time alarms

Status: Proposed

## Objective

Provide cancellable one-shot callbacks at absolute roo_testing uptimes, with
deterministic explicit delivery in manual time and autonomous best-effort
delivery while uptime follows the host monotonic clock.

## Motivation

Emulated peripherals need to complete asynchronous work when fake time reaches a
deadline. Private timer queues would duplicate ordering, cancellation,
concurrency, and callback-lifetime logic. Advancing global time from a blocking
peripheral call is also incorrect because it changes time for every task.

A shared alarm primitive lets a peripheral register a completion deadline while
the configured clock or test driver remains responsible for time progression. A
waiting peripheral suspends only its calling FreeRTOS task; other runnable tasks
and the selected time driver continue independently.

## Background

The [emulated-time clock](emulated_time_clock.md) supplies one monotonic uptime
with an immutable process mode. Passive reads are ISR-safe. Clock mutation and
alarm management are task/native-only and use scheduler-safe host locking when
native pthreads share state with FreeRTOS task pthreads.

In manual mode, only an explicit test driver advances uptime. In auto-sync mode,
uptime follows `CLOCK_MONOTONIC`. Plain clock reads remain passive and never
invoke callbacks.

The Linux FreeRTOS scheduler tick cannot serve as the alarm source. It runs at
1 kHz, and its signal handler cannot lock, allocate, or invoke arbitrary alarm
handlers. A CPU-busy selected task therefore requires an independently scheduled
native waiter to notice a wall-clock deadline.

ESP-IDF `esp_timer`, Arduino hardware timers, GPTimer, and FreeRTOS software
timers retain their own public lifecycle, periodic, and callback-context
semantics. This proposal supplies only the lower-level host uptime deadline.
Adapters translate its neutral callback into their promised context.

## Requirements

1. A caller can register and cancel a one-shot callback at an absolute
   microsecond uptime exactly representable by the service. Invalid deadlines
   terminate via `CHECK` before registration changes service state.
2. Registration, cancellation, clock mutation, and explicit dispatch are safe
   from multiple native threads and FreeRTOS tasks. They are not POSIX-signal-
   or ISR-safe.
3. Registration never invokes a callback inline, including for an already-due
   deadline.
4. Due callbacks run by deadline and then registration order. Only one delivery
   owner invokes callbacks at a time, delivery never nests, and newly due work is
   reconsidered before that owner becomes idle.
5. An explicit delay crosses intervening deadlines chronologically before
   returning when its caller owns delivery. Concurrent delivery never moves
   uptime backward.
6. Manual-mode callbacks run only at explicit pump points. Auto-sync callbacks
   become eligible without application polling, including while the selected
   FreeRTOS task is CPU-busy. Autonomous delivery has no hard latency bound.
7. Callbacks are short and non-throwing and never wait for a task, interrupt, or
   external completion. Invocation and capture destruction occur without the
   service lock held.
8. Cancellation prevents an unclaimed callback. Cancelling executing,
   completed, invalid, or unknown work is a no-op and never waits for delivery.
9. A consumer can tear down its pending registrations without resetting or
   invalidating unrelated alarms.
10. The primitive imposes no ESP-IDF ISR, task-affinity, priority, or periodic
    policy on consumers.
11. A peripheral completion wait suspends only its calling FreeRTOS task. It
    never advances uptime or pumps alarms to manufacture completion.

### Out of scope

- Public `esp_timer`, GPTimer, or Arduino hardware-timer APIs.
- FreeRTOS software timers, task delays, or `roo_scheduler` jobs.
- Interrupt delivery; see [Emulated interrupts](emulated_interrupts.md).
- A built-in periodic alarm API.
- Hard-real-time latency or sustained high-frequency edge delivery.
- RTC, Unix-time, calendar, or deep-sleep wake alarms.
- Changing time mode; see the [emulated-time clock](emulated_time_clock.md).

## Design Overview

| Term | Meaning and lifetime |
| --- | --- |
| Alarm record | A value-owned deadline, callback, and nonreused ID, retained until cancellation or claim. |
| Delivery owner | The one caller allowed to claim and invoke due callbacks during a non-nesting drain. |
| Explicit pump | A manual call that observes or advances uptime and requests delivery. |
| Native waiter | One process-wide pthread that waits for the next host-mapped deadline in auto-sync mode. |

The modes differ only in who notices that a deadline arrived:

```text
manual:    test driver -> advance or pump -> delivery owner -> alarm callback
auto-sync: condition timeout -> native waiter -> delivery owner -> alarm callback
```

The waiter uses a pthread condition variable configured with `CLOCK_MONOTONIC`.
Queue, clock-floor, and lifecycle changes notify it. After every notification,
timeout, or spurious wake it recomputes the earliest deadline under the service
lock. Manual-mode processes never create the waiter.

An alarm callback has neutral task/native context. A hardware-like consumer
publishes completion state and then requests delivery through the interrupt
controller or its task-level dispatcher.

| Requirements | Design element |
| --- | --- |
| 1, 3, 8–9 | Checked registration, nonreused IDs, ordered records, and ID index |
| 2, 7 | Scheduler-safe lock; invocation and destruction after unlock |
| 4–5 | Single non-nesting owner and chronological explicit delay |
| 6 | Explicit manual pumps and one auto-sync condition waiter |
| 10 | Neutral one-shot callback contract |
| 11 | Consumer-owned FreeRTOS wait independent of alarm pumping |

## Design Details

### Alarm storage and identity

Each record owns an absolute microsecond deadline, a nonzero
`SystemTimeAlarmId`, and a `std::function<void()>` callback. A
`std::multimap<int64_t, AlarmRecord>` preserves insertion order among equal
deadlines. An `unordered_map<SystemTimeAlarmId, iterator>` makes cancellation
independent of queue length.

IDs increase monotonically, reserve zero, and are never reused in one process.
Exhaustion is fatal. Nonreuse prevents a claimed callback's ID from referring to
a newer registration while that callback still runs.

Insertion provides the strong exception guarantee across both containers. If
index insertion fails, the ordered record is removed and its callback destroyed
after unlocking before `std::bad_alloc` propagates. No-exception builds follow
their allocation runtime's fatal policy.

Callback captures follow normal `std::function` lifetime. Borrowed state must
outlive work that may already be claimed. Deletable adapters use shared or
deferred lifetime, generation checks, or synchronization excluding a claim.

### Scheduling and cancellation

Registration checks that the callback is nonempty, the deadline is in the
inclusive range `[-9,223,372,036,854,775, +9,223,372,036,854,775]`, the ID space
is open, and the service is running. This range converts exactly to the clock's
signed-nanosecond representation.

The service inserts the record under its lock. If it changes the earliest
auto-sync deadline, the condition is notified. An already-due alarm is queued;
the waiter can claim it after registration unlocks, but registration never
invokes it.

Cancellation removes an unclaimed record from both containers. It moves the
callback out while locked and destroys it after unlocking and restoring the
caller's signal mask. Claimed work is unaffected. Rescheduling means cancelling
and registering again, producing a new ID. Consumers add generation tokens when
stale callbacks must remain harmless after a race.

### Explicit pump points

`ProcessSystemTimeAlarms()` observes current uptime and drains everything due.
It adds no time. Plain reads, registration, cancellation, `system_time_sync()`,
and `system_time_lag_ns()` never invoke callbacks inline.

`system_time_delay_micros()` is the dispatching explicit advance. After checking
its final target, it advances through intervening deadlines. At each boundary it
publishes that uptime and drains every alarm then due before continuing. Work
registered by a callback at the same deadline follows older registrations.

In manual mode, work made due by `system_time_lag_ns()` waits for a later delay
or pump. In auto-sync mode, lag notifies the waiter. A peripheral task blocked on
its completion semaphore or notification only waits; it never pumps the queue.

### Autonomous auto-sync delivery

The first successful auto-sync registration starts one process-wide joinable
pthread. Startup is transactional and completes before the creating registration
returns. The creator temporarily blocks signals so the worker inherits an
all-blocked mask. A manual process creates no worker.

The condition variable is initialized with
`pthread_condattr_setclock(..., CLOCK_MONOTONIC)`. The worker repeatedly:

1. locks service state and exits if lifecycle is closing;
2. waits indefinitely while the queue is empty;
3. observes uptime and competes for delivery when the earliest alarm is due;
4. otherwise maps that deadline through the immutable auto-sync origin and
   performs an absolute monotonic timed wait; and
5. after notification, timeout, or spurious wake, starts the predicate check
   again.

The condition wait atomically releases and reacquires the host mutex, preventing
a missed state change between predicate inspection and sleep. Every operation
that can stale the wait changes state under that mutex and notifies the
condition. A wake never implies that an alarm is due; the predicate is always
rechecked.

If an explicit pump already owns delivery when the worker finds due work, the
worker waits on the condition for ownership or lifecycle to change instead of
repeating an already-expired timed wait. The owner notifies the condition when
it releases ownership.

The worker is independent of the FreeRTOS scheduler, so it can publish state and
request an interrupt while the selected task is CPU-busy. Its callbacks may
briefly acquire consumer locks but cannot wait on FreeRTOS, external completion,
public callbacks, or the interrupt they request.

`pthread_cond_timedwait()` is best-effort. The design avoids timerfd/eventfd
until measurements or a new signal-side ingress requirement demonstrate that
their additional Linux-specific machinery is needed.

### Dispatch and re-entry

Only one delivery owner invokes callbacks. It establishes ownership and its
explicit or wall-observed limit under the lock, claims the earliest due record,
unlocks to invoke and destroy it, then locks and rescans. It clears ownership in
the same critical section that confirms no due work remains.

A scope guard clears ownership on abnormal exit. Callbacks must not throw. In an
exception-enabled explicit pump, a violating exception releases ownership,
leaves later alarms queued, and propagates. At the native worker boundary the
same violation is fatal because there is no caller to receive it.

A recursive or concurrent pump records that a rescan is needed and returns
without invoking callbacks. The owner processes new work after the current
callback, so callbacks can safely register or cancel alarms and delivery never
nests. A concurrent explicit delay may advance to
its checked target; the owner resumes from the later uptime and never moves time
backward. Only the owner has a before-return drain guarantee.

### Service lifecycle

Lifecycle changes `running -> closing -> stopped` under the service lock. Two
internal functions expose the runner boundary without a public reset:

```cpp
// Task/native-only and nonblocking. False while delivery is active.
bool TryBeginSystemTimeServiceShutdownForHost();

// Native-only, after the FreeRTOS scheduler has returned.
void FinishSystemTimeServiceShutdownForHost();
```

Begin is forbidden inside the active callback. With no delivery owner it changes
the state to closing, rejects new registration, mutation, and drain entry, and
broadcasts the condition. A runner that receives false delays one FreeRTOS tick
and retries.

After the scheduler returns, finish joins a created worker, extracts unclaimed
callbacks, clears both containers, publishes stopped, and destroys captures
after unlocking. Cancellation retains its no-op/removal behavior during
shutdown. The ESP-IDF and FreeRTOS GTest runners use this sequence. The ordinary
Arduino sketch runner has no normal return path and relies on process teardown.

### Consumer adapters

Alarm callbacks run on the pump owner or native waiter, never in ISR context. A
hardware-like consumer:

1. validates its generation and materializes authoritative completion state;
2. completes externally visible publication preceding the interrupt;
3. releases alarm, peripheral, and publication locks; and
4. raises its logical interrupt source.

The [LEDC design](ledc_voltage_emulation.md#alarm-service-integration) applies
this sequence. The
[interrupt design](emulated_interrupts.md#alarm-and-peripheral-integration)
defines later ISR delivery. Future timer adapters retain public lifecycle,
periodic, and callback-context policy above this one-shot boundary. Consumers
keep high-frequency waveforms analytical rather than scheduling every edge.

## Proposed API

The C++ API is added to [`timer.h`](../roo_testing/system/timer.h) outside its
`extern "C"` block:

```cpp
using SystemTimeAlarmId = uint64_t;

SystemTimeAlarmId ScheduleSystemTimeAlarm(
    int64_t deadline_uptime_us, std::function<void()> callback);
void CancelSystemTimeAlarm(SystemTimeAlarmId id);
void ProcessSystemTimeAlarms();
```

Zero is invalid. Cancellation has no return because a boolean cannot safely
indicate whether concurrent execution began. Empty callbacks, invalid deadlines,
ID exhaustion, and post-shutdown registration terminate via `CHECK` before
state changes. Cancellation of zero, unknown, completed, claimed, or stopped IDs
is a no-op. All operations are task/native-only. No public framework timer API
is added.

## Implementation Plan

Authoring reference: follow this repository's
[C++ code-authoring guidance](../.github/instructions/embedded-cpp-code-authoring.instructions.md).

The completed [emulated-time clock implementation](emulated_time_clock.md#implementation-plan)
is a prerequisite.

### Phase 1: Deterministic manual-time alarms

Add the API, ordered records and index, cancellation, single-owner explicit
drain, chronological delay integration, and exception-safe callback lifetime.

Proposed commit: `Add deterministic system-time alarms`

Validation: cover future, due, past, equal, and extreme deadlines; cancellation
and claimed work; callback-created alarms; recursive and concurrent pumps;
multi-deadline delays; allocation rollback; throwing callbacks; and capture
destruction after unlock.

This phase is explicit-pump-only. No production alarm consumer lands yet.

### Phase 2: Autonomous auto-sync waiter

Add the joinable pthread, monotonic condition variable, predicate notifications,
transactional startup, delivery handoff, and two-part lifecycle seam. Manual
processes create no worker.

Proposed commit: `Wake system-time alarms from host monotonic time`

Validation: separate mode targets cover host delivery, concurrent first
registration, earlier insertion, cancellation retargeting, spurious and
coalesced notifications, exclusive delivery, startup failure, shutdown, and the
no-worker path. A FreeRTOS target proves eventual delivery while a task is
CPU-busy and task-local blocking while another task runs.

Until Phase 3, targets starting the worker use a dedicated lifecycle-aware main.

### Phase 3: Host-runner shutdown integration

Replace direct process exit in the ESP-IDF and FreeRTOS GTest runner tasks with
result capture and two-part shutdown. The runner retries begin with a one-tick
delay, ends the scheduler after success, and lets native `main()` finish the
service and return the result.

Proposed commit: `Shut down system-time alarms from host runners`

Validation: exercise all three runner paths, including a lower-priority owner
preempted between callbacks. Prove that the owner finishes, the worker joins,
and no alarm capture remains live.

## Testing Plan

Manual tests cover deterministic ordering, cancellation, lifetime, non-inline
registration, chronological advancement, and non-nesting delivery. Auto tests
cover startup, monotonic waits, retargeting, autonomous delivery, and shutdown.
Lifecycle cases use isolated processes because stopping the service is
irreversible.

FreeRTOS integration covers CPU-busy preemption and task-local blocking.
Wall-time assertions require no early delivery and eventual delivery, not
microsecond latency. Optional diagnostics record unloaded wake lateness so any
future transport change can be measurement-driven.

Tests use relative deadlines from observed starting uptime, retain and cancel
remaining IDs, quiesce worker-visible state, and never switch time mode.

## Caveats

Callbacks can observe a later uptime when advancement or host scheduling passes
their deadline before they acquire ownership. Microsecond timestamps do not
promise microsecond delivery resolution. Auto latency includes the condition
timeout, pthread scheduling, earlier callbacks, and interrupt masking.

A blocking callback stalls its pump or the only worker and all callbacks behind
it. An already-due callback can be claimed before registration returns, so
callers establish state and lifetime first. Cancellation does not wait for
claimed work; deletable consumers require generation checks and shared or
deferred lifetime.

### Rejected Alternatives

#### Keep the queue private to LEDC

A shared boundary avoids duplicating ordering, cancellation, and clock
integration in later timer and peripheral shims.

#### Use FreeRTOS software timers or the scheduler tick

They follow the host tick rather than manually driven uptime. The 1 kHz signal
handler also cannot safely run the queue, while a follow-up task remains subject
to scheduler priority and starvation.

#### Use timerfd and eventfd for the native waiter

They add Linux-specific descriptors, polling, cleanup, startup rollback, and
wake-coalescing rules. The current service needs one retargetable deadline and
task/native notifications, which a monotonic condition variable provides
directly. No measured latency requirement justifies the additional transport.

If later measurements show a concrete latency problem, or a separately designed
signal-side ingress needs a descriptor wake, the transport can change without
altering queue or delivery semantics.

#### Dispatch from every clock read

Clock reads occur in logging and observation paths. Dispatch would introduce
surprising re-entry and violate the clock's passive ISR-safe contract.

#### Reimplement public `esp_timer` semantics

The vendored common implementation already owns handles, periodic policy, and
TASK/ISR dispatch. A future backend should emulate its hardware boundary.

#### Add periodic alarms to the core

Periodic APIs must decide drift, missed periods, overrun, restart, and sleep
behavior. Those policies remain with consumers.

## Future Work

- Implement LEDC fade completion over the alarm and interrupt services.
- Design an `esp_timer` backend around the vendored common implementation.
- Specify Arduino hardware-timer and GPTimer adapters over these boundaries.
