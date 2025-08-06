#include <stdarg.h> /* va_list, va_start, va_arg, va_end */
#include <stdio.h>  /* printf */

#include "rom/ets_sys.h"

extern "C" {

void ets_delay_us(uint32_t us) {}

void ets_install_putc1(void (*p)(char c)) {}

int ets_printf(const char *fmt, ...) {
  va_list arglist;
  va_start(arglist, fmt);
  vprintf(fmt, arglist);
  va_end(arglist);
  return 0;
}

}  // extern "C"
