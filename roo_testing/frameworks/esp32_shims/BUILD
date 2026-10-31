load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

# Architecture-independent sources kept current by the ESP-IDF import.  They
# live in the existing destination-owned shim package so adding host build
# metadata does not turn imported component directories into Bazel packages.
cc_library(
    name = "esp_err_to_name",
    srcs = [
        "//roo_testing/frameworks/esp-idf:components/esp_common/src/esp_err_to_name.c",
    ],
    local_defines = ["CONFIG_ESP_ERR_TO_NAME_LOOKUP=1"],
    deps = ["//roo_testing/frameworks/esp-idf:headers"],
)

cc_library(
    name = "esp_ringbuf",
    srcs = [
        "//roo_testing/frameworks/esp-idf:components/esp_ringbuf/ringbuf.c",
    ],
    deps = [
        "//roo_testing/frameworks/esp-idf:headers",
        "//roo_testing/frameworks/esp-idf/components/freertos",
    ],
)

# Internal host model of the ESP timer compare and IRQ path.
cc_library(
    name = "esp_timer_impl_host",
    srcs = [
        "esp_timer_impl_host.cpp",
        "//roo_testing/frameworks/esp-idf:components/esp_timer/src/esp_timer_impl_common.c",
    ],
    copts = [
        "-Werror",
        "-Wno-error=format",
    ],
    local_defines = [
        "CONFIG_ESP_TIMER_ISR_AFFINITY_CPU0=1",
        "CONFIG_LOG_VERSION=1",
    ],
    visibility = ["//roo_testing/frameworks/esp32_shims:__pkg__"],
    deps = [
        "//roo_testing/frameworks/esp-idf:headers",
        "//roo_testing/frameworks/esp-idf/components/freertos",
        "//roo_testing/frameworks/esp_idf_support:interrupts",
        "//roo_testing/system:timer",
        "@glog",
    ],
)

cc_library(
    name = "idf",
    srcs = [
        "esp_now.cpp",
        "esp_timer_host_etm.cpp",
        "esp_timer_host_lifecycle.cpp",
        "idf_core.cpp",
        "idf_drivers.cpp",
        "idf_i2c.cpp",
        "idf_ledc.cpp",
        "idf_sd.cpp",
        "nvs.cpp",
        "ota_partition.cpp",
        "sntp_host.cpp",
        "sntp_netif.cpp",
        "wifi.cpp",
        "//roo_testing/frameworks/esp-idf:components/esp_timer/src/esp_timer.c",
        "//roo_testing/frameworks/esp-idf:components/esp_timer/src/esp_timer_init.c",
        "//roo_testing/frameworks/esp-idf:components/esp_timer/src/ets_timer_legacy.c",
    ],
    hdrs = [
        "esp_timer_host.h",
        "shim_internal.h",
        "sntp_host.h",
        "sntp_netif.h",
        "wifi_host.h",
    ],
    copts = [
        "-Werror",
        "-Wno-error=format",
    ],
    linkstatic = 1,
    local_defines = [
        "CONFIG_ESP_TIMER_ISR_AFFINITY_CPU0=1",
        "CONFIG_LOG_VERSION=1",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":esp_err_to_name",
        ":esp_ringbuf",
        ":esp_timer_impl_host",
        ":ledc_fade_engine",
        "//roo_testing/frameworks/esp-idf:esp_event",
        "//roo_testing/frameworks/esp-idf:headers",
        "//roo_testing/frameworks/esp-idf/components/freertos",
        "//roo_testing/frameworks/esp_idf_support:interrupts",
        "//roo_testing/frameworks/posix_time",
        "//roo_testing/frameworks/posix_time:host_time",
        "//roo_testing/microcontrollers/esp32:core",
        "//roo_testing/sys",
        "//roo_testing/system:timer",
        "//roo_testing/system:wall_clock",
        "@glog",
    ],
)

cc_library(
    name = "arduino",
    srcs = [
        "arduino_adc.cpp",
        "arduino_esp_class.cpp",
        "arduino_gpio.cpp",
        "arduino_i2c.cpp",
        "arduino_ledc.cpp",
        "arduino_misc.cpp",
        "arduino_periman.cpp",
        "arduino_spi.cpp",
        "arduino_time.cpp",
        "arduino_timer.cpp",
        "arduino_uart.cpp",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":arduino_logging",
        ":idf",
        ":ledc_fade_engine",
        "//roo_testing/frameworks/arduino-esp32/cores/esp32:headers",
        "//roo_testing/frameworks/arduino-esp32/cores/esp32:string",
        "//roo_testing/frameworks/esp-idf:headers",
        "//roo_testing/frameworks/esp-idf/components/freertos",
        "//roo_testing/microcontrollers/esp32:core",
        "//roo_testing/system:timer",
        "@glog",
    ],
)

