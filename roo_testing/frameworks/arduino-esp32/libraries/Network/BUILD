load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "Network",
    srcs = glob(["src/**/*.cpp"]),
    hdrs = glob(["src/**/*.h"]),
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/posix_lwip",
        "//roo_testing/frameworks/arduino-esp32/cores/esp32",
        "//roo_testing/frameworks/esp-idf:headers",
        "//roo_testing/frameworks/esp32_shims:idf",
    ],
)
