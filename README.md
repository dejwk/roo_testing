# roo_testing

Experimental ESP32 emulator that can be used to test Arduino sketches on Linux, before uploading them to the microcontroller. Supports emulation of external I2C and SPI devices, including TFT displays.

## TL;DR:

```
sudo apt-get install bazel
mkdir foo; cd foo
git clone git@github.com:dejwk/roo_testing.git
cp -R roo_testing/examples/gpio/* .
bazel run :main
```

## More interesting example

Now let's look at an example that simulates an external I2C real-time clock, using the popular DS3231 module:

```
sudo apt-get install bazel
mkdir foo; cd foo
git clone git@github.com:dejwk/roo_testing.git
cp -R roo_testing/examples/tft_display/* .
lib/init.sh
bazel run :main
```

## Even more interesting example

Now let's simulate a SPI-based TFT display. This example requires an FLTK library.

```
sudo apt-get install bazel
sudo apt-get install libfltk1.3-dev
mkdir foo; cd foo
git clone git@github.com:dejwk/roo_testing.git
cp -R roo_testing/examples/tft_display/* .
lib/init.sh
bazel run :main
```
