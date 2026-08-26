#include "roo_testing/transducers/voltage/voltage_signal.h"

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "glog/logging.h"
#include "roo_testing/system/timer.h"

namespace roo_testing_transducers {
namespace {

constexpr double kMicrosPerSecond = 1000000.0;

double NormalizeCycles(double cycles) {
  // Keep phases in [0, 1) so all shapes share stable half-open boundaries,
  // including when callers provide a negative phase offset.
  const double normalized = std::fmod(cycles, 1.0);
  return normalized < 0 ? normalized + 1.0 : normalized;
}

uint64_t UnsignedDistance(int64_t lhs, int64_t rhs) {
  // Convert before subtraction so opposite int64_t endpoints cannot overflow.
  return lhs >= rhs ? static_cast<uint64_t>(lhs) - static_cast<uint64_t>(rhs)
                    : static_cast<uint64_t>(rhs) - static_cast<uint64_t>(lhs);
}

double CarrierPhase(const PeriodicVoltageCarrier& carrier, double offset,
                    int64_t uptime_us) {
  if (uptime_us == carrier.carrier_origin_uptime_us) return offset;
  // Accumulate the product modulo one cycle. This preserves fractional phase
  // across huge uptime differences without one overflowing large multiply.
  double addend =
      std::fmod(carrier.frequency_hz, kMicrosPerSecond) / kMicrosPerSecond;
  double delta = 0;
  uint64_t distance =
      UnsignedDistance(uptime_us, carrier.carrier_origin_uptime_us);
  while (distance != 0) {
    if ((distance & 1) != 0) delta = NormalizeCycles(delta + addend);
    addend = NormalizeCycles(addend + addend);
    distance >>= 1;
  }
  return NormalizeCycles(
      offset + (uptime_us > carrier.carrier_origin_uptime_us ? delta : -delta));
}

void ValidateCarrier(float low, float high, double frequency_hz) {
  CHECK(std::isfinite(low));
  CHECK(std::isfinite(high));
  CHECK(std::isfinite(frequency_hz));
  CHECK_LE(low, high);
  CHECK_GT(frequency_hz, 0);
}

void ValidateDuty(double duty) {
  CHECK(std::isfinite(duty));
  CHECK_GE(duty, 0);
  CHECK_LE(duty, 1);
}

void ValidateDutyProfile(const DutyProfile& profile) {
  std::visit(
      [](const auto& value) {
        if constexpr (std::is_same_v<std::decay_t<decltype(value)>,
                                     ConstantDuty>) {
          ValidateDuty(value.duty);
        } else {
          ValidateDuty(value.start_duty);
          ValidateDuty(value.target_duty);
          CHECK_LE(value.duration_us, kMaxVoltageSignalDurationMicros);
        }
      },
      profile);
}

double ActiveDuty(const DutyProfile& profile, int64_t uptime_us) {
  return std::visit(
      [uptime_us](const auto& value) -> double {
        if constexpr (std::is_same_v<std::decay_t<decltype(value)>,
                                     ConstantDuty>) {
          return value.duty;
        } else {
          if (uptime_us < value.fade_start_uptime_us) return value.start_duty;
          if (value.duration_us == 0) return value.target_duty;
          // The preceding ordering check makes this an overflow-safe elapsed
          // duration and avoids constructing an unchecked fade deadline.
          const uint64_t elapsed =
              UnsignedDistance(uptime_us, value.fade_start_uptime_us);
          if (elapsed >= value.duration_us) return value.target_duty;
          return value.start_duty + (value.target_duty - value.start_duty) *
                                        static_cast<double>(elapsed) /
                                        static_cast<double>(value.duration_us);
        }
      },
      profile);
}

VoltageAnalysis MakeAnalysis(double dc, double rms, double ac_rms,
                             double minimum, double maximum) {
  // Derive peak metrics from the same engineering bounds for every waveform.
  return {static_cast<float>(dc),
          static_cast<float>(rms),
          static_cast<float>(ac_rms),
          static_cast<float>(minimum),
          static_cast<float>(maximum),
          static_cast<float>(maximum - minimum),
          static_cast<float>(std::max(std::abs(minimum), std::abs(maximum))),
          static_cast<float>(std::max(maximum - dc, dc - minimum))};
}

bool SameCarrier(const PeriodicVoltageCarrier& lhs,
                 const PeriodicVoltageCarrier& rhs) {
  return lhs.low_voltage == rhs.low_voltage &&
         lhs.high_voltage == rhs.high_voltage &&
         lhs.frequency_hz == rhs.frequency_hz &&
         lhs.carrier_origin_uptime_us == rhs.carrier_origin_uptime_us;
}

bool SameDuty(const DutyProfile& lhs, const DutyProfile& rhs) {
  if (lhs.index() != rhs.index()) return false;
  if (const ConstantDuty* left = std::get_if<ConstantDuty>(&lhs)) {
    return left->duty == std::get<ConstantDuty>(rhs).duty;
  }
  const LinearDutyFade& left = std::get<LinearDutyFade>(lhs);
  const LinearDutyFade& right = std::get<LinearDutyFade>(rhs);
  return left.start_duty == right.start_duty &&
         left.target_duty == right.target_duty &&
         left.fade_start_uptime_us == right.fade_start_uptime_us &&
         left.duration_us == right.duration_us;
}

}  // namespace

VoltageSignal VoltageSignal::Constant(float voltage) {
  CHECK(std::isfinite(voltage));
  return VoltageSignal(ConstantVoltageSpec{voltage});
}

VoltageSignal VoltageSignal::Sine(float low, float high, double frequency_hz,
                                  int64_t origin_us, double phase_cycles) {
  ValidateCarrier(low, high, frequency_hz);
  CHECK(std::isfinite(phase_cycles));
  return VoltageSignal(SineVoltageSpec{{low, high, frequency_hz, origin_us},
                                       NormalizeCycles(phase_cycles)});
}

VoltageSignal VoltageSignal::Triangle(float low, float high,
                                      double frequency_hz, int64_t origin_us,
                                      double phase_cycles) {
  ValidateCarrier(low, high, frequency_hz);
  CHECK(std::isfinite(phase_cycles));
  return VoltageSignal(TriangleVoltageSpec{{low, high, frequency_hz, origin_us},
                                           NormalizeCycles(phase_cycles)});
}

VoltageSignal VoltageSignal::Sawtooth(float low, float high,
                                      double frequency_hz, int64_t origin_us,
                                      double phase_cycles,
                                      SawtoothDirection direction) {
  ValidateCarrier(low, high, frequency_hz);
  CHECK(std::isfinite(phase_cycles));
  CHECK(direction == SawtoothDirection::kRising ||
        direction == SawtoothDirection::kFalling);
  return VoltageSignal(SawtoothVoltageSpec{{low, high, frequency_hz, origin_us},
                                           NormalizeCycles(phase_cycles),
                                           direction});
}

VoltageSignal VoltageSignal::Square(float low, float high, double frequency_hz,
                                    DutyProfile duty, int64_t origin_us,
                                    double pulse_start_cycles, bool inverted) {
  ValidateCarrier(low, high, frequency_hz);
  CHECK(std::isfinite(pulse_start_cycles));
  ValidateDutyProfile(duty);
  return VoltageSignal(SquareVoltageSpec{{low, high, frequency_hz, origin_us},
                                         std::move(duty),
                                         NormalizeCycles(pulse_start_cycles),
                                         inverted});
}

VoltageSignalKind VoltageSignal::kind() const {
  switch (spec_.index()) {
    case 0:
      return VoltageSignalKind::kConstant;
    case 1:
      return VoltageSignalKind::kSine;
    case 2:
      return VoltageSignalKind::kTriangle;
    case 3:
      return VoltageSignalKind::kSawtooth;
    default:
      return VoltageSignalKind::kSquare;
  }
}

double VoltageSignal::activeDutyAtUptimeMicros(int64_t uptime_us) const {
  const SquareVoltageSpec* square = std::get_if<SquareVoltageSpec>(&spec_);
  CHECK(square != nullptr);
  return ActiveDuty(square->duty, uptime_us);
}

double VoltageSignal::highFractionAtUptimeMicros(int64_t uptime_us) const {
  const SquareVoltageSpec* square = std::get_if<SquareVoltageSpec>(&spec_);
  CHECK(square != nullptr);
  const double duty = ActiveDuty(square->duty, uptime_us);
  return square->inverted ? 1.0 - duty : duty;
}

float VoltageSignal::voltageAtUptimeMicros(int64_t uptime_us) const {
  return std::visit(
      [uptime_us](const auto& value) -> float {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, ConstantVoltageSpec>) {
          return value.voltage;
        } else if constexpr (std::is_same_v<T, SineVoltageSpec>) {
          const double phase =
              CarrierPhase(value.carrier, value.phase_offset_cycles, uptime_us);
          const double center =
              (value.carrier.low_voltage + value.carrier.high_voltage) / 2;
          const double amplitude =
              (value.carrier.high_voltage - value.carrier.low_voltage) / 2;
          return static_cast<float>(center +
                                    amplitude * std::sin(2 * M_PI * phase));
        } else if constexpr (std::is_same_v<T, TriangleVoltageSpec>) {
          const double phase =
              CarrierPhase(value.carrier, value.phase_offset_cycles, uptime_us);
          return static_cast<float>(
              value.carrier.low_voltage +
              (value.carrier.high_voltage - value.carrier.low_voltage) *
                  (1 - std::abs(2 * phase - 1)));
        } else if constexpr (std::is_same_v<T, SawtoothVoltageSpec>) {
          const double phase =
              CarrierPhase(value.carrier, value.phase_offset_cycles, uptime_us);
          const double span =
              value.carrier.high_voltage - value.carrier.low_voltage;
          return static_cast<float>(
              value.direction == SawtoothDirection::kRising
                  ? value.carrier.low_voltage + span * phase
                  : value.carrier.high_voltage - span * phase);
        } else {
          const double phase = CarrierPhase(value.carrier, 0, uptime_us);
          const double pulse_phase =
              NormalizeCycles(phase - value.pulse_start_phase_cycles);
          // A half-open active interval gives stable duty boundaries and makes
          // 0% and 100% duty naturally produce constant output.
          const bool active = pulse_phase < ActiveDuty(value.duty, uptime_us);
          const bool high = value.inverted ? !active : active;
          return high ? value.carrier.high_voltage : value.carrier.low_voltage;
        }
      },
      spec_);
}

