// #include "fake_esp32_soc_gpio.h"

// #include <limits.h>
// #include <unistd.h>

// #include <algorithm>
// #include <cstring>
// #include <random>
// #include <set>
// #include <thread>

// #include "soc/gpio_struct.h"
#include "fake_esp32_soc_gpio.h"

#include <algorithm>

#include "fake_esp32.h"
#include "fake_esp32_soc_gpio_struct.h"

gpio_dev_t GPIO;

uint32_t Esp32GpioOutW1tc::operator=(uint32_t val) const {
  uint32_t result = val;
  for (int i = 0; i < std::min(32, SOC_GPIO_PIN_COUNT); ++i) {
    if ((val & 1) != 0 && roo_testing::soc::IsOutputPin(i))
      FakeEsp32().gpio.get(i).digitalWriteLow();
    val >>= 1;
  }
  return result;
}

uint32_t Esp32GpioOutW1ts::operator=(uint32_t val) const {
  uint32_t result = val;
  for (int i = 0; i < std::min(32, SOC_GPIO_PIN_COUNT); ++i) {
    if ((val & 1) != 0 && roo_testing::soc::IsOutputPin(i))
      FakeEsp32().gpio.get(i).digitalWriteHigh();
    val >>= 1;
  }
  return result;
}

uint32_t Esp32GpioOut1W1tc::operator=(uint32_t val) const {
  uint32_t result = val;
  for (int i = 32; i < SOC_GPIO_PIN_COUNT; ++i) {
    if ((val & 1) != 0 && roo_testing::soc::IsOutputPin(i))
      FakeEsp32().gpio.get(i).digitalWriteLow();
    val >>= 1;
  }
  return result;
}

uint32_t Esp32GpioOut1W1ts::operator=(uint32_t val) const {
  uint32_t result = val;
  for (int i = 32; i < SOC_GPIO_PIN_COUNT; ++i) {
    if ((val & 1) != 0 && roo_testing::soc::IsOutputPin(i))
      FakeEsp32().gpio.get(i).digitalWriteHigh();
    val >>= 1;
  }
  return result;
}

Esp32GpioInReadSpec::operator uint32_t() const {
  uint32_t val = mask;
  uint32_t result = 0;
  for (int i = 0; i < std::min(32, SOC_GPIO_PIN_COUNT); ++i) {
    if ((val & 1) != 0 && roo_testing::soc::IsValidPin(i)) {
      result |= (FakeEsp32().gpio.get(i).read() > 1.7) << i;
    }
    val >>= 1;
  }
  return result >> shift;
}

Esp32GpioIn1ReadSpec::operator uint32_t() const {
  uint32_t val = mask;
  uint32_t result = 0;
  for (int i = 32; i < SOC_GPIO_PIN_COUNT; ++i) {
    if ((val & 1) != 0 && roo_testing::soc::IsValidPin(i)) {
      result |= (FakeEsp32().gpio.get(i).read() > 1.7) << (i - 32);
    }
    val >>= 1;
  }
  return result >> shift;
}

extern "C" {

void gpioFakeWrite(uint8_t pin, float voltage) {
  FakeEsp32().gpio.get(pin).write(voltage);
}

float gpioFakeRead(uint8_t pin) { return FakeEsp32().gpio.get(pin).read(); }
}
