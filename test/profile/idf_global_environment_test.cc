#include <cstring>

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
      Is(roo_testing_idf_unrelated_c_target(), "esp32") &&
      Is(roo_testing_idf_unrelated_c_arch(), "xtensa") &&
      Is(roo_testing_idf_unrelated_c_soc(), "esp32");
  const bool cpp_environment_is_exact =
      Is(roo_testing_idf_unrelated_cpp_version(), "v6.0.2") &&
      Is(roo_testing_idf_unrelated_cpp_target(), "esp32") &&
      Is(roo_testing_idf_unrelated_cpp_arch(), "xtensa") &&
      Is(roo_testing_idf_unrelated_cpp_soc(), "esp32");
  return c_environment_is_exact && cpp_environment_is_exact ? 0 : 1;
}
