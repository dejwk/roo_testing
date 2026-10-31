# SNTP clocks

These examples use ordinary platform APIs to observe a cold clock and wait for
synchronization. Each source explains its steps. The emulator supplies a virtual
server using host UTC; there is no DNS/UDP exchange and no board is needed.
On real hardware, add network setup before starting SNTP.

From the roo_testing repository root, select your frontend:

```sh
bazel run //examples/sntp/SntpClock:SntpClock --config=roo_testing_arduino_esp32
bazel run //examples/sntp/idf:SntpClock --config=roo_testing_idf_esp32
```

Use Ctrl+C to stop either runner. Arduino continues printing after sync;
ESP-IDF prints its demonstration once and then the runner idles. The first line
shows epoch-era seconds; after the virtual response the clock shows current UTC.
Serial uses 115200 baud on a board and the terminal in the emulator.

Run bounded tests (they terminate automatically):

```sh
bazel test //examples/sntp/... --config=roo_testing_arduino_esp32
bazel test //examples/sntp/... --config=roo_testing_idf_esp32
```

Add `--config=asan` for AddressSanitizer. Replace `esp32` with `esp32s3` or
`esp32c3` to test the other supported SoCs. The incompatible frontend is skipped.
See the [SNTP reference](../../doc/sntp_reference.md) for deterministic and silent
server scenarios, clock behavior, and supported APIs.
