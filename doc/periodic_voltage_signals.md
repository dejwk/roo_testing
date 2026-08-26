# Periodic voltage signals

Status: Proposed

## Objective

Represent constant and periodic voltage outputs as inspectable, time-aware
values, propagate them through voltage sinks and fake GPIO, and provide exact
DC, RMS, and peak-related analysis.

## Motivation

[`VoltageSink`](../roo_testing/transducers/voltage/voltage.h) currently receives
only a scalar voltage. That loses the frequency, phase, shape, and duty cycle of
an AC or PWM output. Tests and emulated devices consequently cannot choose
between instantaneous voltage, average voltage, or RMS voltage according to
their physical behavior.

## Background

`VoltageSink` is the push side of the voltage transducer API. Its built-in
implementations, `SimpleVoltageSink` and `SimpleDigitalSink`, retain the last
scalar written to them. [`FakeGpioPin`](../roo_testing/buses/gpio/fake_gpio.h)
also stores and forwards a scalar. `VoltageSource` is already pull-based and
can return a time-varying sample through a callback; this design does not change
it.

The fake platform has one monotonic uptime, returned by
[`system_time_get_micros()`](../roo_testing/system/timer.h). Arduino `micros()`
and `millis()`, and ESP-IDF `esp_timer_get_time()`, use the same clock. It is not
Unix time and this design introduces no second clock or time epoch.

## Requirements

1. A voltage output can describe a constant, sine, triangle, rising or falling
   sawtooth, or square/PWM waveform.
2. A square waveform can have constant duty or a duty that changes linearly
   over a finite uptime interval. This generic envelope is needed by the
   dependent [LEDC design](ledc_voltage_emulation.md), but contains no LEDC
   state or behavior.
3. A signal is immutable, copyable, and safe to retain after its producer is
   reconfigured or destroyed.
4. Signal phase is based only on absolute emulator uptime and remains defined
   for every finite positive frequency and every signed 64-bit time/origin.
5. Callers can inspect every configured field and sample the instantaneous
   voltage at an explicit uptime.
6. Global analysis functions report DC voltage, total RMS, AC RMS, minimum,
   maximum, peak-to-peak, absolute peak, and peak AC excursion with unambiguous
   definitions.
7. Voltage sinks and fake GPIO preserve the complete signal. Scalar writes
   remain convenient constant-signal wrappers.
8. Built-in sinks define whether legacy scalar/digital observations use an
   instantaneous sample or the local DC component.
9. Signal assignment invokes at most one sink notification. Periodic edges are
   never scheduled as callbacks or emulator events.
10. All roo-owned call sites and subclasses are migrated. Source compatibility
    for external subclasses is not required.

### Out of scope

- Electrical network solving, impedance, capacitance, contention, and a
  high-impedance signal state.
- Arbitrary sampled/noisy waveforms.
- Analysis over an arbitrary observation window.
- Inferring power without a load model.
- A time-aware replacement for `VoltageSource`.
- Any driver-specific behavior, including LEDC configuration, blocking,
  completion callbacks, and alarm scheduling.

## Design Overview

Add an immutable `VoltageSignal` value whose specification is either constant
or one of the supported carrier shapes. A periodic carrier stores rails,
frequency, and its phase-zero uptime. Shape-specific data adds phase placement,
sawtooth direction, or square duty and inversion. A square can use a
`LinearDutyFade`; the carrier remains periodic while its duty envelope changes
over time, so the complete faded signal is not itself periodic.

Sampling is a pure operation at an explicit uptime. Convenience `...Now()`
operations read the global fake uptime, but signals never advance time and
never retain a clock reference. Analysis is analytical rather than sampled and
summarizes one local carrier cycle at the requested uptime.

`VoltageSink` makes the complete signal its required virtual contract. Built-in
sinks and fake GPIO store a signal by value and perform callbacks outside their
state mutex. This retains metadata and permits re-entrant observation without
creating one notification per carrier edge.

