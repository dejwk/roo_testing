// Compile guard for the emulator layer. This target carries no definitions of
// its own, so a matching platform without its bazelrc profile fails loudly.

#ifndef ROO_TESTING
#error "Enable roo_testing_idf_esp32 or roo_testing_arduino_esp32"
#endif
#if ROO_TESTING != 1
#error "ROO_TESTING has an unsupported value"
#endif

int roo_testing_emulator_profile_contract(void) { return 0; }
