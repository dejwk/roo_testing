// Compile guard for the generic Arduino frontend. Backend-specific identity
// is checked by the selected core, independently of Arduino capability.

#ifndef ARDUINO
#error "The selected Arduino platform requires the roo_testing compiler profile"
#endif
#if ARDUINO != 10819
#error "ARDUINO has an unsupported compatibility value"
#endif

int roo_testing_arduino_profile_contract() { return 0; }
