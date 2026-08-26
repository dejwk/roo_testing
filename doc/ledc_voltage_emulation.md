# LEDC voltage emulation

Status: Proposed

Depends on: [Periodic voltage signals](periodic_voltage_signals.md)

## Objective

Make Arduino and ESP-IDF LEDC output, including fades and hardware-compatible
blocking behavior, observable through fake GPIO and usable by deterministic
roo_blink examples and tests.

## Motivation

The host LEDC shims retain configuration and duty but publish no electrical
output. They also complete fades immediately or omit parts of their lifecycle,
so tests cannot observe PWM brightness, intermediate fade levels, completion
callbacks, or the blocking that real ESP32 channel updates exhibit.

## Background

[`idf_ledc.cpp`](../roo_testing/frameworks/esp32_shims/idf_ledc.cpp) emulates
the ESP-IDF API used by roo libraries. A configured timer supplies frequency
and resolution; a channel supplies GPIO, duty, `hpoint`, and inversion. Duty
setters stage state and `ledc_update_duty()` commits it.

[`arduino_ledc.cpp`](../roo_testing/frameworks/esp32_shims/arduino_ledc.cpp)
implements the Arduino-ESP32 LEDC facade with per-channel frequency/resolution
state. The host facade does not emulate the hardware timer allocator.

The vendored classic-ESP32 ESP-IDF implementation serializes fade and duty
changes per channel. `LEDC_FADE_NO_WAIT` returns from the fade-start call, but
the channel remains busy until fade completion. A subsequent same-channel duty
setter waits on the fade semaphore. A bare `ledc_update_duty()` does not take
that semaphore, but its low-level `duty_start` guard still busy-waits until the
active hardware operation completes. Thus a roo_blink `GpioLed::setLevel()`
during a one-second fade can advance fake uptime by one second: the setter is
emulating a task blocked while hardware and the global clock continue.

The dependent [periodic-signal design](periodic_voltage_signals.md) supplies
value-owned square signals, linear duty envelopes, fake-GPIO propagation, and
local DC/RMS analysis. This document does not redefine those facilities.

## Requirements

1. Every configured LEDC output publishes a square voltage signal with the
   correct pin, rails, frequency, duty, phase placement, and inversion.
2. ESP-IDF pending and committed duty/`hpoint` behavior matches its public API;
   a setter alone does not change observable output.
3. Timer/channel reconfiguration, detach, deconfiguration, tone, and Arduino
   full-on mapping have deterministic observable behavior.
4. A fade changes PWM duty over fake uptime without scheduling carrier edges or
   jumping immediately to the target.
5. Duty readback during a fade is monotonic integer state, while the published
   signal exposes a continuous analytical envelope.
6. Non-blocking fade start returns after publishing the envelope. Blocking fade
   start returns only after its generation completes or is cancelled.
7. Same-channel operations block or reject exactly as the supported classic
   ESP32/Arduino facade does; other channels remain independent.
8. Blocking advances the one global fake uptime through peripheral deadlines.
   It does not run cooperative application scheduler work automatically.
9. Fade completion commits exactly once, publishes steady target output before
   a still-valid callback, and suppresses stale callbacks after cancellation or
   re-registration.
10. Time and driver callbacks run outside timer, driver, GPIO, and sink locks;
    callback re-entry cannot deadlock or create nested delivery stacks.
11. Invalid API calls are atomic and do not advance fake time.
12. No retained signal or scheduled closure refers into mutable channel/timer
    storage.
13. The first downstream milestone provides deterministic emulation tests and
    a finite runnable trace for roo_blink's monochrome `GpioLed`/`Blinker` path.

### Out of scope

- Bit-accurate LEDC clock-divider, fade-step, and interrupt-controller timing.
- ESP-IDF step, gamma, multi-range, and fade-stop support beyond APIs already
  required by roo libraries.
- Arduino hardware-timer allocation.
- roo_blink RGB/NeoPixel output, which uses a serialized pixel protocol.
- Running `roo_scheduler` tasks implicitly while an LEDC API is blocked.

## Design Overview

The driver keeps explicit committed output, pending output, timer carrier
origin, and active-fade state. Each publication is an immutable square
`VoltageSignal`; later state changes create a new value rather than mutating a
signal already retained by a sink.

