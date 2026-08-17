#include "test/profile/global_environment_contract.h"

const char* roo_testing_unrelated_c_idf_version(void) { return IDF_VER; }
const char* roo_testing_unrelated_c_idf_target(void) {
  return CONFIG_IDF_TARGET;
}
const char* roo_testing_unrelated_c_idf_arch(void) {
  return CONFIG_IDF_TARGET_ARCH;
}
const char* roo_testing_unrelated_c_soc(void) { return ROO_TESTING_SOC; }
const char* roo_testing_unrelated_c_board(void) { return ARDUINO_BOARD; }
const char* roo_testing_unrelated_c_variant(void) { return ARDUINO_VARIANT; }
