#!/usr/bin/env python3
"""Routine-level port coverage, resolved by symbol rather than by name.

The obvious way to measure this is to grep each source routine's name in
the port's text and call it covered if it appears. That is what an
earlier ad-hoc script did, and it is wrong in both directions:

  * A name mentioned only in a comment counts as covered -- including a
    comment that says the routine is NOT translated.
  * A routine the port implements under a different name counts as
    missing. `get_opp_plyrmode` reads as the most-referenced
    untranslated routine in the game at 115 call sites, and the port
    reads `o->player_mode` inline in seventy-odd places.
  * A prefixed translation counts as missing too, because
    `wm_get_stick_val_cur` and `get_stick_val_cur` are different tokens.

So this does three things instead.

  1. It separates DEFINITIONS from MENTIONS. Comments and string
     literals are stripped before the port is tokenised, and the comment
     text is kept separately -- so "cited in a comment" is its own
     answer, not evidence of translation.

  2. It matches on token boundaries, so `wm_arcade_round_award` counts
     for `round_award` but `unrounded_awards` would not.

  3. Everything a name cannot settle goes in port/routine_map.json, a
     hand-written ledger where each entry has to name a real source
     routine AND a real port symbol or a reason. A test checks both, so
     the ledger cannot become somewhere to hide an unanswered question.

It also measures the one thing that needs no judgement at all: a routine
with no caller outside its own file is dead code, and a port that does
not have it is not missing anything.
"""
from __future__ import annotations
import argparse
import collections
import json
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parents[1]
LEDGER = ROOT / "port" / "routine_map.json"

PORT_DIRS = ("include", "src")
# Generated files are emitted BY the tools; counting them as evidence
# that a routine is translated would be circular for tables, but they
# are legitimate homes for extracted data, so they count -- with the
# origin recorded.
GENERATED = "src/generated/"

SUBR_RE = re.compile(
    r"^[ \t]*SUBR(P?)[ \t]+(#?)([A-Za-z_][A-Za-z0-9_]*):?[ \t]*$",
    re.I | re.M)
SUBR_LINE_RE = re.compile(
    r"^[ \t]*SUBRP?[ \t]+#?[A-Za-z_][A-Za-z0-9_]*:?[ \t]*$", re.I)

def linked_asm() -> set[str]:
    """The .ASM files WRESTLE.CMD actually links.

    The tree carries code from other Williams games -- ROBO.ASM's
    `bullet`, `hulk`, `enforcer` and `quark`, FIREBAK.ASM, ADAM.ASM and
    the Adam Bomb sequence files. None of it is in this game, and
    counting it as untranslated inflates the denominator by hundreds of
    routines that were never in scope.
    """
    cmd = wlanim.ORIG / "WRESTLE.CMD"
    if not cmd.exists():
        return set()
    objs = re.findall(r"^([A-Za-z0-9_]+)\.obj",
                      cmd.read_text(errors="replace"), re.I | re.M)
    return {o.upper() + ".ASM" for o in objs}


# Animation programs are translated wholesale by tools/wlprogram.py, so
# counting them beside WRESTLE.ASM's routines buries the number that
# matters. Classify by the ROUTINE, not by the file: FINISEQ.ASM holds
# `bam_stand_anim` beside `check_roll` and `close_door`, and calling the
# whole file animation would hide three dozen real logic routines.
ANIM_NAME_RE = re.compile(r"_(anim|move)\d*$", re.I)

# Too short to match on without collecting noise.
MIN_NAME = 4

# The statuses a ledger entry may claim. Each means "this routine is
# accounted for, and here is why it is not a same-named C function".
LEDGER_STATUSES = {
    "inlined",      # the port does it, spelled out at each use site
    "renamed",      # the port does it under a different name
    "display",      # object/DMA drawing; this port has no renderer
    "process",      # a CREATE/SLEEP wrapper around something else
    "hardware",     # talks to the cabinet (sound board, CMOS, watchdog)
    "dead",         # no caller anywhere; asserted against the source
    "data",         # a table, extracted by a tool rather than written
    "partial",      # part of it is translated; the note says which part
}


# ------------------------------------------------------------------ #
# conditional assembly

