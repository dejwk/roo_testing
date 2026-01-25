cc_library(
    name = "sys",
    srcs = glob(["**/*.cpp"]),
    hdrs = glob(["**/*.h"]),
    includes = [
        ".",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/system:timer",
        "@glog",
    ],
)
