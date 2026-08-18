#!/usr/bin/env python3
"""Extract the source-visible player-select constants/tables from SELECT.ASM.

This is intentionally narrow: it translates data that is explicitly represented
in the original source and leaves process/control-flow semantics in src/core/select.c.
"""
from __future__ import annotations
import argparse
import pathlib
import re
import sys

COMMENT_RE = re.compile(r";.*$")
WORD_RE = re.compile(r"^\s*\.word\s+(.+?)\s*$", re.I)
LONG_RE = re.compile(r"^\s*\.long\s+(.+?)\s*$", re.I)


def clean(line: str) -> str:
    return COMMENT_RE.sub("", line).strip()


def split_args(s: str) -> list[str]:
    return [x.strip() for x in s.split(",") if x.strip()]


def parse_int_expr(expr: str) -> int:
    s = expr.strip().lower()
    # Midway assembler hex syntax: 0c8h, 20h.  Keep arithmetic limited to
    # integer literals and +/-, which is all the source fields used here need.
    s = re.sub(r"\b([0-9][0-9a-f]*)h\b", lambda m: str(int(m.group(1), 16)), s)
    if not re.fullmatch(r"[0-9+\-() ]+", s):
        raise ValueError(f"unsupported integer expression: {expr}")
    return int(eval(s, {"__builtins__": {}}, {}))


def is_label_line(code: str, label: str) -> bool:
    c = code.lstrip("#").strip()
    if c == label or c.startswith(label + " ") or c.startswith(label + ":"):
        return True
    m = re.match(r"SUBR(?:P)?\s+#?([A-Za-z_][A-Za-z0-9_]*)\b", c, re.I)
    return bool(m and m.group(1).lower() == label.lower())

def lines_between(text: str, start_label: str, end_label: str) -> list[str]:
    lines = text.splitlines()
    start = None
    for i, raw in enumerate(lines):
        c = clean(raw)
        if is_label_line(c, start_label):
            start = i + 1
            break
    if start is None:
        raise ValueError(f"label not found: {start_label}")
    out = []
    for raw in lines[start:]:
        c = clean(raw)
        if is_label_line(c, end_label):
            break
        out.append(raw)
    else:
        raise ValueError(f"end label not found: {end_label}")
    return out


def parse_croutons(text: str) -> list[tuple[int,int]]:
    vals: list[int] = []
    for raw in lines_between(text, "crouton_pos_table", "player_cursor"):
        m = WORD_RE.match(clean(raw))
        if not m:
            continue
        args = split_args(m.group(1))
        for a in args:
            v = parse_int_expr(a)
            if len(args) == 1 and v == 0:
                if len(vals) != 16:
                    raise ValueError(f"expected 16 crouton coordinates, got {len(vals)}")
                return list(zip(vals[0::2], vals[1::2]))
            vals.append(v)
    raise ValueError("crouton_pos_table terminator not found")


def parse_attributes(text: str) -> list[tuple[int,int,int,int]]:
    vals: list[tuple[int,int,int,int]] = []
    for raw in lines_between(text, "wrestler_attributes", "scramble_table"):
        m = WORD_RE.match(clean(raw))
        if not m:
            continue
        args = [parse_int_expr(x) for x in split_args(m.group(1))]
        if len(args) == 4:
            vals.append(tuple(args))
    if len(vals) != 9:
        raise ValueError(f"expected 9 wrestler attribute rows, got {len(vals)}")
    return vals


def parse_scramble(text: str) -> list[int]:
    vals: list[int] = []
    for raw in lines_between(text, "scramble_table", "attbars"):
        m = WORD_RE.match(clean(raw))
        if m:
            vals += [parse_int_expr(x) for x in split_args(m.group(1))]
    if len(vals) != 8:
        raise ValueError(f"expected 8 scramble entries, got {len(vals)}")
    return vals