cc_library(
    name = "ledc_fade_engine",
    srcs = ["ledc_fade_engine.cpp"],
    hdrs = ["ledc_fade_engine.h"],
    copts = ["-fno-exceptions"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/host:synchronization",
        "//roo_testing/microcontrollers/esp32:core",
        "//roo_testing/system:timer",
        "//roo_testing/transducers/voltage",
    ],
)

cc_library(
    name = "esp32_shims",
    visibility = ["//visibility:public"],
    deps = [
        ":arduino",
        ":idf",
    ],
)

cc_test(
    name = "event_test",
    size = "small",
    srcs = ["event_test.cpp"],
    linkstatic = 1,
    deps = [
        ":idf",
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "ets_timer_legacy_test",
    size = "small",
    srcs = ["ets_timer_legacy_test.cpp"],
    linkstatic = 1,
    deps = [
        ":idf",
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "ets_timer_legacy_manual_test",
    size = "small",
    srcs = ["ets_timer_legacy_test.cpp"],
    linkstatic = 1,
    deps = [
        ":idf",
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/system:manual_time_mode",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "esp_timer_host_test",
    size = "small",
    srcs = ["esp_timer_host_test.cpp"],
    linkstatic = 1,
    deps = [
        ":idf",
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "esp_timer_host_manual_test",
    size = "small",
    srcs = ["esp_timer_host_test.cpp"],
    linkstatic = 1,
    deps = [
        ":idf",
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/system:manual_time_mode",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "esp_timer_impl_host_auto_test",
    size = "small",
    srcs = ["esp_timer_impl_host_auto_test.cpp"],
    linkstatic = 1,
    deps = [
        ":esp_timer_impl_host",
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "esp_timer_impl_host_test",
    size = "small",
    srcs = ["esp_timer_impl_host_test.cpp"],
    linkstatic = 1,
    deps = [
        ":esp_timer_impl_host",
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/system:manual_time_mode",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "idf_sd_compat_test",
    size = "small",
    srcs = ["idf_sd_compat_test.cpp"],
    linkstatic = 1,
    deps = [
        ":idf",
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "runtime_compat_test",
    size = "small",
    srcs = ["runtime_compat_test.cpp"],
    linkstatic = 1,
    deps = [
        ":idf",
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "wifi_compat_test",
    size = "small",
    srcs = ["wifi_compat_test.cpp"],
    linkstatic = 1,
    deps = [
        ":idf",
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/microcontrollers/esp32:core",
        "//roo_testing/transducers/wifi",
        "@googletest//:gtest",
    ],
)

# Logging is shared by portable String code and the Arduino HAL shims.
cc_library(
    name = "arduino_logging",
    srcs = ["arduino_log.cpp"],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/frameworks/arduino-esp32/cores/esp32:headers"],
)

[
    cc_test(
        name = name,
        size = "small",
        srcs = [source],
        deps = [
            ":idf",
            "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
            "@googletest//:gtest",
        ] + mode,
    )
    for name, source, mode in [
        (
            "sntp_host_test",
            "sntp_host_test.cpp",
            ["//roo_testing/system:manual_time_mode"],
        ),
        ("sntp_auto_test", "sntp_host_test.cpp", []),
        (
            "sntp_override_test",
            "sntp_override_test.cpp",
            ["//roo_testing/system:manual_time_mode"],
        ),
    ]
]

cc_test(
    name = "arduino_sntp_test",
    size = "small",
    srcs = ["arduino_sntp_test.cpp"],
    target_compatible_with = ["//roo_testing/platforms:arduino"],
    deps = [
        ":arduino",
        "//:arduino_gtest_main",
        "//roo_testing/system:manual_time_mode",
    ],
)

[
    cc_test(
        name = name,
        size = "small",
        srcs = ["netif_sntp_test.cpp"],
        deps = [
            ":idf",
            "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
            "@googletest//:gtest",
        ] + mode,
    )
    for name, mode in [
        (
            "netif_sntp_test",
            ["//roo_testing/system:manual_time_mode"],
        ),
        ("netif_sntp_auto_test", []),
    ]
]

cc_test(
    name = "sntp_raw_test",
    size = "small",
    srcs = [
        "sntp_raw_probe.c",
        "sntp_raw_test.cpp",
    ],
    deps = [
        ":idf",
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/system:manual_time_mode",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "sntp_unsupported_test",
    size = "small",
    srcs = ["sntp_unsupported_test.cpp"],
    deps = [
        ":idf",
        "@googletest//:gtest_main",
    ],
)

test_suite(
    name = "sntp_tests",
    tests = [
        ":netif_sntp_auto_test",
        ":netif_sntp_test",
        ":sntp_auto_test",
        ":sntp_host_test",
        ":sntp_override_test",
        ":sntp_raw_test",
        ":sntp_unsupported_test",
    ],
)
