# Framework build environments

These targets publish the compile-time identity that normally comes from the
Arduino or ESP-IDF build frontend. They are layered so a consumer gets one
consistent context through its ordinary framework dependency:

* `:emulator` defines `ROO_TESTING=1`.
* `:esp_idf` adds `ESP_PLATFORM`, `IDF_VER`, and the selected
  `//roo_testing/soc:target` profile.
* `:arduino` adds the Arduino frontend, board, variant, USB, and logging
  defaults. It currently publishes `ARDUINO=10819`.

Applications normally depend on `@roo_testing//:arduino`,
`@roo_testing//:arduino_main`, or `@roo_testing//:arduino_gtest_main`, rather
than these implementation targets. ESP-IDF users depend on the public
`//roo_testing/frameworks/esp-idf` targets. All of those framework targets
carry the corresponding environment transitively.

Use `@roo_testing//roo_testing/frameworks/environment:emulator` directly only
for a host-only target that needs to test `ROO_TESTING` but intentionally does
not depend on Arduino or ESP-IDF.

Do not define `ARDUINO`, `ESP32`, `ESP_PLATFORM`, `ROO_TESTING`, or individual
`CONFIG_IDF_TARGET_*` macros in a workspace `.bazelrc`. Bazel does not inherit
roo_testing's `.bazelrc` when roo_testing is a dependency, and command-line
definitions can conflict with the framework-owned values. A future selectable
SoC or Arduino frontend profile should replace the relevant environment target
atomically rather than override one macro with `--copt`.
