# LEDC voltage emulation

Status: Proposed

Depends on: [Periodic voltage signals](periodic_voltage_signals.md),
[Emulated-time alarms](emulated_time_alarms.md),
[Emulated interrupts](emulated_interrupts.md)

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
local DC/RMS analysis. The [emulated-time alarm
design](emulated_time_alarms.md) supplies deterministic one-shot deadline
callbacks in manual time, autonomous best-effort wall-time delivery,
cancellation, ordering, and non-nesting drain semantics. The [emulated
interrupt design](emulated_interrupts.md) supplies signal-based ISR delivery,
ESP-IDF interrupt allocation, and ISR-exit yields. This document does not
redefine those facilities.

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
   explicitly writes or advances fake uptime; the configured auto-sync waiter
   or another task/test driver remains responsible for observed time progress,
   and other runnable tasks remain able to run.
9. Fade completion commits exactly once, publishes steady target output, then
   raises the LEDC source. A still-valid callback runs in emulated ISR context
   after the channel semaphore is given; its task-wakeup result participates in
   the ISR-exit yield.
10. LEDC never advances uptime. Nonblocking entry does not pump the global
    alarm queue; the documented channel-wait retry may dispatch already-due
    alarms only after releasing every LEDC lock and gate. GPIO and sink code
    never runs while a driver or delivery lock is held. ISR-side state,
    callback pointers, and completion mailboxes use signal-safe storage; the
    ISR never takes an ordinary host mutex. Task/native host-mutex ownership
    uses the alarm design's signal-mask discipline so a selected FreeRTOS task
    cannot be switched away while holding a lock needed by another task or the
    native waiter; this includes the retained-signal mutex inside
    `FakeGpioPin` and the built-in simple voltage/digital sinks.
11. Invalid API calls are atomic and do not explicitly write or delay fake
    time; ordinary clock observation may still apply configured auto-sync.
12. No retained signal or scheduled closure refers into mutable channel/timer
    storage.
13. The first downstream milestone provides deterministic emulation tests and
    a finite runnable trace for roo_blink's monochrome `GpioLed`/`Blinker` path.
14. Successful task/native-context detach, deconfiguration, reassignment,
    uninstall, and test reset establish quiescence for affected alarm handlers
    and LEDC delivery before topology or borrowed sink state may be destroyed.

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
- Generic system-time alarm storage, ordering, pumping, and concurrency, which
  belong to the dependent alarm design.

## Design Overview

The driver keeps explicit committed output, pending output, timer carrier
origin, and active-fade state. Each publication is an immutable square
`VoltageSignal`; later state changes create a new value rather than mutating a
signal already retained by a sink.

The dependent alarm queue marks fade deadlines and dispatches them from an
explicit manual-time drainer or its auto-sync native waiter. A valid deadline
handler commits target state and atomically appends an ordered delivery pair:
final GPIO publication, then an internal completion finalizer. After the GPIO
write and sink calls return, the finalizer publishes a generation-tagged
fade-end mailbox and raises `ETS_LEDC_INTR_SOURCE` with no alarm, driver,
delivery, GPIO, or sink lock held. The registered emulated LEDC ISR claims that
mailbox, gives the channel semaphore, invokes the framework callback, and
requests any required yield.

LEDC adds a per-channel binary semaphore mirroring ESP-IDF's fade semaphore.
Supported blocking calls wait on that gate in bounded FreeRTOS waits and retry
after waking; they neither write the system clock nor call an explicit
time-advance function.

Three tokens protect asynchronous work. `fade_generation` identifies the
active fade and lets exactly one alarm or lazy path claim its completion;
claiming does not increment it. A lock-free `completion_generation` remains
valid through the queued publication/finalizer pair and is invalidated only by
a cancellation path that also resolves the gate. `callback_generation`
suppresses only borrowed callbacks when registration or ownership changes. A
FIFO delivery queue serializes GPIO publication and internal completion
finalizers outside driver locks. User callbacks are not host-queue items; the
emulated ISR loads only lock-free callback state and invokes them with
ISR-facing semantics.

