# Native Pico SDK runtime

Pico integration stages 2 through 7 provide the SDK-only profile
`--config=roo_testing_pico_rp2040` and the public `@roo_testing//:pico_sdk` target.
Applications supply ordinary `main()` and execute with the Linux host ABI/CPU.
Arduino-Pico sources are staged for a later release stage; no Arduino RP2040
profile or core implementation is exposed yet.

## Pinned sources and ownership

| Source | Version | Immutable revision |
| --- | --- | --- |
| Arduino-Pico | 6.3.1 | `53f1d8ee31a9565a15731c5aad8a6297bdf70144` |
| Pico SDK | 2.3.1 | `079c6f39023649b154152db30f1d781e884879bc` |

The SDK revision is the `pico-sdk` gitlink in that Arduino-Pico commit. Both
SDK-only and later Arduino builds use this one SDK copy. Imports use the
[Copybara workflows](../../../copybara/README.md) and
[version pins](../../../copybara/versions.bara.sky). Pristine imported files
retain upstream formatting and licenses. BUILD files and
`roo_testing_host_include` are destination-owned. The generated version header
is the pinned upstream template expanded to 2.3.1; `config_autogen.h` supplies the
original Pico board header without switching execution to the device platform.

The [patch series](../../../copybara/patches/pico-sdk/series) retains the stage-2
host timer fixes and corrects `delayed_by_us` saturation before unsigned
addition can wrap. The upstream host timer implementation is now excluded from
the link. SDK time entry points are owned by [time.cpp](time.cpp), and the [GPIO adapter](gpio.cpp) supplies checked digital I/O. No upstream
GPIO/UART/USB or clock-configuration success stubs are compiled.

## Profile and headers

Import these files into the root workspace's `.bazelrc`:

```text
import %workspace%/.roo_testing/bazelrc/common/asan.bazelrc
import %workspace%/.roo_testing/bazelrc/rp2040/base.bazelrc
import %workspace%/.roo_testing/bazelrc/rp2040/pico.bazelrc
```

The profile selects the native host platform plus RP2040 SoC, Pico SDK
capability, SDK-only frontend, and bare-metal runtime. Global compile identity is
`ROO_TESTING=1`, `ROO_TESTING_SOC="rp2040"`, `ROO_TESTING_SOC_RP2040=1`,
`ROO_TESTING_RUNTIME_BARE_METAL=1`, `PICO_BUILD=1`, `PICO_NO_HARDWARE=1`,
`PICO_ON_DEVICE=0`, `PICO_RP2040=1`, `PICO_RP2350=0`,
`PICO_PLATFORM_STRING="host"`, and `PICO_BOARD="pico"`.
No Arduino, ESP-IDF, ARM-compiler, or RISC-V-compiler identity is injected.
Physical RP2040 identity does not change the native ABI.

Exposed include roots are `src/common/pico_base_headers/include`,
`src/common/pico_time/include`, `src/host/pico_platform/include`,
`src/host/hardware_timer/include`, `src/boards/include`, and
`roo_testing_host_include`. Host `hardware_gpio/include` and
`hardware_irq/include` are additionally exposed for supported GPIO declarations
and IRQ parameter types. Host `hardware_uart/include` exposes the synchronous
UART adapter. The destination-owned `roo_testing_host_include/hardware/spi.h`
exposes the supported native SPI declarations; the pinned SDK has no host SPI
module. It matches the selected API signatures and enums in the pinned
`rp2_common/hardware_spi` header without exporting its register dependencies.
Device register include roots remain excluded. Only `src/host/pico_platform/platform_base.c` is compiled from the
upstream SDK. The [time adapter](time.cpp) and existing shared
[clock implementation](../../system/timer.cpp) supply time operations. There is
one linked implementation of each supported SDK entry point.

## Support matrix after stage 7

