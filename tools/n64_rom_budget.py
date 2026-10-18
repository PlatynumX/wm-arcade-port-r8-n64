#!/usr/bin/env python3
"""Measure the ROM against its cartridge budget.

Nobody knew how big this ROM was. The wrestler art had been measured
(docs recorded 38.1 MiB, later 35.7 after de-duplicating palettes) but
the CODE side never had been, and the code side is where the surprise
would be: src/generated/anim_programs.c is 25,000 lines, bret_sprites.c
76,000 and progress_wrestlers.c 106,000, and all of that becomes
read-only data in the cartridge.

So: compile the ROM's own source list for mips64 -- the same list
tools/n64_link_check.py links and the same compiler -- sum the sections
that occupy cartridge space, add the DragonFS payload, and compare the
total with the budget. A number that is checked on every run cannot
drift into "probably fine".

WHAT COUNTS, AND WHY IT IS NOT A LIST OF NAMES. The first version of
this tool matched section names: .text, .rodata* and .data. It reported
6.11 MiB of code, and it was wrong by 2.8 MiB, because the single
biggest thing in the ROM lands in a section that list does not name.
anim_programs.c is 2.4 MiB of *pointer tables* -- arrays of pointers to
other static data -- and GCC puts those in `.data.rel.ro.local`: data
that is read-only once the relocations are applied. Read-only, so not
.rodata by name; write-flagged, so not matched as .data either. It is
as cartridge-resident as anything else.

So classify by what the ELF actually says instead. A section occupies
cartridge space if it is ALLOC and has contents; if it is ALLOC and
NOBITS it is .bss, which is RAM the loader zeroes and a different
budget (reported separately -- the N64 has 4 or 8 MiB of it). Anything
not ALLOC -- .pdr, .comment, .debug_*, the relocation tables -- is
link-time metadata that never reaches the cartridge. Named lists go
stale when the compiler invents a section; flags do not.

HOW GOOD IS THE NUMBER. Two effects push it up and two push it down,
so it is an estimate rather than a bound. Up: mergeable string sections
(.rodata.str1.8) are summed per object, and the linker deduplicates
identical strings across all of them; and --gc-sections work discards
whatever nothing references. Down: it omits libdragon itself, the DFS
header and per-file overhead, and any padding to a power of two. The
cross-compiler also targets mips64-linux rather than bare-metal
mips64-elf, so codegen sizes are indicative. Good enough to answer "is
there room", not the figure a real build prints.
"""
from __future__ import annotations

import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
CC = "mips64-linux-gnuabi64-gcc"
READELF = "mips64-linux-gnuabi64-readelf"

# The cartridge this port targets. Stated by the project rather than
# derived: 64 MiB is the usual N64 maximum, and this build is allowed
# more, so the number lives here where it can be changed deliberately.
BUDGET_BYTES = 78 * 1000 * 1000

# Fail before the cartridge is actually full, so the first warning is
# not also the emergency.
WARN_FRACTION = 0.85

# ALLOC sections that a bare-metal link does not carry: per-object ABI
# metadata for the Linux dynamic loader. Tens of bytes each, but 169
# objects' worth of it would be noise in a byte-exact total.
METADATA_SECTIONS = (".MIPS.options", ".MIPS.abiflags", ".reginfo")


def _libdragon_include() -> pathlib.Path | None:
    """Reuse whatever tools/n64_platform_check.py found."""
    import importlib.util
    spec = importlib.util.spec_from_file_location(
        "pc", ROOT / "tools" / "n64_platform_check.py")
    if spec is None or spec.loader is None:
        return None
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    inc, _how = mod._libdragon_include()
    return inc


def rom_sources() -> list[str]:
    """Every .c the ROM builds, platform layer included.

    n64_link_check.py reads the same three variables and deliberately
    leaves out N64_C because it cannot compile the platform layer; this
    can, so it counts it.
    """
    text = (ROOT / "Makefile").read_text()
    found: list[str] = []
    for var in ("CORE_C", "FIX38_ARCADE_C", "ASSET_C", "N64_C"):
        for m in re.finditer(rf"^{var}\s*[:+]?=\s*((?:.*\\\n)*.*)$", text, re.M):
            body = m.group(1).replace("\\\n", " ")
            found += [t for t in body.split() if t.endswith(".c")]
    return sorted(set(found))


_HDR = re.compile(r"^\s*\[\s*\d+\]\s+(.*)$")


def sections(obj: pathlib.Path) -> list[tuple[str, str, int, str]]:
    """(name, type, size, flags) for every section header.

    readelf's columns are name/type/address/off/size/es, then flags --
    which are absent entirely for an unflagged section like .pdr --
    then link/info/align. So read the fixed six from the front and the
    fixed three from the back, and whatever is left over is the flags.
    """
    out = subprocess.run([READELF, "-S", "-W", str(obj)],
                         capture_output=True, text=True, check=True).stdout
    found = []
    for line in out.splitlines():
        m = _HDR.match(line)
        if not m:
            continue
        t = m.group(1).split()
        if len(t) < 9:          # the NULL section prints with no name
            continue
        name, kind, size = t[0], t[1], t[4]
        middle = t[6:-3]
        flags = middle[0] if middle else ""
        try:
            n = int(size, 16)
        except ValueError:
            continue
        found.append((name, kind, n, flags))
    return found


