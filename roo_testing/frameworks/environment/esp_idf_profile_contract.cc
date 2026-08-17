// Compile guard for the ESP-IDF layer. SoC identity is validated independently
// by //roo_testing/soc so a future IDF profile can select another supported SoC.

#ifndef ESP_PLATFORM
#error "The selected ESP-IDF platform requires the roo_testing compiler profile"
#endif
#if ESP_PLATFORM != 1
#error "ESP_PLATFORM has an unsupported value"
#endif
#ifndef IDF_VER
#error "The roo_testing compiler profile must define IDF_VER"
#endif

namespace {

constexpr bool StringEquals(const char* lhs, const char* rhs) {
  while (*lhs != '\0' && *lhs == *rhs) {
    ++lhs;
    ++rhs;
  }
  return *lhs == *rhs;
}

static_assert(StringEquals(IDF_VER, "v6.0.2"),
              "IDF_VER does not match the imported ESP-IDF sources");

}  // namespace

int roo_testing_esp_idf_profile_contract() { return 0; }
