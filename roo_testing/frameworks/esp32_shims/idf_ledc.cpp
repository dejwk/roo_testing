#include <algorithm>
#include <array>
#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

#include "driver/ledc.h"
#include "esp_intr_alloc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "ledc_fade_engine.h"
#include "roo_testing/frameworks/esp_idf_support/esp_interrupts.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/system/timer.h"
#include "roo_testing/transducers/voltage/voltage.h"
#include "soc/interrupts.h"

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
  uint32_t fade_target = 0;
  uint64_t fade_duration_us = 0;
  ledc_cbs_t callbacks{};
  void* callback_arg = nullptr;
  SemaphoreHandle_t gate = nullptr;
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
  LedcFadeEngine fade_engine;
  bool fade_installed = false;
  intr_handle_t interrupt_handle = nullptr;
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

constexpr uint8_t FadeIndex(ledc_mode_t mode, ledc_channel_t channel) {
  return static_cast<uint8_t>(static_cast<unsigned>(mode) * LEDC_CHANNEL_MAX +
                              static_cast<unsigned>(channel));
}

ChannelState* ChannelForIndex(uint8_t index) {
  const unsigned mode = index / LEDC_CHANNEL_MAX;
  const unsigned channel = index % LEDC_CHANNEL_MAX;
  if (mode >= LEDC_SPEED_MODE_MAX || channel >= LEDC_CHANNEL_MAX) return nullptr;
  return &State().channels[mode][channel];
}

void RaiseLedcInterrupt(void*) {
  roo_testing::esp_idf::raiseInterruptSource(ETS_LEDC_INTR_SOURCE);
}

void LedcInterrupt(void*) {
  BaseType_t higher_priority_task_woken = pdFALSE;
  for (uint8_t index = 0; index < LEDC_SPEED_MODE_MAX * LEDC_CHANNEL_MAX;
       ++index) {
    const std::optional<LedcFadeEngine::Completion> completion =
        State().fade_engine.TakeCompletion(index);
    if (!completion.has_value()) continue;
    ledc_cb_t callback = nullptr;
    void* callback_arg = nullptr;
    {
      std::lock_guard<std::mutex> lock(State().mutex);
      ChannelState* channel = ChannelForIndex(index);
      if (channel == nullptr) continue;
      channel->duty = completion->duty;
      channel->pending_duty = completion->duty;
      callback = channel->callbacks.fade_cb;
      callback_arg = channel->callback_arg;
      if (channel->gate != nullptr) {
        xSemaphoreGiveFromISR(channel->gate, &higher_priority_task_woken);
      }
    }
    if (callback != nullptr) {
      const ledc_cb_param_t param{LEDC_FADE_END_EVT,
                                  static_cast<uint32_t>(index / LEDC_CHANNEL_MAX),
                                  static_cast<uint32_t>(index % LEDC_CHANNEL_MAX),
                                  completion->duty};
      if (callback(&param, callback_arg)) higher_priority_task_woken = pdTRUE;
    }
  }
  if (higher_priority_task_woken == pdTRUE) portYIELD_FROM_ISR();
}

SemaphoreHandle_t EnsureGate(ChannelState* channel) {
  if (channel->gate != nullptr) return channel->gate;
  channel->gate = xSemaphoreCreateBinary();
  if (channel->gate != nullptr) xSemaphoreGive(channel->gate);
  return channel->gate;
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

esp_err_t ledc_fade_func_install(int intr_alloc_flags) {
  std::lock_guard<std::mutex> lock(State().mutex);
  if (State().fade_installed) return ESP_ERR_INVALID_STATE;
  intr_handle_t handle = nullptr;
  const esp_err_t result = esp_intr_alloc(ETS_LEDC_INTR_SOURCE, intr_alloc_flags,
                                          LedcInterrupt, nullptr, &handle);
  if (result != ESP_OK) return result;
  if (esp_intr_enable(handle) != ESP_OK) {
    esp_intr_free(handle);
    return ESP_FAIL;
  }
  State().interrupt_handle = handle;
  State().fade_installed = true;
  State().fade_engine.SetCompletionNotifier(RaiseLedcInterrupt, nullptr);
  return ESP_OK;
}

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
    if (State().fade_engine.snapshot(FadeIndex(mode, channel)).gate_held) {
      return ESP_ERR_INVALID_STATE;
    }
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
    if (State().fade_engine.snapshot(FadeIndex(mode, channel)).gate_held) {
      return ESP_ERR_INVALID_STATE;
    }
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
  if (!state.configured) return LEDC_ERR_DUTY;
  const LedcFadeEngine::Snapshot fade =
      State().fade_engine.snapshot(FadeIndex(mode, channel));
  return fade.gate_held ? fade.duty : state.duty;
}

