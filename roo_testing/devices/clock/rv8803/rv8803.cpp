#include "roo_testing/devices/clock/rv8803/rv8803.h"

FakeRv8803::FakeRv8803(roo_testing_transducers::Clock* clock)
    : I2cRtc("RV-8803-C7", 0x32, roo_testing_rtc::Chip::kRv8803, clock) {}
