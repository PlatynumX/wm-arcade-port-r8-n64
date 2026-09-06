#!/usr/bin/env python3
"""ANIM.ASM:2681 ANI_SUPERSLAVE2's puppet tables.

A grapple, slam, suplex or throw is one animation driving BOTH wrestlers.
The attacker's routine carries

    WWLLW ANI_SUPERSLAVE2,ticks,ATTACKER_FRAME,#puppet_tbl,index

and `_ani_superslave2` sets the attacker's own frame from that operand,
then looks the DEFENDER's frame up in `#puppet_tbl[defender WRESTLERNUM]`
at `index`. That is why the same throw shows a different victim pose for
each wrestler: it is one table per wrestler behind one shared routine.

Each per-wrestler table is a list of

    LWWW frame,xoff,yoff,flip

rows -- the defender's frame plus where to hang it relative to the
attacker, and whether it is mirrored. The offsets here are RAW: the
runtime adjusts them by both frames' own animation origins and widths (the
source's get_mpart_offsets / get_mpart_xsize), which this port already has
as real per-frame geometry.

`#puppet_tbl` is a local label, so it is scoped per sequence file and the
same name means different data in each -- 21 distinct tables across the
eight playable wrestlers, 4306 rows in all.
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim  # noqa: E402

SUPERSLAVE2_RE = re.compile(
    r"^\s*WWLLW\s+ANI_SUPERSLAVE2\s*,\s*([^,]+),\s*([A-Za-z0-9_]+)\s*\+\s*"
    r"FR(\d+)\s*,\s*(#?\w+)\s*,\s*(\d+)\s*$", re.I)
# The three operands are small arithmetic expressions, not plain integers:
# nine rows across the roster are written like `46-10` or `-41+5`. Requiring
# a bare number silently truncated the list at the first such row, and since
# seven of the nine are the FIRST row of a per-wrestler list, that emptied
# seven whole wrestlers out of one of Bret's grapple tables.
LWWW_RE = re.compile(
    r"^\s*LWWW\s+([A-Za-z0-9_]+)\s*\+\s*FR(\d+)\s*,\s*([-+0-9 ]+)\s*,\s*"
    r"([-+0-9 ]+)\s*,\s*([-+0-9 ]+)\s*$", re.I)
LWWW_ANY_RE = re.compile(r"^\s*LWWW\s+\S", re.I)
REF_RE = re.compile(r"^\s*\.ref\b", re.I)


def _num(expr: str) -> int:
    return int(eval(expr.strip(), {"__builtins__": {}}, {}))
LONG_RE = re.compile(r"^\s*\.long\s+(#?[A-Za-z_][A-Za-z0-9_]*|0)\s*$", re.I)

# The .long list is in WRESTLERNUM order (GAME.EQU W_*), nine slots with
# Adam Bomb's spare seventh among them.
ROSTER_SLOTS = 9

_ORIG = pathlib.Path(__file__).resolve().parents[1] / "original" / "wwf-wrestlemania"


def canonical_files() -> list[pathlib.Path]:
    """Every sequence file, in one fixed order.

    Table ids are positions in this list's tables, and tools/wlprogram.py
    has to agree with the generated data about them -- so the order comes
    from here rather than from whatever either was invoked with.
    """
    return sorted(q for q in _ORIG.glob("*SEQ*.ASM") if "'" not in q.name)


def _frame(name: str, n: str) -> str:
    return "%s%02d" % (name.upper(), int(n))


def _label_lines(lines: list[str], label: str) -> list[int]:
    return [i for i, l in enumerate(lines) if wlanim.label_def(l) == label]


def _rows_from(lines: list[str], start: int) -> list[tuple]:
    out = []
    for i in range(start + 1, len(lines)):
        m = LWWW_RE.match(lines[i])
        if m:
            out.append((_frame(m.group(1), m.group(2)),
                        _num(m.group(3)), _num(m.group(4)), _num(m.group(5))))
            continue
        if not lines[i]:
            continue
        # A row that looks like one but does not parse is a gap in this
        # reader, not the end of the list -- say so rather than quietly
        # returning a short table.
        if LWWW_ANY_RE.match(lines[i]):
            raise ValueError("unparsed LWWW row: %r" % lines[i])
        # `.ref` declares the frame symbol the row about to follow uses --
        # DNKSEQ3.ASM writes one before every entry. It is a declaration,
        # not data, so it does not end the list; treating it as the end gave
        # seven of the nine slots no rows at all.
        if REF_RE.match(lines[i]):
            continue
        # A label starts the next wrestler's list; anything else ends this
        # table.
        break
    return out


def _slots_at(lines: list[str], at: int, where: str) -> list[list[tuple]]:
    """The nine per-wrestler row lists of the table defined at line `at`."""
    slots: list[list[tuple]] = []
    i = at + 1
    while len(slots) < ROSTER_SLOTS and i < len(lines):
        if not lines[i]:
            i += 1
            continue
        m = LONG_RE.match(lines[i])
        if not m:
            break
        target = m.group(1)
        if target == "0":
            slots.append([])
        else:
            spots = _label_lines(lines, target)
            # A slot label is local too, so take the nearest one AFTER the
            # table, for the same reason the table itself resolves forward.
            spot = next((s for s in spots if s > at), None)
            slots.append(_rows_from(lines, spot) if spot is not None else [])
        i += 1
    if len(slots) != ROSTER_SLOTS:
        raise ValueError(
            f"{where}: table has {len(slots)} slots, not {ROSTER_SLOTS} -- "
            f"the .long list is not a roster table")
    return slots


def tables_in(path: pathlib.Path) -> dict[int, list[list[tuple]]]:
    """{definition line: nine per-wrestler row lists} for one file.

    Keyed on the LINE, not the label. `#puppet_tbl` is a local label and the
    files reuse it freely -- HRTSEQ3.ASM alone defines it eight times -- so
    the name identifies nothing on its own. Resolving by name would have
    given every grapple in that file Bret's first table.
    """
    lines = [wlanim.strip_comment(r)
             for r in path.read_text(errors="replace").splitlines()]
    out: dict[int, list[list[tuple]]] = {}
    for i, line in enumerate(lines):
        m = SUPERSLAVE2_RE.match(line)
        if not m:
            continue
        at = table_line_for(lines, i, m.group(4), path.name)
        if at not in out:
            out[at] = _slots_at(lines, at, "%s:%d" % (path.name, at + 1))
    return out


def table_line_for(lines: list[str], use_line: int, label: str,
                   where: str = "") -> int:
    """Which definition of `label` the use at `use_line` resolves to.

    The sequence files place a puppet table AFTER the routines that use it
    -- HRTSEQ3.ASM's uses at 332-347 and 484-501 both belong to the table at
    594, and 900-905 to the one at 937 -- so resolution runs forward.
    """
    spots = _label_lines(lines, label)
    at = next((s for s in spots if s > use_line), None)
    if at is None:
        at = next((s for s in reversed(spots) if s < use_line), None)
    if at is None:
        raise ValueError(f"{where}:{use_line + 1}: no {label} for an "
                         f"ANI_SUPERSLAVE2")
    return at


def render_c(paths: list[pathlib.Path] | None = None) -> str:
    paths = paths if paths is not None else canonical_files()
    out = ["/* Auto-generated by tools/wlpuppet.py: ANIM.ASM:2681",
           "   ANI_SUPERSLAVE2's per-wrestler defender-frame tables. */",
           '#include "wm/anim_puppet.h"',
           ""]
    ids: list[tuple[str, str]] = []
    bodies: list[str] = []
    for path in paths:
        for at, slots in sorted(tables_in(path).items()):
            sym = "pup_%s_%d" % (re.sub(r"\W", "_", path.stem.lower()), at)
            for w, rows in enumerate(slots):
                if not rows:
                    continue
                bodies.append("static const wm_anim_puppet_row %s_w%d[] = {"
                              % (sym, w))
                for f, x, y, fl in rows:
                    bodies.append('    {"%s", %d, %d, %d},' % (f, x, y, fl))
                bodies += ["};", ""]
            bodies.append("static const wm_anim_puppet_slot %s[] = {" % sym)
            for w, rows in enumerate(slots):
                if rows:
                    bodies.append("    { %s_w%d, %d }," % (sym, w, len(rows)))
                else:
                    bodies.append("    { 0, 0 },")
            bodies += ["};", ""]
            ids.append((("%s:%d" % (path.name, at)), sym))
    out += bodies
    out.append("static const wm_anim_puppet_table tables[] = {")
    for key, sym in ids:
        out.append('    { "%s", %s },' % (key, sym))
    out += ["};", "",
            "const wm_anim_puppet_table *wm_anim_puppet_table_at(size_t id) {",
            "    if (id >= sizeof(tables)/sizeof(tables[0])) return 0;",
            "    return &tables[id];",
            "}",
            "",
            "size_t wm_anim_puppet_table_count(void) {",
            "    return sizeof(tables)/sizeof(tables[0]);",
            "}",
            ""]
    return "\n".join(out)


_ID_CACHE: dict[str, int] | None = None


def table_ids(paths: list[pathlib.Path] | None = None) -> dict[str, int]:
    """{"FILE.ASM:defline": id}, in the order render_c emits."""
    global _ID_CACHE
    if paths is None and _ID_CACHE is not None:
        return _ID_CACHE
    use = paths if paths is not None else canonical_files()
    ids: dict[str, int] = {}
    for path in use:
        for at in sorted(tables_in(path)):
            ids["%s:%d" % (path.name, at)] = len(ids)
    if paths is None:
        _ID_CACHE = ids
    return ids


def table_id_for(path: pathlib.Path, use_line: int, label: str) -> int:
    """The id of the table the ANI_SUPERSLAVE2 at `use_line` resolves to."""
    lines = [wlanim.strip_comment(r)
             for r in path.read_text(errors="replace").splitlines()]
    at = table_line_for(lines, use_line, label, path.name)
    key = "%s:%d" % (path.name, at)
    ids = table_ids()
    if key not in ids:
        raise ValueError(f"no generated puppet table for {key}")
    return ids[key]


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", action="append", default=[], type=pathlib.Path)
    ap.add_argument("--out")
    ns = ap.parse_args(argv)
    paths = [p for p in ns.source if p.exists()] or None
    text = render_c(paths)
    if ns.out:
        pathlib.Path(ns.out).write_text(text)
        n = len(table_ids(paths))
        print(f"wrote {n} ANI_SUPERSLAVE2 puppet tables -> {ns.out}")
        return 0
    for key, i in table_ids(paths).items():
        print("%3d  %s" % (i, key))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
