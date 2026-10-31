# Bounded Pico blink

The SDK application toggles the original Pico LED pin (GP25) through three
one-millisecond high/low cycles and then exits:

```sh
bazel run //examples/pico_gpio:blink_example --config=roo_testing_pico_rp2040
bazel test //examples/pico_gpio:blink_test --config=roo_testing_pico_rp2040
```

The executable follows host time by default. Its test links manual time and
attaches a shared voltage sink to GP25 before configuring it. The test checks
initial low drive, six subsequent edges, and exact timestamps through 6000 us.
No display server or physical LED is needed. Root setup and current API limits
are described in the [support matrix](../../roo_testing/frameworks/pico_support/README.md).