A generic one-shot alarm queue on the existing system timer marks fade
deadlines. Alarms are dispatched only at explicit event-pump points. Blocking
LEDC calls advance the shared fake clock one alarm boundary at a time and retry
the channel operation after callbacks may have changed driver state.

Two generations protect asynchronous work: `fade_generation` invalidates stale
completion alarms, while `callback_generation` invalidates queued callbacks
when registration or ownership changes. A FIFO delivery queue serializes GPIO
publication and user callbacks outside driver locks and prevents callback
re-entry from recursively delivering another batch.

| Requirements | Design element |
| --- | --- |
| 1-3 | One state-to-signal publication path for both frontends |
| 4-5 | `LinearDutyFade` publication plus integer duty materialization |
| 6-8, 11 | Alarm-driven channel gate with validate-before-wait retries |
| 9-10 | Generation tokens and ordered non-nesting delivery queue |
| 12 | Value snapshots and stable channel identifiers in closures |
| 13 | roo_blink level fixes, scheduler-driven tests, and VoltageTrace example |

## Design Details

### Steady-state signal mapping

Both shims snapshot committed state, construct a signal while holding their
state lock, release the lock, and publish through:

```cpp
FakeEsp32().gpio.get(pin).write(signal);
```

Active channels use 0 V and `VoltageDigitalHigh()` as rails. Let
`period_counts = 1 << resolution`.

- Native ESP-IDF duty is `duty_count / period_counts`; full on is the valid raw
  count `period_counts`.
- For Arduino resolutions above one bit, a public duty at or above
  `period_counts - 1` maps to `period_counts`. Therefore 8-bit duty 255 is full
  on and `ledcRead()` returns mapped count 256.
- Timer frequency becomes carrier frequency.
- ESP-IDF `hpoint / period_counts` becomes pulse-start phase.
- Output inversion changes square polarity but not duty readback.
- All ESP-IDF channels bound to a timer share its carrier origin.

The Arduino shim keeps its existing per-channel timer model. Each channel gets
an origin on attach and resets it on `ledcChangeFrequency`. ESP-IDF timer
configuration/restart captures one shared origin; frequency/resolution
reconfiguration resets it and republishes every bound channel. Duty writes and
fade starts preserve carrier origin.

Tone is a 50% square. Zero-frequency tone publishes the idle rail: low when
uninverted and high when inverted. `analogWrite*` continues to delegate to LEDC
and gains waveform publication transitively.

### Configuration and lifecycle

IDF keeps pending and committed duty/`hpoint` separately.
`ledc_set_duty()` changes pending duty, `ledc_set_duty_with_hpoint()` changes
both pending fields, and neither publishes. `ledc_update_duty()` commits and
publishes after acquiring the applicable channel gate.

An IDF channel configured before its timer is retained but publishes low.
After timer configuration, duty must be in `[0, period_counts]` and `hpoint` in
`[0, period_counts)`; deferred invalid state remains low until corrected. Fade
start requires a valid channel and timer.

Idle resolution changes preserve raw committed, pending, and configured-fade
counts, saturating them to the new period; `hpoint` saturates to
`period_counts - 1`. Active-fade resolution changes fail atomically with
`ESP_ERR_INVALID_STATE` or Arduino frequency-change result zero. A
frequency-only change preserves the fade's absolute start/deadline, resets
carrier origin, and republishes the envelope at the new frequency.

Moving a channel first validates and reserves its new binding, then drives its
old pin low and publishes the new output. An actively fading IDF channel waits
for completion before reconfiguration. Arduino reassignment behaves as detach
plus attach: cancel the fade/callback, drive the old pin low, and establish a
fresh zero-duty channel and origin.

IDF timer deconfiguration materializes each active channel's current integer
duty into pending and committed state, cancels fades without completion
callbacks, clears configured fades, and publishes low. It retains channel
binding and persistent callback registration. Reconfiguring the timer later
publishes steady PWM.

