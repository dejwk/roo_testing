// Verifies platform runtime selections agree with compiler identity,
// independent of the Arduino/IDF frontend. Runtime contract checks also cover C
// compilation.
#if defined(EXPECT_FREERTOS)
#if !defined(ROO_TESTING_RUNTIME_FREERTOS) || \
    ROO_TESTING_RUNTIME_FREERTOS != 1 ||      \
    defined(ROO_TESTING_RUNTIME_BARE_METAL)
#error "FreeRTOS selection disagrees with compiler runtime identity"
#endif
#elif defined(EXPECT_BARE_METAL)
#if !defined(ROO_TESTING_RUNTIME_BARE_METAL) || \
    ROO_TESTING_RUNTIME_BARE_METAL != 1 ||      \
    defined(ROO_TESTING_RUNTIME_FREERTOS)
#error "Bare-metal selection disagrees with compiler runtime identity"
#endif
#else
#error "A supported runtime selection is required"
#endif

int main() { return 0; }
