#include "soc/rtc.h"

rtc_xtal_freq_t _xtal_freq = RTC_XTAL_FREQ_AUTO;
bool _clk_32k_enabled_ = false;
bool _clk_8m_enabled_ = false;
bool _clk_8md256_enabled_ = false;
rtc_slow_freq_t _clk_slow_freq = RTC_SLOW_FREQ_RTC;

rtc_cpu_freq_config_t _clk_cpu_freq_config = {
    .source = RTC_CPU_FREQ_SRC_8M,
    .source_freq_mhz = 8,
    .div = 1,
    .freq_mhz = 80,
};

extern "C" {

void rtc_clk_init(rtc_clk_config_t cfg) {}

rtc_xtal_freq_t rtc_clk_xtal_freq_get() {
  return _xtal_freq;
}

void rtc_clk_xtal_freq_update(rtc_xtal_freq_t xtal_freq) {
  _xtal_freq = xtal_freq;
}

void rtc_clk_32k_enable(bool en) {}

bool rtc_clk_32k_enabled() { return _clk_32k_enabled_; }

void rtc_clk_32k_bootstrap(uint32_t cycle) {
    _clk_32k_enabled_ = true;
}

void rtc_clk_8m_enable(bool clk_8m_en, bool d256_en) {
    _clk_8m_enabled_ = true;
}

bool rtc_clk_8m_enabled() {
    return _clk_8m_enabled_;
}

bool rtc_clk_8md256_enabled() {
    return _clk_8md256_enabled_;
}

void rtc_clk_apll_enable(bool enable, uint32_t sdm0, uint32_t sdm1,
        uint32_t sdm2, uint32_t o_div) {}

void rtc_clk_slow_freq_set(rtc_slow_freq_t slow_freq) {
    _clk_slow_freq = slow_freq;
}

rtc_slow_freq_t rtc_clk_slow_freq_get() {
    return _clk_slow_freq;
}

uint32_t rtc_clk_slow_freq_get_hz() {
    switch (_clk_slow_freq) {
        case RTC_SLOW_FREQ_RTC: return -150000;
        case RTC_SLOW_FREQ_32K_XTAL: return 32768;
        case RTC_SLOW_FREQ_8MD256: return -33000;
    }
}

// void rtc_clk_fast_freq_set(rtc_fast_freq_t fast_freq);

// rtc_fast_freq_t rtc_clk_fast_freq_get(void);

// void rtc_clk_cpu_freq_to_config(rtc_cpu_freq_t cpu_freq, rtc_cpu_freq_config_t* out_config);

bool rtc_clk_cpu_freq_mhz_to_config(uint32_t freq_mhz, rtc_cpu_freq_config_t* out_config) {
    out_config->freq_mhz = freq_mhz;
    out_config->div = 1;
    out_config->source_freq_mhz = freq_mhz;
    out_config->source = RTC_CPU_FREQ_SRC_XTAL;
    return true;
}

void rtc_clk_cpu_freq_set_config(const rtc_cpu_freq_config_t* config) {
    _clk_cpu_freq_config = *config;
}

void rtc_clk_cpu_freq_set_config_fast(const rtc_cpu_freq_config_t* config) {
    _clk_cpu_freq_config = *config;
}

void rtc_clk_cpu_freq_get_config(rtc_cpu_freq_config_t* out_config) {
    *out_config = _clk_cpu_freq_config;
}

// void rtc_clk_cpu_freq_set_xtal(void);

void rtc_clk_apb_freq_update(uint32_t apb_freq) {}

// uint32_t rtc_clk_apb_freq_get(void);

// uint32_t rtc_clk_cal(rtc_cal_sel_t cal_clk, uint32_t slow_clk_cycles);

// uint32_t rtc_clk_cal_ratio(rtc_cal_sel_t cal_clk, uint32_t slow_clk_cycles);

// uint64_t rtc_time_us_to_slowclk(uint64_t time_in_us, uint32_t period);

// uint64_t rtc_time_slowclk_to_us(uint64_t rtc_cycles, uint32_t period);

// uint64_t rtc_time_get(void);

void rtc_clk_wait_for_slow_cycle(void) {}

// void rtc_dig_clk8m_enable(void);

// void rtc_dig_clk8m_disable(void);

// bool rtc_dig_8m_enabled(void);

// uint32_t rtc_clk_freq_cal(uint32_t cal_val);

// void rtc_sleep_get_default_config(uint32_t sleep_flags, rtc_sleep_config_t *out_config);

// void rtc_sleep_init(rtc_sleep_config_t cfg);

// void rtc_sleep_low_init(uint32_t slowclk_period);

// void rtc_sleep_set_wakeup_time(uint64_t t);

// uint32_t rtc_sleep_start(uint32_t wakeup_opt, uint32_t reject_opt);

// uint32_t rtc_deep_sleep_start(uint32_t wakeup_opt, uint32_t reject_opt);

/**
 * Initialize RTC clock and power control related functions
 * @param cfg configuration options as rtc_config_t
 */
void rtc_init(rtc_config_t cfg);

// rtc_vddsdio_config_t rtc_vddsdio_get_config(void);

// void rtc_vddsdio_set_config(rtc_vddsdio_config_t config);

}
