#include "roo_testing/devices/clock/ds1307/ds1307.h"

FakeDs1307::FakeDs1307(roo_testing_transducers::Clock* clock)
    : I2cRtc("DS1307", 0x68, roo_testing_rtc::Chip::kDs1307, clock) {}
