load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "esp32",
    hdrs = ["esp32/pins_arduino.h"],
    includes = ["esp32"],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/frameworks/environment:arduino"],
)

cc_library(
    name = "esp32s3",
    hdrs = ["esp32s3/pins_arduino.h"],
    includes = ["esp32s3"],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/frameworks/environment:arduino"],
)

cc_library(
    name = "esp32c3",
    hdrs = ["esp32c3/pins_arduino.h"],
    includes = ["esp32c3"],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/frameworks/environment:arduino"],
)

alias(
    name = "target",
    actual = select({
        "//roo_testing/platforms:is_esp32s3": ":esp32s3",
        "//roo_testing/platforms:is_esp32c3": ":esp32c3",
        "//conditions:default": ":esp32",
    }),
    visibility = ["//visibility:public"],
)
