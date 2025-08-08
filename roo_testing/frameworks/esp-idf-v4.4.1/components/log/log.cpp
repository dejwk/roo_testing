
#include "esp_err.h"
#include "esp_log.h"

#include "roo_testing/system/timer.h"

extern "C" {

uint32_t esp_log_timestamp(void) {
    return system_time_get_micros() / 1000;
}

uint32_t esp_log_early_timestamp(void) {
    return system_time_get_micros() / 1000;
}

}
