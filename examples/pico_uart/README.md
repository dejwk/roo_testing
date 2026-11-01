# Bounded Pico UART cable example

Two native SDK controllers exchange `ping` and `pong` over an existing
`FakeUartCable`, then exit. The fixture attaches UART0 GP0/GP1 and UART1 GP4/GP5
before mux setup. Each receive first uses a bounded readiness wait; the expected
four bytes are already queued by the synchronous peer write. No host input or
Arduino startup is needed.

```sh
bazel run //examples/pico_uart:serial_example --config=roo_testing_pico_rp2040
bazel test //examples/pico_uart:serial_test --config=roo_testing_pico_rp2040
```

The test selects manual emulated time. The executable uses the default host
clock. Both execute the same bounded SDK application and validate its result.
