#include "driver/ledc.h"

#include <array>
#include <mutex>

namespace {

struct TimerState {
  bool configured = false;
  ledc_timer_config_t config{};
};

struct ChannelState {
  bool configured = false;
  ledc_channel_config_t config{};
  uint32_t pending_duty = 0;
  uint32_t duty = 0;
  uint32_t fade_target = 0;
  bool fade_configured = false;
};

struct LedcState {
  std::array<std::array<TimerState, LEDC_TIMER_MAX>, LEDC_SPEED_MODE_MAX>
      timers;
  std::array<std::array<ChannelState, LEDC_CHANNEL_MAX>, LEDC_SPEED_MODE_MAX>
      channels;
  bool fade_installed = false;
  std::mutex mutex;
};

LedcState& State() {
  static LedcState state;
  return state;
}

bool ValidMode(ledc_mode_t mode) {
  return static_cast<unsigned>(mode) < LEDC_SPEED_MODE_MAX;
}

bool ValidTimer(ledc_timer_t timer) {
  return static_cast<unsigned>(timer) < LEDC_TIMER_MAX;
}

bool ValidChannel(ledc_channel_t channel) {
  return static_cast<unsigned>(channel) < LEDC_CHANNEL_MAX;
}

}  // namespace

extern "C" {

esp_err_t ledc_timer_config(const ledc_timer_config_t* config) {
  if (config == nullptr || !ValidMode(config->speed_mode) ||
      !ValidTimer(config->timer_num)) {
    return ESP_ERR_INVALID_ARG;
  }

  std::lock_guard<std::mutex> lock(State().mutex);
  TimerState& timer = State().timers[config->speed_mode][config->timer_num];
  if (config->deconfigure) {
    timer = {};
    return ESP_OK;
  }
  if (config->freq_hz == 0 || config->duty_resolution < LEDC_TIMER_1_BIT ||
      config->duty_resolution >= LEDC_TIMER_BIT_MAX) {
    return ESP_ERR_INVALID_ARG;
  }
  timer.configured = true;
  timer.config = *config;
  return ESP_OK;
}

esp_err_t ledc_channel_config(const ledc_channel_config_t* config) {
  if (config == nullptr || !ValidMode(config->speed_mode) ||
      !ValidChannel(config->channel) || !ValidTimer(config->timer_sel)) {
    return ESP_ERR_INVALID_ARG;
  }

  std::lock_guard<std::mutex> lock(State().mutex);
  ChannelState& channel =
      State().channels[config->speed_mode][config->channel];
  if (config->deconfigure) {
    channel = {};
    return ESP_OK;
  }
  channel.configured = true;
  channel.config = *config;
  channel.pending_duty = config->duty;
  channel.duty = config->duty;
  channel.fade_target = config->duty;
  channel.fade_configured = false;
  return ESP_OK;
}

esp_err_t ledc_fade_func_install(int) {
  std::lock_guard<std::mutex> lock(State().mutex);
  if (State().fade_installed) return ESP_ERR_INVALID_STATE;
  State().fade_installed = true;
  return ESP_OK;
}

esp_err_t ledc_set_duty(ledc_mode_t mode, ledc_channel_t channel,
                        uint32_t duty) {
  if (!ValidMode(mode) || !ValidChannel(channel)) return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(State().mutex);
  ChannelState& state = State().channels[mode][channel];
  if (!state.configured) return ESP_ERR_INVALID_STATE;
  state.pending_duty = duty;
  return ESP_OK;
}

esp_err_t ledc_update_duty(ledc_mode_t mode, ledc_channel_t channel) {
  if (!ValidMode(mode) || !ValidChannel(channel)) return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(State().mutex);
  ChannelState& state = State().channels[mode][channel];
  if (!state.configured) return ESP_ERR_INVALID_STATE;
  state.duty = state.pending_duty;
  return ESP_OK;
}

uint32_t ledc_get_duty(ledc_mode_t mode, ledc_channel_t channel) {
  if (!ValidMode(mode) || !ValidChannel(channel)) return LEDC_ERR_DUTY;
  std::lock_guard<std::mutex> lock(State().mutex);
  const ChannelState& state = State().channels[mode][channel];
  return state.configured ? state.duty : LEDC_ERR_DUTY;
}

esp_err_t ledc_set_fade_with_time(ledc_mode_t mode, ledc_channel_t channel,
                                  uint32_t target_duty, int fade_time_ms) {
  if (!ValidMode(mode) || !ValidChannel(channel) || fade_time_ms < 0) {
    return ESP_ERR_INVALID_ARG;
  }
  std::lock_guard<std::mutex> lock(State().mutex);
  ChannelState& state = State().channels[mode][channel];
  if (!State().fade_installed || !state.configured) {
    return ESP_ERR_INVALID_STATE;
  }
  state.fade_target = target_duty;
  state.fade_configured = true;
  return ESP_OK;
}

esp_err_t ledc_fade_start(ledc_mode_t mode, ledc_channel_t channel,
                          ledc_fade_mode_t fade_mode) {
  if (!ValidMode(mode) || !ValidChannel(channel) ||
      static_cast<unsigned>(fade_mode) >= LEDC_FADE_MAX) {
    return ESP_ERR_INVALID_ARG;
  }
  std::lock_guard<std::mutex> lock(State().mutex);
  ChannelState& state = State().channels[mode][channel];
  if (!State().fade_installed || !state.configured ||
      !state.fade_configured) {
    return ESP_ERR_INVALID_STATE;
  }
  // Timing and interrupts are outside the emulator's current scope. Applying
  // the target immediately preserves deterministic state for host tests.
  state.pending_duty = state.fade_target;
  state.duty = state.fade_target;
  state.fade_configured = false;
  return ESP_OK;
}

}  // extern "C"