| Requirements | Design element |
| --- | --- |
| 1-5 | Validated `VoltageSignal` specification and explicit-uptime sampling |
| 6 | One analytical dispatch returning all named metrics |
| 7-8 | Signal-based sink contract with documented legacy views |
| 9 | Lazy sampling; one notification per signal assignment |
| 10 | Breaking migration of all roo-owned sinks, pin adapters, and callers |

## Design Details

### Time and phase

Each periodic signal stores `carrier_origin_uptime_us`, the absolute fake
uptime at which carrier phase is zero. A linear duty fade separately stores
`fade_start_uptime_us`; changing duty does not restart the carrier.

For frequency `f`, phase offset `q`, carrier origin `o`, and requested uptime
`t`, normalized phase is conceptually:

```text
p(t) = frac(q + f * (t - o) / 1,000,000)
frac(x) = x - floor(x)
```

All phase arithmetic uses `double`. The implementation must not directly
multiply an enormous time delta by frequency. It first determines the sign and
unsigned magnitude `N = abs(t-o)` without signed overflow, reduces frequency
modulo 1,000,000 Hz, and accumulates the fractional product with binary
double-and-add:

```text
addend = fmod(f, 1,000,000) / 1,000,000
phase_delta = 0
while N != 0:
  if N & 1: phase_delta = frac(phase_delta + addend)
  addend = frac(addend + addend)
  N >>= 1
p = frac(q + sign(t-o) * phase_delta)
```

The loop has at most 64 iterations and keeps every intermediate phase below
one. Floating-point rounding remains, but integer-cycle magnitude cannot erase
the fractional phase or overflow the calculation.

### Signal model and validation

The public read-only specification is a `std::variant`. Factories are the only
construction path and normalize phase fields. `kind()` is derived from the
active variant alternative, so there is no duplicate discriminator.

Factories enforce these invariants with `CHECK`:

- voltages, frequencies, duties, and phases are finite;
- `low_voltage <= high_voltage`;
- periodic frequency is greater than zero;
- duties are in `[0, 1]`;
- sawtooth direction is a named enumerator; and
- fade duration is no greater than `2^53` microseconds, preserving exact
  integral-microsecond conversion to `double`.

Zero-duration fades are valid and take the target duty immediately. Values
outside the usual 0-3.3 V GPIO range remain valid generic voltage signals. A
0%/100% square or equal rails remain square specifications so inspection does
not lose their configured frequency or shape.

Invalid factory arguments are programmer errors. Public drivers validate their
own arguments and return their documented errors before calling a factory.

### Sampling

Let `L` and `H` be the configured rails, `A = (H-L)/2`, `C = (L+H)/2`, and `p`
the normalized phase.

| Shape | Instantaneous voltage |
| --- | --- |
| Constant | Configured voltage |
| Sine | `C + A*sin(2*pi*p)` |
| Triangle | `L + (H-L)*(1-abs(2*p-1))` |
| Rising sawtooth | `L + (H-L)*p` |
| Falling sawtooth | `H - (H-L)*p` |

Sine starts at its rising midpoint. Triangle starts at `L` and reaches `H` at
phase 0.5. Sawtooth uses a half-open cycle, so one configured rail is an
engineering bound rather than an attained sample.

A square has a single `pulse_start_phase_cycles` placement rather than the
general phase-offset field. Compute `pulse_phase = frac(p - pulse_start)` and
treat `pulse_phase < active_duty` as the half-open active interval. The active
interval emits `H` unless `inverted` is true. `activeDutyAtUptimeMicros()`
returns the pre-inversion active fraction; `highFractionAtUptimeMicros()`
returns the fraction physically emitting `H`.

Linear duty is evaluated piecewise before division:

```text
if t < start:             duty = start_duty
else if duration == 0:   duty = target_duty
else if t-start >= duration: duty = target_duty
else: duty = start_duty
             + (target_duty-start_duty) * (t-start) / duration
```

After proving `t >= start`, form elapsed time as an unsigned difference. Do not
form unchecked `start + duration`. Convert elapsed and duration to `double`
before division.

### Analysis

