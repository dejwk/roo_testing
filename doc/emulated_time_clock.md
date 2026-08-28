# Emulated-time clock

Status: Proposed

## Objective

Provide one process-wide monotonic uptime with immutable manual or host-following
progression, ISR-safe passive reads, and checked task/native clock mutation.

## Motivation

Emulated peripherals, alarms, and framework clock shims must agree on one
uptime. Tests also need deterministic control without leaking a mutable global
time policy between cases. Once host signals can preempt ordinary code, passive
clock reads must remain bounded and cannot take a lock held by the interrupted
task.

This design establishes that clock contract independently of any callback,
peripheral, or timer API.

## Background

[`timer.cpp`](../roo_testing/system/timer.cpp) owns roo_testing's intended
monotonic uptime. Arduino `micros()`/`millis()` and ESP-IDF
`esp_timer_get_time()` read it. The current implementation follows host elapsed
time by default and exposes `system_time_set_auto_sync()` to switch between host
and explicit progression at runtime.

The clock currently uses `high_resolution_clock`, whose steadiness is not
guaranteed by C++; has unsynchronized mutable state; and does not check every
unsigned lag or delay conversion for overflow. Clock reads are reachable from
emulated ISR context, including through Arduino `micros()`.

A repository-wide audit found no production caller that changes time mode while
running. Existing callers select deterministic behavior for a test or restore
process-global state after one. The affected tests are roo_testing's simple
timer test, the mixed manual/host-paced roo_scheduler suite, roo_prefs timing
tests, and the roo_windows touch test. They can instead select one policy per
test binary.

The Linux FreeRTOS port schedules task pthreads through signals. An ordinary
host mutex is unsafe for state shared by those tasks: task A can be suspended
while holding the mutex and task B can then block on it while A is not runnable.
Signals must be masked before such a mutex is acquired.

## Requirements

1. Passive uptime reads are monotonic, bounded, nonblocking, allocation-free,
   and safe from an emulated ISR.
2. Each process selects manual or host-following time before execution begins;
   the selection never changes during that process.
3. Host-following time uses a steady monotonic host clock and never publishes an
   uptime earlier than an already observed or explicitly published value.
4. Manual time advances only through explicit task/native operations.
5. Clock mutations are safe from multiple native threads and FreeRTOS tasks but
   are not POSIX-signal- or ISR-safe.
6. Duration conversion and addition fail before changing state when the result
   is not representable.
7. A task/native operation may synchronize or pace against host time, but a
   passive read never sleeps or runs unrelated work.
8. An ISR-visible microsecond delay occupies host time without changing global
   emulated uptime or dispatching callbacks.

### Out of scope

- Deadline callbacks and alarm delivery; see
  [Emulated-time alarms](emulated_time_alarms.md).
- Changing time mode after process startup.
- RTC, Unix time, calendar time, or deep-sleep clocks.
- Replacing ESP-IDF or Arduino critical sections and synchronization APIs.
- Non-Linux host support for the current FreeRTOS signal scheduler.

## Design Overview

The design introduces three internal concepts:

| Term | Meaning |
| --- | --- |
| Published uptime | An atomic process-wide monotonic floor, stored in signed nanoseconds. |
| Time mode | The immutable choice between manual and host-following progression. Auto-sync is the default. |
| Auto-sync origin | The immutable offset mapping `CLOCK_MONOTONIC` to emulated uptime in an auto-sync process. |

`AtomicUptimePublication` owns those values and is constant-initialized before
C++ dynamic initialization. Its passive read method is the entire ISR-visible
clock path. Slow mutation and synchronization use scheduler-safe host locking
outside that helper.

A final Bazel binary selects manual mode by depending on
`//roo_testing/system:manual_time_mode`. Otherwise the weak default selects
auto-sync. Link-time selection resolves before global constructors, `main()`,
Arduino `setup()`, or ESP-IDF `app_main()` can read the clock.

| Requirements | Design element |
| --- | --- |
| 1, 3 | Constant-initialized lock-free uptime and one-time immutable host origin |
| 2, 4 | Link-selected process mode |
| 5 | Scheduler-safe host lock for slow mutable state |
| 6 | Checked signed-nanosecond conversion and arithmetic |
| 7 | Separate passive-read and slow synchronization paths |
| 8 | Host-monotonic busy wait used only from ISR context |

