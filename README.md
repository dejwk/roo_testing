# roo_testing

Experimental ESP32 and RP2040 emulator that can be used to test Arduino sketches on Linux, before uploading them to the microcontroller. Supports emulation of external I2C and SPI devices, including TFT displays.

I built it to test my sketches that tend to use a variety of external devices, from TFT displays to temperature sensors, networking, SD cards, and real-time clock modules. It is not 100% accurate, but it gets the job done.

The current version imports Arduino-ESP32 3.3.11 and ESP-IDF 6.0.2 into stable,
unversioned framework paths. The upstream source trees are maintained with
[Copybara](copybara/README.md), while Linux host shims and Bazel BUILD overlays
remain owned by roo_testing.

## TL;DR:

```
sudo apt-get install bazel
mkdir foo; cd foo
git clone https://github.com/dejwk/roo_testing.git
cp -rT roo_testing/examples/gpio/ .
bazel run :main
```

## More interesting example

Now let's look at an example that simulates an external I2C real-time clock, using the popular DS3231 module:

```
sudo apt-get install bazel
mkdir foo; cd foo
git clone https://github.com/dejwk/roo_testing.git
cp -rT roo_testing/examples/rtc_ds3231_i2c/ .
lib/init.sh
bazel run :main
```

## Even more interesting example

Now let's simulate a SPI-based TFT display. This example uses the bundled FLTK
core library and requires the X11 development files.

```
sudo apt-get install bazel
sudo apt-get install libx11-dev
mkdir foo; cd foo
git clone https://github.com/dejwk/roo_testing.git
cp -rT roo_testing/examples/tft_display/ .
lib/init.sh
bazel run :main
```

# How it works

The ESP32 backend uses the Espressif Arduino framework and ESP-IDF; the Pico backend uses the pinned Pico SDK and Arduino-Pico core. Hardware calls connect to shared simulated buses, devices, signals and host integrations.

The code does not interpret Xtensa, RISC-V or ARM instructions. Applications compile and run on the host architecture, using portable framework code and native peripheral adapters.

## Native Pico SDK and Arduino-Pico

The original non-wireless Pico (RP2040, core 0) supports three native profiles:

| Profile | Public library / process entry point |
| --- | --- |
| `roo_testing_pico_rp2040` | `:pico_sdk`, ordinary C/C++ `main()`, `:pico_gtest_main` |
| `roo_testing_arduino_rp2040` | `:arduino`, `:arduino_main`, `:arduino_gtest_main` |
| `roo_testing_arduino_rp2040_freertos` | Same Arduino targets, with a single-core FreeRTOS scheduler |

Vendor these canonical fragments into the root workspace and import them in
its `.bazelrc` alongside the shared ASAN fragment:

```bazelrc
import %workspace%/.roo_testing/bazelrc/rp2040/base.bazelrc
import %workspace%/.roo_testing/bazelrc/rp2040/pico.bazelrc
import %workspace%/.roo_testing/bazelrc/rp2040/arduino.bazelrc
import %workspace%/.roo_testing/bazelrc/rp2040/freertos.bazelrc
```

The SDK and original Arduino profiles use a bare-metal runtime. The FreeRTOS
variant adds task scheduling. All three share clock, GPIO, 8N1 UART,
synchronous SPI/I2C, external ADC and free-running PWM adapters. They reuse the
existing console, fake buses/devices and voltage signals. Arduino-Pico 6.3.1
uses its actual pinned API/core and original `rpipico` variant above Pico SDK
2.3.1. `Serial` substitutes the unbuffered host console for USB and warns once.
The Pico graph has no ESP-IDF dependency.

Run `bazel run --config=roo_testing_arduino_rp2040 //examples/arduino_pico:peripherals -- --iterations=2`
for a bounded blink/UART/DS3231/SPI sketch. SDK examples cover
[serial](examples/pico_uart/README.md), [I2C](examples/pico_i2c/README.md),
[ADC/PWM](examples/pico_signal/README.md) and a
[headless SPI display](examples/pico_spi/README.md).
See the [SDK support matrix](roo_testing/frameworks/pico_support/README.md) and
[Arduino support matrix](roo_testing/frameworks/arduino_pico_support/README.md).

