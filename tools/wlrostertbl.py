"""The global per-wrestler animation tables -- one animation per roster slot.

A lot of the game's shared behaviour is "everyone does this, but each in
his own animation", and the source spells that as a flat table indexed by
WRESTLERNUM:

    fall_back_tbl
        .long   hrt_fall_back_anim      ;0 Bret Hart
        .long   rzr_fall_back_anim      ;1 Razor Ramon
        ...
        .long   0                       ;7 spare
        .long   lex_fall_back_anim      ;8 Lex Luger

Slot 7 is Adam Bomb, cut from the game, and a tenth slot -- present in
some of these tables and not others -- is the Referee. Tables that carry
them sometimes hand both Doink's animation rather than nothing, so the
row count and the contents are both read rather than assumed.

tools/wlpuppet.py already extracts the tables that ANI_SLAVEANIM,
ANI_CHANGEANIM_TBL and friends name as command operands, resolving each
from its use site because those labels are `#local` and reused. These
are the other kind: plain globals reached by ordinary code
(`movi fall_back_tbl,a0`), which have exactly one definition each and so
need no use site to disambiguate. Anything defined more than once is
refused here rather than guessed at -- that is wlpuppet's job.
"""
from __future__ import annotations
import argparse
import collections
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim  # noqa: E402

ROSTER_SLOTS = 10           # 0..8 plus the Referee at 9

# Two macros index these tables, and they disagree about the row width.
#
#   FACETBL   (MACROS.H:102) scales WRESTLERNUM by X32 -- one long per
#             wrestler, so the row is just the slot.
#   FACE24TBL (MACROS.H:65)  scales by X64 -- two longs per wrestler --
#             and then adds 32 bits when MOVE_UP_BIT is *clear*. So
#             column 0 is the facing-up animation (the `_2_` name) and
#             column 1 the facing-down one (`_4_`).
#
# A two-long row is not automatically a facing pair, though. PROGRESS.ASM
# has six tables of the same shape -- taunting_addr, standing_addr and
# friends -- whose two longs are a leg animation and a torso animation,
# read by the loop at PROGRESS.ASM:3218 and handed to change_anim1a and
# change_anim2a in turn. Same stride, different meaning.
#
# So the column meaning is read off the use site rather than the shape:
# a table some caller passes to FACE24TBL is a facing pair, and a
# two-column table nobody indexes that way is left as an unnamed pair
# for its caller to interpret.
COL_SLOT = "slot"       # one long per wrestler
COL_FACING = "facing"   # [0] MOVE_UP set, [1] MOVE_UP clear
COL_PAIR = "pair"       # two longs whose meaning the use site decides

# The slot names, straight off the source's own end-of-line comments.
SLOT_NAMES = (
    "Bret Hart", "Razor Ramon", "Undertaker", "Yokozuna",
    "Shawn Michaels", "Bam Bam", "Doink", "Adam Bomb",
    "Lex Luger", "Referee",
)

LABEL_RE = re.compile(r"^\s*(?:SUBRP?\s+)?([A-Za-z_][A-Za-z0-9_]*):?\s*$")
LONG_RE = re.compile(r"^\s*\.long\s+(.+)$", re.I)
# REFLONG expands to `.globl label` + `.long label` (MACROS.H:45), so a
# REFLONG line is one table row that also exports the name.
REFLONG_RE = re.compile(r"^\s*REFLONG\s+([A-Za-z_][A-Za-z0-9_]*)\s*$", re.I)
# Declarations, not data. knee_hit_tbl (REACT3.ASM:223) puts eight `.ref`
# lines between its label and its rows; reading them as the end of the
# table loses the table entirely.
SKIP_RE = re.compile(r"^\s*\.(ref|globl|global|def|even|align)\b", re.I)

# A table has to be mostly animation labels to be one of these; the rest
# of the rows may be `0`, which is how a cut wrestler is spelled.
MIN_ANIM_ROWS = 5


def _blocks(path: pathlib.Path):
    """Every `label` followed by a run of .long rows, in one file."""
    lines = [wlanim.strip_comment(r)
             for r in path.read_text(errors="replace").splitlines()]
    i = 0
    while i < len(lines):
        m = LABEL_RE.match(lines[i]) if lines[i].strip() else None
        if not m:
            i += 1
            continue
        rows: list[str] = []
        j = i + 1
        while j < len(lines):
            if not lines[j].strip():
                j += 1
                continue
            if SKIP_RE.match(lines[j]):
                j += 1
                continue
            rm = REFLONG_RE.match(lines[j])
            if rm:
                rows.append(rm.group(1))
                j += 1
                continue
            lm = LONG_RE.match(lines[j])
            if not lm:
                break
            rows += [t.strip() for t in lm.group(1).split(",") if t.strip()]
            j += 1
        if rows:
            yield m.group(1), i + 1, rows
        i = max(j, i + 1)


