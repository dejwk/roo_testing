#!/usr/bin/env python3
"""Validate Pico FreeRTOS consumers with coordinated local Roo checkouts."""

import os
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "pico_clients"))
from verify_clients import Setup, TEST_ROOT, WORKSPACE

PROFILE = "--config=roo_testing_arduino_rp2040_freertos"
TARGETS = [
    "@roo_threads//:thread_test", "@roo_threads//:mutex_test",
    "@roo_threads//:condition_variable_test", "@roo_threads//:backend_selection_test",
    "@roo_threads//:pico_freertos_auto_test", "@roo_threads//:pico_freertos_manual_test",
    "@roo_time//:roo_time_test", "@roo_time//:compact_time_test",
    "@roo_time//:format_test", "@roo_time//:format_portable_test",
    "@roo_scheduler//:roo_scheduler_test", "@roo_scheduler//:roo_scheduler_allocation_test",
    "@roo_scheduler//:roo_scheduler_auto_time_test", "@roo_scheduler//:roo_scheduler_concurrency_test",
    "@roo_display//:pico_spi_transport_test",
]


def Run(command, *arguments):
    invocation = [os.environ.get("BAZEL", "bazel"),
                  f"--output_user_root={TEST_ROOT / 'configured'}", command]
    if command != "shutdown":
        invocation.append(PROFILE)
    result = subprocess.run(invocation + list(arguments), cwd=WORKSPACE,
                            text=True, capture_output=True)
    if result.returncode != 0:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout + result.stderr


def main():
    Setup()
    try:
        for mode in ("normal", "asan"):
            options = [] if mode == "normal" else ["--config=asan"]
            output = Run("test", *TARGETS, *options, "--test_output=errors")
            (TEST_ROOT / f"freertos-clients-{mode}.log").write_text(output)
            print(output, end="", flush=True)
        labels = Run("cquery", "deps(set(" + " ".join(TARGETS) + "))")
        for forbidden in ("frameworks/esp-idf", "frameworks/arduino-esp32",
                          "frameworks/esp32_shims", "microcontrollers/esp32"):
            if forbidden in labels:
                raise RuntimeError(f"Foreign consumer dependency: {forbidden}")
        (TEST_ROOT / "freertos-clients.deps").write_text(labels)
        print("Pico FreeRTOS Roo consumers passed normal/ASAN and isolation checks")
    finally:
        Run("shutdown")


if __name__ == "__main__":
    main()
