#!/usr/bin/env python3
"""Cross-compile the N64 ROM's own C source list for MIPS64 and check it links.

The ROM is built with libdragon, which is not available in every checkout, so
nothing here ever compiled the N64 target -- and the Makefile's source list
drifted from CMakeLists.txt unnoticed. `bret_defense.c` was added to the host
build and not the ROM; by the time this was written five more files were
missing too, so the ROM had six undefined symbols and could not have linked.

A full ROM needs libdragon. Catching this class of drift does not: an ordinary
mips64 cross-compiler compiles the portable core for the real target word size
and endianness, and a relocatable link reports any symbol the ROM's own source
list fails to define.

The platform layer (src/platform/n64) needs libdragon's headers, so it used to
be skipped outright -- which meant the RDPQ code had never been compiled by
anything in this repo, not even type-checked. It is now compiled WHEN those
headers can be found (N64_INST, or a libdragon checkout the finder locates) and
reported as skipped when they cannot. Opportunistic is worth having: a renderer
that only ever gets read is a renderer whose API calls nobody has checked.

Exits non-zero, naming the symbols and the files that define them, when the
ROM source list is incomplete.
"""
from __future__ import annotations

import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
CC = "mips64-linux-gnuabi64-gcc"
LD = "mips64-linux-gnuabi64-ld"
NM = "mips64-linux-gnuabi64-nm"

# Provided by libc or the libdragon platform layer, not by the ported core.
EXTERNAL = re.compile(
    r"^(mem(cpy|set|move|cmp)|str(len|cmp|ncmp|cpy|ncpy|chr|str)|abs|labs|"
    r"sqrt|snprintf|sprintf|printf|puts|putchar|malloc|free|calloc|realloc|"
    r"exit|assert|qsort|rand|srand|__).*")


def libdragon_include() -> "pathlib.Path | None":
    """libdragon's include directory, if this machine has one.

    N64_INST is the official variable. Beyond that, look for a checkout whose
    include/ holds libdragon.h -- a fetched tree is enough to TYPE-CHECK the
    platform layer even when it is not enough to link a ROM.
    """
    env = os.environ.get("N64_INST")
    cands: list[pathlib.Path] = []
    if env:
        cands += [pathlib.Path(env) / "mips64-elf" / "include",
                  pathlib.Path(env) / "include"]
    cands += [pathlib.Path("/opt/libdragon/include"),
              pathlib.Path("/usr/local/libdragon/include")]
    cands += sorted(pathlib.Path("/tmp").glob("wm-libdragon-*/include"))
    for d in cands:
        if (d / "libdragon.h").is_file():
            return d
    return None


def check_platform_layer(inc: pathlib.Path) -> "list[str]":
    """Compile each src/platform/n64 file. Returns the ones that fail.

    THE FLAGS ARE THE CHECK. A first version of this passed -Wall -Wextra and
    treated warnings as tolerable, because libdragon's own rdpq.h emits
    -Wformat warnings from its assert macros. Mutation testing then showed it
    accepted `rdpq_set_prim_colour(...)` -- a misspelt API call -- and
    `rdpq_mode_alphacompare("1")`, because in C both are warnings, not errors.
    A check that compiles broken RDPQ code is worse than no check: it reads as
    verification.

    So the diagnostics that mean "this call does not exist" or "this argument
    is the wrong kind of thing" are promoted to errors, and only libdragon's
    own -Wformat noise is silenced -- narrowly, by name, rather than by
    tolerating every warning.
    """
    strict = [
        "-Werror=implicit-function-declaration",
        "-Werror=implicit-int",
        "-Werror=int-conversion",
        "-Werror=incompatible-pointer-types",
        "-Werror=return-type",
        "-Werror=undef",
        # libdragon's assertf macros, not ours.
        "-Wno-format",
    ]
    bad: list[str] = []
    plat = ROOT / "src" / "platform" / "n64"
    with tempfile.TemporaryDirectory() as tmp:
        for src in sorted(plat.glob("*.c")):
            r = subprocess.run(
                [CC, "-c", "-I", str(inc), "-I", str(ROOT / "include"),
                 "-I", str(plat), "-I", str(ROOT / "build/include"),
                 "-O1", "-std=gnu11", "-Wall"] + strict + ["-o",
                 str(pathlib.Path(tmp) / (src.stem + ".o")), str(src)],
                capture_output=True, text=True)
            if r.returncode != 0:
                bad.append(f"{src.relative_to(ROOT)}:\n{r.stderr}")
    return bad


