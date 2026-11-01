#include <cstdio>

#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

// A bounded external ADC -> shared PWM signal example.
int main() {
  roo_testing_transducers::ConstVoltage input(1.65f);
  roo_testing_transducers::SimpleVoltageSink output("PWM");
  FakePico().attachInput(26, input);
  FakePico().attachOutput(0, output);
  adc_init();
  adc_gpio_init(26);
  adc_select_input(0);
  const uint16_t sample = adc_read();
  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv(&config, 125);
  pwm_config_set_wrap(&config, 999);
  pwm_init(0, &config, false);
  pwm_set_gpio_level(0, sample * 1000u / 4096);
  gpio_set_function(0, GPIO_FUNC_PWM);
  pwm_set_enabled(0, true);
  const int64_t origin = FakePico().pwmState(0).origin_us;
  if (sample != 2048 || output.sampleAtUptimeMicros(origin + 250) < 3.0f ||
      output.sampleAtUptimeMicros(origin + 750) > 0.1f)
    return 1;
  std::puts("Pico ADC/PWM: 1.65 V -> 2048 -> 1 kHz, 50% duty verified");
  return 0;
}
