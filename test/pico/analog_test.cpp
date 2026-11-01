#include <cmath>
#include <limits>

#include "gtest/gtest.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/time.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

namespace {
using roo_testing_transducers::ConstVoltage;
using roo_testing_transducers::SimpleVoltageSink;

/// Owns borrowed ADC sources and PWM observers for process lifetime.
struct Wiring {
  Wiring() {
    for (uint i = 0; i < 3; ++i) {
      FakePico().attachInput(26 + i, sources[i]);
    }
    FakePico().attachInput(29, timed_source);
    FakePico().attachOutput(0, a);
    FakePico().attachOutput(1, b);
    FakePico().attachOutput(16, alias);
  }
  ConstVoltage sources[4] = {0.0f, 1.65f, 3.3f, 4.0f};
  std::optional<roo_testing_transducers::VoltageSignal> timed;
  roo_testing_transducers::SimpleVoltageSource timed_source{
      "timed ADC", [this] {
        return timed.has_value() ? timed->voltageAtUptimeMicros(time_us_64())
                                 : sources[3].read();
      }};
  SimpleVoltageSink a{"A"};
  SimpleVoltageSink b{"B"};
  SimpleVoltageSink alias{"alias"};
};
Wiring& Wires() {
  static Wiring wires;
  return wires;
}
class PicoAnalogTest : public testing::Test {
 protected:
  void SetUp() override {
    Wires().timed.reset();
    adc_init();
    for (uint pin = 26; pin < 30; ++pin) adc_gpio_init(pin);
    const pwm_config config = pwm_get_default_config();
    pwm_init(0, &config, false);
    for (uint pin : {0u, 1u, 16u}) gpio_set_function(pin, GPIO_FUNC_PWM);
  }
};

// Verifies the four external ADC channels, 12-bit floor/clamp behavior, pin
// digital isolation, and explicit rejection of floating/internal/FIFO modes.
TEST_F(PicoAnalogTest, ExternalAdcClampsAndSelectsChannels) {
  const uint16_t expected[] = {0, 2048, 4095, 4095};
  const uint64_t before = time_us_64();
  for (uint i = 0; i < 4; ++i) {
    adc_select_input(i);
    EXPECT_EQ(adc_get_selected_input(), i);
    EXPECT_EQ(adc_read(), expected[i]);
    EXPECT_FALSE(FakePico().state(26 + i).input_enabled);
    EXPECT_FALSE(FakePico().state(26 + i).pull_down);
  }
  Wires().sources[0].set(-1.0f);
  adc_select_input(0);
  EXPECT_EQ(adc_read(), 0);
  Wires().sources[0].set(std::numeric_limits<float>::quiet_NaN());
  EXPECT_DEATH(adc_read(), "ADC voltage");
  Wires().sources[0].set(0.0f);
  EXPECT_DEATH(adc_select_input(4), "internal channel");
  EXPECT_DEATH(adc_gpio_init(25), "ADC GPIO");
  EXPECT_DEATH(adc_set_temp_sensor_enabled(false), "temperature sensor");
  EXPECT_DEATH(adc_run(false), "continuous conversion");
  EXPECT_EQ(time_us_64(), before);
}

// Verifies ADC samples the same time-indexed voltage source as shared GPIO/PWM.
TEST_F(PicoAnalogTest, AdcSamplesSharedTimeVoltage) {
  Wires().timed = roo_testing_transducers::VoltageSignal::Triangle(
      0, 3.3f, 1000, time_us_64());
  adc_select_input(3);
  EXPECT_EQ(adc_read(), 0);
  sleep_us(250);
  EXPECT_EQ(adc_read(), 2048);
  sleep_us(250);
  EXPECT_EQ(adc_read(), 4095);
}

// Verifies shared A/B slice period, opposite duty levels, GPIO aliases, and
// retained periodic voltage samples against the emulator's manual clock.
TEST_F(PicoAnalogTest, PwmPublishesSharedSliceWaveforms) {
  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv(&config, 125.0f);
  pwm_config_set_wrap(&config, 999);
  pwm_init(0, &config, true);
  pwm_set_both_levels(0, 250, 750);
  const int64_t origin = FakePico().pwmState(0).origin_us;
  EXPECT_EQ(pwm_gpio_to_slice_num(16), 0u);
  EXPECT_EQ(pwm_gpio_to_channel(1), 1u);
  EXPECT_FLOAT_EQ(Wires().a.sampleAtUptimeMicros(origin + 100), 3.3f);
  EXPECT_FLOAT_EQ(Wires().a.sampleAtUptimeMicros(origin + 500), 0.0f);
  EXPECT_FLOAT_EQ(Wires().b.sampleAtUptimeMicros(origin + 500), 3.3f);
  EXPECT_FLOAT_EQ(Wires().b.sampleAtUptimeMicros(origin + 900), 0.0f);
  EXPECT_EQ(Wires().a.signal(), Wires().alias.signal());
  sleep_us(500);
  EXPECT_EQ(pwm_get_counter(0), 500);
  EXPECT_FALSE(gpio_get(0));
  EXPECT_TRUE(gpio_get(1));
  pwm_set_enabled(0, false);
  const uint16_t stopped = pwm_get_counter(0);
  sleep_us(1000);
  EXPECT_EQ(pwm_get_counter(0), stopped);
  EXPECT_FLOAT_EQ(Wires().a.sample(), 0.0f);
  EXPECT_FLOAT_EQ(Wires().b.sample(), 3.3f);
  pwm_set_enabled(0, true);
  pwm_set_counter(0, 0);
  EXPECT_TRUE(gpio_get(0));
}

// Verifies compare saturation, polarity, divider quantization, slice/pin
// bounds, unsupported modes, and source/output conflicts before state
// publication.
TEST_F(PicoAnalogTest, PwmBoundariesAndUnsupportedModes) {
  pwm_set_wrap(0, 99);
  pwm_set_clkdiv(0, 125.1f);
  EXPECT_EQ(FakePico().pwmState(0).divider16, 2001);
  pwm_set_both_levels(0, 0, 100);
  pwm_set_enabled(0, true);
  EXPECT_FALSE(gpio_get(0));
  EXPECT_TRUE(gpio_get(1));
  pwm_set_output_polarity(0, true, true);
  EXPECT_TRUE(gpio_get(0));
  EXPECT_FALSE(gpio_get(1));
  EXPECT_DEATH(pwm_set_clkdiv(0, 0.5f), "divider");
  EXPECT_DEATH(pwm_set_chan_level(0, 2, 0), "channel");
  EXPECT_DEATH(pwm_set_wrap(8, 100), "slice");
  EXPECT_DEATH(pwm_gpio_to_slice_num(30), "GPIO pin");
  EXPECT_DEATH(pwm_set_phase_correct(0, true), "phase correct");
  EXPECT_DEATH(pwm_set_clkdiv_mode(0, PWM_DIV_B_HIGH), "gated divider");
  EXPECT_DEATH(pwm_set_irq_enabled(0, false), "interrupts");
  EXPECT_DEATH(pwm_set_mask_enabled(256), "mask");
  EXPECT_DEATH(gpio_set_function(26, GPIO_FUNC_PWM), "attached input source");
}
}  // namespace