Coordinated local `roo_threads` and `roo_time` overrides are required until
matching client releases are available. Supported client validation covers
roo_time, cooperative roo_scheduler and roo_display's Arduino SPI transport;
thread-dependent clients use the separate FreeRTOS profile. The full
filesystem/display stack remains outside the supported Pico surface.
Run `python3 test/pico_clients/verify_clients.py` with sibling Roo checkouts, or
set `ROO_CLIENT_ROOT`. See [release validation](doc/pico_validation.md) for exact
scope and the pending physical-board qualification. See
[the FreeRTOS runtime contract](doc/pico_freertos.md) for task-aware support.

The Arduino-Pico FreeRTOS profile uses the pinned 11.1.0+ kernel with shared
emulated uptime. Run:

```sh
bazel test //:arduino_pico_freertos_tests --config=roo_testing_arduino_rp2040_freertos
```

This exercises task startup, preemption, queues, notifications, software
timers, peripheral contention, and cleanup;
append `--config=asan` for sanitizer validation. Run
`python3 test/pico_freertos/verify_clients.py` with sibling Roo checkouts for
thread, time, scheduler, and display-transport consumer validation. See
[the runtime design and support contract](doc/pico_freertos.md).
Vendor `.roo_testing/bazelrc/rp2040/freertos.bazelrc` and import it after the RP2040
base fragment to enable this config in a client workspace. Roo consumers select
`//roo_testing/runtime:freertos` (or `:freertos_headers`) to obtain the matching
kernel without importing ESP-IDF. Supported execution is one scheduler core and
one scheduler lifetime per native executable. Complete bus reservations before
deleting their owner; core 1, hardware IRQ callbacks, DMA, USB and networking
retain the existing unsupported status.

## Emulated SoC identity

Host builds support **ESP32**, **ESP32-S3**, and **ESP32-C3** through both
Arduino and ESP-IDF-only frontends. Each Arduino profile uses the generic Dev
Module variant with USB boot features disabled and a UART console. Classic
ESP32 remains the default.

The [platforms](roo_testing/platforms/README.md) define independent frontend
and SoC constraints. Matching Bazel profiles supply identity macros globally
and select the chip's headers, pin maps, and existing peripheral models.
For example, use `--config=roo_testing_arduino_esp32s3` or
`--config=roo_testing_idf_esp32c3` after importing their matching rc fragments.
See [SoC profiles](roo_testing/soc/README.md) for the support boundaries.

Linux is the execution host. ESP32 profiles retain the single-core host scheduler;
reported physical core counts and target architecture reflect the selected chip.

For S3 or C3, import all three matching fragments, then select one frontend:

```text
import %workspace%/.roo_testing/bazelrc/esp32s3/base.bazelrc
import %workspace%/.roo_testing/bazelrc/esp32s3/arduino.bazelrc
import %workspace%/.roo_testing/bazelrc/esp32s3/idf.bazelrc
```

Replace `esp32s3` with `esp32c3` for C3. Existing classic-only client workspaces
can keep their current configuration until they opt into the additional chips.

### Enable the build profile in the root workspace

Bazel reads rc files only from the root workspace; it does not inherit the
`.bazelrc` of a Bzlmod dependency or local override. Vendor the support files
from [`.roo_testing`](.roo_testing) into the same nested directory in the root
workspace. The SoC-first layout keeps future profiles incremental:

```text
.roo_testing/
  bin/bazel
  bin/test_all_profiles
  bazelrc/common/asan.bazelrc
  bazelrc/esp32/{base,arduino,idf}.bazelrc
  bazelrc/esp32s3/{base,arduino,idf}.bazelrc
  bazelrc/esp32c3/{base,arduino,idf}.bazelrc
  bazelrc/rp2040/{base,pico,arduino}.bazelrc
```

Import the common ASAN fragment, the shared base, and every frontend that the
workspace supports. For an Arduino-only workspace:

```bazelrc
import %workspace%/.roo_testing/bazelrc/common/asan.bazelrc
import %workspace%/.roo_testing/bazelrc/esp32/base.bazelrc
import %workspace%/.roo_testing/bazelrc/esp32/arduino.bazelrc
```

For ESP-IDF only, import `asan.bazelrc`, `base.bazelrc`, and `idf.bazelrc`.
Mixed workspaces import all four fragments. They include canonical source URLs
so vendored copies can be audited and refreshed.

A root supporting only classic ESP32 with Arduino may keep
`build --config=roo_testing_arduino_esp32` in `.bazelrc`. Roots supporting
multiple SoCs or frontends must use the wrapper or select a profile explicitly
without an unconditional rc default.

To make Arduino the interactive default without contaminating an explicit IDF
invocation, use **Bazelisk 1.21.0 or newer** and vendor these root settings:

