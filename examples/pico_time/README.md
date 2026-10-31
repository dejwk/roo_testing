# Pico time in manual and host-following modes

Both targets compile the same SDK application. Each prints three elapsed uptime
samples separated by a 10 ms SDK sleep:

```sh
bazel run //examples/pico_time:manual_time --config=roo_testing_pico_rp2040
bazel run //examples/pico_time:auto_time --config=roo_testing_pico_rp2040
```

The manual executable links `//roo_testing/system:manual_time_mode`: sleeps
advance simulated uptime immediately, producing exact 10000, 20000, and 30000 us
samples. The auto executable uses the default shared clock and paces against
host time, with ordinary scheduling variation. Both require the RP2040 base and
Pico rc fragments in the root workspace; see the
[SDK support matrix](../../roo_testing/frameworks/pico_support/README.md).

These clock-only examples create no alarm worker. SDK alarm callbacks and other
peripherals remain outside the current API subset.