| Requirements | Design element |
| --- | --- |
| 1-3 | One state-to-signal publication path for both frontends |
| 4-5 | `LinearDutyFade` publication plus integer duty materialization |
| 6-8 | Emulated-time alarm plus task-blocking channel gate |
| 9-10 | Generation-tagged completion mailbox plus emulated LEDC ISR |
| 11 | Validation before waiting plus transactional mutation |
| 12 | Value snapshots and stable channel identifiers in closures |
| 13 | roo_blink level fixes, scheduler-driven tests, and VoltageTrace example |
| 14 | Generation invalidation plus context-aware delivery quiescence barrier |

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
fades, resolves each retained channel gate exactly once in task context, and
publishes low. It retains channel binding and persistent callback registration.
Reconfiguring the timer later publishes steady PWM. This is not presented as a
supported way to cancel a fade on hardware.

Detach/channel deconfiguration cancels asynchronous work and publishes constant
0 V. This is a deterministic host policy because high impedance is outside the
signal model. For safe host teardown, `ledc_fade_func_uninstall()` materializes
active duties, cancels fades without completion callbacks, publishes steady
PWM, resolves retained gates, invalidates callback generations, clears IDF
callback registrations, and marks the service uninstalled. ESP-IDF's uninstall
routine simply frees its fade records and is not a supported active-fade
cancellation API.

Cancellation and generation checks prevent a stale completion from committing,
but they do not by themselves protect `FakeGpioPin` or a borrowed sink while a
native waiter already owns delivery. Each fade generation therefore has a
task-side value-owned activity token captured by its alarm and completion pair.
Every lifecycle operation also uses a FIFO quiescence fence; fade tokens alone
would not cover ordinary steady, reconfiguration, terminal-low, or already
popped GPIO publications.

A task/native-context lifecycle operation first closes the affected channel or
pin to new work, wins the applicable packed completion cancellation transition,
advances future generations, and invalidates old queue revisions under LEDC
locks. It atomically appends any required final low/steady publication followed
by its fence, then releases the locks, cancels alarms, and attempts the
non-waiting delivery drain. Because there is one FIFO owner, observing that
fence proves that every item queued or popped before it--including the
lifecycle publication and any in-flight `FakeGpioPin::write()`/sink call--has
returned or been invalidated. The caller also waits for claimed fade-alarm
activity tokens and any whole-channel ISR completion already claimed. Only then
may detach,
reassignment, timer/channel deconfiguration, uninstall, or test reset return
success and permit topology or borrowed callback state to be destroyed.

The quiescence wait never holds a driver, FIFO, pin, or sink lock. A FreeRTOS
caller uses bounded task delays and rechecks so other task pthreads can run; an
ordinary native caller uses bounded monotonic sleeps and atomic rechecks. It
does not depend on condition notification from the POSIX-signal ISR. Calls from
the active GPIO/sink delivery context cannot wait for themselves. Lifecycle APIs
with a result fail before mutation in that context. The void
`ledc_fade_func_uninstall()` records a deferred uninstall and returns; a later
non-delivery-context call to the idempotent uninstall performs the barrier.
Until that call completes, the caller must not detach fake GPIO topology or
destroy a borrowed sink/callback argument.

`FakeGpioInterface` retains its existing external-synchronization contract.
Code that directly calls `FakeGpioInterface::detach()` or destroys a borrowed
sink first performs the corresponding successful LEDC detach/uninstall barrier;
the fake GPIO layer does not discover or wait for LEDC work on its own.

Every failed call preserves prior driver/GPIO state. Validation independent of
channel ownership is performed before a wait, so an already-invalid call does
not block. No failure path writes uptime or calls a delay function.

### Fade state and integer readback

