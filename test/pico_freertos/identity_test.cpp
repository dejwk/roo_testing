#include <cstdint>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" int PicoFreeRtosCIdentity(void);
static_assert(sizeof(TickType_t) == sizeof(uint32_t));
static_assert(configNUMBER_OF_CORES == 1);
static_assert(configTICK_RATE_HZ == 1000);
static_assert(portMAX_DELAY == UINT32_MAX);

// Verifies C and C++ actions share the intended identity and pinned kernel.
int main() {
  return PicoFreeRtosCIdentity() == 1 &&
                 std::strcmp(tskKERNEL_VERSION_NUMBER, "V11.1.0+") == 0
             ? 0
             : 1;
}
