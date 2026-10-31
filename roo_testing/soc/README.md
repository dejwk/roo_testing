# Emulated SoC profiles

The target platform selects the emulated chip. Linux remains the native execution
host; Xtensa and RISC-V identity describes the firmware target, not host instructions.

| Chip | Constraint | Architecture | Physical cores | Arduino variant |
| --- | --- | --- | --- | --- |
| ESP32 | `:esp32` | Xtensa | 2 | `esp32` |
| ESP32-S3 | `:esp32s3` | Xtensa | 2 | `esp32s3` |
| ESP32-C3 | `:esp32c3` | RISC-V | 1 | `esp32c3` |
| RP2040 | `:rp2040` | Arm (firmware only) | 2 | Not exposed yet |

For each ESP32-family chip, select `--config=roo_testing_arduino_<soc>` or
`--config=roo_testing_idf_<soc>`. Import the matching `base.bazelrc` and frontend
fragment from `.roo_testing/bazelrc/<soc>/` into the root workspace first.
Classic ESP32 remains the wrapper default. `test_all_profiles` tests all six
combinations sequentially and stops at the first failure.

The profiles publish identity macros globally to C and C++ dependencies.
`//roo_testing/soc:identity` validates framework-independent chip macros and
exports `roo_testing/soc_profile.h`, including `Model::kEsp32`, `Model::kEsp32S3`, and
`Model::kEsp32C3`. `//roo_testing/soc:target` preserves the concrete ESP-IDF
contract selection for the six supported profiles. Platform guards and compile
contracts reject mismatched, unsupported, and mixed selections.

Each Arduino frontend uses the generic Dev Module variant. USB boot features
remain disabled; console I/O uses UART0 and that variant's default TX/RX pins.
The emulator shares SoC traits across GPIO, matrix routing, UART, SPI, I2C, ADC,
LEDC, and chip-information shims. Missing GPIOs and controllers are rejected.
C3 has no SDMMC host; SDSPI and host filesystem facilities remain available.

The six ESP32 profiles retain the single-core pthread-backed FreeRTOS scheduler.
The Linux backend identity is private to its own translation units. Memory,
analog conversion, clocks, and peripheral timing retain the emulator's existing
approximations. This support does not add USB, Bluetooth radio, or other new
peripheral implementations merely because their hardware capability macros exist.

The generic header requires only the selected `ROO_TESTING_SOC` name and one
supported `ROO_TESTING_SOC_*` value. Its framework-independent contract checks
that these agree with the platform. ESP-IDF identity checks and `kIdfTarget` /
`kIdfArchitecture` live in `roo_testing/esp_idf_soc_profile.h`, included by
`soc_profile.h` only when `ESP_PLATFORM` is defined. Existing ESP-IDF consumers
retain the constants through their original include and `:target` dependency.
Generic identity alone grants neither ESP-IDF capability nor a runnable backend.

RP2040 uses the SDK-only `roo_testing_pico_rp2040` profile. `Model::kRp2040`
identifies the physical chip while the compiler retains native host architecture.
For RP2040, `:target` selects the generic `:identity` contract and does not expose
IDF constants. The initial Pico
profile has a bare-metal runtime.
