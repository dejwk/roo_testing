load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:private"])

# Wrong profiles skip these tests in wildcard builds; public :pico_sdk itself
# has a hard guard and fails when explicitly requested with no SDK capability.
cc_library(
    name = "unrelated_c",
    srcs = [
        "profile_contract.h",
        "unrelated_c.c",
    ],
    copts = ["-Werror"],
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
)

cc_test(
    name = "sdk_probe_c",
    size = "small",
    srcs = [
        "profile_contract.h",
        "sdk_probe.c",
    ],
    copts = ["-Werror"],
    linkstatic = True,
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = ["//:pico_sdk"],
)

cc_test(
    name = "sdk_probe_cpp",
    size = "small",
    srcs = [
        "profile_contract.h",
        "sdk_probe.cc",
    ],
    copts = ["-Werror"],
    linkstatic = True,
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [
        ":unrelated_c",
        "//:pico_sdk",
    ],
)

cc_binary(
    name = "unsupported_alarm_pool_probe",
    srcs = ["unsupported_alarm_pool_probe.c"],
    tags = ["manual"],
    deps = ["//:pico_sdk"],
)

exports_files(["verify_pico.py"])

cc_test(
    name = "time_manual_test",
    size = "small",
    srcs = [
        "time_manual_test.cpp",
        "time_test.cpp",
    ],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    linkstatic = True,
    local_defines = ["PICO_TEST_AUTO=0"],
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [
        "//:pico_gtest_main",
        "//roo_testing/system:manual_time_mode",
    ],
)

cc_test(
    name = "time_auto_test",
    size = "small",
    srcs = [
        "time_auto_test.cpp",
        "time_test.cpp",
    ],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    linkstatic = True,
    local_defines = ["PICO_TEST_AUTO=1"],
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = ["//:pico_gtest_main"],
)

cc_test(
    name = "runner_manual_test",
    size = "small",
    srcs = ["runner_test.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    linkstatic = True,
    local_defines = ["PICO_TEST_AUTO=0"],
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [
        "//:pico_gtest_main",
        "//roo_testing/system:manual_time_mode",
    ],
)

cc_test(
    name = "runner_auto_test",
    size = "small",
    srcs = ["runner_test.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    linkstatic = True,
    local_defines = ["PICO_TEST_AUTO=1"],
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = ["//:pico_gtest_main"],
)

cc_test(
    name = "gpio_test",
    size = "small",
    srcs = ["gpio_test.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    linkstatic = True,
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [
        "//:pico_gtest_main",
        "//roo_testing/microcontrollers/rp2040:pico",
        "//roo_testing/system:manual_time_mode",
    ],
)

cc_test(
    name = "uart_test",
    size = "small",
    srcs = ["uart_test.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    linkstatic = True,
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [
        "//:pico_gtest_main",
        "//roo_testing/buses/uart",
        "//roo_testing/microcontrollers/rp2040:pico",
        "//roo_testing/system:manual_time_mode",
        "//roo_testing/system:timer",
        "//roo_testing/transducers/voltage",
    ],
)

cc_binary(
    name = "console_probe",
    srcs = ["console_probe.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    linkstatic = True,
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = ["//:pico_sdk"],
)

cc_test(
    name = "uart_wiring_test",
    size = "small",
    srcs = ["uart_wiring_test.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    linkstatic = True,
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [
        "//:pico_gtest_main",
        "//roo_testing/buses/uart",
        "//roo_testing/microcontrollers/rp2040:pico",
        "//roo_testing/system:manual_time_mode",
        "//roo_testing/system:timer",
        "//roo_testing/transducers/voltage",
    ],
)

cc_test(
    name = "spi_test",
    size = "small",
    srcs = ["spi_test.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    linkstatic = True,
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [
        "//:pico_gtest_main",
        "//roo_testing/buses/spi",
        "//roo_testing/microcontrollers/rp2040:pico",
        "//roo_testing/system:manual_time_mode",
    ],
)

cc_test(
    name = "spi_wiring_test",
    size = "small",
    srcs = ["spi_wiring_test.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    linkstatic = True,
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [
        "//:pico_gtest_main",
        "//roo_testing/buses/spi",
        "//roo_testing/microcontrollers/rp2040:pico",
        "//roo_testing/system:manual_time_mode",
    ],
)

cc_test(
    name = "spi_manual_test",
    size = "small",
    srcs = ["spi_clock_test.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    linkstatic = True,
    local_defines = ["PICO_SPI_AUTO=0"],
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [
        "//:pico_gtest_main",
        "//roo_testing/buses/spi",
        "//roo_testing/microcontrollers/rp2040:pico",
        "//roo_testing/system:manual_time_mode",
        "//roo_testing/system:timer",
    ],
)

cc_test(
    name = "spi_auto_test",
    size = "small",
    srcs = ["spi_clock_test.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    linkstatic = True,
    local_defines = ["PICO_SPI_AUTO=1"],
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [
        "//:pico_gtest_main",
        "//roo_testing/buses/spi",
        "//roo_testing/microcontrollers/rp2040:pico",
        "//roo_testing/system:timer",
    ],
)