# A `SUBR` inside a `.if` the assembler evaluated as false is not in the
# game. It is in the FILE -- so an enumeration that only greps SUBR
# lines counts it -- but the object never contained it, nothing can call
# it, and reporting it as an untranslated routine inflates the
# denominator with work that does not exist.
#
# 77 routines in the linked source are like this:
#
#   30  `.if DEBUG`         SYS.EQU:13 sets DEBUG to 0, so SPECIAL.ASM's
#                           skirt/sweat/blood debug helpers and friends
#                           are development-only.
#   30  `.if NUM_*_FINISHES` GAME.EQU:580-587 sets seven of the eight
#                           switches to 0, and Undertaker's `> 1` guard
#                           still excludes his second -- so exactly one
#                           finishing move exists, und_finish_move1.
#   16  `.if 0`             switched off outright.
#    1  `.if NMBPAL`        no back-palette bank on this board.
_IF_RE = re.compile(r"^\s*\.if\s+(.+?)\s*$", re.I)
_ELSE_RE = re.compile(r"^\s*\.else\s*$", re.I)
_ENDIF_RE = re.compile(r"^\s*\.endif\s*$", re.I)
# The assembler writes the directive with or without its leading dot,
# and `equ` and `.set` both bind a plain integer here.
_SET_RE = re.compile(
    r"^([A-Za-z_][A-Za-z0-9_]*)\s+\.?(?:set|equ)\s+(-?\d+)\s*$", re.I)

_EQU_CONSTS: dict[str, int] | None = None


def equ_constants() -> dict[str, int]:
    """Plain-integer equates, for deciding `.if` conditions.

    A symbol two .EQU files give DIFFERENT values is dropped: an
    ambiguous condition has to stay undecidable rather than be resolved
    by whichever file happened to be read last.
    """
    global _EQU_CONSTS
    if _EQU_CONSTS is not None:
        return _EQU_CONSTS
    seen: dict[str, int] = {}
    clash: set[str] = set()
    for equ in sorted(wlanim.ORIG.glob("*.EQU")):
        for line in equ.read_text(errors="replace").splitlines():
            m = _SET_RE.match(wlanim.strip_comment(line).strip())
            if not m:
                continue
            name, val = m.group(1), int(m.group(2))
            if name in seen and seen[name] != val:
                clash.add(name)
            seen[name] = val
    for name in clash:
        del seen[name]
    _EQU_CONSTS = seen
    return _EQU_CONSTS


def _cond_value(expr: str, consts: dict[str, int]) -> bool | None:
    """True/False for a condition this can decide, else None.

    None means "cannot tell", and a routine under a condition this
    cannot tell is left alone -- assumed assembled. Erring that way
    keeps a routine in the denominator when in doubt, which is the
    honest direction for a coverage number.
    """
    expr = expr.strip()
    if re.fullmatch(r"-?\d+", expr):
        return int(expr) != 0
    m = re.fullmatch(r"([A-Za-z_][A-Za-z0-9_]*)", expr)
    if m:
        return consts[m.group(1)] != 0 if m.group(1) in consts else None
    m = re.fullmatch(r"([A-Za-z_][A-Za-z0-9_]*)\s*([<>])\s*(-?\d+)", expr)
    if m and m.group(1) in consts:
        lhs, rhs = consts[m.group(1)], int(m.group(3))
        return lhs > rhs if m.group(2) == ">" else lhs < rhs
    return None


def unassembled_routines() -> dict[str, str]:
    """{routine name: "FILE:line, .if <condition>"} for skipped SUBRs."""
    consts = equ_constants()
    out: dict[str, str] = {}
    linked = linked_asm()
    for path in sorted(wlanim.ORIG.glob("*.ASM")):
        if linked and path.name.upper() not in linked:
            continue
        #
        # A file may define its own switches, and the ones that matter
        # most are defined behind a condition themselves. WRESTLE.ASM:48
        # opens `.if DEBUG` and sets SCRT_DEBUG, DIR_DEBUG and COL_DEBUG
        # in each arm; DEBUG is 0, so the `.else` arm is the live one and
        # all three are 0. Reading only the .EQU files left those three
        # unknown, which left five routines -- collis_debug,
        # collis_debug2, dir_debug, scrt_debug and direction_test --
        # counted as missing from a port when they are not in the ARCADE
        # either.
        #
        # Scoped to this file, seeded from the shared .EQU values, and
        # only updated from a branch already known to be taken. A
        # definition inside a dead branch is not a definition.
        #
        local = dict(consts)
        stack: list[tuple[bool | None, str]] = []
        for n, raw in enumerate(path.read_text(errors="replace").splitlines(),
                                1):
            line = wlanim.strip_comment(raw)
            m = _IF_RE.match(line)
            if m:
                stack.append((_cond_value(m.group(1), local), m.group(1)))
                continue
            if _ELSE_RE.match(line):
                if stack:
                    val, why = stack[-1]
                    stack[-1] = (None if val is None else not val,
                                 f"not ({why})")
                continue
            if _ENDIF_RE.match(line):
                if stack:
                    stack.pop()
                continue
            false_at = next((why for val, why in stack if val is False), None)
            if false_at is None:
                # Live text: a constant defined here is a real one.
                dm = _SET_RE.match(line.strip())
                if dm:
                    local[dm.group(1)] = int(dm.group(2))
                continue
            sm = wlanim.SUBR_RE.match(line)
            if sm:
                out[sm.group(1)] = f"{path.name}:{n}, .if {false_at}"
    return out


