# roo_testing 2.3.0

- Fix Wi-Fi scan cancellation to emit a completion event, preventing controller timeouts and subsequent “Wi-Fi is not ready” errors.
- Add WPA3 and mixed WPA2/WPA3 simulation, with negotiated authentication reporting and per-attempt overrides.
- Improve ESP-IDF DNS and DHCP compatibility: validate DNS addresses, require DHCP to stop before setting static IP information, and preserve fallback DNS when clearing primary and secondary entries.
- Make FLTK viewports fail immediately with diagnostics for invalid dimensions or drawing rectangles.

No uncommitted dependency upgrades were present in the working tree.

---

# roo_testing 2.2.0

### Added

- Added comprehensive ESP32 NVS emulation, including key iteration, partition lifecycle, storage statistics, capacity limits, and additional security APIs.
- Added scriptable Wi‑Fi scenarios for connection failures, retries, roaming, timed link loss, AP visibility, and RSSI changes.

### Changed

- Replaced the custom event shim with ESP-IDF’s queued event loop.
- Improved Wi‑Fi scan, driver lifecycle, netif state, event ordering, and Arduino reconnect fidelity.

### Fixed

- Preserved static IPv4 configuration during Wi‑Fi association.
- Fixed stale IP events, invalid lifecycle calls, hidden-AP filtering, dangling AP state, and NVS validation and commit/erase semantics.

---

# roo_testing 2.1.3

### Added

- Added `nvs_find_key()` support to ESP32 NVS emulation, including key existence checks and stored-type reporting for all supported NVS value types.

---

# roo_testing 2.1.2

### Added

- Emulated the modern ESP-IDF I2C master API, including bus/device lifecycle, probing, synchronous transmit/receive, and repeated-start combined transfers through fake ESP32 GPIO routing.
- Added modern I2C support and regression coverage for `FakeDs3231`.

### Changed

- Updated Bazel dependencies, including `rules_cc`, `rules_python`, GoogleTest, and Protobuf.

---

# [roo_testing 2.1.1](https://github.com/dejwk/roo_testing/releases/tag/2.1.1)

Published 2026-08-30.

This patch release improves stability and fidelity of the ESP-IDF host runtime.

### Fixed

- Prevented stopped system timers from linking manual time and disrupting automatic time advancement.
- Fixed FreeRTOS event waits so cancellation reliably releases event mutexes.
- Moved emulated tick and peripheral interrupt handling onto the alternate signal stack.
- Released pthread attribute resources after FreeRTOS task creation.
- Corrected host `app_main()` behavior: returning now deletes only the main task, while the scheduler and peripheral emulation continue running—matching ESP-IDF behavior.
- Updated the vendored ESP-IDF shutdown patch for the improved host-runtime lifecycle.

**Full Changelog:** https://github.com/dejwk/roo_testing/compare/2.1.0...2.1.1

---

# [roo_testing 2.1.0](https://github.com/dejwk/roo_testing/releases/tag/2.1.0)

Published 2026-08-29.

# roo_testing 2.1.0

roo_testing 2.1.0 substantially expands ESP32 host emulation with deterministic
time and interrupts, ESP-IDF `esp_timer`, waveform-aware GPIO output, complete
steady-state LEDC emulation, linear PWM fades, and improved UART and SD support.

This release imports Arduino-ESP32 3.3.11 and ESP-IDF 6.0.2.

## Highlights

### Deterministic time and interrupt emulation

- Added link-selected manual and host-synchronized emulated-time modes.
- Added deterministic one-shot system-time alarms with explicit manual-time
  delivery and autonomous host-time delivery.
- Added simulated ISR execution through the FreeRTOS Linux port, including
  `FromISR` API and deferred-yield behavior.
- Added an interrupt controller and ESP-IDF adapter supporting allocation,
  shared handlers, status filtering, enable/disable, and safe handle lifetime.
- Added scheduler-safe host locking for state shared with simulated interrupts.

### ESP-IDF `esp_timer`

