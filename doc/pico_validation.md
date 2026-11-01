# Pico host integration validation

The host implementation covers stages 1–11 of the
[Pico integration design](pico_integration.md). Physical-board release
qualification remains pending. Pico FreeRTOS is the explicitly planned
subsequent release, rather than an enabled profile in this implementation.

## Host profile matrix

Validation was performed on Linux on 2026-11-01. The canonical verifier is
`./test/profile/verify_profile.sh`; it serializes Bazel commands and preserves
the user's resource limits and disk cache. Its default output-user root is
`~/.cache/roo_testing/profile-integration/configured`, never `/tmp`.

| Profile | Main suite | Result |
| --- | --- | --- |
| `roo_testing_arduino_esp32` | `//:all_tests` | 55 passed |
| `roo_testing_idf_esp32` | `//:idf_tests` | 17 passed |
| `roo_testing_arduino_esp32s3` | `//:all_tests` | 55 passed |
| `roo_testing_idf_esp32s3` | `//:idf_tests` | 17 passed |
| `roo_testing_arduino_esp32c3` | `//:all_tests` | 55 passed |
| `roo_testing_idf_esp32c3` | `//:idf_tests` | 17 passed |
| `roo_testing_pico_rp2040` | `//:pico_tests` | 21 passed, normal and ASAN |
| `roo_testing_arduino_rp2040` | `//:arduino_pico_tests` | 23 passed, normal and ASAN |

The Arduino-Pico suite skips the two SDK-only compiler-identity probes by
explicit frontend compatibility; the SDK suite runs them. The ESP32 String
link regression was also run under ASAN on all three Arduino profiles.

The verifier additionally checks global C/C++ compiler identity, native ELF
identity, selected implementations, framework dependency isolation, wrapper
argument preservation/profile conflicts, public entry-point guards, console
binary I/O/EOF and output visibility before subsequent communication, manual
and automatic clock modes, runner cleanup and exact failing-test exit status.
Unsupported IRQ, core-1, FreeRTOS and alarm paths are checked through explicit
runtime failures or negative compilation/link probes. The Pico CI jobs cover
both frontends in normal and ASAN builds; the reusable client workflow accepts
both Pico profile names.

Both pinned Copybara workflows were replayed into isolated folders: all 570
Arduino-Pico core files and all 48 ArduinoCore-API files matched the checkout
byte for byte. Imported code retains its upstream formatting and licenses;
host edits are recorded in the patch series.

## Coordinated Roo clients

Run `python3 test/pico_clients/verify_clients.py` with sibling local checkouts,
or set `ROO_CLIENT_ROOT`. It creates one persistent validation workspace with
local overrides, copies the canonical profile fragments, checks dependencies,
runs normal/ASAN suites serially, and shuts down that workspace's Bazel server.
Client release versions have not been bumped or published. Review the coordinated
client commits: roo_threads `fa93a8b`, roo_time `b384f5c`, roo_scheduler `19b02c0`
and roo_display `b608881`.

| Client | Tested scope | Result |
| --- | --- | --- |
| `roo_threads` | Existing single-threaded locks/sleep on both Pico profiles; ESP32 Arduino/IDF backend selection; compile-only future RP2040 FreeRTOS selection | Passed |
| `roo_time` | Core time, compact time, native and portable formatting using the selected frontend | Passed on both Pico profiles, normal and ASAN |
| `roo_scheduler` | Dispatch, cancellation, allocation, delay, automatic time, and idle cooperative event pumping | Passed on Arduino-Pico, normal and ASAN |
| `roo_display` | Existing synchronous Arduino SPI transport drawing a headless frame through the shared ST77xx model; const input and byte order | Passed on Arduino-Pico, normal and ASAN; both real frontend APIs compile |

ESP32 client regressions also cover formatting, scheduler dispatch/concurrency,
and backend selection. The supported display target is `@roo_display//:arduino_spi`.
The full display library, filesystem stack, thread-dependent touch/reset drivers
and cross-thread application behavior are excluded from the initial Pico
support claim. Bare-metal mutexes do not provide native-host-thread exclusion.

The client validation exposed two integration requirements beyond runtime
selection: roo_time formatting must use the stable Arduino frontend target,
and the generic display transport must use Pico's set-pin/transfer API.
It also exposed undeclared ESP32 String/logging archive dependencies; those are
now explicit and covered by an ordinary-main regression without startup.

## Physical-board release qualification

The available PlatformIO ARM GCC 14.3.0, CMake and Ninja tools were found
outside PATH and used with pristine pinned source archives. The
[target compilation harness](../test/pico_target/README.md) linked an SDK
firmware executable and compiled the Arduino peripheral sketch plus supported
Roo consumers into ARM EABI5 32-bit objects. No native overlay headers or
`ROO_TESTING` identity were used. The real Arduino umbrella header's ancillary
lwIP headers came from the SDK's exact gitlink. This validates target API
signatures; it does not enable host networking support.

No serial board device was available. Full Arduino firmware linking and
physical smoke runs have not been performed. Physical-board release
qualification remains outstanding; host/compile results do not establish
electrical behavior, final memory fit or real-time performance.

Before a hardware-qualified release, complete Arduino firmware linking with
the pinned revisions and a matching 125 MHz system clock. Run
blink, crossed UARTs, DS3231 repeated-START reads, ADC/PWM signals and an SPI
display on the original Pico. Confirm the observed data and pin behavior on
the board. The host-only attachments in the bounded Arduino example are
excluded from hardware compilation; physical peripherals/wiring are required.
