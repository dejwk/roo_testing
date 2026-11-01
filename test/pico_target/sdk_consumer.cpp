#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

#if !PICO_ON_DEVICE || !PICO_RP2040 || defined(ROO_TESTING)
#error Real-target compilation requires the RP2040 device SDK
#endif

// Verifies the supported native API signatures also compile and link for ARM.
int main() {
  gpio_init(25);
  gpio_set_dir(25, GPIO_OUT);
  gpio_put(25, true);
  sleep_ms(1);
  uart_init(uart0, 115200);
  gpio_set_function(0, GPIO_FUNC_UART);
  gpio_set_function(1, GPIO_FUNC_UART);
  uart_puts(uart0, "Pico target compile\n");
  i2c_init(i2c0, 400000);
  gpio_set_function(4, GPIO_FUNC_I2C);
  gpio_set_function(5, GPIO_FUNC_I2C);
  const uint8_t reg = 0x11;
  uint8_t temperature[2];
  i2c_write_blocking(i2c0, 0x68, &reg, 1, true);
  i2c_read_blocking(i2c0, 0x68, temperature, 2, false);
  spi_init(spi0, 1000000);
  gpio_set_function(18, GPIO_FUNC_SPI);
  gpio_set_function(19, GPIO_FUNC_SPI);
  gpio_set_function(16, GPIO_FUNC_SPI);
  uint8_t received;
  spi_write_read_blocking(spi0, &reg, &received, 1);
  adc_init();
  adc_gpio_init(26);
  adc_select_input(0);
  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv(&config, 125);
  pwm_config_set_wrap(&config, 999);
  pwm_init(3, &config, false);
  gpio_set_function(22, GPIO_FUNC_PWM);
  pwm_set_gpio_level(22, adc_read() * 1000u / 4096u);
  pwm_set_enabled(3, true);
  return 0;
}
