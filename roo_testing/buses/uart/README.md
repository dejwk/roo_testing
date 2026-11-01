# Shared UART byte devices

`FakeUartDevice` is the byte interface used by the ESP32 and Pico board adapters.
`FakeUartCable` connects two thread-safe queues; writes deliver bytes immediately
to the opposite endpoint and notify its receiver. Pin routing, SDK lifecycles,
blocking loops, emulated time, and scheduler policy belong to each board/runtime.

`ConsoleUartDevice` is the common Linux stdin/stdout bridge. Its default
constructor uses descriptor 0 for input and 1 for output; fixtures can supply
borrowed descriptors for isolated pipe tests. The device never closes them or
changes terminal modes/descriptor flags. Use one console as the sole reader of
its descriptor; mixing other readers, libc, or iostream input is unsupported.

Input readiness polls without waiting and retains a one-byte lookahead. Reads
copy only ready bytes, return the actual count, and leave the rest of the caller's
buffer untouched. A read batch is limited to 256 bytes to bound the shared
host-state critical section. Input calls serialize with `SchedulerSafeHostLock`
so FreeRTOS task handoff cannot strand a held input mutex. Output does not hold
that lock. Both directions use POSIX descriptor I/O. Output retries interrupted
and partial writes until the entire buffer has been accepted, waiting for writable
readiness on EAGAIN. This preserves ESP32's complete-buffer return contract and
blocking behavior. Direct `write()` bypasses stdio buffering: a standalone
`Serial.println()` is visible without subsequent traffic, a later flush, or
process exit. No `fflush()`, `fsync()`, or terminal drain operation is needed.
`availableForWrite()` retains the base device's value of one instead of reporting
host pipe capacity. The console performs no text translation. No background
reader or interrupt/event pump is introduced.

After reads/readiness observe EOF, `inputClosed()` returns true. Empty open input
returns zero without reporting closure. Live devices such as cables inherit the
false default. This separates permanent input closure from a temporary lack of
data: Pico's blocking adapter diagnoses EOF; ESP32 nonblocking reads return zero.

The shared implementation replaces the earlier iostream bridge's blocking
`peek()` availability check and zero-filling short reads. ESP32 still constructs
the same public device class on its console pins. Pico instantiates that class
for its GP0/GP1 fallback; neither board depends on the other's framework.

```sh
bazel test //roo_testing/buses/uart:console_test --config=roo_testing_pico_rp2040
bazel test //test:arduino_console_test --config=roo_testing_arduino_esp32
bazel test //test:idf_console_test --config=roo_testing_idf_esp32
```

The profile verifier runs stdin/stdout subprocess checks through ESP32 board
wiring on all three chips and both frontends. The Pico verifier runs the same
checks with Pico blocking-EOF behavior, including ASAN. A live child writes once
and waits for input; its output must arrive before later communication or exit.
