# Voltage waveform design documents

The original combined proposal has been split into three design documents:

1. [Periodic voltage signals](periodic_voltage_signals.md) defines the reusable
   signal model, analysis functions, voltage sinks, and fake-GPIO propagation.
2. [Emulated-time alarms](emulated_time_alarms.md) defines the generic one-shot
   deadline service used by emulated asynchronous peripherals, building on the
   separate [emulated-time clock](emulated_time_clock.md).
3. [LEDC voltage emulation](ledc_voltage_emulation.md) builds on both designs to
   emulate ESP32 LEDC PWM, fades, completion behavior, and roo_blink use cases.

The periodic-signal and alarm designs are independent. Complete the periodic
design before steady LEDC publication and the alarm design before LEDC fades;
the LEDC design declares both dependencies and does not redefine their APIs or
behavior.
