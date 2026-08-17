#include "test/profile/global_environment_contract.h"

#ifndef ROO_TESTING_USER_COPT_PROBE
#error "Build with --copt=-DROO_TESTING_USER_COPT_PROBE=73"
#endif
#if ROO_TESTING_USER_COPT_PROBE != 73
#error "The user copt probe has the wrong value"
#endif

int roo_testing_user_copt_probe() { return ROO_TESTING_USER_COPT_PROBE; }