def parse_bmods(text: str) -> list[tuple[str,int,int]]:
    lines = lines_between(text, "plyrsel_mod", "crutplt_z")
    out: list[tuple[str,int,int]] = []
    pending: str | None = None
    for raw in lines:
        c = clean(raw)
        lm = LONG_RE.match(c)
        if lm:
            arg = split_args(lm.group(1))[0]
            if arg == "0":
                break
            pending = arg.lstrip("#")
            continue
        wm = WORD_RE.match(c)
        if wm and pending:
            xy = [parse_int_expr(x) for x in split_args(wm.group(1))]
            if len(xy) != 2:
                raise ValueError("select BMOD placement must be x,y")
            out.append((pending, xy[0], xy[1]))
            pending = None
    if len(out) != 2:
        raise ValueError(f"expected two select BMODs, got {len(out)}")
    return out


def parse_player_info(text: str, label: str) -> dict:
    end = "p2info" if label == "p1info" else "plt_b"
    lines = lines_between(text, label, end)
    word_rows: list[list[str]] = []
    for raw in lines:
        m = WORD_RE.match(clean(raw))
        if m:
            word_rows.append(split_args(m.group(1)))
    # Source layout: start index; mug x,y; mug ctrl; ...; move/select sounds.
    if len(word_rows) < 4:
        raise ValueError(f"incomplete {label}")
    start = parse_int_expr(word_rows[0][0])
    mug = tuple(parse_int_expr(x) for x in word_rows[1])
    sounds = tuple(parse_int_expr(x) for x in word_rows[-1])
    if len(mug) != 2 or len(sounds) != 2:
        raise ValueError(f"bad {label} mug/sounds")
    flip = "M_FLIPH" in "\n".join(lines)
    return {"start": start, "mug": mug, "flip": flip,
            "move_sound": sounds[0], "select_sound": sounds[1]}


def parse(text: str) -> dict:
    return {
        "croutons": parse_croutons(text),
        "attributes": parse_attributes(text),
        "scramble": parse_scramble(text),
        "bmods": parse_bmods(text),
        "players": [parse_player_info(text, "p1info"), parse_player_info(text, "p2info")],
    }


def emit(data: dict, out: pathlib.Path) -> None:
    lines = [
        "/* Auto-generated from historical SELECT.ASM by tools/select_source.py. */",
        '#include "wm/select.h"', "",
        "const wm_select_point wm_select_crouton_positions[WM_SELECT_VISIBLE_SLOTS] = {",
    ]
    for x,y in data["croutons"]:
        lines.append(f"    {{{x}, {y}}},")
    lines += ["};", "", "const uint8_t wm_select_slot_source_wrestlers[WM_SELECT_VISIBLE_SLOTS] = {"]
    lines.append("    " + ", ".join(str(v) for v in data["scramble"]) + ",")
    lines += ["};", "", "const wm_select_attributes wm_select_source_attributes[WM_SELECT_SOURCE_WRESTLERS] = {"]
    for row in data["attributes"]:
        lines.append("    {" + ", ".join(str(v) for v in row) + "},")
    lines += ["};", "", "const wm_select_player_def wm_select_players[2] = {"]
    for p in data["players"]:
        lines.append(f"    {{{p['start']}, {{{p['mug'][0]}, {p['mug'][1]}}}, "
                     f"{'true' if p['flip'] else 'false'}, 0x{p['move_sound']:04x}, 0x{p['select_sound']:04x}}},")
    lines += ["};", "", "const wm_select_bmod_entry wm_select_background_modules[2] = {"]
    for name,x,y in data["bmods"]:
        lines.append(f'    {{"{name}", {x}, {y}}},')
    lines += ["};", ""]
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines))


def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument('--source', required=True, type=pathlib.Path)
    ap.add_argument('--out', required=True, type=pathlib.Path)
    ns=ap.parse_args()
    data=parse(ns.source.read_text(errors='replace'))
    emit(data, ns.out)
    print(f"select source: {len(data['croutons'])} slots, {len(data['attributes'])} source wrestlers")
    return 0

if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"select_source: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
