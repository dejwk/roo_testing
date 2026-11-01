#include "hardware/pwm.h"

#include <cmath>

#include "glog/logging.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"
#include "roo_testing/system/timer.h"

namespace {
/// Quantizes a valid divider downward to the SDK's four fractional bits.
uint16_t Divider(float value) {
  CHECK(std::isfinite(value) && value >= 1 && value < 256)
      << "Invalid Pico PWM divider";
  return static_cast<uint16_t>(value * 16);
}
/// Rebases the retained counter before any immediate slice update.
FakePicoBoard::PwmState Current(uint slice) {
  FakePicoBoard::PwmState state = FakePico().pwmState(slice);
  state.counter = FakePico().pwmCounter(slice);
  state.origin_us = system_time_get_micros();
  return state;
}
/// Checks an eight-slice hardware mask before any publication.
void CheckMask(uint32_t mask) {
  CHECK_EQ(mask & ~0xffu, 0u) << "Invalid Pico PWM mask";
}
}  // namespace

extern "C" {
uint pwm_gpio_to_slice_num(uint gpio) {
  FakePicoBoard::CheckPin(gpio);
  return (gpio / 2) % 8;
}
uint pwm_gpio_to_channel(uint gpio) {
  FakePicoBoard::CheckPin(gpio);
  return gpio % 2;
}
pwm_config pwm_get_default_config() { return {0, 16, 65535}; }
void pwm_config_set_clkdiv(pwm_config* config, float div) {
  CHECK(config != nullptr);
  config->div = Divider(div);
}
void pwm_config_set_clkdiv_int_frac4(pwm_config* config, uint32_t integer,
                                     uint8_t fract) {
  CHECK_LT(fract, 16);
  pwm_config_set_clkdiv(config, integer + fract / 16.0f);
}
void pwm_config_set_clkdiv_int(pwm_config* config, uint32_t integer) {
  pwm_config_set_clkdiv_int_frac4(config, integer, 0);
}
void pwm_config_set_clkdiv_int_frac(pwm_config* config, uint8_t integer,
                                    uint8_t fract) {
  pwm_config_set_clkdiv_int_frac4(config, integer, fract);
}
void pwm_config_set_wrap(pwm_config* config, uint16_t wrap) {
  CHECK(config != nullptr);
  config->top = wrap;
}
void pwm_config_set_phase_correct(pwm_config* config, bool phase_correct) {
  CHECK(config != nullptr);
  CHECK(!phase_correct) << "Unimplemented: Pico PWM phase correct";
}
void pwm_config_set_clkdiv_mode(pwm_config* config, pwm_clkdiv_mode mode) {
  CHECK(config != nullptr);
  CHECK(mode == PWM_DIV_FREE_RUNNING)
      << "Unimplemented: Pico PWM gated divider";
}
void pwm_config_set_output_polarity(pwm_config* config, bool a, bool b) {
  CHECK(config != nullptr);
  config->csr = (config->csr & ~12u) | (a ? 4u : 0u) | (b ? 8u : 0u);
}
void pwm_init(uint slice, const pwm_config* config, bool start) {
  CHECK(config != nullptr);
  FakePico().pwmState(slice);
  CHECK_EQ(config->csr & ~12u, 0u) << "Unimplemented: Pico PWM config mode";
  CHECK_LE(config->top, 65535u);
  CHECK(config->div >= 16 && config->div <= 4095) << "Invalid Pico PWM divider";
  FakePicoBoard::PwmState state;
  state.enabled = start;
  state.divider16 = config->div;
  state.wrap = config->top;
  state.inverted = {{(config->csr & 4) != 0, (config->csr & 8) != 0}};
  state.origin_us = system_time_get_micros();
  FakePico().setPwmState(slice, state);
}
void pwm_set_enabled(uint slice, bool enabled) {
  FakePicoBoard::PwmState state = Current(slice);
  state.enabled = enabled;
  FakePico().setPwmState(slice, state);
}
void pwm_set_mask_enabled(uint32_t mask) {
  CheckMask(mask);
  for (uint slice = 0; slice < 8; ++slice) {
    pwm_set_enabled(slice, (mask & (1u << slice)) != 0);
  }
}
void pwm_set_wrap(uint slice, uint16_t wrap) {
  FakePicoBoard::PwmState state = Current(slice);
  state.wrap = wrap;
  state.counter %= uint32_t{wrap} + 1;
  FakePico().setPwmState(slice, state);
}
void pwm_set_clkdiv(uint slice, float divider) {
  FakePicoBoard::PwmState state = Current(slice);
  state.divider16 = Divider(divider);
  FakePico().setPwmState(slice, state);
}
void pwm_set_chan_level(uint slice, uint chan, uint16_t level) {
  CHECK_LT(chan, 2u) << "Invalid Pico PWM channel";
  FakePicoBoard::PwmState state = Current(slice);
  state.levels[chan] = level;
  FakePico().setPwmState(slice, state);
}
void pwm_set_both_levels(uint slice, uint16_t a, uint16_t b) {
  FakePicoBoard::PwmState state = Current(slice);
  state.levels = {{a, b}};
  FakePico().setPwmState(slice, state);
}
void pwm_set_gpio_level(uint gpio, uint16_t level) {
  pwm_set_chan_level(pwm_gpio_to_slice_num(gpio), pwm_gpio_to_channel(gpio),
                     level);
}
void pwm_set_output_polarity(uint slice, bool a, bool b) {
  FakePicoBoard::PwmState state = Current(slice);
  state.inverted = {{a, b}};
  FakePico().setPwmState(slice, state);
}
void pwm_set_counter(uint slice, uint16_t count) {
  FakePicoBoard::PwmState state = Current(slice);
  state.counter = count % (uint32_t{state.wrap} + 1);
  FakePico().setPwmState(slice, state);
}
uint16_t pwm_get_counter(uint slice) { return FakePico().pwmCounter(slice); }
void pwm_set_phase_correct(uint slice, bool phase_correct) {
  FakePico().pwmState(slice);
  CHECK(!phase_correct) << "Unimplemented: Pico PWM phase correct";
}
void pwm_set_clkdiv_mode(uint slice, pwm_clkdiv_mode mode) {
  FakePico().pwmState(slice);
  CHECK(mode == PWM_DIV_FREE_RUNNING)
      << "Unimplemented: Pico PWM gated divider";
}
void pwm_set_irq_enabled(uint slice, bool) {
  FakePico().pwmState(slice);
  LOG(FATAL) << "Unimplemented: Pico PWM interrupts";
}
void pwm_clear_irq(uint slice) {
  FakePico().pwmState(slice);
  LOG(FATAL) << "Unimplemented: Pico PWM interrupts";
}
}
