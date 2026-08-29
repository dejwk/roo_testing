#include <algorithm>
#include <array>
#include <cstdint>
#include <mutex>
#include <vector>

#include "driver/ledc.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/system/timer.h"
#include "roo_testing/transducers/voltage/voltage.h"

namespace {

using roo_testing_transducers::ConstantDuty;
using roo_testing_transducers::VoltageDigitalHigh;
using roo_testing_transducers::VoltageSignal;

struct TimerState {
  bool configured = false;
  ledc_timer_config_t config{};
  int64_t carrier_origin_us = 0;
};

struct ChannelState {
  bool configured = false;
  ledc_channel_config_t config{};
  uint32_t pending_duty = 0;
  uint32_t pending_hpoint = 0;
  uint32_t duty = 0;
  uint32_t hpoint = 0;
};

struct Publication {
  int pin;
  VoltageSignal signal;
};

struct LedcState {
  std::array<std::array<TimerState, LEDC_TIMER_MAX>, LEDC_SPEED_MODE_MAX>
      timers;
  std::array<std::array<ChannelState, LEDC_CHANNEL_MAX>, LEDC_SPEED_MODE_MAX>
      channels;
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

bool ValidPin(int pin) { return pin >= 0 && pin < SOC_GPIO_PIN_COUNT; }

uint32_t PeriodCounts(ledc_timer_bit_t resolution) {
  return uint32_t{1} << static_cast<unsigned>(resolution);
}

bool ValidTimerConfig(const ledc_timer_config_t& config) {
  return config.freq_hz != 0 && config.duty_resolution >= LEDC_TIMER_1_BIT &&
         config.duty_resolution < LEDC_TIMER_BIT_MAX;
}

bool ValidOutput(const TimerState& timer, uint32_t duty, uint32_t hpoint) {
  if (!timer.configured) return false;
  const uint32_t period = PeriodCounts(timer.config.duty_resolution);
  return duty <= period && hpoint < period;
}

VoltageSignal BuildSignal(const LedcState& state, ledc_mode_t mode,
                          const ChannelState& channel) {
  const TimerState& timer = state.timers[mode][channel.config.timer_sel];
  if (!channel.configured ||
      !ValidOutput(timer, channel.duty, channel.hpoint)) {
    return VoltageSignal::Constant(0.0f);
  }
  const uint32_t period = PeriodCounts(timer.config.duty_resolution);
  return VoltageSignal::Square(
      0.0f, VoltageDigitalHigh(), timer.config.freq_hz,
      ConstantDuty{static_cast<double>(channel.duty) / period},
      timer.carrier_origin_us, static_cast<double>(channel.hpoint) / period,
      channel.config.flags.output_invert);
}

void AddPublication(const LedcState& state, ledc_mode_t mode,
                    const ChannelState& channel,
                    std::vector<Publication>* publications) {
  if (channel.configured) {
    publications->push_back(
        {channel.config.gpio_num, BuildSignal(state, mode, channel)});
  }
}

void Publish(const std::vector<Publication>& publications) {
  for (const Publication& publication : publications) {
    FakeEsp32().gpio.get(publication.pin).write(publication.signal);
  }
}

}  // namespace

extern "C" {

esp_err_t ledc_timer_config(const ledc_timer_config_t* config) {
  if (config == nullptr || !ValidMode(config->speed_mode) ||
      !ValidTimer(config->timer_num)) {
    return ESP_ERR_INVALID_ARG;
  }
  if (!config->deconfigure && !ValidTimerConfig(*config)) {
    return ESP_ERR_INVALID_ARG;
  }

  const int64_t origin_us = config->deconfigure ? 0 : system_time_get_micros();
  std::vector<Publication> publications;
  {
    std::lock_guard<std::mutex> lock(State().mutex);
    TimerState& timer = State().timers[config->speed_mode][config->timer_num];
    if (config->deconfigure) {
      timer = {};
    } else {
      const bool resolution_changed =
          timer.configured &&
          timer.config.duty_resolution != config->duty_resolution;
      timer.configured = true;
      timer.config = *config;
      timer.carrier_origin_us = origin_us;
      if (resolution_changed) {
        const uint32_t period = PeriodCounts(config->duty_resolution);
        for (ChannelState& channel : State().channels[config->speed_mode]) {
          if (channel.configured &&
              channel.config.timer_sel == config->timer_num) {
            channel.pending_duty = std::min(channel.pending_duty, period);
            channel.duty = std::min(channel.duty, period);
            channel.pending_hpoint =
                std::min(channel.pending_hpoint, period - 1);
            channel.hpoint = std::min(channel.hpoint, period - 1);
          }
        }
      }
    }
    for (const ChannelState& channel : State().channels[config->speed_mode]) {
      if (channel.configured && channel.config.timer_sel == config->timer_num) {
        AddPublication(State(), config->speed_mode, channel, &publications);
      }
    }
  }
  Publish(publications);
  return ESP_OK;
}

esp_err_t ledc_channel_config(const ledc_channel_config_t* config) {
  if (config == nullptr || !ValidMode(config->speed_mode) ||
      !ValidChannel(config->channel)) {
    return ESP_ERR_INVALID_ARG;
  }
  if (!config->deconfigure &&
      (!ValidTimer(config->timer_sel) || !ValidPin(config->gpio_num) ||
       config->hpoint < 0)) {
    return ESP_ERR_INVALID_ARG;
  }

  std::vector<Publication> publications;
  {
    std::lock_guard<std::mutex> lock(State().mutex);
    ChannelState& channel =
        State().channels[config->speed_mode][config->channel];
    if (config->deconfigure) {
      if (channel.configured) {
        publications.push_back(
            {channel.config.gpio_num, VoltageSignal::Constant(0.0f)});
      }
      channel = {};
    } else {
      const TimerState& timer =
          State().timers[config->speed_mode][config->timer_sel];
      if (timer.configured &&
          !ValidOutput(timer, config->duty,
                       static_cast<uint32_t>(config->hpoint))) {
        return ESP_ERR_INVALID_ARG;
      }
      if (channel.configured && channel.config.gpio_num != config->gpio_num) {
        publications.push_back(
            {channel.config.gpio_num, VoltageSignal::Constant(0.0f)});
      }
      channel.configured = true;
      channel.config = *config;
      channel.pending_duty = config->duty;
      channel.pending_hpoint = config->hpoint;
      channel.duty = config->duty;
      channel.hpoint = config->hpoint;
      AddPublication(State(), config->speed_mode, channel, &publications);
    }
  }
  Publish(publications);
  return ESP_OK;
}

esp_err_t ledc_fade_func_install(int) { return ESP_ERR_NOT_SUPPORTED; }

esp_err_t ledc_set_duty(ledc_mode_t mode, ledc_channel_t channel,
                        uint32_t duty) {
  if (!ValidMode(mode) || !ValidChannel(channel)) return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(State().mutex);
  ChannelState& state = State().channels[mode][channel];
  if (!state.configured) return ESP_ERR_INVALID_STATE;
  const TimerState& timer = State().timers[mode][state.config.timer_sel];
  if (timer.configured && !ValidOutput(timer, duty, state.pending_hpoint)) {
    return ESP_ERR_INVALID_ARG;
  }
  state.pending_duty = duty;
  return ESP_OK;
}

esp_err_t ledc_set_duty_with_hpoint(ledc_mode_t mode, ledc_channel_t channel,
                                    uint32_t duty, uint32_t hpoint) {
  if (!ValidMode(mode) || !ValidChannel(channel)) return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(State().mutex);
  ChannelState& state = State().channels[mode][channel];
  if (!state.configured) return ESP_ERR_INVALID_STATE;
  const TimerState& timer = State().timers[mode][state.config.timer_sel];
  if (timer.configured && !ValidOutput(timer, duty, hpoint)) {
    return ESP_ERR_INVALID_ARG;
  }
  state.pending_duty = duty;
  state.pending_hpoint = hpoint;
  return ESP_OK;
}

esp_err_t ledc_update_duty(ledc_mode_t mode, ledc_channel_t channel) {
  if (!ValidMode(mode) || !ValidChannel(channel)) return ESP_ERR_INVALID_ARG;
  std::vector<Publication> publications;
  {
    std::lock_guard<std::mutex> lock(State().mutex);
    ChannelState& state = State().channels[mode][channel];
    if (!state.configured) return ESP_ERR_INVALID_STATE;
    const TimerState& timer = State().timers[mode][state.config.timer_sel];
    if (!ValidOutput(timer, state.pending_duty, state.pending_hpoint)) {
      return ESP_ERR_INVALID_ARG;
    }
    state.duty = state.pending_duty;
    state.hpoint = state.pending_hpoint;
    AddPublication(State(), mode, state, &publications);
  }
  Publish(publications);
  return ESP_OK;
}

esp_err_t ledc_set_duty_and_update(ledc_mode_t mode, ledc_channel_t channel,
                                   uint32_t duty, uint32_t hpoint) {
  if (!ValidMode(mode) || !ValidChannel(channel)) return ESP_ERR_INVALID_ARG;
  std::vector<Publication> publications;
  {
    std::lock_guard<std::mutex> lock(State().mutex);
    ChannelState& state = State().channels[mode][channel];
    if (!state.configured) return ESP_ERR_INVALID_STATE;
    const TimerState& timer = State().timers[mode][state.config.timer_sel];
    if (!ValidOutput(timer, duty, hpoint)) return ESP_ERR_INVALID_ARG;
    state.pending_duty = duty;
    state.pending_hpoint = hpoint;
    state.duty = duty;
    state.hpoint = hpoint;
    AddPublication(State(), mode, state, &publications);
  }
  Publish(publications);
  return ESP_OK;
}

uint32_t ledc_get_duty(ledc_mode_t mode, ledc_channel_t channel) {
  if (!ValidMode(mode) || !ValidChannel(channel)) return LEDC_ERR_DUTY;
  std::lock_guard<std::mutex> lock(State().mutex);
  const ChannelState& state = State().channels[mode][channel];
  return state.configured ? state.duty : LEDC_ERR_DUTY;
}

esp_err_t ledc_set_fade_with_time(ledc_mode_t, ledc_channel_t, uint32_t, int) {
  return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t ledc_fade_start(ledc_mode_t, ledc_channel_t, ledc_fade_mode_t) {
  return ESP_ERR_NOT_SUPPORTED;
}

}  // extern "C"
