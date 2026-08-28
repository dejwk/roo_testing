# Emulated interrupts

Status: Partially implemented

## Objective

Run host-emulated peripheral interrupt handlers in recognizable FreeRTOS ISR
context, including while application code is CPU-bound, without advancing the
emulated clock or blocking unrelated tasks.

## Motivation

Many ESP-IDF and Arduino drivers complete asynchronous operations in interrupt
handlers. Calling those handlers from an [alarm drainer or native
waiter](emulated_time_alarms.md#dispatch-and-re-entry) gives them the wrong
context, while delivering only at FreeRTOS calls cannot interrupt a CPU-busy
loop. Advancing global emulated time to imitate a busy wait also changes time
for every task and conflicts with wall-clock synchronization.

A reusable interrupt path lets peripheral models publish completion state and
request the associated framework interrupt. The registered handler then runs
with the same FreeRTOS ISR-facing contract it uses on hardware, including
`FromISR` APIs and deferred task switching.

## Background

On hardware, an interrupt handler does not run as a FreeRTOS task. The CPU
suspends the current task context, enters an interrupt frame, masks interrupts
according to the architecture's rules, invokes the handler, and may select a
different task when the outermost interrupt returns.

roo_testing uses ESP-IDF's single-core pthread-backed [Linux FreeRTOS
port](../roo_testing/frameworks/esp-idf/components/freertos/FreeRTOS-Kernel/portable/linux/port.c).
Each FreeRTOS task owns a pthread, but only the selected task is intended to
execute. The port already uses POSIX signals for the scheduler tick and task
handoff. Peripheral delivery adds a dedicated signal targeted at the currently
selected FreeRTOS pthread, allowing host code to interrupt ordinary
instructions rather than waiting for a cooperative call site.

The port tracks two related but distinct depths:

- `uxCriticalNesting` is logical critical-section/ISR exclusion bookkeeping
  for the active FreeRTOS context. Direct signal masking need not change it.
- `uxInterruptNesting` counts active ISR frames for that context. A task
  critical section changes only `uxCriticalNesting`; ISR entry changes both
  because the POSIX handler also runs with port signals masked.

These values are saved on a suspended pthread's stack across an ISR-triggered
task switch, so the globals always describe the selected execution context.
The signal-shared depths and ISR-exit yield latch use `volatile sig_atomic_t`;
ordinary FreeRTOS integer types are not sufficient for asynchronous signal
access even when their machine representation happens to be atomic.

ESP-IDF's [interrupt allocator](../roo_testing/frameworks/esp-idf/components/esp_hw_support/include/esp_intr_alloc.h)
accepts target-specific signed interrupt sources. Positive `ETS_*` values vary
across ESP32, C3, S3, C6, and other SoCs, while negative pseudo-sources denote
CPU-local interrupts. A reusable host controller therefore cannot use a
classic-ESP32 source count or source number as generic storage identity.

## Requirements

1. A native host event can interrupt a CPU-busy FreeRTOS task without a clock
   mutation or cooperative safe point.
2. Framework handlers observe FreeRTOS ISR context and can use the supported
   `FromISR` APIs.
3. A task critical section defers peripheral delivery, and only the interrupted
   task's execution is suspended.
4. A requested yield occurs after the outermost emulated handler returns, not
   in the middle of dispatcher bookkeeping.
5. Repeated assertions for one interrupt coalesce. Masking its delivery retains
   the asserted state, and unmasking it requests delivery.
6. Removing and later replacing a handler cannot redirect a stale host event
   to the replacement.
7. Signal-side delivery is lock-free, nonblocking, nonthrowing, and performs no
   allocation.
8. Host interrupt routing and storage do not assume Espressif source numbering
   or a target-specific interrupt count.
9. ESP-IDF-facing behavior preserves handler arguments, signed sources,
   initial enable state, anonymous allocations, same-source vector
   compatibility, shared-handler chaining, status filtering, and documented
   invalid flag combinations.
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

In this document, a *source assertion* is one event identified by a signed
ESP-IDF source value. While that source is allocated, one adapter *source
vector* owns one generic controller registration. A *controller registration*
is a fixed slot plus generation, handler, argument, enabled state, and one
coalesced pending bit. Each public `intr_handle_t` is a separate *registration
record* that owns one framework handler and may share its source vector with
compatible records. A record's matched bit captures status-filter eligibility
for the next vector pass, whereas the controller pending bit requests the pass
itself. Controller generations and adapter publication epochs identify one
table-slot lifetime and permanently retire at exhaustion. A *status snapshot*
is a borrowed address/value pair materialized by a peripheral for one source
assertion; the address is an opaque target key. The final port nudge carries no
source, vector, or handler identity.

This split satisfies Requirements 1-4 in the port, Requirements 5-8 in the
controller, and Requirements 9-10 at the adapter boundary.

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

