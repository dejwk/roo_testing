load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "scheduler_safe_host_lock_test",
    size = "small",
    srcs = ["scheduler_safe_host_lock_test.cpp"],
    deps = [
        "//roo_testing/host:synchronization",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "emulator_environment_test",
    size = "small",
    srcs = [
        "arduino_environment_contract.h",
        "emulator_environment_test.cpp",
        "idf_environment_contract.h",
    ],
    copts = ["-Werror"],
    deps = ["//:environment"],
)

cc_test(
    name = "idf_environment_test",
    size = "small",
    srcs = [
        "arduino_environment_contract.h",
        "idf_environment_contract.h",
        "idf_environment_test.cpp",
    ],
    copts = ["-Werror"],
    deps = ["//roo_testing/frameworks/esp-idf:headers"],
)

cc_test(
    name = "soc_environment_test",
    size = "small",
    srcs = [
        "arduino_environment_contract.h",
        "idf_environment_contract.h",
        "soc_environment_test.cpp",
    ],
    copts = ["-Werror"],
    deps = ["//roo_testing/soc:target"],
)

cc_test(
    name = "arduino_environment_test",
    size = "small",
    srcs = [
        "arduino_environment_contract.h",
        "arduino_environment_test.cpp",
        "idf_environment_contract.h",
    ],
    copts = ["-Werror"],
    linkstatic = 1,
    deps = ["//:arduino"],
)

cc_test(
    name = "arduino_main_environment_test",
    size = "small",
    timeout = "short",
    srcs = [
        "arduino_environment_contract.h",
        "arduino_main_environment_test.cpp",
        "idf_environment_contract.h",
    ],
    copts = ["-Werror"],
    linkstatic = 1,
    deps = ["//:arduino_main"],
)

cc_test(
    name = "arduino_gtest_environment_test",
    size = "small",
    srcs = [
        "arduino_environment_contract.h",
        "arduino_gtest_environment_test.cpp",
        "idf_environment_contract.h",
    ],
    copts = ["-Werror"],
    linkstatic = 1,
    deps = ["//:arduino_gtest_main"],
)

cc_test(
    name = "arduino_preferences_startup_test",
    size = "small",
    srcs = ["arduino_preferences_startup_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/frameworks/arduino-esp32/libraries/Preferences",
    ],
)

cc_test(
    name = "arduino_periman_static_initialization_test",
    size = "small",
    srcs = ["arduino_periman_static_initialization_test.cpp"],
    linkstatic = 1,
    deps = ["//:arduino_gtest_main"],
)