- Added a host backend for the vendored ESP-IDF `esp_timer` service.
- Supports public timer creation, start, restart, stop, deletion, activity,
  period, and time-query APIs.
- Timer expiration is delivered through the emulated hardware and interrupt
  boundary rather than a separate host-only callback implementation.
- Added ESP-IDF and Arduino startup integration.
- Added runnable Arduino and ESP-IDF `esp_timer` examples.

ISR-dispatched `esp_timer` callbacks remain unsupported.

### Waveform-aware voltage and GPIO emulation

- Added immutable, time-aware `VoltageSignal` values.
- Supports constant, sine, triangle, rising/falling sawtooth, and square/PWM
  waveforms.
- Added linear duty-cycle envelopes for PWM fades.
- Added explicit-uptime sampling and analytical DC, RMS, AC RMS, minimum,
  maximum, peak-to-peak, and peak measurements.
- Fake GPIO and built-in voltage sinks now retain and propagate complete
  waveforms instead of reducing outputs to scalar voltages.

### ESP-IDF and Arduino LEDC

- ESP-IDF LEDC now publishes PWM waveforms through fake GPIO, including timer
  frequency, shared carrier origins, duty, `hpoint`, inversion, reconfiguration,
  pin reassignment, and deconfiguration behavior.
- Arduino LEDC now supports attach/write/read, tone and note generation,
  frequency and resolution changes, inversion, detach, channel reuse, and
  `analogWrite` delegation.
- Added linear LEDC fades for ESP-IDF and Arduino.
- Fade support includes:
  - Time-aware PWM envelopes and integer midpoint readback.
  - Interrupt-backed endpoint completion.
  - ESP-IDF wait/no-wait operation and ISR callbacks.
  - Arduino completion callbacks with and without callback arguments.
  - Ordered final GPIO publication before completion notification.
  - Cancellation on detach or channel reassignment.
  - Rejection of conflicting writes and concurrent same-channel fades.

Arduino gamma fades and advanced ESP-IDF operations such as step or multi-range
fades, fade stop, timer pause/resume, and channel-timer rebinding remain
unsupported.

### UART, SD, networking, and host integration

- Added a bidirectional `FakeUartCable` for connecting two emulated UART
  endpoints.
- Fixed UART RX notification routing and ESP-IDF UART shim behavior.
- Fixed delayed visibility of `Serial.print()` output.
- Added ESP-IDF SDMMC/SDSPI VFS mounts backed by host directories.
- Added an ESP-IDF Bazel example helper.
- `bazel run` now uses the workspace directory as the emulated filesystem root.
- ESP-IDF example paths can automatically select the ESP-IDF build profile.
- Fixed station-IP events being reported against the AP network interface.
- Improved Wi-Fi shim and scanning behavior.
- Improved FLTK Escape-key handling without closing emulator windows.

SD block transactions and realistic SD timing are not emulated.

## Build and test improvements

- Added a clean FreeRTOS-only GoogleTest runner for low-level interrupt tests.
- Improved host-runner shutdown for the emulated time and `esp_timer` services.
- Fixed Linux FreeRTOS scheduler shutdown under AddressSanitizer while retaining
  the supplied FreeRTOS buffers as the actual pthread stacks.
- Scheduler-owned task teardown is ordered safely around page-granular ASAN
  stack reclamation, preserving stack-size and overflow fidelity.
- Fixed global interrupt-dispatcher state leaking into low-level interrupt
  tests.
- Fixed profile verification failures and `esp_timer` compilation warnings.

## Compatibility notes

- `VoltageSink` now uses the waveform-aware `VoltageSignal` virtual contract.
  External custom sink implementations must migrate from the former
  scalar-only interface.
- Emulated-time mode is selected at link time and remains immutable for the
  lifetime of the process.
- Host builds continue to emulate only the classic dual-core Xtensa ESP32.
  ESP32-C3, ESP32-S3, and other Espressif SoCs are not yet selectable.
- The host ABI remains in effect: pointers, `size_t`, and `long` are generally
  64-bit on Linux but 32-bit on ESP32.

