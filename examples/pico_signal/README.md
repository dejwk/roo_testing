# Pico ADC/PWM signal example

A borrowed 1.65 V source on GP26 is sampled as 2048/4096. The code publishes
a shared 1 kHz, 50% duty square signal on GP0 and checks retained samples.

```sh
bazel run //examples/pico_signal:signal_example --config=roo_testing_pico_rp2040
```