| API or facility | Behavior |
| --- | --- |
| `pico.h`, SDK types/version, `roo_testing/soc_profile.h` | Native host definitions with typed RP2040 identity. |
| C stdio (`printf`, etc.) | Ordinary libc stdin/stdout; no USB or UART hardware initialization. |
| `time_us_64`, `time_us_32`, `get_absolute_time`, `time_reached` | Passive reads/comparisons of the shared emulated uptime; the narrow counter truncates. |
| `nil_time`, `at_the_end_of_time`, inline timestamp arithmetic | SDK sentinel values and conversions, including saturation before unsigned addition overflow. |
| `sleep_us`, `sleep_ms`, `sleep_until` | Shared emulated-time delays; manual mode advances deterministically, auto mode uses shared host pacing. |
| `busy_wait_us_32`, `busy_wait_us`, `busy_wait_ms` | Host-monotonic busy waits without explicit emulated-time publication or callback dispatch. |
| `busy_wait_until` | Auto mode rechecks shared uptime until the target; manual mode consumes the sampled remaining host duration without changing manual time. |
| `best_effort_wfe_or_timeout` | Sleeps until the timeout and returns true; processor events are not modeled. |
| `get_core_num` | Always 0 for this single-threaded application runtime. |
| Hardware alarm allocation | Optional allocation returns -1; required allocation/claim/unclaim fails explicitly. |
| Hardware alarm callbacks/targets | `LOG(FATAL)` with the unsupported operation; SDK interrupt context is not emulated. |
| SDK alarm pools, repeating timers | Unavailable implementations fail to link. |
| Digital GPIO, SIO latches/direction, pulls, input enable and overrides | Checked GP0–GP29 state, shared voltage sources/sinks, and low-bank mask operations. |
| Function mux | SIO, NULL, legal UART TX/RX, and SPI SCK/TX/RX; other peripheral functions fail explicitly. |
| GPIO interrupts and unsupported analog pad controls | Explicit fatal failure, including IRQ-disable requests and two-pull bus keeper mode. |
| Synchronous UART | Two checked controllers, 8N1 byte transfers, routing, readiness, CRLF helpers, and UART0 console. |
| Synchronous SPI | Two masters, modes 0–3, 8/16-bit MSB-first frames, software GPIO CS, bounded shared-device transfers, and transfer time. |
| I2C | Two seven-bit masters, shared devices, STOP/repeated START, bounded transfers, timeout/error mapping. |
| `pico/stdlib.h`, ADC/PWM, core 1, USB, filesystem, FreeRTOS | Not exported through this stage's include/link surface. |

See the [RP2040 board reference](../../microcontrollers/rp2040/README.md) for
fixture wiring, raw latch versus pad semantics, reset/init behavior, mask
validation, floating inputs, and retained sink history. GPIO uses the same
emulated uptime as the time APIs below.

### Shared clock behavior and limits

The [clock contract](../../../doc/emulated_time_clock.md) and
[shared alarm contract](../../../doc/emulated_time_alarms.md) apply to Pico too.
Time mode is fixed by the executable's link dependencies before global
constructors. The default follows host elapsed time; depending on
`//roo_testing/system:manual_time_mode` selects manual progression. SDK reads do
not dispatch work. SDK sleeps use the shared chronological alarm pump, so
emulated device callbacks agree with application timestamps. Those callbacks
are native shared-service callbacks, not Pico SDK interrupt handlers.

Zero sleeps and nil/past deadlines return without advancing or dispatching.
Milliseconds are widened before multiplication. Shared uptime is signed
nanoseconds: finite microsecond deadlines and durations are checked against
`INT64_MAX / 1000`, and the shared service additionally checks addition against
the current precise uptime. Invalid waits fail before publication. The SDK
end-of-time marker remains `INT64_MAX` microseconds and is never reached by the
shared clock; waiting until it fails explicitly instead of wrapping or
pretending to complete. The marker remains valid for comparisons and saturated
timeout arithmetic.

Busy waits obey the shared host-busy-wait contract. In manual mode they do not
make `time_reached(future_deadline)` become true; the driver still controls
progression. In auto mode subsequent reads observe elapsed host time. Host
busy-wait conversions and host deadline addition are also range checked. This
API subset does not promise hardware power behavior or cycle-accurate timing.

### Native test runner and shutdown

Use `@roo_testing//:pico_gtest_main` for GoogleTest tests using this SDK backend.
It runs tests on the native main thread, initializes the already link-selected
clock without resetting uptime, and returns the GoogleTest result. On both
success and failure it waits for active shared delivery to finish, closes the
alarm service, joins its worker, and destroys pending callback captures before
returning. Manual mode uses the same cleanup without starting a worker.