---

# [roo_testing 2.0.1](https://github.com/dejwk/roo_testing/releases/tag/2.0.1)

Published 2026-08-21.

## What's Changed
* Bump actions/checkout from 6.0.2 to 7.0.1 by @dependabot[bot] in https://github.com/dejwk/roo_testing/pull/1
* Unifying asan support

**Full Changelog**: https://github.com/dejwk/roo_testing/compare/2.0.0...2.0.1

---

# [roo_testing 2.0.0](https://github.com/dejwk/roo_testing/releases/tag/2.0.0)

Published 2026-08-18.

# roo_testing 2.0.0

roo_testing 2.0 modernizes the ESP32 host emulator around **Arduino-ESP32 3.3.11** and **ESP-IDF 6.0.2**, adds a first-class ESP-IDF-only frontend, and substantially improves display, networking, filesystem, FreeRTOS, and peripheral emulation.

## Highlights

- Upgraded from Arduino-ESP32 2.0.4 and ESP-IDF 4.4.1 to Arduino-ESP32 3.3.11 and ESP-IDF 6.0.2.
- Added separate, mutually exclusive Arduino and ESP-IDF-only build profiles.
- Made Arduino, ESP-IDF, emulator, and SoC identity available globally to every compiled library.
- Added native host entry points for ESP-IDF `app_main()` applications and tests.
- Improved Linux-backed networking, filesystems, SPI, I2C, UART, LEDC, NVS, SD compatibility, and FreeRTOS behavior.
- Added multi-display support and resizable, automatically magnified TFT emulator windows.
- Added a public Bazel macro for building and running `.ino` examples.
- Pinned and validated the project with Bazel 9.2.0.
- Expanded automated testing and added reusable GitHub Actions CI for Roo libraries.

## Modern framework stack

Framework sources now live under stable, unversioned Bazel paths:

- `roo_testing/frameworks/arduino-esp32`
- `roo_testing/frameworks/esp-idf`

Legacy versioned Bazel paths are retained as compatibility aliases where practical.

Framework imports are now reproducible: Copybara imports immutable upstream revisions while preserving Roo-owned Bazel overlays, host shims, and portability patches.

## Arduino and ESP-IDF frontends

Two build profiles are now available for the emulated classic ESP32:

- `roo_testing_arduino_esp32`
- `roo_testing_idf_esp32`

Build identity is configuration-wide rather than propagated through a particular library dependency. Consequently, libraries can reliably detect `ARDUINO`, `ROO_TESTING`, `ESP_PLATFORM`, and the ESP32 target macros even when they do not directly depend on roo_testing.

Public Bazel constraints and `config_setting` targets allow BUILD files to select appropriate sources and dependencies:

- `is_roo_testing`
- `is_esp_idf`
- `is_arduino`
- `is_idf`
- `is_esp32`
- Combined Arduino/IDF ESP32 selectors

Missing, mixed, or contradictory profiles now fail with actionable analysis or compilation errors.

New public entry points include:

- `@roo_testing//:esp_idf_main` for applications defining `extern "C" void app_main()`
- `@roo_testing//:esp_idf_gtest_main` for ESP-IDF-only tests
- Existing `arduino_main` and `arduino_gtest_main` entry points remain available

Both ESP-IDF entry points start the pthread-backed FreeRTOS scheduler without initializing Arduino.

## Improved emulator fidelity

### Networking and Wi-Fi

- `NetworkClient`, `NetworkServer`, and `NetworkUDP` now bridge to native Linux TCP and UDP sockets.
- Hostname resolution uses the host resolver and supports IPv4- or IPv6-first `localhost` configurations.
- `select()` and `poll()` timeouts remain correct while FreeRTOS scheduler ticks are active.
- Added compatibility shims for current Wi-Fi event, SmartConfig, antenna, and lifecycle APIs.

### Filesystems and storage

