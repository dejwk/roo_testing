#include "rom/ets_sys.h"

extern "C" {

void ets_delay_us(uint32_t us) {}

void ets_install_putc1(void (*p)(char c)) {}

int ets_printf(const char *fmt, ...) { return 0; }

}  // extern "C"
