#!/usr/bin/env python3
"""Count ANI_CODE call sites and say which routines are still gaps.

ANIM.ASM:1277 _ani_code is `move *a4+,a0,L / call a0` -- an animation
naming a subroutine. The port emits every one as a WM_AOP_CODE carrying
that name, so an untranslated routine is a named, countable gap rather
than a line the extractor threw away, and this is what counts them.

A call site is covered when src/core/anim_code.c has a row its own
resolution order would reach -- this exact definition, else this file's
unnumbered row, else a global of the name -- or when it names one of the
announcer callers that resolve out of the generated DCSSOUND.ASM table
instead of a registry row. Counting by name alone would call a routine
covered because its NAME is translated while the particular definition
that call site refers to is not.

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
ROW_RE = re.compile(
    r'^\s*\{\s*"(#?[A-Za-z_0-9]+)"\s*,\s*(NULL|"[^"]+")\s*,'
    r'.*,\s*(-?\d+)\s*\},\s*$', re.M)
# The announcer group resolves by name out of the generated table.
CALL_RE = re.compile(r'\{\s*"(CALL_[A-Z_0-9]+|DO_REVERSAL)"\s*,')


def call_sites() -> collections.Counter:
    """(name, file, definition line) -> how many animations name it.

    A local `#name` is counted per DEFINITION, not per name or even per
    file: the same name is defined more than once in one file with a
    different body each time, so two call sites spelled identically can be
    two different routines. `file` and `line` are empty/0 for a global.
    """
    uses: collections.Counter = collections.Counter()
    for src in wlanim.linked_files():
        if "SEQ" not in src.name or src.name.startswith("FINI"):
            continue
        kept = [r.split(";", 1)[0]
                for r in src.read_text(errors="replace").splitlines()]
        for i, raw in enumerate(kept):
            m = USE_RE.match(wlanim.strip_comment(raw))
            if not m:
                continue
            name = m.group(1)
            # A global name still comes from a file: the registry may
            # scope its row to one, and wm_anim_code_run honours that.
            uses[(name, src.name,
                  wlanim.local_label_site(kept, i, name))] += 1
    return uses


def covered() -> set:
    """The (name, file, line) keys the port can actually run."""
    out = set()
    for name, where, line in ROW_RE.findall(
            (ROOT / "src/core/anim_code.c").read_text()):
        out.add((name, "" if where == "NULL" else where.strip('"'), int(line)))
    gen = ROOT / "src/generated/announce_tables.c"
    if gen.exists():
        for name in CALL_RE.findall(gen.read_text()):
            out.add((name, "", 0))
    return out


def resolves(key, known: set) -> bool:
    """Same order wm_anim_code_run resolves in."""
    name, where, line = key
    if line and (name, where, line) in known:
        return True
    if where and (name, where, 0) in known:
        return True
    return (name, "", 0) in known


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
    done = sum(n for k, n in uses.items() if resolves(k, known))

    # Distinct ROUTINES, not distinct call-site keys: a global is one
    # routine however many files call it, a local one per definition.
    def routine(key):
        name, where, line = key
        return (name, where, line) if name.startswith("#") else (name,)

    all_routines, done_routines = set(), set()
    for k in uses:
        all_routines.add(routine(k))
        if resolves(k, known):
            done_routines.add(routine(k))
    print(f"ANI_CODE {done}/{total} ({100.0 * done / total:.1f}%), "
          f"{len(done_routines)}/{len(all_routines)} routines")
    gaps = sorted(((n, k) for k, n in uses.items() if not resolves(k, known)),
                  reverse=True)
    print(f"remaining {total - done} uses across "
          f"{len(all_routines) - len(done_routines)} routines")
    if ns.gaps:
        for n, (name, where, line) in gaps:
            at = f"  ({where}:{line})" if where else ""
            print(f"  {n:4d}  {name}{at}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
