# Emulated SoC profiles

The target platform selects the emulated chip. Linux remains the native execution
host; Xtensa and RISC-V identity describes the firmware target, not host instructions.

| Chip | Constraint | Architecture | Physical cores | Arduino variant |
| --- | --- | --- | --- | --- |
| ESP32 | `:esp32` | Xtensa | 2 | `esp32` |
| ESP32-S3 | `:esp32s3` | Xtensa | 2 | `esp32s3` |
| ESP32-C3 | `:esp32c3` | RISC-V | 1 | `esp32c3` |

For each chip, select `--config=roo_testing_arduino_<soc>` or
`--config=roo_testing_idf_<soc>`. Import the matching `base.bazelrc` and frontend
fragment from `.roo_testing/bazelrc/<soc>/` into the root workspace first.
Classic ESP32 remains the wrapper default. `test_all_profiles` tests all six
combinations sequentially and stops at the first failure.

The profiles publish identity macros globally to C and C++ dependencies.
`//roo_testing/soc:target` selects a concrete contract and exports
`roo_testing/soc_profile.h`, including `Model::kEsp32`, `Model::kEsp32S3`, and
`Model::kEsp32C3`. Platform guards and compile contracts reject mismatched,
unsupported, and mixed selections.

Each Arduino frontend uses the generic Dev Module variant. USB boot features
remain disabled; console I/O uses UART0 and that variant's default TX/RX pins.
The emulator shares SoC traits across GPIO, matrix routing, UART, SPI, I2C, ADC,
LEDC, and chip-information shims. Missing GPIOs and controllers are rejected.
C3 has no SDMMC host; SDSPI and host filesystem facilities remain available.

All profiles retain the single-core pthread-backed FreeRTOS scheduler.
The Linux backend identity is private to its own translation units. Memory,
analog conversion, clocks, and peripheral timing retain the emulator's existing
approximations. This support does not add USB, Bluetooth radio, or other new
peripheral implementations merely because their hardware capability macros exist.
