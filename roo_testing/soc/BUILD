load("@rules_cc//cc:cc_library.bzl", "cc_library")

# This is the one source of compile-time SoC identity for host builds.  Keep
# each supported SoC in its own concrete target, then point :target at the
# profile whose behavior roo_testing actually implements.
cc_library(
    name = "esp32",
    hdrs = ["include/roo_testing/soc_profile.h"],
    defines = [
        "CONFIG_IDF_FIRMWARE_CHIP_ID=0x0000",
        "CONFIG_IDF_TARGET=\\\"esp32\\\"",
        "CONFIG_IDF_TARGET_ARCH=\\\"xtensa\\\"",
        "CONFIG_IDF_TARGET_ARCH_XTENSA=1",
        "CONFIG_IDF_TARGET_ESP32=1",
        "ESP32=1",
        "ROO_TESTING_SOC=\\\"esp32\\\"",
        "ROO_TESTING_SOC_ESP32=1",
    ],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)

# Framework and emulator targets depend on this stable label.  A future SoC
# implementation gets a new concrete profile; :target must not be changed
# until its headers, Arduino variant, shims, and FakeEsp32 behavior agree.
alias(
    name = "target",
    actual = ":esp32",
    visibility = ["//visibility:public"],
)
