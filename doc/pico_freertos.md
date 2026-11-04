# Pico FreeRTOS host runtime

## Objective

Run Arduino-Pico applications and Roo consumers under a real, preemptive,
single-core FreeRTOS scheduler in roo_testing.

## Motivation

The initial Pico backend executes one application thread. Its mutex rejects
contention, UART waits block that thread, and delay advances time immediately.
These operations cannot support a second task making progress during a wait.

## Background

[Pico integration](pico_integration.md) provides the RP2040 board, Pico SDK
2.3.1, and Arduino-Pico 6.3.1. Runtime identity already distinguishes bare metal
and FreeRTOS. The ESP32 scheduler is an IDF-modified 10.5.1 kernel and must remain
isolated from the Pico graph. Arduino-Pico pins its own 11.1.0+ kernel at
`2e588af95bad29aef577373512883370c9408c1c`.

## Requirements

- Preserve all eight existing profiles and their APIs.
- Run native host executables with one emulated scheduler core, priorities,
  preemption, queues, synchronization, notifications, and software timers.
- Use the shared emulated uptime for scheduler deadlines and peripherals.
- Release execution during blocking I/O and serialize complete bus exchanges.
- Support bounded startup/shutdown, task deletion, and sanitizer validation.
- Keep ESP-IDF, ARM register code, USB, networking, and core 1 outside this graph.

## Design Overview

A new `roo_testing_arduino_rp2040_freertos` profile selects Arduino, RP2040,
Pico SDK capability, and FreeRTOS runtime. Existing public Arduino targets select
runtime-specific startup while reusing the same board and device models.

The pinned kernel runs on its POSIX port. Native pthreads implement kernel
contexts; FreeRTOS owns scheduling and blocking. The runtime bridge connects
kernel ticks and high-resolution peripheral waits to the shared clock.
Short host-state operations mask scheduler signals; blocking reservations use
FreeRTOS mutexes. Application callbacks never enter task APIs from a clock worker.

This separates application synchronization (FreeRTOS objects), short host-state
protection (scheduler-safe host locks), and hardware-bus reservations (held across
selected-device exchanges). No board lock spans a blocking operation.

## Design Details

### Kernel and identity

Import the exact Arduino-Pico FreeRTOS gitlink through a dedicated Copybara
workflow. Keep upstream kernel/port changes in a patch series and native host
configuration/glue in destination-owned files. Use one scheduler core and a
1 kHz tick; retain the pinned core's priority/API configuration where supported.
Host stacks have a 64 KiB minimum for signal delivery and pthread cancellation.
Tick counters are 32-bit, independent of native pointer/stack word size.

### Time and event delivery

Kernel tick zero is anchored when its scheduler starts. Auto mode derives elapsed
ticks from shared uptime, carrying sub-tick time instead of using an independent
wall-clock counter. Manual mode advances only through explicit clock operations
or the idle scheduler's next known deadline. An idle advance selects the earliest
kernel timeout, high-resolution runtime wait, or shared-clock alarm. Indefinite
waits with no pending deadline yield to host input without inventing time.

For example, tasks blocked for 10 ms and 20 ms at the same origin wake at their
own deadlines; the clock advances 20 ms overall. Tick-based APIs retain tick
rounding; SDK microsecond waits use runtime wait records rather than rounding
all bus transfers to a millisecond. Long idle gaps use kernel tick stepping before
the last wake tick. Time overflow is rejected before mutation.

Runtime wait records belong to the blocked task and are removed before its native
thread/stack is reclaimed. Scheduler-signal delivery performs only bounded,
nonallocating work. Host watchdogs always use host monotonic time, so a stalled
manual-time task can still fail a test.

### Startup and lifecycle

Before scheduler startup, clock reads/global initialization retain native behavior.
Arduino initialization and setup/loop execute in a kernel task after startup.
`delay()` and `yield()` release execution; `delayMicroseconds()` retains busy-wait
semantics. Core-1 entry points and affinity requests fail explicitly.

GoogleTest runs in a task and returns its exact status to process main. Shutdown
quiesces clock callbacks, deletes remaining application tasks that hold no controller reservations, ends the scheduler,
joins native contexts, and releases the final test task before process exit.
Support is one scheduler lifetime per executable; repeated lifecycle coverage
uses separate child processes, matching existing runners.

### Peripheral ownership

