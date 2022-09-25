
#include "fake_esp32_wifi.h"

namespace {
roo_testing_transducers::wifi::Environment empty_env;
}

Esp32WifiAdapter::Esp32WifiAdapter()
      : mac_address_(0x78, 0x21, 0x84, 0x01, 0x01, 0x01),
        env_(&empty_env),
        scan_completed_(false),
        state_(DISCONNECTED) {}

