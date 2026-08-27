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

The vendored [ESP-IDF 6.0.2 LEDC
driver](../roo_testing/frameworks/esp-idf/components/esp_driver_ledc/src/ledc.c)
serializes supported fade and duty changes per channel on classic ESP32.
`LEDC_FADE_NO_WAIT` returns from the fade-start call, but the fade keeps the
channel's binary semaphore until its completion ISR gives it. A subsequent
same-channel duty setter waits forever on that semaphore. FreeRTOS therefore
blocks only the calling task; hardware time and other runnable tasks continue.

`ledc_update_duty()` is different. It does not acquire the fade semaphore. On
classic ESP32 its [low-level
`ledc_ll_set_duty_start()`](../roo_testing/frameworks/esp-idf/components/esp_hal_ledc/esp32/include/hal/ledc_ll.h)
spins on the hardware `duty_start` bit while the caller is inside an LEDC
critical section. The [public LEDC
header](../roo_testing/frameworks/esp-idf/components/esp_driver_ledc/include/driver/ledc.h)
also documents the split set/update APIs as non-thread-safe. A fade may
comprise multiple hardware segments restarted by its ISR, so this loop is not
a specified task-blocking wait for the complete logical fade. This design does
not treat that unsafe implementation detail as a supported blocking contract.

| API path | ESP-IDF behavior | Emulator behavior |
| --- | --- | --- |
| Duty/fade setters and thread-safe combined helpers | Take the channel fade semaphore | Suspend only the calling FreeRTOS task |
| `LEDC_FADE_NO_WAIT` / `LEDC_FADE_WAIT_DONE` | Fade retains the semaphore; `WAIT_DONE` takes it again | Return immediately / wait on the channel gate |
| Bare update during an active fade | Undocumented critical-section spin | Return `ESP_ERR_INVALID_STATE` |

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
7. Same-channel operations block or reject as the supported classic
   ESP32/Arduino facade does; other channels remain independent.
8. A supported blocking call suspends only its calling FreeRTOS task. It never
   advances fake uptime; other runnable tasks and explicit time drivers remain
   able to run.
9. Fade completion commits exactly once, publishes steady target output before
   a still-valid callback, and suppresses stale callbacks after cancellation or
   re-registration.
10. Time and driver callbacks run outside timer, driver, GPIO, and sink locks;
    callback re-entry cannot deadlock on those locks or create nested delivery
    stacks.
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
- Treating a `roo_scheduler` callback as an independently suspendable task. A
  blocked callback blocks its FreeRTOS runner, not other FreeRTOS tasks.
- Reproducing the undocumented CPU/core starvation or interrupt-watchdog
  effects of classic ESP32's low-level `duty_start` spin.

## Design Overview

The driver keeps explicit committed output, pending output, timer carrier
origin, and active-fade state. Each publication is an immutable square
`VoltageSignal`; later state changes create a new value rather than mutating a
signal already retained by a sink.

A generic one-shot alarm queue on the existing system timer marks fade
deadlines. Alarms are dispatched only at explicit event-pump points. A
per-channel binary semaphore mirrors ESP-IDF's fade semaphore. Supported
blocking LEDC calls wait on that gate in bounded FreeRTOS waits and retry after
waking; they do not write or advance the system clock.

Two generations protect asynchronous work: `fade_generation` invalidates stale
completion alarms, while `callback_generation` invalidates queued callbacks
when registration or ownership changes. A FIFO delivery queue serializes GPIO
publication and user callbacks outside driver locks and prevents callback
re-entry from recursively delivering another batch.