Bounded signal-side work and stale-lifetime rejection require fixed storage with
no allocation or pointer reuse in the dispatcher. The controller therefore has
64 static registration slots. Each slot contains lock-free atomics for a
handler, argument, generation, allocated/enabled flags, and a coalesced pending
bit. A value handle contains a slot and generation. Reusing a slot increments
its generation, so a delayed producer holding an old handle cannot assert the
replacement registration. Generations never wrap: after a slot publishes its
maximum generation and that registration is removed, the slot is permanently
retired. Registration tries another slot and eventually returns `kNoCapacity`
rather than making an ancient handle valid again.

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

Preserving ESP-IDF sharing and status filtering without putting target source
IDs in the generic controller requires a separate fixed mapping layer. The
adapter therefore has source-vector and public-registration tables. Source
values are signed, compared as values, and never used as array indexes. One
live source maps to one logical vector and therefore one generic controller
slot. Its signal-side vector handler walks every compatible shared record in a
single ISR pass, matching ESP-IDF's `shared_intr_isr` structure instead of
entering a separate host interrupt for each handler.

Source-vector publication epochs and registration epochs likewise never wrap.
An entry is retired after its last representable epoch; allocation skips
retired entries and returns `ESP_ERR_NOT_FOUND` when no fresh identity remains.

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
The snapshot overload supplies address/value pairs materialized from the
peripheral's authoritative emulated state. Before nudging the port, the adapter
marks unconditional shared handlers and only status-filtered handlers whose
matching snapshot has a masked bit set. Missing status means “not known
asserted,” never “dereference this target address.” Non-shared handlers ignore
intrstatus metadata, as ESP-IDF's direct non-shared dispatch does. Filtering
occurs at assertion time rather than ISR entry so borrowed snapshots require no
asynchronous lifetime.

### Alarm and peripheral integration

