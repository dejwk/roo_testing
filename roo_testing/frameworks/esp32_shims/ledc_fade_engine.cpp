#include "ledc_fade_engine.h"

#include <array>
#include <limits>
#include <mutex>
#include <vector>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/system/timer.h"
#include "roo_testing/transducers/voltage/voltage.h"

namespace {
constexpr size_t kChannels = 16;
using roo_testing_transducers::ConstantDuty;
using roo_testing_transducers::DutyProfile;
using roo_testing_transducers::LinearDutyFade;
using roo_testing_transducers::VoltageDigitalHigh;
using roo_testing_transducers::VoltageSignal;

uint32_t Period(uint8_t resolution) { return uint32_t{1} << resolution; }

uint32_t Materialize(const LedcFadeEngine::Request& request, int64_t now) {
  if (request.duration_us == 0 || now <= 0) return request.start_duty;
  const uint64_t elapsed = static_cast<uint64_t>(now);
  if (elapsed >= request.duration_us) return request.target_duty;
  const uint64_t delta = request.target_duty >= request.start_duty
                             ? request.target_duty - request.start_duty
                             : request.start_duty - request.target_duty;
  const uint64_t step = delta * elapsed / request.duration_us;
  return request.target_duty >= request.start_duty ? request.start_duty + step
                                                   : request.start_duty - step;
}
}  // namespace

struct LedcFadeEngine::Impl {
  enum class State { kIdle, kArmed, kQueued, kPending, kCancelled };
  struct Channel {
    State state = State::kIdle;
    Request request{};
    uint64_t generation = 0;
    uint32_t duty = 0;
    int64_t fade_start_us = 0;
    SystemTimeAlarmId alarm = 0;
  };
  struct Delivery {
    VoltageSignal signal = VoltageSignal::Constant(0);
    int pin;
    bool finalize;
    uint8_t channel;
    uint64_t generation;
  };
  mutable std::mutex mutex;
  std::array<Channel, kChannels> channels;
  std::vector<Delivery> delivery;
  bool draining = false;

  VoltageSignal signal(const Channel& channel, bool endpoint) const {
    const Request& r = channel.request;
    const double scale = Period(r.resolution);
    DutyProfile duty = endpoint
                           ? DutyProfile{ConstantDuty{
                                 static_cast<double>(r.target_duty) / scale}}
                           : DutyProfile{LinearDutyFade{
                                 static_cast<double>(r.start_duty) / scale,
                                 static_cast<double>(r.target_duty) / scale,
                                 channel.fade_start_us, r.duration_us}};
    return VoltageSignal::Square(0, VoltageDigitalHigh(), r.frequency_hz, duty,
                                 r.carrier_origin_us, r.pulse_start_phase,
                                 r.inverted);
  }

  void drain() {
    while (true) {
      Delivery item{};
      {
        std::lock_guard<std::mutex> lock(mutex);
        if (!draining || delivery.empty()) {
          draining = false;
          return;
        }
        item = delivery.front();
        delivery.erase(delivery.begin());
      }
      FakeEsp32().gpio.get(item.pin).write(item.signal);
      if (item.finalize) {
        std::lock_guard<std::mutex> lock(mutex);
        Channel& channel = channels[item.channel];
        if (channel.generation == item.generation &&
            channel.state == State::kQueued) {
          channel.state = State::kPending;
        }
      }
    }
  }

  void deadline(uint8_t index, uint64_t generation) {
    bool drain_now = false;
    {
      std::lock_guard<std::mutex> lock(mutex);
      Channel& channel = channels[index];
      if (channel.generation != generation || channel.state != State::kArmed)
        return;
      channel.duty = channel.request.target_duty;
      channel.state = State::kQueued;
      delivery.push_back({signal(channel, true), channel.request.pin, false,
                          index, generation});
      delivery.push_back({VoltageSignal::Constant(0), channel.request.pin, true,
                          index, generation});
      delivery.back().signal = signal(channel, true);
      if (!draining) {
        draining = true;
        drain_now = true;
      }
    }
    if (drain_now) drain();
  }
};

LedcFadeEngine::LedcFadeEngine() : impl_(new Impl) {}
LedcFadeEngine::~LedcFadeEngine() { delete impl_; }

