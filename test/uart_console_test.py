#!/usr/bin/env python3
"""Exercise UART console readiness and binary output using private pipes."""

import argparse
import os
from pathlib import Path
import resource
import selectors
import subprocess
import tempfile


def DisableCoreDump():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))


def RunConsole(binary, args, incoming):
    # ESP32 initializes NVS; a fresh private working directory keeps its storage
    # and startup diagnostics separate from the user's files and UART bytes.
    with tempfile.TemporaryDirectory(prefix="roo-console-") as work:
        return subprocess.run([str(binary), *args], input=incoming,
                              capture_output=True, timeout=20,
                              cwd=work, env={**os.environ, "BUILD_WORKSPACE_DIRECTORY": work},
                              preexec_fn=DisableCoreDump)


def CheckFlushedOutput(binary):
    # The child waits for input after writing. Output must be visible before
    # subsequent communication or process exit can incidentally flush it.
    with tempfile.TemporaryDirectory(prefix="roo-console-") as work:
        with subprocess.Popen([str(binary), "flush"], stdin=subprocess.PIPE,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              cwd=work, env={**os.environ, "BUILD_WORKSPACE_DIRECTORY": work}) as child:
            try:
                received = b""
                expected = b"ready\n"
                with selectors.DefaultSelector() as selector:
                    selector.register(child.stdout, selectors.EVENT_READ)
                    while len(received) < len(expected):
                        if not selector.select(timeout=10):
                            raise RuntimeError("UART output was buffered awaiting later communication")
                        data = os.read(child.stdout.fileno(), len(expected) - len(received))
                        if not data:
                            raise RuntimeError("Console exited before producing its flushed output")
                        received += data
                output, error = child.communicate(input=b"!", timeout=10)
                if received != expected or child.returncode != 0 or output:
                    raise RuntimeError(f"Console immediate flush failed: {error!r}")
            finally:
                if child.poll() is None:
                    child.kill()
                child.communicate()


def CheckConsole(binary, eof_diagnostic=b"Pico UART input reached EOF"):
    # Keep stdin open without providing data: readiness must return immediately.
    with tempfile.TemporaryDirectory(prefix="roo-console-") as work:
        with subprocess.Popen([str(binary), "empty"], stdin=subprocess.PIPE,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              cwd=work, env={**os.environ, "BUILD_WORKSPACE_DIRECTORY": work}) as child:
            try:
                if child.wait(timeout=10) != 0:
                    raise RuntimeError("Empty open stdin reported readable")
            finally:
                # A readiness regression must not make shutdown wait forever.
                if child.poll() is None:
                    child.kill()
                child.communicate()
    CheckFlushedOutput(binary)
    data = bytes(range(256)) * 300
    for mode, incoming, expected in (("empty", b"", b""),
                                     ("echo", data, data),
                                     ("text", b"", b"hello\r\nworld\r\n")):
        args = [mode]
        if mode == "echo":
            args.append(str(len(incoming)))
        result = RunConsole(binary, args, incoming)
        if result.returncode != 0 or result.stdout != expected:
            raise RuntimeError(f"Console {mode} failed: {result.stderr!r}")
    failure = RunConsole(binary, ["eof"], b"")
    if eof_diagnostic is None:
        if failure.returncode != 0:
            raise RuntimeError("Nonblocking console EOF must return zero bytes")
    elif failure.returncode >= 0 or eof_diagnostic not in failure.stderr:
        raise RuntimeError("Blocking console EOF must fail explicitly")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--nonblocking-eof", action="store_true")
    args = parser.parse_args()
    diagnostic = None if args.nonblocking_eof else b"Pico UART input reached EOF"
    CheckConsole(args.binary.resolve(), eof_diagnostic=diagnostic)
