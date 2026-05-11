#!/usr/bin/env python3
import argparse
import os
import selectors
import subprocess
import sys
import time


PASS_MARKERS = ("Passed",)
FAIL_MARKERS = ("Failed", "Unimplemented opcode", "HALT instruction not implemented")


def main() -> int:
    parser = argparse.ArgumentParser(description="Run a Blargg Game Boy test ROM.")
    parser.add_argument("emulator")
    parser.add_argument("rom")
    parser.add_argument("--timeout", type=float, default=30.0)
    args = parser.parse_args()

    proc = subprocess.Popen(
        [args.emulator, args.rom],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )

    assert proc.stdout is not None
    os.set_blocking(proc.stdout.fileno(), False)
    selector = selectors.DefaultSelector()
    selector.register(proc.stdout, selectors.EVENT_READ)

    output = []
    deadline = time.monotonic() + args.timeout

    while True:
        for key, _ in selector.select(timeout=0.05):
            chunk = os.read(key.fileobj.fileno(), 4096)
            if not chunk:
                selector.unregister(key.fileobj)
                break

            text_chunk = chunk.decode(errors="replace")
            output.append(text_chunk)
            sys.stdout.write(text_chunk)
            sys.stdout.flush()

        text = "".join(output)
        if any(marker in text for marker in FAIL_MARKERS):
            proc.kill()
            proc.wait()
            return 1

        if any(marker in text for marker in PASS_MARKERS):
            proc.kill()
            proc.wait()
            return 0

        if proc.poll() is not None:
            break

        if time.monotonic() >= deadline:
            proc.kill()
            proc.wait()
            print(f"\nTimed out after {args.timeout:.1f}s")
            return 1

    text = "".join(output)
    if proc.returncode == 0 and any(marker in text for marker in PASS_MARKERS):
        return 0

    return proc.returncode or 1


if __name__ == "__main__":
    raise SystemExit(main())