Detach/channel deconfiguration cancels asynchronous work and publishes constant
0 V. This is a deterministic host policy because high impedance is outside the
signal model. `ledc_fade_func_uninstall()` materializes active duties, cancels
fades without completion callbacks, publishes steady PWM, invalidates queued
callbacks, clears IDF callback registrations, and marks the service uninstalled.

Every failed call preserves prior driver/GPIO state. All validation that can
fail is performed before a channel wait, so invalid calls never move uptime.

### Fade state and integer readback

`ledc_set_fade_with_time()` validates and stores target/duration without
changing output. An IDF positive-duration start captures current integer duty
and uptime. An Arduino `ledcFade*` call first commits its explicit start duty,
then starts the same transition to its target. Both assign a new fade
generation, register its non-dispatching alarm, and publish a square signal
with `LinearDutyFade`. Alarm registration precedes GPIO publication because a
re-entrant sink can advance time across the deadline.

Zero-duration and equal-endpoint fades commit synchronously, enqueue steady
publication followed by a completion callback, and create no alarm. Positive
durations use checked microsecond conversion and checked signed deadline
addition. An unrepresentable deadline returns `ESP_ERR_INVALID_ARG` or Arduino
`false` without changing active output.

Driver readback uses integer interpolation for positive durations:

```text
elapsed = clamp(now - fade_start, 0, duration)
steps = floor(abs(target-start) * elapsed / duration)
current = start + direction * steps
```

The product uses a sufficiently wide intermediate. At the deadline, commit the
exact target to both pending and committed duty. The published signal remains
the ideal continuous envelope described by the periodic-signal design.

### System-time alarms

Add a generic C++ one-shot alarm queue to
[`timer.h`](../roo_testing/system/timer.h) and
[`timer.cpp`](../roo_testing/system/timer.cpp). `timer.h` remains usable from C:
the new `<functional>` include and declarations are guarded by `__cplusplus`
and placed outside `extern "C"`.

The complete `EmulatedTime` state—uptime, real-time origin, auto-sync flag,
alarms, IDs, and drain state—is protected by one mutex. Wall-clock sleeping
never holds it; after sleeping, auto-sync reacquires the lock and re-evaluates
state.

Plain get-time, sync, and lag functions never dispatch callbacks.
`system_time_delay_micros()`, explicit `ProcessSystemTimeAlarms()`, and LEDC API
entry are event-pump points. There is no background alarm thread. A long delay
walks deadlines chronologically; same-deadline alarms run in registration
order. Scheduling never invokes inline, including an already-due alarm. The
outer active drain also processes due work scheduled by one of its callbacks.

The pump removes an alarm and marks it executing before releasing the timer
mutex and invoking it. Cancellation of an executing, completed, or unknown ID
is a no-op. A callback can cancel later work at the same deadline.

Recursive pumping never dispatches nested callbacks. When called from the
active drainer, `PumpSystemTimeToNextAlarmOr(limit)` advances uptime directly to
`max(now, limit)` and returns; the outer drain later processes skipped alarms.
This exception can make a skipped callback observe a later uptime, but it
prevents recursion and allows a blocking LEDC call made from an alarm callback
to reach its fade deadline. LEDC lazily materializes any due fade under its own
lock, so the channel gate still releases exactly once.

### Channel blocking

Every operation guarded by the real fade semaphore or classic-ESP32
`duty_start` follows this retry algorithm:

1. Process due alarms, capture uptime, and validate arguments/configuration.
2. Under the LEDC lock, lazily complete any due fade. If a generation still
   owns the channel, capture its generation/deadline and release all locks.
3. Pump to the next alarm or that deadline, then retry. Earlier callbacks can
   cancel or reconfigure the channel, so no pre-wait snapshot is reused.
4. Retain a generation-keyed terminal result while waiters reference it. On
   completion retry acquisition; on cancellation revalidate and return the
   API's applicable invalid-state result.
5. Drain completion publication/callback before applying the newly unblocked
   mutation in ordinary single-threaded execution.

This advances the global fake clock exactly as hardware time advances while a
CPU task waits. With auto-sync disabled it incurs no wall delay. With auto-sync
enabled it uses the timer's existing pacing and adds no LEDC-specific sleep.
The loop processes peripheral alarms but never pumps `roo_scheduler`.