| Requirements | Design element |
| --- | --- |
| 1-3 | One state-to-signal publication path for both frontends |
| 4-5 | `LinearDutyFade` publication plus integer duty materialization |
| 6-8, 11 | Task-blocking channel gate and validation retries |
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
both pending fields, and neither publishes. With no fade active,
`ledc_update_duty()` commits and publishes immediately; the active-fade case is
rejected as described under [Channel blocking](#channel-blocking).

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

As a deterministic host lifecycle policy, IDF timer deconfiguration
materializes each active channel's current integer duty into pending and
committed state, cancels fades without completion callbacks, clears configured
fades, and publishes low. It retains channel binding and persistent callback
registration. Reconfiguring the timer later publishes steady PWM. This is not
presented as a supported way to cancel a fade on hardware.

Detach/channel deconfiguration cancels asynchronous work and publishes constant
0 V. This is a deterministic host policy because high impedance is outside the
signal model. For safe host teardown, `ledc_fade_func_uninstall()` materializes
active duties, cancels fades without completion callbacks, publishes steady
PWM, invalidates queued callbacks, clears IDF callback registrations, and marks
the service uninstalled. ESP-IDF's uninstall routine simply frees its fade
records and is not a supported active-fade cancellation API.

Every failed call preserves prior driver/GPIO state. Validation independent of
channel ownership is performed before a wait, so an already-invalid call does
not block. No failure path advances uptime.

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
is a no-op. A callback can cancel later work at the same deadline. Recursive
pumping never dispatches nested callbacks; the outer drain processes newly due
work after the current callback returns. LEDC waits do not depend on recursive
alarm dispatch: every retry can lazily complete its own due fade under the LEDC
lock and invalidate the later alarm by generation.

### Channel blocking

The emulator gives every channel a binary FreeRTOS semaphore with the same
ownership rule as ESP-IDF's `ledc_fade_sem`: it is available while the channel
is idle, and a started fade retains it until completion. Calls corresponding
to ESP-IDF paths that acquire that semaphore use this retry algorithm:

1. Process due alarms, capture uptime, and validate arguments and immutable
   bounds before attempting the gate.
2. Try to take the channel semaphore without waiting. On success, lock LEDC
   state, revalidate mutable configuration, and apply the operation. Ordinary
   operations give the gate before returning; a started fade retains it.
3. If the gate is unavailable, lock LEDC state and lazily complete any due
   fade. Otherwise capture the owning fade generation/deadline, if any, then
   release all locks. An unavailable gate without an active fade is transient
   contention with another ordinary channel operation.
4. Wait for at most one FreeRTOS tick on the channel semaphore. This places the
   calling task in the Blocked state, so other FreeRTOS tasks can run. On
   success, retain the gate and continue through step 2's locked revalidation
   path; on timeout, process due alarms and retry from current state. No
   pre-wait snapshot is reused; another task may have completed, cancelled, or
   reconfigured the channel.
5. Retain a generation-keyed terminal result while waiters reference it. On
   completion retry acquisition; on cancellation revalidate and return the
   API's applicable invalid-state result.
6. Drain completion publication/callback before applying the newly unblocked
   mutation in ordinary single-task execution.

The bounded wait is also the fallback that lets a re-entrant alarm callback
notice a due fade without nested alarm delivery. It never calls
`system_time_delay_micros()` and never writes uptime. With auto-sync enabled,
ordinary time reads bring uptime forward from wall time and a retry completes
the fade at or after its deadline. With auto-sync disabled, the wait remains
blocked in fake-time terms until another FreeRTOS task explicitly advances or
pumps time. Deterministic blocking tests therefore use separate worker and
time-driver tasks.

A blocking path requires the FreeRTOS scheduler to be running, as it is for
ESP-IDF `app_main`, Arduino `setup`/`loop`, and the framework's FreeRTOS test
main. If such a path is reached before scheduler startup, the shim returns
`ESP_ERR_INVALID_STATE` or the corresponding Arduino failure instead of
hanging the host process.

`LEDC_FADE_WAIT_DONE` starts the same fade as `NO_WAIT`, publishes it, then
waits on that generation's channel gate until it completes or is cancelled.
`NO_WAIT` returns after the initial envelope publication.

On classic ESP32, a setter during a fade waits, then stages its new duty; the
following update commits it at the time the calling task resumes.
`ledc_set_duty_and_update` waits and commits atomically. `ledc_get_duty()`
remains non-blocking.

A bare `ledc_update_duty()` encountered during an active fade returns
`ESP_ERR_INVALID_STATE` without changing state or time. ESP-IDF does not route
that call through the fade semaphore, documents the split set/update sequence
as non-thread-safe, and offers no safe whole-fade behavior for the shim to
reproduce. Normal `ledc_set_duty(); ledc_update_duty();` use is unaffected:
the setter has already waited for an earlier fade before the update runs.
The gate does not make concurrent use of the split APIs thread-safe.

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

Add injected-scheduler host tests using the framework's FreeRTOS test main.
Disable timer auto-sync and capture a starting uptime instead of assuming zero.
A worker FreeRTOS task executes eligible scheduler callbacks; the test task is
the explicit time driver, walking to the earlier of each requested sample or
scheduler deadline and pumping system-time alarms. Test semaphores establish
when the worker has entered or left a blocking LEDC call before state is
sampled. Tests never call `Scheduler::run()` and never execute scheduler
callbacks on the time-driver task.

Coverage includes direct `GpioLed` levels/polarities/fades and
`Blink(Millis(1000), 30, 30, 90)`: fade on from 0-90 ms, steady on to 300 ms,
fade off to 930 ms, then steady off. Replacing a pattern mid-fade verifies the
task-local blocking rule: its immediate terminal `setLevel()` suspends the
scheduler worker until the test task advances fake time to old fade completion.
An independent probe task remains runnable, and overdue scheduler work begins
only after the runner resumes.

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
enabled cases. Verify that alarm scheduling and pumping never explicitly
increment uptime, with global state restored at teardown.

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
generation-checked alarms, FreeRTOS channel gates, callback
registration/claiming, the ordered delivery queue, cancellation/uninstall, and
all supported combined helpers to both shims. Move blocking IDF tests to the
FreeRTOS test main and document the classic target behavior.

Proposed commit: `Emulate LEDC fades and channel blocking`

Validation: run IDF and Arduino fade suites covering start/midpoint/deadline,
wait/no-wait, zero duration, task-local setter waits, active-fade bare-update
rejection, cancellation, reconfiguration, callback invalidation/re-entry,
cross-channel task progress, and concurrent delivery. Run blocking cases with
auto-sync both enabled and disabled; disabled cases use a separate time-driver
task.

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
readback, task-local blocking setters, active-fade bare-update rejection,
persistent callbacks, cancellation, re-entry, and cross-channel/task progress.
Arduino tests cover its distinct full-on, per-channel origin, tone,
reconfiguration, concurrent-fade rejection, callback, and detach behavior.

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
Time advances only when the application explicitly drives it or wall-clock
auto-sync observes progress.

#### Advance fake uptime from a blocked LEDC call

Moving the global clock makes a task-local wait affect every task and can force
the timer's auto-sync logic to sleep until wall time catches up. It also skips
the scheduling opportunity that a real FreeRTOS semaphore wait creates. The
channel gate therefore blocks only its caller; another task or wall-clock sync
is responsible for time progress.

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
channel gate reproduces the supported task-blocking behavior without changing
global time.

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