```text
# .bazeliskrc
BAZELISK_WRAPPER_DIRECTORY=.roo_testing/bin
```

Bazelisk then discovers `.roo_testing/bin/bazel`. For configured commands such
as `build`, `test`, `run`, `info`, `cquery`, and `aquery`, the wrapper inserts
the Arduino ESP32 config only when no `roo_testing_arduino_*` or
`roo_testing_idf_*` or `roo_testing_pico_*` config was supplied, and prints a
concise notice. These commands each select one profile:

```sh
bazel test :all
bazel test ...
bazel test ... --config=roo_testing_idf_esp32
```

Wildcard builds skip libraries and tests belonging to other frontend/SoC
profiles. Explicitly requesting an incompatible target still fails with a
constraint diagnostic. `REPO.bazel` excludes generated `bazel-*` output trees
and the six standalone example modules; run those examples from their own
directories so Bazel loads their dependencies.

An unconditional `build --config=roo_testing_arduino_esp32` in `.bazelrc`
cannot provide this behavior. Bazel expands a command-line SoC/frontend config in
addition to the rc default, and repeatable `--copt` values have no reset
operation, leaving both macro sets on the compiler command line. The wrapper
chooses the default before rc expansion instead. It leaves explicit profiles
untouched, rejects two distinct explicit profiles, and does not inject a
default when workspace rc loading is disabled. Invoking the Bazel binary
directly or setting `BAZELISK_SKIP_WRAPPER=true` bypasses this convenience and
therefore requires an explicit profile.

Mixed workspaces can exercise their native test targets under all nine profiles
without Starlark transitions:

```sh
.roo_testing/bin/test_all_profiles ... --test_output=errors
```

The helper requires the three fragments for each ESP32 SoC and the RP2040
`base`, `pico`, and `arduino` fragments in the root `.bazelrc`.

With no arguments, the helper tests `...`. It runs Arduino and then IDF for
ESP32, ESP32-S3, and ESP32-C3, then SDK, Arduino and Arduino with FreeRTOS for RP2040, forwarding the same target patterns and options
to all nine runs and stopping on the first failure. Targets must support the
selected profiles or declare their platform compatibility. The standalone
Arduino examples vendor the configuration fragments and nested wrapper, so
their plain Bazel commands use the same announced default. An rc file cannot import an `@roo_testing//...` label because
repository resolution happens after rc parsing.

### AddressSanitizer builds and tests

The vendored `bazelrc/common/asan.bazelrc` fragment defines a frontend-neutral
AddressSanitizer mode. Append `--config=asan` to the same command you normally
use. With the wrapper's Arduino default, the short form remains:

```sh
bazel build ... --config=asan
bazel test ... --config=asan --test_output=errors
bazel run //path/to/package:example --config=asan
```

Select ESP-IDF explicitly and compose it with the same ASAN config:

```sh
bazel build ... --config=roo_testing_idf_esp32 --config=asan
bazel test ... --config=roo_testing_idf_esp32 --config=asan \
  --test_output=errors
```

To test all nine profiles under ASAN, pass the config through the existing
nine-profile helper:

```sh
.roo_testing/bin/test_all_profiles ... --config=asan --test_output=errors
```

These commands require the common ASAN fragment to be imported by the root
workspace; dependency rc files are never inherited. Repository-specific ASAN
workarounds may remain as additional `build:asan` entries in the root
`.bazelrc`, where Bazel composes them with the shared flags.

This one root configuration gives every source in the emulated build the same
selected frontend, ESP-IDF capability, emulator, and SoC identity, including
sublibraries with no roo_testing dependency. A sketch target does not add
`@roo_testing//:arduino`
merely to receive macros. It still needs the appropriate API and link
dependencies: normally `@roo_testing//:arduino_main` for an Arduino sketch,
`@roo_testing//:arduino_gtest_main` for a test, and
`@roo_testing//:arduino` for a library that directly uses Arduino symbols or
headers. ESP-IDF tests use the stable
`@roo_testing//:esp_idf_gtest_main` entry point, which starts the emulated
FreeRTOS scheduler without initializing Arduino. An ESP-IDF application that
defines `extern "C" void app_main()` links `@roo_testing//:esp_idf_main`.

### Runnable Arduino sketches

Use the public macro for a package-local `.ino` that should become a native
host executable:

