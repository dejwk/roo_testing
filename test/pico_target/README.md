# RP2040 target compilation

This optional CMake harness uses pristine device SDK/core headers and an ARM
cross compiler. It does not use roo_testing host include overlays or define
`ROO_TESTING`. It links an SDK firmware probe, compiles the bounded Arduino
peripheral sketch, and compiles the supported Roo time/scheduler/display
transport consumer. Arduino/Roo outputs are object files, not complete
Arduino firmware. No firmware is flashed or run.

Supply Pico SDK 2.3.1 at `079c6f39023649b154152db30f1d781e884879bc`,
Arduino-Pico 6.3.1 at `53f1d8ee31a9565a15731c5aad8a6297bdf70144`, and
its ArduinoCore-API submodule at `82928635c893189343cf8eb78569f0c4136fded0`.
The SDK's lwIP submodule headers are required by the real Arduino umbrella
header: revision `77dcd25a72509eb83f72b033d219b1d40cd8eb95`. This does not enable
or validate networking in the host emulator. Supply sibling Roo checkouts
containing the coordinated client commits.

```sh
cmake -S test/pico_target -B "$HOME/.cache/roo_testing/pico-target" -G Ninja \
  -DPICO_SDK_PATH=/path/to/pico-sdk \
  -DPICO_TOOLCHAIN_PATH=/path/to/arm-toolchain \
  -DPICO_BOARD=pico -DPICO_NO_PICOTOOL=1 \
  -DROO_ARDUINO_PICO_PATH=/path/to/arduino-pico \
  -DROO_LWIP_PATH=/path/to/pico-sdk/lib/lwip \
  -DROO_CLIENT_ROOT=/path/to/sibling/roo/checkouts
cmake --build "$HOME/.cache/roo_testing/pico-target" --parallel 2
```

The toolchain directory contains `bin/arm-none-eabi-g++`. If CMake/Ninja are
outside PATH, use their absolute executable paths and pass
`-DCMAKE_MAKE_PROGRAM=/path/to/ninja` at configuration time. The physical
single-threaded Roo consumer sets the existing
`ROO_THREADS_SINGLETHREADED=1` option explicitly; it does not fake the native
emulation runtime identity. The configuration uses the original Pico at
125 MHz. Physical blink/UART/DS3231/ADC/PWM/display smoke tests remain a separate
release gate.
