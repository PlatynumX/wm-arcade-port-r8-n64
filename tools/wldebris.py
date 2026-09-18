#!/usr/bin/env python3
"""SPECIAL.ASM's debris tables -- the shape and the sound of a hit.

ANI_DEBRIS / ANI_DEBRISAT create a `react_debris` process, and what that
process decides before any pixel exists is all data:

  #debris_table   eight shape rows (SPECIAL.ASM:3535) -- how many bursts,
                  how many pieces each, how long between them, the random
                  position and velocity spreads, the gravity and lifespan
  WHICH_DEBRIS_SOUND  a per-wrestler sound list (SPECIAL.ASM:3505): the
                  first word is RNDRNG0's inclusive maximum and the rest
                  are the sounds, so a hit on Bam Bam is one of five
  debris_anims    a per-wrestler list of eight debris sprites

Several wrestlers SHARE a sound row -- the labels are stacked, so
BRET_DEBRIS and SHAWN_DEBRIS are the same two sounds and LEX_DEBRIS and
RAZOR_DEBRIS are the same pair -- and that sharing is in the data rather
than asserted here.
"""
from __future__ import annotations
import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim  # noqa: E402
import wlcommands  # noqa: E402

SRC = wlanim.ORIG / "SPECIAL.ASM"
ROSTER_SLOTS = 9
DEBRIS_SHAPES = 8
DEBRIS_ANIMS_PER = 8

WORD_RE = re.compile(r"^\s*\.word\s+(.+)$", re.I)
LONG_RE = re.compile(r"^\s*\.long\s+(.+)$", re.I)
LABEL_RE = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)\s*$")


def _lines() -> list[str]:
    return [wlanim.strip_comment(r)
            for r in SRC.read_text(errors="replace").splitlines()]


def _at(lines: list[str], name: str) -> int:
    for i, line in enumerate(lines):
        if wlanim.label_def(line) == name:
            return i
    raise ValueError(f"SPECIAL.ASM: no {name}")


def shapes() -> list[dict]:
    """The eight #debris_table rows, in order."""
    lines = _lines()
    out = []
    for n in range(DEBRIS_SHAPES):
        i = _at(lines, f"#db{n}") + 1
        vals: list[int] = []
        while i < len(lines) and len(vals) < 15:
            line = lines[i]
            i += 1
            if not line:
                continue
            m = WORD_RE.match(line) or LONG_RE.match(line)
            if not m:
                break
            vals += [wlcommands._value(v, {}) for v in m.group(1).split(",")]
        if len(vals) != 15:
            raise ValueError(f"#db{n} has {len(vals)} values, not 15")
        out.append({
            "loop": vals[0], "count": vals[1], "sleep": vals[2],
            "rxoff": vals[3], "ryoff": vals[4], "rzoff": vals[5],
            "xvel": vals[6], "yvel": vals[7], "zvel": vals[8],
            "rxvel": vals[9], "ryvel": vals[10], "rzvel": vals[11],
            "gravity": vals[12],
            "lifespan": vals[13], "rlifespan": vals[14],
        })
    return out


def sounds() -> list[list[int]]:
    """WHICH_DEBRIS_SOUND, resolved per wrestler.

    The pointer list names labels that are STACKED on shared rows, so
    several wrestlers legitimately get the same sounds. Slot 7 is Adam
    Bomb, whose pointer is a literal 0 -- he makes no noise because he is
    not in the game.
    """
    lines = _lines()
    i = _at(lines, "WHICH_DEBRIS_SOUND") + 1
    names: list[str] = []
    while i < len(lines) and len(names) < ROSTER_SLOTS:
        line = lines[i]
        i += 1
        if not line:
            continue
        m = LONG_RE.match(line)
        if not m:
            break
        names.append(m.group(1).strip())
    if len(names) != ROSTER_SLOTS:
        raise ValueError(f"WHICH_DEBRIS_SOUND has {len(names)} entries")

    out: list[list[int]] = []
    for name in names:
        if name == "0":
            out.append([])
            continue
        j = _at(lines, name) + 1
        vals: list[int] = []
        while j < len(lines):
            line = lines[j]
            j += 1
            if not line:
                continue
            if LABEL_RE.match(line):        # a second label on the same row
                continue
            m = WORD_RE.match(line)
            if not m:
                break
            vals += [wlcommands._value(v, {}) for v in m.group(1).split(",")]
            break
        if not vals:
            raise ValueError(f"{name}: no sound row")
        # `MOVE *A10+,A0 / JRZ / CALLA RNDRNG0` -- the first word is the
        # inclusive maximum, so there are max+1 sounds after it.
        want = vals[0] + 1
        if len(vals) - 1 != want:
            raise ValueError(f"{name}: header says {want} sounds, "
                             f"{len(vals) - 1} follow")
        out.append(vals[1:])
    return out


