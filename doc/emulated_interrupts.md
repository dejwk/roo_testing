# Emulated interrupts

Status: Partially implemented

## Objective

Run host-emulated peripheral interrupt handlers in recognizable FreeRTOS ISR
context, including while application code is CPU-bound, without advancing the
emulated clock or blocking unrelated tasks.

## Motivation

Many ESP-IDF and Arduino drivers complete asynchronous operations in interrupt
handlers. Calling those handlers from an alarm-pump task gives them the wrong
context, while delivering only at FreeRTOS calls cannot interrupt a CPU-busy
loop. Advancing global emulated time to imitate a busy wait also changes time
for every task and conflicts with wall-clock synchronization.

A reusable interrupt path lets peripheral models publish state and assert a
logical source. The registered framework handler then runs with the same
FreeRTOS ISR-facing contract it uses on hardware, including `FromISR` APIs and
deferred task switching.

## Background

On hardware, an interrupt handler does not run as a FreeRTOS task. The CPU
suspends the current task context, enters an interrupt frame, masks interrupts
according to the architecture's rules, invokes the handler, and may select a
different task when the outermost interrupt returns.

roo_testing uses ESP-IDF's single-core pthread-backed Linux FreeRTOS port. Each
FreeRTOS task owns a pthread, but only the selected task is intended to execute.
The port already uses POSIX signals for the scheduler tick and task handoff.
Peripheral delivery adds a dedicated signal targeted at the currently selected
FreeRTOS pthread, allowing host code to interrupt ordinary instructions rather
than waiting for a cooperative call site.

The port tracks two related but distinct depths:

- `uxCriticalNesting` is logical critical-section/ISR exclusion bookkeeping
  for the active FreeRTOS context. Direct signal masking need not change it.
- `uxInterruptNesting` counts active ISR frames for that context. A task
  critical section changes only `uxCriticalNesting`; ISR entry changes both
  because the POSIX handler also runs with port signals masked.

These values are saved on a suspended pthread's stack across an ISR-triggered
task switch, so the globals always describe the selected execution context.

ESP-IDF's allocator accepts target-specific signed interrupt sources. Positive
`ETS_*` values vary across ESP32, C3, S3, C6, and other SoCs, while negative
pseudo-sources denote CPU-local interrupts. A reusable host controller therefore
cannot use a classic-ESP32 source count or source number as generic storage
identity.

## Requirements

1. A native host event can interrupt a CPU-busy FreeRTOS task without a clock
   mutation or cooperative safe point.
2. Framework handlers observe FreeRTOS ISR context and can use the supported
   `FromISR` APIs.
3. A task critical section defers peripheral delivery, and only the interrupted
   task's execution is suspended.
4. A requested yield occurs after the outermost emulated handler returns, not
   in the middle of dispatcher bookkeeping.
5. Pending requests coalesce. Disabling a registration retains pending state,
   and enabling it requests delivery.
6. Registration, removal, and reuse cannot redirect a stale host event to a new
   handler.
7. Signal-side delivery is lock-free, nonblocking, nonthrowing, and performs no
   allocation.
8. Generic controller storage is independent of Espressif source numbers and
   SoC-specific interrupt counts.
9. The ESP-IDF adapter preserves handler arguments, signed sources, initial
   enable state, anonymous allocations, same-source vector compatibility,
   shared-handler chaining, status filtering, and documented invalid flag
   combinations.
10. Target MMIO addresses are opaque numeric keys and are never dereferenced
    as host pointers.

### Out of scope

- SMP or true dual-core interrupt affinity.
- Nested priority preemption, NMI, high-level assembly handlers, and exact CPU
  vector allocation.
- Bit-accurate interrupt-matrix routing, edge acknowledgement, IRAM placement,
  and cache-disabled execution.
- A generic target-MMIO register file. Peripheral models explicitly supply
  materialized status snapshots when asserting a source.
- Asynchronous host-signal portability beyond the Linux pthread FreeRTOS
  backend.

## Design Overview

Delivery has three layers:

```text
peripheral/alarm model
        |
        | assert signed ESP source
        v
ESP-IDF adapter ---- esp_intr_alloc/free/enable/disable
        |
        | set opaque registration pending
        v
SoC-neutral controller ---- fixed slot + generation
        |
        | payload-free process nudge
        v
FreeRTOS Linux port ---- SIGUSR2 on selected task pthread
        |
        v
framework ISR ---- FromISR API, optional yield at ISR exit
```

