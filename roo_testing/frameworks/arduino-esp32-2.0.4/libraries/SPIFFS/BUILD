cc_library(
    name = "SPIFFS",
    srcs = glob(["src/**/*.cpp"]),
    hdrs = glob(["src/**/*.h"]),
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32-2.0.4/cores/esp32",
        "//roo_testing/frameworks/arduino-esp32-2.0.4/libraries/FS",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/spiffs",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/vfs",
    ],
)