- Updated host-backed SPIFFS and LittleFS behavior, including directory and format operations.
- Preserved the current Arduino FS, SD, and SD_MMC compatibility surfaces.
- Added missing SD initialization and teardown shims.
- NVS is initialized during Arduino startup, so `Preferences` is immediately usable.

SD emulation remains intentionally simplified and maps storage operations to the host filesystem.

### SPI, I2C, UART, and LEDC

- ESP-IDF `spi_master` transactions now route through attached fake devices.
- Arduino SPI clock-divider selection and encoding now match current upstream behavior.
- Current Arduino Wire master operations route through the emulated ESP32 I2C controllers.
- Unsupported I2C slave functionality fails safely without breaking master-only programs.
- Updated UART support for current event queues, pin routing, IRDA direction, and RX pull APIs.
- Added deterministic host emulation for IDF LEDC timer, channel, duty, update, and fade operations.
- Peripheral-manager state is now safe to use from global constructors.

### FreeRTOS and runtime

- Ported and linked the current ESP-IDF pthread-backed FreeRTOS host runtime.
- Improved task, semaphore, and thread-join lifecycle handling.
- Scheduler tick delivery no longer corrupts `errno` or prematurely interrupts blocking host operations.
- Added current GPIO, LCD HAL, FATFS, SDMMC, ring-buffer, and other public ESP-IDF header surfaces needed by downstream libraries.

## Better TFT emulator experience

- Multiple emulated displays can now run simultaneously in separate windows.
- Display windows can be resized while preserving their aspect ratio.
- Windows automatically snap to an integral magnification after resizing.
- Touch and mouse coordinates remain mapped to the display’s logical resolution.
- Closing an emulator window terminates the emulator instead of leaving the process hung.
- The bundled FLTK core no longer links unused JPEG, PNG, or zlib image loaders. TFT emulation requires only X11 development files, not a system FLTK installation or image-codec development packages.

## Bazel and developer experience

roo_testing now pins **Bazel 9.2.0** and provides profile support under `.roo_testing/`.

A Bazelisk wrapper preserves the convenient historical workflow:

```sh
bazel build ...
bazel test ...
bazel run ...
```

When no frontend is selected, the wrapper announces and selects Arduino ESP32. An explicit IDF profile remains uncontaminated:

```sh
bazel test ... --config=roo_testing_idf_esp32
```

Mixed projects can exercise the same targets under both frontends:

```sh
.roo_testing/bin/test_all_profiles ...
```

A new public helper creates a native runnable target directly from a sketch:

```starlark
load("@roo_testing//roo_testing/emulation:arduino.bzl", "roo_arduino_example")

roo_arduino_example(
    name = "demo",
    sketch = "demo.ino",
    deps = ["//:application"],
)
```

The resulting target is a normal `cc_binary` usable with `bazel build` and `bazel run`. Sketches must be valid C++; Arduino IDE automatic prototype generation is not reproduced.

All bundled standalone examples now use the same profile and wrapper layout.

## CI and regression coverage

- Added reusable GitHub Actions CI for host, Arduino, and ESP-IDF profiles.
- Added normal and AddressSanitizer build/test phases.
- Added pull-request-safe Bazel caching and pinned action versions.
- Expanded the Arduino-profile suite from 7 to 29 tests.
- Added a separate six-target ESP-IDF-only suite.
- Added validation for global compiler identity, platform selection, framework versions, SoC identity, entry points, host networking and filesystems, FreeRTOS, SPI, I2C, UART, SD, NVS, LEDC, and runnable sketches.

## Upgrading from 1.3.7

This is a major framework and build-configuration upgrade.

1. Update the module dependency:

   ```starlark
   bazel_dep(name = "roo_testing", version = "2.0.0")
   ```

2. Remove manually defined framework identity macros such as `ARDUINO`, `ROO_TESTING`, `ESP32`, `ESP_PLATFORM`, and `CONFIG_IDF_TARGET_*`.

3. Vendor and import the profile files in the root workspace. Arduino-only workspaces import:

   ```bazelrc
   import %workspace%/.roo_testing/bazelrc/esp32/base.bazelrc
   import %workspace%/.roo_testing/bazelrc/esp32/arduino.bazelrc
   ```

   Mixed Arduino/IDF workspaces import the base, IDF, and Arduino fragments.

