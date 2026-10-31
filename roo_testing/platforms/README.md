# Emulated target platforms

`//roo_testing/platforms:arduino_<soc>` and
`//roo_testing/platforms:idf_<soc>` support `esp32`, `esp32s3`, and `esp32c3`.
They inherit the detected host platform because emulator binaries execute
natively, then add independent public constraints:

* `:roo_testing` selects the emulator;
* `:esp_idf` says ESP-IDF APIs are available in both profiles;
* mutually exclusive `:idf` and `:arduino` values select the framework
  frontend;
* mutually exclusive `:bare_metal` and `:freertos` values select application
  runtime; all six current profiles use FreeRTOS;
* mutually exclusive `:esp32`, `:esp32s3`, and `:esp32c3` select SoC behavior.

Public `config_setting` targets (`:is_roo_testing`, `:is_esp_idf`, `:is_idf`,
`:is_arduino`, `:is_esp32`, `:is_idf_esp32`, and `:is_arduino_esp32`) let
clients select optional sources and dependencies without testing compiler flags
directly. The SoC and combined frontend settings also exist with `esp32s3`
and `esp32c3` suffixes. `:is_esp_idf` is a capability test and matches both frontends; use
`:is_idf` for an IDF-only choice.

The platform deliberately has no `flags` attribute. Bazel replaces all prior
values of a repeatable option such as `--copt` when that option comes from a
platform, which would discard ASAN, warning, and user flags. The matching
named bazelrc profiles own compiler identity and select these platforms.

A future SoC gets a new `constraint_value`, platform, and named compiler
profile. It must also supply matching headers, Arduino variant/pins, shims, and
emulated peripheral behavior before it is advertised.

Use `:is_freertos` and `:is_bare_metal` for runtime-dependent selections.
Neither implies a framework frontend, SoC, or ESP-IDF capability. A platform
must pair its runtime constraint with the matching global compiler identity;
see the [environment contract](../frameworks/environment/README.md#runtime-identity).
Bare-metal identity is used by contract fixtures and the SDK-only RP2040
platform described below.

## SDK-only RP2040 profile

`--config=roo_testing_pico_rp2040` selects `:pico_rp2040`, combining `:pico`
frontend, `:pico_sdk` capability, `:rp2040` SoC, `:bare_metal` runtime, and
`:roo_testing` environment. Public settings `:is_pico`, `:is_pico_sdk`, and
`:is_rp2040` select these dimensions independently. The Pico SDK capability is
separate from ESP-IDF capability; adding Pico does not alter existing settings.

The wrapper recognizes explicit Pico profiles and rejects mixing them with
Arduino or IDF profiles. Arduino ESP32 remains the interactive default. The
six-profile helper retains its existing ESP32 matrix in this stage; run the
[Pico verifier](../../test/pico/verify_pico.py) separately. The full profile
verifier invokes it after the ESP32 checks. See the
[SDK support matrix](../frameworks/pico_support/README.md) for the current subset.