`ledc_set_fade_with_time()` validates and stores target/duration without
changing output. An IDF positive-duration start captures current integer duty
and uptime. An Arduino `ledcFade*` call first commits its explicit start duty,
then starts the same transition to its target. Both assign a new fade
generation, register its non-dispatching alarm, and publish a square signal
with `LinearDutyFade`. Alarm registration precedes GPIO publication because a
re-entrant sink can advance time across the deadline.

Zero-duration and equal-endpoint fades commit synchronously, enqueue the same
steady-publication/completion-finalizer pair, and create no alarm. They do not
special-case callback or gate delivery. Positive durations use checked
microsecond conversion and checked signed deadline addition. An
unrepresentable deadline returns `ESP_ERR_INVALID_ARG` or Arduino `false`
without changing active output.

Driver readback uses integer interpolation for positive durations:

```text
elapsed = clamp(now - fade_start, 0, duration)
steps = floor(abs(target-start) * elapsed / duration)
current = start + direction * steps
```

The product uses a sufficiently wide intermediate. At the deadline, commit the
exact target to both pending and committed duty. The published signal remains
the ideal continuous envelope described by the periodic-signal design.

### Alarm-service integration

The [emulated-time alarm service](emulated_time_alarms.md) owns deadline
storage, cancellation, chronological dispatch, auto-sync interaction, and
non-nesting pump behavior. Nonblocking LEDC entry does not pump the global alarm
queue; it reads uptime and lazily materializes only its own due fade. Only the
documented channel-wait timeout may call the explicit due-alarm pump.

A fade alarm captures stable mode/channel identity, its `fade_generation`, and
the value-owned activity token. Under the LEDC lock, its handler verifies that
generation,
atomically transitions the fade from active to completion-queued without
changing the generation, commits the exact target, and appends a value-owned
pair consisting of steady GPIO publication and a completion finalizer. A raced
alarm or lazy path sees that the generation is no longer active and does
nothing. The alarm handler never gives the channel gate and never enqueues or
invokes the public callback.

After a successful handler has appended the pair, it releases the LEDC lock and
always attempts the non-waiting LEDC delivery drain. If it wins ownership, it
drains through that pair before returning. If another owner is active, the
handler leaves the pair in the FIFO and returns; the existing owner observes it
before atomically relinquishing ownership. This step is what makes autonomous
wall-time completion live when no later application call happens.

The LEDC delivery owner calls GPIO and sink code outside locks. Only after the
final GPIO call returns does the paired finalizer revalidate its completion
generation against the lock-free `completion_generation`, publish the ISR
mailbox/fade-end status with release semantics, and raise the LEDC source. The
ISR then gives the gate and conditionally invokes the callback. In ordinary
single-threaded use, the alarm handler's drain attempt completes the final GPIO
write and source request before it returns; POSIX-signal delivery and callback
completion remain asynchronous. If another LEDC drainer is active, the pair
stays FIFO-ordered for that owner.

LEDC does not rely solely on alarm-handler dispatch. A channel wait or other
LEDC entry lazily completes a due fade under the LEDC lock and invalidates its
later handler by claiming the active-to-completion-queued transition, not by
incrementing `fade_generation`. It retains the claimed generation in the same
publication/finalizer pair and cancels the alarm ID after releasing the LEDC
lock; cancellation of an already-claimed alarm is harmless. Before taking or
waiting on the still-held channel gate, the lazy-completion winner attempts to
drain LEDC delivery. If another owner exists, that owner will reach the
finalizer. This lets an LEDC operation called re-entrantly from an alarm
handler observe completion without requiring nested alarm delivery.

### Channel blocking

The emulator gives every channel a binary FreeRTOS semaphore with the same
ownership rule as ESP-IDF's `ledc_fade_sem`: it is available while the channel
is idle, and a started fade retains it until completion. Calls corresponding
to ESP-IDF paths that acquire that semaphore use this retry algorithm:

1. Capture uptime, validate arguments and immutable bounds, and lazily claim
   any due fade on the addressed channel. After releasing the LEDC lock, cancel
   that fade's alarm ID and attempt to drain the queued publication/finalizer
   pair before taking or waiting on the gate. If another drainer owns delivery,
   it is responsible for the pair.
