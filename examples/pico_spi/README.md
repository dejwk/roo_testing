# Headless Pico SPI display example

The SDK-only [application](pattern.cpp) draws a bounded 16x16 RGB565 pattern:
red on the left half and green on the right. It configures SPI0 SCK GP18, TX GP19,
software CS GP17, DC GP20, and reset GP21. No MISO is needed for these writes.
Commands use 8-bit frames; pixels use 16-bit words sent high byte first.

The [host fixture](main.cpp) attaches the existing `FakeSt77xxSpi` model and a
recording viewport before application configuration. It checks five command
bytes, the parameter bytes, both 256-byte pixel chunks under active CS, and every
final pixel. Rendering uses the real shared display parser. No GUI toolkit or
display server is needed, and the application exits after drawing one frame.

```sh
bazel run //examples/pico_spi:display_example --config=roo_testing_pico_rp2040
bazel test //examples/pico_spi:display_test --config=roo_testing_pico_rp2040
```

The executable follows the host clock. The test selects manual emulated time;
both run the same application and assert captured transactions and pixels.
See the [SPI support matrix](../../roo_testing/frameworks/pico_support/README.md#synchronous-spi-stage-6)
for supported formats, legal routes, error behavior, and transfer duration.
