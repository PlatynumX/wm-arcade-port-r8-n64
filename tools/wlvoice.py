#!/usr/bin/env python3
"""Extract DCSSOUND.ASM's announcer line tables and their callers.

Every announcer routine in the game is the same shape: a `CALL_x` that
CREATEs a process, a `PROC_x` that sleeps and then calls ADD_IF_SILENT
with a table address and a percentage, and the table itself. The tables
carry their shape in three values written immediately BEFORE the label,
which ADD_TO_QUEUE reads at negative offsets (DCSSOUND.ASM:2921-2941):

        .WORD   -1              ; at -050H: reset REPEAT_STATE first
        .LONG   CROWD_FAIL      ; at -040H: crowd reaction table, 0 = none
        .WORD   17,010H         ; at -020H: last row index; -010H: stride
    MISSES
        .WORD   A_MISS          ; row 0
        ...                     ; rows 1..17
        .WORD   A_MISS          ; padding, see below
        ...

The source's own header comment says "WORD x(NUMBER OF TABLE ENTRIES -1),
TABLE ENTRY SIZE", so RNDRNG0 is called with the last index and picks
0..x inclusive. Stride is a TMS34010 BIT count: 010H is one word per row,
020H two.

The rows written after the picked range are not dead. When
ARE_WE_REPEATING rejects a row, ADD_TO_QUEUE walks FORWARD to the next one
(`SUBI 010H,A1 / ADD A3,A1 / JRUC ADD_AGAIN`) with no bound check at all,
so a rejected row near the end reads into the padding -- which is why the
padding of every table is a copy of its own first few rows. They are
extracted with the table for exactly that reason.
"""
from __future__ import annotations
import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim  # noqa: E402

SRC = wlanim.ORIG / "DCSSOUND.ASM"

# SOUND.EQU carries the line ids (`A_MISS .EQU 159H`) and the six negative
# sentinels ADD_TO_QUEUE tests for by name.
EQU = wlanim.load_equ(wlanim.ORIG / "SOUND.EQU", "")

# SOUND.EQU:57-63. Every one of these means "this row is not a line id,
# work out the real one first".
SPECIAL = {
    "GIVE_CREDIT": -1,
    "VERY_IMPRESSIVE": -2,
    "END_GAME_STUFF": -3,
    "IT_DOESNT_LOOK_GOOD": -4,
    "R_IMPRESSIVE_MOVE": -5,
    "GIDDUP_MODE": -6,
    "REPEAT_MODE": -7,
}

# The five per-wrestler tables SET_UP_PERSONAL_CALL indexes with A5, plus
# the repeat counter's own. Slot 7 is Adam Bomb, the cut wrestler, and is
# a literal `.WORD 0` in all of them.
PERSONAL_TABLES = ["GIVE_CREDIT_TO", "VERY_IMPRESSIVE_MOVE",
                   "IT_DOESNT_LOOK_GOOD_FOR", "VERY_IMPRESSIVE_MOVE_R",
                   "GIDDUP_ALL"]

# DO_END_STUFF (DCSSOUND.ASM:3149) reads SPECIAL_LAST_STUFF at -010H and
# -020H only, so the table was written with just the `.WORD count,stride`
# pair and no crowd long or reset flag above it -- the words that DO sit at
# its -040H and -050H belong to the previous table's padding, and nothing
# ever reads them. It is the one table admitted without a full header, by
# name, so a malformed one elsewhere still fails to parse.
PARTIAL_HEADER_OK = {"SPECIAL_LAST_STUFF"}

WORD_RE = re.compile(r"^\s*\.WORD\s+(.+)$", re.I)
LONG_RE = re.compile(r"^\s*\.LONG\s+(.+)$", re.I)
LABEL_RE = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)\s*$")
CALL_RE = re.compile(r"^(CALL_[A-Z0-9_]+|DO_REVERSAL)\s*$")
PROC_RE = re.compile(r"^(PROC_[A-Z0-9_]+)\s*$")
CREATE_RE = re.compile(r"^\s*CREATE\s+\S+\s*,\s*(\S+)", re.I)
SLEEP_RE = re.compile(r"^\s*SLEEP\s+(\S+)", re.I)
MOVI_RE = re.compile(r"^\s*MOVI\s+(\S+?)\s*,\s*(A\d+)", re.I)
WRESTLERNUM_RE = re.compile(r"^\s*MOVE\s+\*(A\d+)\(WRESTLERNUM\)\s*,\s*A9", re.I)


