#include "gpio.h"

#include "glog/logging.h"
#include "roo_testing/boards/esp32/fake_esp32.h"

extern "C" {

void gpio_matrix_in(uint32_t gpio, uint32_t signal_idx, bool inv) {
  FakeEsp32().in_matrix.assign(gpio, signal_idx, inv);
}

void gpio_matrix_out(uint32_t gpio, uint32_t signal_idx, bool out_inv,
                     bool oen_inv) {
  FakeEsp32().out_matrix.assign(gpio, signal_idx, out_inv, oen_inv);
}
}