2. Try to take the channel semaphore without waiting. On success, lock LEDC
   state, revalidate mutable configuration, and apply the operation. Ordinary
   operations give the gate before returning; a started fade retains it.
3. If the gate is unavailable, lock LEDC state and lazily claim any due fade.
   Otherwise capture the owning fade generation/deadline, if any, then release
   all locks. A lazy-completion winner again cancels its alarm and attempts
   LEDC delivery before waiting. An unavailable gate without an active fade or
   completion is transient contention with another ordinary channel operation.
4. Wait for at most one FreeRTOS tick on the channel semaphore. This places the
   calling task in the Blocked state, so other FreeRTOS tasks can run. On
   success, retain the already-acquired gate and enter step 2's locked
   revalidation/application path without taking it again. On timeout, call
   `ProcessSystemTimeAlarms()` as an explicit global pump point, then retry from
   current state. This may dispatch unrelated due alarm handlers on the waiting
   task. No pre-wait snapshot is reused; another task may have completed,
   cancelled, or reconfigured the channel.
5. Retain a generation-keyed terminal result while waiters reference it. After
   taking the gate, inspect that result before mutation. `completed` means the
   ordered finalizer ran: final GPIO publication returned, the ISR gave the
   gate, and any valid ISR callback finished before the resumed task can run.
   Revalidate state and apply the operation. `cancelled` returns the API's
   applicable invalid-state result without mutation and releases or retires the
   acquired gate according to the lifecycle transition.

The bounded wait is also the fallback that lets a re-entrant alarm handler
notice a due fade without nested alarm delivery. It never calls
`system_time_delay_micros()` and never writes uptime. With auto-sync enabled,
the native deadline waiter normally completes the fade; ordinary time reads and
the bounded retry remain a race-safe lazy fallback. With auto-sync disabled,
the wait remains blocked in fake-time terms until another FreeRTOS task
explicitly advances or pumps time. Deterministic blocking tests therefore use
separate worker and time-driver tasks.

LEDC tracks whether the current host call stack is inside its GPIO/sink
delivery. A call from that context must not wait for a channel gate: doing so
would block the only drainer before it can reach the gate-giving completion
finalizer. `LEDC_FADE_WAIT_DONE` therefore fails before starting a fade, and a
setter or other gate-taking API whose nonblocking take fails returns
`ESP_ERR_INVALID_STATE` or the Arduino failure value without mutation.
Gate-free reads and operations whose gate is immediately available may still
commit and append work behind the active owner.

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

The deadline or lazy-completion path first validates the active
`fade_generation`, claims the active-to-completion-queued transition without
incrementing it, commits target duty, and atomically appends two value-owned
items to the non-nesting LEDC FIFO. The first publishes the steady signal. The
second is an internal finalizer that runs only after `FakeGpioPin::write()` and
all sink calls for that publication have returned. It must win the packed
completion-state transition from `queued` to `pending`; only then does it
release-publish a fade-end mailbox and status and raise
`ETS_LEDC_INTR_SOURCE` through the ESP-IDF interrupt adapter with the
corresponding status-address/value snapshot.

The mailbox is deliberately invisible before the finalizer. All LEDC channels
share one source, so an unrelated channel's earlier assertion must not let the
ISR claim this completion before its final GPIO publication. The finalizer
runs with no alarm, LEDC, FIFO, GPIO, or sink mutex held.

The emulated LEDC ISR atomically claims every pending channel completion. For
each valid generation it gives the channel semaphore with
`xSemaphoreGiveFromISR()`, builds `LEDC_FADE_END_EVT`, and invokes the currently
valid callback. It ORs the callback's task-wakeup result with the semaphore's
higher-priority-task result and calls `portYIELD_FROM_ISR()` when either asks
for a switch. The port performs that switch only after the ISR returns.