Analysis means one local carrier cycle. For a faded square, freeze duty at the
requested uptime and analyze one cycle at that duty. It is not an average over
the fade interval. For a square, `D` below is the physical high fraction after
inversion.

| Signal | DC | Total RMS | AC RMS |
| --- | --- | --- | --- |
| Constant `V` | `V` | `abs(V)` | `0` |
| Sine | `C` | `sqrt(C*C + A*A/2)` | `A/sqrt(2)` |
| Triangle/sawtooth | `C` | `sqrt((L*L + L*H + H*H)/3)` | `(H-L)/sqrt(12)` |
| Square | `(1-D)*L + D*H` | `sqrt((1-D)*L*L + D*H*H)` | `(H-L)*sqrt(D*(1-D))` |

Minimum and maximum are engineering bounds. A non-degenerate square uses both
rails; at `D == 0` or `D == 1`, all metrics use only the reached level. Derive
the remaining metrics from those bounds:

```text
peak_to_peak = maximum - minimum
absolute_peak = max(abs(minimum), abs(maximum))
ac_peak = max(maximum - dc, dc - minimum)
```

All calculations use `double` internally and convert at the public boundary.
Direct per-shape AC RMS formulas avoid cancellation. `AverageDcVoltage`
delegates to `AnalyzeVoltage`; there is one shape-dispatch implementation.

For a 0-3.3 V, 50% square this yields 1.65 V DC, approximately 2.33345 V total
RMS, 1.65 V AC RMS, and 1.65 V AC peak.

### Sinks

The required virtual becomes `write(const VoltageSignal&)`; `write(float)` is a
non-virtual constant wrapper. There is deliberately no scalar virtual fallback
that silently discards waveform metadata. Derived classes that also expose the
scalar overload add `using VoltageSink::write`.

`SimpleVoltageSink` retains the last complete signal. `voltage()` is the
current local DC value, while `sample()` and `sampleAtUptimeMicros()` explicitly
mean instantaneous voltage. Its existing float callback receives local DC at
assignment time. `WithSignalCallback` provides a separately named signal
callback so `nullptr` and generic lambdas are not ambiguous.

`SimpleDigitalSink` also retains the signal. Its legacy `value()` and callback
classify current local DC with `DigitalLevelFromVoltage`. Explicit
`instantaneousValue...()` operations classify a carrier sample. No edge
callbacks are generated.

Before any write, signal access returns `std::nullopt`;
`SimpleVoltageSink` scalar/sample views return NaN and `SimpleDigitalSink`
returns `kDigitalUndef`. Adding mutexes makes both sink types explicitly
non-copyable and non-movable. State is stored before a callback, and callbacks
run after releasing the mutex.

### Fake GPIO propagation

`FakeGpioPin` stores the complete signal and retains its existing unwritten
state. Scalar and digital writes construct constants. `readAtUptimeMicros(t)`
samples retained output at `t`; `read()` uses current uptime. `lastSignal()`
returns a value copy, while legacy `last_written()` returns current local DC.

The output and input/output adapters forward the descriptor unchanged through
`onWrite(const VoltageSignal&)`. Input adapters still delegate explicit-time
reads to the attached legacy `VoltageSource::read()` and therefore ignore the
timestamp. They never fall back to stored output. While updating adapters, fix
`FakeGpioInterface::attach(VoltageIO&)` to instantiate `InputOutput` rather
than `Output`.

Pins protect retained state with a mutex, copy it under lock, and sample or
invoke external code after unlock. Topology mutation and references returned by
`get()` retain their existing external-synchronization contract.

## Proposed API

The API lives in `roo_testing_transducers` under the existing voltage package.

