load("@rules_cc//cc:cc_library.bzl", "cc_library")

alias(
    name = "environment",
    actual = "//roo_testing/frameworks/environment:emulator",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "testing",
    visibility = ["//visibility:public"],
    deps = [
        ":arduino",
    ],
)

cc_library(
    name = "arduino",
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32/cores/esp32:arduino",
    ],
)

cc_library(
    name = "arduino_main",
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32/cores/esp32:main",
    ],
)

cc_library(
    name = "arduino_gtest_main",
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32/cores/esp32:gtest_main",
    ],
)

# Stable public test entry point for ESP-IDF-only host tests. It starts the
# pthread-backed FreeRTOS scheduler without initializing Arduino.
cc_library(
    name = "esp_idf_gtest_main",
    linkstatic = 1,
    tags = ["manual"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/frameworks/environment:idf",
    ],
)

# Stable public process entry point for an ESP-IDF application that defines
# extern "C" void app_main().
cc_library(
    name = "esp_idf_main",
    linkstatic = 1,
    tags = ["manual"],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/frameworks/esp_idf_support:main"],
)

test_suite(
    name = "all_tests",
    tests = [
        "//fltk-1.3.5:core_link_test",
        "//roo_testing/frameworks/esp32_shims:esp_timer_host_manual_test",
        "//roo_testing/frameworks/esp32_shims:esp_timer_host_test",
        "//roo_testing/frameworks/esp32_shims:esp_timer_impl_host_auto_test",
        "//roo_testing/frameworks/esp32_shims:esp_timer_impl_host_test",
        "//roo_testing/frameworks/esp32_shims:idf_sd_compat_test",
        "//test:arduino_console_test",
        "//test:arduino_environment_test",
        "//test:arduino_gtest_environment_test",
        "//test:arduino_main_environment_test",
        "//test:arduino_periman_static_initialization_test",
        "//test:arduino_preferences_startup_test",
        "//test:arduino_sd_diskio_link_test",
        "//test:arduino_spi_clock_test",
        "//test:arduino_uart_api_test",
        "//test:emulator_environment_test",
        "//test:esp_intr_alloc_anonymous_test",
        "//test:esp_intr_alloc_epoch_exhaustion_test",
        "//test:esp_intr_alloc_test",
        "//test:framework_version_test",
        "//test:freertos_posix_isr_context_test",
        "//test:freertos_posix_simulated_interrupt_install_test",
        "//test:freertos_posix_simulated_interrupt_test",
        "//test:freertos_posix_thread_join_regression_test",
        "//test:gpio_test",
        "//test:host_event_gateway_test",
        "//test:host_filesystem_test",
        "//test:host_network_test",
        "//test:idf_environment_test",
        "//test:idf_ledc_test",
        "//test:idf_spi_device_test",
        "//test:idf_uart_device_test",
        "//test:interrupt_controller_exhaustion_test",
        "//test:interrupt_controller_test",
        "//test:legacy_i2c_interface_test",
        "//test:nvs_test",
        "//test:onewire_test",
        "//test:rtc_ds3231_i2c_test",
        "//test:scheduler_safe_host_lock_test",
        "//test:simple_test",
        "//test:soc_environment_test",
        "//test:soc_peripheral_test",
        "//test:soc_profile_test",
        "//test:system_time_alarm_lifecycle_test",
        "//test:system_time_alarms_test",
        "//test:system_time_auto_alarms_test",
        "//test:time_mode_auto_static_initialization_test",
        "//test:time_mode_manual_static_initialization_test",
        "//test:time_publication_test",
        "//test/legacy_sd_headers:legacy_sd_headers_test",
        "//test/profile:arduino_select_test",
        "//test/profile:global_environment_test",
        "//test/profile:runtime_selection_test",
        "//test/wire_master:wire_master_test",
    ],
)

# Kept separate because these targets intentionally opt out of the Arduino
# default and are selected explicitly under the ESP-IDF-only profile.
test_suite(
    name = "idf_tests",
    tags = ["manual"],
    tests = [
        "//roo_testing/frameworks/esp32_shims:esp_timer_host_manual_test",
        "//roo_testing/frameworks/esp32_shims:esp_timer_host_test",
        "//roo_testing/frameworks/esp32_shims:esp_timer_impl_host_auto_test",
        "//roo_testing/frameworks/esp32_shims:esp_timer_impl_host_test",
        "//test:idf_console_test",
        "//test:idf_i2c_device_test",
        "//test:idf_ledc_test",
        "//test:idf_spi_device_test",
        "//test:idf_uart_device_test",
        "//test:legacy_i2c_interface_test",
        "//test:rtc_ds3231_i2c_test",
        "//test:soc_peripheral_test",
        "//test/profile:esp_idf_gtest_main_test",
        "//test/profile:esp_idf_main_test",
        "//test/profile:idf_global_environment_test",
        "//test/profile:idf_sdkconfig_purity_test",
        "//test/profile:idf_select_test",
    ],
)

# SDK applications supply ordinary main(); no ESP-IDF or Arduino startup.
cc_library(
    name = "pico_sdk",
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/frameworks/pico-sdk:pico_sdk"],
)

test_suite(
    name = "pico_tests",
    tags = ["manual"],
    tests = [
        "//examples/pico_gpio:blink_test",
        "//examples/pico_i2c:rtc_test",
        "//examples/pico_spi:display_test",
        "//examples/pico_uart:serial_test",
        "//roo_testing/buses/uart:console_test",
        "//test/pico:gpio_test",
        "//test/pico:i2c_test",
        "//test/pico:runner_auto_test",
        "//test/pico:runner_manual_test",
        "//test/pico:sdk_probe_c",
        "//test/pico:sdk_probe_cpp",
        "//test/pico:spi_auto_test",
        "//test/pico:spi_manual_test",
        "//test/pico:spi_test",
        "//test/pico:spi_wiring_test",
        "//test/pico:time_auto_test",
        "//test/pico:time_manual_test",
        "//test/pico:uart_test",
        "//test/pico:uart_wiring_test",
    ],
)

# Native Pico test runner owns shutdown of the shared clock/alarm service.
cc_library(
    name = "pico_gtest_main",
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/frameworks/pico_support:gtest_main"],
)
