
#include "fake_esp32_wifi.h"

namespace {
roo_testing_transducers::wifi::Environment empty_env;
}

Esp32WifiAdapter::Esp32WifiAdapter()
      : scan_completed_(false),
        env_(&empty_env),
        connected_(false) {}

