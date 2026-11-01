load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "rtc_test",
    size = "small",
    srcs = ["rtc_test.cpp"],
    linkstatic = True,
    deps = [
        "//roo_testing/devices/clock/ds1307",
        "//roo_testing/devices/clock/ds3231",
        "//roo_testing/devices/clock/ds3232",
        "//roo_testing/devices/clock/ds3234",
        "//roo_testing/devices/clock/pcf8523",
        "//roo_testing/devices/clock/rv3028",
        "//roo_testing/devices/clock/rv8803",
        "@googletest//:gtest_main",
    ],
)
