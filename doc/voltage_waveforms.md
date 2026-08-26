# Voltage waveform design documents

The original combined proposal has been split into two independently
implementable designs:

1. [Periodic voltage signals](periodic_voltage_signals.md) defines the reusable
   signal model, analysis functions, voltage sinks, and fake-GPIO propagation.
2. [LEDC voltage emulation](ledc_voltage_emulation.md) builds on that model to
   emulate ESP32 LEDC PWM, fades, completion behavior, and roo_blink use cases.

Implement the periodic-signal design first. The LEDC design declares it as a
dependency and does not redefine its API or mathematics.
