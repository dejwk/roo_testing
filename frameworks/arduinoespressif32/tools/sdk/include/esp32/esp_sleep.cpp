
#include "esp_sleep.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t esp_sleep_disable_wakeup_source(esp_sleep_source_t source) {
  return 0;
}

esp_err_t esp_sleep_enable_ulp_wakeup() {
  return 0;
}

esp_err_t esp_sleep_enable_timer_wakeup(uint64_t time_in_us) {
  return 0;
}

esp_err_t esp_sleep_enable_touchpad_wakeup() {
  return 0;
}

//touch_pad_t esp_sleep_get_touchpad_wakeup_status();

esp_err_t esp_sleep_enable_ext0_wakeup(gpio_num_t gpio_num, int level) {
  return 0;
}

esp_err_t esp_sleep_enable_ext1_wakeup(uint64_t mask, esp_sleep_ext1_wakeup_mode_t mode) {
  return 0;
}

uint64_t esp_sleep_get_ext1_wakeup_status() {
  return 0;
}

esp_err_t esp_sleep_pd_config(esp_sleep_pd_domain_t domain,
                                   esp_sleep_pd_option_t option) {
  return 0;
}

void esp_deep_sleep_start() {}

esp_err_t esp_light_sleep_start() { return 0; }

void esp_deep_sleep(uint64_t time_in_us) {}

void system_deep_sleep(uint64_t time_in_us) {}

esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause() { 
  return ESP_SLEEP_WAKEUP_UNDEFINED; 
}

void esp_wake_deep_sleep(void) {}

void esp_set_deep_sleep_wake_stub(esp_deep_sleep_wake_stub_fn_t new_stub) {}

esp_deep_sleep_wake_stub_fn_t esp_get_deep_sleep_wake_stub(void) {
  return nullptr;
}

void esp_default_wake_deep_sleep(void) {}


#ifdef __cplusplus
}
#endif
