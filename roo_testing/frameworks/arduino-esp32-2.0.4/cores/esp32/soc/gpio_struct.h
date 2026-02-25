// C++ emulation shim for soc/gpio_struct.h.
//
// Why this file exists:
// - Test code and libraries often include <soc/gpio_struct.h> and write
//   GPIO.out_w1ts / GPIO.out_w1tc directly.
// - In emulation, those writes must be intercepted so they drive FakeEsp32 GPIO.
// - The interception is implemented by custom C++ field types in
//   fake_esp32_soc_gpio_struct.h (operator= hooks).
//
// Why this is C++-only:
// - The fake struct relies on C++ classes/operators and cannot be included by C.
// - For C translation units we must forward to the original ESP-IDF header.
//
// Include order note:
// - This shim must appear earlier on include paths than ESP-IDF's
//   components/soc/esp32/include/soc/gpio_struct.h.
#ifdef __cplusplus
#ifndef _SOC_GPIO_STRUCT_H_
#define _SOC_GPIO_STRUCT_H_

#include "roo_testing/microcontrollers/esp32/fake_esp32_soc_gpio_struct.h"

#endif  // _SOC_GPIO_STRUCT_H_
#else
#include_next "soc/gpio_struct.h"
#endif
