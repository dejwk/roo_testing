load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_binary(
    name = "auto_time",
    srcs = ["clock.cpp"],
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = ["//:pico_sdk"],
)

cc_binary(
    name = "manual_time",
    srcs = ["clock.cpp"],
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [
        "//:pico_sdk",
        "//roo_testing/system:manual_time_mode",
    ],
)
