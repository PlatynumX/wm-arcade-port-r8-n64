#!/usr/bin/env python3
"""Emit each animation's inline ANIM.ASM motion commands as a C table.

tools/wlanim.py extracts the frames an animation shows; this extracts the
motion commands sitting *between* them -- ANI_OFFSET, ANI_SET_XVEL /
ANI_SET_YVEL / ANI_SET_ZVEL, ANI_ZEROVELS, ANI_ZERO_XZVELS, ANI_MIN_YVEL,
ANI_FRICTION -- with the 0-based index of the frame each one precedes.
Without them a wired attack plays its real frames and its real hit box but
never moves, which is why every strike in this port is currently
stationary.

Rows are keyed on the animation's own source label rather than a port-side
enum id, so the same generated table serves any wrestler whose sequences
are extracted -- there is nothing Bret-specific here.

Operands are resolved, never guessed:
  - 16.16 fixed hex (`38000h`), decimal, and sums of those are evaluated
  - AM_ABS / AM_FACE_REL / AM_HIT_REL / AM_NEWFACE_REL resolve to
    ANIM.EQU's own 0/1/2/3
  - a local `#name equ value` is tracked as the walk passes it, so uses of
    `#yoff` pick up the definition in scope (they are redefined per routine
    -- 57, 50 and 37 all appear in HRTSEQ2.ASM alone)
Anything else raises rather than emitting a made-up number.
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim  # noqa: E402

# ANIM.EQU:167-170
AM_MODES = {"AM_ABS": 0, "AM_FACE_REL": 1, "AM_HIT_REL": 2, "AM_NEWFACE_REL": 3}

# kind, operand count (mode operand counted separately)
COMMANDS = {
    "ANI_ZEROVELS":    ("ZEROVELS", 0, False),
    "ANI_ZERO_XZVELS": ("ZERO_XZVELS", 0, False),
    "ANI_SET_XVEL":    ("SET_XVEL", 1, True),
    "ANI_SET_YVEL":    ("SET_YVEL", 1, False),
    "ANI_SET_ZVEL":    ("SET_ZVEL", 1, True),
    "ANI_MIN_YVEL":    ("MIN_YVEL", 1, False),
    "ANI_FRICTION":    ("FRICTION", 1, False),
    "ANI_OFFSET":      ("OFFSET", 3, False),
    # Instant state commands with no new subsystem behind them.
    "ANI_SETSPEED":    ("SETSPEED", 1, False),
    "ANI_STARTATTACK": ("STARTATTACK", 2, False),
    "ANI_FACEUP":      ("FACEUP", 0, False),
    "ANI_FACEDOWN":    ("FACEDOWN", 0, False),
    "ANI_SET_WRESTLER_XFLIP": ("SET_WRESTLER_XFLIP", 0, False),
    "ANI_CLR_BUTCOUNT": ("CLR_BUTCOUNT", 0, False),
    "ANI_SAFE_TIME":   ("SAFE_TIME", 1, False),
    "ANI_GRAVITY_ON":  ("GRAVITY_ON", 0, False),
    "ANI_CLR_STATUS":  ("CLR_STATUS", 0, False),
}

# ANIM.ASM:71 _ani_ifbuttons -- "if EVERY named button is currently held,
# this animation becomes the target one". Every occurrence across HRTSEQ2-4
# is the same case, PLAYER_PUNCH_VAL|PLAYER_KICK_VAL -> start_run_anim: the
# run cancel out of an attack's opening frames. Its operands are a button
# mask and an animation label rather than numbers, so it gets its own row
# kind rather than being squeezed into the motion table.
IFBUTTONS_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_IFBUTTONS\s*,\s*([^,]+)\s*,\s*"
    r"([A-Za-z_][A-Za-z0-9_]*)\s*$", re.I)

# GAME.EQU:407/411/416
PLAYER_BUTTON_BITS = {"PLAYER_PUNCH_VAL": 1 << 0, "PLAYER_BLOCK_VAL": 1 << 1,
                      "PLAYER_KICK_VAL": 1 << 3}


def _button_mask(expr: str) -> int:
    mask = 0
    for tok in expr.split("|"):
        tok = tok.strip().upper()
        if tok not in PLAYER_BUTTON_BITS:
            raise ValueError(f"unknown button value {tok!r}")
        mask |= PLAYER_BUTTON_BITS[tok]
    return mask

CMD_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+(" + "|".join(COMMANDS) + r")\b\s*,?\s*(.*)$", re.I)
EQU_RE = re.compile(r"^\s*(#[A-Za-z_][A-Za-z0-9_]*)\s+equ\s+(.+)$", re.I)
HEX_RE = re.compile(r"\b([0-9A-Fa-f]+)h\b")


# The `.EQU` reader and the table of plain global constants live in
# wlanim, so the tick-count path and the operand path resolve a symbol the
# same way rather than from two drifting copies.
_load_equ = wlanim.load_equ
_ORIG = wlanim.ORIG

# DAMAGE.EQU:174+ AT_* attack types, the operand ANI_STARTATTACK names.
AT_TYPES = _load_equ(_ORIG / "DAMAGE.EQU", "AT_")

GLOBAL_EQU = wlanim.GLOBAL_EQU


def _value(tok: str, equates: dict[str, int]) -> int:
    tok = tok.strip()
    if tok.upper() in AM_MODES:
        return AM_MODES[tok.upper()]
    if tok.upper() in AT_TYPES:
        return AT_TYPES[tok.upper()]
    expr = tok
    # Only touch the symbol tables when the operand actually names a symbol
    # -- most are plain numbers, and substituting several hundred globals
    # into every one of them is what made a roster-wide sweep crawl.
    if re.search(r"[A-Za-z_#]", expr):
        for name, val in equates.items():
            if name in expr:
                expr = re.sub(re.escape(name) + r"\b", str(val), expr)
        for name in re.findall(r"\b[A-Za-z_][A-Za-z0-9_]*\b", expr):
            if name in GLOBAL_EQU:
                expr = re.sub(r"\b" + re.escape(name) + r"\b",
                              str(GLOBAL_EQU[name]), expr)
    expr = HEX_RE.sub(lambda m: "0x" + m.group(1), expr)
    # Operands are written as small arithmetic expressions: `5-10`,
    # `-1+15`, `60*60`, `TSEC*60`. Multiplication is as real as the rest.
    if not re.fullmatch(r"[-+*0-9xXa-fA-F() \t]+", expr):
        raise ValueError(f"unresolved operand {tok!r}")
    return int(eval(expr, {"__builtins__": {}}, {}))


def commands_for(path: pathlib.Path, label: str):
    """[(frame_index, kind, mode, a, b, c)] for one animation."""
    lines = [wlanim.strip_comment(r)
             for r in path.read_text(errors="replace").splitlines()]
    order = wlanim.slice_line_order(lines, label)

    equates: dict[str, int] = {}
    # Local equates are scoped and redefined per routine, so seed from every
    # definition that precedes this routine's own start as well as those
    # inside it -- the most recent one before a use always wins.
    start = order[0] if order else 0
    for i in range(0, start):
        m = EQU_RE.match(lines[i])
        if m:
            try:
                equates[m.group(1)] = _value(m.group(2), equates)
            except ValueError:
                pass

    out = []
    nframes = 0
    for idx in order:
        line = lines[idx]
        if not line:
            continue
        m = EQU_RE.match(line)
        if m:
            equates[m.group(1)] = _value(m.group(2), equates)
            continue
        if wlanim._frame_from_line(line):
            nframes += 1
            continue
        if wlanim.END_RE.match(line) or wlanim.REPEAT_RE.match(line):
            break
        if wlanim.CHANGEANIM_RE.match(line):
            if not any(wlanim.END_RE.match(lines[j])
                       for j in order[order.index(idx) + 1:]):
                break
            continue
        b = IFBUTTONS_RE.match(line)
        if b:
            # (frame, kind, mode, a, b, c, target): the button mask is an
            # operand, not a mode -- putting it in `mode` left `a` at 0 and
            # made `(but_val_cur & 0) == 0` fire on every frame.
            out.append((nframes, "IFBUTTONS", 0,
                        _button_mask(b.group(1)), 0, 0, b.group(2)))
            continue
        c = CMD_RE.match(line)
        if not c:
            continue
        kind, nargs, has_mode = COMMANDS[c.group(1).upper()]
        args = [a for a in (x.strip() for x in c.group(2).split(",")) if a]
        vals = [_value(a, equates) for a in args]
        mode = 0
        if has_mode and len(vals) > nargs:
            mode = vals[nargs]
        vals = (vals + [0, 0, 0])[:3]
        out.append((nframes, kind, mode, vals[0], vals[1], vals[2], None))
    return out


def render(entries: list[tuple[str, str, str]]) -> str:
    out = ["/* Auto-generated by tools/wlcommands.py from inline ANIM.ASM",
           "   motion commands. Frame indices match the wm_visual_sequence",
           "   tools/wlanim.py extracts from the same routine. */",
           '#include "wm/anim_frame_commands.h"',
           "",
           "static const wm_anim_frame_command commands[] = {"]
    total = 0
    for source_name, label, _sym in entries:
        rows = commands_for(pathlib.Path(source_name), label)
        if not rows:
            continue
        out.append(f'    /* {label} ({pathlib.Path(source_name).name}) */')
        for frame, kind, mode, a, b, c, target in rows:
            tgt = f'"{target}"' if target else "0"
            out.append(f'    {{ "{label}", {frame}, WM_ANICMD_{kind}, {mode}, '
                       f'{a}, {b}, {c}, {tgt} }},')
            total += 1
    out += ["};", "",
            "const wm_anim_frame_command *wm_anim_frame_commands(size_t *count) {",
            "    *count = sizeof(commands) / sizeof(commands[0]);",
            "    return commands;",
            "}",
            ""]
    print(f"wrote {total} motion commands", file=sys.stderr)
    return "\n".join(out)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--animation", nargs=2, metavar=("SOURCE", "LABEL"),
                    action="append", required=True)
    ap.add_argument("--out", required=True)
    ns = ap.parse_args()
    entries = [(s, l, l) for s, l in ns.animation]
    pathlib.Path(ns.out).write_text(render(entries))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError) as exc:
        print(f"wlcommands: error: {exc}", file=sys.stderr)
        sys.exit(1)
