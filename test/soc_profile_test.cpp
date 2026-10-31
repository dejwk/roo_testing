#include "roo_testing/soc_profile.h"

#include "gtest/gtest.h"
#include "sdkconfig.h"
#include "soc/soc_caps.h"
#include "test/idf_environment_contract.h"

static_assert(CONFIG_SOC_CPU_CORES_NUM == ROO_EXPECTED_CORES);
static_assert(SOC_CPU_CORES_NUM == ROO_EXPECTED_CORES);
#if CONFIG_IDF_TARGET_ESP32S3
static_assert(roo_testing::soc::kModel == roo_testing::soc::Model::kEsp32S3);
#elif CONFIG_IDF_TARGET_ESP32C3
static_assert(roo_testing::soc::kModel == roo_testing::soc::Model::kEsp32C3);
#else
static_assert(roo_testing::soc::kModel == roo_testing::soc::Model::kEsp32);
#endif

// Verifies typed identity agrees with the global framework profile.
TEST(SocProfileTest, PublishesConsistentFrameworkIdentity) {
  EXPECT_STREQ(roo_testing::soc::kName, ROO_EXPECTED_SOC);
  EXPECT_STREQ(roo_testing::soc::kIdfTarget, ROO_EXPECTED_SOC);
  EXPECT_STREQ(roo_testing::soc::kIdfArchitecture, ROO_EXPECTED_ARCH);
}