```starlark
load("@roo_testing//roo_testing/emulation:arduino.bzl", "roo_arduino_example")

roo_arduino_example(
    name = "demo",
    sketch = "examples/demo/demo.ino",
    deps = ["//:application"],
)
```

`//:demo` is a normal `cc_binary`, so it works with `bazel build` and
`bazel run`. The macro generates a C++ wrapper for the sketch, forwards caller
dependencies and ordinary binary attributes, links the stable
`@roo_testing//:arduino_main`, and becomes incompatible under an IDF-only
profile so wildcard builds skip it cleanly. It does not implement Arduino's
automatic prototype generation: the sketch must be valid C++, include
`Arduino.h` when needed, and declare functions before use. See the
[build-helper reference](roo_testing/emulation/README.md) for the complete API.

Each profile uses ordinary `--copt` entries, so user copts and
`--config=asan` compose with it. Do not redefine `ARDUINO`, `ESP32`,
`ESP_PLATFORM`, `ROO_TESTING`, or `CONFIG_IDF_TARGET_*` in another rc file;
that creates contradictory command lines and is rejected by the profile
guards. The default compatibility value is `ARDUINO=10819`; it is distinct
from the Arduino-ESP32 framework version reported by
`ESP_ARDUINO_VERSION_*`.

Client BUILD files can branch on public platform settings instead of inspecting
macros:

```starlark
deps = select({
    "@roo_testing//roo_testing/platforms:is_arduino": [":arduino_only_dep"],
    "//conditions:default": [],
})
```

Use `:is_idf` for an IDF-only frontend branch. `:is_esp_idf` means that ESP-IDF
APIs are available, so it intentionally matches both IDF-only and Arduino
profiles.

See the [framework environment contract](roo_testing/frameworks/environment/README.md)
for the failure modes and the complete profile location.

## GitHub Actions for Roo clients

Roo repositories can call the reusable workflow in
[`.github/workflows/client-ci.yml`](.github/workflows/client-ci.yml). Pin the
call to a full roo_testing commit SHA, rather than `main` or a movable tag:

```yaml
jobs:
  ci:
    uses: dejwk/roo_testing/.github/workflows/client-ci.yml@FULL_COMMIT_SHA
    with:
      profiles: '["arduino", "idf"]'
      build_targets: '//...'
      test_targets: '//...'
```

`profiles` is a JSON array containing any of `host`, `arduino`, and `idf`.
Arduino and IDF select their frontend configs explicitly; `host` bypasses the
interactive Arduino default. Each matrix entry builds and tests normally and
with ASAN. For emulator profiles, the ASAN phase also compiles the public
`@roo_testing//test/profile:asan_profile_probe`, so an undefined or incomplete
ASAN config cannot silently produce an unsanitized build. Every ASAN matrix
entry also checks that the canonical fragment is present and imported by the
root `.bazelrc`, including host-only clients that do not depend on roo_testing.
`run_tests` and `run_asan` can disable the corresponding phases for build-only
repositories, while `system_packages` and `extra_bazel_args` accept
space-separated additions. All callers use their checked-in `.bazelversion`;
Roo clients pin Bazel 9.2.0. Pull requests may restore trusted Bazel caches but
cannot save new cache entries.

For the 2.1.0 release, push and tag roo_testing first, register that release in
the Roo Bazel registry, and only then push clients whose `MODULE.bazel` requests
`roo_testing` 2.1.0. The reusable workflow does not add a local path override,
so reversing that order makes client dependency resolution fail even when the
workflow itself is already reachable at its pinned commit.

Use this coordinated release order. Complete each group—push and tag its
commits, register every version, and verify that the registry serves them—before
pushing repositories in the next group. Repositories within one group may be
released in parallel.

1. `roo_testing`
2. `roo_time`
3. `roo_threads`
4. `roo_logging`
5. `roo_quantity`, `roo_scheduler`, `roo_io`
6. `roo_prefs`, `roo_display`, `roo_transport`, `roo_io_arduino`,
   `roo_monitoring`, `roo_time_ds3231`, `roo_powersafefs`
7. `roo_blink`, `roo_control`, `roo_onewire`, `roo_fonts_basic`,
   `roo_fonts_material`, `roo_icons`, `roo_display_lvgl`, `roo_wifi`
8. `roo_transceivers`, `roo_windows`
9. `roo_dashboard`, `roo_comms`, `roo_windows_transceivers`,
   `roo_windows_wifi`

## How to use it

