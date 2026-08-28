# Emulated-time alarms

Status: Proposed

## Objective

Provide cancellable one-shot callbacks at absolute roo_testing uptimes, with
deterministic explicit delivery in manually driven time and autonomous
best-effort delivery while uptime follows the host monotonic clock. Peripheral
completion waits remain task-local and never advance global uptime themselves.

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

A peripheral API that waits for such completion still blocks only its calling
FreeRTOS task. It waits on a framework semaphore or notification; it does not
advance global uptime to manufacture completion. Other runnable tasks continue,
and time progresses either from wall-clock auto-sync or from a separate test
driver in manual mode.

## Background

[`timer.cpp`](../roo_testing/system/timer.cpp) owns roo_testing's intended
monotonic uptime. Its `EmulatedTime` currently defaults to following host
elapsed time and exposes a process-global setter that can switch to explicit
test advancement and back. Arduino
`micros()`/`millis()` and ESP-IDF `esp_timer_get_time()` use this same clock.
The current timer has no callback queue, uses `high_resolution_clock` even
though C++ does not guarantee that clock is steady, has unsynchronized mutable
state, and does not check all unsigned lag/delay conversions for overflow.
Clock reads are also reachable from framework ISRs. In particular, Arduino
marks `micros()` as ISR-callable. Once interrupts can preempt ordinary host
code, a clock read must not take a mutex that the interrupted code might hold.

A repository-wide call-site audit found no sketch, emulator runner, or other
production caller of `system_time_set_auto_sync()`. Runtime changes occur only
in tests: roo_testing's [simple timer test](../test/simple_test.cpp), the mixed
manual/wall-clock [roo_scheduler suite](../../roo_scheduler/test/roo_scheduler_test.cpp),
the [roo_prefs timing tests](../../roo_prefs/test/lazy_write_pref_test.cpp), and
the [roo_windows touch test](../../roo_windows/test/touch_sensor_test.cpp).
Those uses select a test policy or restore leaked process-global state; they do
not exercise an application requirement to change policy while running. This
proposal therefore makes the policy immutable per process and isolates tests
that need different policies in different binaries.

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

The current implementation boundaries are:

| Area | Current behavior |
| --- | --- |
| [`system/timer.cpp`](../roo_testing/system/timer.cpp) | Unsynchronized clock state; no alarms or native waiter |
| [FreeRTOS Linux port](../roo_testing/frameworks/esp-idf/components/freertos/FreeRTOS-Kernel/portable/linux/port.c) | Implemented scheduler tick and simulated-interrupt signals; no alarm queue |
| [`roo_testing::mutex`](../roo_testing/sys/mutex.h) and vendored framework code | FreeRTOS task synchronization; not usable by a non-FreeRTOS native waiter |
| Host shims, fake GPIO, and built-in sinks | Some roo_testing-owned state currently uses ordinary C++ host mutexes |

This proposal changes roo_testing host infrastructure and selected host-owned
state. It does not replace ESP-IDF or Arduino framework semaphores, critical
sections, or `FromISR` APIs.

## Requirements

1. A caller can schedule and cancel a one-shot callback at any absolute
   microsecond deadline in the inclusive range
   `[-9,223,372,036,854,775, +9,223,372,036,854,775]`; values outside that
   range fail before changing service state.
2. Scheduling, cancellation, clock mutation, and dispatch are safe from
   multiple native host threads and FreeRTOS tasks. These management operations
   are explicitly not POSIX-signal- or ISR-safe.
3. A passive uptime read is monotonic, bounded, nonblocking, and safe from an
   emulated ISR; it never allocates or sleeps.
4. An alarm never runs inline from scheduling, including when its deadline is
   already due.
5. Due alarms run in deadline order; alarms at one deadline run in registration
   order.
6. An explicit fake-time delay crosses deadlines chronologically before
   returning when that caller owns delivery. Concurrent delivery never nests or
   moves uptime backward; already-owned due work completes through its owner.
7. Each process uses one time-progression policy for its entire lifetime. A
   normal emulator follows host monotonic time by default; a deterministic test
   can select manual time before process execution begins.
8. In a manual-time process, alarms run only at documented explicit pump points
   and the service never advances uptime on its own. In an auto-synchronized
   process, a due alarm becomes eligible without application polling, even when
   the selected FreeRTOS task is CPU-busy. Wall-synchronized delivery is
   best-effort and never promises a hard latency bound.
9. Alarm handlers are short and non-throwing by contract and never wait for a
   task, interrupt, or external completion. Handler invocation and capture
   destruction do not block alarm management, and handler re-entry does not
   create a nested callback stack. In an exception-enabled explicit pump, a
   contract-violating exception leaves later alarms queued and propagates to
   that pump's caller; the same violation on the autonomous waiter is fatal
   because there is no caller to receive it.
10. Cancellation prevents an unclaimed callback. Cancelling executing,
   completed, or unknown work is a no-op.
11. Work scheduled or made due by a callback is reconsidered before the active
    delivery cycle becomes idle.
12. The core primitive imposes no ESP-IDF ISR, task affinity, priority, or
    periodic-timer semantics on its consumers.
13. A consumer can tear down its own pending alarms without a global reset that
    invalidates unrelated registrations.
14. Host-time observation and explicit clock mutation preserve monotonic uptime;
    numeric conversions and additions fail before overflow.
15. A peripheral completion wait suspends only its calling FreeRTOS task. It
    does not advance uptime; other runnable tasks and the configured time driver
    continue independently.

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
- Changing between manual and auto-synchronized time after process startup.

## Design Overview

Everything from this section through Caveats describes proposed behavior unless
it is explicitly marked implemented. The design introduces six internal
concepts:

| Term | Meaning and lifetime |
| --- | --- |
| Published uptime | The process-wide monotonic time floor. It is atomic and remains valid for the process lifetime. |
| Time mode | The process-lifetime choice between manual and auto-synchronized time. The final binary selects it; auto-sync is the default. |
| Auto-sync origin | The immutable offset that maps `CLOCK_MONOTONIC` onto published uptime in an auto-synchronized process. It is published once during clock initialization. |
| Dynamic alarm | A value-owned one-shot record with an absolute deadline, callback, and nonreused ID. It exists until cancellation or claim. |
| Drainer | The one caller currently allowed to claim and invoke due alarms. Ownership lasts only across one non-nesting delivery loop. |
| Native deadline waiter | One process-wide pthread that observes wall-clock deadlines in auto-sync mode and asks the same drainer path to deliver them. |

