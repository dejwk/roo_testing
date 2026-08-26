#pragma once

#include <cstdint>
#include <variant>

namespace roo_testing_transducers {

/// Identifies the waveform represented by a VoltageSignal.
enum class VoltageSignalKind {
  kConstant,
  kSine,
  kTriangle,
  kSawtooth,
  kSquare
};

/// Selects the direction of a sawtooth ramp.
enum class SawtoothDirection { kRising, kFalling };

/// Largest fade duration whose integral microseconds remain exactly
/// representable.
inline constexpr uint64_t kMaxVoltageSignalDurationMicros = uint64_t{1} << 53;

/// A PWM duty that does not vary with uptime.
struct ConstantDuty {
  double duty;
};

/// A PWM duty linearly interpolated over an absolute-uptime interval.
struct LinearDutyFade {
  double start_duty;
  double target_duty;
  int64_t fade_start_uptime_us;
  uint64_t duration_us;
};

/// Describes the duty envelope used by a square signal.
using DutyProfile = std::variant<ConstantDuty, LinearDutyFade>;

/// Common configuration for a periodic voltage carrier.
struct PeriodicVoltageCarrier {
  float low_voltage;
  float high_voltage;
  double frequency_hz;
  int64_t carrier_origin_uptime_us;
};

/// Configuration for a constant signal.
struct ConstantVoltageSpec {
  float voltage;
};

/// Configuration for a sine signal.
struct SineVoltageSpec {
  PeriodicVoltageCarrier carrier;
  double phase_offset_cycles;
};

/// Configuration for a triangle signal.
struct TriangleVoltageSpec {
  PeriodicVoltageCarrier carrier;
  double phase_offset_cycles;
};

/// Configuration for a sawtooth signal.
struct SawtoothVoltageSpec {
  PeriodicVoltageCarrier carrier;
  double phase_offset_cycles;
  SawtoothDirection direction;
};

/// Configuration for a square/PWM signal.
struct SquareVoltageSpec {
  PeriodicVoltageCarrier carrier;
  DutyProfile duty;
  double pulse_start_phase_cycles;
  bool inverted;
};

/// Read-only normalized specification held by a VoltageSignal.
using VoltageSignalSpec =
    std::variant<ConstantVoltageSpec, SineVoltageSpec, TriangleVoltageSpec,
                 SawtoothVoltageSpec, SquareVoltageSpec>;

/// Exact local analysis of a voltage signal at one uptime.
struct VoltageAnalysis {
  float dc_voltage;
  float rms_voltage;
  float ac_rms_voltage;
  float minimum_voltage;
  float maximum_voltage;
  float peak_to_peak_voltage;
  float absolute_peak_voltage;
  float ac_peak_voltage;
};

/// Immutable, value-owned voltage waveform evaluated against emulator uptime.
class VoltageSignal {
 public:
  /// Creates a constant voltage signal.
  static VoltageSignal Constant(float voltage);

  /// Creates a sine signal with phase zero at its rising midpoint.
  static VoltageSignal Sine(float low, float high, double frequency_hz,
                            int64_t origin_us, double phase_cycles = 0);

  /// Creates a triangle signal beginning at its low rail.
  static VoltageSignal Triangle(float low, float high, double frequency_hz,
                                int64_t origin_us, double phase_cycles = 0);

  /// Creates a rising or falling sawtooth signal.
  static VoltageSignal Sawtooth(
      float low, float high, double frequency_hz, int64_t origin_us,
      double phase_cycles = 0,
      SawtoothDirection direction = SawtoothDirection::kRising);

  /// Creates a square/PWM signal with a constant or linear duty envelope.
  static VoltageSignal Square(float low, float high, double frequency_hz,
                              DutyProfile duty, int64_t origin_us,
                              double pulse_start_cycles = 0,
                              bool inverted = false);

  /// Returns the active specification's waveform kind.
  VoltageSignalKind kind() const;

  /// Returns the normalized specification, valid for this signal's lifetime.
  const VoltageSignalSpec& spec() const { return spec_; }

  /// Returns instantaneous voltage at the supplied absolute emulator uptime.
  float voltageAtUptimeMicros(int64_t uptime_us) const;

  /// Returns pre-inversion active duty; CHECKs that this is a square signal.
  double activeDutyAtUptimeMicros(int64_t uptime_us) const;

  /// Returns physical high-rail fraction; CHECKs that this is a square signal.
  double highFractionAtUptimeMicros(int64_t uptime_us) const;

  /// Compares normalized specifications structurally.
  friend bool operator==(const VoltageSignal& lhs, const VoltageSignal& rhs);

  /// Compares normalized specifications structurally.
  friend bool operator!=(const VoltageSignal& lhs, const VoltageSignal& rhs) {
    return !(lhs == rhs);
  }

 private:
  explicit VoltageSignal(VoltageSignalSpec spec) : spec_(std::move(spec)) {}
  VoltageSignalSpec spec_;
};

/// Returns local DC, RMS, and peak properties at an explicit uptime.
VoltageAnalysis AnalyzeVoltage(const VoltageSignal& signal, int64_t uptime_us);

/// Returns local DC, RMS, and peak properties at current emulator uptime.
VoltageAnalysis AnalyzeVoltageNow(const VoltageSignal& signal);

/// Returns the local DC component at an explicit uptime.
float AverageDcVoltage(const VoltageSignal& signal, int64_t uptime_us);

/// Returns the local DC component at current emulator uptime.
float AverageDcVoltage(const VoltageSignal& signal);

}  // namespace roo_testing_transducers
