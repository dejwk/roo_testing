#include "examples/pico_gpio/blink.h"

#include "hardware/gpio.h"
#include "pico/time.h"

void RunPicoBlink() {
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  for (int cycle = 0; cycle < 3; ++cycle) {
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    sleep_ms(1);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    sleep_ms(1);
  }
}
