# Emulated target platforms

`//roo_testing/platforms:arduino_esp32` and
`//roo_testing/platforms:idf_esp32` are the supported target configurations.
They inherit the detected host platform because emulator binaries execute
natively, then add independent public constraints:

* `:roo_testing` selects the emulator;
* `:esp_idf` says ESP-IDF APIs are available in both profiles;
* mutually exclusive `:idf` and `:arduino` values select the framework
  frontend;
* `:esp32` selects the classic ESP32 SoC behavior.

Public `config_setting` targets (`:is_roo_testing`, `:is_esp_idf`, `:is_idf`,
`:is_arduino`, `:is_esp32`, `:is_idf_esp32`, and `:is_arduino_esp32`) let
clients select optional sources and dependencies without testing compiler flags
directly. `:is_esp_idf` is a capability test and matches both frontends; use
`:is_idf` for an IDF-only choice.

The platform deliberately has no `flags` attribute. Bazel replaces all prior
values of a repeatable option such as `--copt` when that option comes from a
platform, which would discard ASAN, warning, and user flags. The matching
named bazelrc profiles own compiler identity and select these platforms.

A future SoC gets a new `constraint_value`, platform, and named compiler
profile. It must also supply matching headers, Arduino variant/pins, shims, and
emulated peripheral behavior before it is advertised.
