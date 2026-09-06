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

# ANIM.ASM:2130 ANI_SLAVEANIM's tables are the same shape but hold whole
# ANIMATIONS rather than frames: nine `.long` entries, one per wrestler, each
# the label of the routine the VICTIM is to start playing. That is how a slam
# makes the victim run his own landing animation instead of being posed.
# Entries are sometimes written several to a line.
SLAVEANIM_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_SLAVEANIM\s*,\s*(#?\w+)\s*$", re.I)
LONG_LIST_RE = re.compile(r"^\s*(?:\.long|REFLONG)\s+(.+)$", re.I)

# The .long list is in WRESTLERNUM order (GAME.EQU W_*), nine slots with
# Adam Bomb's spare seventh among them.
ROSTER_SLOTS = 9

_ORIG = pathlib.Path(__file__).resolve().parents[1] / "original" / "wwf-wrestlemania"


def canonical_files() -> list[pathlib.Path]:
    """Every sequence file, in one fixed order.

    Table ids are positions in this list's tables, and tools/wlprogram.py
    has to agree with the generated data about them -- so the order comes
    from here rather than from whatever either was invoked with.

    Only the files WRESTLE.CMD links count: ADMSEQ1-3.ASM, REFSEQ1.ASM and
    the superseded HRTSEQ.ASM/YOKSEQ.ASM are in the drop but not in the
    game, and their tables are not the game's tables.
    """
    linked = {q.name for q in wlanim.linked_files()}
    return sorted(q for q in _ORIG.glob("*SEQ*.ASM")
                  if "'" not in q.name and q.name in linked)


def _frame(name: str, n: str) -> str:
    return "%s%02d" % (name.upper(), int(n))


def _shared_table_files() -> list[pathlib.Path]:
    """Where a table reached by `.ref` might live, in a fixed order."""
    named = [_ORIG / n for n in ("ANIM.ASM", "WRESTLE.ASM", "WRESTLE2.ASM",
                                 "REACT1.ASM", "REACT2.ASM")]
    rest = sorted(wlanim.linked_files())
    seen, out = set(), []
    for p in [q for q in named if q.exists()] + rest:
        if p.name not in seen:
            seen.add(p.name)
            out.append(p)
    return out


def _subr_lines(lines: list[str], label: str) -> list[int]:
    """A table can live behind a SUBR, and behind a SUBRP with a `#` name --
    BAMSEQ2.ASM's own `SUBRP #release_table`. SUBR_RE captures the name
    without the sigil, so the sigil comes off the wanted name too."""
    want = label.lstrip("#")
    out = []
    for i, l in enumerate(lines):
        m = wlanim.SUBR_RE.match(l)
        if m and m.group(1) == want:
            out.append(i)
    return out


def _table_line(lines: list[str], use_line: int, label: str, where: str) -> int:
    """Where `label`'s table body starts -- a local label, a file-scope
    label, or a SUBR (a shared table lives behind one)."""
    spots = _label_lines(lines, label) + _subr_lines(lines, label)
    spots.sort()
    if use_line < 0:
        if not spots:
            raise ValueError(f"{where}: no {label}")
        return spots[0]
    at = next((s for s in spots if s > use_line), None)
    if at is None:
        at = next((s for s in reversed(spots) if s < use_line), None)
    if at is None:
        raise ValueError(f"{where}:{use_line + 1}: no {label}")
    return at


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


def slave_key_for(path: pathlib.Path, lines: list[str], use_line: int,
                  label: str):
    """(key, home file, home lines, definition line) for one ANI_SLAVEANIM.

    A table may live in this file, or -- when it is a global reached by
    `.ref` -- in another. ANIM.ASM:4590's slaveanim_tbl is the one every
    wrestler's slam points at, and the fall-back tables are in REACT1.ASM.
    Cross-file keys are negative so they cannot collide with in-file ones.
    """
    home, home_lines = path, lines
    if (not label.startswith("#") and not _label_lines(lines, label)
            and not _subr_lines(lines, label)):
        found = None
        for cand in _shared_table_files():
            cl = [wlanim.strip_comment(r)
                  for r in cand.read_text(errors="replace").splitlines()]
            if _label_lines(cl, label) or _subr_lines(cl, label):
                found = (cand, cl)
                break
        if not found:
            raise ValueError(f"{path.name}: no {label} in any source file")
        home, home_lines = found
    at = _table_line(home_lines, use_line if home is path else -1, label,
                     home.name)
    key = "%s:%d" % (home.name, at) if home is path else "@%s:%d" % (home.name, at)
    return key, home, home_lines, at


CHANGEANIM_TBL_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_CHANGEANIM_TBL\s*,\s*(#?\w+)\s*$", re.I)
# ANIM.ASM:109 _ani_xflip_tbl and :82 _ani_oppoffset both index a nine-slot
# per-wrestler table of WORDs -- one value each for the flip flag, two (x
# and y) for the release offset.
XFLIP_TBL_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_XFLIP_TBL\s*,\s*(#?\w+)\s*$", re.I)
OPPOFFSET_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_OPPOFFSET\s*,\s*(#?\w+)\s*$", re.I)
WORD_LIST_RE = re.compile(r"^\s*\.word\s+(.+)$", re.I)


def word_tables_in(path: pathlib.Path, use_re, per_row: int
                   ) -> dict[str, list[tuple]]:
    """{definition line: nine per-wrestler rows of `per_row` WORDs}.

    Same resolution as the slave tables -- per use site, forward, across
    files -- because these labels are `#local` and reused just as freely
    (`#xflip_tbl` and `#release_table` both appear in several files).
    """
    lines = [wlanim.strip_comment(r)
             for r in path.read_text(errors="replace").splitlines()]
    out: dict[str, list[tuple]] = {}
    for i, line in enumerate(lines):
        m = use_re.match(line)
        if not m:
            continue
        key, home, at_lines, at = slave_key_for(path, lines, i, m.group(1))
        if key in out:
            continue
        vals: list[int] = []
        j = at + 1
        while len(vals) < ROSTER_SLOTS * per_row and j < len(at_lines):
            if not at_lines[j]:
                j += 1
                continue
            if REF_RE.match(at_lines[j]):
                j += 1
                continue
            wm = WORD_LIST_RE.match(at_lines[j])
            if not wm:
                if not vals and wlanim.label_def(at_lines[j]):
                    j += 1
                    continue
                break
            for part in wm.group(1).split(","):
                part = part.strip()
                if part:
                    vals.append(_num(part))
            j += 1
        want = ROSTER_SLOTS * per_row
        if len(vals) < want:
            raise ValueError(
                f"{home.name}:{at + 1}: word table has {len(vals)} values, "
                f"not {want}")
        out[key] = [tuple(vals[k * per_row:(k + 1) * per_row])
                    for k in range(ROSTER_SLOTS)]
    return out


def slave_tables_in(path: pathlib.Path, use_re=None) -> dict[str, list[str]]:
    """{definition line: nine victim animation labels} for one file.

    Keyed on the line for the same reason the puppet tables are: the label
    may be a `#local` the file reuses.
    """
    lines = [wlanim.strip_comment(r)
             for r in path.read_text(errors="replace").splitlines()]
    out: dict[str, list[str]] = {}
    for i, line in enumerate(lines):
        m = (use_re or SLAVEANIM_RE).match(line)
        if not m:
            continue
        key, home, at_lines, at = slave_key_for(path, lines, i, m.group(1))
        if key in out:
            continue
        names: list[str] = []
        j = at + 1
        while len(names) < ROSTER_SLOTS and j < len(at_lines):
            if not at_lines[j]:
                j += 1
                continue
            # `.ref` declares the symbols the rows below use -- the same
            # habit the puppet tables have, and the same trap: treating one
            # as the end of the list gives an empty table.
            if REF_RE.match(at_lines[j]):
                j += 1
                continue
            lm = LONG_LIST_RE.match(at_lines[j])
            if not lm:
                # A second label on the line right after the first is an
                # alias for the same table -- RZRSEQ3.ASM writes
                # `#flyout_tbl` and `flyout_tbl` back to back so the data
                # can be reached from inside the file and from outside it.
                if not names and wlanim.label_def(at_lines[j]):
                    j += 1
                    continue
                break
            for part in lm.group(1).split(","):
                part = part.strip()
                if not part:
                    continue
                names.append("" if part == "0" else part)
            j += 1
        if len(names) < ROSTER_SLOTS:
            raise ValueError(
                f"{home.name}:{at + 1}: slave table has {len(names)} entries, "
                f"not {ROSTER_SLOTS}")
        out[key] = names[:ROSTER_SLOTS]
    return out


def changeanim_targets() -> list[tuple[pathlib.Path, str]]:
    """Every animation an ANI_CHANGEANIM_TBL row can hand off to.

    Same principle as slave_targets: the op names a whole animation, so
    naming one the port cannot play would be a hole. Read out of the
    tables rather than listed.
    """
    wanted: list[tuple[str, str]] = []
    for path in canonical_files():
        for key, rows in sorted(_aux_tables_in(path, "changeanim").items()):
            home = key.split(":")[0].lstrip("@")
            for name in rows:
                if name and (name, home) not in wanted:
                    wanted.append((name, home))
    subr_in: dict[str, pathlib.Path] = {}
    for q in wlanim.linked_files():
        for line in q.read_text(errors="replace").splitlines():
            m = wlanim.SUBR_RE.match(line)
            if m:
                subr_in.setdefault(m.group(1), q)
    out, seen = [], set()
    for name, home in wanted:
        if name in seen:
            continue
        seen.add(name)
        out.append((subr_in.get(name, _ORIG / home), name))
    return out


def slave_targets() -> list[tuple[pathlib.Path, str]]:
    """Every animation an ANI_SLAVEANIM table can name, with its source file.

    ANI_SLAVEANIM hands the victim a whole animation of his own to run, so
    each of these is a program the port has to be able to play -- the op
    names them, and naming an animation nothing can run is a hole. The set
    is read out of the tables rather than listed by hand, so it cannot
    drift from them.

    A label is resolved to the file that defines it: a SUBR is a global,
    so it may live in any linked file; a bare column-0 label is file-local
    (UNDSEQ3.ASM's eight `*_choking_anim`), so it resolves in the file that
    holds the table naming it.
    """
    wanted: list[tuple[str, str]] = []   # (label, file holding the table)
    for path in canonical_files():
        for key, rows in sorted(slave_tables_in(path).items()):
            home = key.split(":")[0].lstrip("@")
            for name in rows:
                if name and (name, home) not in wanted:
                    wanted.append((name, home))

    subr_in: dict[str, pathlib.Path] = {}
    for q in wlanim.linked_files():
        for line in q.read_text(errors="replace").splitlines():
            m = wlanim.SUBR_RE.match(line)
            if m:
                subr_in.setdefault(m.group(1), q)

    out: list[tuple[pathlib.Path, str]] = []
    seen: set[str] = set()
    for name, home in wanted:
        if name in seen:
            continue
        where = subr_in.get(name, _ORIG / home)
        seen.add(name)
        out.append((where, name))
    return out


def slave_table_ids(paths: list[pathlib.Path] | None = None) -> dict[str, int]:
    use = paths if paths is not None else canonical_files()
    ids: dict[str, int] = {}
    for path in use:
        for key in sorted(slave_tables_in(path)):
            if key not in ids:
                ids[key] = len(ids)
    return ids


_SLAVE_ID_CACHE: dict[str, int] | None = None


def slave_table_id_for(path: pathlib.Path, use_line: int, label: str) -> int:
    global _SLAVE_ID_CACHE
    if _SLAVE_ID_CACHE is None:
        _SLAVE_ID_CACHE = slave_table_ids()
    lines = [wlanim.strip_comment(r)
             for r in path.read_text(errors="replace").splitlines()]
    key, _home, _hl, _at = slave_key_for(path, lines, use_line, label)
    if key not in _SLAVE_ID_CACHE:
        raise ValueError(f"no generated slave table for {key}")
    return _SLAVE_ID_CACHE[key]


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
    # ANI_SLAVEANIM's tables: nine victim ANIMATION labels apiece.
    slave_syms = []
    for path in paths:
        for key, names in sorted(slave_tables_in(path).items()):
            sym = "slv_%d" % len(slave_syms)
            out.append("static const char *const %s[] = {" % sym)
            for n in names:
                out.append('    %s,' % (('"%s"' % n) if n else "0"))
            out += ["};", ""]
            slave_syms.append(sym)

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
    out.append("static const char *const *const slave_tables[] = {")
    for sym in slave_syms:
        out.append("    %s," % sym)
    out += ["};", "",
            "const char *wm_anim_slave_label(size_t id, int32_t wrestler_num) {",
            "    if (id >= sizeof(slave_tables)/sizeof(slave_tables[0]))",
            "        return 0;",
            "    if (wrestler_num < 0 || wrestler_num >= 9) return 0;",
            "    return slave_tables[id][wrestler_num];",
            "}",
            "",
            "size_t wm_anim_slave_table_count(void) {",
            "    return sizeof(slave_tables)/sizeof(slave_tables[0]);",
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



# The three remaining per-wrestler tables, all resolved and keyed exactly
# like the slave tables above.
_AUX = (("changeanim", CHANGEANIM_TBL_RE, None),
        ("xflip", XFLIP_TBL_RE, 1),
        ("oppoffset", OPPOFFSET_RE, 2))


def _aux_tables_in(path: pathlib.Path, kind: str):
    for name, use_re, per in _AUX:
        if name != kind:
            continue
        if per is None:
            return slave_tables_in(path, use_re)
        return word_tables_in(path, use_re, per)
    raise ValueError(kind)


def aux_table_ids(kind: str) -> dict[str, int]:
    ids: dict[str, int] = {}
    for path in canonical_files():
        for key in sorted(_aux_tables_in(path, kind)):
            if key not in ids:
                ids[key] = len(ids)
    return ids


_AUX_ID_CACHE: dict[str, dict[str, int]] = {}


def aux_table_id_for(kind: str, path: pathlib.Path, use_line: int,
                     label: str) -> int:
    if kind not in _AUX_ID_CACHE:
        _AUX_ID_CACHE[kind] = aux_table_ids(kind)
    lines = [wlanim.strip_comment(r)
             for r in path.read_text(errors="replace").splitlines()]
    key, _home, _hl, _at = slave_key_for(path, lines, use_line, label)
    if key not in _AUX_ID_CACHE[kind]:
        raise ValueError(f"no generated {kind} table for {key}")
    return _AUX_ID_CACHE[kind][key]


def render_aux_c() -> str:
    out = ["/* Auto-generated by tools/wlpuppet.py: the per-wrestler tables",
           "   ANI_CHANGEANIM_TBL, ANI_XFLIP_TBL and ANI_OPPOFFSET index. */",
           '#include "wm/anim_puppet.h"', ""]

    rows = {}
    for path in canonical_files():
        for kind, _re, _per in _AUX:
            for key, val in _aux_tables_in(path, kind).items():
                rows.setdefault(kind, {}).setdefault(key, val)

    ids = {k: aux_table_ids(k) for k, _r, _p in _AUX}

    ca = rows.get("changeanim", {})
    out.append("static const char *const changeanim_rows[][WM_ANIM_ROSTER_SLOTS] = {")
    for key in sorted(ca, key=lambda k: ids["changeanim"][k]):
        cells = ", ".join("0" if not n else '"%s"' % n for n in ca[key])
        out.append("    { %s },   /* %s */" % (cells, key))
    out += ["};", ""]

    xf = rows.get("xflip", {})
    out.append("static const int16_t xflip_rows[][WM_ANIM_ROSTER_SLOTS] = {")
    for key in sorted(xf, key=lambda k: ids["xflip"][k]):
        out.append("    { %s },   /* %s */"
                   % (", ".join(str(v[0]) for v in xf[key]), key))
    out += ["};", ""]

    oo = rows.get("oppoffset", {})
    out.append("static const int16_t oppoffset_rows[][WM_ANIM_ROSTER_SLOTS][2] = {")
    for key in sorted(oo, key=lambda k: ids["oppoffset"][k]):
        cells = ", ".join("{%d, %d}" % (v[0], v[1]) for v in oo[key])
        out.append("    { %s },   /* %s */" % (cells, key))
    out += ["};", ""]

    out += [
        "const char *wm_anim_changeanim_label(size_t id, int32_t num) {",
        "    if (id >= sizeof(changeanim_rows) / sizeof(changeanim_rows[0]))",
        "        return 0;",
        "    if (num < 0 || num >= WM_ANIM_ROSTER_SLOTS) return 0;",
        "    return changeanim_rows[id][num];",
        "}",
        "",
        "int wm_anim_xflip_for(size_t id, int32_t num) {",
        "    if (id >= sizeof(xflip_rows) / sizeof(xflip_rows[0])) return 0;",
        "    if (num < 0 || num >= WM_ANIM_ROSTER_SLOTS) return 0;",
        "    return xflip_rows[id][num];",
        "}",
        "",
        "int wm_anim_oppoffset_for(size_t id, int32_t num,",
        "                          int16_t *x, int16_t *y) {",
        "    if (id >= sizeof(oppoffset_rows) / sizeof(oppoffset_rows[0]))",
        "        return 0;",
        "    if (num < 0 || num >= WM_ANIM_ROSTER_SLOTS) return 0;",
        "    if (x) *x = oppoffset_rows[id][num][0];",
        "    if (y) *y = oppoffset_rows[id][num][1];",
        "    return 1;",
        "}",
        "",
    ]
    return "\n".join(out)

def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", action="append", default=[], type=pathlib.Path)
    ap.add_argument("--out")
    ap.add_argument("--aux-out")
    ns = ap.parse_args(argv)
    paths = [p for p in ns.source if p.exists()] or None
    text = render_c(paths)
    if ns.aux_out:
        pathlib.Path(ns.aux_out).write_text(render_aux_c())
        print("wrote %d changeanim / %d xflip / %d oppoffset tables -> %s"
              % (len(aux_table_ids("changeanim")), len(aux_table_ids("xflip")),
                 len(aux_table_ids("oppoffset")), ns.aux_out))
        return 0
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
