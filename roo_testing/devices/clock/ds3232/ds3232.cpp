#include "roo_testing/devices/clock/ds3232/ds3232.h"

FakeDs3232::FakeDs3232(roo_testing_transducers::Clock* clock)
    : I2cRtc("DS3232", 0x68, roo_testing_rtc::Chip::kDs3232, clock) {}
