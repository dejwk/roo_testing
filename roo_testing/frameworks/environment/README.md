# Framework build environments

Framework identity is a property of the target configuration, not of a
particular library dependency. The `roo_testing_arduino_esp32` and
`roo_testing_idf_esp32` bazelrc profiles supply canonical macros globally,
while their corresponding platforms describe the same choices with public
Bazel constraints.

The environment targets do not export `defines`. Instead, they validate the
selected configuration in layers:

* `:runtime` requires an explicit runtime platform constraint and checks that
  exactly one matching global runtime macro has value 1;
* `:emulator` requires `:roo_testing`, validates `ROO_TESTING`, and validates
  `:runtime` without linking a scheduler;
* `:arduino` adds only the Arduino frontend constraint and checks the supported
  `ARDUINO` compatibility value. It has no ESP-IDF capability requirement;
* `//roo_testing/soc:identity` exports typed chip identity and validates its
  macro values and name without requiring a framework capability;
* `:esp_idf` additionally requires ESP-IDF capability, validates `ESP_PLATFORM`
  and `IDF_VER`, and requires the current FreeRTOS runtime and concrete IDF SoC
  contract through `//roo_testing/soc:target`;
* `:idf` adds the IDF-only frontend constraint for IDF-only entry points;
* `:arduino_esp32` composes generic Arduino and ESP-IDF validation and checks
  ESP32-specific board, variant, USB, and logging values. The Arduino-ESP32 core
  requires this target rather than relying on generic Arduino identity.

The six supported profiles still select the same ESP32 implementations.
Separating identity does not add a Pico backend or a public bare-metal profile.
The identity-only platform in `test/profile` proves that generic Arduino and SoC
contracts do not imply ESP-IDF capability or framework implementation headers.

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

`base.bazelrc` is the single source for emulator, runtime, ESP-IDF, and classic
ESP32 macros. The frontend fragments add only their platform and
frontend-specific values. This hierarchy leaves room for other frontends and SoCs without
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
* `@roo_testing//roo_testing/platforms:is_freertos`
* `@roo_testing//roo_testing/platforms:is_bare_metal`
* `@roo_testing//roo_testing/platforms:is_esp32`
* `@roo_testing//roo_testing/platforms:is_idf_esp32`
* `@roo_testing//roo_testing/platforms:is_arduino_esp32`

Inside roo_testing itself, omit the `@roo_testing` repository prefix. These
settings examine platform constraints, not free-form compiler flags.
`is_esp_idf` is a capability present in both frontends; `is_idf` is the
mutually exclusive IDF-only frontend.

## Runtime identity

Runtime is independent of frontend and SoC. The `:freertos` and `:bare_metal`
values belong to the `:runtime` constraint setting. Their compiler counterparts
are `ROO_TESTING_RUNTIME_FREERTOS=1` and `ROO_TESTING_RUNTIME_BARE_METAL=1`.
All six existing profiles select FreeRTOS; both C and C++ actions receive that
identity from each chip's canonical base fragment. The bare-metal value reserves
a truthful runtime identity for the subsequent Pico backend. It does not change
roo_threads selection in this phase.

A runtime constraint without its matching macro, both macros together, a value
other than 1, or a missing runtime constraint is rejected. `:is_freertos` tests
application runtime capability, not ESP-IDF availability or native worker-thread
existence. Runtime contracts do not export defines or introduce scheduler links.
Refresh vendored base fragments when adopting this change; old copies omit the
new mandatory runtime identity and fail its compile contract.
