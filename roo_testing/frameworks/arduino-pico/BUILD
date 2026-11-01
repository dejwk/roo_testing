load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "headers",
    hdrs = glob([
        "cores/rp2040/**/*.h",
        "ArduinoCore-API/api/**/*.h",
        "variants/rpipico/*.h",
        "variants/generic/*.h",
        "libraries/SPI/src/*.h",
        "libraries/Wire/src/*.h",
    ]),
    includes = [
        ".",
        "cores/rp2040",
        "libraries/SPI/src",
        "libraries/Wire/src",
        "variants/rpipico",
    ],
    textual_hdrs = [
        "cores/rp2040/CoreMutex.cpp",
        "ArduinoCore-API/api/deprecated-avr-comp/avr/dtostrf.c.impl",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/environment:arduino_pico",
        "//roo_testing/frameworks/pico-sdk:headers",
    ],
)

cc_library(
    name = "portable",
    srcs = [
        "ArduinoCore-API/api/Common.cpp",
        "ArduinoCore-API/api/Print.cpp",
        "ArduinoCore-API/api/Stream.cpp",
        "ArduinoCore-API/api/String.cpp",
        "cores/rp2040/CoreMutex.cpp",
        "cores/rp2040/WMath.cpp",
        "cores/rp2040/stdlib_noniso.cpp",
        "cores/rp2040/wiring_analog.cpp",
    ],
    copts = [
        "-fno-exceptions",
        "-fno-rtti",
    ],
    linkstatic = True,
    visibility = ["//roo_testing/frameworks/arduino_pico_support:__pkg__"],
    deps = [
        ":headers",
        "//:pico_sdk",
        "//roo_testing/frameworks/arduino_pico_support:libc",
    ],
)
