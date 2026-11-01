#pragma once

#include "roo_testing/buses/i2c/fake_i2c.h"

/// Returns the selected ESP32 chip's stable legacy controller, or nullptr.
/// Retained for callers of the original shared header; link the ESP32 backend.
FakeI2cInterface* getI2cInterface(uint8_t i2c_num);