# ------------------------------------------------------------------ #
# the source side

def source_routines() -> dict[str, list[tuple[str, int, bool]]]:
    """{name: [(file, line, is_local)]} for every SUBR in the tree."""
    linked = linked_asm()
    out: dict[str, list[tuple[str, int, bool]]] = collections.defaultdict(list)
    for path in sorted(wlanim.ORIG.glob("*.ASM")):
        if linked and path.name.upper() not in linked:
            continue
        text = path.read_text(errors="replace")
        for m in SUBR_RE.finditer(text):
            line = text.count("\n", 0, m.start()) + 1
            out[m.group(3)].append((path.name, line, bool(m.group(2))))
    return out


def reference_counts() -> tuple[collections.Counter, collections.Counter]:
    """(references outside the defining file, references anywhere).

    "Anywhere" excludes `.ref`/`.def` declarations and the `SUBR` line
    that defines the routine, so a total of zero means genuinely
    nothing names it -- not even a table.

    The distinction matters. An earlier version of this called a
    routine dead when nothing outside its own file mentioned it, and
    that is wrong for every routine reached through an address table:
    ATTRACT.ASM's `show_operatormsg` is in the attract sequence table
    in its own file, and MPROC.ASM's kernel entry points are reached
    by CREATE. Both looked dead and are not.
    """
    outside: collections.Counter = collections.Counter()
    total: collections.Counter = collections.Counter()
    per_file: dict[str, collections.Counter] = {}

    linked = linked_asm()
    for path in sorted(wlanim.ORIG.glob("*.ASM")):
        if linked and path.name.upper() not in linked:
            continue
        text = path.read_text(errors="replace")
        body_lines = []
        for line in text.splitlines():
            if re.match(r"^\s*\.(ref|def|globl)\b", line, re.I):
                continue
            if SUBR_LINE_RE.match(line):
                continue        # the definition is not a reference
            body_lines.append(line)
        per_file[path.name] = collections.Counter(
            re.findall(r"[A-Za-z_][A-Za-z0-9_]*", "\n".join(body_lines)))

    routines = source_routines()
    for name, defs in routines.items():
        home = {f for f, _l, _loc in defs}
        outside[name] = sum(c[name] for f, c in per_file.items() if f not in home)
        total[name] = sum(c[name] for c in per_file.values())
    return outside, total


# ------------------------------------------------------------------ #
# the port side

BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.S)
LINE_COMMENT = re.compile(r"//[^\n]*")
STRING_LIT = re.compile(r'"(?:\\.|[^"\\])*"')
CHAR_LIT = re.compile(r"'(?:\\.|[^'\\])*'")
IDENT = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


def _split_code_and_prose(text: str) -> tuple[str, str]:
    """(code with comments and literals removed, the removed prose)."""
    prose: list[str] = []

    def take(m):
        prose.append(m.group(0))
        return " "

    code = BLOCK_COMMENT.sub(take, text)
    code = LINE_COMMENT.sub(take, code)
    # String literals hold extracted source labels -- real evidence that
    # a table names a routine -- so they are kept as prose, not code.
    code = STRING_LIT.sub(take, code)
    code = CHAR_LIT.sub(lambda m: " ", code)
    return code, "\n".join(prose)


