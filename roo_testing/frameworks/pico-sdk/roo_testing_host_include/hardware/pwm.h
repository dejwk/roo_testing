#pragma once
#include "pico.h"
#ifdef __cplusplus
extern "C" {
#endif
/// Stores SDK-compatible configuration fields without exposing registers.
typedef struct {
  uint32_t csr;
  uint32_t div;
  uint32_t top;
} pwm_config;

/// Selects divider gating; only free-running mode is supported.
typedef enum pwm_clkdiv_mode {
  PWM_DIV_FREE_RUNNING = 0,
  PWM_DIV_B_HIGH = 1,
  PWM_DIV_B_RISING = 2,
  PWM_DIV_B_FALLING = 3
} pwm_clkdiv_mode;

/// Returns the checked GPIO's slice (0..7).
uint pwm_gpio_to_slice_num(uint gpio);

/// Returns the checked GPIO's A/B channel (0/1).
uint pwm_gpio_to_channel(uint gpio);

/// Returns free-running, divide-by-one, top-65535 defaults.
pwm_config pwm_get_default_config(void);

/// Configures a supported 4-bit fractional clock divider.
void pwm_config_set_clkdiv(pwm_config* config, float div);

/// Configures an integer/fraction divider pair.
void pwm_config_set_clkdiv_int_frac4(pwm_config* config, uint32_t integer,
                                     uint8_t fract);

/// Selects channel A or B.
typedef enum pwm_chan { PWM_CHAN_A = 0, PWM_CHAN_B = 1 } pwm_chan;

/// Configures an integer divider.
void pwm_config_set_clkdiv_int(pwm_config* config, uint32_t integer);

/// Compatibility spelling for a four-bit fractional divider.
void pwm_config_set_clkdiv_int_frac(pwm_config* config, uint8_t integer,
                                    uint8_t fract);

/// Configures the shared wrap value.
void pwm_config_set_wrap(pwm_config* config, uint16_t wrap);

/// Rejects phase-correct mode; false keeps free-running operation.
void pwm_config_set_phase_correct(pwm_config* config, bool phase_correct);

/// Selects free-running divider mode; other modes fail.
void pwm_config_set_clkdiv_mode(pwm_config* config, pwm_clkdiv_mode mode);

/// Configures channel output inversion.
void pwm_config_set_output_polarity(pwm_config* config, bool a, bool b);

/// Initializes a slice, clears levels/counter, and optionally starts it.
void pwm_init(uint slice, const pwm_config* config, bool start);

/// Enables/disables a slice; disabled output retains its stopped comparator
/// level.
void pwm_set_enabled(uint slice, bool enabled);

/// Updates the enabled state of all eight slices from a checked mask.
void pwm_set_mask_enabled(uint32_t mask);

/// Updates shared slice wrap; publication is immediate rather than buffered.
void pwm_set_wrap(uint slice, uint16_t wrap);

/// Updates a slice divider with four-bit fractional resolution.
void pwm_set_clkdiv(uint slice, float divider);

/// Updates channel A or B compare; values above wrap produce full duty.
void pwm_set_chan_level(uint slice, uint chan, uint16_t level);

/// Updates both channel compares together.
void pwm_set_both_levels(uint slice, uint16_t a, uint16_t b);

/// Updates the compare selected by a checked physical GPIO.
void pwm_set_gpio_level(uint gpio, uint16_t level);

/// Updates both channel inversions on a slice.
void pwm_set_output_polarity(uint slice, bool a, bool b);

/// Sets the counter base at current microsecond-resolution uptime.
void pwm_set_counter(uint slice, uint16_t count);

/// Returns the counter at current microsecond-resolution uptime.
uint16_t pwm_get_counter(uint slice);

/// Rejects phase-correct mode; false retains free-running operation.
void pwm_set_phase_correct(uint slice, bool phase_correct);

/// Rejects divider gating modes other than free-running.
void pwm_set_clkdiv_mode(uint slice, pwm_clkdiv_mode mode);

/// Rejects unsupported PWM interrupts, including disable and acknowledge.
void pwm_set_irq_enabled(uint slice, bool enabled);

/// Rejects unsupported PWM interrupt acknowledgement.
void pwm_clear_irq(uint slice);
#ifdef __cplusplus
}
#endif
