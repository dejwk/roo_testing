load("@rules_cc//cc:cc_library.bzl", "cc_library")

# Arduino-ESP32 uses lwIP's socket-prefixed entry points even though the host
# emulator deliberately sends network traffic through the Linux network stack.
# Keep this include overlay destination-owned so refreshing ESP-IDF does not
# replace the Linux bridge.
cc_library(
    name = "posix_lwip",
    srcs = ["dns.cpp"],
    hdrs = glob(["include/**/*.h"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
)
