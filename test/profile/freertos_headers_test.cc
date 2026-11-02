#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static_assert(sizeof(TickType_t) >= 4);

// Verifies public kernel headers link without a scheduler or application hooks.
int main() { return configTICK_RATE_HZ > 0 ? 0 : 1; }
