# Emulated SoC profiles

The target platform is the source of truth for which SoC roo_testing emulates.
The currently supported platform carries
`//roo_testing/platforms:esp32`, representing the classic dual-core Xtensa
ESP32 and Arduino's generic **ESP32 Dev Module** variant.

The shared base used by `roo_testing_arduino_esp32` and
`roo_testing_idf_esp32` supplies
`CONFIG_IDF_TARGET_ESP32`, `CONFIG_IDF_TARGET`,
`CONFIG_IDF_TARGET_ARCH_XTENSA`, `ESP32`, `ROO_TESTING_SOC`, and the other
identity macros globally. `//roo_testing/soc:target` supplies the typed
`roo_testing/soc_profile.h` API and aliases the concrete `:esp32` behavioral
profile. Its analysis and compile guards ensure the selected constraint and
macro values agree; it no longer exports `defines`.

Linux is the execution host, not the emulated ESP-IDF target. The platform
inherits `@platforms//host:host` for native CPU and OS constraints, while its
roo_testing constraints describe the software environment and emulated SoC.
Public sources do not see `CONFIG_IDF_TARGET_LINUX`; the pthread-backed
FreeRTOS implementation receives its private backend definition only for its
own sources.

## Adding another SoC

Support can be added incrementally without conflating host and emulated
architecture:

1. add a new value of `//roo_testing/platforms:soc` and a host-parented
   platform containing it;
2. add a shared SoC base and named fragment under
   `bazelrc/<soc>/<frontend>.bazelrc`, keeping compiler flags out of the
   platform `flags` attribute;
3. add a concrete `//roo_testing/soc` target with typed identity and guards,
   then select it from the stable `:target` label;
4. add matching ESP-IDF SoC headers, Arduino board/variant/pins, shims, and
   `FakeEsp32` peripheral behavior;
5. add global C/C++ macro, constraint-`select()`, and behavioral tests for the
   new profile.

Do not advertise a family merely to enable its conditional source. At present,
`FakeEsp32` implements classic ESP32 behavior; ESP32-C3, ESP32-S3, and other
families remain intentionally unsupported.
