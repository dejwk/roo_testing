# roo_testing

Experimental ESP32 emulator that can be used to test Arduino sketches on Linux, before uploading them to the microcontroller. Supports emulation of external I2C and SPI devices, including TFT displays.

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

Now let's simulate a SPI-based TFT display. This example requires an FLTK library.

```
sudo apt-get install bazel
sudo apt-get install libfltk1.3-dev
mkdir foo; cd foo
git clone https://github.com/dejwk/roo_testing.git
cp -rT roo_testing/examples/tft_display/ .
lib/init.sh
bazel run :main
```

# How it works

The code is based on the Espressif ESP32 Arduino framework, instrumented to intercept hardware- or interface-level events and couple them with simulated implementations.

The code does not emulate the Xtensa microprocessors. Your sketch simply compiles and runs on your computer's architecture. This works because the Espressif framework is implemented in standard, portable C/C++.

## Emulated SoC identity

Host builds currently emulate the classic dual-core Xtensa **ESP32** through
either an ESP-IDF-only frontend or Arduino's generic **ESP32 Dev Module**
frontend. The
[`//roo_testing/platforms`](roo_testing/platforms/README.md) package records the
emulator, ESP-IDF capability, mutually exclusive frontend, and concrete SoC as
independent constraints. A matching named Bazel profile supplies its canonical
macro set to every C and C++ compile action.

Linux is the execution host, not the target exposed to application code. Other
Espressif families are not selectable yet: advertising ESP32-C3, ESP32-S3, or
another SoC must wait for matching headers, Arduino pins, shims, and
`FakeEsp32` behavior.

### Enable the build profile in the root workspace

Bazel reads rc files only from the root workspace; it does not inherit the
`.bazelrc` of a Bzlmod dependency or local override. Vendor the shared ESP32
base fragment and the frontend fragment(s) the workspace supports from
`bazelrc/esp32/`, then import them in the root `.bazelrc`. Activate exactly one
frontend per invocation. For an Arduino-only workspace:

```bazelrc
import %workspace%/bazelrc/esp32/base.bazelrc
import %workspace%/bazelrc/esp32/arduino.bazelrc
build --config=roo_testing_arduino_esp32
```

For ESP-IDF only, import `base.bazelrc` and `idf.bazelrc`, then activate
`roo_testing_idf_esp32`. A mixed workspace may import both frontend fragments
and choose one with `--config` on each command. The fragments include their
canonical source URLs so vendored copies can be audited and refreshed. Do not
activate both frontends in one invocation: their platform values are mutually
exclusive, and accumulating repeatable compiler options would be
contradictory.

For backward compatibility, roo_testing's own root rc imports all three
fragment definitions and defaults to Arduino, so its original smoke command
continues to work:

```sh
bazel test :all
```

Its IDF regression tests run in a clean rc context through
`test/profile/verify_profile.sh`; adding the IDF config to the default Arduino
context would intentionally fail as a mixed profile. The standalone Arduino
examples vendor and activate their own copies. An rc file cannot import an
`@roo_testing//...` label because repository resolution happens after rc
parsing.

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

The profile uses ordinary `--copt` entries, so user copts and
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

## How to use it

The behavior of the physical world is modeled in _transducers_, sensing or indicating physical quantities. Transducers are represented by abstract virtual classes. A simple example is the Thermometer class, with a virtual method to report the temperature. By implementing an arbitrary logic in your own subclasses, you can simulate various real-life scenarios.

The transducers are used by the simulated _devices_, provided as part of the library and mimicking the real hardware, that you virtually 'connect' to your microcontroller at the beginning of the program (as illustrated in the examples). For example, the FakeOneWireThermometer device simulates an actual sensor such as DS18B20, and communicates with the emulated microcontroller using the actual One Wire protocol, but reports temperatures indicated by your custom thermometer sensor.

Another basic example is the VoltageSource, which is a transducer that you can use to feed signals to the microcontroller via its GPIO pins. Using emulated GPIO and voltage inputs, you can emulate an external logic, e.g. calculate a logical function of some GPIO outputs and feed it back to a GPIO input. 

## What is supported

* GPIO, both digital I/O and analog inputs
* SPI, emulated at pin level, accurately modeling bus speeds
* I2C
* UART
* Networking
* FreeRTOS API
* SPIFFS and LittleFS, mouting a local directory
* NVS, using a local file for storage
* SD (rudimentary)
* External devices: a couple of TFT displays, the DS3231 real time clock, and the temperature sensors using the OneWire interface.

## Limitations (call for contributors!)

* Arduino and ESP-IDF-only frontends are supported for the classic ESP32; other
  Espressif SoCs are not yet selectable.
* WiFi is incomplete; only the station mode is reasonably emulated. Some functions are no-op. The network bridges to your native connection. As long as your computer is connected to the network, the emulated microcontroller will also have network access.
* SD is also very rudimentary; it redirects file system operations to a local directory, without emulating any of the SPI protocol. (The consequence is, for example, that performance is unrealistically fast).
* I2C is modeled at the interface level, bypassing some low-level OS queues and hardware pins. (As long as you use standard libraries, it doesn't matter much).
* Simulated TFT displays don't model all commands, just the basic set used by common libraries.
* Interrupts are not currently supported.
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
