load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//roo_testing/platforms:profile_guard.bzl", "profile_guard")

# Keep each supported SoC in its own concrete target. Build-wide identity is
# owned by the matching platform/bazelrc profile; this target supplies the
# typed header and rejects a contradictory target platform.
profile_guard(
    name = "esp32_platform_guard",
    environment = "The roo_testing classic ESP32 SoC profile",
    required_constraints = ["//roo_testing/platforms:esp32"],
)

cc_library(
    name = "esp32",
    srcs = ["esp32_profile_contract.cc"],
    hdrs = ["include/roo_testing/soc_profile.h"],
    copts = ["-Werror"],
    data = [":esp32_platform_guard"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)

# Framework and emulator targets depend on this stable label.  A future SoC
# implementation gets a new concrete target and platform; :target must not be
# changed until its headers, Arduino variant, shims, and behavior agree.
alias(
    name = "target",
    actual = ":esp32",
    visibility = ["//visibility:public"],
)