def anims() -> list[list[str]]:
    """debris_anims: eight sprite labels per wrestler."""
    lines = _lines()
    i = _at(lines, "debris_anims") + 1
    names: list[str] = []
    while i < len(lines) and len(names) < ROSTER_SLOTS:
        line = lines[i]
        i += 1
        if not line:
            continue
        m = LONG_RE.match(line)
        if not m:
            break
        names.append(m.group(1).strip())
    out: list[list[str]] = []
    for name in names:
        j = _at(lines, name) + 1
        row: list[str] = []
        while j < len(lines) and len(row) < DEBRIS_ANIMS_PER:
            line = lines[j]
            j += 1
            if not line:
                continue
            m = LONG_RE.match(line)
            if not m:
                break
            row.append(m.group(1).strip())
        if len(row) != DEBRIS_ANIMS_PER:
            raise ValueError(f"{name}: {len(row)} anims, "
                             f"not {DEBRIS_ANIMS_PER}")
        out.append(row)
    return out


def render_c() -> str:
    sh, sn, an = shapes(), sounds(), anims()
    out = ["/* Auto-generated by tools/wldebris.py from the original",
           "   SPECIAL.ASM debris tables -- do not edit. */",
           '#include "wm/arcade/wm_arcade_debris.h"',
           ""]
    out.append("/* SPECIAL.ASM:3542 #debris_table -- the shape of a burst. */")
    out.append("const wm_debris_shape wm_debris_shapes[WM_DEBRIS_SHAPES] = {")
    for i, r in enumerate(sh):
        out.append("    { /* #db%d */" % i)
        out.append("        %d, %d, %d," % (r["loop"], r["count"], r["sleep"]))
        out.append("        %d, %d, %d," % (r["rxoff"], r["ryoff"], r["rzoff"]))
        out.append("        %d, %d, %d," % (r["xvel"], r["yvel"], r["zvel"]))
        out.append("        %d, %d, %d," % (r["rxvel"], r["ryvel"], r["rzvel"]))
        out.append("        %d, %d, %d" % (r["gravity"], r["lifespan"],
                                           r["rlifespan"]))
        out.append("    },")
    out += ["};", ""]

    out.append("/* SPECIAL.ASM:3505 WHICH_DEBRIS_SOUND. Several wrestlers")
    out.append("   share a row -- the labels are stacked on it. */")
    for w, row in enumerate(sn):
        if not row:
            continue
        out.append("static const int16_t debris_snd_%d[] = { %s };"
                   % (w, ", ".join(str(v) for v in row)))
    out.append("")
    out.append("const wm_debris_sounds "
               "wm_debris_sound_table[WM_DEBRIS_WRESTLERS] = {")
    for w, row in enumerate(sn):
        if row:
            out.append("    { debris_snd_%d, %d }," % (w, len(row)))
        else:
            out.append("    { 0, 0 },   /* the cut wrestler */")
    out += ["};", ""]

    out.append("/* SPECIAL.ASM:3789 debris_anims -- eight sprites each. */")
    out.append("const char *const wm_debris_anims"
               "[WM_DEBRIS_WRESTLERS][WM_DEBRIS_ANIMS] = {")
    for row in an:
        out.append("    { " + ", ".join('"%s"' % n for n in row) + " },")
    out += ["};", ""]
    return "\n".join(out) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out")
    ap.add_argument("--list", action="store_true")
    ns = ap.parse_args()
    if ns.list:
        for i, r in enumerate(shapes()):
            print("#db%d %s" % (i, r))
        for w, row in enumerate(sounds()):
            print("wrestler %d sounds %s" % (w, [hex(v) for v in row]))
        return 0
    text = render_c()
    if ns.out:
        pathlib.Path(ns.out).write_text(text)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
