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
quiesces clock callbacks, deletes remaining application tasks, ends the scheduler,
joins native contexts, and releases the final test task before process exit.
Support is one scheduler lifetime per executable; repeated lifecycle coverage
uses separate child processes, matching existing runners.

### Peripheral ownership

SDK state mutations are short and protected from preemption. UART waits park the
calling task. Arduino SPI transactions reserve their controller until
endTransaction; SDK transfers serialize individual calls. Wire protects a complete
beginTransmission/write/endTransmission sequence and read state. Fixtures wire
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
continue rejecting FreeRTOS headers. During the kernel prototype, public Arduino
startup remains unavailable until the task-aware core adapter stage lands.

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
   Wire sequence isolation, and cancellation while blocked.
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