The port transports only “interrupt work is pending.” It has no source IDs,
priorities, or framework handles. The generic controller owns fixed-capacity
handler registrations and coalesced pending bits. The ESP-IDF adapter maps one
logical source vector onto a controller registration and walks compatible
shared `intr_handle_t` records from that vector handler. Peripheral models
update their authoritative state before raising a source.

This split satisfies Requirements 1-4 in the port, Requirements 5-7 in the
controller, and Requirements 8-10 at the adapter boundary.

## Design Details

### POSIX signal ingress

The Linux port reserves `SIGUSR2` for simulated peripheral work. Native
producers atomically set a binary pending bit, load the selected FreeRTOS
pthread, and send the signal. Publishing the selected pthread before waking it
and re-kicking pending work at every handoff closes the race where selection
changes between those operations.

The signal handler preserves `errno`, enters FreeRTOS ISR bookkeeping, and
drains the one-time-installed controller dispatcher. A request made before
dispatcher installation remains pending and is kicked by successful
installation. Requests made during dispatch set the bit again and are drained
without recursively entering another signal frame.

All port-controlled signals are masked in critical sections. Consequently a
CPU-busy loop is preemptible, while a loop inside a FreeRTOS critical section is
not. Directly masking the signal outside port APIs has the corresponding POSIX
effect but does not alter FreeRTOS nesting bookkeeping.

### ISR exit and task switching

`portYIELD_FROM_ISR()` latches a yield while interrupt nesting is nonzero. The
outermost interrupt exit clears ISR bookkeeping first and then performs the
FreeRTOS context selection and pthread handoff. The resumed task therefore sees
task context, and no awakened task runs in the middle of a framework handler.

### Generic controller

The controller has 64 static registration slots. Each slot contains lock-free
atomics for a handler, argument, generation, allocated/enabled flags, and a
coalesced pending bit. A value handle contains a slot and generation. Reusing a
slot increments its generation, so a delayed producer holding an old handle
cannot assert the replacement registration.

Registration and unregistration are FreeRTOS-task operations. They mask port
signals while publishing or retiring handler lifetime. Enable, disable, and
pending operations use only lock-free atomics and may run in an emulated ISR or
native producer. A disabled registration keeps its pending bit; enabling it
nudges the port.

On the current single-core backend, successful unregistration is quiescent:
the sole dispatching FreeRTOS pthread cannot still be running the retired
handler. A future SMP backend must add per-core affinity or in-flight
quiescence before making the same lifetime guarantee; generation checks alone
prevent stale delivery but do not stop another core that already claimed a
handler.

The signal-side dispatcher claims a pending bit before calling its handler.
The handler may therefore assert itself or another registration. The
dispatcher rescans until no enabled pending handler remains. Handler call order
is deliberately unspecified because real allocation and shared-vector order
are not portable API guarantees.

### ESP-IDF adapter

The adapter has fixed source-vector and public-registration tables. Source
values are signed, compared as values, and never used as array indexes. One
live source maps to one logical vector and therefore one generic controller
slot. Its signal-side vector handler walks every compatible shared record in a
single ISR pass, matching ESP-IDF's `shared_intr_isr` structure instead of
entering a separate host interrupt for each handler.

When a source already has a vector, a second non-shared allocation or a
shared/non-shared mix returns `ESP_ERR_NOT_FOUND`. Same-source shared
allocations must agree on IRAM classification, and the existing vector's
selected level must be allowed by the new request. The first vector selects
the lowest requested level after applying ESP-IDF's default masks. Exact
cross-source packing into a target's finite CPU vectors remains target-profile
work; unrelated logical sources do not spuriously share a host vector in this
phase.

`ESP_INTR_FLAG_INTRDISABLED` controls each record's initial state. The adapter
enables the controller vector while any record is enabled. If the whole source
is disabled, a coalesced source assertion remains pending at the controller and
is delivered when one record enables. If another shared handler remains
enabled, the shared-vector pass skips disabled records rather than retaining a
private callback for them. A null `ret_handle` still leaves a live permanent
record, matching ESP-IDF's anonymous-allocation behavior.

The adapter rejects shared-plus-edge, a C handler requested at high/NMI level,
a shared null handler, a shared negative source, and a nonzero status address
without a mask. `ESP_INTR_FLAG_IRAM` is retained as host metadata because a host
function address cannot pass target IRAM classification.

