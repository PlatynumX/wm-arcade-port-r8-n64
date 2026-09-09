#!/usr/bin/env python3
"""Count ANI_CODE call sites and say which routines are still gaps.

ANIM.ASM:1277 _ani_code is `move *a4+,a0,L / call a0` -- an animation
naming a subroutine. The port emits every one as a WM_AOP_CODE carrying
that name, so an untranslated routine is a named, countable gap rather
than a line the extractor threw away, and this is what counts them.

A name is covered when src/core/anim_code.c registers it, or when it is
one of the announcer callers that resolve out of the generated
DCSSOUND.ASM table instead of a registry row.

The corpus is the same one tools/wlprogram.py --roster emits: every
sequence file WRESTLE.CMD links, minus FINISEQ. Counted from the source
rather than from the emitted programs, because a program can be emitted
more than once (a shared body reached under several names) and the point
of the number is how much of the game's own script the port understands.
"""
from __future__ import annotations
import argparse
import collections
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parent.parent
USE_RE = re.compile(r"^\s*(?:\.word|W+L+W*)\s+ANI_CODE\s*,\s*(#?\w+)", re.I)
# A registry row: `{ "name", enter, run, param },` at the top of a line.
ROW_RE = re.compile(r'^\s*\{\s*"([^"]+)"\s*,', re.M)
# The announcer group resolves by name out of the generated table.
CALL_RE = re.compile(r'\{\s*"(CALL_[A-Z_0-9]+|DO_REVERSAL)"\s*,')


def call_sites() -> collections.Counter:
    """routine name -> how many animations name it."""
    uses: collections.Counter = collections.Counter()
    for src in wlanim.linked_files():
        if "SEQ" not in src.name or src.name.startswith("FINI"):
            continue
        for line in src.read_text(errors="replace").splitlines():
            m = USE_RE.match(wlanim.strip_comment(line))
            if m:
                uses[m.group(1)] += 1
    return uses


def covered() -> set[str]:
    names = set(ROW_RE.findall((ROOT / "src/core/anim_code.c").read_text()))
    gen = ROOT / "src/generated/announce_tables.c"
    if gen.exists():
        names |= set(CALL_RE.findall(gen.read_text()))
    return names


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--gaps", action="store_true",
                    help="list the untranslated routines, busiest first")
    ns = ap.parse_args()

    uses = call_sites()
    known = covered()
    total = sum(uses.values())
    if not total:
        print("no ANI_CODE call sites found -- is original/ fetched?")
        return 1
    done = sum(n for k, n in uses.items() if k in known)
    print(f"ANI_CODE {done}/{total} ({100.0 * done / total:.1f}%), "
          f"{sum(1 for k in uses if k in known)}/{len(uses)} names")
    gaps = sorted(((n, k) for k, n in uses.items() if k not in known),
                  reverse=True)
    print(f"remaining {total - done} uses across {len(gaps)} names")
    if ns.gaps:
        for n, k in gaps:
            print(f"  {n:4d}  {k}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
