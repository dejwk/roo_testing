# Build-profile integration tests

The Arduino and IDF global-environment tests compile unrelated C and C++
libraries with no roo_testing dependency and verify their exact macro sets.
The IDF test additionally rejects every Arduino/board macro. Frontend tests
prove the shared ESP-IDF capability and mutually exclusive `is_idf` and
`is_arduino` selections. IDF smoke tests exercise the stable
`//:esp_idf_main` and `//:esp_idf_gtest_main` entry points.

The complete profile matrix covers ESP32, ESP32-S3, and ESP32-C3 under both
frontends, with sequential Bazel runs and per-chip peripheral regressions.

The Arduino positives run in the normal root suite. IDF positives are manual so
the Arduino-default wildcard does not analyze them. Run both frontend matrices,
configuration-boundary checks, composition probes, and action-command-line
checks with:

```sh
./test/profile/verify_profile.sh
```

The default and ESP-IDF profiles check both `//:all` and `//...` with a
deliberately broken package behind a nested `bazel-*` symlink. Successful discovery proves generated
outputs are excluded. Profile guards declare their required constraints as
Bazel target compatibility: wildcards skip foreign profiles, while explicit
mismatches fail with Bazel's incompatible-target diagnostic. Macro/identity
compile contracts still reject contradictory compiler flags.

This also runs `verify_bazel_tools.sh`, whose fake-Bazel checks cover nested
Bazelisk wrapper discovery settings, argument forwarding, startup-option and
`--` parsing, explicit and future frontend configs, disabled rc loading,
mixed-profile rejection, and the six-profile helper's fail-fast behavior.
The profile matrix also runs the public `roo_arduino_example` fixture, verifies
that its public target is a native `cc_binary`, and requires it to be
incompatible under the IDF-only frontend.

The runner uses `$HOME/.cache/roo_testing/profile-integration` by default and
rejects `/tmp` for every configurable Bazel storage path. It verifies expected
failures for a plain host platform, an unsupported SoC, and a selected
roo_testing platform whose compiler profile is absent. It also builds explicit
user-`--copt` and ASAN probes and checks with `aquery` that every canonical
definition occurs exactly once in unrelated C and C++ compile actions.
The ASAN probe is public at
`@roo_testing//test/profile:asan_profile_probe` so reusable client CI can prove
that the root workspace's `--config=asan` actually enables compiler
instrumentation.
It preserves the user's home rc (and its disk cache), suppressing only the
roo_testing workspace rc when exercising the mutually exclusive IDF profile.
Real interactive defaulting requires Bazelisk 1.21.0 or newer; direct Bazel
invocations select a frontend explicitly.

## Generic identity and runtime boundaries

Run the focused phase-1 checks independently with:

```sh
./test/profile/verify_identity.sh
```

The full verifier also invokes them. An identity-only test platform declares
Arduino, a known SoC, and bare-metal runtime without ESP-IDF capability. The
fixture compiles generic Arduino/SoC contracts and verifies that no IDF or
Arduino-ESP32 dependency is selected. Negative checks assert diagnostic text for
backend capability requirements, SoC mismatches, and missing/contradictory
runtime macros. C runtime contracts and C++ selection probes run across all six
existing profiles, alongside global environment and frontend selections.
No fixture provides a supported new framework implementation.

The full verifier also runs the [SDK-only Pico checks](../pico/verify_pico.py)
after the six ESP32 profiles. They validate native C/C++ SDK probes, dependency
and compile-action isolation, symbol uniqueness, explicit unsupported behavior,
profile failures, and ASAN. The six-profile helper remains unchanged until the
later Arduino-Pico matrix integration stage.

Pico stage 3 adds clock/runner tests in separate manual and auto executables.
The Pico verifier also checks the optional opaque timestamp ABI and intentional
GoogleTest failure results after pending alarm captures are destroyed. Shared
clock/alarm regressions remain ordinary framework-independent test targets.

Pico stage 4 adds digital GPIO and a bounded blink test to `//:pico_tests`. The
verifier checks the board/adapter dependencies and C GPIO symbols as well as
time, and runs the blink executable. GPIO tests include invalid masks/banks,
preflighted wiring conflicts, overrides, floating inputs, and shared-time voltage
observations. The existing ESP32 GPIO tests retain their original implementation.

Pico stage 5 adds UART route/baud/invalid-mode tests, two-controller cable I/O,
short and large binary transfers, and a bounded serial example to `//:pico_tests`.
The verifier checks UART adapter isolation and unique C symbols, runs the serial
example, and tests console pipes in normal/ASAN configurations. Console tests
keep an empty stdin pipe open to catch blocking readiness and require explicit
failure for Pico blocking reads at EOF. Both boards use the shared
`ConsoleUartDevice`; ESP32 nonblocking reads return zero on EOF. Shared console
unit tests cover short reads without padding, stable lookahead, EOF, unchanged
input flags, concurrent readers, and complete output under descriptor
backpressure. Both directions use POSIX I/O; a live subprocess verifies a single
write becomes visible before subsequent communication or process exit. They also run through the Arduino/IDF FreeRTOS test runners. The full profile verifier
checks pipe input/output through the default ESP32 console wiring under all six
profiles with `test/uart_console_test.py`.

Pico stage 6 adds SPI routing, modes 0–3, 8/16-bit byte order, CS selection,
multiple-responder failures, bounded chunks, transfer duration, and invalid-call
preflight tests. Separate manual/auto tests cover shared-clock overflow and
progression. The ST77xx example asserts command/pixel transactions and rendered
pixels headlessly. The verifier checks SPI C symbols and shared bus isolation,
runs the example, and includes the new targets in its ASAN sweep.