That atomic claim covers the entire completion lifetime, not merely the
optional borrowed callback. A statically asserted lock-free word combines the
completion generation with `armed`, `queued`, `pending`, `isr_active`,
`cancelled`, and `completed` state. Fade start publishes `armed`; the deadline
or lazy winner CASes `armed -> queued` when it appends the pair. The finalizer
must CAS the same generation `queued -> pending`, while lifecycle cancellation
can win `armed`, `queued`, or `pending -> cancelled`. Only the winner proceeds:
a stale finalizer cannot republish pending after cancellation, although an
already-raised source may harmlessly find no pending mailbox.

The ISR must win `pending -> isr_active` before touching the gate and retains
`isr_active` through `xSemaphoreGiveFromISR()`, callback claim/invocation, and
wake-result publication before release-publishing `completed`. A lifecycle
path that wins `-> cancelled` resolves the gate in task context. If it instead
observes `isr_active`, it invalidates the optional callback and waits by atomic
recheck for `completed`; it never also gives or retires the gate. Thus teardown
cannot slip either between finalizer validation and pending publication or
between mailbox claim and semaphore give.

IDF callback registration persists until replaced, uninstalled, or channel
deconfiguration. Arduino callbacks belong to one fade. Lock-free ISR-facing
records publish callback pointer, argument, callback generation, stable channel
identity, completion/cancellation generation, and event values. Replacing or
clearing a registration increments `callback_generation` and suppresses only
the borrowed callback; a valid completion mailbox still reaches the ISR and
gives the channel gate.

Callback claim and invalidation use one statically asserted lock-free packed
atomic containing generation, an accepting bit, and an in-flight count. The ISR
may claim only by compare/exchange from the expected accepting generation to
that same generation with an incremented count; it acquires callback fields
published before the accepting state. Replacement/teardown first clears
accepting with compare/exchange while preserving the count. From that instant
no new ISR can claim the old argument; the writer waits by atomic recheck until
existing claims release-decrement to zero, then publishes replacement fields
and a new accepting generation. This closes the load-then-increment race in
which teardown could otherwise free an argument just before an ISR claimed it.

Both packed protocols reserve enough count/generation bits for their fixed
single-core maximum and fail a checked invariant before generation reuse or
count overflow. Later nested/SMP interrupt support must widen or redesign the
encoding rather than silently wrapping it.

Uninstall, deconfiguration, detach, or cancellation may invalidate an entire
completion pair only through this arbitration, ensuring that either the
lifecycle winner or the ISR winner resolves the retained gate exactly once
before callback/channel state is retired.

An IDF callback receives `LEDC_FADE_END_EVT`, speed mode, channel, and completed
target duty. Its task-wakeup return value contributes to the ISR-exit yield.
Arduino argument and no-argument callback variants use the same ISR mailbox.

Callback pointers and user arguments remain borrowed as in the public driver
API. Once claimed, callback execution has begun in ISR context. A task cannot
concurrently unregister it on the single-core backend; external native owners
use the callback-generation activity barrier before treating the replaced
argument as dead. Lifecycle teardown includes the same barrier.

### Concurrency and delivery ordering

Voltage signals and alarm closures contain values, stable mode/channel
identifiers, and generations only. They never retain `ChannelState*` or timer
references. The interrupt mailbox similarly contains only atomically published
values, callback pointers, and borrowed arguments with the documented lifetime.
Ordinary LEDC entry samples uptime and lazily completes only its addressed
channel before taking the channel gate. A channel-wait retry releases all LEDC
locks before explicitly pumping alarms. LEDC never invokes an event pump or
delay while an LEDC mutex or channel gate is held.

Every ordinary task/native acquisition of the LEDC state or FIFO host mutex
saves the POSIX signal mask, blocks all maskable signals, acquires and releases
the short lock, and then restores the mask. Nested state-then-FIFO acquisition
preserves the already-blocked outer
mask. This is the same port-safety rule as the alarm service: a higher-priority
FreeRTOS pthread cannot become selected and block on a host mutex whose owner
the port just suspended. Signal masks are restored before FreeRTOS semaphore
waits, alarm calls, delivery, allocation destruction, or external code.

