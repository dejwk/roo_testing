#include "hardware/adc.h"

#include "glog/logging.h"
#include "roo_testing/host/scheduler_signal_guard.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

extern "C" {
void adc_init() {
  roo_testing::SchedulerSignalGuard signal_guard;
  FakePico().initAdc();
}
void adc_gpio_init(uint gpio) {
  roo_testing::SchedulerSignalGuard signal_guard;
  FakePico().initAdcGpio(gpio);
}
void adc_select_input(uint input) {
  roo_testing::SchedulerSignalGuard signal_guard;
  FakePico().selectAdc(input);
}
uint adc_get_selected_input() {
  roo_testing::SchedulerSignalGuard signal_guard;
  return FakePico().adcInput();
}
uint16_t adc_read() {
  roo_testing::SchedulerSignalGuard signal_guard;
  return FakePico().readAdc();
}
void adc_set_temp_sensor_enabled(bool) {
  roo_testing::SchedulerSignalGuard signal_guard;
  LOG(FATAL) << "Unimplemented: Pico ADC temperature sensor";
}
void adc_run(bool) {
  roo_testing::SchedulerSignalGuard signal_guard;
  LOG(FATAL) << "Unimplemented: Pico ADC continuous conversion";
}
void adc_set_round_robin(uint) {
  roo_testing::SchedulerSignalGuard signal_guard;
  LOG(FATAL) << "Unimplemented: Pico ADC round robin";
}
}
