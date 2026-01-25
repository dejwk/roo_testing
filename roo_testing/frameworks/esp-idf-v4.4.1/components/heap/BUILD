load("@rules_cc//cc:cc_library.bzl", "cc_library")

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
        "include/heap_memory_layout.h",
        "include/multi_heap.h",
        "include/soc/soc_memory_layout.h",
    ],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
    ],
)
