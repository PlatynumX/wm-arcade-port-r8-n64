#!/usr/bin/env python3
"""Type-check the N64 platform sources on a host with no libdragon.

tools/n64_link_check.py compiles the ported CORE for mips64 and skips
src/platform/n64 outright, because that layer "genuinely needs
libdragon headers". The consequence is that in a checkout without
libdragon -- which is most of them, and every CI host this project has
-- the platform layer is compiled by NOTHING. A file nobody can
compile is a file whose next edit ships unverified, and this one
exists because a rewrite of the DragonFS frame loader had no check at
all behind it.

This does not build a ROM and does not pretend to. It puts
tests/host_shims on the include path, where a small libdragon.h
declares the handful of entry points these files really use, and runs
the host compiler with -fsyntax-only. That catches what an unverified
edit actually gets wrong: undeclared identifiers, wrong argument
types, bad struct members, typos.

A file that needs something the shim does not declare fails here, and
the fix is to add the declaration -- which is also the record of what
the platform layer depends on.
"""
import pathlib
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SHIMS = ROOT / "tests" / "host_shims"

# Sources this check covers. Not the whole directory: main.c pulls in
# far more of libdragon (rdpq, display, controller, audio) than a shim
# can honestly stand in for, and a shim that large would be a fake
# libdragon rather than a check. These are the self-contained ones.
COVERED = (
    "src/platform/n64/streamed_character_art.c",
)


def main() -> int:
    cc = shutil.which("cc") or shutil.which("gcc")
    if not cc:
        print("n64_platform_check: no host compiler, skipping")
        return 0

    failures = 0
    for rel in COVERED:
        path = ROOT / rel
        if not path.exists():
            print("n64_platform_check: missing %s" % rel)
            failures += 1
            continue
        cmd = [cc, "-fsyntax-only", "-Wall", "-Wextra", "-std=c11",
               "-I", str(SHIMS), "-I", str(ROOT / "include"), str(path)]
        proc = subprocess.run(cmd, capture_output=True, text=True)
        if proc.returncode != 0:
            sys.stderr.write(proc.stdout + proc.stderr)
            failures += 1
        elif proc.stderr.strip():
            sys.stderr.write(proc.stderr)
            failures += 1          # warnings are failures here too

    if failures:
        print("n64_platform_check: %d file(s) failed" % failures)
        return 1
    print("n64_platform_check: %d platform source(s) type-check clean"
          % len(COVERED))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
