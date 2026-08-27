# Emulated-time alarms

Status: Proposed

## Objective

Provide cancellable one-shot callbacks at absolute roo_testing uptimes, with
deterministic explicit delivery in manually driven time and autonomous
best-effort delivery while uptime follows the host monotonic clock.

## Motivation

Emulated peripherals need to finish asynchronous work when fake time reaches a
deadline. Implementing a private timer queue in each shim would duplicate
ordering, cancellation, concurrency, and callback-lifetime logic. Advancing the
global clock from a blocking peripheral call is also incorrect: it changes time
for every task and interacts badly with wall-clock auto-sync.

A shared alarm primitive lets peripherals describe completion deadlines while
leaving time progress under the existing system clock and test driver. In
wall-synchronized mode it must also observe an otherwise CPU-busy system and
initiate delivery; the interrupt controller cannot preempt a task until some
deadline source actually asserts an interrupt.

## Background

[`timer.cpp`](../roo_testing/system/timer.cpp) owns roo_testing's intended
monotonic uptime. Its `EmulatedTime` can follow host elapsed time or, with
auto-sync disabled, be advanced explicitly by tests. Arduino
`micros()`/`millis()` and ESP-IDF `esp_timer_get_time()` use this same clock.
The current timer has no callback queue, uses `high_resolution_clock` even
though C++ does not guarantee that clock is steady, has unsynchronized mutable
state, and does not check all unsigned lag/delay conversions for overflow.
Clock reads are also reachable from framework ISRs. In particular, Arduino
marks `micros()` as ISR-callable. Once interrupts can preempt ordinary host
code, a clock read must not take a mutex that the interrupted code might hold.

The Linux FreeRTOS port receives a preemptive scheduler tick through
`SIGALRM`, currently at 1 kHz. That handler advances the FreeRTOS tick and runs
the application tick hook, but it does not sample roo_testing uptime or pump
system-time alarms. The tick can interrupt CPU-busy code, but it is neither the
deadline source nor a safe place to lock, allocate, or invoke alarm handlers.

ESP-IDF has several timer facilities rather than one universal alarm API. The
closest generic API is [`esp_timer`](../roo_testing/frameworks/esp-idf/components/esp_timer/include/esp_timer.h),
which provides one-shot and periodic software timers. Its [vendored
implementation](../roo_testing/frameworks/esp-idf/components/esp_timer/src/esp_timer.c)
keeps armed timers ordered by absolute alarm time and separates task and
optional ISR dispatch. Classic ESP32 uses the lower-level [LAC
implementation](../roo_testing/frameworks/esp-idf/components/esp_timer/src/esp_timer_impl_lac.c);
newer Espressif targets generally use the [SYSTIMER
implementation](../roo_testing/frameworks/esp-idf/components/esp_timer/src/esp_timer_impl_systimer.c).
Both program one hardware compare and allocate an interrupt whose lower ISR
clears peripheral interrupt status and invokes the common
`timer_alarm_handler`.

