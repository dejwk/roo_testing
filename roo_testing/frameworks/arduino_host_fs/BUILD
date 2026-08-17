load("@rules_cc//cc:cc_library.bzl", "cc_library")

# Host implementations of Arduino's filesystem adapters.  These live outside
# the imported framework tree so a Copybara refresh cannot overwrite the
# emulator-specific mounting and path translation behavior.
cc_library(
    name = "FS",
    srcs = ["vfs_api.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32/libraries/FS:upstream",
        "//roo_testing/microcontrollers/esp32",
    ],
)

cc_library(
    name = "SPIFFS",
    srcs = ["spiffs.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        ":FS",
        "//roo_testing/frameworks/arduino-esp32/libraries/SPIFFS:headers",
    ],
)

cc_library(
    name = "LittleFS",
    srcs = ["littlefs.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        ":FS",
        "//roo_testing/frameworks/arduino-esp32/libraries/LittleFS:headers",
    ],
)

cc_library(
    name = "SD",
    srcs = [
        "sd.cpp",
        "sd_diskio.cpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":FS",
        "//roo_testing/frameworks/arduino-esp32/libraries/SD:headers",
    ],
)

cc_library(
    name = "SD_MMC",
    srcs = ["sd_mmc.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        ":FS",
        "//roo_testing/frameworks/arduino-esp32/libraries/SD_MMC:headers",
    ],
)
