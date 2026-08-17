# Emulated SoC profiles

`//roo_testing/soc:target` is the single compile-time identity used by the
emulator, ESP-IDF headers, and Arduino headers. It currently aliases the
concrete `:esp32` profile, which represents the classic dual-core Xtensa ESP32
and Arduino's generic **ESP32 Dev Module** variant.

The profile exports the target and architecture macros shared by ESP-IDF and
Arduino, including `CONFIG_IDF_TARGET_ESP32`, `CONFIG_IDF_TARGET`,
`CONFIG_IDF_TARGET_ARCH_XTENSA`, and `ESP32`. It also exports
`ROO_TESTING_SOC` and `ROO_TESTING_SOC_ESP32`, and provides typed C++ constants
in `roo_testing/soc_profile.h`.

Frontend-specific values do not live in the SoC profile. The
[`//roo_testing/frameworks/environment:arduino`](../frameworks/environment/README.md)
target owns `ARDUINO`, `ARDUINO_ARCH_ESP32`, `ARDUINO_ESP32_DEV`,
`ARDUINO_BOARD`, and `ARDUINO_VARIANT`; the ESP-IDF environment owns
`ESP_PLATFORM` and `IDF_VER`. Both environments select this SoC profile and
are propagated by the public framework targets.

The Linux process is the execution host, not the emulated ESP-IDF target.
Consequently, public framework consumers do not see
`CONFIG_IDF_TARGET_LINUX`. The pthread-backed FreeRTOS port receives that
implementation detail only while its own sources are compiled.

## Adding another SoC

Do not point `:target` at a new family just to make its conditional code
compile. Add a separate concrete profile and all of the corresponding pieces:

1. its mutually exclusive ESP-IDF target and architecture macros;
2. matching ESP-IDF SoC header/include roots and an Arduino environment with
   the corresponding architecture, board, and variant;
3. target-specific shims and `FakeEsp32` capabilities (GPIO count, buses,
   peripherals, chip information, and register behavior);
4. compile-time identity tests and behavioral tests for those capabilities.

Only then should `:target` be changed or made configurable. At present,
`FakeEsp32` implements classic ESP32 behavior; ESP32-C3, ESP32-S3, and other
families are deliberately not advertised as supported profiles.
