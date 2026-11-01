#ifndef ROO_TESTING_PICO_HARDWARE_CLOCKS_H_
#define ROO_TESTING_PICO_HARDWARE_CLOCKS_H_
#include "pico/types.h"
#ifdef __cplusplus
extern "C" {
#endif
/// Modeled clock: the RP2040 system clock fixed at 125 MHz.
typedef enum { clk_sys = 5 } clock_handle_t;
/// Returns the fixed modeled system frequency; other clocks are unsupported.
uint32_t clock_get_hz(clock_handle_t clock);
#ifdef __cplusplus
}
#endif
#endif