## Design Details

### One-time clock publication

`AtomicUptimePublication` has namespace-scope `constinit` storage. Its uptime
and initialization-state atomics are statically asserted always lock-free. The
initialization state is one of `uninitialized`, `initializing`, `manual-ready`,
or `auto-ready`.

The final binary chooses the initial mode through one internal C-linkage
`volatile sig_atomic_t` symbol. The timer library supplies a weak auto-sync
definition, while the `alwayslink` manual-mode target supplies a strong manual
definition. The winner of first-use initialization reads that symbol once.

A first-use contender blocks maskable signals before attempting to become the
initializer. The winner samples `CLOCK_MONOTONIC`, calculates the immutable
host offset in auto mode, release-publishes the ready state, and restores its
signal mask. A task/native loser yields until it observes a ready state.

An ISR or passive reader that encounters `initializing` immediately returns the
already-published uptime floor. It neither waits nor reads the unpublished host
offset. An auto-mode reader that observes `auto-ready` acquire-loads the
immutable offset, forms a host-derived candidate, and atomically publishes the
later of that candidate and the current floor. A manual-mode read returns the
floor.

This bounded fallback can briefly return the previous floor during concurrent
first initialization. It preserves monotonicity and lets a later read catch up
without risking signal-side deadlock.

### Scheduler-safe slow state

`//roo_testing/host:synchronization` provides `SchedulerSafeHostLock` for
roo_testing-owned state shared between native pthreads and FreeRTOS task
pthreads. It saves the caller's POSIX signal mask, blocks all maskable signals,
then acquires a host mutex. Destruction unlocks before restoring the saved mask.

No callback, capture destruction, allocation retry, sleep, or thread join occurs
while this lock is held. This utility does not replace framework synchronization
or make arbitrary host code ISR-safe.

The existing host locks in `FakeGpioPin`, `SimpleVoltageSink`, and
`SimpleDigitalSink` migrate to this guard because alarm-driven peripherals can
later publish state from a native worker.

### Clock observation and mutation

`CLOCK_MONOTONIC` is the only host elapsed-time source. The auto-sync origin is
never rebased. Explicit mutation can move the published floor ahead of the host
mapping; host time subsequently catches up.

`system_time_get_micros()` only reads `AtomicUptimePublication` and converts its
result. It never locks, allocates, starts a worker, sleeps, or dispatches work.

