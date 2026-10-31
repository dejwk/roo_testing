#pragma once
#include <stdint.h>

namespace roo_testing::system {
/// Reads actual host UTC solely as a simulated server source, bypassing
/// wrappers. Leaves output unchanged on host read failure or conversion
/// overflow.
bool GetHostUnixTimeMicros(int64_t& unix_us);
}  // namespace roo_testing::system