def rom_sources() -> list[str]:
    text = (ROOT / "Makefile").read_text()
    found: list[str] = []
    for var in ("CORE_C", "FIX38_ARCADE_C", "ASSET_C"):
        for m in re.finditer(rf"^{var}\s*[:+]?=\s*((?:.*\\\n)*.*)$", text, re.M):
            body = m.group(1).replace("\\\n", " ")
            found += [t for t in body.split() if t.endswith(".c")]
    return sorted(set(found))


def main() -> int:
    if not shutil.which(CC):
        print(f"n64_link_check: {CC} not installed; skipping", file=sys.stderr)
        return 0

    missing_on_disk: list[str] = []
    with tempfile.TemporaryDirectory() as tmp:
        objs = []
        for rel in rom_sources():
            src = ROOT / rel
            if not src.exists():
                # Generated asset data, produced from original artwork that is
                # not in every checkout. Absent files cannot be compiled, but
                # they also cannot be the cause of an undefined symbol here.
                missing_on_disk.append(rel)
                continue
            obj = pathlib.Path(tmp) / (rel.replace("/", "_")[:-2] + ".o")
            r = subprocess.run(
                [CC, "-c", "-I", str(ROOT / "include"), "-I", str(ROOT / "build/include"),
                 "-O1", "-std=c11", "-Wall", "-Wextra", "-o", str(obj), str(src)],
                capture_output=True, text=True)
            if r.returncode != 0:
                print(f"n64_link_check: {rel} fails to compile for mips64:\n{r.stderr}",
                      file=sys.stderr)
                return 1
            objs.append(str(obj))

        combined = pathlib.Path(tmp) / "core.o"
        r = subprocess.run([LD, "-r", "-o", str(combined)] + objs,
                           capture_output=True, text=True)
        if r.returncode != 0:
            print("n64_link_check: relocatable link failed:\n" + r.stderr, file=sys.stderr)
            return 1

        r = subprocess.run([NM, "-u", str(combined)], capture_output=True, text=True)
        undef = sorted({line.split()[-1] for line in r.stdout.splitlines() if line.strip()})

    inc = libdragon_include()
    if inc is None:
        print("n64_link_check: libdragon headers not found "
              "(set N64_INST); src/platform/n64 NOT compiled")
    else:
        bad = check_platform_layer(inc)
        if bad:
            print("n64_link_check: the platform layer fails to compile for "
                  "mips64:\n" + "\n".join(bad), file=sys.stderr)
            return 1
        n = len(list((ROOT / "src" / "platform" / "n64").glob("*.c")))
        print(f"n64_link_check: platform layer compiles for mips64 "
              f"({n} files, libdragon at {inc})")

    unresolved = [s for s in undef if not EXTERNAL.match(s)]
    if unresolved:
        print("n64_link_check: the ROM source list in Makefile does not define:",
              file=sys.stderr)
        for sym in unresolved:
            hit = subprocess.run(
                ["grep", "-rl", rf"\b{sym}\s*(", str(ROOT / "src"), "--include=*.c"],
                capture_output=True, text=True).stdout.split("\n")[0]
            where = pathlib.Path(hit).relative_to(ROOT) if hit else "?"
            print(f"    {sym}  (defined in {where})", file=sys.stderr)
        print("Add the naming file(s) to the Makefile's source list.", file=sys.stderr)
        return 1

    print(f"n64_link_check: ROM core links clean for mips64 "
          f"({len(undef) - len(unresolved)} libc/platform externs"
          + (f", {len(missing_on_disk)} generated asset files absent)" if missing_on_disk
             else ")"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
