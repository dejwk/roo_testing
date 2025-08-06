#include "hal/gpio_types.h"
#include "roo_testing/devices/microcontroller/esp32/fake_esp32.h"

extern "C" {

void gpio_ll_set_level(gpio_dev_t *hw, gpio_num_t gpio_num, uint32_t level) {
  FakeEsp32().gpio.get(gpio_num).digitalWrite(
      level ? roo_testing_transducers::kDigitalHigh
            : roo_testing_transducers::kDigitalLow);
}

int gpio_ll_get_level(gpio_dev_t *hw, gpio_num_t gpio_num) {
  if (gpio_num == 0x30) return 0;
  if (gpio_num == 0x38) return 1;
  return FakeEsp32().gpio.get(gpio_num).read() > 1.7;
}
}
