#ifndef ADDRESS_SANITIZER
#error "Build with --config=asan"
#endif
#ifndef __SANITIZE_ADDRESS__
#error "The compiler did not enable AddressSanitizer instrumentation"
#endif

int roo_testing_asan_profile_probe() { return ADDRESS_SANITIZER; }