def _lines() -> list[str]:
    return [wlanim.strip_comment(r).rstrip()
            for r in SRC.read_text(errors="replace").splitlines()]


def _word(sym: str) -> int:
    """Resolve one `.WORD` operand to its number."""
    s = sym.strip()
    if re.fullmatch(r"-?[0-9]+", s):
        return int(s)
    if re.fullmatch(r"[0-9A-Fa-f]+[hH]", s):
        return int(s[:-1], 16)
    up = s.upper()
    if up in EQU:
        return EQU[up]
    if up in wlanim.GLOBAL_EQU:
        return wlanim.GLOBAL_EQU[up]
    raise ValueError(f"unresolved announcer line id {sym!r}")


def _split_words(operand: str) -> list[str]:
    return [p for p in (q.strip() for q in operand.split(",")) if p]


def announce_tables() -> dict[str, dict]:
    """Every table with the three-value ADD_TO_QUEUE header before it."""
    lines = _lines()
    out: dict[str, dict] = {}
    for i, line in enumerate(lines):
        m = LABEL_RE.match(line)
        if not m:
            continue
        name = m.group(1)
        # Walk back over blank lines to the `.WORD count,stride`.
        j = i - 1
        while j >= 0 and not lines[j].strip():
            j -= 1
        if j < 0:
            continue
        mw = WORD_RE.match(lines[j])
        if not mw:
            continue
        parts = _split_words(mw.group(1))
        if len(parts) != 2:
            continue
        try:
            last_index = _word(parts[0])
            stride_bits = _word(parts[1])
        except ValueError:
            continue
        if stride_bits % 16 or not 0 < stride_bits <= 64 or last_index < 0:
            continue
        # ...the `.LONG crowd` above it, and the `.WORD reset` above that.
        k = j - 1
        while k >= 0 and not lines[k].strip():
            k -= 1
        ml = LONG_RE.match(lines[k] if k >= 0 else "")
        crowd, reset_repeat = None, False
        if ml:
            crowd = ml.group(1).strip()
            p = k - 1
            while p >= 0 and not lines[p].strip():
                p -= 1
            mr = WORD_RE.match(lines[p] if p >= 0 else "")
            if not mr or len(_split_words(mr.group(1))) != 1:
                continue
            reset_repeat = _word(_split_words(mr.group(1))[0]) != 0
        elif name not in PARTIAL_HEADER_OK:
            continue

        stride = stride_bits // 16
        rows: list[list[int]] = []
        for q in range(i + 1, len(lines)):
            body = lines[q]
            if not body.strip():
                continue
            mb = WORD_RE.match(body)
            if not mb:
                break
            vals = [_word(v) for v in _split_words(mb.group(1))]
            if len(vals) != stride:
                # A short final row is the next table's header, not data.
                break
            rows.append(vals)
        if len(rows) <= last_index:
            raise ValueError(
                f"{name}: header says rows 0..{last_index} but only "
                f"{len(rows)} were read")
        out[name] = {
            "name": name,
            "line": i + 1,
            "last_index": last_index,
            "stride": stride,
            "reset_repeat": reset_repeat,
            "crowd": None if crowd in (None, "0", "0H") else crowd,
            "partial_header": ml is None,
            "rows": rows,
        }
    return out


