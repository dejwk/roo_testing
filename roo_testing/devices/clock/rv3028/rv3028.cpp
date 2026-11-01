#include "roo_testing/devices/clock/rv3028/rv3028.h"

FakeRv3028::FakeRv3028(roo_testing_transducers::Clock* clock)
    : I2cRtc("RV-3028-C7", 0x52, roo_testing_rtc::Chip::kRv3028, clock) {}
