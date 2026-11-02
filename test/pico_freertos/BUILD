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
