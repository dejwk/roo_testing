// C++ emulation shim for soc/gpio_struct.h.
//
// Libraries often write GPIO.out_w1ts / GPIO.out_w1tc directly. On Linux,
// those writes must be intercepted so they drive FakeEsp32 GPIO state. The
// proxy field types implement that interception with C++ assignment operators.
// C translation units continue to see ESP-IDF's unmodified public structure.
#ifdef __cplusplus
#ifndef _SOC_GPIO_STRUCT_H_
#define _SOC_GPIO_STRUCT_H_

#include "roo_testing/microcontrollers/esp32/fake_esp32_soc_gpio_struct.h"

#endif  // _SOC_GPIO_STRUCT_H_
#else
#include_next "soc/gpio_struct.h"
#endif