The behavior of the physical world is modeled in _transducers_, sensing or indicating physical quantities. Transducers are represented by abstract virtual classes. A simple example is the Thermometer class, with a virtual method to report the temperature. By implementing an arbitrary logic in your own subclasses, you can simulate various real-life scenarios.

The transducers are used by the simulated _devices_, provided as part of the library and mimicking the real hardware, that you virtually 'connect' to your microcontroller at the beginning of the program (as illustrated in the examples). For example, the FakeOneWireThermometer device simulates an actual sensor such as DS18B20, and communicates with the emulated microcontroller using the actual One Wire protocol, but reports temperatures indicated by your custom thermometer sensor.

Another basic example is the `VoltageSource` transducer, which feeds signals to
the microcontroller through GPIO pins. GPIO outputs likewise retain complete,
time-aware `VoltageSignal` values, so tests and emulated devices can inspect a
PWM or other periodic output at a chosen uptime, rather than observing only its
last scalar level. This makes it possible to emulate external logic that
observes GPIO outputs and feeds a result back to a GPIO input. See the
[periodic voltage-signal design](doc/periodic_voltage_signals.md) for the
available waveforms and analysis helpers.

## What is supported

* GPIO: digital I/O, analog inputs, and retained constant or periodic voltage
  signals (including PWM)
* SPI, emulated at pin level, accurately modeling bus speeds
* I2C
* UART, including host console devices and a bidirectional `FakeUartCable`
* Networking and the station-mode Wi-Fi path; see the
  [Wi-Fi simulation reference](doc/wifi_simulation.md)
* FreeRTOS API, simulated ISR context, and the supported ESP-IDF interrupt
  allocation APIs
* Emulated manual or host-synchronized time, one-shot alarms, and ESP-IDF
  `esp_timer`
* SNTP synchronization, cold-boot process-local UTC, and controllable virtual
  server responses; see the [SNTP reference](doc/sntp_reference.md) and
  [Arduino/IDF examples](examples/sntp/README.md)
* ESP-IDF and Arduino LEDC PWM output and linear fades; see the
  [LEDC emulation reference](doc/ledc_voltage_emulation.md)
* SPIFFS and LittleFS, mounting a local directory
* NVS, using a local file for storage
* SD VFS mounts backed by a local directory
* External devices: a couple of TFT displays, the DS3231 real time clock, and the temperature sensors using the OneWire interface.

## Limitations (call for contributors!)

* Arduino and ESP-IDF-only frontends support ESP32, ESP32-S3, and ESP32-C3.
  New chip-specific peripherals such as USB are not emulated.
* Wi-Fi is incomplete; the station mode is the supported path, while many
  radio and AP controls are no-ops. Networking uses the host connection, so an
  emulated device has network access whenever its computer does.
* SD VFS mounts redirect filesystem access to an existing local directory; they
  do not emulate SDMMC/SDSPI block transactions or timing. Consequently,
  performance is unrealistically fast and code that performs raw card I/O is
  unsupported.
