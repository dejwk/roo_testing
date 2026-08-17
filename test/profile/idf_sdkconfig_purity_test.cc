#include "sdkconfig.h"

#include "test/profile/idf_environment_contract.h"

#if !defined(CONFIG_I2C_SUPPRESS_DEPRECATE_WARN) || \
    CONFIG_I2C_SUPPRESS_DEPRECATE_WARN != 1
#error "The host profile must explicitly suppress the retained legacy I2C API warning"
#endif

int main() { return 0; }
