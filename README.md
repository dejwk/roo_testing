# roo_testing

Experimental ESP32 emulator that can be used to test Arduino sketches on Linux, before uploading them to the microcontroller. Supports emulation of external I2C and SPI devices, including TFT displays.

I built it to test my sketches that tend to use a variety of external devices, from TFT displays to temperature sensors, networking, SD cards, and real-time clock modules. It is not 100% accurate, but it gets the job done.

## TL;DR:

```
sudo apt-get install bazel
mkdir foo; cd foo
git clone https://github.com/dejwk/roo_testing.git
cp -R roo_testing/examples/gpio/* .
bazel run :main
```

## More interesting example

Now let's look at an example that simulates an external I2C real-time clock, using the popular DS3231 module:

```
sudo apt-get install bazel
mkdir foo; cd foo
git clone https://github.com/dejwk/roo_testing.git
cp -R roo_testing/examples/rtc_ds3231_i2c/* .
lib/init.sh
bazel run :main
```

## Even more interesting example

Now let's simulate a SPI-based TFT display. This example requires an FLTK library.

```
sudo apt-get install bazel
sudo apt-get install libfltk1.3-dev
mkdir foo; cd foo
git clone https://github.com/dejwk/roo_testing.git
cp -R roo_testing/examples/tft_display/* .
lib/init.sh
bazel run :main
```

# How it works

The code is based on the Espressif ESP32 Arduino framework, instrumented to intercept hardware- or interface-level events and couple them with simulated implementations.

The code does not does not emulate the Xtensa microprocessors. Your sketch simply compiles and runs on your computer's architecture. This works because the Espressif framework is implemented in a standard, portable C/C++.

## How to use it

The behavior of the physical world is modeled in _transducers_, sensing or indicating physical quantities. Transducers are represented by abstract virtual classes. A simple example is the Thermometer class, with a virtual method to report the temperature. By implementing an arbitrary logic in your own subclasses, you can simulate various real-life scenarios.

The transducers are used by the simulated _devices_, provided as part of the library and mimicking the real hardware, that you virtually 'connect' to your microcontroller at the beginning of the program (as illustrated in the examples). For example, the FakeOneWireThermometer device simulates an actual sensor such as DS18B20, and communicates with the emulated microcontroller using the actual One Wire protocol, but reports temperatures indicated by your custom thermometer sensor.

Another basic example is the VoltageSource, which is a transducer that you can use to feed signals to the microcontroller via its GPIO pins. Using emulated GPIO and voltage inputs, you can emulate an external logic, e.g. calculate a logical function of some GPIO outputs and feed it back to a GPIO input. 

## What is supported

* GPIO, both digital I/O and analog inputs
* SPI, emulated at pin level, accurately modeling bus speeds
* I2C
* Networking
* SPIFFS, mouting a local directory
* SD (rudimentary)
* External devices: a couple of TFT displays, the DS3231 real time clock, and the temperature sensors using the OneWire interface.

## Limitations (call for contributors!)

* Only the Arduino framework is supported for now.
* Networking is very rudimentary: most functions are no-op, and the network bridges to your native connection. As long as your computer is connected to the network, the emulated microcontroller will also have network access.
* SD is also very rudimentary; it redirects file system operations to a local directory, without emulating any of the SPI protocol. (The consequence is, for example, that performance is unrealistically fast).
* I2C is modeled at the interface level, bypassing some low-level OS queues and hardware pins. (As long as you use standard libraries, it doesn't matter much).
* Simulated TFT displays don't model all commands, just the basic set used by common libraries.
* Interrupts are not currently supported.
* The emulator does not accurately reflect the microcontroller's performance - it tends to run faster because your computer has a faster CPU. (Notable exception is the SPI emulation, which reflects communication delays accurately). Also, your computer has way more memory, both on the heap and the stack. Finally, the int type is (most likely) 32-bit on your computer, but only 16-bit on the microcontroller. (It may be a good habit to use explicit integer types, like int16_t, in your sketches). Because of all that, your sketch may run great on the simulator, but still fail miserably on the real device.

## Debugging with VS Code

Yes! You can debug your sketches using a graphical debugger.

in VS Code, navigate to Run > Add Configuration ..., then select "(gdb) Launch" as the configuration type. Change "program" to "${workspaceFolder}/bazel-bin/main", and "cwd" to "${workspaceFolder}". Finally, build the debug binary by calling

```
bazel build -c dbg :main
```
 
After that, you can Run > Start Debugging (make sure to select the just created configuration).

# Please get involved!

If you find this library useful, but perhaps missing something important for you, please consider contributing. I will be happy to guide and I will gladly review and accept external contributions.
