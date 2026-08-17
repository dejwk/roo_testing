load("@rules_cc//cc:cc_library.bzl", "cc_library")

# Older dependent modules treated Arduino's SD package as a monolithic header
# surface and compiled SD_MMC sources behind target macros. Keep that legacy
# include graph without broadening the modern SD target.
cc_library(
    name = "SD",
    defines = ["ROO_TESTING_LEGACY_SD=1"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32/libraries/SD",
        "//roo_testing/frameworks/arduino-esp32/libraries/SD_MMC:headers",
        "//roo_testing/microcontrollers/esp32:core",
    ],
)
