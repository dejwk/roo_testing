#pragma once

/// Draws a bounded 16x16 RGB565 red/green pattern through Pico SDK SPI0.
/// Uses SCK GP18, TX GP19, software CS GP17, DC GP20, and reset GP21.
/// Returns false if an SDK transfer fails; no host/display fixture is required
/// by this application code.
bool DrawPicoSpiPattern();