The runner owns process shutdown; tests must not close the shared service
independently. SDK applications retain ordinary `main()`. Applications that
register shared emulated-device alarms own the corresponding two-part shutdown
from `roo_testing/system/timer.h`; the clock-only examples start no worker.

```starlark
cc_test(
    name = "my_pico_test",
    srcs = ["my_pico_test.cpp"],
    deps = ["@roo_testing//:pico_gtest_main"],
)
```

For deterministic tests add
`@roo_testing//roo_testing/system:manual_time_mode` to the executable's deps.
Do not select time mode per source or switch it at runtime. Optional
`PICO_OPAQUE_ABSOLUTE_TIME_T=1` must be a global compiler option for the complete
build so SDK function declarations and implementations share the same ABI.

Host SDK headers contain additional inline primitives; their presence is not a
claim of interrupt, synchronization, or hardware fidelity. Only the operations
listed above are supported. Arduino-Pico's default and optional FreeRTOS modes
remain later stages of the [integration design](../../../doc/pico_integration.md).

## Running and checking

```sh
bazel run //test/pico:sdk_probe_c --config=roo_testing_pico_rp2040
bazel test //:pico_tests --config=roo_testing_pico_rp2040
python3 test/pico/verify_pico.py
bazel run //examples/pico_time:manual_time --config=roo_testing_pico_rp2040
bazel run //examples/pico_time:auto_time --config=roo_testing_pico_rp2040
bazel run //examples/pico_gpio:blink_example --config=roo_testing_pico_rp2040
```

The verifier checks C/C++ execution, profile failures, dependency/action
isolation, one implementation per SDK symbol, native ELF identity, unsupported
alarm failure, GPIO/voltage behavior and bounded blink, manual/auto clock
behavior, opaque timestamps, runner result
propagation and post-main cleanup, and ASAN composition. It uses the existing persistent validation
output root and preserves the global Bazel cache and resource limits.

## Synchronous UART and host console (stage 5)

`hardware/uart.h` exports checked, distinct `uart0`/`uart1` handles and native
adapters for initialization, baud selection, 8N1 format, blocking byte transfers,
read/write readiness, bounded readability waits, and text helpers. The imported
host UART implementation is excluded; its null identities and terminal setup do
not participate. A Copybara patch corrects `UART_FUNCSEL_NUM` and supplies the
matching supported SDK declarations missing from the host header.

Attach an existing `FakeUartDevice` or `FakeUartCable` endpoint with
`FakePico().attachUartDevice(device, tx, rx)` before configuring its GPIOs. Use
physical GP numbers and -1 for an unused direction. Each TX/RX pair must belong
to the same RP2040 controller. Mux each used pin to `GPIO_FUNC_UART`; the adapter
resolves current mux state on each operation. Exactly one selected physical pin
is required per requested direction; missing wiring, ambiguous routes, and RX
input disable/overrides fail. CTS/RTS muxes and voltage observation of UART pins
are unsupported. See the [board routing contract](../../microcontrollers/rp2040/README.md)
and [bounded cable example](../../../examples/pico_uart/README.md).

UART0 TX GP0 and RX GP1 default to host stdout/stdin, independently per direction.
An explicit attachment replaces that direction's console route. UART1 never
implicitly uses the console. Both Pico and ESP32 use the shared
[`ConsoleUartDevice`](../../buses/uart/README.md). It polls without waiting,
leaves unread destination bytes intact, and reports EOF through `inputClosed()`.
The Pico blocking-read adapter fails explicitly on EOF; ESP32's nonblocking
read adapter returns zero. Both directions use descriptor I/O; output completes
partial writes and bypasses stdio buffering, preserving immediate visibility. No terminal mode/flags are changed, and unread bytes are never
zero-filled. The console assumes exclusive stdin consumption; do not mix UART
input with libc/iostream reads. Libc `printf` remains ordinary host stdio, separate
from UART configuration and CRLF conversion.

The default text mode is untranslated, matching the pinned SDK. Optional
`uart_set_translate_crlf()` converts LF in `uart_putc()` and avoids duplicate CR
in `uart_puts()`; raw and block writes remain binary. Init resets this setting.
Baud selection returns the pinned SDK's divisor-rounded result using a nominal
125 MHz peripheral clock (115200 requests return 115207). Clock reconfiguration,
byte pacing, framing, hardware FIFOs, register access, DMA, breaks, flow control,
and UART IRQs are unsupported. Non-8N1 formats and unsupported controls fail
explicitly, including IRQ-disable and break-clear requests. Disabling a UART
retains pin muxes and external device queues.

