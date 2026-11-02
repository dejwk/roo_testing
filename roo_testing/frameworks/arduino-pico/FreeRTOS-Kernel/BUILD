load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "headers",
    hdrs = glob([
        "include/**/*.h",
        "portable/ThirdParty/GCC/Posix/*.h",
    ]),
    includes = [
        "include",
        "portable/ThirdParty/GCC/Posix",
    ],
    target_compatible_with = [
        "//roo_testing/platforms:freertos",
        "//roo_testing/platforms:rp2040",
    ],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/frameworks/freertos_posix:config"],
)

cc_library(
    name = "freertos",
    srcs = [
        "event_groups.c",
        "list.c",
        "portable/MemMang/heap_3.c",
        "portable/ThirdParty/GCC/Posix/port.c",
        "portable/ThirdParty/GCC/Posix/utils/wait_for_event.c",
        "portable/ThirdParty/GCC/Posix/utils/wait_for_event.h",
        "queue.c",
        "stream_buffer.c",
        "tasks.c",
        "timers.c",
    ],
    copts = [
        "-pthread",
        "-fno-asynchronous-unwind-tables",
        "-fno-unwind-tables",
    ],
    defines = ["projCOVERAGE_TEST=0"],
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
    deps = [
        ":headers",
        "//roo_testing/frameworks/freertos_posix:hooks",
    ],
    alwayslink = True,
)

cc_library(
    name = "native_event_test_support",
    testonly = True,
    srcs = ["portable/ThirdParty/GCC/Posix/utils/wait_for_event.c"],
    hdrs = ["portable/ThirdParty/GCC/Posix/utils/wait_for_event.h"],
    includes = ["portable/ThirdParty/GCC/Posix/utils"],
    visibility = ["//test/pico_freertos:__pkg__"],
)
