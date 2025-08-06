
#include "esp_err.h"
#include "esp_log.h"

#include "roo_testing/devices/microcontroller/esp32/fake_esp32.h"

extern "C" {

uint32_t esp_log_timestamp(void) {
    return FakeEsp32().time().getTimeMicros() / 1000;
}

uint32_t esp_log_early_timestamp(void) {
    return FakeEsp32().time().getTimeMicros() / 1000;
}

}
