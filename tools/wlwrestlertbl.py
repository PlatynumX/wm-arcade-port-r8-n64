"""Each wrestler's own walk, turn and torso animation tables.

tools/wlrostertbl.py reads the GLOBAL tables -- one animation per roster
slot, indexed by WRESTLERNUM, the "everyone does this, each in his own
animation" shape. These are the other half: three tables that belong to
one wrestler and are indexed by where he is facing and where he is
going. WRESTLE.ASM reaches them through a per-wrestler dispatch list, so
the roster order here is read from that list rather than assumed:

    #wres_torso_anims   WRESTLE.ASM:5031
    #wres_leg_anims     WRESTLE.ASM:5043
    #wres_rotate_anims  WRESTLE.ASM:5100

WRESTLE.ASM:4946 change_walk_anim drives the first two every tick a
wrestler walks:

    torso  [convert_facing(FACING_DIR) >> 1][convert_facing(NEW_FACING_DIR) >> 1]
           -- 4x4, diagonals only, played through change_anim2 (the
           SECOND animation channel) and skipped entirely when ANIMODE2
           already has MODE_UNINT set.

    legs   [convert_facing(MOVE_DIR)][convert_facing(FACING_DIR)]
           -- 8x8, full compass, played through change_anim1.

and WRESTLE.ASM:5062 set_rotate_anim drives the third from the idle
(#zip) path, where a wrestler turns on the spot:

    rotate [old facing >> 1][new facing >> 1]  -- 4x4, change_anim1.

The rotate and torso tables are the same 4x4 turn matrix twice over: the
diagonal is the standing/torso pose for that facing, and every other
entry is the turn animation from one diagonal to another. Torso's are
the `2` variants -- hrt_2_to_4_turn2_anim beside hrt_2_to_4_turn_anim.

Bret's and Razor's three tables were already in the port, transcribed by
hand into src/core/arcade/wm_arcade_{bret,razor}_tables.c. Those two are
extracted here as well rather than skipped, so the generated data and
the hand-written data can be compared -- which is the only way to find
out whether either was ever right.

A slot may legitimately be empty: WRESTLE.ASM:5109 gives the Referee `0`
for rotate, and slot 7 is Adam Bomb, cut from the game. An empty slot is
emitted as a null row rather than filled in.
"""
from __future__ import annotations
import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim  # noqa: E402

# The three dispatch lists, and the shape each table they name has.
#
# Shapes are NOT a guess: the index arithmetic in change_walk_anim and
# set_rotate_anim fixes them. `X4 a0` after `srl 1` is a 4-wide row of
# LONGs over four diagonals; `X8 a0` with no shift is an 8-wide row over
# the full compass. A table whose row count disagrees is refused.
KINDS = {
    "torso":  ("#wres_torso_anims",  4, 4),
    "leg":    ("#wres_leg_anims",    8, 8),
    "rotate": ("#wres_rotate_anims", 4, 4),
}

DISPATCH_FILE = "WRESTLE.ASM"
ROSTER_SLOTS = 10           # 0..8 plus the Referee at 9

SLOT_NAMES = (
    "Bret Hart", "Razor Ramon", "Undertaker", "Yokozuna",
    "Shawn Michaels", "Bam Bam", "Doink", "Adam Bomb",
    "Lex Luger", "Referee",
)

LOCAL_LABEL_RE = re.compile(r"^\s*(#[A-Za-z0-9_]+)\s*$")
LONG_RE = re.compile(r"^\s*\.long\s+(.+)$", re.I)


def _lines(path: pathlib.Path) -> list[str]:
    return [wlanim.strip_comment(r)
            for r in path.read_text(errors="replace").splitlines()]


def dispatch(kind: str) -> list[str | None]:
    """The ten table names one `#wres_*_anims` list names, in slot order.

    Read from the list rather than built from a prefix per wrestler,
    because the list is not a clean mapping: slot 7 (Adam Bomb, cut)
    carries Doink's table rather than nothing, and the Referee's rotate
    slot is a plain 0.
    """
    label = KINDS[kind][0]
    path = wlanim.ORIG / DISPATCH_FILE
    lines = _lines(path)
    at = None
    for i, line in enumerate(lines):
        m = LOCAL_LABEL_RE.match(line)
        if m and m.group(1) == label:
            at = i
            break
    if at is None:
        raise ValueError(f"{DISPATCH_FILE}: no {label}")

    out: list[str | None] = []
    for i in range(at + 1, len(lines)):
        if not lines[i].strip():
            continue
        m = LONG_RE.match(lines[i])
        if not m:
            break
        name = m.group(1).strip()
        out.append(None if name == "0" else name)
        if len(out) == ROSTER_SLOTS:
            break
    if len(out) != ROSTER_SLOTS:
        raise ValueError(f"{label}: {len(out)} slots, expected {ROSTER_SLOTS}")
    return out