SDK state mutations are short and protected from preemption. UART waits park the
calling task. Arduino SPI transactions reserve their controller until
endTransaction; SDK transfers serialize individual calls. Wire has the pinned
core's shared buffer and no cross-call reservation. Applications must hold their
own mutex across beginTransmission/write/endTransmission/requestFrom/read when
sharing Wire between tasks. Unread RAM bytes do not keep a task reservation;
no-STOP bus state persists independently until a transfer emits STOP or end()
disables the controller. flush() is a no-op. Fixtures wire
borrowed devices before scheduling and retain them through scheduler shutdown.
Device callbacks must remain synchronous and nonblocking.

## Proposed API

```sh
bazel test //:arduino_pico_freertos_tests \
  --config=roo_testing_arduino_rp2040_freertos
```

Public `:arduino`, `:arduino_main`, and `:arduino_gtest_main` retain their names.
`//roo_testing/runtime:freertos` and `:freertos_headers` select the matching kernel
for runtime consumers such as `roo_testing/sys`. Existing bare-metal profiles
continue rejecting FreeRTOS headers. The portable core compiles with `__FREERTOS`; destination-owned runners replace
the hardware-specific upstream startup (USB, networking, and core 1).

## Implementation Plan

Authoring: [C++ instructions](../.github/instructions/general-cpp-code-authoring-instructions.md)
and [Copybara ownership](../copybara/README.md).

1. **Kernel prototype.** Pin/import the kernel, add the native config/profile,
   and prove task creation, priorities, preemption, deletion, and exit.
   Proposed commit: `Pico FreeRTOS stage 1: Add the pinned POSIX kernel prototype.`
   Validation: isolated kernel tests and dependency checks with no IDF graph.
2. **Clock bridge.** Add shared-uptime ticks, high-resolution task waits, manual
   idle advancement, and deletion-safe waiter cleanup.
   Proposed commit: `Pico FreeRTOS stage 2: Connect scheduling to shared time.`
   Validation: concurrent deadlines, queue/notification timeouts, wrap bounds,
   software timers, and busy-task preemption in auto mode.
3. **Synchronization and peripherals.** Add task-aware SDK mutexes, preemption
   protection for short state operations, controller reservations, and UART waits.
   Proposed commit: `Pico FreeRTOS stage 3: Make Pico peripherals task-aware.`
   Validation: contended mutexes, independent bus traffic, complete SPI framing,
   caller-serialized Wire sequences, and cancellation while blocked.
4. **Arduino runners.** Compile supported FreeRTOS core paths and provide
   scheduler-backed main/gtest startup and complete cleanup.
   Proposed commit: `Pico FreeRTOS stage 4: Run Arduino applications in tasks.`
   Validation: setup/loop, exact test status, process shutdown, explicit core-1
   rejection, and ASAN lifecycle tests.
5. **Roo integration.** Select kernel dependencies through runtime aliases and
   validate roo_threads and roo_time without IDF includes.
   Proposed commit: `Pico FreeRTOS stage 5: Connect Roo threading and time.`
   Validation: thread joins, mutexes, condition variables, timed waits, and
   source/dependency identity probes.
6. **Release validation.** Add the ninth profile to helpers/CI, document support,
   run normal/ASAN and previous-profile regressions.
   Proposed commit: `Pico FreeRTOS stage 6: Validate the ninth host profile.`
   Validation: profile isolation, wildcards, example consumers, all existing
   profiles, and serialized builds using persistent caches/resource limits.

## Testing Plan

Each stage owns focused tests, documentation, and its changes. Kernel tests use
bounded native executables. Integration tests cover actual FreeRTOS APIs and the
same fake-device models as the bare-metal backend. Test manual/auto time in
separate executables. Use child processes for expected failures and lifecycle
repetition. Verify identity via compile actions, dependency queries, and linked
symbols. Run AddressSanitizer and the eight existing profile suites sequentially.

## Caveats

This release models one scheduler core, not dual-core SMP timing. Hardware
interrupt sources, GPIO callbacks, DMA, PIO, USB, radio, and filesystems remain
outside the supported surface. Host stack sizes and scheduler overhead differ
from RP2040 firmware. Manual mode has no instruction-count model: compute-only
tasks must eventually block/yield or be terminated by a host watchdog.

### Rejected alternatives

- Reusing the IDF kernel/config would inject foreign APIs and a different kernel.
- Native threads without FreeRTOS would lose priorities, queues, and task semantics.
- Advancing time inside each task delay would incorrectly add concurrent waits.
- A complete ESP32 port refactor would enlarge regression risk before the Pico
  runtime proves which host mechanisms can actually be shared.

## Future Work

Dual-core SMP, SDK alarms/IRQs as application interrupt APIs, RP2350, Pico W,
networking, and asynchronous peripherals require separate contracts and tests.

