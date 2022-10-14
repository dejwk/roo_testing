#include "esp_rom_gpio.h"

extern "C" {

void esp_rom_gpio_connect_in_signal(uint32_t gpio_num, uint32_t signal_idx,
                                    bool inv) {}

void esp_rom_gpio_connect_out_signal(uint32_t gpio_num, uint32_t signal_idx,
                                     bool out_inv, bool oen_inv) {}
}
