#include "esp_rom_gpio.h"
#include "roo_testing/devices/microcontroller/esp32/fake_esp32.h"

extern "C" {

void gpio_matrix_in(uint32_t gpio, uint32_t signal_idx, bool inv) {
  FakeEsp32().in_matrix.assign(gpio, signal_idx, inv);
}

void gpio_matrix_out(uint32_t gpio, uint32_t signal_idx, bool out_inv,
                     bool oen_inv) {
  FakeEsp32().out_matrix.assign(gpio, signal_idx, out_inv, oen_inv);
}

void esp_rom_gpio_connect_in_signal(uint32_t gpio_num, uint32_t signal_idx,
                                    bool inv) {
  gpio_matrix_in(gpio_num, signal_idx, inv);
}

void esp_rom_gpio_connect_out_signal(uint32_t gpio_num, uint32_t signal_idx,
                                     bool out_inv, bool oen_inv) {
  gpio_matrix_out(gpio_num, signal_idx, out_inv, oen_inv);
}

void esp_rom_gpio_pad_select_gpio(uint32_t iopad_num) {}

}
