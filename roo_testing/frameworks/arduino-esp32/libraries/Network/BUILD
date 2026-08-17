load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "Network",
    srcs = glob(["src/**/*.cpp"]),
    hdrs = glob(["src/**/*.h"]),
    # Make the Linux socket overlay part of Network's own include context.
    # Depending on the ordering of sibling dependencies is not sufficient:
    # consumers such as tests may otherwise select ESP-IDF's lwIP socket ABI.
    includes = [
        "src",
        "../../../posix_lwip/include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/posix_lwip",
        "//roo_testing/frameworks/arduino-esp32/cores/esp32",
        "//roo_testing/frameworks/esp-idf:headers",
        "//roo_testing/frameworks/esp32_shims:idf",
    ],
)
