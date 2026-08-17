# Arduino emulator build helpers

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
