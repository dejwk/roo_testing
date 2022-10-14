cc_library(
    name = "include",
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
    ],
)

cc_library(
    name = "heap",
    srcs = [
        "heap_caps.c",
    ],
    hdrs = [
        "include/esp_heap_caps.h",
        "include/multi_heap.h",
    ],
    visibility = ["//visibility:public"],
    includes = [
        "include",
    ],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
    ]
)
