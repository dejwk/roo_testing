# Framework build environments

Framework identity is a property of the target configuration, not of a
particular library dependency. The `roo_testing_arduino_esp32` and
`roo_testing_idf_esp32` bazelrc profiles supply canonical macros globally,
while their corresponding platforms describe the same choices with public
Bazel constraints.

The environment targets do not export `defines`. Instead, they validate the
selected configuration in layers:

* `:emulator` requires the `:roo_testing` platform constraint and validates
  `ROO_TESTING`;
* `:esp_idf` additionally requires `:esp_idf` and validates `ESP_PLATFORM` and
  `IDF_VER`;
* `:idf` adds the mutually exclusive IDF frontend constraint for IDF-only
  entry points, while `:esp_idf` remains valid under Arduino too;
* `//roo_testing/soc:esp32` independently requires the `:esp32` constraint and
  validates all classic-ESP32 target and architecture values;
* `:arduino` requires the complete Arduino/ESP-IDF/ESP32 emulator platform and
  validates the Arduino frontend, board, variant, USB, and logging values.

Missing or wrong constraints fail during analysis, rather than marking tests
incompatible and silently skipping them. Selecting the correct platform
without its compiler profile passes analysis but fails a compile guard with an
actionable error. Exact string values such as `IDF_VER`, `ARDUINO_BOARD`, and
the SoC name are compile-time assertions too.

Applications normally depend on `@roo_testing//:arduino`,
`@roo_testing//:arduino_main`, or `@roo_testing//:arduino_gtest_main` for APIs,
implementation, and startup behavior. Those dependencies are not the mechanism
that supplies global identity. An unrelated library, a main sketch, and a
framework source all receive the same macros from the root build
configuration.

## Root-workspace setup

Dependency `.bazelrc` files are not inherited. Vendor the canonical shared base
and the frontend fragment(s) supported by the root workspace under
`.roo_testing/bazelrc`. Arduino-only clients import:

```bazelrc
import %workspace%/.roo_testing/bazelrc/esp32/base.bazelrc
import %workspace%/.roo_testing/bazelrc/esp32/arduino.bazelrc
```

ESP-IDF-only clients replace the Arduino fragment and config with
`.roo_testing/bazelrc/esp32/idf.bazelrc` and `roo_testing_idf_esp32`. Mixed
clients import both frontend fragments. The source of truth is the
[`.roo_testing/bazelrc/esp32`](../../../.roo_testing/bazelrc/esp32) hierarchy;
source URLs are embedded in its files so vendored copies can be audited and
refreshed. The filesystem import is required because Bazel cannot resolve an
`@roo_testing//...` label while reading rc files. Dependency rc files are never
inherited, including with a local module override.

For a conditional Arduino default, vendor `.roo_testing/bin/bazel`, set
`BAZELISK_WRAPPER_DIRECTORY=.roo_testing/bin` in the root `.bazeliskrc`, and
use Bazelisk 1.21.0 or newer. Plain configured commands then announce and use
Arduino, while an explicit `--config=roo_testing_idf_esp32` remains IDF-only.
This cannot be expressed as an unconditional rc default because named configs
and their repeatable compiler options accumulate. The wrapper also recognizes
future `roo_testing_arduino_*` and `roo_testing_idf_*` profile names.
An Arduino-only root may instead set its Arduino config unconditionally in
`.bazelrc`; mixed roots must use conditional wrapper or explicit selection.

`base.bazelrc` is the single source for emulator, ESP-IDF, and classic ESP32
macros. The frontend fragments add only their platform and frontend-specific
values. This hierarchy leaves room for other frontends and SoCs without
duplicating common identity. Each profile uses ordinary `--copt` entries so
ASAN, warning, debug, and user copts append normally. Compiler flags
deliberately do not live in the platform's `flags` attribute:
platform-supplied values for a repeatable option replace earlier values and
would discard those user flags.

The profiles are explicit and mutually exclusive. A client workspace should
activate exactly one. The wrapper preserves roo_testing's historical
`bazel test :all` command without placing an additive default in `.bazelrc`.
Mixed clients can run `.roo_testing/bin/test_all_profiles`; it invokes native
tests once per frontend and stops on the first failure.

Do not add a second definition of a canonical identity macro. The profile
integration tests use `aquery` to ensure each one occurs exactly once.

## Public selections

Clients can use the public settings below in `select()` expressions:

* `@roo_testing//roo_testing/platforms:is_roo_testing`
* `@roo_testing//roo_testing/platforms:is_esp_idf`
* `@roo_testing//roo_testing/platforms:is_idf`
* `@roo_testing//roo_testing/platforms:is_arduino`
* `@roo_testing//roo_testing/platforms:is_esp32`
* `@roo_testing//roo_testing/platforms:is_idf_esp32`
* `@roo_testing//roo_testing/platforms:is_arduino_esp32`

Inside roo_testing itself, omit the `@roo_testing` repository prefix. These
settings examine platform constraints, not free-form compiler flags.
`is_esp_idf` is a capability present in both frontends; `is_idf` is the
mutually exclusive IDF-only frontend.
