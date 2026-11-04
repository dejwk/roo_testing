# Native Arduino-Pico runtime

Select `--config=roo_testing_arduino_rp2040` and depend on the stable
`@roo_testing//:arduino`, `:arduino_main`, or `:arduino_gtest_main` target.
The public `roo_arduino_example` helper selects this main automatically.
Applications use the real pinned Arduino-Pico 6.3.1 public headers and original
`rpipico` variant. The independently pinned ArduinoCore-API gitlink supplies
the forwarding headers' implementation. This original profile is bare metal on core 0. The separate
`roo_testing_arduino_rp2040_freertos` profile uses the same public targets
with task-backed startup and no ESP-IDF dependency.

The upstream Common, String, Print, Stream, WMath, CoreMutex, non-ISO conversion
and analog implementations are compiled. Small recorded Copybara patches
exclude hardware-only umbrella includes, correct value-based chip detection,
and make failed Stream reads advance the shared clock through `delay(1)`.
The native GPIO/time/serial/SPI/Wire leaves call the same SDK adapters as native
SDK applications. Analog scaling and serial software queues are upstream code.
The SDK clock remains fixed at 125 MHz; native libc supplies integer conversion
functions absent from glibc. Linux ABI limitations still apply.

| API | Native behavior |
| --- | --- |
| GPIO/time | Basic input, pulls, output and shared clock; `delay` pumps alarms. Drive-strength variants beyond default, open drain, IRQs and pulse measurement fail. |
| `Serial` | Host stdin/stdout through the existing POSIX console. Warns once about USB substitution. Writes complete immediately without stdio buffering. Nonblocking read/peek; blocking Stream reads observe timeouts. USB descriptors, DTR/RTS and USB flow control fail. |
| `Serial1/2` | SDK UART0/1, 8N1, polling the upstream software queue. Shared cable/device models; UART0 defaults to the host console. Pin routes are checked. IRQ mode, inversion, breaks and flow control are unsupported. |
| `SPI/SPI1` | Synchronous transfers, manual GPIO CS, four modes, MSB or software-reversed LSB. Bounded transfer storage; borrowed const TX data is preserved. Hardware CS, DMA, asynchronous transfers and IRQ masking fail. |
| `Wire/Wire1` | Synchronous seven-bit master, configurable software buffer, repeated START, timeout flag and existing I2C device models. Address-only probes model ACK/NACK from device routing and always finish with STOP, without payload delivery. Slave, IRQ and DMA operations fail. |
| Analog | Upstream resolution, range and frequency scaling over four external ADC channels and shared PWM slices. Internal temperature, FIFO, phase-correct PWM and IRQs fail. |
| Core mutex | Checked core-0 ownership for the upstream portable core. No host-thread exclusion or ISR synchronization is claimed. |

Wire follows the pinned core's shared TX/RX buffer rules: oversized or zero-length
reads return zero without changing buffered bytes; `flush()` does nothing.
Repeated `begin()` and nested `beginTransmission()` calls preserve active state.
Probe timing models address clocks rather than GPIO edges or clock stretching;
attached devices acknowledge their configured address.

Networking (including the fork's lwIP-dependent IPAddress/UDP), filesystem,
USB hardware, PIO, semihosting, direct register access, core 1 are
excluded. Their headers are not replaced with success stubs. SDK unsupported
operations retain their explicit failures. The FreeRTOS variant is described below.
Do not mix independent readers of stdin: Serial and the default SDK UART0
share one console, not two independent byte streams.

Native startup calls init, initVariant, starts Serial, calls setup once, then
loop/yield. `--iterations=N` bounds the sketch for CI; otherwise it runs
continuously. The test runner initializes the core on the native thread,
returns GoogleTest's status and shuts down the shared timer service. Examples
and tests are in `//examples/arduino_pico` and `//test/arduino_pico`.


## FreeRTOS variant

`--config=roo_testing_arduino_rp2040_freertos` compiles the portable core with
`__FREERTOS`, real contended CoreMutex support, and the pinned POSIX kernel.
Destination-owned startup replaces hardware USB/network/core-1 initialization.
The main/test body runs in a FreeRTOS task; delay() and yield() release execution.
delayMicroseconds() consumes host time without a kernel wait (and does not move
manual uptime). noInterrupts()/interrupts() balance nested scheduler critical
sections; hardware IRQ sources and user interrupt callbacks remain unsupported.

SPI beginTransaction/endTransaction reserve a controller across settings, CS
edges and waits. Wire follows the real core: applications must serialize whole
beginTransmission/write/endTransmission/requestFrom/read conversations using
their own mutex. Unread RX bytes hold no controller reservation, and the next
beginTransmission replaces them. A no-STOP transfer retains bus START/STOP
state until a subsequent STOP or end(), independently of task ownership;
flush() neither clears RX nor finishes that bus conversation.
SerialUART serializes RX queue and TX operations; Serial's peek state is guarded
against task preemption. Configure lifecycle/pins and borrowed device wiring
before concurrent use. Finish owned mutexes/reservations before deleting tasks.
Independent native threads must not call the board concurrently.

GoogleTest status is returned unchanged. Runners delete remaining application
tasks, stop the ticker, join native contexts, and close pending clock callbacks.
One scheduler lifetime is supported per executable. Core-1 entry points fail
explicitly. See [the design and validation](../../../doc/pico_freertos.md).
