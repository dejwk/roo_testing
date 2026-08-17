# ESP32 host shims

This package is the stable host-emulation boundary between roo_testing and the
Copybara-managed ESP-IDF/Arduino-ESP32 source trees.  Code in this directory is
owned by roo_testing and must not be copied into either upstream tree.  A
framework refresh may change the declarations included here, but must not
silently replace these implementations.

The package deliberately models observable library behaviour, not the ESP32
CPU or peripherals.  Unsupported hardware operations return success, a safe
default, or `ESP_ERR_NOT_SUPPORTED` as documented below.  Interrupt routing is
a no-op.

## Migrated customisations

The implementation was reconstructed from the custom sources and modified
framework files in the former `esp-idf-v4.4.1` and `arduino-esp32-2.0.4`
trees, then adjusted to ESP-IDF 5.5.4 / Arduino-ESP32 3.3.8 declarations.

| Area | Preserved host behaviour | Historical source |
| --- | --- | --- |
| Time/core | `esp_timer_get_time()` and Arduino delays use the roo_testing clock; watchdog and hardware-init calls are harmless | `tools/sdk/esp32/src/hal.cpp`, Arduino `esp32-hal-misc.c`, `esp32-hal-time.c` |
| Random/heap/log/ROM | Linux randomness and allocation, stdout/stderr logging, ROM printf/delay/interrupt stubs | IDF `esp_hw_support/hw_random.cpp`, `esp_rom/*.cpp`, Arduino `ets.cpp` |
| GPIO/matrix | Digital levels and signal-matrix routes use `FakeEsp32` | IDF `hal/esp32/gpio.cpp`, `esp_rom/gpio.cpp`, modified Arduino GPIO HAL |
| I2C/SPI/UART | Transfers are forwarded to the matching `FakeEsp32` bus | IDF `hal/esp32/i2c.cpp`, modified Arduino I2C/SPI HAL, `tools/sdk/esp32/src/uart.cpp` |
| NVS | IDF NVS handles and values use the existing fake persistent storage | `fake_esp32_nvs.cpp`, old NVS BUILD wiring |
| OTA/partitions | OTA writes and partition operations are accepted/no-op; queries return no real flash partitions | `tools/sdk/esp32/src/esp_ota.cpp`, `esp_partition.cpp` |
| Events | A synchronous, thread-safe host event loop implements the registration/posting subset used by Arduino networking | previously the in-tree IDF event component |
| Wi-Fi/netif/DNS | Scan/connect events use the fake Wi-Fi environment; sockets remain native Linux sockets; unsupported radio/AP controls are safe no-ops | IDF `esp_wifi/esp_wifi.cpp` and modified Arduino WiFi sources |
| ESP-NOW | Callbacks and payloads are forwarded to the fake ESP-NOW bus | IDF `esp_wifi/esp_now.cpp` |
| Arduino runtime | Modern `EspClass` dependencies and Arduino HAL entry points are backed by the same services above | modified Arduino core/HAL sources |

When an upstream declaration changes, prefer adapting this package over
patching the imported source.  Add a focused host test for any newly emulated
behaviour; do not add register-accurate hardware code here.