The [emulated-time alarm service](emulated_time_alarms.md#consumer-adapters)
remains a neutral deadline mechanism. A hardware-like consumer handles a due
deadline in this order:

1. materialize peripheral status and completion state without invoking the
   public framework ISR;
2. finish any consumer-visible publication that hardware completes before the
   interrupt, using an ordered post-publication continuation when external sink
   delivery cannot occur inline;
3. publish ISR-facing status and release alarm, peripheral, delivery, and sink
   locks;
4. raise the adapter's signed source with any required status snapshots; and
5. let the registered framework handler run through the signal path.

Task-dispatched timers add another layer. Real `esp_timer` first enters its
hardware timer ISR; ISR-dispatch callbacks run there, while ordinary callbacks
are released by notifying the dedicated timer task. A dedicated future backend
design will specify both the initial task-dispatch path and later public
ISR-dispatch support; neither is part of this interrupt proposal.

## Proposed API

The implemented generic API is declared in
[`interrupt_controller.h`](../roo_testing/interrupts/interrupt_controller.h):

```cpp
using InterruptHandler = void (*)(void* argument);

struct InterruptHandle {
  uint32_t slot;
  uint32_t generation;
};

enum class InterruptRegistrationResult {
  kRegistered,
  kInvalidArgument,
  kWrongContext,
  kNoCapacity,
  kBackendUnavailable,
};

InterruptRegistrationResult registerInterrupt(
    InterruptHandler handler, void* argument, bool initially_enabled,
    InterruptHandle* out_handle);
bool unregisterInterrupt(InterruptHandle handle);
bool enableInterrupt(InterruptHandle handle) noexcept;
bool disableInterrupt(InterruptHandle handle) noexcept;
bool setInterruptPending(InterruptHandle handle) noexcept;
```

Registration and unregistration require a scheduler-running FreeRTOS task;
`kWrongContext` reports any other caller. `kNoCapacity` includes both live and
permanently retired slots, and `kBackendUnavailable` reports a conflicting or
unavailable port dispatcher. Enable, disable, and pending are lock-free
operations available to native producers and emulated ISRs. A false boolean
result means the value handle no longer identifies a live registration.

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

Those five functions are the complete implemented host allocator surface in
this design. Other declarations from `esp_intr_alloc.h`, including binding,
inspection, reservation, IRAM/non-IRAM control, and interrupt-number control,
are intentionally absent from the host link surface. A consumer that uses one
fails at link time rather than receiving a silent success from a no-op shim.
Adding any such API requires a separate design that specifies its observable
host semantics.

## Implementation Plan

Follow the repository's
[embedded C++ authoring guidance](../.github/instructions/embedded-cpp-code-authoring.instructions.md).

### Phase 1: FreeRTOS ISR bookkeeping

Track active interrupt frames separately from logical critical nesting, expose
accurate ISR queries, and make zero- and one-argument `portYIELD_FROM_ISR` forms
compatible with ESP-IDF and Arduino code.

Implemented commit:
[`8e66ad73`](https://github.com/dejwk/roo_testing/commit/8e66ad73)
`Recognize ISR context in the FreeRTOS Linux port`

Validation: tick handlers and ISR-unblocked tasks observe the correct context;
task switches restore both nesting counters.

### Phase 2: Preemptive signal transport

Add payload-free signal ingress, selected-pthread publication, pending
coalescing, late installation, critical-section deferral, and ISR-exit yields.

Implemented commit:
[`56aa4072`](https://github.com/dejwk/roo_testing/commit/56aa4072)
`Deliver simulated interrupts through the FreeRTOS Linux port`

Validation: interrupt CPU-busy code, deliberately clobber and restore `errno`,
defer through nested critical sections until the outer exit, drain re-requests,
and exercise both argument forms of `portYIELD_FROM_ISR()` while waking a
higher-priority task only at ISR exit.

### Phase 3: Generic controller and ESP-IDF adapter

Add generation-checked fixed registrations, lock-free assertion and dispatch,
logical same-source vectors, safe status snapshots, real allocator handles,
and focused lifecycle tests.

Implemented commit:
[`5616d666`](https://github.com/dejwk/roo_testing/commit/5616d666)
`Add the emulated interrupt controller and ESP-IDF adapter`

Validation: run controller and allocator suites repeatedly with shuffled test
order, including disabled pending delivery, stale handles, handler reassertion,
shared chaining/filtering and compatibility, anonymous allocation, native
source assertion racing reuse, and invalid flags.

### Phase 4: Signal and identity hardening

Use signal-safe scalar types for port bookkeeping, preserve `errno`, verify
nested critical deferral and both yield forms, and permanently retire exhausted
controller generations and adapter epochs.

Implemented commit:
[`cfa45f11`](https://github.com/dejwk/roo_testing/commit/cfa45f11)
`Harden emulated interrupt identity and signal state`

Validation: run the focused port/controller/allocator suites uncached and with
20 shuffled repetitions. Reduced identity-space variants must reach exhaustion
without making any stale handle, callback argument, generation, or epoch live
again.

### Phase 5: Alarm-backed LEDC interrupt completion

After all [emulated-time alarm phases](emulated_time_alarms.md#implementation-plan),
make the LEDC fade consumer materialize status and raise its logical source.
Include ISR semaphore release, callback context, and task-local blocking. This
is the interrupt slice of [LEDC Phase
5](ledc_voltage_emulation.md#phase-5-esp-idf-fade-and-blocking-api), not a
second sequential implementation.

Proposed commit: `Emulate ESP-IDF LEDC fades through interrupts`

Validation uses two isolated Bazel binaries whose immutable mode is selected by
linkage before execution. `//test:idf_ledc_freertos_autosync_test` proves that
an auto-synchronized deadline interrupts a CPU-busy task without an explicit
pump. `//test:idf_ledc_freertos_manual_test` links
`//roo_testing/system:manual_time_mode` and uses a separate native host
time-driver thread to advance and pump the deadline before the source interrupts
that busy task; a lower-priority FreeRTOS driver is not used because it cannot
preempt the busy task. Blocked same-channel tasks resume while unrelated tasks
continue; callbacks see ISR context and can request a yield. Update the LEDC
design status and its fade example in the same commit.

## Testing Plan

The implemented port layer is covered by
`//test:freertos_posix_isr_context_test`,
`//test:freertos_posix_simulated_interrupt_install_test`, and
`//test:freertos_posix_simulated_interrupt_test`. The generic layer is covered
by `//test:interrupt_controller_test` and
`//test:interrupt_controller_exhaustion_test`. The ESP-IDF boundary is covered
by `//test:esp_intr_alloc_test`, `//test:esp_intr_alloc_anonymous_test`, and
`//test:esp_intr_alloc_epoch_exhaustion_test`.

Run the controller and allocator suites with Bazel `--runs_per_test` and the
GTest `--gtest_shuffle` argument to expose process-global installation and
stale-state dependencies. Phase 5 adds its two integration targets in the same
commit; those targets combine emulated deadlines with task and ISR
observations rather than duplicating the layer-focused suites.

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
generation before entering `raiseInterruptSource()`, following the [alarm
consumer lifetime contract](emulated_time_alarms.md#consumer-adapters).

### Rejected Alternatives

#### Deliver only at FreeRTOS safe points

This cannot interrupt a CPU-busy loop and would make peripheral correctness
depend on unrelated API calls.

#### Advance emulated time from a blocking driver call

Global time advancement affects every task and conflicts with a process
configured for host-clock synchronization. A blocking peripheral call must
suspend only its FreeRTOS task.

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

The following work is intentionally outside this design and requires a
separate design document before implementation:

- Adding peripheral-specific status-snapshot integration beyond the LEDC
  adapter selected in Phase 5.
- Adding allocator binding, inspection, reservation, IRAM/non-IRAM control, or
  interrupt-number control APIs to the implemented host link surface.
- Defining target vector inventories, cross-source vector packing, and source
  profiles for additional Espressif SoCs.
- Adding SMP delivery, per-core selected-pthread ingress, and affinity. The
  separate design must provide handler quiescence across dispatching cores.
- Adding priority preemption, nested interrupt delivery, NMI, or high-level
  handlers. The separate design must define their POSIX signal masks and
  FreeRTOS critical-section interaction.
- Adding a host `esp_timer_impl_*` backend. Its separate design must cover the
  signal-safe fixed compare source, the initial `ESP_TIMER_TASK` path, startup
  and teardown, and a later `ESP_TIMER_ISR` configuration phase.
- Routing GPIO, Arduino hardware timers, GPTimer, and further peripheral shims
  through the controller.
