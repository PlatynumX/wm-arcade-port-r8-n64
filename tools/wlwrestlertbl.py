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



# ---------------------------------------------------------------------
# The secret-move tables.

SMOVE_LIST = "#special_moves"
SMOVE_FILE = "WRESTLE2.ASM"

# `.if NUM_<WRESTLER>_FINISHES` wraps the finishing-move entries in every
# one of these tables, and GAME.EQU:580 sets seven of the eight to 0.
# Reading the table as plain text therefore reports moves the arcade
# never assembled -- which is exactly what the port's six hand-written
# lists did. Conditions are evaluated against the real equates and
# anything this cannot decide is refused rather than guessed.
IF_RE = re.compile(r"^\s*\.if\s+(.+?)\s*$", re.I)
ELSE_RE = re.compile(r"^\s*\.else\s*$", re.I)
ENDIF_RE = re.compile(r"^\s*\.endif\s*$", re.I)
SET_RE = re.compile(
    r"^([A-Za-z_][A-Za-z0-9_]*)\s+\.(?:set|equ)\s+(-?\d+)\s*$")
REFLONG_RE = re.compile(r"^\s*(?:REFLONG|\.long)\s+(.+)$", re.I)


_CONSTS: dict[str, int] | None = None


def _constants() -> dict[str, int]:
    """Every plain-integer equate the .EQU files define.

    GAME.EQU holds the NUM_*_FINISHES switches these tables turn on;
    SYS.EQU holds the board switches (DEBUG and friends) that other
    `.if`s in the same files test. A symbol two files define with
    DIFFERENT values is dropped rather than resolved by file order --
    an ambiguous condition must refuse, not pick.
    """
    global _CONSTS
    if _CONSTS is not None:
        return _CONSTS
    seen: dict[str, int] = {}
    clash: set[str] = set()
    for equ in sorted(wlanim.ORIG.glob("*.EQU")):
        for line in equ.read_text(errors="replace").splitlines():
            m = SET_RE.match(wlanim.strip_comment(line).strip())
            if not m:
                continue
            name, val = m.group(1), int(m.group(2))
            if name in seen and seen[name] != val:
                clash.add(name)
            seen[name] = val
    for name in clash:
        del seen[name]
    _CONSTS = seen
    return _CONSTS


def _cond(expr: str, consts: dict[str, int]) -> bool:
    """Evaluate a `.if` condition, or refuse.

    Only the two forms these tables actually use are accepted -- a bare
    symbol and `SYMBOL > n`. Anything else raises, because silently
    treating an unknown condition as true is how the finishing moves got
    into the port in the first place.
    """
    expr = expr.strip()
    m = re.fullmatch(r"([A-Za-z_][A-Za-z0-9_]*)", expr)
    if m:
        if m.group(1) not in consts:
            raise ValueError(f".if {expr}: not a constant this tool knows")
        return consts[m.group(1)] != 0
    m = re.fullmatch(r"([A-Za-z_][A-Za-z0-9_]*)\s*>\s*(-?\d+)", expr)
    if m:
        if m.group(1) not in consts:
            raise ValueError(f".if {expr}: not a constant this tool knows")
        return consts[m.group(1)] > int(m.group(2))
    if re.fullmatch(r"-?\d+", expr):
        return int(expr) != 0
    raise ValueError(f".if {expr}: this tool cannot decide it")


def assembled_lines(path: pathlib.Path) -> list[str]:
    """The file's lines with false `.if` branches blanked out.

    Blanked rather than removed so line numbers still line up with the
    source, which every error message in this tool quotes.
    """
    consts = _constants()
    out: list[str] = []
    stack: list[bool] = []
    for raw in _lines(path):
        m = IF_RE.match(raw)
        if m:
            stack.append(_cond(m.group(1), consts))
            out.append("")
            continue
        if ELSE_RE.match(raw):
            if stack:
                stack[-1] = not stack[-1]
            out.append("")
            continue
        if ENDIF_RE.match(raw):
            if stack:
                stack.pop()
            out.append("")
            continue
        out.append(raw if all(stack) else "")
    return out


def smove_dispatch() -> list[str | None]:
    """The nine table names WRESTLE2.ASM:4079 #special_moves names."""
    path = wlanim.ORIG / SMOVE_FILE
    lines = assembled_lines(path)
    at = next((i for i, l in enumerate(lines)
               if LOCAL_LABEL_RE.match(l)
               and LOCAL_LABEL_RE.match(l).group(1) == SMOVE_LIST), None)
    if at is None:
        raise ValueError(f"{SMOVE_FILE}: no {SMOVE_LIST}")
    out: list[str | None] = []
    for i in range(at + 1, len(lines)):
        if not lines[i].strip():
            continue
        m = REFLONG_RE.match(lines[i])
        if not m:
            break
        name = m.group(1).strip()
        out.append(None if name == "0" else name)
    # Nine slots: the eight playable wrestlers plus Adam Bomb's spare,
    # which is a plain 0 here rather than Doink's table.
    if len(out) != 9:
        raise ValueError(f"{SMOVE_LIST}: {len(out)} slots, expected 9")
    return out