bool operator==(const VoltageSignal& lhs, const VoltageSignal& rhs) {
  if (lhs.spec_.index() != rhs.spec_.index()) return false;
  if (const ConstantVoltageSpec* left =
          std::get_if<ConstantVoltageSpec>(&lhs.spec_))
    return left->voltage == std::get<ConstantVoltageSpec>(rhs.spec_).voltage;
  if (const SineVoltageSpec* left = std::get_if<SineVoltageSpec>(&lhs.spec_))
    return SameCarrier(left->carrier,
                       std::get<SineVoltageSpec>(rhs.spec_).carrier) &&
           left->phase_offset_cycles ==
               std::get<SineVoltageSpec>(rhs.spec_).phase_offset_cycles;
  if (const TriangleVoltageSpec* left =
          std::get_if<TriangleVoltageSpec>(&lhs.spec_))
    return SameCarrier(left->carrier,
                       std::get<TriangleVoltageSpec>(rhs.spec_).carrier) &&
           left->phase_offset_cycles ==
               std::get<TriangleVoltageSpec>(rhs.spec_).phase_offset_cycles;
  if (const SawtoothVoltageSpec* left =
          std::get_if<SawtoothVoltageSpec>(&lhs.spec_)) {
    const SawtoothVoltageSpec& right = std::get<SawtoothVoltageSpec>(rhs.spec_);
    return SameCarrier(left->carrier, right.carrier) &&
           left->phase_offset_cycles == right.phase_offset_cycles &&
           left->direction == right.direction;
  }
  const SquareVoltageSpec& left = std::get<SquareVoltageSpec>(lhs.spec_);
  const SquareVoltageSpec& right = std::get<SquareVoltageSpec>(rhs.spec_);
  return SameCarrier(left.carrier, right.carrier) &&
         SameDuty(left.duty, right.duty) &&
         left.pulse_start_phase_cycles == right.pulse_start_phase_cycles &&
         left.inverted == right.inverted;
}

