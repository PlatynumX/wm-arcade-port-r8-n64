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
import wlpuppet    # noqa: E402

BRANCHES = {
    "ANI_IFSTATUS":    "IFSTATUS",
    "ANI_IFNOTSTATUS": "IFNOTSTATUS",
    "ANI_IFBLOCKED":   "IFBLOCKED",
    "ANI_IF_RPTCOUNT": "IF_RPTCOUNT",
    # ANIM.ASM:91 -- the same branch inverted: taken when RPT_COUNT
    # has run out rather than while it is still going.
    "ANI_IFNOT_RPTCOUNT": "IFNOT_RPTCOUNT",
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

# ANIM.ASM:3214/:3239 ANI_IF_BUTCOUNT_GE / _LT -- the button-mash branches:
#
#     WWWL ANI_IF_BUTCOUNT_LT,KICKB_COUNT,1,#exit
#
# The first operand is one of PLYR.EQU:152-156's five adjacent WORDs, which
# the source passes as a struct offset and reads with `add a13,a14`. They
# are declared in a fixed order the source itself marks "keep ordered",
# because WRESTLE.ASM:4681 count_button_presses walks them by bit position;
# that order is the index carried in the op.
BUTCOUNT_FIELDS = {
    "PUNCHB_COUNT": 0,
    "BLOCKB_COUNT": 1,
    "SPUNCHB_COUNT": 2,
    "KICKB_COUNT": 3,
    "SKICKB_COUNT": 4,
}
# ANIM.ASM:1277 ANI_CODE,<routine> -- an ordinary subroutine call. Carried
# by name: src/core/anim_code.c translates the ones this port has, and the
# rest stay in the program as named, countable gaps rather than lines the
# extractor drops on the floor.
CODE_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_CODE\s*,\s*"
    r"(#?[A-Za-z_][A-Za-z0-9_]*)\s*$", re.I)

# ANIM.ASM:119 -- branch when RPT_COUNT is at least the operand.
RPTGE_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_IF_RPTCOUNT_GE\s*,\s*([^,]+)\s*,\s*"
    r"(#?[A-Za-z_][A-Za-z0-9_]*)\s*$", re.I)

BUTCOUNT_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+(ANI_IF_BUTCOUNT_GE|ANI_IF_BUTCOUNT_LT)\s*,\s*"
    r"([A-Za-z_][A-Za-z0-9_]*)\s*,\s*([^,]+)\s*,\s*"
    r"(#?[A-Za-z_][A-Za-z0-9_]*)\s*$", re.I)

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
    r"^\s*(?:\.word|W+L+W*)\s+(ANI_ATTACK_OFF|ANI_SETFACING|ANI_XFLIP"
    r"|ANI_SET_ATTACH|ANI_DETACH)\s*$", re.I)

# The attach machinery's operand-carrying ops. ANI_ATTACHZ is written
# `.word ANI_ATTACHZ,x,y,z`; the source reads x and y as one long and z as a
# word, which lands on PLYR.EQU's three adjacent ATTACH_XOFF/YOFF/ZOFF words
# and means exactly what it reads like.
ATTACH_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+(ANI_ATTACHZ)\s*,\s*([^,]+),\s*([^,]+),\s*"
    r"([^,]+)\s*$", re.I)
SETOPPVELS_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_SETOPPVELS\s*,\s*([^,]+),\s*([^,]+),\s*"
    r"([^,]+)\s*$", re.I)
DAMAGEOPP_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_DAMAGEOPP\s*,\s*([^,]+),\s*([^,]+)\s*$", re.I)
OPPMODE_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+(ANI_SETOPPMODE|ANI_CLROPPMODE"
    r"|ANI_IMMOBILIZE)\s*,\s*(.+)$", re.I)
# ANI_IFOPPMODE mode,#branch -- the high bit of the mode inverts the test.
IFOPPMODE_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_IFOPPMODE\s*,\s*([^,]+),\s*"
    r"(#?[A-Za-z_][A-Za-z0-9_]*)\s*$", re.I)
SETMODE_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+(ANI_SETMODE|ANI_SETPLYRMODE)\s*,\s*(.+)$", re.I)