## Implementation evidence

Stage 1: the isolated pinned-kernel prototype passes normal/ASAN tests for
priority handoff, preemption of a busy task, timed blocking, deletion, and exit.
Its configured dependency graph contains no ESP-IDF or ESP32 implementation.

Stage 2: normal auto/manual clock tests cover overlapping 10/20 ms deadlines,
50 us waits, preservation of application notification slots, shared-clock alarms,
queue timeouts, software timers, deleted blocked tasks, shutdown with a pending
waiter, and a near-50-day manual delay crossing the 32-bit tick rollover.

Stage 3: normal and ASAN tests cover contended SDK mutexes, a task-fed UART read,
deletion of a blocked UART reader, and overlapping transfers on independent SPI
controllers. The 21-test bare-metal Pico suite remains green. Native descriptors
retain their blocking/nonblocking mode; nonblocking console backpressure parks
FreeRTOS callers. Controller owners must finish transactions before deletion.

Stage 4: task-backed Arduino and SDK GoogleTest runners preserve exact exit
status and close pending clock captures. Process tests repeat successful and
failing lifetimes and reject core-1 entry points. Arduino tests exercise nested
critical sections, portable String code, busy waits, full SPI transactions and
caller-serialized Wire TX/repeated-START/RX exchanges across competing tasks.
Wire parity regressions verify buffer limits, no-op flush, lifecycle and errors,
and that unread RX or no-STOP bus state never reserves Wire for a task. A second
task can replace unread RX or alter an active TX buffer, matching the real core;
application mutexes are required to prevent these interleavings.
SPI frequencies use the SDK's divider rounding. Lifecycle/configuration changes
and fixture wiring must complete before concurrent peripheral use.

Stage 5: runtime library/header targets select the independently pinned kernel;
ESP-style Roo include paths are supplied by small forwarding headers. The legacy
sys adapter supports upstream critical-section signatures and native stack sizes
above 64 KiB. roo_threads' existing FreeRTOS backend works without production
changes. Dedicated auto/manual consumer tests verify roo_time Delay, shared
uptime, joins, semaphore handoffs, mutex contention, CV notification and timeout.
roo_time itself needs no implementation change: its emulation delay already uses
the shared clock and now reaches the task-aware runtime hook.

Stage 6: helpers enumerate nine profiles and CI has normal/ASAN FreeRTOS jobs.
Unrelated C and public-header C++ probes verify global identity, 32-bit ticks,
one core and the pinned kernel version. Configured dependency/compile-action
checks reject ESP32/IDF code and identity macros. Header-only runtime consumers
link without starting a scheduler. Default and recursive discovery skip fixtures
whose contracts require another runtime; the legacy SPI fixture now uses pins
present on all three ESP32 variants.

The cancellation stress test exposed a POSIX condition-wait cleanup race:
cancellation reacquires the event mutex, and without a cleanup handler the
subsequent signal deadlocks. The recorded event patch fixes both wait paths.
The direct test cancels 100 native waiters; 20 ASAN repetitions each of this test,
manual runner cleanup and the multi-process runner test pass. All three port
patches replay byte-for-byte from the pristine pinned kernel. Copybara itself
was unavailable locally; replay, allowlists and source comparisons validate the
recorded import instead.

The previous profile suites pass in normal and ASAN modes: 55 tests per Arduino
ESP32/S3/C3 profile, 17 per IDF profile, 21 for SDK Pico, and 23 for Arduino Pico
(with two SDK-frontend identity probes correctly skipped). The combined consumer
verifier passes 15 tests in each mode across roo_threads, roo_time, roo_scheduler
and the existing roo_display transport. Its dependency graph also excludes
ESP32/IDF. No production change was necessary in those latter three consumers.

To reproduce with coordinated sibling checkouts:

```sh
bazel test //:arduino_pico_freertos_tests --config=roo_testing_arduino_rp2040_freertos
bazel test //:arduino_pico_freertos_tests --config=roo_testing_arduino_rp2040_freertos --config=asan
python3 test/pico_freertos/verify_profile.py
python3 test/pico_freertos/verify_clients.py
```

The profile verifier accepts `ROO_TESTING_BAZEL_OUTPUT_BASE` for an existing
persistent workspace base. Client validation reuses the stable persistent
workspace from the Pico consumer verifier and shuts its extra server down.
Physical board/ARM builds are not part of host-runtime qualification; upstream
kernel files and hardware-specific startup remain separate ownership boundaries.
