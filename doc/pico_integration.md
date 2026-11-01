# Pico SDK and Arduino-Pico integration

Status: Proposed for review

Release plan: initial Pico SDK/Arduino support, followed by a committed
Arduino-Pico FreeRTOS release.

## Objective

Run RP2040 Pico SDK applications and Arduino-Pico sketches as native Linux
executables in roo_testing, reusing its clock, transducers, and external-device
models while preserving existing ESP32 behavior.

## Motivation

A sketch that reads a DS3231 or draws to an SPI display needs substantially the
same simulated external hardware on a Pico as on an ESP32. Today roo_testing
can model those devices, but its public Arduino entry points select ESP32 code,
so a Pico application cannot use them with its actual framework identity.

Adding Pico support lets developers exercise application behavior and peripheral
interactions before uploading to a board. It also exposes accidental ESP-IDF
dependencies in libraries intended to work across microcontroller families.

## Background

roo_testing compiles application and framework C/C++ for the Linux host. It
does not execute microcontroller instructions. Its existing
[platform profiles](../roo_testing/platforms/README.md) distinguish frontend,
SoC, and ESP-IDF capability. Arduino and IDF frontends currently support ESP32,
ESP32-S3, and ESP32-C3.

Several current boundaries are narrower than their names suggest:

- The root [Arduino targets](../BUILD) always select Arduino-ESP32.
- The [Arduino environment guard](../roo_testing/frameworks/environment/BUILD)
  requires ESP-IDF, and its compile contract checks ESP32-specific macros.
- The [SoC contract](../roo_testing/soc/include/roo_testing/soc_profile.h)
  combines generic identity with mandatory ESP-IDF target identity.
- [FakeEsp32Board](../roo_testing/microcontrollers/esp32/fake_esp32.h) combines
  peripheral state, device attachments, and the ESP32 GPIO matrix.
- The [I2C model](../roo_testing/buses/i2c/fake_i2c.cpp) includes ESP-IDF SoC
  capabilities to allocate controller objects, although its device interface
  itself has no ESP-IDF types.
- [roo_testing/sys](../roo_testing/sys/BUILD) uses the ESP-IDF FreeRTOS port.
  The timer and GPIO model have separate host synchronization dependencies.

