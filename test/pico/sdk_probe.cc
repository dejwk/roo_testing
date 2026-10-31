#include <cstdio>
#include <string_view>

#include "pico/time.h"
#include "roo_testing/soc_profile.h"
#include "test/pico/profile_contract.h"

extern "C" int PicoUnrelatedC(void);

static_assert(PICO_SDK_VERSION_MAJOR == 2);
static_assert(PICO_SDK_VERSION_MINOR == 3);
static_assert(PICO_SDK_VERSION_REVISION == 1);
static_assert(std::string_view(PICO_SDK_VERSION_STRING) == "2.3.1");
static_assert(std::string_view(PICO_PLATFORM_STRING) == "host");
static_assert(std::string_view(PICO_BOARD) == "pico");
static_assert(roo_testing::soc::kModel == roo_testing::soc::Model::kRp2040);
static_assert(std::string_view(roo_testing::soc::kName) == "rp2040");
static_assert(NUM_BANK0_GPIOS == 30);

// Verifies C++ consumers share the C implementation and globally selected
// target identity. Explicit unsupported-alarm mode is checked by the
// integration script.
int main(int argc, char** argv) {
  if (argc == 2 && std::string_view(argv[1]) == "unsupported-alarm") {
    hardware_alarm_claim(0);
    return 1;
  }
  const uint64_t before = time_us_64();
  busy_wait_us_32(1000);
  const uint64_t after = time_us_64();
  if (after < before || PicoUnrelatedC() != 1) return 2;
  std::printf("Pico C++ probe passed: %s\n", roo_testing::soc::kName);
  return 0;
}
