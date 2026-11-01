# Pico I2C DS3231 example

A bounded native SDK program uses the existing DS3231 model on GP4/GP5. It
checks alarm-register selection with repeated START and a 25 C temperature read.

```sh
bazel run //examples/pico_i2c:rtc_example --config=roo_testing_pico_rp2040
```