* I2C is modeled at the interface level, bypassing some low-level OS queues and hardware pins. (As long as you use standard libraries, it doesn't matter much).
* Simulated TFT displays don't model all commands, just the basic set used by common libraries.
* Interrupt emulation covers the supported FreeRTOS signal path and ESP-IDF
  allocation adapter; it is not a register-accurate model and does not yet
  cover every peripheral or ESP-IDF interrupt-control API. See
  [emulated interrupts](doc/emulated_interrupts.md).
* LEDC intentionally supports a focused API subset. Arduino gamma APIs and
  advanced ESP-IDF LEDC operations such as step/multi-range fades, fade stop,
  timer pause/resume, and channel-timer rebinding remain unsupported.
* The emulator does not accurately reflect the microcontroller's performance - it tends to run faster because your computer has a faster CPU. (Notable exception is the SPI emulation, which reflects communication delays accurately). Also, your computer has way more memory, both on the heap and the stack. The host ABI remains in effect: on a typical 64-bit Linux host, pointers, `size_t`, and `long` are 64-bit, while they are 32-bit on ESP32. (`int` is 32-bit on both.) Use fixed-width types when the width is part of a protocol or stored representation. Because of these differences, a sketch can work in the emulator and still fail on real hardware.

## Debugging with VS Code

Yes! You can debug your sketches using a graphical debugger.

in VS Code, navigate to Run > Add Configuration ..., then select "(gdb) Launch" as the configuration type. Change "program" to "${workspaceFolder}/bazel-bin/main", and "cwd" to "${workspaceFolder}". Finally, build the debug binary by calling

```
bazel build -c dbg :main
```
 
After that, you can Run > Start Debugging (make sure to select the just created configuration).

# Please get involved!

If you find this library useful, but perhaps missing something important for you, please consider contributing. I will be happy to guide and I will gladly review and accept external contributions.

### Modern ESP-IDF I2C master emulation

The native shim supports `i2c_new_master_bus`, bus lookup and deletion, device
registration/removal, synchronous transmit/receive/combined transfers, and
address probing. It routes SDA/SCL through the ESP32 GPIO matrix to devices
attached using `FakeEsp32().attachI2cDevice(...)`, including `FakeDs3231`.
Combined transfers preserve the repeated START (no STOP after the write), and
device errors map to IDF status codes. Complete calls are serialized on the host.

This models synchronous 7-bit transactions. It does not model bus clock timing,
electrical pullups, asynchronous callbacks/queues, power management, or
coexistence with legacy/Arduino bus ownership. Unsupported queue, address-mode,
and ACK-disabling configurations fail explicitly. Device timeouts are forwarded
in milliseconds, saturated to the emulated device's 16-bit range; host mutex
waits do not model the hardware timeout deadline. Transfers over 65535 bytes
are rejected because the emulated peripheral interface has a 16-bit length.

Validate with:

```sh
bazel test --config=roo_testing_idf_esp32 //test:idf_i2c_device_test
```

## RTC device fakes

Calendar/control fakes are available at
`//roo_testing/devices/clock/<device>:<device>` for `ds3231`, `ds3232`, `ds3234`,
`ds1307`, `pcf8523`, `rv3028`, and `rv8803`. Include the matching
`roo_testing/devices/clock/<device>/<device>.h` and construct `FakeDs3231`,
`FakeDs3232`, `FakeDs3234`, `FakeDs1307`, `FakePcf8523`, `FakeRv3028`, or
`FakeRv8803`. They attach to the existing emulated I2C/SPI wiring:

```cpp
FakeDs3232 rtc;
FakeEsp32().attachI2cDevice(rtc, 18, 19);

FakeDs3234 spi_rtc;
FakeEsp32().attachSpiDevice(spi_rtc, 18, 19, 23);
FakeEsp32().gpio.attachOutput(5, spi_rtc.cs());
```

The attached objects must outlive their wiring. Default clocks follow emulator
time; pass a borrowed `roo_testing_transducers::Clock*` to control oscillator
time independently. `FakeDs3231` accepts a clock reference and retains its
existing owning thermometer constructor. Fakes begin at 2000-01-01 with a
chip-appropriate time-loss/halt indication. Initialize their calendar and clear
validity flags through the bus, or use `roo_rtc`'s `set()`.

`reset()` restores the model's initial registers. `simulateTimeLoss()` raises
the chip's loss indication without replacing its plausible calendar.
`injectRegister(address, value)` deliberately bypasses write masks for tests
of invalid BCD, stopped clocks, and pending flags. Ordinary bus writes obey
status write-zero-clear/read-only behavior. `FakeDs3234::protocolError()`
reports unsupported SPI modes, order, frequency, addresses, or partial bytes;
CS edges delimit commands even when a frame spans many transfer() calls.

Models advance their calendar independently of drivers, using a four-year
leap cycle and per-chip weekday/hour encoding. I2C bursts latch calendar reads
where hardware does; RV-8803 reads remain live. DS3234 snapshots on falling CS
and refreshes when the calendar bank wraps from 0x13 to 0x00. DS1307 CH,
PCF8523 STOP, and RV-8803 RESET hold timekeeping; DS323x EOSC has no effect
without a modeled backup-power transition. PCF8523 backup settings and RV-3028
volatile backup/refresh/charging bits are retained through normal register IO.

The scope is calendar/control software testing. Alarm/timer firing, calibration
drift, electrical backup/supply transitions, timestamp capture, EEPROM/password
protocols, RV-3028's independent Unix counter, and DS3234's indirect SRAM port
are not simulated. Unmodeled registers are not a promise of complete chip
emulation. DS3231 temperature follows its thermometer source; DS3232/DS3234
provide fixed 25 degree Celsius temperature registers.

`//roo_testing/devices/clock:rtc_test` tests the models with independent golden
register bytes, oscillator steps, latch/rollover behavior, status masks, and
SPI framing. The seven per-device integration targets in roo_rtc test the
production drivers through the real Arduino/ESP-IDF adapters and these fakes.