def _mode_value(expr: str, table: dict) -> int:
    # ANI_IFOPPMODE's own negative form: the source writes `~MODE_ONGROUND`
    # and reads it back as "jump on PLYRMODE != ~#MODE", so the operand is
    # the ones' complement of the mode it means.
    stripped = expr.strip()
    if stripped.startswith("~"):
        return ~_mode_value(stripped[1:], table)

    # Written with either separator: MODE_UNINT|MODE_NOAUTOFLIP in most
    # files, MODE_UNINT+MODE_NOAUTOFLIP throughout ADMSEQ.
    total = 0
    for tok in re.split(r"[|+]", expr):
        tok = tok.strip().upper()
        if tok in table:
            total |= table[tok]
            continue
        # The assembler has one flat symbol table, so an operand naming a
        # MODE_* from the *other* file still assembles -- RZRSEQ2.ASM:2203
        # writes `ANI_SETMODE,MODE_INAIR`, and since MODE_INAIR is
        # PLYR.EQU's player-mode ordinal 2, the shipped ROM sets ANIM.EQU's
        # 02h, MODE_INTURN. Resolving the same way it does keeps the port
        # faithful to the game rather than to what the line looks like it
        # meant.
        if tok in wlanim.GLOBAL_EQU:
            total |= wlanim.GLOBAL_EQU[tok]
            continue
        raise ValueError(f"unknown mode {tok!r}")
    return total
# ANIM.ASM:3913 _ani_setlong writes a LONG straight into the wrestler
# process at a field offset. Across the eight playable wrestlers it names
# exactly two fields: OBJ_GRAVITY (115 uses -- an animation choosing its own
# fall rate) and DEBRIS_X (148, the renderer's). Both are real fields on the
# port's actor, so both are emitted; a THIRD field would be a silent hole,
# so anything else refuses.
SETLONG_FIELDS = {"OBJ_GRAVITY": 0, "DEBRIS_X": 1}
# ANIM.ASM:3512 _ani_setword, the WORD counterpart. Across the eight
# playable wrestlers it names exactly three fields.
SETWORD_FIELDS = {"USR_VAR1": 0, "USR_VAR2": 1, "DELAY_METER": 2}
# ANIM.ASM:49 _ani_checkword reads the same fields ANI_SETWORD writes
# and sets or clears MODE_STATUS from whether the word is non-zero.
CHECKWORD_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_CHECKWORD\s*,\s*(\w+)\s*$", re.I)
# ANIM.ASM:86 _ani_ifopp takes a VARIABLE-length list of wrestler
# numbers terminated by -1: "sets STATUS if opponent is one of the
# wrestlers in the list, else clears".
IFOPP_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_IFOPP\s*,\s*(.+)$", re.I)
SETWORD_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_SETWORD\s*,\s*(\w+)\s*,\s*([^,]+)\s*$", re.I)
SETLONG_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_SETLONG\s*,\s*(\w+)\s*,\s*([^,]+)\s*$", re.I)

DEC_RPT_RE = re.compile(r"^\s*(?:\.word|W+L+W*)\s+ANI_DEC_RPTCOUNT\s*$", re.I)


def _body(lines: list[str], label: str) -> tuple[int, int]:
    span = wlanim._routine_span(lines, label)
    if span is None:
        raise ValueError(f"no routine {label}")
    return span


_FILE_EQU_CACHE: dict[str, dict] = {}


