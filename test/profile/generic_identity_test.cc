#include <string_view>

#include "roo_testing/soc_profile.h"

#if defined(ESP_PLATFORM) || defined(ESP32) || defined(IDF_VER) || \
    defined(CONFIG_IDF_TARGET) || defined(ARDUINO_ARCH_ESP32)
#error "Generic Arduino/SoC identity acquired ESP-IDF or Arduino-ESP32 identity"
#endif
#if !defined(ROO_TESTING_RUNTIME_BARE_METAL) || \
    ROO_TESTING_RUNTIME_BARE_METAL != 1 ||      \
    defined(ROO_TESTING_RUNTIME_FREERTOS)
#error "The generic fixture must retain its bare-metal runtime"
#endif

static_assert(ARDUINO == 10819);
static_assert(roo_testing::soc::kModel == roo_testing::soc::Model::kEsp32);
static_assert(std::string_view(roo_testing::soc::kName) == "esp32");

// Verifies generic Arduino and typed SoC identity compile without IDF
// capability.
int main() { return 0; }