The same reusable guard replaces direct `std::lock_guard` use for
`FakeGpioPin`'s retained-signal mutex. Otherwise a FreeRTOS task could be
preempted inside `write()`/`lastSignal()` and the native deadline waiter could
deadlock trying to publish LEDC output through that pin. `onWrite()` remains
outside the pin mutex and after signal-mask restoration.

It likewise guards the retained-signal mutexes in `SimpleVoltageSink` and
`SimpleDigitalSink`; their user callbacks run only after unlock and signal-mask
restoration. A custom sink attached to an LEDC pin must provide equivalent
native/task synchronization: any task-side host-mutex ownership masks scheduler
signals, or the sink uses lock-free/nonblocking state. Merely keeping its
callback short does not prevent owner-suspension deadlock.

State transitions enqueue value-owned publication work with monotonically
increasing per-pin revisions. Completion appends its final publication and
finalizer atomically so no other LEDC item can split the pair. A single active
drainer pops work under the queue mutex, releases it, then calls GPIO or sink
code or runs an internal finalizer. Ordinary superseded publications may be
discarded by revision, but a live completion publication is an ordering barrier
and is delivered before its finalizer; lifecycle cancellation invalidates the
whole pair and resolves its gate instead. Relinquishing drainer ownership and
observing an empty queue are one locked operation, so concurrent work is not
stranded. Framework callbacks bypass this host queue and run from the LEDC ISR
after their completion finalizer raises the source.

Because the native deadline waiter may win this drain, fake-GPIO sinks used by
LEDC are non-throwing, remain short, call no FreeRTOS API (including `FromISR`
forms), and do not wait for an ISR, another delivery item, or external
completion. A scope
guard always releases delivery ownership and activity accounting. If a sink
violates the non-throwing contract, the drainer restores that bookkeeping and
terminates with a diagnostic rather than letting an exception escape through
the alarm service with a channel gate stranded. A slow sink is likewise outside
the contract. It does not corrupt FIFO ordering, but it delays every later
autonomous system-time alarm.

To preserve commit order, a driver transition may briefly take the FIFO mutex
after the LEDC state lock solely to append value-owned items. The drainer never
holds the FIFO mutex while acquiring LEDC state or executing an item, and a
finalizer consults only lock-free completion/callback records, so that lock
order cannot cycle through external code.

GPIO delivery never nests and never waits for another drainer. Alarm-drain
ownership and LEDC-delivery ownership are independent. If a sink advances time
past a fade deadline, the alarm handler appends the completion pair and
returns; it neither recursively drains, raises inline, nor waits for the LEDC
owner. That owner finishes the current sink call, publishes the fade endpoint,
then runs the finalizer. Concurrent callers follow the same append-and-return
rule. In ordinary single-threaded use, a mutating API that owns delivery drains
its entire batch before returning. No timer, LEDC, delivery, pin, or sink mutex
is held across external code. ISR callbacks may call only ISR-safe APIs; they do
not enter this host delivery path.

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

