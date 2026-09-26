#!/usr/bin/env python3
"""Extract every wrestler's JJXM move tables -- the opponent-mode dispatch.

JJXM.H is four lines of macro and it decides what every attack button in
the game actually does:

    JJXM_INIT   calla get_opp_plyrmode        ; a0 = the OPPONENT's PLYRMODE
                move  *a13(CLOSEST_XDIST),a1
                move  *a13(CLOSEST_ZDIST),a2

    JJXM MODE,DX,DZ,LESS,MORE
                cmpi MODE_:MODE:,a0 / jrne next
                cmpi :DX:,a1 / jrgt :MORE:
                cmpi :DZ:,a2 / jrgt :MORE:
                jruc :LESS:

    JJXM MODE,TARGET            ; the `.if $isname(DX)` arm
                cmpi MODE_:MODE:,a0 / jrne next
                jruc :TARGET:

    JJXM_END    LOCKUP / rets   ; a mode with no row does NOTHING

So one press is answered by a 22-row table keyed on what the OTHER
wrestler is doing, and only then by how far away he is. `cmpi X,a1`
computes a1-X on a TMS34010, so `jrgt MORE` is taken when the distance is
STRICTLY GREATER than the threshold -- the LESS arm is the closed
interval, xdist <= DX AND zdist <= DZ.

Six tables per wrestler (eight for Shawn): punch, super punch, kick and
super kick under mode_normal, and the punch and kick groups again under
mode_running, where the same button means a clothesline instead.

This tool reads them out rather than transcribing them, because there are
1100 rows of it and a hand-copied row is a bug nobody can see.

Names are kept exactly as the source writes them -- `#punch_hdbutt`,
`std_punch`, `do_pile` -- so a reader can grep the ASM for any target in
the generated file and land on the routine it names.
"""
from __future__ import annotations
import argparse
import json
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim  # noqa: E402

ORIG = wlanim.ORIG

# PLYR.EQU:276-305. The three "RPT" aliases are equ 0 and are never
# written in a JJXM row, so they are deliberately absent.
MODES = {
    "NORMAL": 0, "RUNNING": 1, "INAIR": 2, "ATTACHED": 3, "ONGROUND": 4,
    "BOUNCING": 5, "ONTURNBKL": 6, "BLOCK": 7, "DIZZY": 8, "DEAD": 9,
    "OPPOVERHEAD": 10, "CLIMBTURNBKL": 11, "WAITANIM": 12, "GRAPPLE": 13,
    "MASTER": 14, "SLAVE": 15, "HEADHOLD": 16, "PUPPET2": 17,
    "HEADHELD": 19, "PUPPET": 20, "INAIR2": 21, "CHOKEHOLD": 24,
    "CHOKING": 25,
}

# The eight wrestler files, in WRESTLERNUM order. Adam Bomb (slot 7) has
# no file of his own -- he is the spare the roster leaves empty.
FILES = [
    ("BRET", "BRET.ASM"),
    ("RAZOR", "RAZOR.ASM"),
    ("TAKER", "TAKER.ASM"),
    ("YOKO", "YOKO.ASM"),
    ("SHAWN", "SHAWN.ASM"),
    ("BAM", "BAM.ASM"),
    ("DOINK", "DOINK.ASM"),
    ("LEX", "LEX.ASM"),
]

SECTION_RE = re.compile(r"^(mode_[a-z0-9_]+)\s*(;.*)?$")
LOCAL_RE = re.compile(r"^(#[A-Za-z0-9_]+)\s*(;.*)?$")
INIT_RE = re.compile(r"^\s+JJXM_INIT\b")
END_RE = re.compile(r"^\s+JJXM_END\b")
ROW_RE = re.compile(r"^\s+JJXM\s+(.*?)\s*$")


def strip_comment(text):
    """A TMS34010 comment: `;` anywhere, or `*` in column one."""
    if text.startswith("*"):
        return ""
    cut = text.find(";")
    return text if cut < 0 else text[:cut]


def parse_number(token):
    """A JJXM threshold. The source writes them decimal."""
    token = token.strip()
    if token.lower().endswith("h"):
        return int(token[:-1], 16)
    return int(token, 10)


