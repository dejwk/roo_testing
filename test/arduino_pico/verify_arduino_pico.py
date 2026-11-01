#!/usr/bin/env python3
"""Check Arduino-Pico selection, source isolation, console and runner cleanup."""

import json
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "pico"))
from verify_pico import ROOT, TEST_ROOT, RunBazel
from uart_console_test import CheckFlushedOutput

PROFILE = "--config=roo_testing_arduino_rp2040"


def Run(command, *args, **kwargs):
    return RunBazel(command, *args, profile=PROFILE, **kwargs)


def CheckIsolation():
    """Check selected implementations and global identity on real compile actions."""
    query = "deps(//examples/arduino_pico:peripherals)"
    labels = Run("cquery", query)
    for forbidden in ("frameworks/esp-idf", "frameworks/arduino-esp32",
                      "frameworks/esp32_shims", "microcontrollers/esp32",
                      "roo_testing/sys:"):
        if forbidden in labels:
            raise RuntimeError(f"Arduino-Pico acquired {forbidden}")
    (TEST_ROOT / "arduino-pico.deps").write_text(labels)
    actions = json.loads(Run("aquery", query, "--output=jsonproto",
                             "--include_commandline"))["actions"]
    for action in actions:
        if action["mnemonic"] != "CppCompile":
            continue
        args = action["arguments"]
        for definition in ("ARDUINO", "ARDUINO_ARCH_RP2040", "ROO_TESTING",
                           "ROO_TESTING_RUNTIME_BARE_METAL", "PICO_RP2040",
                           "PICO_RP2350"):
            if sum(a.startswith(f"-D{definition}=") for a in args) != 1:
                raise RuntimeError(f"Expected one global {definition}: {args}")
        if any(a.startswith(("-DESP_PLATFORM", "-DARDUINO_ARCH_ESP32",
                             "-D__FREERTOS", "-D__arm__", "-D__riscv")) for a in args):
            raise RuntimeError(f"Foreign identity in native Pico action: {args}")
        source = args[args.index("-c") + 1]
        if source.endswith((".S", ".s", ".asm")) or "/freertos/" in source:
            raise RuntimeError(f"Unsupported implementation entered host build: {source}")


def Binary(target, options=()):
    """Materialize one executable after cached tests or profile switches."""
    Run("build", target, *options)
    output = Run("cquery", target, *options, "--output=files")
    name = target.rsplit(":", 1)[1]
    return ROOT / next(line for line in output.splitlines() if line.endswith("/" + name))


def CheckRunners():
    """Require cleanup and exact nonzero result propagation in both time modes."""
    for mode in ("manual", "auto"):
        executable = Binary(f"//test/pico:arduino_runner_{mode}_test")
        result = subprocess.run([str(executable), "--gtest_also_run_disabled_tests",
                                 "--gtest_filter=PicoRunnerTest.DISABLED_PropagatesFailure"],
                                text=True, capture_output=True, timeout=20)
        if result.returncode != 1 or "Intentional Pico runner" not in result.stdout:
            raise RuntimeError("Arduino runner lost its status or timer cleanup")


def Main():
    """Validate the second native frontend without entering unsupported features."""
    if TEST_ROOT == Path("/tmp") or Path("/tmp") in TEST_ROOT.parents:
        raise RuntimeError("Refusing RAM-backed Bazel storage")
    TEST_ROOT.mkdir(parents=True, exist_ok=True)
    print(Run("test", "//:arduino_pico_tests", "--test_output=errors"), end="")
    CheckIsolation()
    CheckRunners()
    for name, diagnostic in (("unsupported_core1_probe", "pico/multicore.h"),
                             ("unsupported_freertos_probe", "FreeRTOS.h")):
        Run("build", f"//test/arduino_pico:{name}", succeeds=False, diagnostic=diagnostic)
    Run("build", "//:arduino", "--copt=-D__FREERTOS=1", succeeds=False,
        diagnostic="cannot carry FreeRTOS/ESP-IDF identity")
    Run("build", "//:esp_idf_main", succeeds=False,
        diagnostic="requires target-platform constraint(s)")
    for options in ((), ("--config=asan",)):
        print(Run("test", "//:arduino_pico_tests", *options, "--test_output=errors"), end="")
        CheckFlushedOutput(Binary("//test/arduino_pico:console_probe", options))
    print("Arduino-Pico core, isolation, explicit exclusions, console, runners and ASAN verified")


if __name__ == "__main__":
    Main()
