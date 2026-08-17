#include "test/profile/global_environment_contract.h"

extern "C" const char* roo_testing_unrelated_cpp_idf_version() {
  return IDF_VER;
}
extern "C" const char* roo_testing_unrelated_cpp_idf_target() {
  return CONFIG_IDF_TARGET;
}
extern "C" const char* roo_testing_unrelated_cpp_idf_arch() {
  return CONFIG_IDF_TARGET_ARCH;
}
extern "C" const char* roo_testing_unrelated_cpp_soc() {
  return ROO_TESTING_SOC;
}
extern "C" const char* roo_testing_unrelated_cpp_board() {
  return ARDUINO_BOARD;
}
extern "C" const char* roo_testing_unrelated_cpp_variant() {
  return ARDUINO_VARIANT;
}
