#include "hal/assert.h"

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

extern "C" {

#include "hal/adc_hal.h"

void adc_ll_rtc_set_output_format(adc_ll_num_t adc_n, adc_bits_width_t bits)
{
    if (adc_n == ADC_NUM_1) {
        SENS.sar_start_force.sar1_bit_width = bits;
        SENS.sar_read_ctrl.sar1_sample_bit = bits;
    } else { // adc_n == ADC_NUM_2
        SENS.sar_start_force.sar2_bit_width = bits;
        SENS.sar_read_ctrl2.sar2_sample_bit = bits;
    }
    FakeEsp32().adc(adc_n).setWidth(bits);
}

void adc_ll_set_atten(adc_ll_num_t adc_n, adc_channel_t channel, adc_atten_t atten)
{
    if (adc_n == ADC_NUM_1) {
        SENS.sar_atten1 = ( SENS.sar_atten1 & ~(0x3 << (channel * 2)) ) | ((atten & 0x3) << (channel * 2));
    } else { // adc_n == ADC_NUM_2
        SENS.sar_atten2 = ( SENS.sar_atten2 & ~(0x3 << (channel * 2)) ) | ((atten & 0x3) << (channel * 2));
    }
    FakeEsp32().adc(adc_n).setAttenuation(channel, atten);
}

/**
 * Get the attenuation of a particular channel on ADCn.
 *
 * @param adc_n ADC unit.
 * @param channel ADCn channel number.
 * @return atten The attenuation option.
 */
adc_atten_t adc_ll_get_atten(adc_ll_num_t adc_n, adc_channel_t channel)
{
    // if (adc_n == ADC_NUM_1) {
    //     return (adc_atten_t)((SENS.sar_atten1 >> (channel * 2)) & 0x3);
    // } else {
    //     return (adc_atten_t)((SENS.sar_atten2 >> (channel * 2)) & 0x3);
    // }
    return (adc_atten_t)FakeEsp32().adc(adc_n).getAttenuation(channel);
}

esp_err_t adc_hal_convert(adc_ll_num_t adc_n, int channel, int *out_raw)
{
    *out_raw = FakeEsp32().adc(adc_n).convert(channel);
    return ESP_OK;
}

}