cc_test(
    name = "host_filesystem_test",
    size = "small",
    srcs = ["host_filesystem_test.cpp"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/arduino-esp32/libraries/LittleFS",
        "//roo_testing/frameworks/arduino-esp32/libraries/SPIFFS",
        "//roo_testing/microcontrollers/esp32",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "host_network_test",
    size = "small",
    timeout = "short",
    srcs = ["host_network_test.cpp"],
    linkstatic = 1,
    # Put the host socket overlay before Arduino's aggregate ESP-IDF headers.
    # The test includes lwip/sockets.h directly, so include-path order is part
    # of the bridge contract being exercised here.
    deps = ["//roo_testing/frameworks/posix_lwip"] + [
        "//:arduino_gtest_main",
        "//roo_testing/frameworks/arduino-esp32/libraries/Network",
    ],
)

cc_test(
    name = "legacy_i2c_interface_test",
    size = "small",
    srcs = ["legacy_i2c_interface_test.cpp"],
    deps = ["//roo_testing/buses/i2c"] + select({
        "//roo_testing/platforms:is_idf": ["//:esp_idf_gtest_main"],
        "//conditions:default": ["@googletest//:gtest_main"],
    }),
)

cc_test(
    name = "posix_lwip_bare_socket_link_test",
    srcs = ["posix_lwip_bare_socket_link_test.cpp"],
    deps = [
        "//roo_testing/frameworks/posix_lwip",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "idf_spi_device_test",
    size = "small",
    srcs = ["idf_spi_device_test.cpp"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/esp32_shims:idf",
        "//roo_testing/microcontrollers/esp32:core",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "idf_uart_device_test",
    size = "small",
    srcs = ["idf_uart_device_test.cpp"],
    linkstatic = 1,
    deps = [
        "//roo_testing/buses/uart",
        "//roo_testing/frameworks/esp32_shims:idf",
        "//roo_testing/microcontrollers/esp32:core",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "idf_ledc_test",
    size = "small",
    srcs = ["idf_ledc_test.cpp"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/esp32_shims:idf",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "arduino_ledc_autosync_test",
    size = "small",
    srcs = ["arduino_ledc_test.cpp"],
    linkstatic = 1,
    deps = ["//:arduino_gtest_main"],
)

cc_test(
    name = "arduino_ledc_manual_test",
    size = "small",
    srcs = ["arduino_ledc_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/system:manual_time_mode",
    ],
)

test_suite(
    name = "arduino_ledc_test",
    tests = [
        ":arduino_ledc_autosync_test",
        ":arduino_ledc_manual_test",
    ],
)

cc_test(
    name = "ledc_fade_engine_autosync_test",
    size = "small",
    srcs = ["ledc_fade_engine_test.cpp"],
    copts = ["-fno-exceptions"],
    deps = [
        "//roo_testing/frameworks/esp32_shims:ledc_fade_engine",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "ledc_fade_engine_manual_test",
    size = "small",
    srcs = ["ledc_fade_engine_test.cpp"],
    copts = ["-fno-exceptions"],
    deps = [
        "//roo_testing/frameworks/esp32_shims:ledc_fade_engine",
        "//roo_testing/system:manual_time_mode",
        "@googletest//:gtest_main",
    ],
)

test_suite(
    name = "ledc_fade_engine_test",
    tests = [
        ":ledc_fade_engine_autosync_test",
        ":ledc_fade_engine_manual_test",
    ],
)

cc_test(
    name = "arduino_spi_clock_test",
    size = "small",
    srcs = ["arduino_spi_clock_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/frameworks/arduino-esp32/libraries/SPI",
        "//roo_testing/microcontrollers/esp32:core",
    ],
)

cc_test(
    name = "arduino_sd_diskio_link_test",
    size = "small",
    srcs = ["arduino_sd_diskio_link_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/frameworks/arduino-esp32/libraries/SD",
    ],
)

cc_test(
    name = "arduino_uart_api_test",
    size = "small",
    srcs = ["arduino_uart_api_test.cpp"],
    linkstatic = 1,
    deps = ["//:arduino_gtest_main"],
)

cc_test(
    name = "arduino_wifi_scan_test",
    size = "small",
    timeout = "short",
    srcs = ["arduino_wifi_scan_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/frameworks/arduino-esp32/libraries/WiFi",
        "//roo_testing/microcontrollers/esp32:core",
        "//roo_testing/transducers/wifi",
    ],
)

cc_test(
    name = "fake_uart_test",
    size = "small",
    srcs = ["fake_uart_test.cpp"],
    linkstatic = 1,
    deps = [
        "//roo_testing/buses/uart",
        "//roo_testing/microcontrollers/esp32:core",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "framework_version_test",
    size = "small",
    srcs = ["framework_version_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/frameworks/esp-idf:headers",
    ],
)

cc_test(
    name = "host_event_gateway_test",
    size = "small",
    srcs = ["host_event_gateway_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/host_event",
    ],
)

cc_test(
    name = "voltage_signal_test",
    size = "small",
    srcs = ["voltage_signal_test.cpp"],
    deps = [
        "//roo_testing/transducers/voltage",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "gpio_test",
    size = "small",
    srcs = ["gpio_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/buses/gpio",
        "//roo_testing/transducers/voltage",
    ],
)

cc_test(
    name = "onewire_test",
    size = "small",
    srcs = ["onewire_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/buses/onewire",
        "//roo_testing/devices/onewire/thermometer",
        "//roo_testing/transducers/temperature",
    ],
)

cc_test(
    name = "rtc_ds3231_i2c_test",
    size = "small",
    srcs = ["rtc_ds3231_i2c_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/devices/clock/ds3231",
    ],
)

cc_test(
    name = "simple_test",
    size = "small",
    srcs = ["simple_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/system:manual_time_mode",
        "//roo_testing/system:timer",
    ],
)

cc_test(
    name = "time_mode_auto_static_initialization_test",
    size = "small",
    linkstatic = 1,
    srcs = ["time_mode_auto_static_initialization_test.cpp"],
    deps = [
        "//roo_testing/system:timer",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "time_mode_manual_static_initialization_test",
    size = "small",
    linkstatic = 1,
    srcs = ["time_mode_manual_static_initialization_test.cpp"],
    deps = [
        "//roo_testing/system:manual_time_mode",
        "//roo_testing/system:timer",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "time_publication_test",
    size = "small",
    linkstatic = 1,
    srcs = ["time_publication_test.cpp"],
    deps = [
        "//roo_testing/system:manual_time_mode",
        "//roo_testing/system:timer",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "system_time_alarms_test",
    size = "small",
    linkstatic = 1,
    srcs = ["system_time_alarms_test.cpp"],
    deps = [
        "//roo_testing/system:manual_time_mode",
        "//roo_testing/system:timer",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "system_time_auto_alarms_test",
    size = "small",
    linkstatic = 1,
    srcs = ["system_time_auto_alarms_test.cpp"],
    deps = [
        "//roo_testing/system:timer",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "system_time_alarm_lifecycle_test",
    size = "small",
    linkstatic = 1,
    srcs = ["system_time_alarm_lifecycle_test.cpp"],
    deps = [
        "//roo_testing/system:timer",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "soc_profile_test",
    size = "small",
    srcs = ["soc_profile_test.cpp"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/esp-idf:headers",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "nvs_test",
    size = "small",
    srcs = ["nvs_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/microcontrollers/esp32:nvs_storage",
    ],
)

cc_test(
    name = "freertos_posix_isr_context_test",
    size = "small",
    srcs = [
        "freertos_portyield_compile_test.c",
        "freertos_posix_isr_context_test.cpp",
    ],
    copts = ["-Werror"],
    linkopts = ["-Wl,--wrap=vApplicationTickHook"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/frameworks/esp-idf/components/freertos",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "freertos_posix_simulated_interrupt_install_test",
    size = "small",
    srcs = ["freertos_posix_simulated_interrupt_install_test.cpp"],
    copts = ["-Werror"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/frameworks/esp-idf/components/freertos",
        "//roo_testing/frameworks/esp-idf/components/freertos:roo_testing_port_api",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "freertos_posix_simulated_interrupt_test",
    size = "small",
    srcs = ["freertos_posix_simulated_interrupt_test.cpp"],
    copts = ["-Werror"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/frameworks/esp-idf/components/freertos",
        "//roo_testing/frameworks/esp-idf/components/freertos:roo_testing_port_api",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "freertos_posix_thread_join_regression_test",
    size = "small",
    srcs = ["freertos_posix_thread_join_regression_test.cpp"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/frameworks/esp-idf/components/freertos",
    ],
)

cc_test(
    name = "interrupt_controller_test",
    size = "small",
    srcs = ["interrupt_controller_test.cpp"],
    copts = ["-Werror"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/interrupts:interrupt_controller",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "interrupt_controller_exhaustion_test",
    size = "small",
    srcs = ["interrupt_controller_exhaustion_test.cpp"],
    copts = ["-Werror"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/interrupts:interrupt_controller_exhaustion_test_support",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "esp_intr_alloc_test",
    size = "small",
    srcs = ["esp_intr_alloc_test.cpp"],
    copts = ["-Werror"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/frameworks/esp_idf_support:interrupts",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "esp_intr_alloc_anonymous_test",
    size = "small",
    srcs = ["esp_intr_alloc_anonymous_test.cpp"],
    copts = ["-Werror"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/frameworks/esp_idf_support:interrupts",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "esp_intr_alloc_epoch_exhaustion_test",
    size = "small",
    srcs = ["esp_intr_alloc_epoch_exhaustion_test.cpp"],
    copts = ["-Werror"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/frameworks/esp_idf_support:interrupts_epoch_test_support",
        "@googletest//:gtest",
    ],
)