def _defining_file(name: str) -> pathlib.Path:
    """The one LINKED file that defines this table.

    It matters: DNK.ASM and DOINK.ASM both define dnk_rotate_anims_table
    and TEMPLATE.ASM defines a copy of bam_rotate_anims_table, but only
    DOINK.ASM is in WRESTLE.CMD. Reading an unlinked file would put data
    the game never assembled into the port.
    """
    hits = []
    for path in wlanim.linked_files():
        for line in _lines(path):
            m = wlanim.SUBR_RE.match(line)
            if m and m.group(1) == name:
                hits.append(path)
                break
    if not hits:
        raise ValueError(f"{name}: not defined in any linked file")
    if len(hits) > 1:
        raise ValueError(f"{name}: defined in {[p.name for p in hits]}")
    return hits[0]


def rows_of(name: str, rows: int, cols: int) -> list[list[str]]:
    """The table's `rows`x`cols` animation labels."""
    path = _defining_file(name)
    lines = _lines(path)
    at = next(i for i, line in enumerate(lines)
              if (m := wlanim.SUBR_RE.match(line)) and m.group(1) == name)

    flat: list[str] = []
    for i in range(at + 1, len(lines)):
        if not lines[i].strip():
            continue
        m = LONG_RE.match(lines[i])
        if not m:
            break
        entry = m.group(1).strip()
        flat.append(entry)
        if len(flat) == rows * cols:
            break
    if len(flat) != rows * cols:
        raise ValueError(
            f"{name} ({path.name}): {len(flat)} entries, expected "
            f"{rows * cols} -- the index arithmetic in WRESTLE.ASM says "
            f"{rows}x{cols}, so a short table is a misread, not a variant")
    # Every entry has to be an animation this port can look up. A `0` here
    # would be a hole the walk code would walk straight into.
    for entry in flat:
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", entry):
            raise ValueError(f"{name}: unreadable entry {entry!r}")
    return [flat[r * cols:(r + 1) * cols] for r in range(rows)]


def tables() -> dict[str, list[tuple[str, list[list[str]]] | None]]:
    """{kind: [ (table name, rows) or None, ... ]} in roster-slot order."""
    out: dict[str, list[tuple[str, list[list[str]]] | None]] = {}
    for kind, (_label, rows, cols) in KINDS.items():
        slots: list[tuple[str, list[list[str]]] | None] = []
        for name in dispatch(kind):
            slots.append(None if name is None
                         else (name, rows_of(name, rows, cols)))
        out[kind] = slots
    return out


def render_c() -> str:
    data = tables()
    out = ["/* Auto-generated by tools/wlwrestlertbl.py from the original",
           "   Midway source. Each wrestler's own turn, walk and torso",
           "   animation tables, indexed as WRESTLE.ASM indexes them.",
           "",
           "   Do not edit: run scripts/regenerate_source_data.sh. */",
           '#include "wm/wrestler_anim_tables.h"', "", "#include <stddef.h>",
           ""]

    for kind in ("rotate", "torso", "leg"):
        _label, rows, cols = KINDS[kind]
        emitted: dict[str, str] = {}
        for slot, entry in enumerate(data[kind]):
            if entry is None:
                continue
            name, grid = entry
            if name in emitted:
                continue        # slot 7 shares Doink's table
            sym = f"tbl_{name}"
            emitted[name] = sym
            out.append(f"/* {name} -- {SLOT_NAMES[slot]} */")
            out.append(f"static const char *const {sym}[{rows}][{cols}] = {{")
            for row in grid:
                out.append("    { " + ", ".join(f'"{c}"' for c in row) + " },")
            out.append("};")
            out.append("")

        out.append(f"const wm_wrestler_anim_table wm_wrestler_{kind}_anims"
                   f"[WM_WRESTLER_ANIM_SLOTS] = {{")
        for slot, entry in enumerate(data[kind]):
            if entry is None:
                out.append(f"    {{ NULL, 0, 0 }},"
                           f"   /* {slot} {SLOT_NAMES[slot]} */")
                continue
            name, _grid = entry
            out.append(f"    {{ (const char *const *){emitted[name]}, "
                       f"{rows}, {cols} }},   /* {slot} {SLOT_NAMES[slot]}"
                       f" -- {name} */")
        out.append("};")
        out.append("")

    out += ["const char *wm_wrestler_anim_label("
            "const wm_wrestler_anim_table *table, int row, int col) {",
            "    if (!table || !table->labels) return NULL;",
            "    if (row < 0 || row >= table->rows) return NULL;",
            "    if (col < 0 || col >= table->cols) return NULL;",
            "    return table->labels[(size_t)row * (size_t)table->cols "
            "+ (size_t)col];",
            "}",
            ""]
    return "\n".join(out)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out")
    ns = ap.parse_args(argv)
    text = render_c()
    if ns.out:
        pathlib.Path(ns.out).write_text(text)
        n = sum(1 for k in KINDS for e in tables()[k] if e)
        print(f"wrestler anim tables: {n} tables across "
              f"{len(KINDS)} kinds -> {ns.out}")
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
