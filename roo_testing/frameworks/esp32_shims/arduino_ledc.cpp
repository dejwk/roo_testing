#include <algorithm>
#include <array>
#include <cmath>

#include "esp32-hal-ledc.h"
#include "esp32-hal-periman.h"
#include "glog/logging.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/system/timer.h"
#include "roo_testing/transducers/voltage/voltage.h"

namespace {
constexpr size_t kPins = SOC_GPIO_PIN_COUNT;
constexpr size_t kChannels = SOC_LEDC_CHANNEL_NUM;

struct Channel {
  bool attached = false;
  uint8_t pin = UINT8_MAX;
  uint8_t resolution = 8;
  uint32_t frequency = 1000;
  uint32_t duty = 0;
  bool inverted = false;
  int64_t carrier_origin_us = 0;
};
std::array<Channel, kChannels> channels;
std::array<int8_t, kPins> pin_channel = [] {
  std::array<int8_t, kPins> value{};
  value.fill(-1);
  return value;
}();
ledc_clk_cfg_t clock_source = static_cast<ledc_clk_cfg_t>(0);

Channel* forPin(uint8_t pin) {
  return pin < pin_channel.size() && pin_channel[pin] >= 0
             ? &channels[pin_channel[pin]]
             : nullptr;
}

uint32_t PeriodCounts(uint8_t resolution) { return uint32_t{1} << resolution; }

uint32_t MappedDuty(const Channel& channel) {
  const uint32_t period = PeriodCounts(channel.resolution);
  if (channel.resolution > 1 && channel.duty >= period - 1) return period;
  return std::min(channel.duty, period);
}

roo_testing_transducers::VoltageSignal BuildSignal(const Channel& channel) {
  using roo_testing_transducers::ConstantDuty;
  using roo_testing_transducers::VoltageDigitalHigh;
  using roo_testing_transducers::VoltageSignal;

  if (channel.frequency == 0) {
    return VoltageSignal::Constant(channel.inverted ? VoltageDigitalHigh()
                                                    : 0.0f);
  }
  return VoltageSignal::Square(
      0.0f, VoltageDigitalHigh(), channel.frequency,
      ConstantDuty{static_cast<double>(MappedDuty(channel)) /
                   PeriodCounts(channel.resolution)},
      channel.carrier_origin_us, 0.0, channel.inverted);
}

void Publish(const Channel& channel) {
  FakeEsp32().gpio.get(channel.pin).write(BuildSignal(channel));
}

void DriveLow(uint8_t pin) { FakeEsp32().gpio.get(pin).write(0.0f); }
}  // namespace

extern "C" {

ledc_clk_cfg_t ledcGetClockSource(void) { return clock_source; }
bool ledcSetClockSource(ledc_clk_cfg_t source) {
  clock_source = source;
  return true;
}

bool ledcAttachChannel(uint8_t pin, uint32_t frequency, uint8_t resolution,
                       uint8_t channel) {
  if (pin >= kPins || channel >= channels.size() || resolution == 0 ||
      resolution > 20)
    return false;
  if (pin_channel[pin] >= 0) ledcDetach(pin);
  auto& state = channels[channel];
  if (state.attached && state.pin < pin_channel.size()) {
    DriveLow(state.pin);
    pin_channel[state.pin] = -1;
    perimanClearPinBus(state.pin);
  }
  state = {
      true, pin, resolution, frequency, 0, false, system_time_get_micros()};
  pin_channel[pin] = channel;
  if (!perimanSetPinBus(pin, ESP32_BUS_TYPE_LEDC, &state, 0, channel)) {
    state = {};
    pin_channel[pin] = -1;
    return false;
  }
  Publish(state);
  return true;
}

bool ledcAttach(uint8_t pin, uint32_t frequency, uint8_t resolution) {
  for (uint8_t channel = 0; channel < channels.size(); ++channel) {
    if (!channels[channel].attached)
      return ledcAttachChannel(pin, frequency, resolution, channel);
  }
  return false;
}

bool ledcWrite(uint8_t pin, uint32_t duty) {
  Channel* state = forPin(pin);
  if (!state) return false;
  state->duty = duty;
  Publish(*state);
  return true;
}
bool ledcWriteChannel(uint8_t channel, uint32_t duty) {
  if (channel >= channels.size() || !channels[channel].attached) return false;
  channels[channel].duty = duty;
  Publish(channels[channel]);
  return true;
}
uint32_t ledcWriteTone(uint8_t pin, uint32_t frequency) {
  Channel* state = forPin(pin);
  if (!state) return 0;
  state->frequency = frequency;
  state->duty = frequency ? 1u << (state->resolution - 1) : 0;
  Publish(*state);
  return frequency;
}
uint32_t ledcWriteNote(uint8_t pin, note_t note, uint8_t octave) {
  if (note >= NOTE_MAX) return 0;
  const double semitones = static_cast<int>(note) - static_cast<int>(NOTE_A) +
                           (static_cast<int>(octave) - 4) * 12;
  return ledcWriteTone(pin, static_cast<uint32_t>(
                                440.0 * std::pow(2.0, semitones / 12.0) + 0.5));
}
uint32_t ledcRead(uint8_t pin) {
  Channel* state = forPin(pin);
  return state ? MappedDuty(*state) : 0;
}
uint32_t ledcReadFreq(uint8_t pin) {
  Channel* state = forPin(pin);
  return state ? state->frequency : 0;
}
bool ledcDetach(uint8_t pin) {
  Channel* state = forPin(pin);
  if (!state) return false;
  DriveLow(pin);
  *state = {};
  pin_channel[pin] = -1;
  perimanClearPinBus(pin);
  return true;
}
uint32_t ledcChangeFrequency(uint8_t pin, uint32_t frequency,
                             uint8_t resolution) {
  Channel* state = forPin(pin);
  if (!state || resolution == 0 || resolution > 20) return 0;
  state->frequency = frequency;
  state->resolution = resolution;
  state->carrier_origin_us = system_time_get_micros();
  Publish(*state);
  return frequency;
}
bool ledcOutputInvert(uint8_t pin, bool inverted) {
  Channel* state = forPin(pin);
  if (!state) return false;
  state->inverted = inverted;
  Publish(*state);
  return true;
}
bool ledcFade(uint8_t, uint32_t, uint32_t, int) { return false; }
bool ledcFadeWithInterrupt(uint8_t, uint32_t, uint32_t, int, void (*)(void)) {
  return false;
}
bool ledcFadeWithInterruptArg(uint8_t, uint32_t, uint32_t, int, void (*)(void*),
                              void*) {
  return false;
}
#ifdef SOC_LEDC_GAMMA_CURVE_FADE_SUPPORTED
bool ledcSetGammaTable(const float*, uint16_t) { return false; }
void ledcClearGammaTable(void) {
  LOG(WARNING) << "Unimplemented: Arduino LEDC gamma";
}
void ledcSetGammaFactor(float) {
  LOG(WARNING) << "Unimplemented: Arduino LEDC gamma";
}
bool ledcFadeGamma(uint8_t, uint32_t, uint32_t, int) { return false; }
bool ledcFadeGammaWithInterrupt(uint8_t, uint32_t, uint32_t, int,
                                void (*)(void)) {
  return false;
}
bool ledcFadeGammaWithInterruptArg(uint8_t, uint32_t, uint32_t, int,
                                   void (*)(void*), void*) {
  return false;
}
#endif

}  // extern "C"