def _file_equates(path: pathlib.Path, lines: list[str]) -> dict:
    """Every local equate in the file, cached per file.

    Local equates are scoped and the in-scope pass overrides this, but some
    files define a constant only AFTER the routine that names it
    (ADMSEQ1.ASM's #RUN_SPD), so a whole-file sweep is the fallback. Cached
    because sweeping per routine is quadratic over 1500+ routines.
    """
    key = str(path)
    if key not in _FILE_EQU_CACHE:
        found: dict[str, int] = {}
        for line in lines:
            m = wlcommands.EQU_RE.match(line)
            if m:
                try:
                    found[m.group(1)] = wlcommands._value(m.group(2), found)
                except ValueError:
                    pass
        _FILE_EQU_CACHE[key] = found
    return _FILE_EQU_CACHE[key]


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
    #
    # ORDER MATTERS HERE, and it has to be the source's own. Growing the
    # body for one missing target can satisfy or move others, so which
    # target is considered first decides where the body ends up. Iterating
    # a set made that order depend on PYTHONHASHSEED -- the same routine
    # emitted 94 ops under one seed and 203 under another, and the
    # generated file therefore changed from run to run for no reason
    # visible in the source. Targets are taken in the order the routine
    # BRANCHES to them, which is stable and is also the order a reader
    # would resolve them in.
    #
    while True:
        wanted = []
        for i in range(start, stop):
            bm = (BRANCH_RE.match(lines[i]) or SLIDE_RE.match(lines[i])
                  or BUTCOUNT_RE.match(lines[i]))
            if bm:
                name = bm.group(bm.re.groups)
                if name not in wanted:
                    wanted.append(name)
        have = {name for name in
                (wlanim.label_def(lines[i]) for i in range(start, stop))
                if name}
        missing = [name for name in wanted if name not in have]
        if not missing:
            break
        # A target can also sit BEFORE this routine -- shared blocks earlier
        # in the file that several routines branch back into. Pull the
        # region in by starting the body there instead.
        for name in list(missing):
            for i in range(start - 1, -1, -1):
                if wlanim.label_def(lines[i]) == name:
                    start = i
                    missing.remove(name)
                    break
        if not missing:
            continue

        grew = stop
        for name in missing:
            for i in range(stop, len(lines)):
                if wlanim.label_def(lines[i]) == name:
                    # Include the code the label names, not just the label
                    # line: stopping at the label itself resolves the branch
                    # to one past the last op, which is not a real target.
                    j = i
                    while j < len(lines):
                        if wlanim.SUBR_RE.match(lines[j]) and j > i:
                            break
                        if (wlanim.END_RE.match(lines[j]) or
                                wlanim.REPEAT_RE.match(lines[j]) or
                                wlanim.ROT_RE.match(lines[j])):
                            j += 1
                            break
                        j += 1
                    grew = max(grew, j)
                    break
        if grew == stop:
            break
        stop = grew

    # Local equates are scoped, so definitions before this routine win --
    # but some files define a speed constant only after the routine that
    # names it (ADMSEQ1.ASM's #RUN_SPD), so the whole file is swept first
    # as a fallback and the in-scope pass overwrites it.
    equates = dict(_file_equates(path, lines))
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

        lm = wlanim.label_def(line)
        if lm:
            label_at.setdefault(lm, len(ops))
            # a label can share its line with an instruction, so fall through

        m = wlcommands.EQU_RE.match(line)
        if m:
            equates[m.group(1)] = wlcommands._value(m.group(2), equates)
            continue

        frame = wlanim._frame_from_line(line)
        if frame:
            # ANIM.ASM:2300 _ani_waithitopp, the source's own note: "This is
            # just like an ordinary WL ticks,frame type command except that
            # the ANICNT is zeroed if we hit the opponent." The handler only
            # sets MODE_WAITHITOPP and hands the same operands straight back
            # to the dispatcher, so one source line is two ops -- the mode
            # set, then the ordinary frame. wlanim's WAIT_FRAME_RE already
            # read the frame out of it; what was being dropped is the mode,
            # which is what lets landing cut the hold short.
            if wlanim.WAIT_FRAME_RE.match(line):
                ops.append(("WAITHITOPP",))
            ops.append(("FRAME", frame.name, frame.ticks))
            continue

        if wlanim.ROT_RE.match(line):
            ops.append(("ROT",))
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

        vm = SETOPPVELS_RE.match(line)
        if vm:
            ops.append(("SETOPPVELS", 0,
                        wlcommands._value(vm.group(1), equates),
                        wlcommands._value(vm.group(2), equates),
                        wlcommands._value(vm.group(3), equates)))
            continue

        dm = DAMAGEOPP_RE.match(line)
        if dm:
            ops.append(("DAMAGEOPP",
                        wlcommands._value(dm.group(1), equates),
                        wlcommands._value(dm.group(2), equates)))
            continue

        sm = wlpuppet.SLAVEANIM_RE.match(line)
        if sm:
            ops.append(("SLAVEANIM",
                        wlpuppet.slave_table_id_for(path, i, sm.group(1))))
            continue

        am = ATTACH_RE.match(line)
        if am:
            ops.append(("ATTACHZ", 0,
                        wlcommands._value(am.group(2), equates),
                        wlcommands._value(am.group(3), equates),
                        wlcommands._value(am.group(4), equates)))
            continue

        om = OPPMODE_RE.match(line)
        if om:
            kind = om.group(1).upper()[4:]
            table = MODE_BITS if kind in ("SETOPPMODE", "CLROPPMODE") else None
            ops.append((kind, _mode_value(om.group(2), table) if table
                        else wlcommands._value(om.group(2), equates)))
            continue

        io = IFOPPMODE_RE.match(line)
        if io:
            fixups.append((len(ops), io.group(2)))
            ops.append(("IFOPPMODE", -1, _mode_value(io.group(1), PLYR_MODES)))
            continue

        ss = wlpuppet.SUPERSLAVE2_RE.match(line)
        if ss:
            # The table is named by a local label the files reuse, so it is
            # resolved from THIS line rather than by name -- see
            # wlpuppet.table_line_for.
            ops.append(("SUPERSLAVE2",
                        wlanim.eval_ticks(ss.group(1)),
                        "%s%02d" % (ss.group(2).upper(), int(ss.group(3))),
                        wlpuppet.table_id_for(path, i, ss.group(4)),
                        int(ss.group(5))))
            continue

        cd = CODE_RE.match(line)
        if cd:
            ops.append(("CODE", cd.group(1)))
            continue

        rg = RPTGE_RE.match(line)
        if rg:
            fixups.append((len(ops), rg.group(2)))
            ops.append(("IF_RPTCOUNT_GE", -1,
                        wlcommands._value(rg.group(1), equates)))
            continue

        bc = BUTCOUNT_RE.match(line)
        if bc:
            field = bc.group(2).upper()
            if field not in BUTCOUNT_FIELDS:
                raise ValueError(
                    f"{label}: ANI_IF_BUTCOUNT names {bc.group(2)!r}, which is "
                    f"not one of PLYR.EQU's five button counters")
            fixups.append((len(ops), bc.group(4)))
            ops.append(("IF_BUTCOUNT_GE" if bc.group(1).upper().endswith("GE")
                        else "IF_BUTCOUNT_LT",
                        -1, BUTCOUNT_FIELDS[field],
                        wlcommands._value(bc.group(3), equates)))
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

        cw = CHECKWORD_RE.match(line)
        if cw:
            field = cw.group(1).upper()
            if field not in SETWORD_FIELDS:
                raise ValueError(
                    f"{label}: ANI_CHECKWORD reads {cw.group(1)!r}, a process "
                    f"field this port does not model")
            ops.append(("CHECKWORD", SETWORD_FIELDS[field]))
            continue

        io = IFOPP_RE.match(line)
        if io:
            nums = []
            for tok in io.group(1).split(","):
                tok = tok.strip()
                if not tok:
                    continue
                v = wlcommands._value(tok, equates)
                if v < 0:
                    break            # the list's own -1 terminator
                nums.append(v)
            if not nums:
                raise ValueError(f"{label}: ANI_IFOPP names no wrestlers")
            ops.append(("IFOPP", nums))
            continue

        sw = SETWORD_RE.match(line)
        if sw:
            field = sw.group(1).upper()
            if field not in SETWORD_FIELDS:
                raise ValueError(
                    f"{label}: ANI_SETWORD writes {sw.group(1)!r}, a process "
                    f"field this port does not model")
            ops.append(("SETWORD", SETWORD_FIELDS[field],
                        wlcommands._value(sw.group(2), equates)))
            continue

        sl = SETLONG_RE.match(line)
        if sl:
            field = sl.group(1).upper()
            if field not in SETLONG_FIELDS:
                raise ValueError(
                    f"{label}: ANI_SETLONG writes {sl.group(1)!r}, a process "
                    f"field this port does not model")
            ops.append(("SETLONG", SETLONG_FIELDS[field],
                        wlcommands._value(sl.group(2), equates)))
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

    if not any(o[0] in ("FRAME", "SUPERSLAVE2") for o in ops):
        # A frameless body is usually a sign the span is wrong -- the
        # extractor ran off the end of a routine and collected commands
        # belonging to nobody. But a few real animations genuinely draw
        # nothing: ANIM.ASM's `wres_slave_anim` is four commands that park
        # the wrestler while his attacker's animation poses him, and
        # WRESTLE2.ASM's `start_run_flung` is an offset and a getup timer.
        # What separates those from a runaway span is that they reach their
        # own ANI_END; a runaway one never does.
        if not wlanim._routine_terminates(lines, (start, stop)):
            raise ValueError(f"{label}: no frames")
    return ops