def callers() -> dict[str, dict]:
    """CALL_x -> {proc, sleep, table, percent, personal}."""
    lines = _lines()
    procs: dict[str, dict] = {}
    for i, line in enumerate(lines):
        m = PROC_RE.match(line)
        if not m:
            continue
        info = {"sleep": 0, "table": None, "percent": None, "personal": False}
        for q in range(i + 1, min(i + 20, len(lines))):
            body = lines[q]
            if re.match(r"^\s*DIE\s*$", body, re.I):
                break
            ms = SLEEP_RE.match(body)
            if ms:
                info["sleep"] = wlanim.eval_ticks(ms.group(1))
                continue
            if re.match(r"^\s*MOVE\s+A9\s*,\s*A5", body, re.I):
                info["personal"] = True
                continue
            mm = MOVI_RE.match(body)
            if mm:
                val, reg = mm.group(1), mm.group(2).upper()
                if reg == "A2":
                    info["table"] = val
                elif reg == "A0":
                    info["percent"] = int(val)
        procs[m.group(1)] = info

    out: dict[str, dict] = {}
    for i, line in enumerate(lines):
        m = CALL_RE.match(line)
        if not m:
            continue
        name = m.group(1)
        info = None
        for q in range(i + 1, min(i + 30, len(lines))):
            body = lines[q]
            if re.match(r"^\s*RETS\s*$", body, re.I):
                break
            mc = CREATE_RE.match(body)
            if mc and mc.group(1) in procs:
                info = dict(procs[mc.group(1)])
                info["proc"] = mc.group(1)
                break
            # DO_REVERSAL does the work inline instead of creating one.
            mm = MOVI_RE.match(body)
            if mm and mm.group(2).upper() == "A2":
                info = {"proc": None, "sleep": 0, "table": mm.group(1),
                        "percent": None, "personal": True}
        if info is None:
            continue
        if info.get("percent") is None:
            for q in range(i + 1, min(i + 40, len(lines))):
                mm = MOVI_RE.match(lines[q])
                if mm and mm.group(2).upper() == "A0":
                    info["percent"] = int(mm.group(1))
                    break
        # `MOVE *A13(WRESTLERNUM),A9` is the animation entry point (a13 is
        # the wrestler running the animation); `*A10(...)` is the other.
        for q in range(i + 1, min(i + 6, len(lines))):
            mw = WRESTLERNUM_RE.match(lines[q])
            if mw:
                info["wrestler_reg"] = mw.group(1).upper()
        if info["table"] is None or info["percent"] is None:
            raise ValueError(f"{name}: no table or no percentage")
        out[name] = info
    return out


def personal_tables() -> dict[str, list[int]]:
    lines = _lines()
    out: dict[str, list[int]] = {}
    for want in PERSONAL_TABLES:
        idx = next((i for i, l in enumerate(lines) if l.strip() == want), None)
        if idx is None:
            raise ValueError(f"no personal-call table {want}")
        rows = []
        for q in range(idx + 1, len(lines)):
            if not lines[q].strip():
                if rows:
                    break
                continue
            mb = WORD_RE.match(lines[q])
            if not mb:
                break
            rows.append(_word(_split_words(mb.group(1))[0]))
        if len(rows) != 9:
            raise ValueError(f"{want}: expected 9 wrestler slots, got {len(rows)}")
        out[want] = rows
    return out


def ascending_table() -> list[list[int]]:
    """SET_UP_PERSONAL_CALL's repeat counter, 4 words per wrestler."""
    lines = _lines()
    idx = next((i for i, l in enumerate(lines)
                if l.strip() == "ASCENDING_TABLE"), None)
    if idx is None:
        raise ValueError("no ASCENDING_TABLE")
    rows = []
    for q in range(idx + 1, len(lines)):
        if not lines[q].strip():
            if rows:
                break
            continue
        mb = WORD_RE.match(lines[q])
        if not mb:
            break
        vals = [_word(v) for v in _split_words(mb.group(1))]
        if len(vals) != 4:
            break
        rows.append(vals)
    if len(rows) != 9:
        raise ValueError(f"ASCENDING_TABLE: expected 9 rows, got {len(rows)}")
    return rows


# Only the tables something translated can actually reach are emitted. The
# rest of DCSSOUND.ASM's tables are real and parse cleanly, but each is
# reached from a part of the game this port has not translated:
# CLIMB_ROPES/JUMP_ROPES from BRET.ASM/BAM.ASM's turnbuckle control layer,
# MATCH_OVER/MATCH_OVER_DL and the seven *_FINISHES from the post-match
# speech. Emitting them would be dead data whose correctness nothing here
# could check.
def wanted_tables(calls: dict[str, dict]) -> list[str]:
    # END_GAME_STUFF in a picked row diverts the whole call to
    # SPECIAL_LAST_STUFF (DCSSOUND.ASM:3149 DO_END_STUFF), so that table is
    # reachable from AVERAGE_MOVE even though no caller names it.
    return sorted({c["table"] for c in calls.values()} | {"SPECIAL_LAST_STUFF"})


