# Native Pico SDK foundation

Pico integration stage 2 provides the SDK-only profile
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

The [patch series](../../../copybara/patches/pico-sdk/series) corrects the host
`time_us_32` symbol and disables unsupported alarm allocation. No GPIO, UART,
USB, clock-configuration, or other hardware success stubs are compiled.

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
are excluded. Exactly two upstream implementations are compiled:
`src/host/pico_platform/platform_base.c` and `src/host/hardware_timer/timer.c`.

## Support matrix after stage 2

| API or facility | Behavior |
| --- | --- |
| `pico.h`, SDK types/version, `roo_testing/soc_profile.h` | Native host definitions with typed RP2040 identity. |
| C stdio (`printf`, etc.) | Ordinary libc stdin/stdout; no USB or UART hardware initialization. |
| `time_us_64`, `time_us_32`, `get_absolute_time`, inline timestamp arithmetic | Upstream host `CLOCK_MONOTONIC` time; 32-bit truncation is supported. |
| `busy_wait_us_32`, `busy_wait_us`, `busy_wait_ms`, `busy_wait_until` | Upstream host waits; not modeled CPU occupancy and not integrated with manual time. |
| `get_core_num` | Always 0 for this single-threaded application runtime. |
| Hardware alarm allocation | Optional allocation returns -1; required allocation/claim/unclaim traps explicitly. |
| Hardware alarm callbacks/targets, unsupported panic paths | Upstream explicit panic/trap; no successful no-op. |
| SDK sleeps, nil/end-of-time constants, SDK alarm pools | Declarations alone do not imply support; missing implementations fail to link. |
| `pico/stdlib.h`, GPIO/UART/SPI/I2C/ADC/PWM, core 1, USB, filesystem, FreeRTOS | Not exported through this stage's include/link surface. |

Time is deliberately the SDK's native host clock in this bootstrap stage. It
uses the host monotonic origin rather than roo_testing's emulated uptime. Do
not mix it with manual-time device tests. [Stage 3](../../../doc/pico_integration.md#3-unify-time-and-sdk-test-lifecycle)
replaces this implementation with the shared emulator clock and adds SDK sleep
and test lifecycle support. No second implementation is linked alongside it.

Host SDK headers contain additional inline primitives; their presence is not a
claim of interrupt, synchronization, or hardware fidelity. Only the operations
listed above are supported. Arduino-Pico's default and optional FreeRTOS modes
remain later stages of the [integration design](../../../doc/pico_integration.md).

## Running and checking

```sh
bazel run //test/pico:sdk_probe_c --config=roo_testing_pico_rp2040
bazel test //:pico_tests --config=roo_testing_pico_rp2040
python3 test/pico/verify_pico.py
```

The verifier checks C/C++ execution, profile failures, dependency/action
isolation, one implementation per SDK symbol, native ELF identity, unsupported
alarm failure, and ASAN composition. It uses the existing persistent validation
output root and preserves the global Bazel cache and resource limits.