`LEDC_FADE_WAIT_DONE` starts the same fade as `NO_WAIT`, publishes it, then uses
the boundary loop until that generation completes or is cancelled. It must not
delay once for the original full remainder because an intervening alarm can
cancel the fade. `NO_WAIT` returns after the initial envelope publication.

On classic ESP32, a setter during a fade waits, then stages its new duty; the
following update commits it at the resulting uptime. `ledc_set_duty_and_update`
waits and commits atomically. `ledc_get_duty()` remains non-blocking.

A bare `ledc_update_duty()` during a fade is a narrow hardware-specific case.
It waits for the encountered `duty_start` generation, but completion itself
already committed pending and active duty. The invocation then returns
`ESP_OK` without another publication and without consuming a different pending
value staged by a completion callback or concurrent caller. That value remains
pending for a later update. A cancelled generation revalidates and returns the
appropriate error.

IDF fade configuration/start calls use the same channel gate. A second Arduino
fade on the supported classic target returns `false` immediately because
fade-stop is unavailable; plain `ledcWrite()` still follows the blocking IDF
duty path. A future target profile with `SOC_LEDC_SUPPORT_FADE_STOP` implements
that target's compiled behavior rather than inheriting the classic rule.

### Completion and callback delivery

Completion commits target duty, clears the active fade, and enqueues steady
publication immediately followed by the user callback. The callback event
contains the completed target, but re-entry or another thread can change
current driver state before the callback reads it.

IDF callback registration persists until replaced, uninstalled, or channel
deconfiguration. Arduino callbacks belong to one fade. A callback queue item
stores stable channel identity, callback generation, and event values—not a raw
callback/user-argument snapshot. At claim time it briefly locks driver state,
verifies generation and registration, copies the callable and argument, marks
the item executing, and unlocks before invocation. Replacing/clearing a
registration, uninstalling, deconfiguring, detaching, or superseding an Arduino
fade increments `callback_generation` and invalidates unclaimed items.

An IDF callback receives `LEDC_FADE_END_EVT`, speed mode, channel, and completed
target duty. Its task-wakeup return value is ignored by the host. Arduino
argument and no-argument callback variants are claimed by the same mechanism.

Callback pointers and user arguments remain borrowed as in the public driver
API. Once claimed, concurrent unregister does not cancel or wait for execution;
the owner must externally synchronize argument lifetime.

### Concurrency and delivery ordering

Signals and alarm closures contain values, stable mode/channel identifiers,
and generations only. They never retain `ChannelState*` or timer references.
LEDC entry processes alarms before taking its state mutex, and no event pump or
delay runs while that mutex is held.

State transitions enqueue value-owned work with monotonically increasing
per-pin revisions. One FIFO can hold publication items and callback items. A
single active drainer pops work under the queue mutex, releases it, then calls
GPIO or user code. A publication is discarded only when that same pin already
delivered a newer revision. Relinquishing drainer ownership and observing an
empty queue are one locked operation, so concurrent work is not stranded.

Delivery never nests and never waits for another drainer. Re-entrant or
concurrent LEDC calls commit and enqueue, then return when a drainer is already
active; the active drainer delivers their work afterward. In ordinary
single-threaded use, a mutating API drains its entire batch before returning.
No timer, LEDC, delivery, pin, or sink mutex is held across external code.

### roo_blink milestone

The first downstream consumer is roo_blink's monochrome ESP32 `GpioLed`, which
uses a 40 kHz, 10-bit IDF LEDC channel and non-blocking time fades. roo_testing
keeps only generic regression coverage; roo_blink depends on roo_testing, never
the reverse.

Update `GpioLed::dutyForLevel` to use the full native range (`P = 1024`) with
64-bit intermediate arithmetic:

```text
brightness_count = (level * P + 32767) / 65535
raw_duty = ON_HIGH ? brightness_count : P - brightness_count
```

This maps both logical endpoints exactly. Also make the signature default
`ON_HIGH`, matching its documentation, and initialize channel duty with
`dutyForLevel(0)` so either polarity starts logically off. `ON_LOW` continues
to reverse duty rather than use LEDC output inversion; the published signal
always describes physical pin voltage.