def roster_tables() -> dict[str, tuple[str, int, list[str | None]]]:
    """{label: (file, line, rows)} for every unambiguous global table.

    Rows are padded to ROSTER_SLOTS with None so every table has the same
    shape in C; the real length is kept so a nine-row table is not
    silently claimed to name a Referee animation.
    """
    seen: dict[str, list[tuple[str, int, list[str]]]] = collections.defaultdict(list)
    for path in sorted(wlanim.ORIG.glob("*.ASM")):
        for name, line, rows in _blocks(path):
            if _facings(rows) is None:
                continue
            if sum(1 for r in rows if r.endswith("_anim")) < MIN_ANIM_ROWS:
                continue
            seen[name].append((path.name, line, rows))

    out: dict[str, tuple[str, int, list[str | None]]] = {}
    for name, defs in sorted(seen.items()):
        if len(defs) != 1:
            # Defined more than once: block-scoped, and only a use site
            # can say which one a given caller means. wlpuppet.py's
            # resolution handles those.
            continue
        fname, line, rows = defs[0]
        facings = _facings(rows)
        padded: list[str | None] = [None if r == "0" else r for r in rows]
        padded += [None] * (ROSTER_SLOTS * facings - len(padded))
        out[name] = (fname, line, padded)
    return out


# The macro may be preceded by a column-0 label, as at WRESTLE.ASM:3698
# (`#bnc\tFACE24TBL bncoff_gate`), so the label is optional here.
_FACE24_RE = re.compile(
    r"^\s*(?:[#A-Za-z_][A-Za-z0-9_]*:?\s+)?FACE24TBL\s+([A-Za-z_#][A-Za-z0-9_]*)",
    re.I)
_FACE24_CACHE: set[str] | None = None


def face24_operands() -> set[str]:
    """Every table name some caller indexes with the FACE24TBL macro."""
    global _FACE24_CACHE
    if _FACE24_CACHE is None:
        names: set[str] = set()
        for path in sorted(wlanim.ORIG.glob("*.ASM")):
            for raw in path.read_text(errors="replace").splitlines():
                m = _FACE24_RE.match(wlanim.strip_comment(raw))
                if m:
                    names.add(m.group(1).lstrip("#"))
        _FACE24_CACHE = names
    return _FACE24_CACHE


_PAIR_2_4_RE = re.compile(r"^(.*?)_2_(.*)$")


def _reads_as_facing(rows: list[str | None]) -> bool:
    """Do the two columns look like the game's own _2_/_4_ facing pair?

    Weaker evidence than a use site, and only consulted when there is no
    live one. head_hit_dizzy_tbl and body_hit_dizzy_tbl are reached only
    from FACE24TBL lines that are all commented out, so nothing in the
    shipped game indexes them at all -- but both write the *same* label
    in each column, which is what a facing pair looks like when the
    wrestler has one animation for both facings. A table is only called
    a facing pair here if every populated row is either that, or an
    exact `x_2_y` / `x_4_y` pair.
    """
    populated = 0
    for i in range(0, len(rows), 2):
        up, down = rows[i], rows[i + 1]
        if up is None and down is None:
            continue
        if up is None or down is None:
            return False
        populated += 1
        if up == down:
            continue
        m = _PAIR_2_4_RE.match(up)
        if not m or down != "%s_4_%s" % (m.group(1), m.group(2)):
            return False
    return populated > 0


def column_kind(name: str, facings: int,
                rows: list[str | None] | None = None) -> str:
    if facings == 1:
        return COL_SLOT
    if name in face24_operands():
        return COL_FACING
    if rows is not None and _reads_as_facing(rows):
        return COL_FACING
    return COL_PAIR


def _facings(rows: list[str]) -> int | None:
    """1, 2, or None when this run of rows is not a roster table at all."""
    if len(rows) in (9, ROSTER_SLOTS):
        return 1
    if len(rows) in (9 * 2, ROSTER_SLOTS * 2):
        return 2
    return None