def render_c() -> str:
    tables = announce_tables()
    calls = callers()
    personal = personal_tables()
    asc = ascending_table()

    missing = [c["table"] for c in calls.values() if c["table"] not in tables]
    if missing:
        raise ValueError(f"callers name tables with no header: {missing}")

    out = ["/* Auto-generated by tools/wlvoice.py from the original",
           "   DCSSOUND.ASM announcer tables -- do not edit. */",
           '#include "wm/announce_tables.h"',
           ""]
    names = [n for n in wanted_tables(calls) if n in tables]
    absent = [n for n in wanted_tables(calls) if n not in tables]
    if absent:
        raise ValueError(f"no header for {absent}")
    for n in names:
        t = tables[n]
        out.append(f"/* DCSSOUND.ASM:{t['line']} {n} -- rows 0..{t['last_index']}"
                   f" of {len(t['rows'])} ({t['stride']} word(s) each,"
                   f" the rest is the walk-forward padding). */")
        out.append(f"static const int16_t {n.lower()}_rows[] = {{")
        for r_i, r in enumerate(t["rows"]):
            tag = "" if r_i <= t["last_index"] else "   /* padding */"
            out.append("    " + ", ".join(str(v) for v in r) + ","
                       + tag)
        out += ["};", ""]

    out.append("const wm_announce_table wm_announce_tables[] = {")
    for n in names:
        t = tables[n]
        out.append(f'    {{ "{n}", {n.lower()}_rows, '
                   f"sizeof({n.lower()}_rows) / sizeof({n.lower()}_rows[0]), "
                   f"{t['last_index']}, {t['stride']}, "
                   f"{'true' if t['reset_repeat'] else 'false'} }},")
    out += ["};", "const size_t wm_announce_table_count =",
            "    sizeof(wm_announce_tables) / sizeof(wm_announce_tables[0]);",
            ""]

    out.append("/* DCSSOUND.ASM:3083 SET_UP_PERSONAL_CALL's five per-wrestler")
    out.append("   tables, in WRESTLERNUM order. Slot 7 is Adam Bomb, the cut")
    out.append("   wrestler, and is a literal 0 in every one of them. */")
    out.append("const int16_t wm_announce_personal"
               "[WM_ANNOUNCE_PERSONAL_KINDS][WM_ANNOUNCE_WRESTLERS] = {")
    for want in PERSONAL_TABLES:
        out.append(f"    /* {want} */")
        out.append("    { " + ", ".join(str(v) for v in personal[want]) + " },")
    out += ["};", ""]

    out.append("/* ASCENDING_TABLE: four lines per wrestler, indexed by the")
    out.append("   REPEAT_STATE counter as it runs 3, 2, 1, 0. */")
    out.append("const int16_t wm_announce_ascending"
               "[WM_ANNOUNCE_WRESTLERS][WM_ANNOUNCE_REPEAT_STEPS] = {")
    for row in asc:
        out.append("    { " + ", ".join(str(v) for v in row) + " },")
    out += ["};", ""]

    out.append("/* The CALL_x entry points: each CREATEs a process that")
    out.append("   sleeps and then calls ADD_IF_SILENT with this table and")
    out.append("   this RNDPER percentage. */")
    out.append("const wm_announce_call wm_announce_calls[] = {")
    for n in sorted(calls):
        c = calls[n]
        out.append(f'    {{ "{n}", "{c["table"]}", {c["sleep"]}, '
                   f"{c['percent']}, {'true' if c['personal'] else 'false'} }},")
    out += ["};", "const size_t wm_announce_call_count =",
            "    sizeof(wm_announce_calls) / sizeof(wm_announce_calls[0]);",
            ""]
    return "\n".join(out) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out")
    ap.add_argument("--list", action="store_true")
    args = ap.parse_args()
    if args.list:
        t = announce_tables()
        for n in sorted(t):
            r = t[n]
            print(f"{n:24s} rows 0..{r['last_index']} of {len(r['rows']):3d} "
                  f"stride {r['stride']} reset={r['reset_repeat']} "
                  f"crowd={r['crowd']}")
        print()
        for n, c in sorted(callers().items()):
            print(f"{n:24s} -> {c['table']:20s} sleep {c['sleep']:3d} "
                  f"pct {c['percent']:4d} personal={c['personal']}")
        return 0
    text = render_c()
    if args.out:
        pathlib.Path(args.out).write_text(text)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