`AtomicUptimePublication` encapsulates both the atomic uptime floor and
one-time publication of the configured mode and auto-sync origin. It has
constant-initialized static storage independent of the dynamically initialized
alarm containers and host mutex; its `nowNanos()` method is the entire
ISR-visible clock path. `SystemTimeService` references that clock core and owns
slow clock mutation, alarm records, ID allocation, and drainer ownership behind
a scheduler-safe host lock.

The two delivery flows differ only in who notices that time reached a deadline:

```text
manual-time process
external test driver -> advance/pump -> drainer -> internal alarm handler

auto-synchronized process
timerfd -> native waiter -> drainer -> internal alarm handler

hardware-like consumer (both modes)
internal alarm handler -> publish peripheral state -> interrupt controller
                       -> framework ISR -> unblock only the waiting task
```

Scheduling never invokes a callback inline. In a manual-time process, explicit
delay and `ProcessSystemTimeAlarms()` are the only pumps. In an
auto-synchronized process, the native waiter sleeps until the earliest deadline
mapped through the immutable host origin. A manual-time process never creates
that waiter.

For example, a deterministic test selects manual time in its final Bazel target,
before any static initialization or clock access:

```python
cc_test(
    name = "manual_alarm_test",
    # ...
    deps = [
        "//roo_testing/system:manual_time_mode",
        # ...
    ],
)
```

Its test code can then prove that a due callback is not delivered early or
inline without changing global mode:

```cpp
bool fired = false;
const int64_t start = system_time_get_micros();
ScheduleSystemTimeAlarm(start + 1000, [&] { fired = true; });

system_time_delay_micros(999);
CHECK(!fired);
system_time_delay_micros(1);  // This call owns the pump and delivers the alarm.
CHECK(fired);
```

A peripheral adapter uses the callback only to commit hardware-visible state
and request the appropriate interrupt. The public framework callback still runs
in its promised context:

```cpp
// Runs on the explicit pump owner or native waiter, never in ISR context.
void CompleteFade(ChannelState& channel, uint64_t generation) {
  if (!channel.commitCompletion(generation)) return;
  channel.publishFinalOutput();
  raiseInterruptSource(kLedcInterruptSource);
}

// Runs later in the emulated interrupt frame on the selected FreeRTOS pthread.
void LedcIsr(void* argument) {
  ChannelState& channel = *static_cast<ChannelState*>(argument);
  BaseType_t task_woken = pdFALSE;
  xSemaphoreGiveFromISR(channel.done(), &task_woken);
  portYIELD_FROM_ISR(task_woken);
}

// WAIT_DONE uses xSemaphoreTake(...); it never advances roo_testing uptime.
```

The generic alarm handler therefore has neutral execution context. Planned
consumers adapt it as follows:

| Code | Execution context | ISR context? |
| --- | --- | --- |
| Internal alarm handler | Explicit pump owner or native waiter pthread | No |
| Peripheral ISR | Selected FreeRTOS pthread's signal frame | Yes |
| `ESP_TIMER_TASK` callback | Dedicated ESP-IDF timer task | No |
| Future `ESP_TIMER_ISR` callback | Timer interrupt frame | Yes |

The implementation ownership is equally explicit:

| Component | Change in this proposal |
| --- | --- |
| `roo_testing/system` | Add immutable link-time mode selection, `AtomicUptimePublication`, alarm storage, drainer, waiter, and the public internal alarm API |
| New `//roo_testing/host:synchronization` utility | Add the scheduler-safe host lock used where native pthreads share state with FreeRTOS task pthreads |
| FreeRTOS Linux port and generic interrupt controller | No alarm-specific change; reuse their implemented signal deferral and interrupt ingress |
| Vendored ESP-IDF/Arduino framework | No locking change; framework code continues to use FreeRTOS primitives |
| Peripheral shims | Later adapter phases schedule alarms, publish peripheral state, and raise logical interrupt sources |

| Requirements | Design element |
| --- | --- |
| 1, 4-5, 10 | Ordered alarm records plus an ID index |
| 2 | Scheduler-safe host locking for task/native management state |
| 3, 7, 14 | Immutable process mode, one-time auto-sync origin, and atomic monotonic uptime |
| 6, 8 | Explicit manual pumps plus one auto-sync deadline waiter |
| 9, 11 | Scope-guarded, invoke-outside-lock non-nesting drain |
| 12 | Neutral one-shot callback contract with consumer adapters |
| 13 | Per-alarm teardown without a global alarm reset |
| 15 | Consumer-owned FreeRTOS wait primitive; alarms never advance time for it |

## Design Details

### Encapsulated one-time clock publication

`AtomicUptimePublication` is a process-lifetime internal helper shared with
`SystemTimeService`. The timer implementation target compiles as C++20 and
defines one namespace-scope `constinit AtomicUptimePublication`; function-local
static construction is forbidden. This keeps first clock use out of the alarm
service's container, host-lock, and dynamic-initialization paths. It hides both
first-use mode publication and the lock-free uptime floor from alarm and shim
code:

```cpp
enum class SystemTimeMode : uint8_t {
  kManual,
  kAutoSync,
};

class AtomicUptimePublication {
 public:
  /// Returns monotonic uptime without locking, waiting, or allocating.
  int64_t nowNanos() noexcept;

  /// Returns the already-published floor without observing host time.
  int64_t publishedFloorNanos() const noexcept;

  /// Initializes if necessary, waits outside ISR context, and returns the mode.
  SystemTimeMode modeForSlowPath() noexcept;

  /// Atomically publishes uptime_ns when it is later than the current floor.
  void publishAtLeast(int64_t uptime_ns) noexcept;

  /// Adds delta_ns to the latest floor with checked compare/exchange retry.
  int64_t advanceByChecked(uint64_t delta_ns) noexcept;

 private:
  enum class ModeState : uint8_t {
    kUninitialized,
    kInitializing,
    kManualReady,
    kAutoReady,
  };

  ModeState initializeForReadOrObserve() noexcept;

  // Written once before kAutoReady is release-published, then immutable.
  int64_t host_offset_ns_ = 0;
  std::atomic<int64_t> published_uptime_ns_{0};
  std::atomic<ModeState> mode_state_{ModeState::kUninitialized};
};
```