Add injected-scheduler host tests. Disable timer auto-sync, capture a starting
uptime instead of assuming zero, and walk to the earlier of each requested
sample or scheduler deadline. At each boundary advance uptime, drain eligible
scheduler work, then sample. Re-read uptime after every task because a blocking
LEDC mutation can advance it. Tests never call `Scheduler::run()`.

Coverage includes direct `GpioLed` levels/polarities/fades and
`Blink(Millis(1000), 30, 30, 90)`: fade on from 0-90 ms, steady on to 300 ms,
fade off to 930 ms, then steady off. Replacing a pattern mid-fade verifies the
real blocking rule: its immediate terminal `setLevel()` advances fake uptime to
old fade completion before the already-due scheduler task begins the new
sequence.

Add a finite host-only `examples/monochrome/VoltageTrace` target. It attaches a
`SimpleVoltageSink` before constructing `GpioLed`, runs the Smooth sequence
with an injected scheduler and disabled auto-sync, and prints relative uptime,
physical DC, total RMS, AC RMS, and logical brightness as CSV for slightly more
than one period. It cleans up and exits instead of entering Arduino's infinite
`loop()` runner.

## Proposed API

The generic timer extension is:

```cpp
using SystemTimeAlarmId = uint64_t;

SystemTimeAlarmId ScheduleSystemTimeAlarm(
    int64_t deadline_uptime_us, std::function<void()> callback);
void CancelSystemTimeAlarm(SystemTimeAlarmId id);
void ProcessSystemTimeAlarms();
int64_t PumpSystemTimeToNextAlarmOr(int64_t limit_uptime_us);
```

No new public Arduino LEDC API is introduced. The implementation updates the
existing attach/write/channel-write, tone/note, inversion,
frequency/resolution, detach, read, and fade/callback variants.

For ESP-IDF, update existing timer/channel configuration, set/update/get duty,
fade installation/configuration/start, and implement these already-declared
entry points through the same state machine:

- `ledc_set_duty_with_hpoint`
- `ledc_set_duty_and_update`
- `ledc_fade_func_uninstall`
- `ledc_cb_register`
- `ledc_set_fade_time_and_start`

New callable stubs for other `esp_err_t` LEDC APIs return
`ESP_ERR_NOT_SUPPORTED` without changing state. Pause/resume/reset/bind,
`ledc_stop`, frequency getters/setters, fade stop, step/gamma, and multi-range
fade remain outside this proposal.

## Implementation Plan

