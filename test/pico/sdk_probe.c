#include <inttypes.h>
#include <stdio.h>

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "pico/time.h"
#include "test/pico/profile_contract.h"

// Verifies C SDK headers, libc stdio, both uptime widths, and shared host
// clocks compile and link together without IDF or target assembly.
int main(void) {
  const uint64_t before = time_us_64();
  const uint32_t low = time_us_32();
  const uint64_t after = to_us_since_boot(get_absolute_time());
  if (after < before || after - before >= UINT32_MAX) return 1;
  if ((uint32_t)(low - (uint32_t)before) > after - before) return 2;
  if (get_core_num() != 0) return 3;
  if (hardware_alarm_claim_unused(false) != -1) return 4;
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_put(PICO_DEFAULT_LED_PIN, 1);
  if (gpio_get(PICO_DEFAULT_LED_PIN)) return 5;
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  if (!gpio_get(PICO_DEFAULT_LED_PIN)) return 6;
  gpio_xor_mask64(UINT64_C(1) << PICO_DEFAULT_LED_PIN);
  if (gpio_get_out_level(PICO_DEFAULT_LED_PIN)) return 7;
  gpio_deinit(PICO_DEFAULT_LED_PIN);
  if (uart0 == NULL || uart1 == NULL || uart0 == uart1) return 8;
  if (uart_get_instance(1) != uart1 || uart_get_index(uart0) != 0) return 9;
  if (uart_init(uart0, 115200) != 115207) return 10;
  if (!uart_is_enabled(uart0)) return 11;
  uart_set_format(uart0, 8, 1, UART_PARITY_NONE);
  uart_deinit(uart0);
  if (uart_is_enabled(uart0)) return 12;
  if (spi0 == NULL || spi1 == NULL || spi0 == spi1) return 13;
  if (spi_get_index(spi_get_instance(1)) != 1) return 14;
  if (spi_init(spi0, 1000000) != 992063) return 15;
  spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  if (spi_write_blocking(spi0, NULL, 0) != 0) return 16;
  spi_deinit(spi0);
  printf("Pico SDK %s host probe: uptime=%" PRIu64 " us, core=0\n",
         PICO_SDK_VERSION_STRING, after);
  return 0;
}
