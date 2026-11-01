#include <cstdio>

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "roo_testing/devices/clock/ds3231/ds3231.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

// A bounded SDK application sharing the DS3231 model with ESP32.
int main() {
  FakeDs3231 rtc;
  FakePico().attachI2cDevice(rtc, 4, 5);
  i2c_init(i2c0, 400000);
  gpio_set_function(4, GPIO_FUNC_I2C);
  gpio_set_function(5, GPIO_FUNC_I2C);
  const uint8_t alarm[] = {7, 0x12, 0x34};
  if (i2c_write_blocking(i2c0, 0x68, alarm, 3, false) != 3) return 1;
  const uint8_t reg = 7;
  uint8_t reply[2];
  if (i2c_write_blocking(i2c0, 0x68, &reg, 1, true) != 1) return 2;
  if (i2c_read_blocking(i2c0, 0x68, reply, 2, false) != 2) return 3;
  if (reply[0] != 0x12 || reply[1] != 0x34) return 4;
  const uint8_t temperature = 0x11;
  if (i2c_write_timeout_us(i2c0, 0x68, &temperature, 1, true, 10000) != 1)
    return 5;
  if (i2c_read_timeout_us(i2c0, 0x68, reply, 2, false, 10000) != 2) return 6;
  if (reply[0] != 25 || reply[1] != 0) return 7;
  std::puts("Pico I2C DS3231: repeated START and 25 C verified");
  return 0;
}