The final binary chooses the mode through one internal C-linkage data symbol:

```cpp
extern "C" const volatile sig_atomic_t
    roo_testing_system_time_initial_auto_sync;
```

`default_time_mode.cpp` supplies a weak definition whose value is 1. The public
`//roo_testing/system:manual_time_mode` library supplies a strong definition
whose value is 0; its Bazel rule uses `alwayslink = True`, ensuring that the
override is extracted into the final executable. Both definitions have external
visibility, and the volatile-qualified read prevents whole-program optimization
from folding the weak default. `sig_atomic_t` makes that read valid in a POSIX
signal handler. The override source lives in a subdirectory so the existing
top-level `*.cpp` glob does not also add it to the timer library. Runners remain
mode-neutral, and no environment variable or startup callback participates in
selection. Link resolution therefore happens before static constructors and
before the first possible clock read.

The `constinit` definition mechanically guarantees initialization before all
C++ dynamic initialization. Both atomic members are also statically asserted
always lock-free. A first-use contender—including an emulated ISR—uses
`sigprocmask()` to block all maskable signals, samples `CLOCK_MONOTONIC` with a
checked conversion to signed nanoseconds, then attempts to change
`kUninitialized` to `kInitializing`. These operations, the volatile mode-symbol
load, and the lock-free atomic operations form the complete async-signal-safe
initialization path. A conversion failure terminates before claiming
initialization.

The winner loads the link-selected value exactly once. For auto-sync it writes
`host_offset_ns_ = published_uptime_ns - host_now_ns`; because both operands
are representable nonnegative values, the subtraction is representable. For
manual time there is no host origin to publish. It then release-stores the
corresponding ready state and restores the signal mask. A loser restores its
mask and handles the state it observed.

An ISR or racing passive reader that observes `kInitializing` immediately
returns the atomic uptime floor; it never reads `host_offset_ns_` or waits. An
auto-mode reader first acquire-loads `kAutoReady`, then reads the immutable
offset and samples `CLOCK_MONOTONIC`. This release/acquire pair is what makes
the ordinary offset field safe. Merely writing the offset before an atomic
`enabled` flag in source order would not establish cross-thread visibility and
would leave a C++ data race if the offset were later rewritten.

`modeForSlowPath()` uses the same signal-mask-before-claim order. A task/native
caller that encounters `kInitializing` yields outside the service lock until
it acquire-loads a ready state. A FreeRTOS initializer cannot be suspended
after winning because its scheduler and interrupt signals remain masked until
publication. A winning native pthread progresses independently. Slow-path
waiting is therefore safe; the ISR-visible path remains bounded.

In auto mode, `nowNanos()` forms a host-derived candidate from the immutable
offset and publishes it with at most one strong compare/exchange. Losing the
race returns the newer floor; a later read can catch up further. Explicit lag
and delay operations only raise that floor. They never rebase the host origin,
so host time eventually catches up to an explicitly advanced clock.
`system_time_sync()` is a no-op in manual mode; in auto mode it observes the
fixed mapping and performs any ahead-of-host pacing outside service locks.

`system_time_get_micros()` converts `nowNanos()` to microseconds. It never
locks, allocates, dispatches, starts the waiter, or sleeps. The mode query and
all clock mutations are task/native-only.

### Guarded slow state

A *slow-path operation* is a task/native operation allowed to lock, allocate,
destroy callbacks, wake the waiter, or sleep. It is not an execution thread or
an ESP-IDF timer-dispatch mode. Alarm management and clock mutation are slow
paths; passive clock reads and signal-side interrupt delivery are not.

The native deadline waiter is a pthread, not a FreeRTOS task. It cannot take a
FreeRTOS semaphore, while the selected FreeRTOS task pthread must share the
same alarm state with it. The design therefore adds
`roo_testing/host/scheduler_safe_host_lock.h/.cpp`, exported only to roo_testing
packages as `//roo_testing/host:synchronization`. Its
`SchedulerSafeHostLock` is backed by a host mutex. This is not a framework API
and does not replace any ESP-IDF or Arduino lock.

Ordinary service code therefore reads like a conventional locked update; the
one-time publication and signal-mask protocols stay inside the clock helper:

```cpp
void SystemTimeService::lagNanos(uint64_t delta_ns) {
  const SystemTimeMode mode = clock_.modeForSlowPath();
  SchedulerSafeHostLock lock(state_mutex_);
  clock_.advanceByChecked(delta_ns);
  if (mode == SystemTimeMode::kAutoSync) wakeWaiterLocked();
}
```

`SchedulerSafeHostLock` saves the caller's POSIX signal mask, blocks all
maskable signals, and only then locks the host mutex. It unlocks before
restoring the saved mask. The order matters: the Linux port cannot suspend task
A while A owns shared host state and then select task B, which would wait for A
while A is no longer runnable. A task can still wait for the native worker,
because that independently scheduled pthread continues running. The waiter
inherits an all-blocked signal mask for its lifetime.

The utility lands in a small roo_testing-only host-synchronization target. The
timer service uses it for clock and alarm slow state. The existing host mutexes
inside `FakeGpioPin`, `SimpleVoltageSink`, and `SimpleDigitalSink` migrate to the
same guard because later alarm-driven LEDC publication can contend with them.
The FreeRTOS Linux port, `roo_testing::mutex`, and vendored framework locking do
not change in this phase.

No callback invocation, callback-capture destruction, allocation retry, host
sleep, descriptor poll, or thread join occurs while this lock is held. A
nonblocking eventfd wake that records a locked state transition is the only
external operation permitted before unlock.

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
erases that record and destroys its callback outside the service lock before
propagating `std::bad_alloc` in exception-enabled host builds. A no-exception
build follows its C++ allocation runtime's fatal policy. Destruction occurs
after the slow-path guard has restored the caller's signal mask; scheduling
never leaves an uncancellable partial registration.