```cpp
enum class VoltageSignalKind {
  kConstant, kSine, kTriangle, kSawtooth, kSquare
};
enum class SawtoothDirection { kRising, kFalling };

inline constexpr uint64_t kMaxVoltageSignalDurationMicros = uint64_t{1} << 53;

struct ConstantDuty { double duty; };
struct LinearDutyFade {
  double start_duty;
  double target_duty;
  int64_t fade_start_uptime_us;
  uint64_t duration_us;
};
using DutyProfile = std::variant<ConstantDuty, LinearDutyFade>;

struct PeriodicVoltageCarrier {
  float low_voltage;
  float high_voltage;
  double frequency_hz;
  int64_t carrier_origin_uptime_us;
};
struct ConstantVoltageSpec { float voltage; };
struct SineVoltageSpec {
  PeriodicVoltageCarrier carrier;
  double phase_offset_cycles;
};
struct TriangleVoltageSpec {
  PeriodicVoltageCarrier carrier;
  double phase_offset_cycles;
};
struct SawtoothVoltageSpec {
  PeriodicVoltageCarrier carrier;
  double phase_offset_cycles;
  SawtoothDirection direction;
};
struct SquareVoltageSpec {
  PeriodicVoltageCarrier carrier;
  DutyProfile duty;
  double pulse_start_phase_cycles;
  bool inverted;
};
using VoltageSignalSpec = std::variant<
    ConstantVoltageSpec, SineVoltageSpec, TriangleVoltageSpec,
    SawtoothVoltageSpec, SquareVoltageSpec>;

class VoltageSignal {
 public:
  static VoltageSignal Constant(float voltage);
  static VoltageSignal Sine(float low, float high, double frequency_hz,
                            int64_t origin_us, double phase_cycles = 0);
  static VoltageSignal Triangle(float low, float high, double frequency_hz,
                                int64_t origin_us, double phase_cycles = 0);
  static VoltageSignal Sawtooth(
      float low, float high, double frequency_hz, int64_t origin_us,
      double phase_cycles = 0,
      SawtoothDirection direction = SawtoothDirection::kRising);
  static VoltageSignal Square(float low, float high, double frequency_hz,
                              DutyProfile duty, int64_t origin_us,
                              double pulse_start_cycles = 0,
                              bool inverted = false);

  VoltageSignal(const VoltageSignal&) = default;
  VoltageSignal(VoltageSignal&&) = default;
  VoltageSignal& operator=(const VoltageSignal&) = default;
  VoltageSignal& operator=(VoltageSignal&&) = default;

  VoltageSignalKind kind() const;
  const VoltageSignalSpec& spec() const;
  float voltageAtUptimeMicros(int64_t uptime_us) const;
  double activeDutyAtUptimeMicros(int64_t uptime_us) const;
  double highFractionAtUptimeMicros(int64_t uptime_us) const;

  friend bool operator==(const VoltageSignal&, const VoltageSignal&);
  friend bool operator!=(const VoltageSignal&, const VoltageSignal&);

 private:
  explicit VoltageSignal(VoltageSignalSpec spec);
  VoltageSignalSpec spec_;
};

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

VoltageAnalysis AnalyzeVoltage(const VoltageSignal&, int64_t uptime_us);
VoltageAnalysis AnalyzeVoltageNow(const VoltageSignal&);
float AverageDcVoltage(const VoltageSignal&, int64_t uptime_us);
float AverageDcVoltage(const VoltageSignal&);  // Uses current uptime.

class VoltageSink : public Transducer {
 public:
  void write(float voltage) { write(VoltageSignal::Constant(voltage)); }
  virtual void write(const VoltageSignal& signal) = 0;
};
```

`spec()` returns a const reference valid for the signal's lifetime. Equality is
structural after factory normalization; numeric-result tests still use an
appropriate tolerance. The square-only duty accessors `CHECK` their kind.

The simple sinks add these operations to their existing scalar views:

```cpp
// SimpleVoltageSink
using VoltageSink::write;
std::optional<VoltageSignal> signal() const;
float sampleAtUptimeMicros(int64_t uptime_us) const;
float sample() const;
float averageDcVoltageAtUptimeMicros(int64_t uptime_us) const;
static SimpleVoltageSink WithSignalCallback(
    std::string name,
    std::function<void(const VoltageSignal&)> callback);

// SimpleDigitalSink
using VoltageSink::write;
std::optional<VoltageSignal> signal() const;
DigitalLevel instantaneousValueAtUptimeMicros(int64_t uptime_us) const;
DigitalLevel instantaneousValue() const;
static SimpleDigitalSink WithSignalCallback(
    std::string name,
    std::function<void(const VoltageSignal&)> callback);
```

