// Validate global runtime macros against the selected platform. Expected values
// are private to this contract; public identity still comes only from rc copts.

#if (defined(ROO_TESTING_RUNTIME_BARE_METAL) + \
     defined(ROO_TESTING_RUNTIME_FREERTOS)) != 1
#error "Select exactly one roo_testing application runtime"
#endif

#if defined(ROO_TESTING_EXPECT_BARE_METAL)
#if !defined(ROO_TESTING_RUNTIME_BARE_METAL) || \
    ROO_TESTING_RUNTIME_BARE_METAL != 1
#error "Compiler runtime identity does not match the bare-metal platform"
#endif
#elif defined(ROO_TESTING_EXPECT_FREERTOS)
#if !defined(ROO_TESTING_RUNTIME_FREERTOS) || ROO_TESTING_RUNTIME_FREERTOS != 1
#error "Compiler runtime identity does not match the FreeRTOS platform"
#endif
#endif

int RooTestingRuntimeProfileContract(void) { return 0; }
