load("@rules_cc//cc:cc_library.bzl", "cc_library")

# Public headers from ESP-IDF 5.5.4's single-core kernel and Linux port.  The
# dependency-free support target supplies sdkconfig.h and the handful of common
# IDF headers included by portmacro_idf.h.
cc_library(
    name = "include",
    hdrs = glob([
        "FreeRTOS-Kernel/include/**/*.h",
        "FreeRTOS-Kernel/portable/linux/include/**/*.h",
        "config/include/**/*.h",
        "config/linux/include/**/*.h",
        "esp_additions/include/**/*.h",
    ]),
    includes = [
        "FreeRTOS-Kernel/include",
        "FreeRTOS-Kernel/include/freertos",
        "FreeRTOS-Kernel/portable/linux/include",
        "FreeRTOS-Kernel/portable/linux/include/freertos",
        "config/include",
        "config/include/freertos",
        "config/linux/include",
        "esp_additions/include",
    ],
    defines = ["ESP_PLATFORM"],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/frameworks/esp-idf:freertos_support_headers"],
)

# Keep the pthread-backed port separate so its cancellation/unwind flags match
# the long-standing roo_testing port.  ESP-IDF 5.5.4's upstream port.c includes
# the important pthread_cancel()+pthread_join() task cleanup fix.
cc_library(
    name = "freertos_posix_port",
    srcs = [
        "FreeRTOS-Kernel/portable/linux/port.c",
        "FreeRTOS-Kernel/portable/linux/utils/wait_for_event.c",
        "FreeRTOS-Kernel/portable/linux/utils/wait_for_event.h",
        "roo_testing_port.cc",
    ],
    copts = [
        "-fno-asynchronous-unwind-tables",
        "-fno-unwind-tables",
        "-pthread",
    ],
    defines = ["projCOVERAGE_TEST=0"],
    includes = [
        "FreeRTOS-Kernel/portable/linux",
        "FreeRTOS-Kernel/portable/linux/utils",
    ],
    linkopts = ["-pthread"],
    local_defines = [
        "CONFIG_IDF_TARGET_LINUX=1",
        "ROO_TESTING_FREERTOS_POSIX_BACKEND=1",
    ],
    visibility = ["//visibility:private"],
    deps = [
        ":include",
        "//roo_testing/host_event",
    ],
)

cc_library(
    name = "freertos",
    srcs = [
        "FreeRTOS-Kernel/event_groups.c",
        "FreeRTOS-Kernel/list.c",
        "FreeRTOS-Kernel/queue.c",
        "FreeRTOS-Kernel/stream_buffer.c",
        "FreeRTOS-Kernel/tasks.c",
        "FreeRTOS-Kernel/timers.c",
        "esp_additions/freertos_tasks_c_additions.h",
    ],
    includes = [
        "FreeRTOS-Kernel/include/freertos",
        "esp_additions",
    ],
    linkopts = ["-pthread"],
    local_defines = [
        "CONFIG_IDF_TARGET_LINUX=1",
        "ROO_TESTING_FREERTOS_POSIX_BACKEND=1",
    ],
    alwayslink = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":freertos_posix_port",
        ":include",
    ],
)
