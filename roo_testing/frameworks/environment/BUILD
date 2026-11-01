load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//roo_testing/platforms:profile_guard.bzl", "profile_guard")

# These targets validate and constrain the globally selected build profile.
# They intentionally do not export defines: build identity must be identical in
# every target-configuration compile action, including dependency sublibraries.
profile_guard(
    name = "emulator_platform_guard",
    environment = "The roo_testing emulator",
    required_constraints = ["//roo_testing/platforms:roo_testing"],
)

cc_library(
    name = "emulator",
    srcs = ["emulator_profile_contract.c"],
    copts = ["-Werror"],
    data = [":emulator_platform_guard"],
    visibility = ["//visibility:public"],
    deps = [":runtime"],
)

profile_guard(
    name = "esp_idf_platform_guard",
    environment = "The roo_testing ESP-IDF environment",
    required_constraints = [
        "//roo_testing/platforms:esp_idf",
        "//roo_testing/platforms:roo_testing",
    ],
)

cc_library(
    name = "esp_idf",
    srcs = ["esp_idf_profile_contract.cc"],
    copts = ["-Werror"],
    data = [":esp_idf_platform_guard"],
    visibility = ["//visibility:public"],
    deps = [
        ":emulator",
        ":freertos",
        "//roo_testing/soc:target",
    ],
)

# IDF-only entry points depend on this target so they cannot silently build
# under the Arduino frontend, even though both frontends expose ESP-IDF APIs.
profile_guard(
    name = "idf_platform_guard",
    environment = "The roo_testing IDF-only environment",
    required_constraints = [
        "//roo_testing/platforms:esp_idf",
        "//roo_testing/platforms:idf",
        "//roo_testing/platforms:roo_testing",
    ],
)

cc_library(
    name = "idf",
    data = [":idf_platform_guard"],
    visibility = ["//visibility:public"],
    deps = [":esp_idf"],
)

profile_guard(
    name = "arduino_platform_guard",
    environment = "The roo_testing Arduino environment",
    required_constraints = [
        "//roo_testing/platforms:arduino",
        "//roo_testing/platforms:roo_testing",
    ],
)

cc_library(
    name = "arduino",
    srcs = ["arduino_profile_contract.cc"],
    copts = ["-Werror"],
    data = [":arduino_platform_guard"],
    visibility = ["//visibility:public"],
    deps = [":emulator"],
)

# A runtime target validates identity only; it does not link a scheduler.
profile_guard(
    name = "freertos_platform_guard",
    environment = "The roo_testing FreeRTOS runtime",
    required_constraints = ["//roo_testing/platforms:freertos"],
)

profile_guard(
    name = "bare_metal_platform_guard",
    environment = "The roo_testing bare-metal runtime",
    required_constraints = ["//roo_testing/platforms:bare_metal"],
)

cc_library(
    name = "runtime",
    srcs = ["runtime_profile_contract.c"],
    copts = ["-Werror"],
    data = select({
        "//roo_testing/platforms:is_bare_metal": [":bare_metal_platform_guard"],
        "//conditions:default": [":freertos_platform_guard"],
    }),
    local_defines = select({
        "//roo_testing/platforms:is_bare_metal": ["ROO_TESTING_EXPECT_BARE_METAL=1"],
        "//conditions:default": ["ROO_TESTING_EXPECT_FREERTOS=1"],
    }),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "freertos",
    data = [":freertos_platform_guard"],
    visibility = ["//visibility:public"],
    deps = [":runtime"],
)

cc_library(
    name = "arduino_esp32",
    srcs = ["arduino_esp32_profile_contract.cc"],
    copts = ["-Werror"],
    visibility = ["//visibility:public"],
    deps = [
        ":arduino",
        ":esp_idf",
    ],
)

profile_guard(
    name = "pico_sdk_platform_guard",
    environment = "The roo_testing Pico SDK environment",
    required_constraints = [
        "//roo_testing/platforms:pico_sdk",
        "//roo_testing/platforms:roo_testing",
        "//roo_testing/platforms:rp2040",
    ],
)

cc_library(
    name = "pico_sdk",
    srcs = ["pico_sdk_profile_contract.cc"],
    copts = ["-Werror"],
    data = [":pico_sdk_platform_guard"],
    local_defines = select({
        "//roo_testing/platforms:is_pico": ["ROO_TESTING_EXPECT_PICO_FRONTEND=1"],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":emulator",
        "//roo_testing/soc:target",
    ],
)

cc_library(
    name = "arduino_pico",
    srcs = ["arduino_pico_profile_contract.cc"],
    copts = ["-Werror"],
    visibility = ["//visibility:public"],
    deps = [
        ":arduino",
        ":pico_sdk",
    ],
)
