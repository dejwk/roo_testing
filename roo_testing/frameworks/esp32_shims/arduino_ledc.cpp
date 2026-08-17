#include "esp32-hal-ledc.h"
#include "esp32-hal-periman.h"

#include <array>
#include <cmath>

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
};
std::array<Channel, kChannels> channels;
std::array<int8_t, kPins> pin_channel = [] {
  std::array<int8_t, kPins> value{};
  value.fill(-1);
  return value;
}();
ledc_clk_cfg_t clock_source = static_cast<ledc_clk_cfg_t>(0);

Channel *forPin(uint8_t pin) {
  return pin < pin_channel.size() && pin_channel[pin] >= 0
             ? &channels[pin_channel[pin]]
             : nullptr;
}
}  // namespace

extern "C" {

ledc_clk_cfg_t ledcGetClockSource(void) { return clock_source; }
bool ledcSetClockSource(ledc_clk_cfg_t source) { clock_source = source; return true; }

bool ledcAttachChannel(uint8_t pin, uint32_t frequency, uint8_t resolution,
                       uint8_t channel) {
  if (pin >= kPins || channel >= channels.size() || resolution == 0 || resolution > 20) return false;
  if (pin_channel[pin] >= 0) ledcDetach(pin);
  auto &state = channels[channel];
  if (state.attached && state.pin < pin_channel.size()) pin_channel[state.pin] = -1;
  state = {true, pin, resolution, frequency, 0, false};
  pin_channel[pin] = channel;
  return perimanSetPinBus(pin, ESP32_BUS_TYPE_LEDC, &state, 0, channel);
}

bool ledcAttach(uint8_t pin, uint32_t frequency, uint8_t resolution) {
  for (uint8_t channel = 0; channel < channels.size(); ++channel) {
    if (!channels[channel].attached) return ledcAttachChannel(pin, frequency, resolution, channel);
  }
  return false;
}

bool ledcWrite(uint8_t pin, uint32_t duty) {
  Channel *state = forPin(pin);
  if (!state) return false;
  state->duty = duty;
  return true;
}
bool ledcWriteChannel(uint8_t channel, uint32_t duty) {
  if (channel >= channels.size() || !channels[channel].attached) return false;
  channels[channel].duty = duty;
  return true;
}
uint32_t ledcWriteTone(uint8_t pin, uint32_t frequency) {
  Channel *state = forPin(pin);
  if (!state) return 0;
  state->frequency = frequency;
  state->duty = frequency ? 1u << (state->resolution - 1) : 0;
  return frequency;
}
uint32_t ledcWriteNote(uint8_t pin, note_t note, uint8_t octave) {
  if (note >= NOTE_MAX) return 0;
  const double semitones = static_cast<int>(note) - static_cast<int>(NOTE_A) + (static_cast<int>(octave) - 4) * 12;
  return ledcWriteTone(pin, static_cast<uint32_t>(440.0 * std::pow(2.0, semitones / 12.0) + 0.5));
}
uint32_t ledcRead(uint8_t pin) { Channel *state = forPin(pin); return state ? state->duty : 0; }
uint32_t ledcReadFreq(uint8_t pin) { Channel *state = forPin(pin); return state ? state->frequency : 0; }
bool ledcDetach(uint8_t pin) {
  Channel *state = forPin(pin);
  if (!state) return false;
  *state = {};
  pin_channel[pin] = -1;
  perimanClearPinBus(pin);
  return true;
}
uint32_t ledcChangeFrequency(uint8_t pin, uint32_t frequency, uint8_t resolution) {
  Channel *state = forPin(pin);
  if (!state || resolution == 0 || resolution > 20) return 0;
  state->frequency = frequency;
  state->resolution = resolution;
  return frequency;
}
bool ledcOutputInvert(uint8_t pin, bool inverted) {
  Channel *state = forPin(pin);
  if (!state) return false;
  state->inverted = inverted;
  return true;
}
bool ledcFade(uint8_t pin, uint32_t start, uint32_t target, int) {
  return ledcWrite(pin, start) && ledcWrite(pin, target);
}
bool ledcFadeWithInterrupt(uint8_t pin, uint32_t start, uint32_t target, int time,
                           void (*callback)(void)) {
  const bool ok = ledcFade(pin, start, target, time);
  if (ok && callback) callback();
  return ok;
}
bool ledcFadeWithInterruptArg(uint8_t pin, uint32_t start, uint32_t target, int time,
                              void (*callback)(void *), void *arg) {
  const bool ok = ledcFade(pin, start, target, time);
  if (ok && callback) callback(arg);
  return ok;
}
#ifdef SOC_LEDC_GAMMA_CURVE_FADE_SUPPORTED
bool ledcSetGammaTable(const float *, uint16_t) { return true; }
void ledcClearGammaTable(void) {}
void ledcSetGammaFactor(float) {}
bool ledcFadeGamma(uint8_t pin, uint32_t start, uint32_t target, int time) { return ledcFade(pin, start, target, time); }
bool ledcFadeGammaWithInterrupt(uint8_t pin, uint32_t start, uint32_t target, int time, void (*callback)(void)) { return ledcFadeWithInterrupt(pin, start, target, time, callback); }
bool ledcFadeGammaWithInterruptArg(uint8_t pin, uint32_t start, uint32_t target, int time, void (*callback)(void *), void *arg) { return ledcFadeWithInterruptArg(pin, start, target, time, callback, arg); }
#endif

}  // extern "C"
