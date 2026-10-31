load("@roo_testing//roo_testing/emulation:esp_idf.bzl", "roo_esp_idf_example")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

roo_esp_idf_example(
    name = "SntpClock",
    srcs = ["main.cpp"],
    deps = ["//roo_testing/frameworks/esp32_shims:idf"],
)

cc_test(
    name = "application_test",
    size = "small",
    srcs = [
        "main.cpp",
        "test.cpp",
    ],
    target_compatible_with = ["//roo_testing/platforms:idf"],
    deps = [
        "//:esp_idf_gtest_main",
        "//roo_testing/frameworks/esp32_shims:idf",
    ],
)
