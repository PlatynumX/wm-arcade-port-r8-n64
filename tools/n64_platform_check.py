#!/usr/bin/env python3
"""Compile the N64 platform sources on a host with no N64 toolchain.

tools/n64_link_check.py compiles the ported CORE for mips64 and skips
src/platform/n64 outright, because that layer needs libdragon headers.
The consequence was that in a checkout without libdragon -- which is
most of them, and every CI host this project has -- the platform layer
was compiled by NOTHING. A file nobody can compile is a file whose next
edit ships unverified.

This used to type-check exactly ONE of the five platform sources
against a 28-line hand-written stand-in for libdragon.h, on the
argument that a shim large enough for main.c "would be a fake libdragon
rather than a check". That argument was right about the shim and wrong
about the conclusion: the real headers can simply be fetched. They are
a public git repository, they are the actual API the ROM will be built
against, and nothing about them has to be guessed.

So: REAL libdragon headers, all five files. In order of preference,

  1. $N64_INST/include -- a real local libdragon install, which is
     what a machine that can build the ROM already has.
  2. A pinned shallow clone, cached under the system temp dir so it is
     fetched once rather than per run.
  3. The old shim, for the one file it can honestly cover.
  4. Skip, saying which.

What this does NOT do is build a ROM, and it must not be mistaken for
one. -fsyntax-only with the HOST compiler catches what an unverified
edit actually gets wrong -- undeclared identifiers, wrong argument
types, bad struct members, typos -- against the real declarations. It
does not catch anything that depends on the target: MIPS word sizes,
endianness, alignment, or codegen. libdragon's own headers emit
format-string warnings here for exactly that reason (int32_t is `int`
on this host and `long` on MIPS), so warnings from inside libdragon are
reported and not counted, while any warning from OUR files is a
failure.
"""
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
SHIMS = ROOT / "tests" / "host_shims"

# Every platform source, which is the point of the rewrite.
COVERED = (
    "src/platform/n64/main.c",
    "src/platform/n64/dcs_effect.c",
    "src/platform/n64/dcs_bank.c",
    "src/platform/n64/audio_backend.c",
    "src/platform/n64/streamed_character_art.c",
)

# Only this one is coverable by the shim, if it comes to that.
SHIM_COVERED = ("src/platform/n64/streamed_character_art.c",)

LIBDRAGON_URL = "https://github.com/DragonMinded/libdragon"
# Pinned so the check is reproducible and so a libdragon change lands
# here deliberately rather than as a mystery failure one morning.
LIBDRAGON_COMMIT = "e356bf3f56f7afbf7e5246329562f145965cfdfc"
LIBDRAGON_BRANCH = "trunk"

# libdragon requires GNU extensions -- pputils.h says so with an #error,
# and dlfcn.h has a top-level asm() that needs them too.
STD = "-std=gnu11"


def _cached_clone() -> pathlib.Path | None:
    """The pinned headers, fetched once into a cache directory."""
    cache = pathlib.Path(tempfile.gettempdir()) / (
        "wm-libdragon-" + LIBDRAGON_COMMIT[:12])
    inc = cache / "include"
    if (inc / "libdragon.h").exists():
        return inc
    if not shutil.which("git"):
        return None
    try:
        subprocess.run(
            ["git", "clone", "--quiet", "--depth", "1",
             "--branch", LIBDRAGON_BRANCH, LIBDRAGON_URL, str(cache)],
            check=True, capture_output=True, timeout=600)
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, OSError):
        shutil.rmtree(cache, ignore_errors=True)
        return None
    return inc if (inc / "libdragon.h").exists() else None


def _libdragon_include() -> tuple[pathlib.Path | None, str]:
    inst = os.environ.get("N64_INST")
    if inst:
        inc = pathlib.Path(inst) / "include"
        if (inc / "libdragon.h").exists():
            return inc, "N64_INST"
    inc = _cached_clone()
    if inc is not None:
        return inc, "pinned clone %s" % LIBDRAGON_COMMIT[:12]
    return None, "unavailable"


def _compile(cc: str, path: pathlib.Path, include: pathlib.Path,
             third_party: pathlib.Path | None) -> int:
    """0 on success. Warnings from OUR file count as failure; warnings
    from inside the headers are shown and forgiven."""
    cmd = [cc, "-fsyntax-only", STD, "-Wall", "-Wextra",
           "-I", str(include), "-I", str(ROOT / "include"), str(path)]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    text = proc.stdout + proc.stderr
    if proc.returncode != 0:
        sys.stderr.write(text)
        return 1
    ours = []
    for line in text.splitlines():
        if "warning:" not in line:
            continue
        if third_party is not None and str(third_party) in line:
            continue
        ours.append(line)
    if ours:
        sys.stderr.write("\n".join(ours) + "\n")
        return 1
    return 0


def main() -> int:
    cc = shutil.which("cc") or shutil.which("gcc")
    if not cc:
        print("n64_platform_check: no host compiler, skipping")
        return 0

    include, how = _libdragon_include()
    if include is not None:
        failures = 0
        for rel in COVERED:
            path = ROOT / rel
            if not path.exists():
                print("n64_platform_check: missing %s" % rel)
                failures += 1
                continue
            failures += _compile(cc, path, include, include)
        if failures:
            print("n64_platform_check: %d file(s) failed" % failures)
            return 1
        print("n64_platform_check: %d platform source(s) clean against real "
              "libdragon headers (%s)" % (len(COVERED), how))
        return 0

    # No real headers. Fall back to the shim, which can only speak for
    # the one self-contained file -- and say so rather than implying the
    # platform layer was checked.
    print("n64_platform_check: no libdragon headers (%s); falling back to "
          "the host shim, which covers %d of %d platform sources"
          % (how, len(SHIM_COVERED), len(COVERED)))
    failures = 0
    for rel in SHIM_COVERED:
        path = ROOT / rel
        if not path.exists():
            print("n64_platform_check: missing %s" % rel)
            failures += 1
            continue
        failures += _compile(cc, path, SHIMS, None)
    if failures:
        print("n64_platform_check: %d file(s) failed" % failures)
        return 1
    print("n64_platform_check: %d platform source(s) type-check clean "
          "against the shim" % len(SHIM_COVERED))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