BRANCH_OPS = {"GOTO", "IFSTATUS", "IFNOTSTATUS", "IFBLOCKED", "IF_RPTCOUNT",
              "IFNOT_RPTCOUNT",
              "SLIDE_BACK", "IF_BUTCOUNT_GE", "IF_BUTCOUNT_LT", "IFOPPMODE",
              "IF_RPTCOUNT_GE"}
# Ops carrying (mode, a, b, c, d, e) from wlcommands' 5-tuple shape.
# Every op that comes out of wlcommands' command table carries the same
# (kind, mode, a, b, c) shape, so this is DERIVED from that table rather
# than listed by hand. It used to be a hand-kept set, and anything added to
# the command table without also being added here fell through to the
# no-operand default and had its operands silently written out as zero --
# which is exactly what happened to ANI_BOUNCE and ANI_GETUP.
MOTION_OPS = {kind for kind, _nargs, _has_mode in wlcommands.COMMANDS.values()}


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
        if kind in ("SLIDE_BACK", "IF_BUTCOUNT_GE", "IF_BUTCOUNT_LT"):
            args[0], args[1] = op[2], op[3]
        elif kind == "IF_RPTCOUNT_GE":
            args[0] = op[2]
        elif kind == "IFOPPMODE":
            args[0] = op[2]
    elif kind == "SUPERSLAVE2":
        text = f'"{op[2]}"'
        args[0], args[1], args[2] = op[1], op[3], op[4]
    elif kind in ("CHANGEANIM", "CODE"):
        text = f'"{op[1]}"'
    elif kind == "IFBUTTONS":
        args[0] = op[1]
        text = f'"{op[2]}"'
    elif kind in ("SET_RPTCOUNT", "SETOPPMODE", "CLROPPMODE", "IMMOBILIZE"):
        args[0] = op[1]
    elif kind in ("ATTACHZ", "SETOPPVELS"):
        args[0], args[1], args[2] = op[2], op[3], op[4]
    elif kind == "DAMAGEOPP":
        args[0], args[1] = op[1], op[2]
    elif kind == "SLAVEANIM":
        args[0] = op[1]
    elif kind in ("SETLONG", "SETWORD"):
        args[0], args[1] = op[1], op[2]
    elif kind == "CHECKWORD":
        args[0] = op[1]
    elif kind == "IFOPP":
        # A bit per wrestler number, so the variable-length source list
        # becomes one operand: every use names two or fewer.
        mask = 0
        for n in op[1]:
            mask |= 1 << n
        args[0] = mask
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
    ap.add_argument("--source")
    ap.add_argument("--label", action="append", default=[])
    # An animation names its own file, so one run can emit programs that
    # span HRTSEQ2/3/4 -- the same shape tools/wlcommands.py takes.
    ap.add_argument("--animation", nargs=2, action="append", default=[],
                    metavar=("SOURCE", "LABEL"))
    # Every animation an ANI_SLAVEANIM table can hand a victim. Read out of
    # the tables (tools/wlpuppet.py) rather than listed here, so the two
    # cannot disagree about what the op is allowed to name.
    ap.add_argument("--slave-targets", action="store_true")
    ap.add_argument("--out")
    ns = ap.parse_args(argv)
    entries = [(src, lab) for src, lab in ns.animation]
    if ns.source:
        entries += [(ns.source, l) for l in ns.label]
    if ns.slave_targets:
        entries += [(str(p), lab) for p, lab in wlpuppet.slave_targets()]
    if not entries:
        ap.error("nothing to emit: pass --animation, or --source with --label")
    seen, unique = set(), []
    for src, lab in entries:
        if lab not in seen:
            seen.add(lab)
            unique.append((src, lab))
    entries = unique
    if ns.out:
        pathlib.Path(ns.out).write_text(render_c(entries))
        print(f"wrote {len(entries)} animation programs -> {ns.out}")
        return 0
    for source, label in entries:
        ops = program_for(pathlib.Path(source), label)
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
