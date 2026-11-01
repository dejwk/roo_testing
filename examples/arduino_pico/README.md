# Arduino-Pico peripherals

Run `bazel run --config=roo_testing_arduino_rp2040 //examples/arduino_pico:peripherals -- --iterations=2`.
The public sketch helper selects native Arduino-Pico startup. Each bounded
cycle blinks the LED, crosses Serial1/Serial2 through the shared UART cable,
reads the existing DS3231 model, and exchanges bytes with an SPI device.
Serial uses the same unbuffered host console as SDK UART0 by default.
On hardware, connect GPIO0/1 to GPIO9/8, a DS3231 on GPIO4/5 and an SPI loopback
on GPIO19/16 with GPIO18 clock and GPIO17 chip select. Host wiring is excluded
from hardware compilation. This is a host regression; it has not been run on
a physical board.