LEDC uses the internal C++ API defined by the dependent
[emulated-time alarm design](emulated_time_alarms.md#proposed-api) and raises
its source through the dependent [interrupt
adapter](emulated_interrupts.md#proposed-api). It adds no generic timer or
interrupt API of its own.

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

Authoring references: follow this repository's
[design-authoring guidance](../.github/instructions/embedded-design-doc-authoring.instructions.md),
[C++ code-authoring guidance](../.github/instructions/embedded-cpp-code-authoring.instructions.md),
and adjacent ESP32-shim/test conventions. Complete the
[periodic-signal implementation](periodic_voltage_signals.md#implementation-plan)
and the alarm design's [ISR-safe clock and shared host-lock
phase](emulated_time_alarms.md#phase-1-isr-safe-monotonic-uptime-and-host-locking)
before Phase 1. Complete the remaining [emulated-time alarm
implementation](emulated_time_alarms.md#implementation-plan) plus [emulated
interrupt phases 1-3](emulated_interrupts.md#implementation-plan) before Phase
3.

### Phase 1: Steady ESP-IDF LEDC publication

Add timer origins, pending/committed state, validation, lifecycle policies,
one state-to-signal builder, and GPIO publication to `idf_ledc.cpp`, using the
already-hardened fake-GPIO and built-in sink locks. Implement the non-fade duty
helpers in scope and update focused tests and shim docs.

Proposed commit: `Publish ESP-IDF LEDC PWM through fake GPIO`

Validation: run IDF LEDC tests for configuration, commit, reconfiguration,
phase, inversion, invalid calls, and old-pin-low behavior.

### Phase 2: Steady Arduino LEDC publication

Add per-channel origins, Arduino full-on conversion, output lifecycle, tone,
analog-write integration, and publication to `arduino_ledc.cpp`; update tests
and shim docs.

Proposed commit: `Publish Arduino LEDC PWM through fake GPIO`

Validation: run Arduino LEDC tests for attach/write/read, 255-to-256 full-on,
tone/note, frequency/resolution, inversion, channel reuse, and detach.

### Phase 3: LEDC fade state machine

Add fade configuration, continuous signal publication, integer materialization,
generation-checked completion-alarm integration, FreeRTOS channel gates,
signal-safe fade-end mailboxes, LEDC interrupt registration/source assertion,
ISR callback delivery, the ordered GPIO delivery queue, signal-masked LEDC state
and FIFO host locking, cancellation/uninstall, and all supported combined
helpers to both shims. Move
blocking IDF tests to the FreeRTOS test main and document the classic target
behavior.

Proposed commit: `Emulate LEDC fades and channel blocking`

Validation: run IDF and Arduino fade suites covering start/midpoint/deadline,
wait/no-wait, zero duration, task-local setter waits, active-fade bare-update
rejection, cancellation, reconfiguration, callback invalidation/re-entry,
cross-channel task progress, strict final-GPIO-before-ISR ordering, unrelated
shared-source assertions, zero/equal-duration ordering, callback unregister
without lost gate release, lazy-completion drain-before-wait, cancellation-woken
waiter failure without mutation, delivery-context blocking rejection, callback
ISR context, callback-requested yields, autonomous alarm-handler drain liveness,
and concurrent/re-entrant delivery. Race detach/uninstall against an alarm
already claimed, a queued completion pair, and an in-flight sink; verify both
ordinary steady/reconfiguration publications, the lifecycle's final
publication/fence, an ISR after completion claim but before gate give, and an
ISR between callback claim and release. Verify both
owned and borrowed sink/argument lifetimes and the deferred void-uninstall rule.
A death test covers a throwing sink without stranded delivery ownership. A
low-/high-priority FreeRTOS plus native-waiter contention test verifies that a
task cannot be preempted while it owns an LEDC, `FakeGpioPin`,
`SimpleVoltageSink`, or `SimpleDigitalSink` host mutex; a custom guarded sink
gets the same coverage.
Run blocking cases with auto-sync both enabled and disabled; disabled cases use
a separate time-driver task.

### Phase 4: roo_blink emulation milestone

In roo_blink, correct endpoint/polarity initialization, add deterministic
`GpioLed` and `Blinker` tests, add the finite VoltageTrace example, document its
command, and update Bazel/module wiring in the same downstream commit.

Proposed commit: `Test roo_blink through emulated LEDC voltage`

Validation: run the two new tests, run
`bazel run //examples/monochrome/VoltageTrace:VoltageTrace`, then run the
roo_blink aggregate tests and existing example builds.

## Testing Plan

The alarm dependency owns isolated deadline ordering, cancellation, pumping,
and timer-lock tests. LEDC tests cover only its generation-checked use of that
service and its driver-visible effects.

IDF tests cover pending/committed state, timer sharing, lifecycle and atomic
validation, exact duty/phase/inversion mapping, every fade transition, integer
readback, task-local blocking setters, active-fade bare-update rejection,
persistent callbacks, callback replacement without lost gate release,
cancellation, re-entry, final-publication/ISR ordering, unrelated-channel
source assertions, active-drainer delay, lazy completion without a drainer,
cancellation terminal results, delivery-context wait rejection, and
cross-channel/task progress. Lifecycle tests cover claimed-alarm, queued-item,
ordinary-publication, FIFO-fence, ISR-callback, and in-flight-sink quiescence
plus whole-ISR-completion claim/cancel arbitration before fake-GPIO topology
destruction. Arduino
tests cover its distinct full-on,
per-channel origin, tone, reconfiguration, concurrent-fade rejection, callback,
and detach behavior.

Integration tests verify that GPIO sinks receive the correct immutable signal
and local analysis throughout a fade. The downstream roo_blink suite verifies
40 kHz metadata, exact logical endpoints for both polarities, the documented
one-second blink timeline, and mid-fade sequence replacement. The VoltageTrace
example must terminate and emit the expected one-period CSV without a wall-clock
dependency.

All tests first complete the LEDC detach/uninstall barrier, then remove GPIO
attachments or destroy borrowed sinks, and finally restore timer auto-sync and
other service state. They cancel any unrelated alarms they create.

## Caveats

The fade envelope follows requested duration exactly and does not reproduce
hardware divider/step rounding. Integer readback preserves monotonic count
semantics, which is sufficient for current roo consumers.

The fade-completion alarm handler runs on the auto-sync native waiter or the
thread/task explicitly pumping manual time. It commits state and enqueues the
ordered completion pair. The LEDC delivery owner publishes the hardware-visible
endpoint, and the paired finalizer raises the source afterward. The framework
callback then runs in emulated ISR context on whichever FreeRTOS task is
selected when the source is asserted, and its wakeup result can switch tasks at
ISR exit. The ISR is a POSIX signal handler, so callback and shim code must stay
within the supported ISR-safe and signal-safe subset. GPIO sinks reached from
the native waiter must also remain short and non-throwing, must not call
any FreeRTOS API, and must not wait for FreeRTOS, ISR, or external
completion. The one waiter serializes autonomous work, so a slow sink delays
unrelated system alarms as well as LEDC completion. Built-in sinks use the
signal-masked host-lock guard; a custom sink must use the same discipline or
lock-free state anywhere it can contend with a selected FreeRTOS task.

The active delivery-drainer exception means a re-entrant or concurrent caller
can return after committing state but before external publication or the
subsequent source assertion and ISR callback. The owning drainer still preserves
the final-publication/finalizer order. When the caller itself owns LEDC
delivery, it completes the final write and source request before returning, but
asynchronous POSIX-signal callback execution is not a before-return guarantee.

Generation cancellation prevents stale state commits but is not a lifetime
barrier. Callers that own fake GPIO topology or borrowed sink/callback state use
the lifecycle quiescence operation before destroying it; direct concurrent
`FakeGpioInterface::detach()` remains outside the supported synchronization
contract.

### Rejected Alternatives

#### Advance fake uptime when a fade starts

This would make `NO_WAIT` synchronous and skip observable intermediate duty.
Time advances only when the application explicitly drives it or wall-clock
auto-sync observes progress.

#### Advance fake uptime from a blocked LEDC call

Moving the global clock makes a task-local wait affect every task and can force
the timer's auto-sync logic to sleep until wall time catches up. It also skips
the scheduling opportunity that a real FreeRTOS semaphore wait creates. The
channel gate therefore blocks only its caller; another task/test driver or the
auto-sync native waiter is responsible for time progress.

#### Complete fades only when LEDC is queried

Pure lazy completion cannot deliver callbacks when a normal fake-time delay
crosses the deadline. The dependent alarm service provides the required
completion event; lazy materialization remains a deadlock-safe companion for
recursive pumps.

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
