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

Three tiers, strongest first, and the check says which one it ran:

  MIPS64 -O2      a real compile to object code with the same cross
                  compiler n64_link_check.py uses for the core. The
                  difference that matters is ENDIANNESS: this host is
                  little-endian and both this target and the N64 are
                  big-endian, so a byte-order assumption that the host
                  pass cannot see fails here. (Type widths happen to
                  agree between the two, so those are not what this
                  buys.) It also generates code rather than parsing.
  host syntax     -fsyntax-only with the host compiler, when no cross
                  compiler is installed.
  shim            the old hand-written stand-in, for the one file it
                  can honestly speak for, when there are no real
                  headers at all.

What none of them does is build a ROM, and this must not be mistaken
for one. The cross-compiler here targets mips64-linux, not the bare
metal mips64-elf libdragon links with, so these objects are not ROM
objects -- what they prove is that the code compiles for the real word
size and endianness, not that it links or runs. The asset pipeline,
DragonFS and the actual link still need the toolchain.

libdragon's own headers emit format-string warnings on the HOST pass
(int32_t is `int` there and `long` on MIPS), so warnings from inside
libdragon are reported and not counted, while any warning from OUR
files is a failure.
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

# The same cross-compiler tools/n64_link_check.py uses for the core.
CROSS_CC = "mips64-linux-gnuabi64-gcc"


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
             third_party: pathlib.Path | None,
             obj: pathlib.Path | None = None) -> int:
    """0 on success. Warnings from OUR file count as failure; warnings
    from inside the headers are shown and forgiven.

    With `obj` this really compiles, to object code, which is what makes
    the cross pass worth more than the host one."""
    if obj is not None:
        cmd = [cc, "-c", "-O2", STD, "-Wall", "-Wextra",
               "-I", str(include), "-I", str(ROOT / "include"),
               str(path), "-o", str(obj)]
    else:
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
        cross = shutil.which(CROSS_CC)
        failures = 0
        with tempfile.TemporaryDirectory() as tmp:
            for rel in COVERED:
                path = ROOT / rel
                if not path.exists():
                    print("n64_platform_check: missing %s" % rel)
                    failures += 1
                    continue
                if cross:
                    obj = pathlib.Path(tmp) / (path.stem + ".o")
                    failures += _compile(cross, path, include, include, obj)
                else:
                    failures += _compile(cc, path, include, include)
        if failures:
            print("n64_platform_check: %d file(s) failed" % failures)
            return 1
        if cross:
            print("n64_platform_check: %d platform source(s) compile clean "
                  "for mips64 at -O2 against real libdragon headers (%s)"
                  % (len(COVERED), how))
        else:
            print("n64_platform_check: %d platform source(s) type-check clean "
                  "on the host against real libdragon headers (%s); no %s, so "
                  "nothing target-dependent was checked"
                  % (len(COVERED), how, CROSS_CC))
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
