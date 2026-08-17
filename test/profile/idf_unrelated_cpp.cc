#include "test/profile/idf_environment_contract.h"

extern "C" const char* roo_testing_idf_unrelated_cpp_version() {
  return IDF_VER;
}
extern "C" const char* roo_testing_idf_unrelated_cpp_target() {
  return CONFIG_IDF_TARGET;
}
extern "C" const char* roo_testing_idf_unrelated_cpp_arch() {
  return CONFIG_IDF_TARGET_ARCH;
}
extern "C" const char* roo_testing_idf_unrelated_cpp_soc() {
  return ROO_TESTING_SOC;
}
