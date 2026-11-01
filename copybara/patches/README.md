# Upstream source patches

The `arduino-esp32` and `esp-idf` directories contain ordered patch series for
the matching Copybara workflow. A series may be empty when all emulation logic
is implemented behind the stable `esp32_shims` boundary.

Each patch must apply with `-p1` at the root of the corresponding upstream
checkout. List one patch filename per line in `series`; blank lines and lines
beginning with `#` are ignored.

The `pico-sdk` series fixes the upstream host `time_us_32` symbol and rejects
alarm allocation until a Pico timer/interrupt adapter exists. `arduino-pico`
starts with an empty series; its staged sources are not compiled in stage 2.

Pico stage 3 excludes the imported host timer from the link and implements SDK
clocks and waits in destination-owned `pico_support/time.cpp`. Its additional
header patch saturates `delayed_by_us` before unsigned addition overflow; the
stage-2 timer patches remain recorded for reproducible source refreshes.

Pico SDK `0004-expose-supported-host-uart-api.patch` corrects the RP2040
`UART_FUNCSEL_NUM` value and adds signature-compatible declarations for instance
identity, enable state, CRLF control, synchronous TX completion, bounded RX
readiness, and the current SDK IRQ spelling. The destination-owned runtime
implements these operations or rejects unsupported IRQs. The upstream host
`uart.c` is never compiled; imported source ownership remains with Copybara.
