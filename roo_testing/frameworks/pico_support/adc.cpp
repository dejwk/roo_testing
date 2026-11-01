#include "hardware/adc.h"

#include "glog/logging.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

extern "C" {
void adc_init() { FakePico().initAdc(); }
void adc_gpio_init(uint gpio) { FakePico().initAdcGpio(gpio); }
void adc_select_input(uint input) { FakePico().selectAdc(input); }
uint adc_get_selected_input() { return FakePico().adcInput(); }
uint16_t adc_read() { return FakePico().readAdc(); }
void adc_set_temp_sensor_enabled(bool) {
  LOG(FATAL) << "Unimplemented: Pico ADC temperature sensor";
}
void adc_run(bool) {
  LOG(FATAL) << "Unimplemented: Pico ADC continuous conversion";
}
void adc_set_round_robin(uint) {
  LOG(FATAL) << "Unimplemented: Pico ADC round robin";
}
}
