#include "roo_testing/devices/clock/pcf8523/pcf8523.h"

FakePcf8523::FakePcf8523(roo_testing_transducers::Clock* clock)
    : I2cRtc("PCF8523", 0x68, roo_testing_rtc::Chip::kPcf8523, clock) {}
