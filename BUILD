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
        "//roo_testing/frameworks/arduinoespressif32:arduino",
    ],
)

cc_library(
    name = "arduino_main",
    visibility = ["//visibility:public"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/arduinoespressif32:arduino_main",
    ],
)
