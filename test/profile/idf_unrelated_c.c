#include "test/profile/idf_environment_contract.h"

const char* roo_testing_idf_unrelated_c_version(void) { return IDF_VER; }
const char* roo_testing_idf_unrelated_c_target(void) {
  return CONFIG_IDF_TARGET;
}
const char* roo_testing_idf_unrelated_c_arch(void) {
  return CONFIG_IDF_TARGET_ARCH;
}
const char* roo_testing_idf_unrelated_c_soc(void) { return ROO_TESTING_SOC; }
