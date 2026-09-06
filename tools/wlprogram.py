#!/usr/bin/env python3
"""Emit an animation as the PROGRAM it actually is, branches included.

tools/wlanim.py flattens a routine into the frames it shows and
tools/wlcommands.py lifts the commands out beside them, both keyed on a
frame index. That model cannot represent a branch: a routine that plays
different frames on a hit than on a miss gets linearised into one list that
no single playthrough ever plays, and the port has been accepting the
longest path as an approximation ever since.

This emits the real thing -- an ordered op stream with resolved branch
targets, which an interpreter walks the way ANIM.ASM's own does:

    FRAME name,ticks        show a frame for N ticks, then continue
    <command>               instant, as ANIM.ASM's own opcode
    IFSTATUS   -> index     ANIM.ASM:29  branch if MODE_STATUS set
    IFNOTSTATUS-> index     ANIM.ASM:45  branch if MODE_STATUS clear
    IFBLOCKED  -> index     ANIM.ASM:83  branch if HITBLOCKER nonzero
    GOTO       -> index     unconditional
    IF_RPTCOUNT-> index     ANIM.ASM:90  branch while RPT_COUNT nonzero
    CHANGEANIM -> label     become another animation
    END                     stop

Branch targets are resolved to op indices here, so the interpreter never
needs the source text. Local labels are scoped, so a target resolves to the
definition inside this routine's own body.

Anything the emitter cannot represent raises rather than being dropped
silently -- a branch to a label outside the body included, since quietly
turning that into a fallthrough would invent a playthrough.
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim      # noqa: E402
import wlcommands  # noqa: E402

BRANCHES = {
    "ANI_IFSTATUS":    "IFSTATUS",
    "ANI_IFNOTSTATUS": "IFNOTSTATUS",
    "ANI_IFBLOCKED":   "IFBLOCKED",
    "ANI_IF_RPTCOUNT": "IF_RPTCOUNT",
    "ANI_GOTO":        "GOTO",
}
BRANCH_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+(" + "|".join(BRANCHES) + r")\s*,\s*"
    r"(#?[A-Za-z_][A-Za-z0-9_]*)\s*$", re.I)

# ANI_SLIDE_BACK range,xvel,#no_slide -- a conditional forward branch taken
# when MODE_STATUS is CLEAR (ANIM.ASM: "was there a collision? jrz #no_slide").
SLIDE_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_SLIDE_BACK\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*"
    r"(#?[A-Za-z_][A-Za-z0-9_]*)\s*$", re.I)

SET_RPT_RE = wlanim.SET_RPT_RE

# The rest of what an animation does: attack boxes and the mode/facing
# commands the backend had been carrying in side tables keyed on a frame
# index. In a program they are just ops, which is what they are in the
# source, so the side tables stop being needed.
AMODE_TYPES = wlcommands._load_equ(wlcommands._ORIG / "PLYR.EQU", "AMODE_")
MODE_BITS = wlcommands._load_equ(wlcommands._ORIG / "ANIM.EQU", "MODE_")
PLYR_MODES = wlcommands._load_equ(wlcommands._ORIG / "PLYR.EQU", "MODE_")

ATTACK_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+(ANI_ATTACK_ON_Z|ANI_ATTACK_ON)\s*,\s*(.*)$", re.I)
SIMPLE_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+(ANI_ATTACK_OFF|ANI_SETFACING|ANI_XFLIP)\s*$", re.I)
SETMODE_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+(ANI_SETMODE|ANI_SETPLYRMODE)\s*,\s*(.+)$", re.I)


def _mode_value(expr: str, table: dict) -> int:
    total = 0
    for tok in expr.split("|"):
        tok = tok.strip().upper()
        if tok not in table:
            raise ValueError(f"unknown mode {tok!r}")
        total |= table[tok]
    return total
DEC_RPT_RE = re.compile(r"^\s*(?:\.word|W+L+W*)\s+ANI_DEC_RPTCOUNT\s*$", re.I)


def _body(lines: list[str], label: str) -> tuple[int, int]:
    span = wlanim._routine_span(lines, label)
    if span is None:
        raise ValueError(f"no routine {label}")
    return span


def program_for(path: pathlib.Path, label: str):
    """[(op, *args)] with branch targets resolved to op indices."""
    lines = [wlanim.strip_comment(r)
             for r in path.read_text(errors="replace").splitlines()]
    start, stop = _body(lines, label)
    # A routine that runs on into another (no ANI_END of its own) can also
    # BRANCH into it -- hrt_4_knee_to_head_anim's #skip_run_check and
    # hrt_faceup_getup_anim's #common_4 both live in the body they continue
    # into. Extend the program to the end of the chained walk so those
    # targets exist, rather than refusing a branch that is perfectly real.
    try:
        chained = wlanim.slice_line_order(lines, label)
        if chained:
            stop = max(stop, max(chained) + 1)
    except ValueError:
        pass

    # A branch can also target shared tail code that sits after the
    # routine's own ANI_END -- hrt_faceup_getup_anim's #common_4, the
    # #missed blocks several attacks share. Grow the body until every
    # target a branch names is inside it, since that is what the assembler
    # resolves against. Iterated because the region pulled in can branch
    # onward itself.
    while True:
        wanted = set()
        for i in range(start, stop):
            bm = BRANCH_RE.match(lines[i]) or SLIDE_RE.match(lines[i])
            if bm:
                wanted.add(bm.group(bm.re.groups))
        have = {m.group(1) for m in
                (wlanim.LOCAL_LABEL_RE.match(lines[i]) for i in range(start, stop))
                if m}
        missing = wanted - have
        if not missing:
            break
        grew = stop
        for name in missing:
            for i in range(stop, len(lines)):
                m = wlanim.LOCAL_LABEL_RE.match(lines[i])
                if m and m.group(1) == name:
                    # Include the code the label names, not just the label
                    # line: stopping at the label itself resolves the branch
                    # to one past the last op, which is not a real target.
                    j = i
                    while j < len(lines):
                        if wlanim.SUBR_RE.match(lines[j]) and j > i:
                            break
                        if (wlanim.END_RE.match(lines[j]) or
                                wlanim.REPEAT_RE.match(lines[j])):
                            j += 1
                            break
                        j += 1
                    grew = max(grew, j)
                    break
        if grew == stop:
            break
        stop = grew

    equates: dict[str, int] = {}
    for i in range(0, start):
        m = wlcommands.EQU_RE.match(lines[i])
        if m:
            try:
                equates[m.group(1)] = wlcommands._value(m.group(2), equates)
            except ValueError:
                pass

    ops: list[tuple] = []
    label_at: dict[str, int] = {}   # local label -> op index
    fixups: list[tuple[int, str]] = []

    for i in range(start, stop):
        line = lines[i]
        if not line:
            continue

        lm = wlanim.LOCAL_LABEL_RE.match(line)
        if lm:
            label_at.setdefault(lm.group(1), len(ops))
            # a label can share its line with an instruction, so fall through

        m = wlcommands.EQU_RE.match(line)
        if m:
            equates[m.group(1)] = wlcommands._value(m.group(2), equates)
            continue

        frame = wlanim._frame_from_line(line)
        if frame:
            ops.append(("FRAME", frame.name, frame.ticks))
            continue

        if wlanim.END_RE.match(line):
            ops.append(("END",))
            continue
        if wlanim.REPEAT_RE.match(line):
            ops.append(("REPEAT",))
            continue

        cm = wlanim.CHANGEANIM_RE.match(line)
        if cm:
            ops.append(("CHANGEANIM", cm.group(1)))
            continue

        bm = BRANCH_RE.match(line)
        if bm:
            fixups.append((len(ops), bm.group(2)))
            ops.append((BRANCHES[bm.group(1).upper()], -1))
            continue

        sm = SLIDE_RE.match(line)
        if sm:
            fixups.append((len(ops), sm.group(3)))
            ops.append(("SLIDE_BACK", -1,
                        wlcommands._value(sm.group(1), equates),
                        wlcommands._value(sm.group(2), equates)))
            continue

        ib = wlcommands.IFBUTTONS_RE.match(line)
        if ib:
            ops.append(("IFBUTTONS", wlcommands._button_mask(ib.group(1)),
                        ib.group(2)))
            continue

        rm = SET_RPT_RE.match(line)
        if rm:
            ops.append(("SET_RPTCOUNT", int(rm.group(1))))
            continue
        if DEC_RPT_RE.match(line):
            ops.append(("DEC_RPTCOUNT",))
            continue

        am = ATTACK_RE.match(line)
        if am:
            args = [a.strip() for a in am.group(2).split(",") if a.strip()]
            mode = wlcommands._value(args[0], equates) if args[0].upper() not in AMODE_TYPES \
                else AMODE_TYPES[args[0].upper()]
            nums = [wlcommands._value(a, equates) for a in args[1:]]
            nums = (nums + [0] * 6)[:6]
            ops.append(("ATTACK_ON_Z" if am.group(1).upper().endswith("_Z")
                        else "ATTACK_ON", mode, *nums))
            continue

        sm2 = SIMPLE_RE.match(line)
        if sm2:
            ops.append((sm2.group(1).upper()[4:],))
            continue

        mm = SETMODE_RE.match(line)
        if mm:
            if mm.group(1).upper() == "ANI_SETMODE":
                ops.append(("SETMODE", _mode_value(mm.group(2), MODE_BITS)))
            else:
                ops.append(("SETPLYRMODE", _mode_value(mm.group(2), PLYR_MODES)))
            continue

        cmd = wlcommands.CMD_RE.match(line)
        if cmd:
            kind, nargs, has_mode = wlcommands.COMMANDS[cmd.group(1).upper()]
            args = [a for a in (x.strip() for x in cmd.group(2).split(",")) if a]
            vals = [wlcommands._value(a, equates) for a in args]
            mode = vals[nargs] if has_mode and len(vals) > nargs else 0
            vals = (vals + [0, 0, 0])[:3]
            ops.append((kind, mode, vals[0], vals[1], vals[2]))
            continue
        # Everything else is a command this port does not model yet. It is
        # skipped, exactly as the flat extractor skips it -- but unlike the
        # flat extractor, skipping it here cannot corrupt the frame ORDER,
        # because order comes from the branches, which are all represented.

    for at, target in fixups:
        if target not in label_at:
            raise ValueError(
                f"{label}: branch to {target}, which is not defined inside "
                f"this routine -- the animation continues somewhere this "
                f"emitter cannot follow")
        op = ops[at]
        ops[at] = (op[0], label_at[target]) + op[2:]

    if not any(o[0] == "FRAME" for o in ops):
        raise ValueError(f"{label}: no frames")
    return ops


BRANCH_OPS = {"GOTO", "IFSTATUS", "IFNOTSTATUS", "IFBLOCKED", "IF_RPTCOUNT",
              "SLIDE_BACK"}
# Ops carrying (mode, a, b, c, d, e) from wlcommands' 5-tuple shape.
MOTION_OPS = {"ZEROVELS", "ZERO_XZVELS", "SET_XVEL", "SET_YVEL", "SET_ZVEL",
              "MIN_YVEL", "FRICTION", "OFFSET", "SETSPEED", "STARTATTACK",
              "FACEUP", "FACEDOWN", "SET_WRESTLER_XFLIP", "CLR_BUTCOUNT",
              "SAFE_TIME", "GRAVITY_ON", "CLR_STATUS"}


def _c_op(op) -> str:
    kind = op[0]
    mode = 0
    target = -1
    args = [0] * 6
    text = "0"
    if kind == "FRAME":
        text = f'"{op[1]}"'
        args[0] = op[2]
    elif kind in BRANCH_OPS:
        target = op[1]
        if kind == "SLIDE_BACK":
            args[0], args[1] = op[2], op[3]
    elif kind in ("CHANGEANIM",):
        text = f'"{op[1]}"'
    elif kind == "IFBUTTONS":
        args[0] = op[1]
        text = f'"{op[2]}"'
    elif kind in ("SET_RPTCOUNT",):
        args[0] = op[1]
    elif kind in ("SETMODE", "SETPLYRMODE"):
        args[0] = op[1]
    elif kind in ("ATTACK_ON", "ATTACK_ON_Z"):
        mode = op[1]
        for i, v in enumerate(op[2:8]):
            args[i] = v
    elif kind in MOTION_OPS:
        mode = op[1]
        for i, v in enumerate(op[2:5]):
            args[i] = v
    a = ", ".join(str(v) for v in args)
    return (f"    {{ WM_AOP_{kind}, {mode}, {target}, {a}, {text} }},")


def render_c(entries) -> str:
    out = ["/* Auto-generated by tools/wlprogram.py: each animation as the",
           "   op stream ANIM.ASM actually runs, branch targets resolved to",
           "   op indices. */",
           '#include "wm/anim_program.h"',
           ""]
    names = []
    for source, label in entries:
        ops = program_for(pathlib.Path(source), label)
        sym = "prog_" + label
        out.append(f"static const wm_anim_op {sym}_ops[] = {{")
        for op in ops:
            out.append(_c_op(op))
        out += ["};", ""]
        names.append((label, pathlib.Path(source).name, sym))
    out.append("static const wm_anim_program programs[] = {")
    for label, src, sym in names:
        out.append(f'    {{ "{label}", "{src}", {sym}_ops,')
        out.append(f'      sizeof({sym}_ops) / sizeof({sym}_ops[0]) }},')
    out += ["};", "",
            "const wm_anim_program *wm_anim_program_find(const char *source_label) {",
            "    size_t i;",
            "    if (!source_label) return 0;",
            "    for (i = 0; i < sizeof(programs)/sizeof(programs[0]); ++i)",
            "        if (__builtin_strcmp(programs[i].source_label, source_label) == 0)",
            "            return &programs[i];",
            "    return 0;",
            "}",
            ""]
    return "\n".join(out)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True)
    ap.add_argument("--label", required=True, action="append")
    ap.add_argument("--out")
    ns = ap.parse_args(argv)
    if ns.out:
        pathlib.Path(ns.out).write_text(
            render_c([(ns.source, l) for l in ns.label]))
        return 0
    for label in ns.label:
        ops = program_for(pathlib.Path(ns.source), label)
        print(f"{label}  ({len(ops)} ops)")
        for i, op in enumerate(ops):
            print(f"    {i:3d}  {op[0]:<20} {op[1:]}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError) as exc:
        print(f"wlprogram: error: {exc}", file=sys.stderr)
        sys.exit(1)