The [Pico SDK host implementation](https://github.com/raspberrypi/pico-sdk/blob/master/src/host/README.md)
supports native compilation through `PICO_PLATFORM=host` in CMake and allows
replacement simulation libraries. It is a source and portability reference;
it does not supply roo_testing device integration or Bazel configuration.

This proposal targets Earle Philhower's Arduino-Pico, rather than the separate
Arduino Mbed RP2040 core. Arduino-Pico
[exposes Pico SDK APIs directly](https://arduino-pico.readthedocs.io/en/stable/sdk.html).
Its default execution model and its optional
[FreeRTOS configuration](https://arduino-pico.readthedocs.io/en/stable/freertos.html)
are distinct configurations.

The existing [clock](emulated_time_clock.md),
[alarm service](emulated_time_alarms.md), and
[periodic voltage signals](periodic_voltage_signals.md) remain authoritative
for time progression and electrical signal representation.

## Requirements

1. Build ordinary C/C++ Pico SDK applications and Arduino-Pico sketches with
   truthful framework identity and no ESP-IDF headers, macros, or runtime.
2. Support the original non-wireless RP2040 Pico board first: console I/O,
   monotonic time and blocking delays, digital GPIO, synchronous UART, SPI,
   I2C, external ADC inputs, and PWM output.
3. Route Arduino and SDK accesses to the same peripheral state and external
   devices. Invalid pin functions and unsupported configurations must produce
   explicit failures rather than plausible successful results.
4. Use one emulated uptime for application reads, blocking operations, and
   timed device signals, including deterministic manual-time tests.
5. Preserve current public ESP32 targets, default profile selection, device
   attachments, and supported behavior across all six existing profiles.
6. Make profile mixing and accidental use of unavailable framework capabilities
   fail during configuration, compilation, or linking.
7. Deliver independently testable increments with a documented support matrix
   and examples as functionality becomes available.
8. Preserve an explicit distinction between the initial bare-metal runtime and
   the committed subsequent FreeRTOS runtime, so adding task support does not
   change existing Pico profile behavior or require replacing peripheral models.

### Out of scope for this release

RP2350/Pico 2, Pico W networking, Bluetooth, the Mbed core, core 1 execution,
FreeRTOS on Pico, SDK alarms and repeating timers, peripheral interrupts,
PIO, DMA, USB protocol emulation, flash programming, persistent filesystems,
and register-accurate or instruction-accurate behavior are excluded. USB-style
`Serial` is mapped to the host console; that does not emulate USB hardware.

FreeRTOS is a committed subsequent release, rather than an optional enhancement.
The first release establishes the compatibility boundaries below; its
single-threaded restriction describes emulator scope, not Arduino-Pico capability.

## Design Overview

The design adds a Pico backend beside the existing ESP32 backend. A backend is
the selected framework source set, native startup code, and hardware adapters
for one microcontroller family; it is fixed for the lifetime of a binary.

A Pico board model is one process-wide object holding RP2040 pin selections,
peripheral configuration, and borrowed device attachments. It has no ESP32 GPIO
matrix and no ESP-IDF state. A Pico adapter implements a supported SDK operation
by validating it and forwarding it to that board model and the existing bus or
signal models. Arduino-Pico runs above those same adapters.

```text
SDK application                 Arduino-Pico sketch
       |                         Arduino-Pico core/libraries
       +------------------------------+
                            |
                    Pico SDK adapters
                            |
                      FakePicoBoard
                            |
          shared GPIO, bus, signal, and device models
                            |
                   existing emulated clock
```

The framework/profile split satisfies requirements 1 and 6. The board and SDK
adapters satisfy requirements 2 and 3. Clock adapters satisfy requirement 4.
Keeping existing ESP32 implementations behind their current profiles, with
explicit compatibility tests, satisfies requirement 5. The staged rollout and
support matrix satisfy requirement 7. Explicit runtime identity and the
follow-up release contract satisfy requirement 8.

For example, an Arduino sketch configures `Wire` on GPIO 4 and 5, then reads
the DS3231 at address 0x68. Arduino-Pico calls the Pico I2C adapter, which finds
the controller's selected pins and the device attached to those pins. The
existing DS3231 model answers the read. A direct SDK transaction through that
controller observes the same configuration and device state.

## Design Details

### Profile and capability boundaries

Add `rp2040` to the SoC constraint and `pico_sdk` to the framework-capability
constraints. Add a `pico` frontend for SDK-only applications. Define two native
host platforms and matching rc profiles:

| Profile | Frontend | Capabilities | SoC |
| --- | --- | --- | --- |
| `roo_testing_pico_rp2040` | Pico SDK | Pico SDK | RP2040 |
| `roo_testing_arduino_rp2040` | Arduino | Pico SDK | RP2040 |

Both inherit the native host CPU/OS constraints. Neither has ESP-IDF capability.
Add `is_pico`, `is_pico_sdk`, and `is_rp2040` settings. Preserve the meanings of
`is_arduino`, `is_idf`, and `is_esp_idf`.

Represent runtime independently of frontend and SoC, using `bare_metal` and
`freertos` constraint values and matching compile-profile identity. The two
initial Pico profiles select `bare_metal`; existing ESP32 profiles select
`freertos`. This describes the emulated application runtime, not internal host
worker threads. Do not expose FreeRTOS headers merely because a build is Arduino
or defines `ROO_TESTING`. The future Arduino-Pico FreeRTOS profile will select
Arduino, RP2040, Pico SDK, and `freertos`, without ESP-IDF capability. It will
retain the same stable Arduino targets and device attachment API.

Split the generic Arduino guard from backend contracts, and split generic SoC
identity from ESP-IDF identity. Existing ESP-specific constants remain available
to ESP32 consumers; Pico profiles do not invent IDF architecture strings.
Use a `select()` at each stable Arduino target to choose the backend.

Store fragments under `.roo_testing/bazelrc/rp2040/{base,pico,arduino}.bazelrc`.
Extend wrapper conflict detection to recognize the Pico frontend and reject
mixed profiles. The interactive default remains Arduino ESP32. Replace the
six-profile helper's hard-coded product with an explicit list of eight valid
profiles; SDK-only ESP32 and IDF RP2040 are not valid combinations.

Compiler profiles define `ROO_TESTING`, generic RP2040 identity, and supported
framework macros globally. Use the SDK's host platform configuration and native
compiler architecture macros. Do not define `__arm__`, device register bases,
or `PICO_ON_DEVICE=1` to trick hardware-only sources into compiling. Arduino
identity uses `ARDUINO_ARCH_RP2040` and the selected core's Pico board macros.
The source-import phase records exact macro values from the pinned versions.

### Source ownership and upstream portability

Import a pinned Arduino-Pico revision and the exact Pico SDK revision it uses.
SDK-only and Arduino builds consume this single SDK copy. Follow the existing
[Copybara ownership model](../copybara/README.md): imported sources, explicit
portable patches, and roo_testing-owned host adapters and BUILD overlays.

Use SDK host implementations for portable components where they satisfy the
contract. Replace clock access and supported hardware operations with
roo_testing adapters. Do not link a second implementation of any SDK symbol.
Some SDK APIs are inline register accesses: route their host headers to host
implementations with signature-compatible overlays, and test header-only calls
as well as exported symbols. Broadly importing device headers is insufficient.

Compile portable Arduino core pieces such as strings, streams, and printing,
plus Wire/SPI and supported serial wrappers. Supply native startup and replace
USB, ARM assembly, boot ROM, newlib-specific startup, and direct hardware paths.
Preserve upstream interfaces and keep patches small. Unsupported libraries are
excluded from supported BUILD targets; transitive core startup requirements
receive explicit host replacements with documented behavior.

### Shared models and board state

Extract the controller registry and `soc_caps.h` dependency from the generic
I2C container. Keep legacy ESP32 lookup functions in an ESP32 adapter with the
same public behavior. Pico uses board-owned controller objects and resolves
attachments by active pin pair. This prevents an ESP32 capability header from
entering Pico's dependency graph.

Retain separate concrete ESP32 and Pico boards. Do not introduce a universal
board interface or move every ESP32 field into a generic base class. Share
existing bus/device types, extracting only code needed by both implementations.

The Pico model has fixed arrays for 30 GPIOs, two SPI controllers, two I2C
controllers, two UART controllers, four external ADC channels, and eight PWM
slices. Each controller retains configuration and enable state. Pin state
includes function, direction, pulls, output latch, and inversion settings for
supported APIs. Use checked pin-function tables for RP2040 routing.

Device attachments use small vectors of borrowed pointers and pin tuples,
registered before application startup. Duplicate registration of the same
device on the same connection is rejected. The caller keeps devices alive
until the application stops; no hot detach or runtime ownership transfer is
introduced. Separate test processes isolate board state.

For D attached devices and B bytes in a transfer, simple lookup costs O(D) and
device processing costs O(B) per selected device, excluding modeled delay.
For S selected SPI devices, broadcast delivery costs O(D + B*S). Expected D is
under ten; there is no reverse index or cache to maintain. Controller and pin
state costs O(1) per process, attachments O(D), and the SPI call retains O(S)
selected pointers. SPI uses fixed 256-byte chunks and three 256-byte scratch
arrays. These are Linux allocations, not a simulated RP2040 RAM budget. Verify actual structure sizes in the implementation rather than making
target-memory claims from host ABI sizes.

### Execution, lifecycle, and concurrency

The initial Pico runtime executes application code on one native thread.
SDK-only applications retain their ordinary `main()`; Arduino startup runs
initialization followed by `setup()` and repeated `loop()` calls. Test entry
points initialize supported runtime state before GoogleTest and shut down the
shared time service after tests finish. Board construction is lazy and contains
no hardware access, so supported clock reads from global constructors work.

Blocking console operations are allowed in auto mode; tests use injected UART
devices and bounded input. Neither initial Pico profile links the ESP-IDF
scheduler or `roo_testing/sys` FreeRTOS implementation. Calls launching core 1
fail explicitly.
Pico SDK mutex/critical-section support covers core-0, non-ISR use and does not
claim interrupt masking or multicore fidelity.

Existing host clock workers can still run internally. Shared clock and signal
state retains its current synchronization; Pico adapters do not dispatch
application interrupt callbacks from those workers. External mutation during a
transaction is outside the initial board API contract.

Coordinate roo_threads support as a small client change: select its existing
single-threaded implementation for the explicit bare-metal runtime before its
general `ROO_TESTING` FreeRTOS branch. Do not use `RP2040` or
`ARDUINO_ARCH_RP2040` alone to force single-threaded behavior: the later Pico
FreeRTOS profile must select the FreeRTOS implementation. Do not change ESP32
selection. Libraries requiring tasks or thread creation remain incompatible
with this initial Pico release.

Keep peripheral operations in their SDK adapters, separate from startup and
runtime wait/synchronization helpers. The first implementation supplies native
blocking helpers; the subsequent FreeRTOS implementation supplies scheduler-aware
helpers behind the same internal boundary. Do not add a dynamic runtime registry
or virtual scheduler API. A later FreeRTOS task must be able to block without
holding a board lock that prevents another runnable task from making progress.
The first release makes no task-safety claim; the follow-up release audits and
tests adapter serialization explicitly.

### Time and peripheral semantics

All Pico uptime APIs and Arduino `micros()`/`millis()` derive from
`system_time_get_micros()`. Blocking SDK sleeps and Arduino delays use the shared
delay operation; busy waits use the existing busy-wait contract. Thus a manual
sleep advances emulated time, whereas a passive read does not. Preserve SDK
absolute-time sentinel behavior and Arduino counter truncation. Check duration
conversions before passing unsigned SDK values to signed emulator deadlines.
SDK alarms and repeating timers are unavailable in this release; do not run
application callbacks with a misleading interrupt context.

| Surface | Initial modeled behavior |
| --- | --- |
| GPIO | Direction, function selection, digital reads/writes, pulls, and voltage signals; conflicts and invalid pins are rejected. |
| UART | Two controllers routed by legal pin selection to console/device models; blocking byte transfers and availability queries. No IRQ or DMA mode. |
| SPI | Two controllers; blocking read/write/full-duplex transfers, mode, baud, and supported word width; reuse existing transfer-time and chip-select signals. |
| I2C | Two controllers; synchronous 7-bit master transactions, repeated START, STOP, NACK, and timeout mapping; no slave mode. |
| ADC | Four external input channels, 12-bit conversion against the board reference voltage; no FIFO, DMA, or internal temperature sensor. |
| PWM | Eight slices with shared slice configuration and two outputs; retained periodic voltage signals for basic free-running PWM. No phase-correct mode or IRQs. |

Do not claim physical I2C waveform or clock-stretch timing. Report achieved
baud consistently with the selected adapter's documented divider calculation.
Chunk SPI transfers to fit the existing device model's 16-bit bit-count limit,
preserving chip select, word boundaries, and byte order. For I2C, reject requests
exceeding the model's 16-bit byte-length limit before calling the device; split
transactions would change device semantics. Round device timeouts to milliseconds
and saturate to the existing model's range, documenting the lost precision.

### Support and failure behavior

Maintain one checked-in API support matrix beside the Pico backend. It lists
supported operations and configurations, exclusions, and host substitutions.
Header presence alone is not a support claim.

Do not create catch-all successful stubs. Unsupported entry points that return
status use the SDK-defined failure result; invalid transfers leave buffers and
peripheral state unchanged. APIs with no safe error return emit
`LOG(FATAL) << "Unimplemented: <operation and configuration>"`. Harmless
initialization substitutions, such as omitting USB enumeration for console
Serial, emit `LOG(WARNING) << "Unimplemented: <details>"` once and document the
fallback. Unsupported libraries with no adapter fail to link or are marked
incompatible. This policy also applies between implementation stages.

## Proposed API

Existing `@roo_testing//:arduino`, `:arduino_main`, `:arduino_gtest_main`, and
`roo_arduino_example()` remain the Arduino-facing entry points. Add:

| Target | Purpose |
| --- | --- |
| `@roo_testing//:pico_sdk` | Headers and implementations of the supported SDK subset. |
| `@roo_testing//:pico_gtest_main` | SDK initialization, GoogleTest execution, and time-service cleanup. |
| `@roo_testing//roo_testing/microcontrollers/rp2040:pico` | Board model used by simulation fixtures. |

SDK applications link `:pico_sdk` and supply `main()`; no SDK application runner
is needed. The SDK targets require Pico SDK capability and reject ESP32 profiles.

The board fixture API follows the existing attachment style:

```cpp
// Proposed declarations; device types are the existing shared interfaces.
class FakePicoBoard {
 public:
  /// Attaches a borrowed I2C device to the specified SDA and SCL pins.
  void attachI2cDevice(FakeI2cDevice& device, int8_t sda, int8_t scl);

  /// Attaches a borrowed SPI device to the specified bus pins.
  void attachSpiDevice(SimpleFakeSpiDevice& device, int clk,
                       int miso, int mosi);

  /// Attaches a borrowed UART device to the specified TX and RX pins.
  void attachUartDevice(FakeUartDevice& device, int tx, int rx);

  // Fixed pin/controller arrays and attachment vectors described above.
};

/// Returns the process-wide emulated RP2040 board.
FakePicoBoard& FakePico();
```

Attachments validate physical pin ranges immediately; controller-function
compatibility is checked when the application configures or uses the bus.
Device chip-select and ADC voltage wiring use the existing shared GPIO and
voltage interfaces, exposed by the board consistently with `FakeEsp32Board`.
Full declarations and Doxygen contracts land with their implementations.

Example SDK consumer:

```starlark
cc_binary(
    name = "blink",
    srcs = ["blink.cpp"],
    deps = ["@roo_testing//:pico_sdk"],
)
```

```sh
bazel run :blink --config=roo_testing_pico_rp2040
bazel run :arduino_blink --config=roo_testing_arduino_rp2040
```

These commands require importing the corresponding vendored rc fragments in
the root workspace. Each target becomes usable only when its stage lands;
unimplemented operations follow the failure policy above.

## Implementation Plan

Follow the repository's
[C++ authoring guidance](../.github/instructions/general-cpp-code-authoring-instructions.md)
and [Copybara guidance](../copybara/README.md). Each stage below maps to one
commit and includes focused tests and documentation in that commit. Target
names for new tests are proposed; use the existing profile test conventions.

### 1. Separate generic framework and SoC identity

Split Arduino and SoC guards into generic and ESP-specific pieces without
adding a selectable Pico runtime. Add independent runtime constraints and compile
identity, with FreeRTOS selected by existing ESP32 profiles. Preserve existing
aliases and IDF constants.
Add tests proving generic identity does not imply IDF capability; update the
environment contract documentation.

Validation: existing profile contract tests under all six ESP32 profiles,
including contradictory-macro and missing-capability failures.

Proposed commit message:

> Pico integration stage 1: Separate generic profile identity from ESP-IDF.
>
> Split frontend and SoC contracts while preserving the six existing ESP32
> profiles, with guard tests and environment documentation for doc/pico_integration.md.

### 2. Import and prove a minimal native Pico SDK build

Implemented: pinned source pair, SDK-only profile, native C/C++ probes, and
isolation/failure validation. See the
[current support matrix](../roo_testing/frameworks/pico_support/README.md) for the
host-clock boundary that stage 3 will replace.

Pin matching upstream sources, add Copybara rules and BUILD overlays, and add
the SDK-only RP2040 profile with `:pico_sdk`. Build a stdio/uptime probe using
the SDK host platform and native compiler. Record exact macro/header selection
and patch ownership in the support matrix. No hardware success stubs are added.

Validation: compile and execute the probe; inspect its configured dependencies
and compile actions for ESP-IDF, device assembly, duplicate SDK symbols, and
contradictory target macros. Exit criterion: one working native SDK binary with
zero ESP-IDF dependencies. Resolve portability failures within this stage before
adding peripherals.

Proposed commit message:

> Pico integration stage 2: Add the pinned native Pico SDK backend.
>
> Import matching SDK sources, expose the RP2040 SDK profile, and validate a
> minimal host executable and dependency isolation for doc/pico_integration.md.

### 3. Unify time and SDK test lifecycle

Implemented: SDK time uses the shared emulated clock, `:pico_gtest_main` owns
test shutdown, and manual/auto examples and tests cover conversion, sentinels,
static initialization, opaque timestamps, and result propagation. The
[support matrix](../roo_testing/frameworks/pico_support/README.md) documents
finite clock limits and the manual busy-wait behavior.

Implement supported SDK time/sleep operations using the emulator clock and add
`:pico_gtest_main`. Add manual and auto-time examples, conversion/sentinel tests,
static-initialization tests, and clean process shutdown tests.

Validation: new Pico time and lifecycle tests in both time modes, plus the
existing shared clock and alarm suites to catch changes in shared behavior.

Proposed commit message:

> Pico integration stage 3: Connect Pico time and tests to the emulator clock.
>
> Add SDK uptime and delay adapters and a native test runner, with time-mode,
> conversion, and shutdown coverage for doc/pico_integration.md.

### 4. Add the board model and digital GPIO

Implemented: a process-wide 30-pin board, checked SDK digital GPIO, shared
voltage wiring, and bounded blink with signal/failure tests. The
[board reference](../roo_testing/microcontrollers/rp2040/README.md) specifies
initially supported SIO/NULL muxing, borrowed attachments, input/drive semantics,
and the remaining electrical/interrupt limitations.

Implement fixed RP2040 pin state, validated function selection, voltage wiring,
and supported GPIO APIs. Add the SDK blink example, signal tests, invalid-pin
tests, and explicit interrupt-mode rejection; extend the support matrix.

Validation: Pico GPIO tests and a bounded blink smoke executable, including
manual-time signal observations and unchanged ESP32 GPIO tests.

Proposed commit message:

> Pico integration stage 4: Add RP2040 board state and digital GPIO.
>
> Route validated Pico GPIO operations through shared voltage models and add
> blink and failure coverage for doc/pico_integration.md.

### 5. Add synchronous UART and console support

Implemented: two checked native UART instances, legal RP2040 TX/RX routes,
borrowed fake-device/cable wiring, partial and large blocking transfers, text
translation, and bounded shared-clock readiness waits. UART0 GP0/GP1 substitute
polling stdin/stdout unless explicitly attached. Pico reuses ESP32's shared
`ConsoleUartDevice`; host polling, lookahead, and byte I/O live in the shared UART
layer. Both directions use file descriptors; complete output bypasses stdio
buffering, preserving standalone `Serial.println()` visibility without a later
write or flush. Its input-closure query lets Pico reject EOF in blocking reads while
ESP32 nonblocking reads return zero. The
[runtime reference](../roo_testing/frameworks/pico_support/README.md#synchronous-uart-and-host-console-stage-5)
specifies console/EOF behavior, nominal-clock baud calculation, and unsupported
modes. The [bounded serial example](../examples/pico_uart/README.md) exchanges
bytes between both controllers. UART fixture arguments use `int` so invalid
physical pin values are checked before narrowing, with -1 for an unused direction.

Implement UART routing and blocking transfers using existing fake UART devices.
Add loopback/cable tests and a bounded serial example, including baud and invalid
function checks. Document host-console substitutions.

Validation: Pico UART device tests, shared console polling/binary/EOF and
concurrency tests, complete descriptor writes under backpressure, existing fake UART
regressions, and console pipe checks through both Pico and ESP32 board adapters. Shared tests also execute through
Arduino/IDF FreeRTOS runners.

Proposed commit message:

> Pico integration stage 5: Add Pico UART and host console adapters.
>
> Support synchronous byte I/O through legal RP2040 pin routes, with cable,
> console, and unsupported-mode tests for doc/pico_integration.md.

### 6. Add synchronous SPI and an existing display model

Implemented: two checked native SPI masters, modes 0–3, 8/16-bit MSB-first
transfers, software GPIO CS, bounded shared-device delivery, and cumulative
shared-clock transfer time. The destination-owned host header exposes the
supported SDK subset without device registers. The
[SPI runtime reference](../roo_testing/frameworks/pico_support/README.md#synchronous-spi-stage-6)
specifies routing, error preflight, framing, timing, and limitations. The
[headless ST77xx example](../examples/pico_spi/README.md) reuses the shared display
model and asserts captured commands, pixel bytes, and final rendered pixels.

Implement SPI configuration and blocking transfers with bounded chunks. Reuse
an existing display model in a Pico SDK example. Add mode, word-order,
chip-select, chunk-boundary, and transfer-duration tests.

Validation: Pico SPI tests and the display example build; assert captured
transactions headlessly so CI does not require a display server.

Proposed commit message:

> Pico integration stage 6: Connect Pico SPI to shared device models.
>
> Add blocking SPI adapters and a display example with transfer, pin-routing,
> and timing coverage for doc/pico_integration.md.

### 7. Decouple I2C and add Pico transactions

Implemented: legacy lookup/C entry points live in the ESP32 backend while the
shared I2C target is framework-independent. Pico resolves legal SDA/SCL routes
onto existing devices, preserves STOP/repeated-START intent, maps errors/timeouts,
and rejects oversized transactions. The DS3231 example and tests share the same
model as ESP32. See the [I2C runtime contract](../roo_testing/frameworks/pico_support/README.md#i2c-stage-7).

Move legacy controller lookup into the ESP32 backend, preserving callers.
Implement Pico controller routing and I2C adapters. Add a DS3231 example and
tests for repeated START, errors, timeout mapping, and oversize rejection.

Validation: existing legacy/Arduino/IDF I2C tests and new Pico DS3231 tests;
verify the generic I2C target no longer depends on ESP-IDF headers.

Proposed commit message:

> Pico integration stage 7: Share I2C device models with the Pico backend.
>
> Isolate ESP32 controller lookup and add Pico master transactions and a DS3231
> example with bus-contract coverage for doc/pico_integration.md.

### 8. Add external ADC and basic PWM

Implemented: four external ADC channels sample the existing voltage sources;
eight PWM slices publish shared periodic VoltageSignal values with A/B aliases,
compare/inversion, checked dividers, and microsecond-resolution counter state.
The [signal example](../examples/pico_signal/README.md) connects a 1.65 V source
to a 50% duty output. The runtime matrix records conversion rounding, immediate
PWM publication, and unsupported FIFO/IRQ/phase modes.

Implement external ADC conversion and free-running PWM slice state. Document
reference voltage and rounding; test shared slice settings, channel outputs,
pin routing, boundary voltages, and unsupported modes. Add a small signal example.

Validation: Pico ADC/PWM tests against time-indexed voltage observations and
the existing shared signal tests.

Proposed commit message:

> Pico integration stage 8: Add Pico external ADC and PWM signals.
>
> Model bounded ADC conversion and shared PWM slices with signal examples and
> boundary coverage for doc/pico_integration.md.

### 9. Integrate the Arduino-Pico core and public Arduino targets

Implemented: the native Arduino profile selects the original Pico variant and
stable public Arduino targets. The pinned String, Print, Stream, Common, WMath,
CoreMutex, non-ISO conversion, and analog implementations run on shared SDK
adapters. Hardware-specific serial, SPI and Wire leaves reuse existing fake
buses, console and device models. The bounded peripheral sketch covers blink,
UART, DS3231 and SPI, with mixed Arduino/SDK regression tests. The exact API
submodule is imported independently by Copybara; native boundary edits and
Stream timeout progress are replayable patches. See the
[Arduino runtime reference](../roo_testing/frameworks/arduino_pico_support/README.md).

Compile the selected core subset against the working SDK adapters, add native
Arduino startup/test runners, and expose the Arduino RP2040 profile. Update
stable Arduino targets and the sketch helper through profile selection. Add
Arduino blink, UART, DS3231, and SPI examples/tests, including mixed Arduino/SDK
access. Record every excluded core feature in the support matrix.

Validation: Arduino Pico example builds and focused runtime tests; all existing
ESP32 Arduino startup and profile tests. Check no ESP-IDF dependency is selected.

Proposed commit message:

> Pico integration stage 9: Run Arduino-Pico through the native Pico backend.
>
> Select Arduino-Pico through stable public targets and add startup and shared
> peripheral examples and tests for doc/pico_integration.md.

### 10. Align Roo threading selection in the client repository

In a separate roo_threads commit, follow that repository's instructions and
select its existing single-threaded implementation for the explicit bare-metal
emulation runtime before its general `ROO_TESTING` FreeRTOS branch. Avoid a
chip-wide Pico override. Preserve ESP32 selection and document
thread-dependent client exclusions. Use local overrides until the coordinated
client release is available.

Validation: roo_threads backend-selection tests for bare-metal Pico and ESP32,
plus a compile-only FreeRTOS-runtime selection probe proving RP2040 identity does
not force the single-threaded backend. The probe does not claim a working Pico
FreeRTOS runtime. Build representative single-threaded Roo consumers without
an ESP-IDF dependency.

Proposed commit message:

> Pico integration stage 10: Select single-threaded Roo support for emulated Pico.
>
> Resolve bare-metal emulation before the generic FreeRTOS branch and preserve
> ESP32 selection, following roo_testing/doc/pico_integration.md.

### 11. Complete the profile matrix and supported client validation

Add the explicit eight-profile helper list, wrapper conflict tests, and CI
coverage. Validate roo_time, roo_scheduler, and supported roo_display Arduino
consumers using local overrides. Document thread-dependent client exclusions
rather than making every client compatible. Add negative coverage for
unsupported core 1, FreeRTOS, IRQ, and alarm paths. Update README setup
instructions and mark this design implemented only after the full support
matrix is verified.

Validation: all eight profile suites, supported client smoke tests, ASAN, wrapper
tests, and dependency-isolation checks. Serialize builds and tests; preserve
the global resource limits and disk cache, and keep every Bazel storage path on
persistent storage under `/home/dawidk`, never `/tmp`.

Proposed commit message:

> Pico integration stage 11: Validate and document the eight-profile release.
>
> Extend wrapper and CI coverage to Pico, record supported Roo clients and
> exclusions, and finish the support matrix for doc/pico_integration.md.

## Testing Plan

Use SDK and Arduino contract tests, device-level integration tests, profile
purity checks, and existing ESP32 regressions. Each feature stage owns its cases;
the release stage checks composition and supported clients. Run clock-dependent
tests in separate manual/auto binaries and board-dependent tests in isolated
processes. Run ASAN using the existing common fragment.

Compile representative consumers for real RP2040 hardware as well as the host
to catch signature and macro drift. Perform a board smoke run for blink, UART,
DS3231, and SPI display behavior before release. Hardware availability is a
release validation dependency; host tests cannot establish electrical or ABI
accuracy. These checks validate this design's API subset, not all Pico SDK APIs.

## Caveats

Native compilation retains the Linux ABI, heap, stack, and execution speed.
Passing tests do not prove 32-bit layout, memory fit, real-time performance,
electrical behavior, or interrupt/multicore correctness. Synchronous I2C and
chunked SPI retain the existing device-model limitations.

Upstream SDK and Arduino headers evolve together. Update their pinned versions
as one unit, replay small patches, and rerun profile and real-target compilation
checks. The initial source integration and Arduino startup are the largest
portability risks; stage 2 establishes the SDK foundation before device work,
and stage 9 has explicit core integration acceptance tests.

The current roo_time emulator branch bypasses its real Pico time implementation.
Emulation tests therefore validate the shared clock integration; retain direct
Pico backend unit/compile tests to cover the actual hardware code path.

### Rejected Alternatives

#### Represent Pico as another ESP32 identity

This reuses startup cheaply but selects incorrect headers, capabilities, and
library branches. The capability split in Design Details is necessary to meet
truthful framework identity and dependency isolation.

#### Write a separate Arduino-shaped mock framework

This produces blink quickly but duplicates strings, streams, and peripheral
wrappers and misses direct SDK calls. The source-ownership design retains actual
Arduino-Pico interfaces above shared adapters.

#### Use the SDK host platform without roo_testing adapters

Upstream host support is a useful portability base, but it does not connect
existing DS3231/display models or guarantee a single emulated clock. The time
and peripheral adapters provide those contracts.

#### Reuse ESP-IDF FreeRTOS for the initial Pico runtime

This preserves current Roo threading choices but misrepresents Arduino-Pico's
default runtime and violates ESP-IDF isolation. The initial execution model is
single-core native code; the committed FreeRTOS release uses a separate profile
and a host port without ESP-IDF. This rejects coupling Pico to ESP-IDF, not
sharing portable FreeRTOS host-port code where its contracts match.

#### Introduce a universal virtual microcontroller interface

This centralizes board access but forces unrelated ESP32 matrix and RP2040
function-routing semantics into a larger abstraction. Separate concrete boards
and shared bus/device interfaces meet the demonstrated needs with less migration.

## Future Work

### Committed subsequent release: Arduino-Pico FreeRTOS

Add `roo_testing_arduino_rp2040_freertos` as a separate profile, retaining the
initial profiles and their behavior. Compile Earle's FreeRTOS-enabled core with
a pinned compatible kernel and a native host port, without ESP-IDF dependencies.
Reuse the Pico board, SDK adapters, and public Arduino targets. Select the
FreeRTOS roo_threads backend through runtime identity.

That release requires its own implementation design for scheduling, task-aware
waits, synchronization, interrupt delivery, and shutdown. Reusing the existing
ESP32 host-port implementation requires first separating any portable kernel/port
code from IDF configuration; the Pico profile must not inherit IDF capability.
Do not substitute unrelated native threads for FreeRTOS task semantics.

Release acceptance includes task creation/deletion, priorities and preemption,
blocking delays, queues, mutexes, semaphores, supported task notifications,
Arduino setup/loop execution, roo_threads integration, and concurrent use of the
supported peripheral adapters. Validate lifecycle cleanup, auto/manual-time
behavior, ASAN, and regressions for the initial Pico and existing ESP32 profiles.
Document supported scheduler width explicitly; FreeRTOS API support does not by
itself claim faithful two-core SMP emulation. Full core-1/SMP fidelity remains a
separate extension.

### Other extensions

- Define callback/interrupt execution semantics before adding SDK alarms, GPIO
  interrupts, or asynchronous peripherals.
- Add core 1, FIFO and lockout semantics as a distinct concurrency project.
- Add RP2350 and board-specific pin maps without equating chip and board identity.
- Add Pico W networking above host networking with explicit radio limitations.
- Add filesystem, DMA, PIO, and USB support only with dedicated support contracts.