esp_err_t ledc_set_fade_with_time(ledc_mode_t mode, ledc_channel_t channel,
                                  uint32_t target_duty,
                                  int desired_fade_time_ms) {
  if (!ValidMode(mode) || !ValidChannel(channel) || desired_fade_time_ms < 0) {
    return ESP_ERR_INVALID_ARG;
  }
  std::lock_guard<std::mutex> lock(State().mutex);
  ChannelState& state = State().channels[mode][channel];
  if (!State().fade_installed || !state.configured) return ESP_ERR_INVALID_STATE;
  const TimerState& timer = State().timers[mode][state.config.timer_sel];
  if (!ValidOutput(timer, target_duty, state.hpoint)) return ESP_ERR_INVALID_ARG;
  state.fade_target = target_duty;
  state.fade_duration_us = static_cast<uint64_t>(desired_fade_time_ms) * 1000;
  return ESP_OK;
}

esp_err_t ledc_fade_start(ledc_mode_t mode, ledc_channel_t channel,
                          ledc_fade_mode_t fade_mode) {
  if (!ValidMode(mode) || !ValidChannel(channel) ||
      (fade_mode != LEDC_FADE_NO_WAIT && fade_mode != LEDC_FADE_WAIT_DONE)) {
    return ESP_ERR_INVALID_ARG;
  }
  SemaphoreHandle_t gate = nullptr;
  LedcFadeEngine::Request request{};
  {
    std::lock_guard<std::mutex> lock(State().mutex);
    ChannelState& state = State().channels[mode][channel];
    if (!State().fade_installed || !state.configured) return ESP_ERR_INVALID_STATE;
    const TimerState& timer = State().timers[mode][state.config.timer_sel];
    if (!ValidOutput(timer, state.fade_target, state.hpoint)) return ESP_ERR_INVALID_ARG;
    gate = EnsureGate(&state);
    if (gate == nullptr || xSemaphoreTake(gate, 0) != pdTRUE) {
      return ESP_ERR_INVALID_STATE;
    }
    request.channel = FadeIndex(mode, channel);
    request.pin = state.config.gpio_num;
    request.frequency_hz = timer.config.freq_hz;
    request.resolution = timer.config.duty_resolution;
    request.start_duty = state.duty;
    request.target_duty = state.fade_target;
    request.carrier_origin_us = timer.carrier_origin_us;
    request.pulse_start_phase = static_cast<double>(state.hpoint) /
                                PeriodCounts(timer.config.duty_resolution);
    request.inverted = state.config.flags.output_invert;
    request.duration_us = state.fade_duration_us;
  }
  if (!State().fade_engine.Start(request)) {
    xSemaphoreGive(gate);
    return ESP_ERR_INVALID_STATE;
  }
  if (fade_mode == LEDC_FADE_WAIT_DONE) {
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) return ESP_ERR_INVALID_STATE;
    if (xSemaphoreTake(gate, portMAX_DELAY) != pdTRUE) return ESP_FAIL;
  }
  return ESP_OK;
}

esp_err_t ledc_set_fade_time_and_start(ledc_mode_t mode, ledc_channel_t channel,
                                       uint32_t target_duty,
                                       uint32_t desired_fade_time_ms,
                                       ledc_fade_mode_t fade_mode) {
  if (desired_fade_time_ms > static_cast<uint32_t>(INT32_MAX)) return ESP_ERR_INVALID_ARG;
  esp_err_t result = ledc_set_fade_with_time(mode, channel, target_duty,
                                             static_cast<int>(desired_fade_time_ms));
  return result == ESP_OK ? ledc_fade_start(mode, channel, fade_mode) : result;
}

esp_err_t ledc_cb_register(ledc_mode_t mode, ledc_channel_t channel,
                           ledc_cbs_t* cbs, void* user_arg) {
  if (!ValidMode(mode) || !ValidChannel(channel) || cbs == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  std::lock_guard<std::mutex> lock(State().mutex);
  ChannelState& state = State().channels[mode][channel];
  if (!State().fade_installed || !state.configured) return ESP_ERR_INVALID_STATE;
  state.callbacks = *cbs;
  state.callback_arg = user_arg;
  return ESP_OK;
}

void ledc_fade_func_uninstall(void) {
  intr_handle_t handle = nullptr;
  {
    std::lock_guard<std::mutex> lock(State().mutex);
    if (!State().fade_installed) return;
    for (unsigned mode = 0; mode < LEDC_SPEED_MODE_MAX; ++mode) {
      for (unsigned channel = 0; channel < LEDC_CHANNEL_MAX; ++channel) {
        ChannelState& state = State().channels[mode][channel];
        State().fade_engine.Cancel(FadeIndex(static_cast<ledc_mode_t>(mode),
                                             static_cast<ledc_channel_t>(channel)));
        state.callbacks = {};
        state.callback_arg = nullptr;
      }
    }
    State().fade_engine.SetCompletionNotifier(nullptr, nullptr);
    State().fade_installed = false;
    handle = State().interrupt_handle;
    State().interrupt_handle = nullptr;
  }
  if (handle != nullptr) {
    esp_intr_disable(handle);
    esp_intr_free(handle);
  }
}

}  // extern "C"
