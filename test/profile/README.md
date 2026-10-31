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
