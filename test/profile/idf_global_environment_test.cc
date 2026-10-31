#include <cstring>

#include "test/profile/esp32_idf_environment_contract.h"

extern "C" const char* roo_testing_idf_unrelated_c_version(void);
extern "C" const char* roo_testing_idf_unrelated_c_target(void);
extern "C" const char* roo_testing_idf_unrelated_c_arch(void);
extern "C" const char* roo_testing_idf_unrelated_c_soc(void);

extern "C" const char* roo_testing_idf_unrelated_cpp_version();
extern "C" const char* roo_testing_idf_unrelated_cpp_target();
extern "C" const char* roo_testing_idf_unrelated_cpp_arch();
extern "C" const char* roo_testing_idf_unrelated_cpp_soc();

namespace {

bool Is(const char* actual, const char* expected) {
  return std::strcmp(actual, expected) == 0;
}

}  // namespace

int main() {
  const bool c_environment_is_exact =
      Is(roo_testing_idf_unrelated_c_version(), "v6.0.2") &&
      Is(roo_testing_idf_unrelated_c_target(), ROO_EXPECTED_SOC) &&
      Is(roo_testing_idf_unrelated_c_arch(), ROO_EXPECTED_ARCH) &&
      Is(roo_testing_idf_unrelated_c_soc(), ROO_EXPECTED_SOC);
  const bool cpp_environment_is_exact =
      Is(roo_testing_idf_unrelated_cpp_version(), "v6.0.2") &&
      Is(roo_testing_idf_unrelated_cpp_target(), ROO_EXPECTED_SOC) &&
      Is(roo_testing_idf_unrelated_cpp_arch(), ROO_EXPECTED_ARCH) &&
      Is(roo_testing_idf_unrelated_cpp_soc(), ROO_EXPECTED_SOC);
  return c_environment_is_exact && cpp_environment_is_exact ? 0 : 1;
}