For `esp_intr_alloc_intrstatus()`, the 32-bit target register is an opaque key.
A peripheral assertion may provide address/value snapshots that were
materialized from its authoritative emulated state. Before nudging the port,
the adapter marks unconditional shared handlers and only status-filtered
handlers whose matching snapshot has a masked bit set. Missing status means
“not known asserted,” never “dereference this target address.” Non-shared
handlers ignore intrstatus metadata, as ESP-IDF's direct non-shared dispatch
does. Filtering occurs at assertion time rather than ISR entry so borrowed
snapshots require no asynchronous lifetime.

### Alarm and peripheral integration

The emulated-time alarm service remains a neutral deadline mechanism. A
hardware-like consumer handles a due deadline in this order:

1. materialize peripheral status and completion state without invoking the
   public framework ISR;
2. finish any consumer-visible publication that hardware completes before the
   interrupt, using an ordered post-publication continuation when external sink
   delivery cannot occur inline;
3. publish ISR-facing status and release alarm, peripheral, delivery, and sink
   locks;
4. raise the adapter's signed source with any required status snapshots; and
5. let the registered framework handler run through the signal path.

Task-dispatched timers add another layer. For example, real `esp_timer` first
enters its hardware timer ISR. ISR-dispatch callbacks run there, while ordinary
callbacks are released by notifying the dedicated timer task. The emulator can
model both using the same deadline engine: raise a timer interrupt, then let
its ISR either invoke ISR callbacks or wake the timer task.

## Proposed API

The implemented generic API is declared in
[`interrupt_controller.h`](../roo_testing/interrupts/interrupt_controller.h):

```cpp
using InterruptHandler = void (*)(void* argument);

struct InterruptHandle {
  uint32_t slot;
  uint32_t generation;
};

InterruptRegistrationResult registerInterrupt(
    InterruptHandler handler, void* argument, bool initially_enabled,
    InterruptHandle* out_handle);
bool unregisterInterrupt(InterruptHandle handle);
bool enableInterrupt(InterruptHandle handle) noexcept;
bool disableInterrupt(InterruptHandle handle) noexcept;
bool setInterruptPending(InterruptHandle handle) noexcept;
```

The implemented ESP-facing source assertion is declared in
[`esp_interrupts.h`](../roo_testing/frameworks/esp_idf_support/esp_interrupts.h):

```cpp
struct InterruptStatusSnapshot {
  uint32_t target_address;
  uint32_t value;
};

void raiseInterruptSource(int source) noexcept;
void raiseInterruptSource(int source,
                          const InterruptStatusSnapshot* snapshots,
                          size_t snapshot_count) noexcept;
```

ESP-IDF code continues to use its existing `esp_intr_alloc`,
`esp_intr_alloc_intrstatus`, `esp_intr_free`, `esp_intr_enable`, and
`esp_intr_disable` APIs.

## Implementation Plan

Follow the repository's
[embedded C++ authoring guidance](../.github/instructions/embedded-cpp-code-authoring.instructions.md).

### Phase 1: FreeRTOS ISR bookkeeping

Track active interrupt frames separately from logical critical nesting, expose
accurate ISR queries, and make zero- and one-argument `portYIELD_FROM_ISR` forms
compatible with ESP-IDF and Arduino code.

Implemented commit: `Recognize ISR context in the FreeRTOS Linux port`

Validation: tick handlers and ISR-unblocked tasks observe the correct context;
task switches restore both nesting counters.

### Phase 2: Preemptive signal transport

Add payload-free signal ingress, selected-pthread publication, pending
coalescing, late installation, critical-section deferral, and ISR-exit yields.

Implemented commit: `Deliver simulated interrupts through the FreeRTOS Linux port`

Validation: interrupt CPU-busy code, preserve `errno`, defer under critical
nesting, drain re-requests, and wake a higher-priority task only at ISR exit.

### Phase 3: Generic controller and ESP-IDF adapter

Add generation-checked fixed registrations, lock-free assertion and dispatch,
logical same-source vectors, safe status snapshots, real allocator handles,
and focused lifecycle tests.

Implemented commit: `Add the emulated interrupt controller and ESP-IDF adapter`

Validation: run controller and allocator suites repeatedly with shuffled test
order, including disabled pending delivery, stale handles, handler reassertion,
shared chaining/filtering and compatibility, anonymous allocation, native
source assertion racing reuse, and invalid flags.

### Phase 4: Alarm-backed peripheral interrupts

