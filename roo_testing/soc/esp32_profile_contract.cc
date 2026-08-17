// Compile guard for the classic ESP32 SoC profile.

#ifndef CONFIG_IDF_TARGET
#error "The ESP32 platform requires CONFIG_IDF_TARGET from its compiler profile"
#endif
#ifndef CONFIG_IDF_TARGET_ARCH
#error "The ESP32 platform requires CONFIG_IDF_TARGET_ARCH"
#endif
#ifndef CONFIG_IDF_TARGET_ESP32
#error "The ESP32 platform requires CONFIG_IDF_TARGET_ESP32"
#endif
#if CONFIG_IDF_TARGET_ESP32 != 1
#error "CONFIG_IDF_TARGET_ESP32 has an unsupported value"
#endif
#ifndef CONFIG_IDF_TARGET_ARCH_XTENSA
#error "The ESP32 platform requires CONFIG_IDF_TARGET_ARCH_XTENSA"
#endif
#if CONFIG_IDF_TARGET_ARCH_XTENSA != 1
#error "CONFIG_IDF_TARGET_ARCH_XTENSA has an unsupported value"
#endif
#ifndef CONFIG_IDF_FIRMWARE_CHIP_ID
#error "The ESP32 platform requires CONFIG_IDF_FIRMWARE_CHIP_ID"
#endif
#if CONFIG_IDF_FIRMWARE_CHIP_ID != 0x0000
#error "CONFIG_IDF_FIRMWARE_CHIP_ID has an unsupported value"
#endif
#ifndef ESP32
#error "The ESP32 platform requires the canonical ESP32 macro"
#endif
#if ESP32 != 1
#error "ESP32 has an unsupported value"
#endif
#ifndef ROO_TESTING_SOC
#error "The ESP32 platform requires ROO_TESTING_SOC"
#endif
#ifndef ROO_TESTING_SOC_ESP32
#error "The ESP32 platform requires ROO_TESTING_SOC_ESP32"
#endif
#if ROO_TESTING_SOC_ESP32 != 1
#error "ROO_TESTING_SOC_ESP32 has an unsupported value"
#endif

#if defined(CONFIG_IDF_TARGET_LINUX) || defined(CONFIG_IDF_TARGET_ARCH_RISCV) || \
    defined(CONFIG_IDF_TARGET_ESP32C2) || defined(CONFIG_IDF_TARGET_ESP32C3) || \
    defined(CONFIG_IDF_TARGET_ESP32C5) || defined(CONFIG_IDF_TARGET_ESP32C6) || \
    defined(CONFIG_IDF_TARGET_ESP32C61) || defined(CONFIG_IDF_TARGET_ESP32H2) || \
    defined(CONFIG_IDF_TARGET_ESP32H21) || defined(CONFIG_IDF_TARGET_ESP32H4) || \
    defined(CONFIG_IDF_TARGET_ESP32P4) || defined(CONFIG_IDF_TARGET_ESP32S2) || \
    defined(CONFIG_IDF_TARGET_ESP32S3)
#error "The ESP32 compiler profile contains contradictory target macros"
#endif

namespace {

constexpr bool StringEquals(const char* lhs, const char* rhs) {
  while (*lhs != '\0' && *lhs == *rhs) {
    ++lhs;
    ++rhs;
  }
  return *lhs == *rhs;
}

static_assert(StringEquals(CONFIG_IDF_TARGET, "esp32"),
              "CONFIG_IDF_TARGET does not match the selected SoC");
static_assert(StringEquals(CONFIG_IDF_TARGET_ARCH, "xtensa"),
              "CONFIG_IDF_TARGET_ARCH does not match the selected SoC");
static_assert(StringEquals(ROO_TESTING_SOC, "esp32"),
              "ROO_TESTING_SOC does not match the selected SoC");

}  // namespace

int roo_testing_esp32_profile_contract() { return 0; }
