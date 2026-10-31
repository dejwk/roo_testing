# Native Pico SDK time and test runtime

Pico integration stages 2 and 3 provide the SDK-only profile
`--config=roo_testing_pico_rp2040` and the public `@roo_testing//:pico_sdk` target.
Applications supply ordinary `main()` and execute with the Linux host ABI/CPU.
Arduino-Pico sources are staged for a later release stage; no Arduino RP2040
profile or core implementation is exposed yet.

## Pinned sources and ownership

| Source | Version | Immutable revision |
| --- | --- | --- |
| Arduino-Pico | 6.3.1 | `53f1d8ee31a9565a15731c5aad8a6297bdf70144` |
| Pico SDK | 2.3.1 | `079c6f39023649b154152db30f1d781e884879bc` |

The SDK revision is the `pico-sdk` gitlink in that Arduino-Pico commit. Both
SDK-only and later Arduino builds use this one SDK copy. Imports use the
[Copybara workflows](../../../copybara/README.md) and
[version pins](../../../copybara/versions.bara.sky). Pristine imported files
retain upstream formatting and licenses. BUILD files and
`roo_testing_host_include` are destination-owned. The generated version header
is the pinned upstream template expanded to 2.3.1; `config_autogen.h` supplies the
original Pico board header without switching execution to the device platform.

The [patch series](../../../copybara/patches/pico-sdk/series) retains the stage-2
host timer fixes and corrects `delayed_by_us` saturation before unsigned
addition can wrap. The upstream host timer implementation is now excluded from
the link. SDK time entry points are owned by [time.cpp](time.cpp), and no GPIO,
UART, USB, or clock-configuration success stubs are compiled.

## Profile and headers

Import these files into the root workspace's `.bazelrc`:

```text
import %workspace%/.roo_testing/bazelrc/common/asan.bazelrc
import %workspace%/.roo_testing/bazelrc/rp2040/base.bazelrc
import %workspace%/.roo_testing/bazelrc/rp2040/pico.bazelrc
```

The profile selects the native host platform plus RP2040 SoC, Pico SDK
capability, SDK-only frontend, and bare-metal runtime. Global compile identity is
`ROO_TESTING=1`, `ROO_TESTING_SOC="rp2040"`, `ROO_TESTING_SOC_RP2040=1`,
`ROO_TESTING_RUNTIME_BARE_METAL=1`, `PICO_BUILD=1`, `PICO_NO_HARDWARE=1`,
`PICO_ON_DEVICE=0`, `PICO_RP2040=1`, `PICO_RP2350=0`,
`PICO_PLATFORM_STRING="host"`, and `PICO_BOARD="pico"`.
No Arduino, ESP-IDF, ARM-compiler, or RISC-V-compiler identity is injected.
Physical RP2040 identity does not change the native ABI.

Exposed include roots are `src/common/pico_base_headers/include`,
`src/common/pico_time/include`, `src/host/pico_platform/include`,
`src/host/hardware_timer/include`, `src/boards/include`, and
`roo_testing_host_include`. Device register roots and host GPIO/UART include roots
are excluded. Only `src/host/pico_platform/platform_base.c` is compiled from the
upstream SDK. The [time adapter](time.cpp) and existing shared
[clock implementation](../../system/timer.cpp) supply time operations. There is
one linked implementation of each supported SDK entry point.

## Support matrix after stage 3

| API or facility | Behavior |
| --- | --- |
| `pico.h`, SDK types/version, `roo_testing/soc_profile.h` | Native host definitions with typed RP2040 identity. |
| C stdio (`printf`, etc.) | Ordinary libc stdin/stdout; no USB or UART hardware initialization. |
| `time_us_64`, `time_us_32`, `get_absolute_time`, `time_reached` | Passive reads/comparisons of the shared emulated uptime; the narrow counter truncates. |
| `nil_time`, `at_the_end_of_time`, inline timestamp arithmetic | SDK sentinel values and conversions, including saturation before unsigned addition overflow. |
| `sleep_us`, `sleep_ms`, `sleep_until` | Shared emulated-time delays; manual mode advances deterministically, auto mode uses shared host pacing. |
| `busy_wait_us_32`, `busy_wait_us`, `busy_wait_ms` | Host-monotonic busy waits without explicit emulated-time publication or callback dispatch. |
| `busy_wait_until` | Auto mode rechecks shared uptime until the target; manual mode consumes the sampled remaining host duration without changing manual time. |
| `best_effort_wfe_or_timeout` | Sleeps until the timeout and returns true; processor events are not modeled. |
| `get_core_num` | Always 0 for this single-threaded application runtime. |
| Hardware alarm allocation | Optional allocation returns -1; required allocation/claim/unclaim fails explicitly. |
| Hardware alarm callbacks/targets | `LOG(FATAL)` with the unsupported operation; SDK interrupt context is not emulated. |
| SDK alarm pools, repeating timers | Unavailable implementations fail to link. |
| `pico/stdlib.h`, GPIO/UART/SPI/I2C/ADC/PWM, core 1, USB, filesystem, FreeRTOS | Not exported through this stage's include/link surface. |