When ISR dispatch is configured, the common layer has separately ordered TASK
and ISR lists and caches both heads behind the one physical compare. The upper
handler runs all due `ESP_TIMER_ISR` callbacks first. Only an interrupt that
processed no ISR timer notifies the dedicated timer task for `ESP_TIMER_TASK`
callbacks; coincident ISR and TASK heads therefore require a follow-up
interrupt for TASK delivery. The current roo_testing host configuration does
not enable `CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD`, so its public header
currently exposes TASK dispatch only. ESP-IDF components use the service for
work including [touch
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
2. Scheduling, cancellation, clock mutation, and dispatch are safe from
   multiple native host threads and FreeRTOS tasks. These management operations
   are explicitly not POSIX-signal- or ISR-safe. A FreeRTOS task cannot be
   preempted while it owns a host mutex shared with the native waiter.
3. A passive uptime read is monotonic, lock-free, nonblocking, and safe from an
   emulated ISR; it never allocates or sleeps.
4. An alarm never runs inline from scheduling, including when its deadline is
   already due.
5. Due alarms run in deadline order; alarms at one deadline run in registration
   order.
6. An owning explicit fake-time delay crosses deadlines chronologically before
   returning. If another drain owns delivery, advancement does not nest or move
   uptime backward, and the owner later dispatches the due handlers.
7. With auto-sync disabled, alarms run only at documented explicit pump points
   and the service never advances uptime on its own.
8. With auto-sync enabled, a due alarm becomes eligible without application
   polling, even when the selected FreeRTOS task is CPU-busy. Wall-synchronized
   delivery is best-effort and never promises a hard latency bound.
9. Alarm handlers are short and non-throwing by contract and never wait for a
   task, interrupt, or external completion. Handler invocation and destruction
   of captured state occur outside the timer mutex, handler re-entry does not
   create a nested dispatch stack, and an escaping host exception cannot strand
   drain ownership.
10. Cancellation prevents an unclaimed callback. Cancelling executing,
   completed, or unknown work is a no-op.
11. An alarm scheduled or made due by a callback is not stranded after the
   active drain relinquishes ownership.
12. The core primitive imposes no ESP-IDF ISR, task affinity, priority, or
    periodic-timer semantics on its consumers.
13. Tests can cancel their own pending alarms and restore auto-sync without a
    global reset that invalidates unrelated registrations.
14. Host-time observation and explicit clock mutation preserve monotonic uptime;
    numeric conversions and additions fail before overflow.

### Out of scope

- Implementing the public `esp_timer`, GPTimer, or Arduino hardware-timer APIs.
  Those are adapters and follow-on work.
- FreeRTOS software timers, task delays, or `roo_scheduler` jobs.
- Implementing interrupt delivery itself, which belongs to the separate
  [emulated interrupt design](emulated_interrupts.md). An alarm consumer may
  use that implemented service.
- A built-in periodic-alarm API. Periodic adapters reschedule one-shot alarms
  according to their own missed-period policy.
- Hard-real-time callback latency or faithful sustained delivery of very short
  periods on a general-purpose host OS.
- RTC, Unix-time, calendar, or deep-sleep wake alarms.
- Scheduling periodic waveform edges; voltage signals remain analytical.

## Design Overview

Split `EmulatedTime` into a lock-free clock-read path and coordinated alarm
state. A lock-free atomic publishes monotonic uptime. An atomically versioned
auto-sync mapping lets an ISR derive a candidate uptime from
`CLOCK_MONOTONIC` and publish only a larger value, without taking a mutex or
spinning on an interrupted writer. Explicit mutation, alarm records, ID
allocation, and drain ownership remain mutex-protected.

The alarm state contains an ordered collection of value-owned records and an
ID index for cancellation. Scheduling inserts work but never invokes it. In
manual mode, `system_time_delay_micros()` and
`ProcessSystemTimeAlarms()` are the dispatch points. In auto-sync mode, one
process-wide native waiter sleeps until the earliest mapped host deadline and
then competes for the same non-nesting drain ownership. It observes wall time;
it never advances manually driven time.

A drainer claims one due alarm, releases the mutex, invokes it, and then
rescans. The explicit callers and native waiter share this path, making
callback re-entry and cancellation safe without nested delivery.

Consumers adapt this neutral deadline mechanism to their own behavior. The
[LEDC adapter](ledc_voltage_emulation.md#alarm-service-integration) uses it to
materialize fade completion and enqueue final GPIO publication followed by a
post-publication interrupt continuation. That continuation publishes ISR state
and raises the LEDC source; the LEDC ISR releases the channel gate and invokes
the registered callback. A future host `esp_timer_impl` backend uses one alarm
as its emulated hardware compare and raises a profile-selected timer source;
the vendored common `esp_timer` layer retains ownership of public handles,
ordering, periodic policy, and TASK/ISR dispatch. Arduino timer or GPTimer
adapters can translate counter values and raise their own logical sources.

| Requirements | Design element |
| --- | --- |
| 1, 4-5, 10 | Ordered alarm records plus an ID index |
| 2 | Signal-masked host locking for management and alarm state |
| 3, 14 | Atomic monotonic uptime and versioned auto-sync mapping |
| 6-8 | Explicit manual pumps plus one auto-sync deadline waiter |
| 9, 11 | Scope-guarded, invoke-outside-lock non-nesting drain |
| 12 | Neutral one-shot callback contract with consumer adapters |
| 13 | Per-alarm teardown without a global alarm reset |

## Design Details

### Lock-free clock-read path

Represent uptime internally as signed nanoseconds in a lock-free atomic and
convert to microseconds at the API boundary. The implementation statically
asserts that the chosen atomic representation is always lock-free on the host.
`system_time_get_micros()` loads this published uptime and, when auto-sync is
enabled, samples `CLOCK_MONOTONIC`, applies the published host-to-emulated
offset, and uses a compare/exchange maximum to publish only forward progress.
It does not take the alarm mutex, allocate, dispatch, or sleep.

Publish the auto-sync mapping as lock-free scalar fields bracketed by an atomic
publication epoch. A writer holds the timer mutex, publishes an odd epoch,
updates the enabled flag and host-to-emulated offset, and publishes the next
even epoch. A reader makes exactly one bounded snapshot attempt: load an even
epoch, sample `CLOCK_MONOTONIC` and the mapping fields, and reload the epoch. It
uses the wall-derived candidate only when the two equal even samples match;
otherwise it returns the already published uptime without retrying. This is a
nonblocking published snapshot, not a conventional spinning seqlock: an ISR
may have interrupted the writer while the epoch is odd.

Enabling auto-sync maps the current published uptime to the current host
monotonic instant without moving uptime. Disabling it first publishes any
wall-derived progress and then changes the configuration generation. A read
that linearized before that change may complete afterward, but its sampled
candidate cannot be later than the disable operation. Explicit mutations also
use compare/exchange loops so they cannot overwrite concurrent forward
progress with an older value. The atomic uptime is the only authoritative
current-time value; the mutex side must reconcile it rather than retaining a
second stale `emu_uptime_`.

All fast-path atomics are constant-initialized and individually asserted
lock-free. The ISR path does not enter a function-local static initialization
guard or start the waiter. A constant-initialized atomic state establishes the
default auto-sync mapping with a bounded `uninitialized -> initializing ->
ready` transition. The first reader that wins one compare/exchange samples
`CLOCK_MONOTONIC`, publishes the mapping, and release-publishes `ready`; a
concurrent or nested ISR that observes `initializing` returns the published
uptime floor without waiting. Slow paths ensure initialization has completed
before mutating the mapping.

Use sequentially consistent operations for the publication epoch and mapping
fields. The writer publishes an odd epoch, then the fields, then the next even
epoch. The reader loads the epoch, fields, and epoch again in that order and
accepts only matching even samples. This deliberately conservative ordering
makes it impossible for a reader that validates the old epoch to accept fields
from a later mapping. Publishing a wall-derived uptime also stays bounded: a
reader makes at most one strong compare/exchange attempt. If it loses, it
returns the newer published floor and lets a later read finish catching up
rather than looping in ISR context.

Passive reads no longer perform ahead-of-wall sleeping. Pacing belongs to
`system_time_sync()` and dispatching explicit delays, which can sleep outside
the timer mutex and then re-evaluate the mapping. This keeps ISR reads safe and
prevents logging or sampling from unexpectedly blocking.

### Slow-path locking

The Linux FreeRTOS port represents every task with a pthread and can switch the
selected task from a signal handler. An ordinary `std::mutex` alone is unsafe:
if task A is preempted while holding it and the newly selected task B blocks on
the same mutex, the port still considers B selected and may never resume A.

Every task/native slow-path acquisition therefore uses a small reusable
roo_testing-internal RAII guard that saves the caller's POSIX signal mask,
blocks all maskable signals, and only then locks the host mutex. The operation
performs any required nonblocking wake-eventfd write as part of the locked state
transition, then unlocks and restores the saved mask. Blocking all signals keeps
the system package
independent of the port's private signal numbers while preventing a selected
FreeRTOS pthread from being switched away inside libc synchronization code. A
task may briefly wait for the native worker to release the mutex, but the worker
is independently scheduled and can make progress. The worker inherits an
all-blocked signal mask for its lifetime.

No callback invocation, callback-capture destruction, allocation retry, host
sleep, descriptor poll, or thread join occurs while an ordinary slow path owns
this guarded mutex. The implementation tests a low-priority task being
preempted around timer mutations while a higher-priority task and the native
worker contend for the same state; the test must make progress without
priority-inversion deadlock.

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

IDs increase monotonically, reserving zero as invalid, and are never reused in
one process. Attempting to allocate after the maximum value is a fatal invariant
violation. This also prevents an ID whose record has been claimed and removed
from the index from colliding with a new registration while its callback still
runs.

Insertion provides the strong exception guarantee across both containers. If
ID-index insertion fails after the ordered record was inserted, scheduling
erases that record and destroys its callback outside the mutex before
propagating or reporting the allocation failure. Destruction occurs after the
slow-path guard has also restored the caller's signal mask; scheduling never leaves an
uncancellable partial registration.

The callback owns its captures according to normal `std::function` rules.
Borrowed state must outlive any handler execution that may already have been
claimed; cancellation alone cannot establish that. Deletable adapters use
shared or deferred lifetime, or external synchronization that excludes a
concurrent claim.

### Scheduling and cancellation

Scheduling `CHECK`s that the callback is non-empty, allocates an ID, and inserts
the record while holding the timer mutex. It then wakes the native waiter if
the earliest deadline or its host mapping changed. An already-due deadline is
queued normally; scheduling never calls user code inline. In auto-sync mode the
waiter may nevertheless claim it concurrently before the scheduling call
returns.

Scheduling and cancellation may run from an ordinary native host thread or a
FreeRTOS task. They must not run in POSIX-signal or emulated ISR context because
they lock and may allocate or destroy captured state.

Cancellation removes an unclaimed record from both containers. Move the
callback out while locked and destroy it after unlock so captured-object
destructors cannot re-enter the timer under its mutex. The slow-path guard also
restores the original signal mask before destruction. Once a drainer has
claimed a record, cancellation is a no-op.

An alarm ID identifies one registration only. Rescheduling is expressed as
cancellation followed by a new schedule and therefore produces a new ID.
Peripheral adapters use their own generation tokens when stale callbacks must
also be harmless after a race with cancellation.

### Time advancement and pump points

`ScheduleSystemTimeAlarm()` and `CancelSystemTimeAlarm()` never readjust
uptime. Plain clock reads never invoke handlers inline. `system_time_sync()`
and `system_time_lag_ns()` also never invoke handlers inline, although in
auto-sync mode any newly due work may be claimed concurrently by the native
waiter.

Use Linux `CLOCK_MONOTONIC` as the host elapsed-time source and never publish an
observed value below current uptime. Both `system_time_delay_micros()` and
`system_time_lag_ns()` perform checked unit conversion and duration addition;
an unrepresentable mutation fails a `CHECK` before changing state.
Deadline comparisons do not blindly multiply an arbitrary signed-microsecond
deadline into nanoseconds. The waiter uses checked conversion and treats an
unrepresentably distant deadline as farther than any host wait it can arm,
rechecking after notifications or bounded long waits.

`system_time_delay_micros()` remains the dispatching explicit-advance
operation. It walks through intervening alarm deadlines. At each boundary it
sets uptime, drains every alarm then due, and continues to the final target.
Alarms registered for the same deadline by a callback join the active drain
after previously registered work. The lower-level `system_time_lag_ns()`
does not run callbacks itself. With auto-sync disabled, work it makes due waits
for a later delay or explicit pump. With auto-sync enabled, it wakes the waiter
because hardware time has become due even though the mutating caller remains a
non-dispatching path.

`ProcessSystemTimeAlarms()` first observes current uptime using the normal
auto-sync behavior and drains everything due at that observed time. It does not
add time itself. If wall-clock synchronization moved uptime past several
deadlines, callbacks still run in deadline/registration order but observe the
current, possibly later, uptime.

With auto-sync disabled, a test or another task must advance time and pump it.
A consumer such as an LEDC channel wait may call
`ProcessSystemTimeAlarms()` at a documented retry boundary, but that is an
integration decision, not another source of time advancement.

### ISR-visible busy delays

Arduino marks `delayMicroseconds()` as ISR-callable, and ESP-IDF low-level code
may call `esp_rom_delay_us()` or `ets_delay_us()` from an ISR. Those shims must
not route an emulated ISR into `system_time_delay_micros()`, which mutates the
clock, locks slow state, and pumps alarms. They check `xPortInIsrContext()` and
use a separate internal `system_time_busy_wait_micros()` path in ISR context.

The ISR path uses only `clock_gettime(CLOCK_MONOTONIC)`, lock-free arithmetic,
and a host busy loop. It does not allocate, lock, publish fake uptime, or pump
alarms. In auto-sync mode the elapsed host time is observed later by the normal
clock reader and deadline waiter. In manual mode fake uptime intentionally
remains under the test driver's control. Scheduler and simulated-interrupt
signals remain deferred until the current ISR returns, matching the fact that a
busy ISR occupies its emulated core. Task-context calls retain the existing
dispatching fake-delay behavior.

### Autonomous auto-sync delivery

One process-wide native deadline waiter supplies the event that real timer
hardware would otherwise generate. It starts lazily from a dynamic scheduling,
auto-sync-enabling, or fixed-compare-source registration slow path, never from
the lock-free clock-read path. Before inserting the first auto-synchronized
alarm, enabling auto-sync with queued work, or publishing a fixed source handle,
the service successfully creates its descriptors and waiter. Fixed-source
registration does this even in manual mode; the worker then remains dormant but
the signal-safe publication transport is valid. Thread construction uses an
RAII signal-mask guard; failure restores the creator's mask and fails fatally
before publishing state that falsely promises autonomous delivery.

A mutex-protected wake generation is incremented for queue changes, clock-mode
or mapping changes, explicit time mutations, drain release or exceptional
cleanup, and shutdown. Before unlocking, the changer performs a nonblocking
eight-byte `write()` to a Linux eventfd. `EAGAIN` means a wake is already
pending and is safely coalesced; `EINTR` gets one bounded retry, and any
remaining error sets a lock-free fatal-status field. Persistent queue state plus
the generation means eventfd counter coalescing cannot lose the reason to
recompute. Ordinary slow paths restore the signal mask and fail immediately on
that status. A signal-side publisher cannot rely on a later task or worker wake,
so after the bounded retry it emits a fixed diagnostic with `write()` and exits
through an async-signal-safe fatal path; it never silently leaves the sole wake
pending only in memory.

The worker owns a `timerfd_create(CLOCK_MONOTONIC, ...)` descriptor and polls it
together with the wake eventfd. While auto-sync is enabled, it maps the earliest
emulated deadline to an absolute host instant and programs the timerfd with
`TFD_TIMER_ABSTIME`. A wake-event read forces a full recomputation and retarget;
a timer-event read samples and publishes wall-derived uptime, then competes for
ordinary drain ownership. It always rechecks generation, mode, and earliest
work after reacquiring the timer mutex. Early/stale readiness simply disarms or
retargets and polls again, so a wall-time callback is never intentionally
delivered before its emulated deadline. Absolute programming avoids accumulated
drift and wall-clock adjustments.

While auto-sync is disabled, the worker disarms the timerfd and waits only for
control/shutdown eventfd wakes. It cannot change uptime or dispatch queued
deadlines. Disabling auto-sync does not revoke a callback already claimed by
another drainer; adapters requiring stronger teardown use their own generation
and lifetime synchronization.

The waiter blocks all maskable signals in its native pthread. Thread creation
temporarily controls the creator's signal mask so the waiter inherits that
mask without adding a FreeRTOS dependency. Descriptor and thread startup is
transactional: eventfd, timerfd, or thread failure restores the creator mask,
closes partial resources, and fails before promising autonomous delivery. The
worker is never detached. An internal, testable service-lifecycle seam
transitions `running -> closing -> stopped` under the timer mutex. New schedule,
mutation, or drain entry while closing/stopped fails a `CHECK` before changing
state; the return-only-ID scheduling API never fabricates a recoverable failure
result.
Passive reads remain available, and cancellation retains its public no-op
contract: during closing it may remove an unclaimed record, while after
extraction/stopping it safely finds nothing. This also lets a pending callback
capture destructor cancel related IDs during shutdown without aborting.
Shutdown is forbidden from the active drainer itself because waiting for that
drain would self-deadlock. After entering `closing`, it wakes the worker and
waits for both worker exit and `drain_active == false`, including an explicit
pump that may already have claimed a callback on another thread. Only then does
it move all unclaimed callbacks out, clear both the ordered queue and ID index,
mark the process-stable service object stopped, and destroy the captures after
unlock and signal-mask restoration. It closes timerfd/eventfd only after the
worker and every registered signal-side compare producer are quiescent.

Production framework shutdown invokes the same path before static consumer
teardown. Its caller first quiesces other FreeRTOS tasks that can enter the
timer, while consumer adapters cancel and quiesce their registrations; the
system package therefore need not block the selected pthread waiting for a
suspended FreeRTOS task. Dedicated lifecycle tests race shutdown with both the
worker and an explicit native drainer, verify the active-drainer rejection, and
cover the production shutdown route without exposing a public global alarm
reset.

The waiter executes only internal alarm handlers. A handler may run on this
native thread, so it must be thread-safe and short and must translate promised
public task or ISR delivery through the relevant adapter. It may briefly
acquire adapter state locks, but it must not wait on a FreeRTOS primitive,
external completion, or the interrupt it raises. The waiter does not depend on
the FreeRTOS scheduler and can still materialize state and assert an emulated
interrupt while the selected FreeRTOS task is CPU-busy.

### Dispatch and re-entry

Only one drainer owns callback dispatch at a time. It performs this loop:

1. Coordinate drain ownership and the applicable explicit or wall-observed
   limit under the timer mutex.
2. Synchronize or advance to that limit and remove the earliest due
   record from both containers, marking it claimed.
3. Release the mutex and invoke the callback.
4. Destroy the callback outside the mutex, then lock and rescan.
5. When no due work remains, clear drain ownership and confirm the queue state
   in the same critical section.

A scope guard owns the drain marker for the entire loop. On normal completion,
step 5 clears ownership and disarms the guard in the same critical section, so
the guard cannot later clear a new drainer's ownership. On abnormal exit, the
guard clears ownership under the mutex. Alarm handlers must not throw. If a
handler nevertheless throws during an explicit pump in a host build with
exceptions enabled, its local `std::function` is destroyed outside the mutex,
the guard releases drain ownership, and the exception propagates. Remaining
alarms stay queued for a later pump; the drainer does not catch the exception
and continue invoking unrelated handlers. The native waiter catches at its
thread boundary and terminates with a diagnostic because no caller exists to
receive the contract violation.

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
the waiter reacquires the mutex and re-evaluates the clock generation, drain
ownership, and earliest alarm rather than relying on a stale snapshot.

### Consumer adapters

The core alarm callback executes on the native waiter or whichever host thread
or FreeRTOS task explicitly pumps time. It must not directly invoke a public
callback that promises a task or ISR context. Consumers add that dispatch layer
after releasing alarm and peripheral locks:

- The [LEDC adapter](ledc_voltage_emulation.md#alarm-service-integration)
  materializes generation-checked completion state and appends an ordered pair:
  final GPIO publication, then an internal completion finalizer. The finalizer
  publishes the ISR mailbox/status and raises the LEDC source only after the
  GPIO and sink calls have returned. Its emulated ISR releases the channel
  gate, invokes the registered callback, and requests any `FromISR` yield. After
  unlocking, the alarm handler attempts the non-waiting LEDC drain so an
  autonomous completion cannot remain queued when no application call follows.
- The future `esp_timer` integration builds the vendored common
  [`esp_timer.c`](../roo_testing/frameworks/esp-idf/components/esp_timer/src/esp_timer.c)
  and `esp_timer_impl_common.c` and supplies a roo_testing
  `esp_timer_impl_*` backend. The backend represents the minimum cached hardware
  compare with one internal fixed system-time compare source. Expiry publishes
  timer interrupt status and raises the profile-selected LAC/SYSTIMER source;
  its lower ISR clears status and invokes the vendored `timer_alarm_handler`.
  The upstream layer therefore retains handle lifetime, separate TASK/ISR
  ordering, periodic catch-up and skip policy, minimum-period clamping,
  callback re-entry, deferred deletion, timer-task notification, yield latching,
  and next-alarm queries.
- The backend includes a signal-safe compare-rearm mailbox from its first
  `ESP_TIMER_TASK` phase. This is required even before public `ESP_TIMER_ISR`
  callbacks are configured: upstream marks start, stop, and restart operations
  as IRAM-safe, framework ISR paths call them, and those operations can call
  `esp_timer_impl_set_alarm_id()`. Under the upstream `s_time_update_lock`, that
  function updates `timestamp_id[id]`, computes the minimum TASK/ISR head
  (`UINT64_MAX` means disarmed), and passes that deadline and a new source
  generation to the source's signal-safe publish operation. That operation
  admits the writer, publishes the atomic compare snapshot, and performs a
  nonblocking wake on the alarm service's eventfd before releasing the writer
  claim. It never calls the dynamic mutex/allocating alarm API.

  The fixed source is a follow-on internal extension, not part of the public API
  proposed here. It has a task-registered, process-stable callback record and an
  ISR-published atomic `{deadline, generation, armed}` snapshot. Both the native
  waiter and every explicit delay/pump merge registered source snapshots with
  the dynamic queue before choosing the next deadline. Claim is a
  generation-checked CAS, so a changed/disarmed source suppresses a stale
  expiry. After invoking a claimed source, the drainer rescans; an ISR-side
  rearm that happened during delivery is therefore visible immediately.

  Registration transactionally starts the alarm transport before publishing an
  accepting source. Signal-side publication first claims a statically asserted
  lock-free packed `{service_epoch, accepting, writer_count}` word, loads the
  eventfd only from the process-stable service record, publishes the compare
  snapshot and writes the wake while still counted as a writer, and only then
  release-decrements the count. It never caches an unprotected raw descriptor.

  Unregister uses `accepting` to gate both publication and drainer claims. Under
  the timer mutex it first clears `accepting`, so no new writer or drainer can
  enter, then releases the mutex and waits for already-admitted writers to
  leave. It reacquires the timer mutex, performs a final generation increment
  and disarm after those publications, and removes the now non-claimable source.
  After unlocking, it waits for handlers claimed before admission closed to
  finish. Only then does unregister return. Only after every fixed source is
  unregistered may service shutdown close the eventfd. This prevents late
  publication, uninitialized-descriptor, close/write, and descriptor-reuse ABA
  races.

  This direct ingress is essential in manual mode. An ISR can start/restart a
  TASK timer and return immediately before the task advances fake time; the one
  explicit pump must already see that compare rather than wait for an
  asynchronous reconciler to allocate a dynamic alarm. Before an explicit pump
  linearizes its final drain release under the timer mutex, it performs a stable
  generation pass and leaves no source published before that point both due and
  unclaimed. A signal delivered after mask restoration but before the C++ call
  physically returns is a later publication. Concurrent native time drivers
  synchronize their intended ordering with framework task/ISR operations, as
  they must for deterministic manual-time tests.
- Initial public `esp_timer` support otherwise matches the current host
  configuration and implements `ESP_TIMER_TASK`. Enabling `ESP_TIMER_ISR` is a
  separate configuration phase for the second upstream list, direct ISR
  callbacks, yield behavior, and their tests--not the point at which signal-safe
  rearming first becomes necessary. There is no total registration order across
  TASK and ISR lists; when both heads coincide, upstream dispatches ISR work
  before the follow-up interrupt that wakes TASK delivery.
- The host does not currently execute ESP-IDF's linker-collected
  `ESP_SYSTEM_INIT_FN` entries, so merely building the upstream sources would
  leave the timer task and interrupt uninitialized. The future adapter adds an
  idempotent host wrapper used by the ESP-IDF runner, Arduino runner, and
  FreeRTOS test main. Each process calls `esp_timer_early_init()` before starting
  the scheduler. Its startup task then calls `esp_timer_init()` before
  `app_main`, tests, or `initArduino()`; Arduino's current pre-scheduler
  `initArduino()` call moves into that task before `setup()`. Only the full init
  needs this timing, because the current host interrupt allocator requires a
  running FreeRTOS task. Wrapper idempotence does not change the public API:
  a second direct `esp_timer_init()` still returns `ESP_ERR_INVALID_STATE`.

  The host sdkconfig also selects the current single-core timer interrupt
  profile with `CONFIG_ESP_TIMER_ISR_AFFINITY_CPU0`; otherwise the vendored
  source cannot define its init mask. A later SoC profile changes this config
  rather than baking ESP32 affinity into the generic backend.
- Framework shutdown first requires clients to stop/delete every timer and
  checks `esp_timer_deinit()`. If either upstream timer list still contains an
  active timer or queued delete event, deinit returns `ESP_ERR_INVALID_STATE`
  and all services remain alive. On success, upstream calls the backend's
  `esp_timer_impl_deinit()` before deleting the timer task. That backend method
  itself synchronously invalidates the compare generation, disables and
  quiesces its interrupt, disarms and quiescently unregisters the fixed compare
  source, and waits for any already-claimed source handler before returning.
  Source generations are never reset across deinit/reinit, and claimed handlers
  retain process-stable/shared backend state, so cancellation cannot create a
  stale-callback use-after-free. Any partially failed init rolls back the same
  source/interrupt resources before reporting failure. Only after successful
  `esp_timer_deinit()` may framework shutdown stop the generic interrupt and
  alarm services.
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

Of the clock-observation/mutation API, only `system_time_get_micros()` is
ISR-safe. `system_time_sync()`, `system_time_lag_ns()`,
`system_time_delay_micros()`, `system_time_set_auto_sync()`, and every alarm
management function are task/native-only. The internal
`system_time_busy_wait_micros()` helper described above is separately
ISR-safe, but deliberately does not readjust or dispatch fake time.

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
without allocating an ID. All three functions are ordinary native-thread or
FreeRTOS-task operations and are not callable from an emulated ISR.

No public ESP-IDF or Arduino timer API is added in this design.

## Implementation Plan

Authoring references: follow this repository's
[design-authoring guidance](../.github/instructions/embedded-design-doc-authoring.instructions.md),
[C++ code-authoring guidance](../.github/instructions/embedded-cpp-code-authoring.instructions.md),
and adjacent system-timer/test conventions.

### Phase 1: ISR-safe monotonic uptime and host locking

Refactor `system/timer.h/.cpp` around the constant-initialized atomic uptime and
bounded initialization/publication protocol. Use
`clock_gettime(CLOCK_MONOTONIC)` for the signal-safe host sample, move
ahead-of-wall pacing out of passive reads, add a reusable internal
signal-masking host-mutex guard, and add checked conversions and arithmetic.
Apply that guard immediately to `FakeGpioPin`,
`SimpleVoltageSink`, and `SimpleDigitalSink` retained-signal mutexes as well as
the clock slow path. The preemptive Linux scheduler already makes those shared
locks unsafe when a selected task can be suspended while owning one; this
hardening is a prerequisite for steady LEDC publication, not a consequence of
adding the native alarm waiter. Route ISR calls to Arduino/ROM microsecond delay
through the separate host busy-wait path. Do not add alarms or a worker in this
phase.

Proposed commit: `Make emulated time and shared host locks ISR-safe`

Validation: add pure-host clock tests for monotonic publication, auto-sync
transitions, ahead-of-wall pacing, concurrent lag/delay/read operations, and
failure-before-mutation overflow. Cover first-call and interrupted
initialization, mixed-mapping rejection, and bounded CAS contention. Add a
FreeRTOS integration test that repeatedly reads `micros()`/`esp_timer_get_time()`
and uses each ISR-visible busy-delay shim in emulated ISR context while task and
native contexts mutate the slow clock state; it must neither deadlock nor
regress. A priority-inversion regression has low- and high-priority tasks plus a
native contender exercise the clock, fake-GPIO, and built-in sink locks.

### Phase 2: Deterministic manual-time alarms

Add the C++ alarm API, ordered records and ID index, cancellation, explicit
single-drainer dispatch, chronological delay integration, rescan generation,
and exception-safe callback lifetime. Add a private allocation-failure
failpoint so rollback across the ordered queue and ID index is testable. Add
focused pure-host tests and BUILD dependencies. Clock reads remain outside the
alarm mutex.

Proposed commit: `Add deterministic system-time alarms`

Validation: with auto-sync disabled, cover future/due/past and equal deadlines,
cancellation and claimed work, callback-created alarms, recursive/concurrent
pumps, explicit delays across multiple deadlines, container-allocation failure,
checked extreme deadlines, throwing explicit handlers, and destruction outside
the mutex. Every test cancels its remaining IDs and restores auto-sync.

The safe interim state after this phase has no autonomous delivery: even with
auto-sync enabled, alarms require `ProcessSystemTimeAlarms()` or a dispatching
explicit delay. No LEDC fade or public timer consumer lands until Phase 3.

### Phase 3: Autonomous auto-sync waiter

Add the one process-wide native waiter, absolute monotonic timerfd, wake
eventfd/generation, signal-mask setup, startup rollback, drain handoff, and
explicit stop/join lifecycle seam. Keep it dormant in manual mode and keep the
system package independent of FreeRTOS and the interrupt controller.

Proposed commit: `Wake system-time alarms from host monotonic time`

Validation: verify that wall time alone fires a future alarm, manual mode never
does, enabling resumes a queued alarm, earlier insertion and cancellation
retarget a far wait, no coalesced eventfd wake is lost, explicit and native
drainers never overlap, and shutdown quiesces the worker. In a separate
FreeRTOS integration test, have an internal alarm handler assert a generic
interrupt and verify it preempts CPU-busy task code without an explicit pump.
Characterize unloaded wake lateness but assert only no-early-fire and eventual
delivery, not a host-specific latency ceiling.

After the shared host-lock hardening in Phase 1, [steady LEDC
publication](ledc_voltage_emulation.md#implementation-plan) may proceed in
parallel with alarm Phases 2-3. LEDC fade Phase 3 is the first downstream
consumer that requires all alarm phases plus the implemented interrupt
controller. Public `esp_timer` support follows as a separate design and build
slice using the vendored common upper layer and a host `esp_timer_impl_*`
backend, not as part of the generic alarm commit.

## Testing Plan

Pure-host tests separately cover the atomic clock, deterministic alarm queue,
and autonomous waiter. Together they exercise monotonic ISR-compatible reads,
checked mutations, mode transitions, deadline/registration ordering,
cancellation and lifetime races, non-inline scheduling, explicit chronological
advancement, single-owner re-entry/concurrency, waiter retargeting and shutdown,
and invoke/destroy-outside-lock behavior. Exception-enabled manual tests verify
that a throwing handler releases drain ownership and leaves unclaimed work for
the next pump.

FreeRTOS integration tests cover ISR clock reads and the full native-deadline to
logical-interrupt path against CPU-busy code. Timing tests verify no early fire
and eventual delivery. They do not assert microsecond host latency: general
Linux scheduling is not a real-time contract.

Alarm unit tests run in an isolated test process because any global drainer can
legitimately dispatch due consumer work. Tests capture a starting uptime and
use relative deadlines rather than resetting or assuming zero. Every test
retains and cancels the IDs it creates, quiesces any worker-visible state, and
restores timer auto-sync. Tests do not expose a global reset operation that
could invalidate another consumer's alarms.

The separate `esp_timer` adapter slice tests upstream behavior at the public
API boundary rather than duplicating it in the generic alarm suite. In
particular, it verifies:

- end-to-end one-shot/periodic timing, FIFO equal deadlines, callback arguments,
  and `xPortInIsrContext() == false` in the dedicated timer task;
- a periodic timer is reinserted before its callback: one callback-side case
  restarts it successfully, a separate case stops it successfully (after which
  restart fails because stop cleared its armed/period state), and direct delete
  while still armed fails;
- a one-shot is disarmed before its callback, so stop/restart fails, while
  separate callback cases can start it again or enqueue deferred deletion;
- TASK-list delete-event reclamation in the initial configuration; moving an
  ISR-list handle to TASK deletion is added with the later ISR-dispatch phase;
- an overdue ordinary periodic timer repeatedly catches up while preserving
  its absolute phase;
- `skip_unhandled_events` collapses backlog only when
  `(now - alarm) / period > 1`; exactly one fully missed period still produces
  the two due callbacks;
- periodic start and periodic restart clamp to the backend's 50 microsecond
  minimum, while the common one-shot path does not apply that clamp;
- start/stop/restart from a real emulated ISR even for TASK-dispatched timers,
  rapid coalesced rearm/disarm commands, post-claim generation revalidation,
  and suppression of a stale already-claimed compare; in manual mode an ISR
  start/restart followed immediately on ISR return by one task-side advance
  across the deadline delivers the callback without a second pump;
- empty and changing nearest-alarm queries, wake-up exclusion for timers marked
  to skip unhandled sleep events, `get_period`, `get_expiry_time`, `is_active`,
  and documented invalid argument/state results;
- once-only early/full startup in every runner, initialization rollback, and a
  second direct public init returning `ESP_ERR_INVALID_STATE`; with `esp_timer`
  as the process's first and only alarm consumer, an auto-synchronized TASK
  callback still fires while application code is CPU-busy; and
- deinit with active or queued-delete work returning `ESP_ERR_INVALID_STATE`
  while leaving the service live, followed by clean deinit with compare-source
  and handler quiescence and no stale interrupt.

## Caveats

Core alarm handlers do not themselves emulate interrupt context and may observe
a later uptime when wall synchronization or explicit advancement passes their
deadline before they acquire drain ownership. Consumer adapters must not assume
that handler execution time equals the registered deadline. They request ISR
delivery only after publishing authoritative peripheral state, completing any
externally observable effect that precedes the hardware interrupt, and
releasing locks. If external publication is already being drained, an ordered
post-publication continuation performs the source assertion later without
nested delivery.

The generic deadline API has one-microsecond timestamp granularity, and
`clock_getres()` reports one-nanosecond `CLOCK_MONOTONIC` resolution on the
authoring host, but neither number is the delivery resolution. The practical
limit is kernel wakeup plus pthread scheduling, callback work ahead in the one
waiter, and any interval for which the selected FreeRTOS pthread masks
simulated-interrupt delivery. An unloaded characterization on that host used
50, 100, and 1000 microsecond absolute timerfd waits. Despite the process's
default 50 microsecond timer slack, overshoot was approximately 4-11
microseconds median and 14-46 microseconds 99th percentile; lowering slack did
not materially improve this timerfd path. Individual samples were roughly
0.1-0.2 milliseconds late, and other host activity can still produce
millisecond-scale outliers. End-to-end interrupt latency also includes the
alarm adapter and signal dispatch. Host load or a slow earlier handler can
increase all of these values. The figures motivate the native waiter over the
1 ms scheduler tick but are not an API guarantee.
Deterministic mode preserves exact deadline ordering; wall mode preserves
no-early-fire and catches up late work.

The timerfd/eventfd transport is Linux-specific, matching the current FreeRTOS
host port. The queue, clock, and manual-pump semantics remain separable from
that transport, but a future non-Linux host port needs its own absolute waiter
and signal-safe control-wake backend.

Consequently, wall mode cannot promise sustained 50 microsecond `esp_timer`
periods even though the ESP-IDF backend reports that minimum. The future
adapter preserves upstream phase, catch-up, and `skip_unhandled_events` policy
so lateness is handled like an overdue hardware compare rather than by
inventing a host timing guarantee.

An alarm handler that blocks stalls whichever explicit task owns the pump or,
on autonomous delivery, the only native waiter and every wall-driven alarm
behind it. Internal handlers must commit/enqueue state and raise a source; they
must not wait on a FreeRTOS primitive, public callback, or the interrupt they
requested.

An already-due alarm may be claimed by the waiter after scheduling unlocks but
before `ScheduleSystemTimeAlarm()` returns. Callers establish callback state,
generation, and lifetime before scheduling and must not require the returned ID
inside that first invocation.

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

#### Use the scheduler tick as the auto-sync deadline source

The tick is preemptive, but it is only 1 kHz in the current host configuration.
Its signal handler cannot lock or run the alarm queue, so it would still need a
FreeRTOS task and would add at least tick granularity plus task scheduling and
priority starvation. The native waiter instead uses the same monotonic timebase
as uptime and leaves the alarm core independent of FreeRTOS.

#### Use only a pthread condition variable for the native waiter

A monotonic condition wait can retarget ordinary task-created alarms, but it
does not provide an async-signal-safe wake for a hardware compare reprogrammed
from an emulated ISR. On the measured host it also inherited roughly 50
microseconds of default timer slack. The Linux timerfd/eventfd pair provides an
absolute high-resolution deadline and a coalescing signal-safe control wake to
the same single waiter.

#### Use `roo_scheduler`

`roo_scheduler` models application-owned cooperative jobs with priorities and
explicit execution. System alarms are lower-level global clock events with
different ownership, cancellation, and callback-context requirements.

#### Dispatch alarms from every clock read

Clock reads occur in logging, sampling, and validation paths. Making them
execute arbitrary callbacks would create surprising re-entry and turn passive
observation into a mutation point.

#### Protect passive clock reads with the alarm mutex or a spinning seqlock

An emulated ISR can preempt the code that owns that mutex or is updating the
seqlock. Waiting or spinning would deadlock the interrupted task. The bounded
atomic snapshot can fall back to already published monotonic uptime instead.

#### Use an unguarded host mutex for slow state

The FreeRTOS Linux port can suspend one task pthread and select another while
the first owns a libc/pthread lock. If the selected task then waits for that
lock, the owner cannot be rescheduled. The signal-mask guard prevents task
handoff only for the short interval in which a task owns shared host state;
callbacks and waits remain outside it.

#### Route ISR busy delays through fake-time advancement

The dispatching delay path locks, mutates global time, and can invoke arbitrary
alarm handlers, none of which is valid from a POSIX signal handler. The
separate monotonic busy loop occupies the emulated core without turning an ISR
into a global fake-time driver.

#### Reimplement the common `esp_timer` service in a C++ shim

The vendored implementation already defines two dispatch lists, equal-deadline
behavior, callback-visible rearming, periodic catch-up and skip thresholds,
deferred task-context deletion, and query semantics. Reimplementing those above
the neutral queue would create a second subtly different timer service. A host
`esp_timer_impl_*` backend reuses those policies and emulates only the hardware
counter, compare, and interrupt boundary.

#### Add periodic alarms to the core

Periodic APIs differ on drift, missed periods, callback overruns, stop/restart,
and sleep handling. Keeping the core one-shot leaves those policies in the
peripheral adapter or, for `esp_timer`, in the reused upstream common layer.

## Future Work

- Complete [LEDC fade Phase 3](ledc_voltage_emulation.md#phase-3-ledc-fade-state-machine)
  over the alarm and interrupt services.
- Add a host `esp_timer_impl_*` backend and Bazel-build the vendored common
  `esp_timer` layer for current `ESP_TIMER_TASK` support. Include the
  internal fixed compare-source extension, signal-safe rearm publication,
  framework startup/shutdown integration, and preserved list, callback,
  lifecycle, periodic, and query tests in this first slice.
- If `ESP_TIMER_ISR` is enabled in the host configuration, extend that backend
  to the second dispatch list and test direct ISR callbacks and yield behavior.
- Complete Arduino hardware-timer alarm delivery and add a direct GPTimer shim
  when required.
- Use alarms for additional asynchronous peripheral completions when their
  observable timing matters to a test.
