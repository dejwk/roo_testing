#pragma once

#include <stdint.h>

namespace roo_testing::esp32_shims {

// A deterministic, locally administered MAC address keeps host tests
// repeatable while avoiding use of a real machine interface address.
constexpr uint8_t kDefaultMac[6] = {0x02, 0x00, 0x00, 0x12, 0x34, 0x56};

}  // namespace roo_testing::esp32_shims

