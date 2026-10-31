#pragma once
#include "test/profile/esp32_idf_environment_contract.h"

constexpr bool RooTestingEnvironmentStringEquals(const char* lhs,
                                                 const char* rhs) {
  while (*lhs != '\0' && *lhs == *rhs) {
    ++lhs;
    ++rhs;
  }
  return *lhs == *rhs;
}
static_assert(RooTestingEnvironmentStringEquals(IDF_VER, "v6.0.2"));
static_assert(RooTestingEnvironmentStringEquals(CONFIG_IDF_TARGET,
                                                ROO_EXPECTED_SOC));
static_assert(RooTestingEnvironmentStringEquals(CONFIG_IDF_TARGET_ARCH,
                                                ROO_EXPECTED_ARCH));
static_assert(RooTestingEnvironmentStringEquals(ROO_TESTING_SOC,
                                                ROO_EXPECTED_SOC));
