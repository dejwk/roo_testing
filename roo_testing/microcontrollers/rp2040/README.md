# RP2040 board, GPIO, UART, and SPI model

`@roo_testing//roo_testing/microcontrollers/rp2040:pico` provides
[`FakePicoBoard` and `FakePico()`](fake_pico.h). The process-wide board constructs
lazily and performs no hardware access. It owns a fixed array of 30 pin states,
attachment kinds, and configuration markers. The shared `FakeGpioInterface`
retains voltage signals and routes borrowed external devices; it stays private
so Pico checks cannot be bypassed by unchecked pin access or replacement wiring.
No ESP32 matrix, ESP-IDF headers, or FreeRTOS scheduler are involved.

## SDK behavior

Each pin retains its mux function, raw SIO direction and output latch, pulls,
input enable, and input/output/output-enable overrides. Initial state selects
NULL with input enabled, output disabled, latch low, and a weak pull-down,
matching the RP2040 pad reset. GPIO numbers are physical GP0 through GP29;
board header positions and QSPI pins are not a second numbering scheme.

`gpio_init()` selects SIO input, clears the latch and overrides, enables input,
and preserves pulls. `gpio_deinit()` selects NULL and releases strong drive,
retaining raw SIO direction/latch and pulls. Function changes clear overrides
and enable input, following the
[pinned SDK implementation](https://github.com/raspberrypi/pico-sdk/blob/079c6f39023649b154152db30f1d781e884879bc/src/rp2_common/hardware_gpio/gpio.c).
The [pad reset definitions](https://github.com/raspberrypi/pico-sdk/blob/079c6f39023649b154152db30f1d781e884879bc/src/rp2040/hardware_regs/include/hardware/regs/pads_bank0.h)
provide the reset defaults.

SIO, NULL, legal UART TX/RX, and SPI SCK/TX/RX function selection are supported.
I2C SDA/SCL muxes are also supported. PIO, UART CTS/RTS, and hardware SPI
CSn muxes remain unsupported.
Non-normal output/output-enable overrides on non-SIO pins also fail;
this stage does not claim forced-drive semantics for disconnected peripherals.
All GPIO interrupt operations, including disable/acknowledge requests, fail
explicitly. Raw IRQ APIs have declarations for types but no implementation.
Input hysteresis, drive strength, slew rate, and the two-pull bus keeper are
unsupported. They are not successful no-ops.

Single-pin APIs, masked set/clear/xor/put and direction operations, whole-bank
operations, and their 64-bit variants use checked low-bank state. RP2040 has
only bank 0 in this model. Masks and whole-bank values containing GP30 or higher
are rejected before state changes. Masked value bits outside the mask are
ignored. The entire prospective mask change is validated before configuration
is committed or any observer is called. Selected pin state is coherent during
callbacks; notification order is ascending pin number and does not model
simultaneous electrical edges.

## Wiring and observation

Attach borrowed voltage devices before configuring their pins and keep them
alive for all GPIO activity. Duplicates, replacement wiring, and late
attachments fail explicitly. Hot detach and ownership transfer are not exposed.
Board configuration and attachments are single-threaded. Observers can inspect
board state and update external voltage sources; they must not reconfigure
GPIOs from inside a notification.

```cpp
#include "hardware/gpio.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

static roo_testing_transducers::ConstVoltage button("button", 0.0f);
static roo_testing_transducers::SimpleVoltageSink led("LED");

void ConfigureFixture() {
  FakePico().attachInput(1, button);
  FakePico().attachOutput(25, led);
  gpio_init(1);
  gpio_init(25);
  gpio_put(25, 1);  // Preload the latch while the pin is still an input.
  gpio_set_dir(25, GPIO_OUT);
}
```

`attachInput()` connects an external source; enabling strong output on it is a
wiring conflict. `attachOutput()` connects an observer of strong output voltage
assignments. `attach()` connects a `VoltageIO`: its read method reports the
external drive, with NaN denoting high impedance, and its write method observes
board drive. Opposing strong input/output levels fail when the pad is read.
Weak pulls yield to external voltage. An unforced floating or intermediate
voltage fails digital reads rather than choosing an arbitrary bit. Input disable
forces the raw input low before input override; forced high/low input overrides
also work on floating pads.

For fixture inspection, `state(pin)` returns a value snapshot, `driveSignal(pin)`
returns the active strong voltage signal or no value, and `padVoltage(pin)`
returns resolved pad voltage or NaN when floating. `gpio_get_out_level()` reads
the raw output latch, while `gpio_get()` reads pad input after enable/override.
Neither direction nor output override changes that raw latch.

Output changes publish complete shared `VoltageSignal` values at the same uptime
used by SDK clocks and sleeps. Identical output assignments and pad-only changes
do not generate false output transitions. External periodic sources sample the
same emulated clock; GPIO operations themselves do not advance manual time.

A `VoltageSink` retains its most recent assignment. Releasing a driver does not
invent a zero voltage or a NaN `VoltageSignal`, and does not erase that history.
Use `driveSignal()` and `padVoltage()` to inspect release/floating state. Weak
pulls are resolved on reads rather than sent as strong output assignments. This
is not a full electrical network or contention/current solver.

The board copies its fixed 30-pin configuration for preflight and publishes only
changed drives. It also owns two small UART and two SPI configuration records, with vectors
of borrowed device connections allocated when fixture wiring is registered. Shared GPIO adapters are created on attachment or first output use;
steady-state board configuration uses fixed storage. These are host allocations,
not a claim about RP2040 RAM usage.

## UART byte routing

`attachUartDevice(device, tx, rx)` reserves legal physical TX/RX pins before
configuration. Either direction may be omitted with -1. Pins are exclusive with
voltage attachments and other UART devices; the same device cannot be registered
twice. The borrowed device must outlive UART activity. Wiring is immutable,
configuration is single-threaded, and reconfiguration from device callbacks is
unsupported. Existing cable queues can receive data from native producer threads.

| TX | RX | Controller |
| --- | --- | --- |
| GP0 | GP1 | UART0 |
| GP4 | GP5 | UART1 |
| GP8 | GP9 | UART1 |
| GP12 | GP13 | UART0 |
| GP16 | GP17 | UART0 |
| GP20 | GP21 | UART1 |
| GP24 | GP25 | UART1 |
| GP28 | GP29 | UART0 |

A pair may combine different groups belonging to the same controller. RX and TX
resolve independently through the currently selected GPIO functions. Each
requested direction must have exactly one selected physical route and an attached
device; UART0 GP0/GP1 substitute the host console when no explicit device occupies
that pin. Unwired alternate routes and simultaneous aliases fail explicitly.

UART pin state retains raw SIO metadata, but selecting SIO output on a reserved
UART pin fails. No UART bit waveform is generated: `driveSignal()` and
`padVoltage()` on reserved or UART-muxed pins fail rather than reporting fake
levels. RX input disable or overrides fail before byte reads. `uartState(index)`
returns lifecycle, actual baud, and CRLF settings for fixture inspection.
See the [runtime contract](../../frameworks/pico_support/README.md) for blocking,
console, clock, and unsupported-mode semantics.

## SPI device routing

`attachSpiDevice(device, clk, miso, mosi)` connects an existing
`SimpleFakeSpiDevice`. SCK and MOSI are required; MISO may be -1 for a write-only
device. All supplied pins must belong to the same controller and have the correct
role. Nonadjacent groups are legal. GP numbers are checked before narrowing.

| Controller | MISO | SCK | MOSI |
| --- | --- | --- | --- |
| SPI0 | GP0, GP4, GP16, GP20 | GP2, GP6, GP18, GP22 | GP3, GP7, GP19, GP23 |
| SPI1 | GP8, GP12, GP24, GP28 | GP10, GP14, GP26 | GP11, GP15, GP27 |

Devices can share bus pins when attached before configuration; duplicate device
registration, voltage/UART attachment conflicts, and late attachment fail. Devices
remain borrowed and must outlive bus activity. No device owns a controller or
changes another device's bindings.

Set each used bus pin to `GPIO_FUNC_SPI`. Exactly one selected SCK and MOSI route
is required for every transfer, plus MISO for receives. Multiple selected aliases
of any SPI role are rejected. Matching attachments are resolved through current
mux state. A selected device on a different physical clock/TX route is unclocked.
GPIO input-enable/override controls on an active receive pad are validated before
transfer. SPI muxes/reservations have no voltage waveform: electrical queries fail,
and SIO output on a reserved bus pin is rejected.

Chip select stays in the shared voltage model:

```cpp
FakePico().attachSpiDevice(display, 18, -1, 19);
FakePico().attachOutput(17, display.cs());
// Attach DC/reset as ordinary output sinks before configuring their GPIOs.
```

Application code configures CS as SIO output and drives it low to select the
model. Attach display DC/reset the same way. Hardware CSn is unsupported.
Selection is sampled once per SDK call and retained through bounded chunks.
Changing bus configuration or CS from device callbacks is unsupported; callbacks
can inspect controller state and observe `busy == true`.

`spiState(index)` returns lifecycle, busy, achieved baud, frame width, and mode.
The [SPI runtime contract](../../frameworks/pico_support/README.md#synchronous-spi-stage-6)
defines transfer errors, dummy bytes, byte order, timing, and unsupported modes.

I2C device wiring follows the [runtime contract](../../frameworks/pico_support/README.md#i2c-stage-7); it shares borrowed device models and reserves SDA/SCL before configuration.

ADC and PWM use the [shared signal runtime](../../frameworks/pico_support/README.md#adc-and-pwm-stage-8). PWM GPIO aliases share slice configuration; ADC consumes borrowed external voltage sources.