def filesystem_bytes() -> tuple[int, int]:
    """(bytes, files) of the DragonFS payload, or (0, 0) when absent.

    Real byte totals, not `du`: with 5,000-odd small files the
    block-rounded figure overstates the cartridge cost by a megabyte
    and a half, all of it tail padding that DragonFS does not pay.
    """
    fs = ROOT / "filesystem"
    if not fs.is_dir():
        return 0, 0
    total = count = 0
    for p in fs.rglob("*"):
        if p.is_file():
            total += p.stat().st_size
            count += 1
    return total, count


def main() -> int:
    if not shutil.which(CC) or not shutil.which(READELF):
        print("n64_rom_budget: %s not installed; skipping" % CC)
        return 0
    include = _libdragon_include()

    # Reporting buckets. The split is for the reader's benefit -- only
    # the sum is the cartridge figure.
    code = ro = relro = rw = bss = 0
    compiled = skipped = failed = 0
    biggest: list[tuple[int, str]] = []

    with tempfile.TemporaryDirectory() as tmp:
        for rel in rom_sources():
            src = ROOT / rel
            if not src.exists():
                # Generated asset data, produced from original artwork
                # that is not in every checkout.
                skipped += 1
                continue
            obj = pathlib.Path(tmp) / (rel.replace("/", "_")[:-2] + ".o")
            cmd = [CC, "-c", "-O2", "-std=gnu11",
                   "-I", str(ROOT / "include"), "-I", str(ROOT), str(src),
                   "-o", str(obj)]
            if include is not None:
                cmd[4:4] = ["-I", str(include)]
            proc = subprocess.run(cmd, capture_output=True, text=True)
            if proc.returncode != 0:
                # Only the platform layer can fail for want of libdragon;
                # everything else failing is a real problem worth naming.
                if include is None and rel.startswith("src/platform/"):
                    skipped += 1
                    continue
                sys.stderr.write(proc.stdout + proc.stderr)
                failed += 1
                continue
            compiled += 1
            own = 0
            for name, kind, n, flags in sections(obj):
                if "A" not in flags:
                    continue            # link metadata, never shipped
                if kind == "NOBITS":
                    bss += n            # RAM, zeroed by the loader
                    continue
                if name in METADATA_SECTIONS:
                    continue
                own += n
                if "X" in flags:
                    code += n
                elif name.startswith(".data.rel.ro"):
                    relro += n
                elif "W" in flags:
                    rw += n
                else:
                    ro += n
            biggest.append((own, rel))

    if failed:
        print("n64_rom_budget: %d file(s) failed to compile" % failed)
        return 1

    rom = code + ro + relro + rw
    fs_bytes, fs_files = filesystem_bytes()
    total = rom + fs_bytes

    mib = 1024.0 * 1024.0
    print("n64_rom_budget: %d compiled, %d absent" % (compiled, skipped))
    print("  code (executable)  %11d  %7.2f MiB" % (code, code / mib))
    print("  read-only data     %11d  %7.2f MiB" % (ro, ro / mib))
    print("  relocated ro data  %11d  %7.2f MiB" % (relro, relro / mib))
    print("  writable data      %11d  %7.2f MiB" % (rw, rw / mib))
    print("  code subtotal      %11d  %7.2f MiB" % (rom, rom / mib))
    if fs_files:
        print("  DragonFS payload   %11d  %7.2f MiB  (%d files)"
              % (fs_bytes, fs_bytes / mib, fs_files))
    else:
        print("  DragonFS payload             0     0.00 MiB  (not built here)")
    print("  TOTAL              %11d  %7.2f MiB" % (total, total / mib))
    print("  budget             %11d  %7.2f MiB  (%.1f%% used)"
          % (BUDGET_BYTES, BUDGET_BYTES / mib, 100.0 * total / BUDGET_BYTES))
    print("  .bss (RAM, not cartridge) %d bytes = %.2f MiB" % (bss, bss / mib))

    biggest.sort(reverse=True)
    print("  largest contributors:")
    for n, rel in biggest[:8]:
        print("    %9d  %6.2f MiB  %s" % (n, n / mib, rel))

    if total > BUDGET_BYTES:
        print("n64_rom_budget: OVER BUDGET by %d bytes" % (total - BUDGET_BYTES))
        return 1
    if total > BUDGET_BYTES * WARN_FRACTION:
        print("n64_rom_budget: over %.0f%% of budget -- worth looking at"
              % (100 * WARN_FRACTION))
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