### Shared clock behavior and limits

The [clock contract](../../../doc/emulated_time_clock.md) and
[shared alarm contract](../../../doc/emulated_time_alarms.md) apply to Pico too.
Time mode is fixed by the executable's link dependencies before global
constructors. The default follows host elapsed time; depending on
`//roo_testing/system:manual_time_mode` selects manual progression. SDK reads do
not dispatch work. SDK sleeps use the shared chronological alarm pump, so
emulated device callbacks agree with application timestamps. Those callbacks
are native shared-service callbacks, not Pico SDK interrupt handlers.

Zero sleeps and nil/past deadlines return without advancing or dispatching.
Milliseconds are widened before multiplication. Shared uptime is signed
nanoseconds: finite microsecond deadlines and durations are checked against
`INT64_MAX / 1000`, and the shared service additionally checks addition against
the current precise uptime. Invalid waits fail before publication. The SDK
end-of-time marker remains `INT64_MAX` microseconds and is never reached by the
shared clock; waiting until it fails explicitly instead of wrapping or
pretending to complete. The marker remains valid for comparisons and saturated
timeout arithmetic.

Busy waits obey the shared host-busy-wait contract. In manual mode they do not
make `time_reached(future_deadline)` become true; the driver still controls
progression. In auto mode subsequent reads observe elapsed host time. Host
busy-wait conversions and host deadline addition are also range checked. This
API subset does not promise hardware power behavior or cycle-accurate timing.

### Native test runner and shutdown

Use `@roo_testing//:pico_gtest_main` for GoogleTest tests using this SDK backend.
It runs tests on the native main thread, initializes the already link-selected
clock without resetting uptime, and returns the GoogleTest result. On both
success and failure it waits for active shared delivery to finish, closes the
alarm service, joins its worker, and destroys pending callback captures before
returning. Manual mode uses the same cleanup without starting a worker.

The runner owns process shutdown; tests must not close the shared service
independently. SDK applications retain ordinary `main()`. Applications that
register shared emulated-device alarms own the corresponding two-part shutdown
from `roo_testing/system/timer.h`; the clock-only examples start no worker.

```starlark
cc_test(
    name = "my_pico_test",
    srcs = ["my_pico_test.cpp"],
    deps = ["@roo_testing//:pico_gtest_main"],
)
```

For deterministic tests add
`@roo_testing//roo_testing/system:manual_time_mode` to the executable's deps.
Do not select time mode per source or switch it at runtime. Optional
`PICO_OPAQUE_ABSOLUTE_TIME_T=1` must be a global compiler option for the complete
build so SDK function declarations and implementations share the same ABI.

Host SDK headers contain additional inline primitives; their presence is not a
claim of interrupt, synchronization, or hardware fidelity. Only the operations
listed above are supported. Arduino-Pico's default and optional FreeRTOS modes
remain later stages of the [integration design](../../../doc/pico_integration.md).

## Running and checking

```sh
bazel run //test/pico:sdk_probe_c --config=roo_testing_pico_rp2040
bazel test //:pico_tests --config=roo_testing_pico_rp2040
python3 test/pico/verify_pico.py
bazel run //examples/pico_time:manual_time --config=roo_testing_pico_rp2040
bazel run //examples/pico_time:auto_time --config=roo_testing_pico_rp2040
```

The verifier checks C/C++ execution, profile failures, dependency/action
isolation, one implementation per SDK symbol, native ELF identity, unsupported
alarm failure, manual/auto clock behavior, opaque timestamps, runner result
propagation and post-main cleanup, and ASAN composition. It uses the existing persistent validation
output root and preserves the global Bazel cache and resource limits.