Authoring reference: follow this repository's
[design-authoring guidance](../.github/instructions/embedded-design-doc-authoring.instructions.md)
and adjacent ESP32-shim/test conventions; roo_testing has no separate
code-authoring guide at the time of this proposal. Complete the
[periodic-signal implementation](periodic_voltage_signals.md#implementation-plan)
first.

### Phase 1: One-shot uptime alarms

Add the C++ alarm API and synchronized timer state in `system/timer.h/.cpp`,
update BUILD dependencies, and add isolated ordering, cancellation, large-delay,
auto-sync, and re-entry tests.

Proposed commit: `Add deterministic system-time alarms`

Validation: run the system timer/alarm tests under auto-sync disabled and
enabled cases, with global state restored at teardown.

### Phase 2: Steady ESP-IDF LEDC publication

Add timer origins, pending/committed state, validation, lifecycle policies,
one state-to-signal builder, and GPIO publication to `idf_ledc.cpp`. Implement
the non-fade duty helpers in scope and update focused tests and shim docs.

Proposed commit: `Publish ESP-IDF LEDC PWM through fake GPIO`

Validation: run IDF LEDC tests for configuration, commit, reconfiguration,
phase, inversion, invalid calls, and old-pin-low behavior.

### Phase 3: Steady Arduino LEDC publication

Add per-channel origins, Arduino full-on conversion, output lifecycle, tone,
analog-write integration, and publication to `arduino_ledc.cpp`; update tests
and shim docs.

Proposed commit: `Publish Arduino LEDC PWM through fake GPIO`

Validation: run Arduino LEDC tests for attach/write/read, 255-to-256 full-on,
tone/note, frequency/resolution, inversion, channel reuse, and detach.

### Phase 4: LEDC fade state machine

Add fade configuration, continuous signal publication, integer materialization,
generation-checked alarms, channel waits, callback registration/claiming, the
ordered delivery queue, cancellation/uninstall, and all supported combined
helpers to both shims. Document the classic target behavior.

Proposed commit: `Emulate LEDC fades and channel blocking`

Validation: run IDF and Arduino fade suites covering start/midpoint/deadline,
wait/no-wait, zero duration, setters and bare updates during a fade,
cancellation, reconfiguration, callback invalidation/re-entry, cross-channel
independence, and concurrent delivery.

### Phase 5: roo_blink emulation milestone

In roo_blink, correct endpoint/polarity initialization, add deterministic
`GpioLed` and `Blinker` tests, add the finite VoltageTrace example, document its
command, and update Bazel/module wiring in the same downstream commit.

Proposed commit: `Test roo_blink through emulated LEDC voltage`

Validation: run the two new tests, run
`bazel run //examples/monochrome/VoltageTrace:VoltageTrace`, then run the
roo_blink aggregate tests and existing example builds.

## Testing Plan

Timer tests validate chronological and same-deadline ordering, cancellation,
callback-created alarms, non-dispatching clock reads, large delays, recursive
pumps, and lock-free callback execution.

IDF tests cover pending/committed state, timer sharing, lifecycle and atomic
validation, exact duty/phase/inversion mapping, every fade transition, integer
readback, blocking setters, the classic bare-update exception, persistent
callbacks, cancellation, re-entry, and cross-channel progress. Arduino tests
cover its distinct full-on, per-channel origin, tone, reconfiguration,
concurrent-fade rejection, callback, and detach behavior.

Integration tests verify that GPIO sinks receive the correct immutable signal
and local analysis throughout a fade. The downstream roo_blink suite verifies
40 kHz metadata, exact logical endpoints for both polarities, the documented
one-second blink timeline, and mid-fade sequence replacement. The VoltageTrace
example must terminate and emit the expected one-period CSV without a wall-clock
dependency.

All tests restore timer auto-sync, LEDC service/timer/channel state, GPIO
attachments, callbacks, and alarms they create.

## Caveats

The fade envelope follows requested duration exactly and does not reproduce
hardware divider/step rounding. Integer readback preserves monotonic count
semantics, which is sufficient for current roo consumers.

Callbacks execute on whichever thread drains fake time/delivery, not genuine
ISR context. Code that depends on ISR scheduling or task wakeup must use a more
specialized emulator. IDF callback task-wakeup return values are ignored.

The active delivery-drainer exception means a re-entrant or concurrent caller
can return after committing state but before its external publication/callback
is delivered. Ordinary single-threaded calls drain their complete batch before
returning.

### Rejected Alternatives

#### Advance fake uptime when a fade starts

This would make `NO_WAIT` synchronous and skip observable intermediate duty.
Time advances only when the application/timer advances it or a later hardware-
compatible blocking operation waits for the channel.

#### Complete fades only when LEDC is queried

Pure lazy completion cannot deliver callbacks when a normal fake-time delay
crosses the deadline. One-shot alarms provide the required completion event;
lazy materialization remains a deadlock-safe companion for recursive pumps.

#### Run a background timer thread

A background worker makes deterministic fake-time tests dependent on host
scheduling and complicates global-time synchronization. Explicit pump points
preserve deterministic execution.

#### Cancel a fade on ordinary duty writes

Classic ESP32 serializes these operations instead. Waiting on the emulated
channel gate reproduces observable hardware behavior, including the fake-uptime
advance seen by `GpioLed::setLevel()`.

#### Invoke GPIO and callbacks while holding driver locks

This gives simple ordering but deadlocks re-entrant sinks and callbacks. The
revisioned FIFO preserves commit order without holding internal locks across
external code.

## Future Work

- Add target profiles for hardware with `SOC_LEDC_SUPPORT_FADE_STOP`.
- Emulate step, gamma, and multi-range fades when a roo consumer needs them.
- Add bit-accurate clock-divider and hardware fade-step timing.
- Design separate NeoPixel waveform/protocol observation for roo_blink RGB
  examples.