Blocking transfers retry short or zero-progress device operations and support
lengths greater than 65535 by chunking. A wait yields host time to external
producers without advancing manual emulated time. An unbounded read therefore
requires a producer; it does not pump manual alarms on its own.
`uart_is_readable_within_us()` has an explicit timeout and uses shared SDK sleeps,
so manual time progresses and shared device alarms can supply data. Writes
complete when accepted by the device; `uart_tx_wait_blocking()` confirms that
synchronous boundary without simulating a physical shift register.

Shared console unit tests run natively and through the Arduino/IDF FreeRTOS
runners. `test/uart_console_test.py` checks private stdin/stdout pipes through
both board adapters; the Pico/profile verifiers invoke it. The Pico verifier
also checks console behavior under ASAN. `//:pico_tests` includes byte
routing/cable regressions and the bounded serial example. No Arduino-Pico or FreeRTOS startup is introduced.

## Synchronous SPI (stage 6)

`hardware/spi.h` provides checked, distinct `spi0` and `spi1` handles (the original
Pico defaults to `spi0`), initialization/deinitialization, baud selection/query,
format selection, busy observation, and all six blocking 8/16-bit transfer APIs.
This is a destination-owned host header; no imported device implementation or
register layout is compiled. See the pinned
[SDK header](https://github.com/raspberrypi/pico-sdk/blob/079c6f39023649b154152db30f1d781e884879bc/src/rp2_common/hardware_spi/include/hardware/spi.h)
and [divider algorithm](https://github.com/raspberrypi/pico-sdk/blob/079c6f39023649b154152db30f1d781e884879bc/src/rp2_common/hardware_spi/spi.c).

Initialization selects an enabled, mode-0, 8-bit master. CPOL/CPHA modes 0–3 and
8/16-bit frames are supported. MSB-first is the only supported order, matching
the PL022; 16-bit native words are packed high byte first, independently of host
endianness. The 8-bit APIs require 8-bit configuration; the 16-bit APIs require
16-bit configuration. Other frame widths, LSB-first, slave mode, and FIFO-status
queries fail explicitly. DMA, IRQs, hardware CSn, register access, and physical
SCK/MOSI/MISO voltage waveforms are unavailable. `spi_set_slave(false)` retains
master operation; `spi_is_busy()` observes device callback execution.

Baud follows the pinned SDK's even prescale (2–254) and postdivide (1–256) search
at nominal 125 MHz `clk_peri`. Requests of 1 MHz achieve 992063 Hz; 125 MHz
requests achieve the maximum 62.5 MHz SCK. Invalid/too-low requests fail before
configuration changes. Dynamic clock configuration is not modeled.

Attach `SimpleFakeSpiDevice` instances through `FakePico().attachSpiDevice()`
before configuring pins. The shared interface supplies achieved SCK, mode, and
MSB-first order. The [board reference](../../microcontrollers/rp2040/README.md#spi-device-routing)
specifies legal routes and software CS wiring. Each call samples selected devices
once, holds that selection across chunks, and supplies each with an independent
copy of the original outgoing bytes. Multiple selected devices can receive a
write; a receive with multiple selected MISO responders fails before any delivery.
With no selected responder, reads sample the configured MISO pull. An undriven
MISO with no pull or unsupported input override fails explicitly.

Transfers use fixed 256-byte chunks (128 whole frames in 16-bit mode) and three
256-byte scratch arrays. Frame boundaries and software CS remain intact; lengths
larger than the shared model's 16-bit bit-count capacity are supported by chunking.
Return values count frames, not bytes. Identical source/destination is supported;
partial overlap is rejected. Read-only calls clock their repeated dummy value;
write-only calls discard incoming bytes. Zero-length calls on an enabled,
matching-format controller need no pin route or non-null buffer.

Each chunk advances the shared emulated clock with `system_time_lag_ns()` and
paces auto mode using `system_time_sync()`, reusing the ESP32 SPI timing boundary.
Duration uses total bits / achieved baud, rounded up to nanoseconds cumulatively
within the call, avoiding extra delay at chunk boundaries. Bus time is counted
once regardless of the number of selected devices. These calls do not pump
manual alarms or emulate SPI interrupts. Device callbacks observe the start of
their chunk; byte-level edges are not modeled. Active configuration and CS must
remain stable; GPIO/SPI reconfiguration from device callbacks fails, and nested
SPI transfers return an in-use error. The initial runtime is single-threaded.

Blocking transfer failures return the following SDK error codes before changing
buffers, device state, or time:

| Error | Condition |
| --- | --- |
| `PICO_ERROR_INVALID_ARG` | Null required buffer, length above `INT_MAX`, API/frame mismatch, partial overlap, or duration outside the shared clock range. |
| `PICO_ERROR_INVALID_STATE` | Disabled controller, missing/ambiguous pin routes, absent bus wiring, unsupported RX pad state, or floating unselected MISO. |
| `PICO_ERROR_RESOURCE_IN_USE` | Multiple selected MISO responders or a nested SPI transaction. |

Invalid opaque handles and unsupported/invalid void configuration APIs fail
fatally. Timing preflight reserves the final 999 ns of the shared clock as a
conservative margin because its public uptime query is in microseconds.

The [headless ST77xx example](../../../examples/pico_spi/README.md) uses the
existing display model, validates captured commands and 512 RGB565 pixel bytes,
and asserts every rendered pixel. No display server, FLTK, or SDL dependency is
needed. The Pico suite tests routing, modes, word order, selection, frame/chunk
boundaries, timing, overflow, invalid inputs, and reentrancy in normal/ASAN builds.

## I2C (stage 7)

The owned `hardware/i2c.h` overlay exposes checked `i2c0`/`i2c1`, init/deinit,
clock selection, and blocking/relative-timeout/deadline/per-character calls.
Devices use the existing `FakeI2cDevice` protocol and DS3231 model. The shared
bus library no longer includes ESP-IDF; the ESP32 backend retains legacy
`getI2cInterface()` and C entry points for existing callers.

`attachI2cDevice(device, sda, scl)` borrows devices before pin configuration.
SDA is an even GP number; SCL is odd; `(pin / 2) % 2` selects controller 0/1.
Nonadjacent pins belonging to the same controller are allowed. Shared pins with
different addresses are allowed; duplicates, conflicting devices, and late
wiring fail. Select GPIO_FUNC_I2C on exactly one SDA and SCL per controller.
Unsupported pad overrides, disabled input, absent or ambiguous routes fail
before delivery. Peripheral pin voltage waveforms and SIO driving reserved pins
are unsupported, as with SPI/UART. Application configuration is single-threaded;
device callbacks cannot reconfigure active buses or GPIOs.

Requests support addresses 0x08–0x77 and lengths 1–65535. Oversized transactions
return PICO_ERROR_INVALID_ARG without splitting or device calls. `nostop` maps
to `sendStop = false`; successful calls retain repeated-START intent for the
next call. Init/deinit and device errors clear that intent. Device reads are
staged and copied to the caller only on success. Models may retain their own
state changes on device-reported failure; invalid preflight requests cause none.

Timeout microseconds round up to model milliseconds, saturated at 65535 ms.
Unbounded calls use that maximum. Expired/zero timeouts return PICO_ERROR_TIMEOUT
without delivery. Per-character variants forward a saturated whole-call budget;
there is no per-byte device callback or physical bus/clock-stretch timing.
Baud uses the pinned SDK period rounding at nominal 125 MHz, limited here to
1 MHz master operation. Slave, FIFO, DMA, IRQ, and register APIs are unsupported.

Success returns the byte count. Device NACK/missing-address/bus failures map to
PICO_ERROR_GENERIC, device timeout to PICO_ERROR_TIMEOUT, busy to
PICO_ERROR_RESOURCE_IN_USE, and allocation failure to
PICO_ERROR_INSUFFICIENT_RESOURCES. Bad arguments and inactive/missing muxes use
PICO_ERROR_INVALID_ARG / PICO_ERROR_INVALID_STATE respectively. Configuration
errors and invalid opaque handles fail explicitly. Calls do not invent bus
latency or advance the shared clock. See [the bounded RTC example](../../../examples/pico_i2c/README.md).
