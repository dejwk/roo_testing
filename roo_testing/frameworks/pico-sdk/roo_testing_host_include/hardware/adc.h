#pragma once
#include "pico.h"
#ifdef __cplusplus
extern "C" {
#endif
/// Initializes the single native ADC converter on external channel 0.
void adc_init(void);

/// Configures GP26..GP29 for analog input, releasing pulls and digital input.
void adc_gpio_init(uint gpio);

/// Selects one of four external channels; the temperature channel is
/// unsupported.
void adc_select_input(uint input);

/// Returns the selected external channel.
uint adc_get_selected_input(void);

/// Samples voltage against 3.3 V, floors 4096*V/Vref, and clamps to 0..4095.
uint16_t adc_read(void);

/// Rejects unsupported internal temperature sensor operation.
void adc_set_temp_sensor_enabled(bool enabled);

/// Rejects unsupported continuous conversions.
void adc_run(bool run);

/// Rejects unsupported round-robin conversion.
void adc_set_round_robin(uint mask);
#ifdef __cplusplus
}
#endif