def port_symbols() -> tuple[set[str], set[str], set[str]]:
    """(identifiers defined in code, identifiers named in prose, generated).

    "Defined in code" is every identifier that survives comment and
    string stripping. That is coarser than a real C parser -- a local
    variable counts -- but it is exactly the distinction that matters
    here, because the failure being fixed is a name that appears ONLY in
    a comment.
    """
    code_ids: set[str] = set()
    prose_ids: set[str] = set()
    generated_ids: set[str] = set()
    for d in PORT_DIRS:
        for path in sorted((ROOT / d).rglob("*")):
            if path.suffix not in (".c", ".h"):
                continue
            rel = str(path.relative_to(ROOT))
            code, prose = _split_code_and_prose(
                path.read_text(errors="replace"))
            ids = set(IDENT.findall(code))
            code_ids |= ids
            prose_ids |= set(IDENT.findall(prose))
            if rel.startswith(GENERATED):
                generated_ids |= ids | set(IDENT.findall(prose))
    return code_ids, prose_ids, generated_ids


# Wrappers the generators put around an extracted routine's own name.
# Derived from what the tools actually emit, not guessed: wlprogram.py
# writes prog_<name>_ops and prog_<name>_labels, wlrostertbl.py writes
# rows_<name>, wlpal.py writes pal_<name>.
GENERATED_WRAPPERS = ("prog_%s_ops", "prog_%s_labels", "rows_%s", "pal_%s")


def _boundary_hits(name: str, pool: set[str]) -> list[str]:
    """Identifiers in `pool` that are a translation of `name`.

    A port identifier counts when it IS the name, or ends with `_` and
    the name -- `wm_arcade_round_award` for `round_award` -- or is one
    of the shapes a generator wraps a name in.

    Deliberately not "contains the name somewhere". That looser rule
    matched LIFEBAR.ASM's `meters` against `drone_meters_on`, an
    AWARD.ASM powerup flag with nothing to do with it, and the ledger
    guard caught it. The cost of the strict rule is that a genuine
    translation whose name embeds rather than ends with the routine --
    `wm_read_switches_one` for `read_switches` -- has to be written
    down in the ledger instead of being inferred. That is the right
    trade: an explicit line in a checked file beats a fuzzy rule that
    is wrong in both directions.
    """
    if len(name) < MIN_NAME:
        return [i for i in pool if i.lower() == name.lower()]
    low = name.lower()
    suffix = "_" + low
    hits = [i for i in pool
            if i.lower() == low or i.lower().endswith(suffix)]
    wrapped = {(w % low) for w in GENERATED_WRAPPERS}
    hits += [i for i in pool if i.lower() in wrapped]
    return sorted(set(hits))


# ------------------------------------------------------------------ #
# the ledger

def load_ledger() -> dict:
    if not LEDGER.exists():
        return {}
    raw = json.loads(LEDGER.read_text())
    return {k: v for k, v in raw.items() if not k.startswith("_")}


# ------------------------------------------------------------------ #

def classify() -> dict:
    routines = source_routines()
    callers, total_refs = reference_counts()
    code_ids, prose_ids, generated_ids = port_symbols()
    ledger = load_ledger()
    skipped = unassembled_routines()

    rows = []
    for name, defs in sorted(routines.items()):
        files = sorted({f for f, _l, _loc in defs})
        local = all(loc for _f, _l, loc in defs)
        entry = ledger.get(name)

        code_hits = _boundary_hits(name, code_ids)
        prose_hits = _boundary_hits(name, prose_ids)

        if code_hits:
            status, why = "implemented", code_hits[0]
        elif name in skipped:
            # Never assembled, so there is nothing to translate. This is
            # checked BEFORE the ledger and before `dead` deliberately: a
            # routine the assembler skipped is not dead code that shipped,
            # it is code that never shipped, and the two want different
            # words. It is checked AFTER `implemented`, because the port
            # translating a cut routine anyway is worth still reporting.
            status, why = "unassembled", skipped[name]
        elif entry:
            status, why = entry["status"], entry.get("symbol") or entry.get("note", "")
        elif total_refs[name] == 0:
            status, why = "dead", "nothing anywhere names it"
        elif prose_hits:
            status, why = "cited", prose_hits[0]
        else:
            status, why = "unknown", ""

        rows.append({
            "name": name,
            "kind": "animation" if ANIM_NAME_RE.search(name) else "logic",
            "files": files,
            "definitions": len(defs),
            "local": local,
            "external_callers": callers[name],
            "references": total_refs[name],
            "status": status,
            "evidence": why,
            "from_generated": bool(code_hits) and code_hits[0] in generated_ids,
            "ledgered": entry is not None,
        })
    return {"routines": rows}


