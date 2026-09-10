#!/usr/bin/env python3
"""TABLES.ASM's target-offset grid -- where on a wrestler you can aim.

ANIM.ASM's ANI_TARGET and everything that calls set_target_offsets ask the
same question: given a wrestler, which of his own body parts is at what
offset right now. TABLES.ASM:43 answers it from one grid:

    a1 = WRESTLERNUM * 5 + target_area       ; 5 areas a wrestler
    a1 = a1 * 3 words                        ; x, y, z each
    a0 = #mode_table[PLYRMODE]               ; ...of the block for his mode
    TGT_XOFF/YOFF/ZOFF = the three words there

So it is [mode block][wrestler 0..8][area 0..4] -> (x, y, z), and the mode
table is 26 entries mapping a PLYRMODE onto one of only FOUR blocks --
most modes share the standing one, and PLYRMODE 17, 18, 22 and 23 have no
block of their own so the table points them at mode_normal too.

`get_mode_height` (TABLES.ASM:71) reads the same grid a different way:
`15n + 1` words into the mode's block, which is wrestler n's HEAD Y. It is
the same number as area 0's Y, and this checks that rather than assuming.
"""
from __future__ import annotations
import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim  # noqa: E402

SRC = wlanim.ORIG / "TABLES.ASM"

ROSTER_SLOTS = 9
AREAS = 5                 # HEAD, CHEST, GROIN, KNEES, FEET
MODE_SLOTS = 26           # #mode_table's own length

WORD_RE = re.compile(r"^\s*\.word\s+(.+)$", re.I)
LONG_RE = re.compile(r"^\s*\.long\s+([A-Za-z_#][\w#]*)\s*$", re.I)
LABEL_RE = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)\s*$")


def _lines() -> list[str]:
    return [wlanim.strip_comment(r)
            for r in SRC.read_text(errors="replace").splitlines()]


def mode_table() -> list[str]:
    """#mode_table: 26 PLYRMODE slots -> the block label each one uses."""
    lines = _lines()
    at = None
    for i, line in enumerate(lines):
        if wlanim.label_def(line) == "#mode_table":
            at = i
            break
    if at is None:
        raise ValueError("TABLES.ASM: no #mode_table")
    out: list[str] = []
    for j in range(at + 1, len(lines)):
        if not lines[j]:
            continue
        m = LONG_RE.match(lines[j])
        if not m:
            break
        out.append(m.group(1))
    if len(out) != MODE_SLOTS:
        raise ValueError(f"#mode_table has {len(out)} entries, not {MODE_SLOTS}")
    return out


def blocks() -> dict[str, list[list[tuple[int, int, int]]]]:
    """{block label: [wrestler][area] -> (x, y, z)}.

    Several labels sit on the same block -- mode_onground and mode_dead
    share one, with the source arguing about it in a comment ("This is for
    dead mode on the ground!?!?!?!") -- so every label that opens a block
    maps to the same rows.
    """
    lines = _lines()
    out: dict[str, list[list[tuple[int, int, int]]]] = {}
    i = 0
    while i < len(lines):
        m = LABEL_RE.match(lines[i]) if lines[i] else None
        if not m or not m.group(1).startswith("mode_"):
            i += 1
            continue
        # Every label stacked here opens the same block.
        names = []
        while i < len(lines):
            if not lines[i]:
                i += 1
                continue
            mm = LABEL_RE.match(lines[i])
            if mm and mm.group(1).startswith("mode_"):
                names.append(mm.group(1))
                i += 1
                continue
            break
        rows: list[tuple[int, int, int]] = []
        while i < len(lines) and len(rows) < ROSTER_SLOTS * AREAS:
            if not lines[i]:
                i += 1
                continue
            wm = WORD_RE.match(lines[i])
            if not wm:
                break
            vals = [int(v.strip()) for v in wm.group(1).split(",")]
            if len(vals) != 3:
                raise ValueError(f"TABLES.ASM:{i + 1}: {len(vals)} words, not 3")
            rows.append((vals[0], vals[1], vals[2]))
            i += 1
        if len(rows) != ROSTER_SLOTS * AREAS:
            raise ValueError(
                f"TABLES.ASM: block {names} has {len(rows)} rows, not "
                f"{ROSTER_SLOTS * AREAS}")
        grid = [rows[w * AREAS:(w + 1) * AREAS] for w in range(ROSTER_SLOTS)]
        for n in names:
            out[n] = grid
    return out


