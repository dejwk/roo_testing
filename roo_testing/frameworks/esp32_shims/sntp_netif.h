#pragma once

namespace roo_testing::esp32_shims {
/// Initializes the adapter mutex before application tasks use esp-netif SNTP.
void InitializeNetifSntpForHost();
}  // namespace roo_testing::esp32_shims