4. For the announced Arduino default, use Bazelisk 1.21 or newer and add:

   ```text
   # .bazeliskrc
   BAZELISK_WRAPPER_DIRECTORY=.roo_testing/bin
   ```

   Bazel does not inherit `.bazelrc` files from dependencies, so this root-workspace setup is required.

5. Prefer the stable public entry points such as `@roo_testing//:arduino_main`, `@roo_testing//:arduino_gtest_main`, `@roo_testing//:esp_idf_main`, and `@roo_testing//:esp_idf_gtest_main`.

6. Use the public platform selectors in BUILD files instead of inspecting or redefining compiler flags.

Applications may also require source changes for upstream Arduino-ESP32 3.x or ESP-IDF 6 APIs.

The current profiles emulate the classic dual-core Xtensa ESP32/ESP32 Dev Module. ESP32-C3, ESP32-S3, and other SoCs are not yet advertised as supported emulator targets.

**Full changelog:** https://github.com/dejwk/roo_testing/compare/1.3.7...2.0.0

---

# [roo_testing 1.3.7](https://github.com/dejwk/roo_testing/releases/tag/1.3.7)

Published 2026-08-15.

Implemented the 'host event endpoint' for delivering interrupt-like notifications from external emulation host to the FreeRTOS environment.


---

# [roo_testing 1.3.6](https://github.com/dejwk/roo_testing/releases/tag/1.3.6)

Published 2026-04-22.

Bug fixes in the display emulation:
* deadlock when probing mouse from a different thread than updating display content.
* incorrect handling of magnification factors.

**Full Changelog**: https://github.com/dejwk/roo_testing/compare/1.3.5...1.3.6

---

# [roo_testing 1.3.5](https://github.com/dejwk/roo_testing/releases/tag/1.3.5)

Published 2026-02-28.

Made it possible to configure display noise for the FLTK viewport programmatically without overriding flags,

Setting the noise level allows seeing which pixels exactly are (over)written by the device drivers, which helps tuning the rendering code.

**Full Changelog**: https://github.com/dejwk/roo_testing/compare/1.3.4...1.3.5

---

# [roo_testing 1.3.4](https://github.com/dejwk/roo_testing/releases/tag/1.3.4)

Published 2026-02-25.

Some small but important usability fixes:
* open the FLTK window in the middle of the screen, rather than the top-left corner (recent regression),
* slightly more precise SPI protocol handling in the display drivers.

**Full Changelog**: https://github.com/dejwk/roo_testing/compare/1.3.3...1.3.4

---

# [roo_testing 1.3.3](https://github.com/dejwk/roo_testing/releases/tag/1.3.3)

Published 2026-02-25.

Fixed the emulated register-level GPIO for C++, which seems to have regressed some time ago. Added a regression test to prevent that from happening again.

With this fix, C++ code that directly writes to reads from GPIO registers, should be properly emulated. This is used, for example, to test the low-level GPIO routines in roo_display.

**Full Changelog**: https://github.com/dejwk/roo_testing/compare/1.3.2...1.3.3

---

# [roo_testing 1.3.2](https://github.com/dejwk/roo_testing/releases/tag/1.3.2)

Published 2026-02-24.

Fixing crashes in roo_threads, in case the libunwind is present.

**Full Changelog**: https://github.com/dejwk/roo_testing/compare/1.3.1...1.3.2

---

# [roo_testing 1.3.1](https://github.com/dejwk/roo_testing/releases/tag/1.3.1)

Published 2026-02-23.

* Cleaned up the build of compiler warnings.
* Some minor cleanup.

**Full Changelog**: https://github.com/dejwk/roo_testing/compare/1.3.0...1.3.1

---

# [roo_testing 1.3.0](https://github.com/dejwk/roo_testing/releases/tag/1.3.0)

Published 2026-02-08.

