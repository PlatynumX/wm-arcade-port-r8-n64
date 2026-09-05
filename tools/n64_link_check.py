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
list fails to define. Only the platform layer (src/platform/n64) is skipped,
since that genuinely needs libdragon headers.

Exits non-zero, naming the symbols and the files that define them, when the
ROM source list is incomplete.
"""
from __future__ import annotations

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