def render_c() -> str:
    table = mode_table()
    blks = blocks()
    missing = sorted(set(table) - set(blks))
    if missing:
        raise ValueError(f"#mode_table names blocks with no data: {missing}")

    # One array per DISTINCT block, and the mode table as indices into it.
    order: list[str] = []
    index_of: dict[int, int] = {}
    for name in table:
        rows = blks[name]
        for k, other in enumerate(order):
            if blks[other] == rows:
                index_of[id(rows)] = k
                break
        else:
            order.append(name)

    def block_index(name: str) -> int:
        rows = blks[name]
        for k, other in enumerate(order):
            if blks[other] == rows:
                return k
        raise ValueError(name)

    out = ["/* Auto-generated by tools/wltarget.py from the original",
           "   TABLES.ASM target-offset grid -- do not edit. */",
           '#include "wm/arcade/wm_arcade_target.h"',
           ""]
    out.append("/* TABLES.ASM's distinct blocks, [wrestler][area] -> x,y,z. */")
    out.append("static const wm_target_offset "
               "blocks[][WM_TARGET_WRESTLERS][WM_TARGET_AREAS] = {")
    for name in order:
        out.append("    { /* %s */" % name)
        for w, areas in enumerate(blks[name]):
            cells = ", ".join("{ %d, %d, %d }" % xyz for xyz in areas)
            out.append("        { %s }," % cells)
        out.append("    },")
    out += ["};", ""]

    out.append("/* TABLES.ASM:99 #mode_table, one entry per PLYRMODE. */")
    out.append("static const uint8_t mode_block[WM_TARGET_MODES] = {")
    for i, name in enumerate(table):
        out.append("    %d,   /* %2d %s */" % (block_index(name), i, name))
    out += ["};", ""]

    names = message_names()
    out.append("/* LIFEBAR.ASM:3490 #message_tbl -- MOVE_NAME_ANNC's own")
    out.append("   image per move index. \"\" is the source's `.long 0`. */")
    out.append("const char *const wm_move_name_images[WM_MOVE_NAME_COUNT] = {")
    for i, n in enumerate(names):
        out.append('    %-12s /* %2d */' % (('"%s",' % n) if n else "0,", i))
    out += ["};", ""]

    out += [
        "int wm_target_offsets(int32_t wrestler_num, int32_t player_mode,",
        "                      int area, wm_target_offset *out) {",
        "    if (wrestler_num < 0 || wrestler_num >= WM_TARGET_WRESTLERS)",
        "        return 0;",
        "    if (player_mode < 0 || player_mode >= WM_TARGET_MODES) return 0;",
        "    if (area < 0 || area >= WM_TARGET_AREAS) return 0;",
        "    if (out)",
        "        *out = blocks[mode_block[player_mode]][wrestler_num][area];",
        "    return 1;",
        "}",
        "",
        "size_t wm_target_block_count(void) {",
        "    return sizeof(blocks) / sizeof(blocks[0]);",
        "}",
        "",
    ]
    return "\n".join(out) + "\n"


# LIFEBAR.ASM:3490 #message_tbl -- the image MOVE_NAME_ANNC shows for each
# move index. A 0 row is an index with no name, which is the source's own
# placeholder ("0 = Blank") rather than a gap here.
MESSAGE_TBL_RE = re.compile(r"^\s*\.long\s+([A-Za-z_0-9]+)", re.I)
LIFEBAR = wlanim.ORIG / "LIFEBAR.ASM"


def message_names() -> list[str]:
    """#message_tbl: move index -> the name image, "" where there is none."""
    lines = LIFEBAR.read_text(errors="replace").splitlines()
    at = None
    for i, line in enumerate(lines):
        if line.startswith("#message_tbl"):
            at = i
            break
    if at is None:
        raise ValueError("LIFEBAR.ASM: no #message_tbl")
    out: list[str] = []
    for line in lines[at + 1:]:
        m = MESSAGE_TBL_RE.match(wlanim.strip_comment(line))
        if not m:
            if wlanim.strip_comment(line):
                break
            continue
        out.append("" if m.group(1) == "0" else m.group(1))
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out")
    ap.add_argument("--list", action="store_true")
    ns = ap.parse_args()
    if ns.list:
        for i, name in enumerate(mode_table()):
            print("%2d  %s" % (i, name))
        print()
        for name, rows in sorted(blocks().items()):
            print("%-20s head of Bret = %s" % (name, rows[0][0]))
        return 0
    text = render_c()
    if ns.out:
        pathlib.Path(ns.out).write_text(text)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
