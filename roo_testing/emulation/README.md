# Emulator build helpers

## Arduino

Load the public sketch macro from roo_testing:

```starlark
load("@roo_testing//roo_testing/emulation:arduino.bzl", "roo_arduino_example")

roo_arduino_example(
    name = "demo",
    sketch = "demo/demo.ino",
    deps = ["//:application"],
)
```

`name` is the runnable native `cc_binary`, so `bazel run //examples:demo`
starts it through roo_testing's stable `arduino_main`. The macro wraps the
package-local `.ino` in generated C++, makes its directory available for quoted
includes, forwards caller dependencies and ordinary `cc_binary` attributes,
and marks all generated targets incompatible outside an Arduino frontend.

The emulator compiles the sketch as ordinary C++. It does not add Arduino's
automatic function prototypes; include `Arduino.h` when needed and declare
functions before use.

## ESP-IDF

Use `roo_esp_idf_example` for an application that defines `app_main()`:

```starlark
load("@roo_testing//roo_testing/emulation:esp_idf.bzl", "roo_esp_idf_example")

roo_esp_idf_example(
    name = "demo",
    srcs = ["main.cpp"],
    deps = ["//:application"],
)
```

The resulting target is a native executable that starts `app_main()` through
roo_testing's ESP-IDF/FreeRTOS entry point. Run it with the ESP-IDF profile:

```sh
bazel run //examples/espidf/demo --config=roo_testing_idf_esp32
```

The vendored roo_testing Bazel wrapper also recognizes an ESP-IDF example path
on `bazel run`, so the explicit config may be omitted for that command.