`system_time_lag_ns()` and `system_time_delay_micros()` convert and add their
durations with checked signed-nanosecond arithmetic before publication.
`system_time_sync()` is a no-op in manual mode. In auto mode it observes the
fixed mapping and performs any ahead-of-host pacing outside locks. The
[alarm design](emulated_time_alarms.md#explicit-pump-points) makes the
task-context delay an explicit dispatch point without changing passive reads.

### ISR-visible busy delays

Arduino `delayMicroseconds()` and the ESP ROM delay shims inspect
`xPortInIsrContext()`. Task-context calls retain the explicit emulated-time delay.
ISR-context calls use an internal `system_time_busy_wait_micros()` helper.

The helper samples `CLOCK_MONOTONIC` and spins until the host duration expires.
It does not lock, allocate, publish emulated uptime, or dispatch callbacks. In
auto mode a later clock read observes the elapsed host time. In manual mode the
test driver retains sole control of emulated uptime. Scheduler and interrupt
signals remain deferred until the emulated ISR returns.

## Proposed API

Remove `system_time_set_auto_sync(bool)` after migrating its audited callers.
Add a task/native-only diagnostic query:

```cpp
bool system_time_is_auto_sync_enabled();
```

The existing API has this context contract:

| Operation | Emulated ISR | Task/native |
| --- | --- | --- |
| `system_time_get_micros()` | Yes | Yes |
| `system_time_is_auto_sync_enabled()` | No | Yes |
| `system_time_sync()` | No | Yes |
| `system_time_lag_ns()` | No | Yes |
| `system_time_delay_micros()` | No | Yes |
| internal `system_time_busy_wait_micros()` | Yes | Yes |

Auto-sync remains the zero-configuration default. A deterministic executable
selects manual mode through its dependency on
`//roo_testing/system:manual_time_mode`.

## Implementation Plan

Authoring reference: follow this repository's
[C++ code-authoring guidance](../.github/instructions/embedded-cpp-code-authoring.instructions.md).

### Phase 1: Scheduler-safe host locking

Add `//roo_testing/host:synchronization` and migrate the roo_testing-owned host
state listed above. Do not change clock semantics.

Proposed commit: `Guard shared host state against FreeRTOS signal preemption`

Validation: add focused mask-order and restoration tests plus a FreeRTOS
regression with competing task and native pthread access to each migrated lock.

### Phase 2: Link-selected time mode

Add the weak auto-sync symbol, the `alwayslink` manual override, and the read-only
mode query. Retain the runtime setter temporarily as deprecated so downstream
test binaries can migrate without a flag day. Add separate auto and manual
static-initialization test targets.

Proposed commit: `Select the emulated time mode at link time`

Validation: prove that both targets observe their selected mode from a global
constructor and the test body. Migrate roo_testing's simple timer test to the
manual target.

Before Phase 3, migrate every audited roo_scheduler, roo_prefs, and roo_windows
test caller to a process-isolated link-selected target. Track exact repository
versions and publication order in the rollout issue rather than this durable
design. A repository-wide search for `system_time_set_auto_sync()` is the gate
for removing the setter.

### Phase 3: Immutable ISR-safe monotonic clock

Add the constant-initialized publication core, switch to `CLOCK_MONOTONIC`, use
checked signed-nanosecond arithmetic, and remove the deprecated setter. Keep the
host offset immutable and all pacing outside passive reads.

Proposed commit: `Publish immutable emulated time through an ISR-safe clock`

Validation: cover both modes, concurrent mutation and reads, first use from an
ISR, an ISR interrupting initialization, publication visibility, overflow before
mutation, and compile-time lock-free and constant-initialization assertions.

### Phase 4: ISR-visible busy delays

Add the internal host-time busy-wait path and route Arduino and ESP ROM delay
shims to it from ISR context. Update shim documentation in the same change.

Proposed commit: `Keep ISR busy delays outside fake-time advancement`

Validation: invoke each shim from task and ISR contexts and verify that ISR calls
consume host time without changing emulated uptime, dispatching callbacks,
allocating, or losing deferred scheduler and interrupt signals.

## Testing Plan

Separate auto and manual binaries prove mode selection before `main()`. Shared
clock cases cover monotonic observation, checked mutation, synchronization,
first-use races, and ISR reads. FreeRTOS integration repeatedly reads Arduino and
ESP-IDF clocks from an emulated ISR while task and native contexts advance the
published floor. Busy-delay cases verify the different task and ISR behavior.

Tests never switch or restore time mode. Manual cases advance from their observed
starting uptime instead of assuming a global zero.

## Caveats

The weak/strong symbol and signal-mask protocol target the current Linux/ELF
host. A non-Linux port needs an equivalent pre-execution mode selector and a
clock source with the same monotonic and signal-safety contract.

An ISR racing first initialization can observe the previous published floor.
The read remains monotonic and bounded, and subsequent reads catch up after
publication completes.

### Rejected Alternatives

#### Keep runtime mode switching

No audited application requires it. Repeated switching would require coherent
republication of the host origin and worker transitions, while retaining
process-global cleanup hazards between tests.

#### Select mode during runner startup

Global constructors can read time before `main()`, `setup()`, `app_main()`, or a
GTest environment runs. Link selection establishes the contract earlier without
adding an environment lookup to the ISR-visible first-use path.

#### Lock or spin during passive reads

An ISR can interrupt the task that owns a lock or is updating a seqlock. Waiting
or spinning would deadlock that interrupted task. Returning the published floor
during initialization keeps the read bounded.

#### Advance emulated time from ISR busy delays

Clock mutation and alarm dispatch are task/native operations. A host-monotonic
busy loop models occupation of the emulated core without turning an ISR into a
global time driver.