* No more compiler warnings!
* A few small bugfixes
* Removed buffer overruns in SPI code which generate asan warnings
* Removed dependency on protobuf for NVS, which should make tests build faster.

**Full Changelog**: https://github.com/dejwk/roo_testing/compare/1.2.1...1.3.0

---

# [roo_testing 1.2.1](https://github.com/dejwk/roo_testing/releases/tag/1.2.1)

Published 2026-01-25.

Updated BUILD and MODULE.bazel files, to make them work again after bazel behavior has changed (removing cc_library and requiring an explicit imports). Also, updated dependencies to the newest version (proto, glog).

**Full Changelog**: https://github.com/dejwk/roo_testing/compare/1.2.0...1.2.1


---

# [roo_testing 1.2.0](https://github.com/dejwk/roo_testing/releases/tag/1.2.0)

Published 2026-01-06.

* Fix: long-standing rendering issue with the TFT emulator not always refreshing completely.
* New feature: support for large-area screen copy, to support emulation of lvgl.
* Fixed examples.

**Full Changelog**: https://github.com/dejwk/roo_testing/compare/1.1.8...1.2.0

---

# [roo_testing 1.1.8](https://github.com/dejwk/roo_testing/releases/tag/1.1.8)

Published 2025-11-12.

Added basic littlefs emulation.

**Full Changelog**: https://github.com/dejwk/roo_testing/compare/1.1.7...1.1.8

---

# [roo_testing 1.1.7](https://github.com/dejwk/roo_testing/releases/tag/1.1.7)

Published 2025-10-30.

* Bugfix: make sure that locks work when called from static initializers (since the Espressif locks seem to work in such case).

**Full Changelog**: https://github.com/dejwk/roo_testing/compare/1.1.6...1.1.7

---

# [roo_testing 1.1.6](https://github.com/dejwk/roo_testing/releases/tag/1.1.6)

Published 2025-10-30.

Build file tweaks, and .gitignore.

**Full Changelog**: https://github.com/dejwk/roo_testing/compare/1.1.5...1.1.6

---

# [roo_testing 1.1.5](https://github.com/dejwk/roo_testing/releases/tag/1.1.5)

Published 2025-10-26.

* Experimental support for shipping with a prebuilt Linux library, to speed up builds.
* Updated .gitignore.

**Full Changelog**: https://github.com/dejwk/roo_testing/compare/1.1.4...1.1.5


---

# [roo_testing 1.1.4](https://github.com/dejwk/roo_testing/releases/tag/1.1.4)

Published 2025-10-08.

* Significantly improved UART emulation.

---

# [roo_testing 1.1.3](https://github.com/dejwk/roo_testing/releases/tag/1.1.3)

Published 2025-10-05.

* Better thread debugging: pushing task names to pthreads so that they show up in debugging tools.
* Make esp_random() thread-safe, to fix deadlocks when called from multiple tasks (due to rand() using global locks that are not aware of freertos).

**Full Changelog**: https://github.com/dejwk/roo_testing/compare/1.1.2...1.1.3

---

# [roo_testing 1.1.2](https://github.com/dejwk/roo_testing/releases/tag/1.1.2)

Published 2025-09-12.

Numerous fixes, particularly related to multithreading, to support emulation for roo_threads.

---

# [roo_testing 1.1.1](https://github.com/dejwk/roo_testing/releases/tag/1.1.1)

Published 2025-08-09.

**Full Changelog**: https://github.com/dejwk/roo_testing/compare/1.1.0...1.1.1

---

# [roo_testing 1.1.0](https://github.com/dejwk/roo_testing/releases/tag/1.1.0)

Published 2025-08-09.

Making roo_testing a Bazel module, to allow depending libraries use it in unit tests.

Fixing a few bugs.

**Full Changelog**: https://github.com/dejwk/roo_testing/compare/1.0.0...1.1.0

---

# [roo_testing 1.0.0](https://github.com/dejwk/roo_testing/releases/tag/1.0.0)

Published 2025-08-06.

Initial release.

---