The callback owns its captures according to normal `std::function` rules.
Borrowed state must outlive any handler execution that may already have been
claimed; cancellation alone cannot establish that. Deletable adapters use
shared or deferred lifetime, or external synchronization that excludes a
concurrent claim.

### Scheduling and cancellation

Scheduling `CHECK`s that the callback is non-empty, that converting the
microsecond deadline to signed nanoseconds is representable, and that the ID
space and service lifecycle remain open. It allocates an ID and inserts the
record while holding the service lock, then wakes the native waiter if the
earliest deadline changed. An already-due deadline is queued
normally; scheduling never calls user code inline. In auto-sync mode the waiter
may nevertheless claim it concurrently before the scheduling call returns.

Scheduling and cancellation may run from an ordinary native host thread or a
FreeRTOS task. They must not run in POSIX-signal or emulated ISR context because
they lock and may allocate or destroy captured state.

Cancellation removes an unclaimed record from both containers. Move the
callback out while locked and destroy it after unlock so captured-object
destructors cannot re-enter the service under its lock. The slow-path guard also
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
Deadline comparisons use the checked conversion already performed by
`ScheduleSystemTimeAlarm()`. Every stored deadline is consequently representable
as signed nanoseconds; values outside the API's documented inclusive range are
rejected before insertion.

`system_time_delay_micros()` remains the dispatching explicit-advance
operation. It walks through intervening alarm deadlines. At each boundary it
sets uptime, drains every alarm then due, and continues to the final target.
Alarms registered for the same deadline by a callback join the active drain
after previously registered work. The lower-level `system_time_lag_ns()`
does not run callbacks itself. In a manual-time process, work it makes due
waits for a later delay or explicit pump. In an auto-synchronized process, it
wakes the waiter because hardware time has become due even though the mutating
caller remains a non-dispatching path.

Only application/test time drivers use the dispatching delay. A peripheral shim
that is waiting for its own completion must block its calling task on a
FreeRTOS primitive; it must not call `system_time_delay_micros()` to force its
deadline to arrive.

`ProcessSystemTimeAlarms()` first observes current uptime using the configured
process mode and drains everything due at that observed time. It does not add
time itself. If host-time observation moved an auto-synchronized process past
several deadlines, callbacks still run in deadline/registration order but
observe the current, possibly later, uptime.

In a manual-time process, a separate test driver, native host thread, or
FreeRTOS task must advance time and pump it. A peripheral task blocked on its
completion primitive only waits; it never pumps the global alarm queue on a
timeout or retry boundary.

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
hardware would otherwise generate. An auto-synchronized process starts it
lazily as part of its first successful alarm schedule; a manual-time process
never creates it. The lock-free clock-read path never starts it. Descriptor and
thread construction is transactional; failure restores the creator's signal
mask, closes partial resources, and terminates before the API promises
autonomous delivery.

A service-lock-protected wake generation changes whenever the queue, clock
floor, drain ownership, or lifecycle changes. Before unlocking, the changer
performs a nonblocking eight-byte `write()` to a Linux eventfd.
`EAGAIN` means a wake is already pending and is safely coalesced. Any other
persistent error is fatal after the caller restores its signal mask. Persistent
queue state plus the generation ensures that eventfd counter coalescing cannot
lose the reason to recompute. No signal-side eventfd publisher is part of this
proposal; ISR-safe hardware-compare rearming belongs to the separate future
`esp_timer` backend design.

The worker owns a `timerfd_create(CLOCK_MONOTONIC, ...)` descriptor and polls it
together with the wake eventfd. It maps the earliest emulated deadline through
the immutable host origin and programs the timerfd with `TFD_TIMER_ABSTIME`. A
wake-event read forces a full recomputation and retarget; a timer-event read
samples and publishes host-derived uptime, then competes for ordinary drain
ownership. It always rechecks the wake generation and earliest work after
reacquiring the service lock. Early or stale readiness simply disarms or
retargets and polls again, so a callback is never intentionally delivered
before its emulated deadline. Absolute programming avoids accumulated drift and
wall-clock adjustments.

The waiter blocks all maskable signals in its native pthread. Thread creation
temporarily controls the creator's signal mask so the waiter inherits that
mask without adding a FreeRTOS dependency. Descriptor and thread startup is
transactional: eventfd, timerfd, or thread failure restores the creator mask,
closes partial resources, and fails before promising autonomous delivery. The
worker is never detached. An internal, testable service-lifecycle seam
transitions `running -> closing -> stopped` under the service lock. New schedule,
clock mutation, or drain entry while closing/stopped fails a `CHECK`
before changing state; cancellation follows the no-op/removal exception below,
and the return-only-ID scheduling API never fabricates a recoverable failure
result.
Passive reads remain available, and cancellation retains its public no-op
contract: during closing it may remove an unclaimed record, while after
extraction/stopping it safely finds nothing. This also lets a pending callback
capture destructor cancel related IDs during shutdown without aborting.
Shutdown uses two internal calls declared in
`roo_testing/system/timer_host_lifecycle.h`; neither is part of `timer.h` or a
framework API:

```cpp
// Task/native-only and nonblocking. Returns false while a drainer is active.
bool TryBeginSystemTimeServiceShutdownForHost();

// Native-only, after the FreeRTOS scheduler has returned.
void FinishSystemTimeServiceShutdownForHost();
```

`TryBeginSystemTimeServiceShutdownForHost()` is forbidden from the active
drainer itself. Under the service lock, it returns `false` without changing
lifecycle state while any native or FreeRTOS drainer is active. When no drainer
is active, the same critical section changes `running` to `closing`, preventing
a new drainer from winning, and wakes the worker. The selected runner task
delays for one FreeRTOS tick after a `false` result, allowing a preempted or
lower-priority task drainer to finish, then retries. Consumer-owned tasks and
registrations must already be quiescent before this loop, so no new management
entry races the successful transition.

After the successful transition, the runner ends the scheduler.
`FinishSystemTimeServiceShutdownForHost()` then joins the worker when one was
created; moves all unclaimed callbacks out; clears the ordered queue and ID
index; closes any timerfd and eventfd; publishes `stopped`; and destroys
captures after unlock and signal-mask restoration. It never waits for a
FreeRTOS drainer after the scheduler has stopped: the successful begin call
proved there was none, and `closing` prevented a replacement. Dedicated
lifecycle tests cover both halves without exposing a public global alarm reset.