def parse_file(name, path):
    lines = path.read_text(errors="replace").splitlines()
    tables = []
    section = None
    pending = []
    table = None
    for lineno, raw in enumerate(lines, 1):
        if table is not None:
            if END_RE.match(raw):
                tables.append(table)
                table = None
                continue
            m = ROW_RE.match(raw)
            if not m:
                # Blank lines and comments sit inside these tables.
                if strip_comment(raw).strip():
                    raise SystemExit(
                        "%s:%d: unexpected line inside a JJXM table: %r"
                        % (path.name, lineno, raw))
                continue
            fields = [f.strip() for f in strip_comment(m.group(1)).split(",")]
            fields = [f for f in fields if f]
            mode = fields[0]
            if mode not in MODES:
                raise SystemExit("%s:%d: unknown mode %r"
                                 % (path.name, lineno, mode))
            if len(fields) == 2:
                row = {"mode": mode, "mode_value": MODES[mode],
                       "dx": None, "dz": None,
                       "less": fields[1], "more": fields[1],
                       "line": lineno}
            elif len(fields) == 5:
                row = {"mode": mode, "mode_value": MODES[mode],
                       "dx": parse_number(fields[1]),
                       "dz": parse_number(fields[2]),
                       "less": fields[3], "more": fields[4],
                       "line": lineno}
            else:
                raise SystemExit("%s:%d: JJXM row has %d fields: %r"
                                 % (path.name, lineno, len(fields), raw))
            table["rows"].append(row)
            continue

        if INIT_RE.match(raw):
            table = {"wrestler": name, "file": path.name, "line": lineno,
                     "section": section, "entries": list(pending),
                     "rows": []}
            pending = []
            continue

        stripped = strip_comment(raw)
        m = SECTION_RE.match(stripped.rstrip())
        if m:
            section = m.group(1)
            pending = []
            continue
        m = LOCAL_RE.match(stripped.rstrip())
        if m:
            pending.append(m.group(1))
            continue
        if stripped.strip():
            # Any real instruction breaks the run of labels that belongs
            # to the next table.
            pending = []
    if table is not None:
        raise SystemExit("%s: JJXM_INIT at line %d never reached JJXM_END"
                         % (path.name, table["line"]))
    return tables


def collect():
    out = []
    for name, filename in FILES:
        path = ORIG / filename
        if not path.exists():
            raise SystemExit("missing source file: %s" % path)
        out.extend(parse_file(name, path))
    return out


def c_string_array(symbol, values):
    body = "".join('    "%s",\n' % v for v in values)
    return "static const char *const %s[] = {\n%s};\n" % (symbol, body)


def emit_c(tables, out_path):
    lines = []
    lines.append("/* GENERATED by tools/wljjxm.py -- do not edit.\n")
    lines.append(" *\n")
    lines.append(" * JJXM.H's opponent-mode move tables, read out of each\n")
    lines.append(" * wrestler's own .ASM. See the tool for what the macro does.\n")
    lines.append(" */\n")
    lines.append('#include "wm/arcade/wm_arcade_jjxm.h"\n\n')

    table_syms = []
    for idx, t in enumerate(tables):
        base = "jjxm_%d" % idx
        targets = []
        for row in t["rows"]:
            for key in ("less", "more"):
                if row[key] not in targets:
                    targets.append(row[key])
        lines.append(c_string_array(base + "_targets", targets))
        lines.append(c_string_array(base + "_entries", t["entries"]))
        lines.append("static const wm_jjxm_row_t %s_rows[] = {\n" % base)
        for row in t["rows"]:
            dx = -1 if row["dx"] is None else row["dx"]
            dz = -1 if row["dz"] is None else row["dz"]
            lines.append(
                "    { %2d, %4d, %4d, %2d, %2d },  /* MODE_%s */\n"
                % (row["mode_value"], dx, dz,
                   targets.index(row["less"]), targets.index(row["more"]),
                   row["mode"]))
        lines.append("};\n\n")
        table_syms.append((base, t, targets))

    lines.append("const wm_jjxm_table_t wm_jjxm_tables[] = {\n")
    for base, t, targets in table_syms:
        lines.append(
            '    { "%s", "%s", "%s", %d, %s_entries, %d, %s_rows, %d,'
            " %s_targets, %d },\n"
            % (t["wrestler"], t["file"], t["section"], t["line"],
               base, len(t["entries"]), base, len(t["rows"]),
               base, len(targets)))
    lines.append("};\n")
    lines.append("const size_t wm_jjxm_table_count =\n"
                 "    sizeof wm_jjxm_tables / sizeof wm_jjxm_tables[0];\n")
    out_path.write_text("".join(lines))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out-c")
    ap.add_argument("--out-json")
    args = ap.parse_args()
    tables = collect()
    if args.out_c:
        emit_c(tables, pathlib.Path(args.out_c))
    if args.out_json:
        pathlib.Path(args.out_json).write_text(
            json.dumps(tables, indent=1) + "\n")
    if not args.out_c and not args.out_json:
        rows = sum(len(t["rows"]) for t in tables)
        for t in tables:
            print("%-6s %-16s %-24s %d rows"
                  % (t["wrestler"], t["section"] or "?",
                     ",".join(t["entries"]) or "?", len(t["rows"])))
        print("%d tables, %d rows" % (len(tables), rows))


if __name__ == "__main__":
    main()
