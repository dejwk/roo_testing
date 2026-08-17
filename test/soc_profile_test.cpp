#include "gtest/gtest.h"
#include "roo_testing/soc_profile.h"
#include "sdkconfig.h"
#include "soc/soc_caps.h"

#ifndef ROO_TESTING_SOC_ESP32
#error "roo_testing must publish its concrete emulated SoC"
#endif

#ifndef CONFIG_IDF_TARGET_ESP32
#error "ESP-IDF consumers must see the classic ESP32 target"
#endif

#ifndef CONFIG_IDF_TARGET_ARCH_XTENSA
#error "Classic ESP32 must select the Xtensa ESP-IDF architecture"
#endif

#ifndef ESP32
#error "Framework consumers must see the canonical ESP32 platform macro"
#endif

#if defined(CONFIG_IDF_TARGET_ESP32C2) || defined(CONFIG_IDF_TARGET_ESP32C3) || \
    defined(CONFIG_IDF_TARGET_ESP32C5) || defined(CONFIG_IDF_TARGET_ESP32C6) || \
    defined(CONFIG_IDF_TARGET_ESP32C61) || defined(CONFIG_IDF_TARGET_ESP32H2) || \
    defined(CONFIG_IDF_TARGET_ESP32H21) || defined(CONFIG_IDF_TARGET_ESP32H4) || \
    defined(CONFIG_IDF_TARGET_ESP32P4) || defined(CONFIG_IDF_TARGET_ESP32S2) || \
    defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_LINUX) || \
    defined(CONFIG_IDF_TARGET_ARCH_RISCV) ||                                \
    defined(ROO_TESTING_FREERTOS_POSIX_BACKEND)
#error "Only one SoC and architecture may be selected"
#endif

static_assert(CONFIG_IDF_TARGET_ESP32 == 1);
static_assert(CONFIG_IDF_TARGET_ARCH_XTENSA == 1);
static_assert(ESP32 == 1);
static_assert(CONFIG_IDF_FIRMWARE_CHIP_ID == 0x0000);
static_assert(CONFIG_SOC_CPU_CORES_NUM == 2);
static_assert(SOC_CPU_CORES_NUM == 2);
static_assert(roo_testing::soc::kModel == roo_testing::soc::Model::kEsp32);

TEST(SocProfileTest, PublishesConsistentFrameworkIdentity) {
  EXPECT_STREQ(ROO_TESTING_SOC, "esp32");
  EXPECT_STREQ(CONFIG_IDF_TARGET, "esp32");
  EXPECT_STREQ(CONFIG_IDF_TARGET_ARCH, "xtensa");
  EXPECT_STREQ(roo_testing::soc::kName, ROO_TESTING_SOC);
  EXPECT_STREQ(roo_testing::soc::kIdfTarget, CONFIG_IDF_TARGET);
  EXPECT_STREQ(roo_testing::soc::kIdfArchitecture,
               CONFIG_IDF_TARGET_ARCH);
}