VoltageAnalysis AnalyzeVoltage(const VoltageSignal& signal, int64_t uptime_us) {
  return std::visit(
      [&signal, uptime_us](const auto& value) -> VoltageAnalysis {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, ConstantVoltageSpec>) {
          return MakeAnalysis(value.voltage, std::abs(value.voltage), 0,
                              value.voltage, value.voltage);
        } else {
          const double low = value.carrier.low_voltage;
          const double high = value.carrier.high_voltage;
          const double center = (low + high) / 2;
          const double amplitude = (high - low) / 2;
          if constexpr (std::is_same_v<T, SineVoltageSpec>) {
            return MakeAnalysis(
                center, std::sqrt(center * center + amplitude * amplitude / 2),
                amplitude / std::sqrt(2), low, high);
          } else if constexpr (std::is_same_v<T, TriangleVoltageSpec> ||
                               std::is_same_v<T, SawtoothVoltageSpec>) {
            return MakeAnalysis(
                center, std::sqrt((low * low + low * high + high * high) / 3),
                (high - low) / std::sqrt(12), low, high);
          } else {
            const double high_fraction =
                signal.highFractionAtUptimeMicros(uptime_us);
            const double dc = (1 - high_fraction) * low + high_fraction * high;
            const double rms = std::sqrt((1 - high_fraction) * low * low +
                                         high_fraction * high * high);
            const double ac_rms =
                (high - low) * std::sqrt(high_fraction * (1 - high_fraction));
            if (high_fraction == 0)
              return MakeAnalysis(dc, rms, ac_rms, low, low);
            if (high_fraction == 1)
              return MakeAnalysis(dc, rms, ac_rms, high, high);
            return MakeAnalysis(dc, rms, ac_rms, low, high);
          }
        }
      },
      signal.spec());
}

VoltageAnalysis AnalyzeVoltageNow(const VoltageSignal& signal) {
  return AnalyzeVoltage(signal, system_time_get_micros());
}
float AverageDcVoltage(const VoltageSignal& signal, int64_t uptime_us) {
  return AnalyzeVoltage(signal, uptime_us).dc_voltage;
}
float AverageDcVoltage(const VoltageSignal& signal) {
  return AnalyzeVoltageNow(signal).dc_voltage;
}

}  // namespace roo_testing_transducers
