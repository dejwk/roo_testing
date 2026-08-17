load("@rules_cc//cc:cc_test.bzl", "cc_test")

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
    name = "arduino_uart_api_test",
    size = "small",
    srcs = ["arduino_uart_api_test.cpp"],
    linkstatic = 1,
    deps = ["//:arduino_gtest_main"],
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
        "//roo_testing/system:timer",
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
    name = "freertos_posix_thread_join_regression_test",
    size = "small",
    srcs = ["freertos_posix_thread_join_regression_test.cpp"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/frameworks/esp-idf/components/freertos",
    ],
)
