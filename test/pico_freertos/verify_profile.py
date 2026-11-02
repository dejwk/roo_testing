#!/usr/bin/env python3
"""Check the Pico FreeRTOS graph and real C/C++ compile identities."""

import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
PROFILE = "--config=roo_testing_arduino_rp2040_freertos"
QUERY = "deps(set(//test/pico_freertos:all //examples/arduino_pico:peripherals_test))"


def Bazel(command, *arguments):
    invocation = [os.environ.get("BAZEL", "bazel")]
    output_base = os.environ.get("ROO_TESTING_BAZEL_OUTPUT_BASE")
    if output_base:
        resolved = Path(output_base).resolve()
        if resolved == Path("/tmp") or Path("/tmp") in resolved.parents:
            raise RuntimeError("Bazel storage must not use /tmp")
        invocation.append(f"--output_base={resolved}")
    invocation.extend((command, PROFILE, *arguments))
    result = subprocess.run(invocation, cwd=ROOT, text=True, capture_output=True)
    if result.returncode != 0:
        raise RuntimeError(result.stdout + result.stderr)
    return result.stdout


def main():
    labels = Bazel("cquery", QUERY, "--output=label")
    for forbidden in ("frameworks/esp-idf", "frameworks/esp32_shims",
                      "frameworks/arduino-esp32", "microcontrollers/esp32"):
        if forbidden in labels:
            raise RuntimeError(f"Foreign kernel/frontend dependency: {forbidden}")
    graph = json.loads(Bazel("aquery", QUERY, "--output=jsonproto",
                            "--include_commandline"))
    seen_c = False
    seen_cpp = False
    for action in graph["actions"]:
        if action["mnemonic"] != "CppCompile":
            continue
        arguments = action["arguments"]
        source = arguments[arguments.index("-c") + 1]
        for macro in ("ROO_TESTING", "ROO_TESTING_SOC_RP2040",
                      "ROO_TESTING_RUNTIME_FREERTOS", "__FREERTOS",
                      "ARDUINO", "ARDUINO_ARCH_RP2040"):
            definitions = [arg for arg in arguments
                           if arg.startswith(f"-D{macro}=")]
            if len(definitions) != 1:
                raise RuntimeError(f"Expected one global {macro}: {source}")
        for argument in arguments:
            if argument.startswith(("-DESP32", "-DESP_PLATFORM", "-DCONFIG_IDF",
                                    "-DARDUINO_ARCH_ESP32", "-D__arm__")):
                raise RuntimeError(f"Foreign compile identity: {argument}")
        if source.endswith((".S", ".s", ".asm")):
            raise RuntimeError(f"Hardware assembly in native graph: {source}")
        seen_c |= source.endswith("test/pico_freertos/identity.c")
        seen_cpp |= source.endswith("test/pico_freertos/identity_test.cpp")
    if not (seen_c and seen_cpp):
        raise RuntimeError("Missing unrelated C or public-header C++ probe")
    print("Pico FreeRTOS dependency and compile identities verified")


if __name__ == "__main__":
    main()