The host-runner integration phase wires that path into
[`esp_idf_support/main.cpp`](../roo_testing/frameworks/esp_idf_support/main.cpp),
[`freertos_gtest_main.cpp`](../roo_testing/frameworks/arduino_support/freertos_gtest_main.cpp),
and the Arduino-aware
[`gtest_main.cpp`](../roo_testing/frameworks/arduino_support/gtest_main.cpp).
The ordinary [Arduino sketch runner](../roo_testing/frameworks/arduino_support/main.cpp)
has no normal return path, so abrupt process termination remains its only stop
path and relies on process teardown rather than pretending to perform orderly
service shutdown.

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
   limit under the service lock.
2. Synchronize or advance to that limit and remove the earliest due
   record from both containers, marking it claimed.
3. Release the lock and invoke the callback.
4. Destroy the callback outside the lock, then lock and rescan.
5. When no due work remains, clear drain ownership and confirm the queue state
   in the same critical section.

A scope guard owns the drain marker for the entire loop. On normal completion,
step 5 clears ownership and disarms the guard in the same critical section, so
the guard cannot later clear a new drainer's ownership. On abnormal exit, the
guard clears ownership under the service lock. Alarm handlers must not throw. If a
handler nevertheless throws during an explicit pump in a host build with
exceptions enabled, its local `std::function` is destroyed outside the lock,
the guard releases drain ownership, and the exception propagates. Remaining
alarms stay queued for a later pump; the drainer does not catch the exception
and continue invoking unrelated handlers. The native waiter catches at its
thread boundary and emits a process-fatal diagnostic because no caller exists
to receive the contract violation.

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

Wall-clock sleeping never holds the service lock. After waking, the waiter
reacquires the lock and re-evaluates the wake generation, drain ownership, and
earliest alarm rather than relying on stale queue state.

### Consumer adapters

The core alarm callback executes on the native waiter or whichever host thread
or FreeRTOS task explicitly pumps time. It must not directly invoke a public
callback that promises a task or ISR context. Consumers add that dispatch layer
after releasing alarm and peripheral locks:

- The planned [LEDC adapter](ledc_voltage_emulation.md#alarm-service-integration)
  materializes generation-checked completion state and appends an ordered pair:
  final GPIO publication, then an internal completion finalizer. The finalizer
  publishes the ISR mailbox/status and raises the LEDC source through the
  implemented [ESP-IDF interrupt adapter](emulated_interrupts.md#esp-idf-adapter)
  only after the GPIO and sink calls have returned. Its emulated ISR releases
  the channel gate, invokes the registered callback, and requests any
  `FromISR` yield. After unlocking, the alarm handler attempts the non-waiting
  LEDC drain so an
  autonomous completion cannot remain queued when no application call follows.
- A future `esp_timer_impl_*` backend will reuse the vendored common
  [`esp_timer`](../roo_testing/frameworks/esp-idf/components/esp_timer/src/esp_timer.c)
  layer and the implemented [interrupt adapter](emulated_interrupts.md#esp-idf-adapter).
  The common layer will continue to own public handles, periodic policy, and
  TASK/ISR dispatch. Because upstream start/stop/restart can rearm the hardware
  compare from ISR context, that backend needs a fixed, signal-safe compare
  ingress rather than this proposal's allocating dynamic-alarm API. Its mailbox,
  startup, source profile, and deinitialization require a separate design before
  implementation; they are intentionally not hidden inside the generic queue.
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

Remove `system_time_set_auto_sync(bool)`. Add the task/native-only
`system_time_is_auto_sync_enabled()` query for diagnostics and tests; it reports
the link-selected process mode and cannot change it. Auto-sync remains the
zero-configuration default. A final executable selects manual time by depending
on `//roo_testing/system:manual_time_mode`, as shown in Design Overview.

Of the clock-observation/mutation API, only `system_time_get_micros()` is
ISR-safe. `system_time_is_auto_sync_enabled()`, `system_time_sync()`,
`system_time_lag_ns()`, `system_time_delay_micros()`, and every alarm management
function are task/native-only. The internal `system_time_busy_wait_micros()`
helper described above is separately ISR-safe, but deliberately does not
readjust or dispatch fake time.

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
without allocating an ID. `deadline_uptime_us` accepts the inclusive range
`[-9,223,372,036,854,775, +9,223,372,036,854,775]`, which is exactly the
microsecond range safely convertible to the internal signed-nanosecond
representation. A value outside that range, ID exhaustion, and scheduling
after service shutdown fail `CHECK` before changing state. Container allocation failure
propagates `std::bad_alloc` in exception-enabled host builds with the queue and
index rolled back; no-exception builds use their allocation runtime's fatal
policy. Cancellation of zero, unknown, completed, claimed, or post-shutdown IDs
is a no-op.

All three functions are ordinary native-thread or FreeRTOS-task operations and
are not callable from an emulated ISR. Until the autonomous waiter phase lands,
the API is safe but explicit-pump-only: auto-sync can update uptime, but only
`ProcessSystemTimeAlarms()` or a dispatching explicit delay invokes handlers.

No public ESP-IDF or Arduino timer API is added in this design.

## Implementation Plan

Authoring reference: follow this repository's
[C++ code-authoring guidance](../.github/instructions/embedded-cpp-code-authoring.instructions.md).

### Phase 1: Scheduler-safe host locking

Add `//roo_testing/host:synchronization` with `SchedulerSafeHostLock`, then
migrate the existing host mutexes in `FakeGpioPin`, `SimpleVoltageSink`, and
`SimpleDigitalSink`.
Document that it is for state shared with native pthreads and does not replace
FreeRTOS synchronization in framework code. Do not change clock semantics or add
alarms in this phase.

Proposed commit: `Guard shared host state against FreeRTOS signal preemption`

Validation: add focused lock-order and signal-mask restoration tests. A
FreeRTOS regression uses low- and high-priority tasks plus a native contender to
exercise each migrated lock and prove that the selected task cannot be switched
away while it owns host state.

### Phase 2: Link-selected process time mode

Add the internal mode-symbol declaration, a weak auto-sync definition in
`default_time_mode.cpp`, and the strong override in
`manual_time/manual_time_mode.cpp`. Export the override as the public,
`alwayslink` `//roo_testing/system:manual_time_mode` target. Make the current
clock constructor use the symbol and add the read-only mode query. Migrate
`//test:simple_test` to the manual target and remove its mode toggles. Add one
static-initialization test source compiled as separate auto and manual targets.
Set the roo_testing module version to 2.1.0 for the mode-selection release.

For rollout only, retain `system_time_set_auto_sync()` with its current behavior
and mark it deprecated in the header comment. No new code calls it. This
temporary behavior ends in Phase 6, after every known caller has migrated; no
alarm API or concurrent clock access is added while the mutable bridge exists.
During this interval, `system_time_is_auto_sync_enabled()` reports the actual
current mode, including a legacy setter change. From Phase 6 onward the same
query reports the immutable link-selected mode.

Proposed commit: `Select the emulated time mode at link time`

Validation: verify the default auto target and the `alwayslink` manual target
both observe their expected mode from a global constructor and from the test
body. Run the migrated simple test without setting or restoring process-global
mode. Before downstream migration,
validate all three sibling repositories with
`--override_module=roo_testing=<phase-2-checkout>`, then publish roo_testing
2.1.0 to the configured registry.

### Phase 3: Split roo_scheduler tests by time mode

In `roo_scheduler`, remove all mode-setter calls from
`test/roo_scheduler_test.cpp`. Compile that source into a manual target that
excludes `Scheduler.DelayWithNormalPriority` and
`Scheduler.DelayWithHeightenedPriority`, and an auto target that contains only
those two host-paced cases. Link `@roo_testing//roo_testing/system:manual_time_mode`
only into the manual target. Preserve `//:roo_scheduler_test` as a `test_suite`
over both process-isolated targets. Update `MODULE.bazel` from roo_testing 2.0.0
to 2.1.0, advance the roo_scheduler module version from 2.1.10 to 2.1.11, and
update `MODULE.bazel.lock` in the same commit.

Proposed commit: `Isolate scheduler tests by emulated time mode`

Validation: run the aggregate test suite and both targets directly. The manual
target performs no host-paced delay assertion; the auto target performs no
explicit fake-time mutation.

### Phase 4: Select manual time in roo_prefs tests

In `roo_prefs`, remove setter calls from `test/lazy_write_pref_test.cpp` and
link `//:lazy_write_pref_test` with
`@roo_testing//roo_testing/system:manual_time_mode`. Update `MODULE.bazel` from
roo_testing 2.0.0 to 2.1.0, advance the roo_prefs module version from 1.3.1 to
1.3.2, and update `MODULE.bazel.lock` in the same commit.

Proposed commit: `Select manual time for lazy preference tests`

Validation: run `//:lazy_write_pref_test` and verify that its existing explicit
time advances remain deterministic without global setup or restoration.

### Phase 5: Select manual time in roo_windows tests

In `roo_windows`, remove `ManualTimeScope` and its uses from
`test/touch_sensor_test.cpp`, then link `//:touch_sensor_test` with
`@roo_testing//roo_testing/system:manual_time_mode`. Update `MODULE.bazel` from
roo_testing 2.0.0 to 2.1.0, advance the roo_windows module version from 1.6.1
to 1.6.2, and update `MODULE.bazel.lock` in the same commit.

Proposed commit: `Select manual time for touch sensor tests`

Validation: run `//:touch_sensor_test` and verify that no test depends on order
or on restoring a process-global clock policy.

Release gate before Phase 6: publish roo_scheduler 2.1.11, roo_prefs 1.3.2, and
roo_windows 1.6.2 to the configured registry, then run each released aggregate
test suite without a local override and confirm that repository-wide search
finds no remaining `system_time_set_auto_sync()` caller. This is a release
coordination gate, not another implementation change; roo_testing 3.0.0 must
not be published until the gate passes.

### Phase 6: Immutable ISR-safe monotonic uptime

Add `AtomicUptimePublication` as the namespace-scope `constinit` clock core,
compile the timer implementation target as C++20, and refactor
`system/timer.h/.cpp` to use the four-state one-time publication protocol,
`CLOCK_MONOTONIC`, checked signed-nanosecond arithmetic, and the Phase 1 host
lock for slow state. Remove
`system_time_set_auto_sync()` and the rollout deprecation. Keep the host offset
immutable, make `system_time_sync()` a no-op in manual mode, and move
ahead-of-host pacing out of passive reads. Update clock API documentation with
the ISR-safe/task-only split. Set the roo_testing module version to 3.0.0 for
the public setter removal. Do not add alarm storage or a worker.

Proposed commit: `Publish immutable emulated time through an ISR-safe clock`

Validation: run the separate auto and manual clock targets for fixed mode,
monotonic publication, concurrent lag/delay/read operations, manual sync
no-op, auto pacing, and failure-before-mutation overflow. First-use tests cover
a racing native initializer, a first-ever ISR call, an ISR interrupting another
initializer, acquire/release visibility of the one-time offset, and bounded
compare/exchange contention. The first-ever ISR case proves that initialization
completes using only the documented signal-safe operations. Compile-time
assertions cover constant initialization and the lock-free mode-state and uptime
atomics. A FreeRTOS
integration test repeatedly reads `micros()` and `esp_timer_get_time()` from
emulated ISR context while task and native contexts mutate the uptime floor.

### Phase 7: ISR-visible busy delays

Add the internal `system_time_busy_wait_micros()` path and route ISR calls from
Arduino `delayMicroseconds()` and ESP ROM delay shims to it. Task-context calls
retain dispatching fake-time delay behavior. Update shim documentation in the
same commit.

Proposed commit: `Keep ISR busy delays outside fake-time advancement`

Validation: invoke every routed delay shim from task and emulated ISR contexts.
Verify that ISR calls consume host monotonic time without changing fake uptime,
dispatching alarms, allocating, or losing deferred scheduler/interrupt signals.

### Phase 8: Deterministic manual-time alarms

Add the C++ alarm API, ordered records and ID index, cancellation, explicit
single-drainer dispatch, chronological delay integration, rescan generation,
and exception-safe callback lifetime. Add a private allocation-failure
failpoint so rollback across the ordered queue and ID index is testable. Add
focused pure-host tests, API documentation, and BUILD dependencies. Clock reads
remain outside the service lock, and the alarm test target selects manual time
through its BUILD dependency.

Proposed commit: `Add deterministic system-time alarms`

Validation: cover future, due, past, and equal deadlines; cancellation and
claimed work; callback-created alarms; recursive and concurrent pumps; explicit
delays across multiple deadlines; container-allocation failure; checked extreme
deadlines; throwing explicit handlers; and destruction outside the service
lock. Every case retains and cancels its remaining IDs; none changes or
restores time mode.

The safe interim state after this phase has no autonomous delivery. An
auto-synchronized binary still requires `ProcessSystemTimeAlarms()` or a
dispatching explicit delay to invoke handlers. No production alarm-backed
peripheral consumer lands until Phase 10.

### Phase 9: Autonomous auto-sync waiter

Add the one process-wide native waiter, absolute monotonic timerfd, wake
eventfd/generation, signal-mask setup, transactional first-alarm startup, drain
handoff, and two-part lifecycle seam. Declare
`TryBeginSystemTimeServiceShutdownForHost()` and
`FinishSystemTimeServiceShutdownForHost()` in the internal
`timer_host_lifecycle.h`, expose them through a restricted
`//roo_testing/system:timer_host_lifecycle` target, and keep the timer service
independent of FreeRTOS and the interrupt controller. A manual-time process
does not create descriptors or a worker. Direct lifecycle tests drive begin
until it establishes `closing`, then call finish to join, extract captures,
close descriptors, and publish `stopped` in the documented order.

Proposed commit: `Wake system-time alarms from host monotonic time`

Validation: `//test:system_time_auto_alarm_test` covers host-only delivery,
concurrent first schedules creating exactly one worker, earlier insertion and
cancellation retargeting a far wait, coalesced eventfd wakes, and
non-overlapping explicit/native drainers.
`//test:system_time_manual_alarm_test` proves that host time alone never fires a
manual-mode alarm. `//test:system_time_auto_alarm_lifecycle_test` covers startup
rollback, shutdown, and begin while a native or FreeRTOS drainer is active; it
verifies that begin returns `false` without changing state, succeeds after the
drainer finishes, and rejects a call from the active handler itself.
`//test:system_time_manual_alarm_lifecycle_test` covers the same lifecycle's
no-worker path.

The default-linked, auto-mode `//test:system_time_alarm_freertos_test` asserts
an interrupt from an alarm and verifies preemption of CPU-busy task code without
an explicit pump. It characterizes unloaded wake lateness but asserts only no
early fire and eventual delivery. Its two-task case has task A wait for
alarm-backed completion, task B continue running, and the waiter raise an
interrupt that releases only task A. Until Phase 10 updates the shared runners,
every target that starts the worker uses a dedicated test main that executes the
two-part lifecycle seam before returning.

### Phase 10: Host-runner shutdown integration

Replace direct `std::exit()` in the ESP-IDF and two FreeRTOS GTest runner tasks
with result capture and the two-part lifecycle sequence. After application/test
tasks and consumer registrations are quiescent, the selected runner task loops
on `TryBeginSystemTimeServiceShutdownForHost()`, using `vTaskDelay(1)` after a
`false` result so another FreeRTOS drainer can finish. It then calls
`vTaskEndScheduler()`. Once `vTaskStartScheduler()` returns, native `main()`
calls `FinishSystemTimeServiceShutdownForHost()` and returns the captured
result. Update the runner contracts and BUILD dependencies in the same commit.

Proposed commit: `Shut down system-time alarms from host runners`

Validation: exercise the real `esp_idf_support/main.cpp`,
`arduino_support/freertos_gtest_main.cpp`, and
`arduino_support/gtest_main.cpp` return paths through
`//test/profile:esp_idf_main_test`, `//test:system_time_alarm_freertos_test`, and
`//test:arduino_gtest_environment_test`. In the focused FreeRTOS target, let a
lower-priority drainer be preempted between callbacks by the runner task; prove
that begin first returns `false`, the one-tick delay lets the drainer finish,
the scheduler then stops, and native finish leaves no joinable worker or live
descriptor. Run `//test:system_time_manual_alarm_lifecycle_test` to cover the
same lifecycle's no-worker path.

After Phase 1, [steady LEDC publication](ledc_voltage_emulation.md#implementation-plan)
can proceed in parallel with the clock and alarm work. The LEDC [internal fade
engine](ledc_voltage_emulation.md#phase-3-internal-fade-and-ordered-completion-engine)
requires alarm Phases 6-10. Its public [ESP-IDF fade
integration](ledc_voltage_emulation.md#phase-5-esp-idf-fade-and-blocking-api)
also requires the implemented [interrupt
controller](emulated_interrupts.md#generic-controller). Public `esp_timer`
support requires a separate backend design because its ISR-safe
hardware-compare rearm path is outside the dynamic API proposed here.

## Testing Plan

The implementation adds focused targets with one immutable mode per process:

- `//test:system_time_auto_mode_static_test` and
  `//test:system_time_manual_mode_static_test` prove selection before `main()`.
- `//test:system_time_auto_clock_test` and
  `//test:system_time_manual_clock_test` share clock cases but link different
  mode selectors; together they cover the scheduler-safe host lock, one-time
  publication, monotonic reads, checked mutation, and ISR busy-delay routing.
- `//test:system_time_manual_alarm_test` covers queue ordering, cancellation and
  lifetime, non-inline scheduling, explicit chronological advancement, and
  non-nesting delivery.
- `//test:system_time_auto_alarm_test` covers autonomous delivery and waiter
  retargeting.
- `//test:system_time_auto_alarm_lifecycle_test` and
  `//test:system_time_manual_alarm_lifecycle_test` cover transactional startup,
  the no-worker path, the two-part shutdown seam, active-drainer handoff,
  capture destruction, and descriptor cleanup.
- `//test:system_time_alarm_freertos_test` covers ISR clock reads, CPU-busy
  preemption, pre-scheduler drainer quiescence, and the task-local blocking
  scenario from Requirement 15. It uses the default auto-sync mode and has no
  transitive dependency on `//roo_testing/system:manual_time_mode`.

The existing `//test/profile:esp_idf_main_test` and
`//test:arduino_gtest_environment_test` targets cover the two framework-aware
runner return paths; the FreeRTOS alarm target uses the plain FreeRTOS GTest
runner.

Timing assertions require no early fire and eventual delivery; they do not
assert microsecond host latency. The detailed cases and failure injection stay
with their implementation phases rather than being repeated here.

Alarm unit-test binaries run in isolated processes because any global drainer
can legitimately dispatch due consumer work. Tests capture a starting uptime
and use relative deadlines rather than resetting or assuming zero. Ordinary
cases retain and cancel the IDs they create, quiesce worker-visible state, and
never change or restore time mode. During Phase 9, a dedicated test main invokes
the two-part lifecycle seam in every target that starts the worker; Phase 10
moves that responsibility into the shared runners. A lifecycle target
deliberately stops its process's service and therefore contains no later alarm
case. Tests do not expose a global reset operation that could invalidate another
consumer's alarms.

The separate future `esp_timer` backend design owns its public-API and upstream
conformance targets; they are not part of the generic alarm suite.

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

The generic deadline API has one-microsecond timestamp granularity, but that is
not its delivery resolution. Wall-mode latency includes the kernel timer wake,
pthread scheduling, earlier callbacks on the single waiter, and any interval
for which the selected FreeRTOS pthread masks simulated interrupts. The native
timerfd avoids the scheduler tick's unavoidable one-millisecond quantization,
but it still has no hard upper latency bound. Phase 9 records unloaded latency
for diagnostics while asserting only no-early-fire and eventual delivery.
Deterministic mode preserves exact deadline ordering; wall mode catches up late
work in that order.

The timerfd/eventfd transport and weak/strong mode symbol rely on the current
Linux/ELF host toolchain. The queue, clock, and manual-pump semantics remain
separable from those mechanisms, but a future non-Linux host port needs its own
final-binary mode selector, absolute waiter, and signal-safe control-wake
backend.

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

#### Keep runtime time-mode switching

The audited callers use mode changes only to configure or clean up tests; no
emulated application changes mode while running. Mutable mode would require
coherent repeated publication of the enabled flag and host offset, waiter
startup and dormancy transitions, and test cleanup that can leak across cases.
The selected process-immutable mode removes those transitions and gives every
binary one stable clock contract.

#### Select mode from runner startup or an environment variable

A runner hook, `main()`, GTest environment, Arduino `setup()`, or ESP-IDF
`app_main()` can run after a global constructor has already read the clock. An
environment lookup on first clock access also introduces allocation and library
behavior that is unsuitable if first use occurs in an emulated ISR. The
link-selected override is resolved before process execution and works with all
existing runners without changing their startup order.

#### Rely only on offset-before-enabled source order

Writing a new host offset and then an atomic enabled flag is insufficient by
itself: source order does not publish a non-atomic field to another thread, and
later offset rewrites would race readers. The chosen design writes the offset
once and release-publishes `kAutoReady`; readers acquire that state before ever
accessing the immutable offset.

#### Use FreeRTOS software timers

The host FreeRTOS tick follows wall time and requires the scheduler. It cannot
deterministically fire when a manual-time test advances roo_testing uptime, and
it would introduce a second timebase for emulated peripherals.

#### Use the scheduler tick as the auto-sync deadline source

The tick is preemptive, but it is only 1 kHz in the current host configuration.
Its signal handler cannot lock or run the alarm queue, so it would still need a
FreeRTOS task and would add at least tick granularity plus task scheduling and
priority starvation. The native waiter instead uses the same monotonic timebase
as uptime and leaves the alarm core independent of FreeRTOS.

#### Use only a pthread condition variable for the native waiter

A monotonic condition wait could satisfy the dynamic queue alone. The selected
Linux timerfd/eventfd pair gives the worker one explicit absolute-deadline source
and one coalescing control source for retarget and shutdown, and it can later
accept the already-identified ISR-safe fixed-compare wake without replacing the
waiter transport. The tradeoff is the Linux dependency recorded in Caveats.

#### Use `roo_scheduler`

`roo_scheduler` models application-owned cooperative jobs with priorities and
explicit execution. System alarms are lower-level global clock events with
different ownership, cancellation, and callback-context requirements.

#### Dispatch alarms from every clock read

Clock reads occur in logging, sampling, and validation paths. Making them
execute arbitrary callbacks would create surprising re-entry and turn passive
observation into a mutation point.

#### Protect passive clock reads with the service lock or a spinning seqlock

An emulated ISR can preempt the code that owns that lock or is updating the
seqlock. Waiting or spinning would deadlock the interrupted task. The one-time
mode publication lets a reader that interrupts initialization return the
already-published monotonic floor immediately.

#### Use an unguarded host mutex for slow state

This alternative concerns only roo_testing host state shared with the native
waiter; ESP-IDF and Arduino framework code continues to use FreeRTOS locks. The
Linux port can suspend one task pthread and select another while the first owns
an ordinary host lock. If the selected task waits for that lock, the owner
cannot be rescheduled. `SchedulerSafeHostLock` prevents task handoff only for
the short host critical section; callbacks and waits remain outside it.

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

- Implement the LEDC [internal fade
  engine](ledc_voltage_emulation.md#phase-3-internal-fade-and-ordered-completion-engine)
  and [ESP-IDF fade API](ledc_voltage_emulation.md#phase-5-esp-idf-fade-and-blocking-api)
  over the completed alarm and interrupt services.
- Write a dedicated `esp_timer` backend design covering the fixed signal-safe
  compare ingress, LAC/SYSTIMER source profiles, framework startup/shutdown,
  `ESP_TIMER_TASK`, and a later `ESP_TIMER_ISR` configuration phase.
- Specify Arduino hardware-timer and GPTimer adapters in a separate design over
  the one-shot alarm and interrupt boundaries.
- Add further asynchronous peripheral completions only through consumer designs
  that define their state-publication and callback-context contracts.