def smove_rows(name: str) -> list[str]:
    """One wrestler's secret-move process list, 0-terminated in source."""
    path = _defining_file(name)
    lines = assembled_lines(path)
    at = next(i for i, line in enumerate(lines)
              if (m := wlanim.SUBR_RE.match(line)) and m.group(1) == name)
    out: list[str] = []
    for i in range(at + 1, len(lines)):
        if not lines[i].strip():
            continue
        m = REFLONG_RE.match(lines[i])
        if not m:
            break
        entry = m.group(1).strip()
        if entry == "0":
            return out              # WRESTLE2.ASM's loop stops on the 0
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", entry):
            raise ValueError(f"{name}: unreadable entry {entry!r}")
        out.append(entry)
    raise ValueError(f"{name}: ran off the end without a terminating 0")


def smove_tables() -> list[tuple[str, list[str]] | None]:
    """[(table name, process labels)] in roster-slot order, 9 slots."""
    return [None if n is None else (n, smove_rows(n))
            for n in smove_dispatch()]



# DOINK.ASM:1632 #taunt_t -- the taunt animation each wrestler plays at
# the start of a round. Shape and reader are the same as #special_moves
# (a local label with REFLONG rows), but it is an ANIMATION table like
# the three above, so it goes through wm_anim_program_find rather than
# naming a routine.
TAUNT_LIST = "#taunt_t"
TAUNT_FILE = "DOINK.ASM"


def _reflong_list(path: pathlib.Path, label: str,
                  slots: int) -> list[str | None]:
    """The `slots` REFLONG/.long rows following a local label."""
    lines = assembled_lines(path)
    at = next((i for i, l in enumerate(lines)
               if LOCAL_LABEL_RE.match(l)
               and LOCAL_LABEL_RE.match(l).group(1) == label), None)
    if at is None:
        raise ValueError(f"{path.name}: no {label}")
    out: list[str | None] = []
    for i in range(at + 1, len(lines)):
        if not lines[i].strip():
            continue
        m = REFLONG_RE.match(lines[i])
        if not m:
            break
        name = m.group(1).strip()
        out.append(None if name == "0" else name)
        if len(out) == slots:
            break
    if len(out) != slots:
        raise ValueError(f"{label}: {len(out)} slots, expected {slots}")
    return out


def taunt_anims() -> list[str | None]:
    """The ten taunt animations, in roster-slot order."""
    return _reflong_list(wlanim.ORIG / TAUNT_FILE, TAUNT_LIST, ROSTER_SLOTS)


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

    smoves = smove_tables()
    emitted_s: dict[str, str] = {}
    for slot, entry in enumerate(smoves):
        if entry is None:
            continue
        name, procs = entry
        if name in emitted_s:
            continue
        sym = f"tbl_{name}"
        emitted_s[name] = sym
        out.append(f"/* {name} -- {SLOT_NAMES[slot]}, {len(procs)} processes */")
        out.append(f"static const char *const {sym}[] = {{")
        for proc in procs:
            out.append(f'    "{proc}",')
        out.append("};")
        out.append("")

    out.append("const wm_wrestler_smove_table wm_wrestler_smoves"
               "[WM_WRESTLER_ANIM_SLOTS] = {")
    for slot in range(ROSTER_SLOTS):
        entry = smoves[slot] if slot < len(smoves) else None
        if entry is None:
            out.append(f"    {{ NULL, 0 }},   /* {slot} {SLOT_NAMES[slot]} */")
            continue
        name, procs = entry
        out.append(f"    {{ {emitted_s[name]}, {len(procs)} }},"
                   f"   /* {slot} {SLOT_NAMES[slot]} -- {name} */")
    out.append("};")
    out.append("")

    out.append("const char *const wm_wrestler_taunt_anims"
               "[WM_WRESTLER_ANIM_SLOTS] = {")
    for slot, lab in enumerate(taunt_anims()):
        if lab is None:
            out.append(f"    NULL,   /* {slot} {SLOT_NAMES[slot]} */")
        else:
            out.append(f'    "{lab}",   /* {slot} {SLOT_NAMES[slot]} */')
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
