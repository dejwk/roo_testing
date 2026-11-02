load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "kernel_test",
    size = "small",
    timeout = "short",
    srcs = ["kernel_test.cpp"],
    linkstatic = True,
    deps = ["//roo_testing/frameworks/arduino-pico/FreeRTOS-Kernel:freertos"],
)

[
    cc_test(
        name = "clock_" + mode + "_test",
        size = "small",
        timeout = "short",
        srcs = ["clock_test.cpp"],
        linkstatic = True,
        deps = [
            "//roo_testing/frameworks/arduino-pico/FreeRTOS-Kernel:freertos",
            "//roo_testing/system:timer",
        ] + (["//roo_testing/system:manual_time_mode"] if mode == "manual" else []),
    )
    for mode in [
        "auto",
        "manual",
    ]
]

[
    cc_test(
        name = "peripheral_" + mode + "_test",
        size = "small",
        timeout = "short",
        srcs = ["peripheral_test.cpp"],
        linkstatic = True,
        deps = [
            "//:pico_sdk",
            "//roo_testing/buses/spi",
            "//roo_testing/buses/uart",
            "//roo_testing/frameworks/arduino-pico/FreeRTOS-Kernel:freertos",
            "//roo_testing/microcontrollers/rp2040:pico",
        ] + (["//roo_testing/system:manual_time_mode"] if mode == "manual" else []),
    )
    for mode in [
        "auto",
        "manual",
    ]
]

[
    cc_test(
        name = "arduino_" + mode + "_test",
        size = "small",
        timeout = "short",
        srcs = ["arduino_test.cpp"],
        copts = [
            "-Werror",
            "-fno-exceptions",
            "-fno-rtti",
        ],
        linkstatic = True,
        target_compatible_with = [
            "//roo_testing/platforms:rp2040",
            "//roo_testing/platforms:freertos",
        ],
        deps = [
            "//:arduino_gtest_main",
            "//roo_testing/buses/i2c",
            "//roo_testing/buses/spi",
            "//roo_testing/microcontrollers/rp2040:pico",
        ] + (["//roo_testing/system:manual_time_mode"] if mode == "manual" else []),
    )
    for mode in [
        "auto",
        "manual",
    ]
]

cc_binary(
    name = "core1_probe",
    srcs = ["core1_probe.cpp"],
    target_compatible_with = [
        "//roo_testing/platforms:rp2040",
        "//roo_testing/platforms:freertos",
    ],
    deps = ["//:arduino_main"],
)

cc_test(
    name = "runner_process_test",
    size = "small",
    timeout = "short",
    srcs = ["runner_process_test.cpp"],
    args = [
        "$(location :arduino_auto_test)",
        "$(location :arduino_manual_test)",
        "$(location :core1_probe)",
        "$(location //test/pico:freertos_runner_manual_test)",
    ],
    data = [
        ":arduino_auto_test",
        ":arduino_manual_test",
        ":core1_probe",
        "//test/pico:freertos_runner_manual_test",
    ],
    target_compatible_with = [
        "//roo_testing/platforms:rp2040",
        "//roo_testing/platforms:freertos",
    ],
)

cc_test(
    name = "sys_test",
    size = "small",
    timeout = "short",
    srcs = ["sys_test.cpp"],
    linkstatic = True,
    target_compatible_with = [
        "//roo_testing/platforms:rp2040",
        "//roo_testing/platforms:freertos",
    ],
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/sys",
        "//roo_testing/system:manual_time_mode",
    ],
)

cc_library(
    name = "unrelated_c_identity",
    srcs = ["identity.c"],
    copts = ["-Werror"],
    target_compatible_with = [
        "//roo_testing/platforms:rp2040",
        "//roo_testing/platforms:freertos",
    ],
)

cc_test(
    name = "identity_test",
    size = "small",
    srcs = ["identity_test.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
        "-fno-rtti",
    ],
    linkstatic = True,
    deps = [
        ":unrelated_c_identity",
        "//roo_testing/runtime:freertos_headers",
    ],
)

cc_test(
    name = "event_test",
    size = "small",
    timeout = "short",
    srcs = ["event_test.cpp"],
    target_compatible_with = [
        "//roo_testing/platforms:rp2040",
        "//roo_testing/platforms:freertos",
    ],
    deps = ["//roo_testing/frameworks/arduino-pico/FreeRTOS-Kernel:native_event_test_support"],
)
