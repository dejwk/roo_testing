#pragma once

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void system_time_sync();
void system_time_lag_ns(uint64_t ns);
int64_t system_time_get_micros();
void system_time_delay_micros(uint64_t us);

void system_time_set_auto_sync(bool auto_sync);

#ifdef __cplusplus
}
#endif
