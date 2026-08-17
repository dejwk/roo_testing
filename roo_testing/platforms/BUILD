# Build-wide profile dimensions. These constraints describe the target
# configuration; compiler macros are supplied by the matching named bazelrc
# profile so they reach every C and C++ action in that configuration.

package(default_visibility = ["//visibility:public"])

constraint_setting(name = "emulation_environment")

constraint_value(
    name = "roo_testing",
    constraint_setting = ":emulation_environment",
)

constraint_setting(name = "idf_capability")

constraint_value(
    name = "esp_idf",
    constraint_setting = ":idf_capability",
)

constraint_setting(name = "framework_frontend")

constraint_value(
    name = "idf",
    constraint_setting = ":framework_frontend",
)

constraint_value(
    name = "arduino",
    constraint_setting = ":framework_frontend",
)

constraint_setting(name = "soc")

constraint_value(
    name = "esp32",
    constraint_setting = ":soc",
)

# The emulator is host-native: @platforms//host:host supplies the detected
# Linux CPU/OS constraints while these values describe the emulated software
# environment and SoC. Do not add --copt flags here. Platform flags replace
# other repeatable --copt values instead of composing with them.
platform(
    name = "arduino_esp32",
    constraint_values = [
        ":arduino",
        ":esp32",
        ":esp_idf",
        ":roo_testing",
    ],
    parents = ["@platforms//host:host"],
)

platform(
    name = "idf_esp32",
    constraint_values = [
        ":esp32",
        ":esp_idf",
        ":idf",
        ":roo_testing",
    ],
    parents = ["@platforms//host:host"],
)

config_setting(
    name = "is_roo_testing",
    constraint_values = [":roo_testing"],
)

config_setting(
    name = "is_esp_idf",
    constraint_values = [":esp_idf"],
)

config_setting(
    name = "is_idf",
    constraint_values = [":idf"],
)

config_setting(
    name = "is_arduino",
    constraint_values = [":arduino"],
)

config_setting(
    name = "is_esp32",
    constraint_values = [":esp32"],
)

config_setting(
    name = "is_arduino_esp32",
    constraint_values = [
        ":arduino",
        ":esp32",
        ":esp_idf",
        ":roo_testing",
    ],
)

config_setting(
    name = "is_idf_esp32",
    constraint_values = [
        ":esp32",
        ":esp_idf",
        ":idf",
        ":roo_testing",
    ],
)