`FakeGpioPin` exposes `write(const VoltageSignal&)`,
`readAtUptimeMicros()`, and `lastSignal()` in addition to its constant wrappers.

The complete API lands with all supported shapes. There is no partially
implemented shape or interim fallback.

## Implementation Plan

Authoring reference: follow this repository's
[design-authoring guidance](../.github/instructions/embedded-design-doc-authoring.instructions.md)
and the conventions in adjacent voltage, GPIO, and test code; roo_testing has
no separate code-authoring guide at the time of this proposal.

### Phase 1: Signal values, sampling, and analysis

Add `voltage_signal.h/.cpp`, export them from the voltage BUILD target, and add
focused unit tests for factories, phase arithmetic, all shapes, duty envelopes,
inspection/equality, and analytical metrics.

Proposed commit: `Add time-aware voltage signal model`

Validation: build and run the voltage tests, including death tests and extreme
time/frequency cases.

### Phase 2: Signal-aware sinks and roo migration

Change the sink virtual contract, implement retained signal state and callbacks
in both simple sinks, and migrate every roo-owned derived class and scalar call
site. Include compile coverage for all affected libraries and tests for
pre-write and re-entrant callback behavior.

Proposed commit: `Make voltage sinks preserve signal metadata`

Validation: run voltage/transducer tests and build all reverse dependencies of
the voltage package.

### Phase 3: Fake GPIO propagation

Store, sample, and forward signals in fake GPIO; add explicit-time reads and
exact inspection; correct the `VoltageIO` adapter; update BUILD dependencies and
GPIO tests in the same commit.

Proposed commit: `Propagate voltage signals through fake GPIO`

Validation: run GPIO and voltage tests, then the full roo_testing test suite.

## Testing Plan

The voltage tests cover every waveform at cardinal phases and cycle boundaries,
negative relative time, extreme finite frequency/time separation, half-open
square boundaries, inversion, 0/100% duty, and linear fades before, during, and
after their interval. They also verify structural inspection and every
analytical formula, including the 0-3.3 V PWM sanity values.

Sink and GPIO tests cover scalar wrappers, unwritten behavior, local-DC versus
instantaneous views, descriptor-preserving forwarding, callback ordering and
re-entry, attached legacy-source behavior, and `VoltageIO` routing. Repository
build coverage proves that every roo-owned subclass and caller migrated to the
breaking virtual contract.

## Caveats

Instantaneous sampling has one-microsecond time resolution. A carrier whose
sample phases align to that grid can alias; a 1 MHz carrier sampled at integer
microseconds is phase-locked. Analytical results remain exact for the declared
shape.

The fade envelope is ideal and continuous. Hardware drivers can expose integer
duty steps and rounded durations; the dependent LEDC design defines its
separate integer readback behavior.

### Rejected Alternatives

#### Keep `write(float)` as the virtual contract

A default signal-to-DC adapter would preserve source compatibility but silently
destroy the metadata this feature is intended to expose. The design accepts a
roo-wide source break instead.

#### Schedule waveform edges

Per-edge scheduling makes high-frequency PWM impractical and does not help
analytical consumers. Lazy explicit-time sampling provides deterministic tests
without event volume.

#### Use a polymorphic waveform hierarchy

Heap-owned implementations complicate copying, inspection, equality, and
lifetime. A closed specification variant matches the intentionally finite
initial shape set.

#### Estimate analysis by sampling

Numerical integration introduces tolerance, aliasing, and sample-count choices
for shapes with simple exact formulas. Analytical dispatch is both faster and
deterministic.

## Future Work

- Add arbitrary-window integration when a concrete consumer requires it.
- Add sampled/noisy and composite signals under explicit storage/cost limits.
- Add nanosecond sampling for tests that must resolve MHz-scale edges.
- Extend `VoltageSource` with an explicit-time signal contract in a separate
  design.
