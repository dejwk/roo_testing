cc_library(
    name = "testing",
    visibility = ["//visibility:public"],
    deps = [
        ":arduino",
    ],
)

cc_library(
    name = "arduino",
    visibility = ["//visibility:public"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/arduino-esp32-2.0.4/cores/esp32",
    ],
)

cc_library(
    name = "arduino_main",
    visibility = ["//visibility:public"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/arduino-esp32-2.0.4/cores/esp32:main",
    ],
)
