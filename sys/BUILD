cc_library(
    name = "sys",
    srcs = [
        "thread.h",
        "thread.cpp",
        "mutex.h",
        "mutex.cpp",
        "condition_variable.h",
        "condition_variable.cpp",
    ],
    hdrs = glob(["*.h"]),
    includes = [
        ".",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_timer",
        "@glog//:glog",
    ],
)

cc_library(
    name = "include",
    srcs = [
        "thread.h",
        "mutex.h",
        "condition_variable.h",
        "internal/callable.h",
    ],
    hdrs = glob(["*.h"]),
    includes = [
        ".",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
    ],
)
