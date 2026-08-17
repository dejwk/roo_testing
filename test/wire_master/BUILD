load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "wire_master_test",
    size = "small",
    srcs = ["wire_master_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/frameworks/arduino-esp32/libraries/Wire",
    ],
)
