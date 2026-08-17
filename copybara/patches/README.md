# Upstream source patches

The `arduino-esp32` and `esp-idf` directories contain ordered patch series for
the matching Copybara workflow. A series may be empty when all emulation logic
is implemented behind the stable `esp32_shims` boundary.

Each patch must apply with `-p1` at the root of the corresponding upstream
checkout. List one patch filename per line in `series`; blank lines and lines
beginning with `#` are ignored.