def _bucket(rows) -> dict:
    by = collections.Counter(r["status"] for r in rows)
    # "Accounted for" is everything with an answer -- implemented, or a
    # ledger entry explaining why there is nothing to implement, or a
    # measured absence of any reference at all.
    accounted = sum(v for k, v in by.items() if k not in ("unknown", "cited"))
    return {
        "total": len(rows),
        "by_status": dict(sorted(by.items())),
        "accounted_for": accounted,
        "open": by["unknown"] + by["cited"],
    }


def summarise(data: dict) -> dict:
    rows = data["routines"]
    out = _bucket(rows)
    out["logic"] = _bucket([r for r in rows if r["kind"] == "logic"])
    out["animation"] = _bucket([r for r in rows if r["kind"] == "animation"])
    return out


def render_md(data: dict) -> str:
    s = summarise(data)
    rows = data["routines"]
    out = [
        "# Routine coverage",
        "",
        "Generated by `tools/port_coverage.py`. Every `SUBR` in the historical",
        "source, resolved against identifiers the port actually **defines** --",
        "comments and string literals are stripped first, so a routine named",
        "only in a note saying it is missing does not count as present.",
        "",
        "| status | meaning | count |",
        "|---|---|---|",
        "| `implemented` | a port identifier IS the name, ends with it, "
        "or is a generator's wrapper around it | %d |"
        % s["by_status"].get("implemented", 0),
    ]
    meanings = {
        "inlined": "the port does it, spelled out at each use site (ledger)",
        "renamed": "the port does it under another name (ledger)",
        "display": "object/DMA drawing; this port has no renderer (ledger)",
        "process": "a CREATE/SLEEP wrapper (ledger)",
        "hardware": "talks to the cabinet (ledger)",
        "data": "a table, extracted by a tool (ledger)",
        "partial": "partly translated; the ledger note says which part",
        "dead": "**measured**: no caller outside its own file",
        "unassembled": "**measured**: inside a `.if` the assembler skipped",
        "cited": "named only in a comment -- not evidence of anything",
        "unknown": "no mention anywhere",
    }
    for k in ("inlined", "renamed", "partial", "data", "process", "display",
              "hardware", "dead", "unassembled", "cited", "unknown"):
        if s["by_status"].get(k):
            out.append("| `%s` | %s | %d |" % (k, meanings[k], s["by_status"][k]))
    out += [
        "",
        "**%d of %d routines are accounted for; %d are open** "
        "(`cited` + `unknown`)." % (s["accounted_for"], s["total"], s["open"]),
        "",
        "## Open routines by file",
        "",
        "| file | open | routines |",
        "|---|---|---|",
    ]
    per_file: dict[str, list[str]] = collections.defaultdict(list)
    for r in rows:
        if r["status"] in ("cited", "unknown"):
            per_file[r["files"][0]].append(r["name"])
    for f, names in sorted(per_file.items(), key=lambda kv: -len(kv[1])):
        shown = ", ".join("`%s`" % n for n in sorted(names)[:12])
        if len(names) > 12:
            shown += ", ..."
        out.append("| %s | %d | %s |" % (f, len(names), shown))
    out.append("")
    return "\n".join(out)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out-md")
    ap.add_argument("--out-json")
    ap.add_argument("--status", help="list every routine with this status")
    ns = ap.parse_args()

    data = classify()
    s = summarise(data)

    if ns.status:
        for r in data["routines"]:
            if r["status"] == ns.status:
                print("%-28s %-16s callers=%-4d %s"
                      % (r["name"], r["files"][0], r["external_callers"],
                         r["evidence"]))
        return 0

    if ns.out_json:
        pathlib.Path(ns.out_json).write_text(json.dumps(data, indent=1) + "\n")
    if ns.out_md:
        pathlib.Path(ns.out_md).write_text(render_md(data))
    for label in ("logic", "animation"):
        b = s[label]
        print("%-10s %4d routines | accounted for %4d | open %4d"
              % (label, b["total"], b["accounted_for"], b["open"]))
        for k, v in b["by_status"].items():
            print("      %-14s %d" % (k, v))
    print("%-10s %4d routines | accounted for %4d | open %4d"
          % ("TOTAL", s["total"], s["accounted_for"], s["open"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
