# Arduino host filesystems

This package keeps roo_testing's filesystem behavior separate from the
Copybara-managed Arduino-ESP32 source tree. Public framework headers define the
API, while these implementations translate every mounted path through
`FakeEsp32().fs_root()`.

- SPIFFS and LittleFS mount a host directory, report zero capacity (matching
  the existing SPIFFS shim), and treat formatting as a successful no-op.
- SD preserves the existing fake SDHC card values and unsupported raw-sector
  operations.
- SD_MMC uses the same host-backed behavior without attempting to configure
  physical pins or a hardware controller.

Applications still select their mount names with the normal Arduino `begin()`
methods. The corresponding directories must exist beneath the configured fake
filesystem root.