Have alarm consumers materialize status and raise a logical source. Integrate
LEDC fade completion first, including ISR semaphore release, callback context,
and task-local blocking. Follow with Arduino hardware timers or GPTimer.

Proposed commit: `Route LEDC fade completion through emulated interrupts`

Validation: a CPU-busy task is interrupted at a fake-time completion; blocked
same-channel tasks resume while unrelated tasks continue; callbacks see ISR
context and can request a yield.

### Phase 5: Additional SoCs and fidelity

Supply target profiles and peripheral source constants without changing the
generic controller. Add per-core ingress before enabling SMP profiles, and add
priority/nesting only with tests that demonstrate a consumer requirement.

Proposed commit: `Add target-specific interrupt routing profiles`

Validation: compile and run source-routing contracts for each supported SoC;
verify no generic storage depends on an `ETS_MAX_INTR_SOURCE` value.

## Testing Plan

Port tests cover POSIX delivery, bookkeeping, task handoff, critical deferral,
and ISR-exit yields. Controller tests cover coalescing, disable/enable latching,
generation reuse, handler reassertion, and signal-side enable/disable. ESP-IDF
adapter tests cover signed source routing, native producers, shared handlers,
same-source compatibility, status snapshots, anonymous allocations, flags,
status-address safety, and handle lifecycle.

Peripheral integration tests will combine fake-time deadlines with task and ISR
observations. Stress runs use Bazel's repeated-test and GTest shuffle options to
expose process-global installation and stale-state dependencies.

## Caveats

The signal backend is more faithful than cooperative safe points for ordinary
code, but it is not a CPU instruction-level simulator. Host operations that
block or mask signals have POSIX behavior, and C/C++ code reached from the
handler must obey the documented signal-safe subset in addition to using
FreeRTOS `FromISR` APIs.

One signal represents all peripheral work, so the current controller drains
serially without nested priorities. This is sufficient for low/medium C
handlers and task-wakeup semantics; it does not claim NMI or high-level vector
fidelity.

The current backend is intentionally single-core. Reporting CPU 0 or accepting
affinity metadata later is not equivalent to SMP delivery; per-core selected
pthread state and interrupt ingress must precede true multi-core support.
Controller unregistration also relies on that single dispatch context for
handler quiescence.

Unlike the hardware allocator, the current adapter installs and frees records
only from a running FreeRTOS task; pre-scheduler allocation is deferred until
the port has an initialization-safe registration path. Public handles are
addresses of reusable fixed records, so using a handle after successful free
remains invalid and may alias a later allocation, just as use-after-free is
outside ESP-IDF's contract. Edge assertions currently use the same coalesced
disabled-source behavior as level assertions.

Source assertion protects a call already in progress from crossing vector
free/reuse. It cannot identify the causal lifetime of a producer that calls
only after an old peripheral operation was cancelled and the same numeric
source was reallocated. Alarm and peripheral models must validate their own
generation before entering `raiseInterruptSource()`.

### Rejected Alternatives

#### Deliver only at FreeRTOS safe points

This cannot interrupt a CPU-busy loop and would make peripheral correctness
depend on unrelated API calls.

#### Advance emulated time from a blocking driver call

Global time advancement affects every task and conflicts with auto-sync. A
blocking peripheral call must suspend only its FreeRTOS task.

#### Run handlers on a dedicated dispatcher task

That gives callbacks task context rather than ISR context and changes which
APIs and yield semantics are valid.

#### Put Espressif source IDs in the generic controller

Source values, aliases, and counts differ across SoCs. Keeping them in the
adapter avoids classic-ESP32 assumptions in reusable storage.

#### Build ESP-IDF's hardware allocator unchanged

The vendored allocator depends on target CPU-vector and interrupt-matrix
primitives that are no-ops in the Linux profile. Reusing its policy while
replacing its hardware mechanism is smaller and makes unsupported fidelity
explicit.

## Future Work

- Integrate status snapshots with each emulated peripheral that uses
  `esp_intr_alloc_intrstatus`.
- Implement the remaining allocator inspection, binding, IRAM, and non-IRAM
  APIs as consumers require them.
- Add target vector inventories when exact cross-source vector packing becomes
  observable to a supported driver.
- Add per-core ingress and affinity when a supported Espressif SMP profile is
  introduced.
- Add priority and nested interrupt modeling only after defining its interaction
  with POSIX signal masks and FreeRTOS critical sections.
- Route GPIO, hardware timer, GPTimer, and other peripheral shims through the
  same controller.
