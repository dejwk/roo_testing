#include <unistd.h>

#include <cstdlib>

#include "FreeRTOS.h"
#include "task.h"

extern "C" {
void vApplicationIdleHook() { usleep(1000); }

void vApplicationMallocFailedHook() { std::abort(); }
}