bool LedcFadeEngine::Start(const Request& request) {
  if (request.channel >= kChannels || request.pin < 0 ||
      request.frequency_hz == 0 || request.resolution == 0 ||
      request.resolution > 20 ||
      request.start_duty > Period(request.resolution) ||
      request.target_duty > Period(request.resolution) ||
      request.duration_us >
          roo_testing_transducers::kMaxVoltageSignalDurationMicros)
    return false;
  const int64_t now = system_time_get_micros();
  if (request.duration_us > 0 &&
      now > std::numeric_limits<int64_t>::max() -
                static_cast<int64_t>(request.duration_us))
    return false;
  uint64_t generation;
  bool drain_now = false;
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    Impl::Channel& channel = impl_->channels[request.channel];
    if (channel.state != Impl::State::kIdle &&
        channel.state != Impl::State::kCancelled)
      return false;
    channel.request = request;
    channel.duty = request.start_duty;
    channel.fade_start_us = now;
    channel.state =
        request.duration_us == 0 || request.start_duty == request.target_duty
            ? Impl::State::kQueued
            : Impl::State::kArmed;
    generation = ++channel.generation;
    impl_->delivery.push_back({impl_->signal(channel, false), request.pin,
                               false, request.channel, generation});
    if (channel.state == Impl::State::kQueued) {
      channel.duty = request.target_duty;
      impl_->delivery.push_back({impl_->signal(channel, true), request.pin,
                                 true, request.channel, generation});
    }
    if (!impl_->draining) {
      impl_->draining = true;
      drain_now = true;
    }
  }
  if (request.duration_us > 0 && request.start_duty != request.target_duty) {
    const SystemTimeAlarmId id = ScheduleSystemTimeAlarm(
        now + request.duration_us, [this, c = request.channel, generation] {
          impl_->deadline(c, generation);
        });
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->channels[request.channel].generation == generation)
      impl_->channels[request.channel].alarm = id;
  }
  if (drain_now) impl_->drain();
  return true;
}

bool LedcFadeEngine::Cancel(uint8_t index) {
  if (index >= kChannels) return false;
  SystemTimeAlarmId alarm = 0;
  bool drain_now = false;
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    Impl::Channel& channel = impl_->channels[index];
    if (channel.state != Impl::State::kArmed &&
        channel.state != Impl::State::kQueued)
      return false;
    const int64_t elapsed = system_time_get_micros() - channel.fade_start_us;
    channel.duty = Materialize(channel.request, elapsed);
    channel.state = Impl::State::kCancelled;
    alarm = channel.alarm;
    ++channel.generation;
    const double duty =
        static_cast<double>(channel.duty) / Period(channel.request.resolution);
    impl_->delivery.push_back(
        {VoltageSignal::Square(
             0, VoltageDigitalHigh(), channel.request.frequency_hz,
             ConstantDuty{duty}, channel.request.carrier_origin_us,
             channel.request.pulse_start_phase, channel.request.inverted),
         channel.request.pin, false, index, channel.generation});
    if (!impl_->draining) {
      impl_->draining = true;
      drain_now = true;
    }
  }
  CancelSystemTimeAlarm(alarm);
  if (drain_now) impl_->drain();
  return true;
}

LedcFadeEngine::Snapshot LedcFadeEngine::snapshot(uint8_t index) const {
  if (index >= kChannels) return {};
  std::lock_guard<std::mutex> lock(impl_->mutex);
  const Impl::Channel& channel = impl_->channels[index];
  const uint32_t duty =
      channel.state == Impl::State::kArmed
          ? Materialize(channel.request,
                        system_time_get_micros() - channel.fade_start_us)
          : channel.duty;
  return {channel.generation, duty,
          channel.state == Impl::State::kArmed ||
              channel.state == Impl::State::kQueued ||
              channel.state == Impl::State::kPending,
          channel.state == Impl::State::kPending};
}

std::optional<LedcFadeEngine::Completion> LedcFadeEngine::TakeCompletion(
    uint8_t index) {
  if (index >= kChannels) return std::nullopt;
  std::lock_guard<std::mutex> lock(impl_->mutex);
  Impl::Channel& channel = impl_->channels[index];
  if (channel.state != Impl::State::kPending) return std::nullopt;
  Completion result{index, channel.generation, channel.duty};
  channel.state = Impl::State::kIdle;
  channel.alarm = 0;
  return result;
}
