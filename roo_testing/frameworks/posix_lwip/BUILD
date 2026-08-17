load("@rules_cc//cc:cc_library.bzl", "cc_library")

# Arduino-ESP32 uses lwIP's socket-prefixed entry points even though the host
# emulator deliberately sends network traffic through the Linux network stack.
# Keep this include overlay destination-owned so refreshing ESP-IDF does not
# replace the Linux bridge.
cc_library(
    name = "posix_lwip",
    # GNU ld's --wrap options create references to __wrap_* only after Bazel
    # has selected static-library members. Keep the tiny bridge object in
    # every final link, including dependents that use only bare BSD names.
    alwayslink = True,
    srcs = [
        "dns.cpp",
        "sockets.cpp",
    ],
    hdrs = glob(["include/**/*.h"]),
    includes = ["include"],
    linkopts = [
        "-Wl,--wrap=accept",
        "-Wl,--wrap=bind",
        "-Wl,--wrap=connect",
        "-Wl,--wrap=getpeername",
        "-Wl,--wrap=getsockname",
        "-Wl,--wrap=getsockopt",
        "-Wl,--wrap=listen",
        "-Wl,--wrap=poll",
        "-Wl,--wrap=recv",
        "-Wl,--wrap=recvfrom",
        "-Wl,--wrap=recvmsg",
        "-Wl,--wrap=select",
        "-Wl,--wrap=send",
        "-Wl,--wrap=sendmsg",
        "-Wl,--wrap=sendto",
        "-Wl,--wrap=setsockopt",
        "-Wl,--wrap=shutdown",
        "-Wl,--wrap=socket",
    ],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/frameworks/esp-idf:headers"],
)
