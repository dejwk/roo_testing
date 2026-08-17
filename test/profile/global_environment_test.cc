#include <cstring>

extern "C" const char* roo_testing_unrelated_c_idf_version(void);
extern "C" const char* roo_testing_unrelated_c_idf_target(void);
extern "C" const char* roo_testing_unrelated_c_idf_arch(void);
extern "C" const char* roo_testing_unrelated_c_soc(void);
extern "C" const char* roo_testing_unrelated_c_board(void);
extern "C" const char* roo_testing_unrelated_c_variant(void);

extern "C" const char* roo_testing_unrelated_cpp_idf_version();
extern "C" const char* roo_testing_unrelated_cpp_idf_target();
extern "C" const char* roo_testing_unrelated_cpp_idf_arch();
extern "C" const char* roo_testing_unrelated_cpp_soc();
extern "C" const char* roo_testing_unrelated_cpp_board();
extern "C" const char* roo_testing_unrelated_cpp_variant();

namespace {

bool Is(const char* actual, const char* expected) {
  return std::strcmp(actual, expected) == 0;
}

}  // namespace

int main() {
  const bool c_environment_is_exact =
      Is(roo_testing_unrelated_c_idf_version(), "v6.0.2") &&
      Is(roo_testing_unrelated_c_idf_target(), "esp32") &&
      Is(roo_testing_unrelated_c_idf_arch(), "xtensa") &&
      Is(roo_testing_unrelated_c_soc(), "esp32") &&
      Is(roo_testing_unrelated_c_board(), "ESP32_DEV") &&
      Is(roo_testing_unrelated_c_variant(), "esp32");
  const bool cpp_environment_is_exact =
      Is(roo_testing_unrelated_cpp_idf_version(), "v6.0.2") &&
      Is(roo_testing_unrelated_cpp_idf_target(), "esp32") &&
      Is(roo_testing_unrelated_cpp_idf_arch(), "xtensa") &&
      Is(roo_testing_unrelated_cpp_soc(), "esp32") &&
      Is(roo_testing_unrelated_cpp_board(), "ESP32_DEV") &&
      Is(roo_testing_unrelated_cpp_variant(), "esp32");
  return c_environment_is_exact && cpp_environment_is_exact ? 0 : 1;
}