def _ambiguous() -> dict[str, int]:
    seen: dict[str, int] = collections.Counter()
    for path in sorted(wlanim.ORIG.glob("*.ASM")):
        for name, _line, rows in _blocks(path):
            if _facings(rows) is not None and \
               sum(1 for r in rows if r.endswith("_anim")) >= MIN_ANIM_ROWS:
                seen[name] += 1
    return {k: v for k, v in seen.items() if v > 1}


def render_c() -> str:
    tables = roster_tables()
    out = [
        "/* Generated by tools/wlrostertbl.py. Do not edit.",
        " *",
        " * One animation per roster slot, indexed by WRESTLERNUM. Slot 7 is",
        " * Adam Bomb, cut from the game; slot 9 is the Referee and is only",
        " * present in the tables that declare ten rows. A NULL is a `.long 0`",
        " * in the source -- that slot has no animation at all.",
        " *",
        " * A row is one long wide (FACETBL, MACROS.H:102) or two",
        " * (FACE24TBL, MACROS.H:65). `kind` says what a second column",
        " * means: WM_ROSTER_COL_FACING is FACE24TBL's up/down pair, read",
        " * off a live use site or off an unambiguous _2_/_4_ naming;",
        " * WM_ROSTER_COL_PAIR is two longs whose meaning only the caller",
        " * knows, as in PROGRESS.ASM's leg/torso `*_addr` tables.",
        " */",
        '#include "wm/arcade/wm_arcade_roster_anims.h"',
        "",
    ]
    for name, (fname, line, rows) in tables.items():
        cols = len(rows) // ROSTER_SLOTS
        kind = column_kind(name, cols, rows)
        out.append("/* %s:%d -- %s */" % (fname, line, kind))
        out.append("static const char *const rows_%s[WM_ROSTER_ANIM_SLOTS * %d] = {"
                   % (name, cols))
        for i, r in enumerate(rows):
            cell = "NULL," if r is None else '"%s",' % r
            if cols == 1:
                note = "%d %s" % (i, SLOT_NAMES[i])
            else:
                note = "%d %s %s" % (i // cols, SLOT_NAMES[i // cols],
                                     _col_note(kind, i % cols))
            out.append('    %-32s /* %s */' % (cell, note))
        out.append("};")
    out.append("")
    out.append("const wm_roster_anim_table wm_roster_anim_tables[] = {")
    for name, (fname, line, rows) in tables.items():
        cols = len(rows) // ROSTER_SLOTS
        declared = ROSTER_SLOTS if _declared_ten(fname, line) else 9
        out.append('    { "%s", "%s", %d, %d, %d, WM_ROSTER_COL_%s, rows_%s },'
                   % (name, fname, line, declared, cols,
                      column_kind(name, cols, rows).upper(), name))
    out.append("};")
    out.append("")
    out.append("const int wm_roster_anim_table_count = %d;" % len(tables))
    out.append("")
    return "\n".join(out)


def _col_note(kind: str, col: int) -> str:
    if kind == COL_FACING:
        return "facing up" if col == 0 else "facing down"
    return "col %d" % col


_TEN_CACHE: dict[tuple[str, int], bool] = {}


def _declared_ten(fname: str, line: int) -> bool:
    """Did this table really write ten rows, or nine?

    A nine-row table has no Referee slot at all; a ten-row one may have
    written `0` there. The two are different statements and the emitted
    length says which.
    """
    key = (fname, line)
    if key not in _TEN_CACHE:
        path = wlanim.ORIG / fname
        for name, at, rows in _blocks(path):
            if at == line:
                _TEN_CACHE[key] = len(rows) in (ROSTER_SLOTS,
                                                ROSTER_SLOTS * 2)
                break
        else:
            _TEN_CACHE[key] = False
    return _TEN_CACHE[key]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out")
    ap.add_argument("--list", action="store_true")
    ns = ap.parse_args()
    tables = roster_tables()
    if ns.list:
        for name, (fname, line, rows) in tables.items():
            cols = len(rows) // ROSTER_SLOTS
            print("%-24s %-16s:%-5d %d col %-7s %d named, %d empty"
                  % (name, fname, line, cols, column_kind(name, cols, rows),
                     sum(1 for r in rows if r), sum(1 for r in rows if not r)))
        print("\n%d unambiguous global tables" % len(tables))
        amb = _ambiguous()
        print("%d labels defined more than once, left to tools/wlpuppet.py: %s"
              % (len(amb), ", ".join(sorted(amb))[:200]))
        return 0
    text = render_c()
    if ns.out:
        pathlib.Path(ns.out).write_text(text)
        print("roster animation tables: %d" % len(tables))
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
