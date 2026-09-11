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
import collections
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
    r"((?:#[A-Za-z0-9_]+|[A-Za-z_][A-Za-z0-9_]*))\s*$", re.I)

# ANI_SLIDE_BACK range,xvel,#no_slide -- a conditional forward branch taken
# when MODE_STATUS is CLEAR (ANIM.ASM: "was there a collision? jrz #no_slide").
SLIDE_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_SLIDE_BACK\s*,\s*([^,]+)\s*,\s*([^,]+)\s*,\s*"
    r"((?:#[A-Za-z0-9_]+|[A-Za-z_][A-Za-z0-9_]*))\s*$", re.I)

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
    r"((?:#[A-Za-z0-9_]+|[A-Za-z_][A-Za-z0-9_]*))\s*$", re.I)

# An operand can be the assembler's own 16.16 literal, `[106,-29]`, which
# has a COMMA IN IT -- so a trailing `[^,]+` silently fails to match the
# line and the whole command is dropped. 130 ANI_SETLONG commands were
# skipped for exactly that reason, all of them DEBRIS_X.
VALUE = r"(?:\[[^\]]*\]|[^,\[\]]+)"

# ANIM.ASM:3634 ANI_CREATEPROC,<proc>,<procid>,<w1>,<w2>,<w3>.
CREATEPROC_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_CREATEPROC\s*,\s*([A-Za-z_][\w]*),"
    r"\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+)\s*$", re.I)

# ANIM.ASM:3565 ANI_SHADOWTRAIL,<palette>,<rate>,<lifespan> -- and its OFF
# form, which is a SHORTER command (one word, not four) rather than a zero
# palette, because the routine branches before reading the rest.
SHADOWTRAIL_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_SHADOWTRAIL\s*,\s*([A-Za-z_][\w]*),"
    r"\s*([^,]+),\s*([^,]+)\s*$", re.I)
SHADOWTRAIL_OFF_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_SHADOWTRAIL\s*,\s*0\s*$", re.I)

# ANIM.ASM:2325 ANI_ATTCHIMAGE,<base>+FRn,<zoff> -- hang an extra image
# off the wrestler. `base` is a table of `.long` frame pointers and FRn is
# n*20h, one long a step, so the operand indexes it; the entry can itself
# be `.long 0`, meaning that frame has no attached image.
ATTCHIMAGE_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_ATTCHIMAGE\s*,\s*"
    r"(?:(0)|([A-Za-z0-9_#]+)\s*\+\s*FR([0-9]+))\s*,\s*([^,]+)\s*$", re.I)


def _image_table_entry(kept: list[str], base: str, index: int,
                       path: pathlib.Path):
    """`base+FRn` -> the frame name at index n, or None for a `.long 0`.

    A `#local` base is scoped like any other local label, so it resolves
    the same way an ANI_CODE target does.
    """
    at = None
    if base.startswith("#"):
        for i, line in enumerate(kept):
            m = wlanim.LOCAL_LABEL_RE.match(line)
            if m and m.group(1) == base:
                at = i
                break
    else:
        for i, line in enumerate(kept):
            if wlanim.label_def(wlanim.strip_comment(line)) == base:
                at = i
                break
    if at is None:
        raise ValueError(f"{path.name}: ANI_ATTCHIMAGE names {base}, "
                         f"which is not a table in this file")
    rows = []
    for line in kept[at + 1:]:
        body = wlanim.strip_comment(line)
        if not body:
            continue
        m = re.match(r"^\.long\s+(.+)$", body, re.I)
        if not m:
            break
        rows += [v.strip() for v in m.group(1).split(",") if v.strip()]
    if index >= len(rows):
        raise ValueError(f"{path.name}: {base}+FR{index} is past the "
                         f"table's {len(rows)} entries")
    return None if rows[index] == "0" else rows[index]


# ANIM.ASM:3324/:3340 ANI_DEBRIS / ANI_DEBRISAT,<%chance>,<shape>,<x,y,z>.
DEBRIS_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+(ANI_DEBRIS|ANI_DEBRISAT)\s*,\s*([^,]+),"
    r"\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+)\s*$", re.I)

# ANIM.ASM:1633 ANI_LEAPATPOS,<ticks>,<maxdist>,<x>,<y>,<z> -- jump so as
# to arrive at TGT_XOFF/YOFF/ZOFF in that many ticks.
LEAPATPOS_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_LEAPATPOS\s*,\s*([^,]+),\s*([^,]+),"
    r"\s*([^,]+),\s*([^,]+),\s*([^,]+)\s*$", re.I)

# ANIM.ASM:3838 ANI_SLIDEATOPP,<ticks>,<vel>,<maxz>,<trgt>,<x>,<y>,<z>.
SLIDEATOPP_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_SLIDEATOPP\s*,\s*([^,]+),\s*([^,]+),"
    r"\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+)\s*$", re.I)

# ANIM.ASM:2504/:2509 ANI_IFROPE / ANI_IFNOTROPE,<mode>,<distance>,<label>
# -- branch on there being (or not being) a rope within `distance`.
IFROPE_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+(ANI_IFROPE|ANI_IFNOTROPE)\s*,\s*([^,]+),"
    r"\s*([^,]+),\s*((?:#[A-Za-z0-9_]+|[A-Za-z_][A-Za-z0-9_]*))\s*$", re.I)

# Four commands that are their own operand, or have none at all.
SOLO_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+(ANI_SET_IDIOT|ANI_SCROLL_CTRL|ANI_LOOP|"
    r"ANI_START_DIZZY)\s*(?:,\s*([^,]+))?\s*$", re.I)

# The four screen/rope shakes. ANI_SHAKER (:31) takes SHAKER2's own
# "ticks and power" value; ANI_SHAKEALL (:55) and ANI_SHAKEROPES (:36) take
# a rope selector; ANI_SHAKECORNER (:77) takes nothing.
SHAKE_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+(ANI_SHAKER|ANI_SHAKEALL|ANI_SHAKEROPES|"
    r"ANI_SHAKECORNER)\s*(?:,\s*(" + VALUE + r"))?\s*$", re.I)

# ANIM.ASM:4389 ANI_DRAW_NAME,<index> -- the move name flashed on screen,
# an index into LIFEBAR.ASM's #message_tbl.
DRAWNAME_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_DRAW_NAME\s*,\s*([^,]+)\s*$", re.I)

# ANIM.ASM:3753 ANI_TARGET,<area1>,<area2>,<ATM_CLOSEST|ATM_FARTHEST> --
# aim at whichever of two body parts is nearer (or further), decided by the
# two wrestlers' flip bits rather than by distance.
TARGET_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_TARGET\s*,\s*([^,]+),\s*([^,]+),\s*"
    r"([^,]+)\s*$", re.I)

# ANIM.ASM:119 -- branch when RPT_COUNT is at least the operand.
RPTGE_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_IF_RPTCOUNT_GE\s*,\s*([^,]+)\s*,\s*"
    r"((?:#[A-Za-z0-9_]+|[A-Za-z_][A-Za-z0-9_]*))\s*$", re.I)

BUTCOUNT_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+(ANI_IF_BUTCOUNT_GE|ANI_IF_BUTCOUNT_LT)\s*,\s*"
    r"([A-Za-z_][A-Za-z0-9_]*)\s*,\s*([^,]+)\s*,\s*"
    r"((?:#[A-Za-z0-9_]+|[A-Za-z_][A-Za-z0-9_]*))\s*$", re.I)

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
    r"((?:#[A-Za-z0-9_]+|[A-Za-z_][A-Za-z0-9_]*))\s*$", re.I)
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
    r"^\s*(?:\.word|W+L+W*)\s+ANI_SETLONG\s*,\s*(\w+)\s*,\s*(" + VALUE +
    r")\s*$", re.I)

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


def _label_def(line: str) -> str | None:
    """The label this line defines, counting `SUBR name` as one.

    wlanim.label_def deliberately does not: its GLOBAL_LABEL_RE wants the
    name alone in column 0, because it is also what delimits a local
    label's scope, and treating every SUBR as a scope boundary is right
    for that job.

    Resolving a branch is the other job. ANIM.ASM:29 implements
    ANI_IFSTATUS as a raw write to the animation PC (`move a0,a4`), so a
    routine can branch to ANY address -- including the head of the next
    routine, which is spelled `SUBR name` rather than as a bare label.
    Three animations do exactly that:

        BAMSEQ2.ASM:3443 bam_faceup_getup_anim -> bam_4_faceup_getup_anim
        DNKSEQ2.ASM:2013 dnk_faceup_getup_anim -> dnk_2_faceup_getup_anim
        YOKSEQ3.ASM:3063 yok_combo_scissor_anim -> yok_overhd_slam_anim

    each to a SUBR later in its own file. Without this the emitter could
    not see the definition and refused all three, and because
    ANI_CHANGEANIM elsewhere names them, a wrestler knocked onto his back
    changed to an animation nothing had emitted and simply stopped.
    """
    got = wlanim.label_def(line)
    if got:
        return got
    m = wlanim.SUBR_RE.match(line)
    return m.group(1) if m else None


def program_for(path: pathlib.Path, label: str, with_entry: bool = False):
    """[(op, *args)] with branch targets resolved to op indices.

    With `with_entry`, returns (ops, entry) -- `entry` being the op the
    animation actually STARTS at, which is not always 0.
    """
    raw = path.read_text(errors="replace").splitlines()
    lines = [wlanim.strip_comment(r) for r in raw]
    # The same lines with their indentation intact. A local label's scope
    # runs between two NON-local labels, and telling those apart needs the
    # first column: strip_comment() lstrips, which makes an indented `rets`
    # read as a label and would cut a scope in half.
    kept = [r.split(";", 1)[0] for r in raw]
    start, stop = _body(lines, label)
    # Where the routine's OWN first line is. Growing the body backwards
    # below (for a branch into shared code that sits earlier in the file)
    # moves `start`, and without remembering this the program would begin
    # by playing the code it merely branches into. HRTSEQ4.ASM's
    # hrt_4_hitblock_anim is exactly that: it branches back into
    # hrt_4_block_anim's `#4block` hold, so its body starts eight ops
    # before its own first command.
    own_start = start
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
                  or BUTCOUNT_RE.match(lines[i])
                  or IFROPE_RE.match(lines[i]))
            if bm:
                name = bm.group(bm.re.groups)
                if name not in wanted:
                    wanted.append(name)
        have = {name for name in
                (_label_def(lines[i]) for i in range(start, stop))
                if name}
        missing = [name for name in wanted if name not in have]
        if not missing:
            break
        # A target can also sit BEFORE this routine -- shared blocks earlier
        # in the file that several routines branch back into. Pull the
        # region in by starting the body there instead.
        for name in list(missing):
            for i in range(start - 1, -1, -1):
                if _label_def(lines[i]) == name:
                    start = i
                    missing.remove(name)
                    break
        if not missing:
            continue

        grew = stop
        for name in missing:
            for i in range(stop, len(lines)):
                if _label_def(lines[i]) == name:
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
    entry = None                    # op index of the routine's own start

    for i in range(start, stop):
        line = lines[i]
        if entry is None and i >= own_start:
            entry = len(ops)
        if not line:
            continue

        lm = _label_def(line)
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

        cp = CREATEPROC_RE.match(line)
        if cp:
            ops.append(("CREATEPROC", cp.group(1),
                        wlcommands._value(cp.group(2), equates),
                        wlcommands._value(cp.group(3), equates),
                        wlcommands._value(cp.group(4), equates),
                        wlcommands._value(cp.group(5), equates)))
            continue

        stoff = SHADOWTRAIL_OFF_RE.match(line)
        if stoff:
            ops.append(("SHADOWTRAIL", 1, None, 0, 0))
            continue
        st = SHADOWTRAIL_RE.match(line)
        if st:
            ops.append(("SHADOWTRAIL", 0, st.group(1),
                        wlcommands._value(st.group(2), equates),
                        wlcommands._value(st.group(3), equates)))
            continue

        ai = ATTCHIMAGE_RE.match(line)
        if ai:
            zoff = wlcommands._value(ai.group(4), equates)
            if ai.group(1) is not None:
                # `move *a4+,a0,L / jrz #offimg` -- the operand itself is
                # zero, which takes a DIFFERENT exit: the frame is
                # cleared, but ATTIMG_LAST_FRAME and ATTACHIMG_ZOFF are
                # not written. Flagged so the VM can tell the two apart.
                ops.append(("ATTCHIMAGE", 1, zoff, None))
            else:
                ops.append(("ATTCHIMAGE", 0, zoff,
                            _image_table_entry(kept, ai.group(2),
                                               int(ai.group(3)), path)))
            continue

        db = DEBRIS_RE.match(line)
        if db:
            ops.append(("DEBRISAT" if db.group(1).upper() == "ANI_DEBRISAT"
                        else "DEBRIS",) + tuple(
                wlcommands._value(db.group(k), equates) for k in range(2, 7)))
            continue

        lp = LEAPATPOS_RE.match(line)
        if lp:
            ops.append(("LEAPATPOS",) + tuple(
                wlcommands._value(lp.group(k), equates) for k in range(1, 6)))
            continue

        sa = SLIDEATOPP_RE.match(line)
        if sa:
            ops.append(("SLIDEATOPP",
                        wlcommands._value(sa.group(2), equates),   # velocity
                        wlcommands._value(sa.group(4), equates),   # target
                        wlcommands._value(sa.group(1), equates)))  # max ticks
            continue

        ir = IFROPE_RE.match(line)
        if ir:
            fixups.append((len(ops), ir.group(4)))
            ops.append(("IFROPE" if ir.group(1).upper() == "ANI_IFROPE"
                        else "IFNOTROPE", -1,
                        wlcommands._value(ir.group(2), equates),
                        wlcommands._value(ir.group(3), equates)))
            continue

        so = SOLO_RE.match(line)
        if so:
            kind = so.group(1).upper()[4:]
            arg = so.group(2)
            ops.append((kind, wlcommands._value(arg, equates) if arg else 0))
            continue

        sk = SHAKE_RE.match(line)
        if sk:
            kind = sk.group(1).upper()
            arg = sk.group(2)
            ops.append((kind[4:],
                        wlcommands._value(arg, equates) if arg else 0))
            continue

        dn = DRAWNAME_RE.match(line)
        if dn:
            ops.append(("DRAW_NAME", wlcommands._value(dn.group(1), equates)))
            continue

        tg = TARGET_RE.match(line)
        if tg:
            ops.append(("TARGET",
                        wlcommands._value(tg.group(1), equates),
                        wlcommands._value(tg.group(2), equates),
                        wlcommands._value(tg.group(3), equates)))
            continue

        cd = CODE_RE.match(line)
        if cd:
            # Which DEFINITION this call site can see. `#name` is local to
            # the assembler's block, and the source defines the same name
            # more than once in one file with a different body each time --
            # BAMSEQ2.ASM's two `#set_zvel2` are -5.0 and +5.0. Emitting
            # only the name would make those indistinguishable, so the
            # resolved definition line rides along.
            ops.append(("CODE", cd.group(1),
                        wlanim.local_label_site(kept, i, cd.group(1))))
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

        for _kind, _re, _op in (("changeanim", wlpuppet.CHANGEANIM_TBL_RE,
                                 "CHANGEANIM_TBL"),
                                ("xflip", wlpuppet.XFLIP_TBL_RE, "XFLIP_TBL"),
                                ("oppoffset", wlpuppet.OPPOFFSET_RE,
                                 "OPPOFFSET")):
            _m = _re.match(line)
            if _m:
                ops.append((_op, wlpuppet.aux_table_id_for(
                    _kind, path, i, _m.group(1))))
                break
        else:
            _m = None
        if _m:
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
        #
        # Skipping is silent in the emitted C, so it is counted here
        # instead: SKIPPED is what `--census` reports, and it is the only
        # honest answer to "how much of the animation VM is left".
        _sk = SKIP_NAME_RE.match(line)
        if _sk:
            _n = _sk.group(1).upper()
            SKIPPED[_n] += 1
            if len(SKIPPED_WHERE[_n]) < 4:
                SKIPPED_WHERE[_n].append(f"{path.name}:{i + 1} {label}: "
                                         f"{line.strip()}")

    for at, target in fixups:
        if target not in label_at:
            raise ValueError(
                f"{label}: branch to {target}, which is not defined inside "
                f"this routine -- the animation continues somewhere this "
                f"emitter cannot follow")
        if label_at[target] >= len(ops):
            # The label is real, but it sits past the last op this body
            # emitted -- trailing ANI_CODE, a puppet table, an `equ`. A
            # branch there resolves to one past the end, which the
            # dispatcher reads as "the program is over" and the animation
            # dies before its first frame. That is exactly how ANI_IFROPE
            # shipped broken; refuse instead of emitting it again.
            raise ValueError(
                f"{label}: branch to {target}, which is defined after this "
                f"routine's last animation command -- one past the end is "
                f"not a target")
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
    if with_entry:
        # Only labels that name an op the animation can actually reach.
        #
        # A routine's trailing assembly, its puppet tables and its `equ`
        # lines all sit after the last animation command, and every symbol
        # among them was being recorded at index len(ops) -- one past the
        # end. 1,794 of them, across the corpus. Nothing distinguishes
        # that from a real index at the C end, so wm_anim_program_label
        # answered an ANIPC redirect to `#yoff` or `#puppet_tbl` with a
        # position the dispatcher then read as "the program is over".
        # A symbol that is not a position in the stream has no position;
        # the honest answer is -1, which dropping it here produces.
        reachable = {name: at for name, at in label_at.items()
                     if at < len(ops)}
        return ops, (entry or 0), reachable
    return ops


# Every op that carries a branch destination. `_c_op` reads op[1] as the
# resolved target for exactly these, so an op that registers a fixup and
# is NOT in here has its destination silently dropped on the way out --
# which is what happened to IFROPE and IFNOTROPE: the fixup resolved
# #fall_back correctly and the renderer then wrote -1.
BRANCH_OPS = {"GOTO", "IFSTATUS", "IFNOTSTATUS", "IFBLOCKED", "IF_RPTCOUNT",
              "IFNOT_RPTCOUNT",
              "SLIDE_BACK", "IF_BUTCOUNT_GE", "IF_BUTCOUNT_LT", "IFOPPMODE",
              "IF_RPTCOUNT_GE", "IFROPE", "IFNOTROPE"}
# Ops carrying (mode, a, b, c, d, e) from wlcommands' 5-tuple shape.
# Every op that comes out of wlcommands' command table carries the same
# (kind, mode, a, b, c) shape, so this is DERIVED from that table rather
# than listed by hand. It used to be a hand-kept set, and anything added to
# the command table without also being added here fell through to the
# no-operand default and had its operands silently written out as zero --
# which is exactly what happened to ANI_BOUNCE and ANI_GETUP.
MOTION_OPS = {kind for kind, _nargs, _has_mode in wlcommands.COMMANDS.values()}

# Every ANI_* the emitter fell through to, tallied by name. Reset by
# census() so a run can be measured on its own.
SKIPPED: "collections.Counter[str]" = collections.Counter()
# A few real call sites per skipped op, so the census says WHERE as well
# as how many -- an op that is skipped only sometimes is a defect, not a
# gap, and the difference is invisible without this.
SKIPPED_WHERE: "collections.defaultdict[str, list]" = \
    collections.defaultdict(list)
SKIP_NAME_RE = re.compile(r"^\s*(?:\.word|\.long|W+L+W*)?\s*,?\s*"
                          r"(ANI_[A-Za-z0-9_]+)", re.I)


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
        elif kind in ("IFROPE", "IFNOTROPE"):
            args[0], args[1] = op[2], op[3]
    elif kind == "SUPERSLAVE2":
        text = f'"{op[2]}"'
        args[0], args[1], args[2] = op[1], op[3], op[4]
    elif kind == "CHANGEANIM":
        text = f'"{op[1]}"'
    elif kind == "CODE":
        text = f'"{op[1]}"'
        # `a` carries the source line the local label resolved to, 0 for a
        # global name. src/core/anim_code.c keys its rows on it.
        args[0] = op[2] if len(op) > 2 else 0
    elif kind == "IFBUTTONS":
        args[0] = op[1]
        text = f'"{op[2]}"'
    elif kind in ("DRAW_NAME", "SHAKER", "SHAKEALL", "SHAKEROPES",
                  "SHAKECORNER", "SET_IDIOT", "SCROLL_CTRL", "LOOP",
                  "START_DIZZY"):
        args[0] = op[1]
    elif kind == "CREATEPROC":
        text = f'"{op[1]}"'
        args[0], args[1], args[2], args[3] = op[2], op[3], op[4], op[5]
    elif kind == "SHADOWTRAIL":
        mode = op[1]          # 1 = the OFF form
        text = "0" if op[2] is None else f'"{op[2]}"'
        args[0], args[1] = op[3], op[4]
    elif kind == "ATTCHIMAGE":
        mode = op[1]          # 1 = the `#offimg` exit (a literal 0 image)
        args[0] = op[2]       # the Z offset
        text = "0" if op[3] is None else f'"{op[3]}"'
    elif kind in ("DEBRIS", "DEBRISAT"):
        args[0], args[1], args[2] = op[1], op[2], op[3]
        args[3], args[4] = op[4], op[5]
    elif kind == "LEAPATPOS":
        args[0], args[1], args[2] = op[1], op[2], op[3]
        args[3], args[4] = op[4], op[5]
    elif kind == "SLIDEATOPP":
        args[0], args[1], args[2] = op[1], op[2], op[3]
    elif kind == "TARGET":
        args[0], args[1], args[2] = op[1], op[2], op[3]
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
    elif kind in ("CHECKWORD", "CHANGEANIM_TBL", "XFLIP_TBL", "OPPOFFSET"):
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
        ops, entry, labels = program_for(pathlib.Path(source), label,
                                         with_entry=True)
        sym = "prog_" + label
        out.append(f"static const wm_anim_op {sym}_ops[] = {{")
        for op in ops:
            out.append(_c_op(op))
        out += ["};", ""]
        if labels:
            out.append(f"static const wm_anim_label {sym}_labels[] = {{")
            for name, at in sorted(labels.items(), key=lambda kv: kv[1]):
                out.append(f'    {{ "{name}", {at} }},')
            out += ["};", ""]
        names.append((label, pathlib.Path(source).name, sym, entry,
                      bool(labels)))
    out.append("static const wm_anim_program programs[] = {")
    for label, src, sym, entry, has_labels in names:
        note = "" if entry == 0 else "   /* branches back into shared code */"
        lab = (f"{sym}_labels, sizeof({sym}_labels) / sizeof({sym}_labels[0])"
               if has_labels else "0, 0")
        out.append(f'    {{ "{label}", "{src}", {sym}_ops,')
        out.append(f'      sizeof({sym}_ops) / sizeof({sym}_ops[0]),'
                   f' {entry}, {lab} }},{note}')
    out += ["};", "",
            "/* The whole corpus, so a test can play every program rather",
            "   than only the ones it thought to name. */",
            "size_t wm_anim_program_count(void) {",
            "    return sizeof(programs) / sizeof(programs[0]);",
            "}",
            "",
            "const wm_anim_program *wm_anim_program_at(size_t index) {",
            "    return index < wm_anim_program_count() ? &programs[index] : 0;",
            "}",
            "",
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


def _roster_table_targets() -> list[tuple[pathlib.Path, str]]:
    """(file, label) for every animation a global roster table names.

    The label is resolved against the file that DEFINES it, not the one
    the table sits in, because a table in FINISEQ.ASM names animations
    defined in FINISEQ.ASM but a table in WRESTLE2.ASM names ones from
    the per-wrestler sequence files.
    """
    import wlrostertbl

    where: dict[str, pathlib.Path] = {}
    for src in wlanim.linked_files():
        for line in src.read_text(errors="replace").splitlines():
            m = wlanim.SUBR_RE.match(line)
            if m and m.group(1) not in where:
                where[m.group(1)] = src

    out: list[tuple[pathlib.Path, str]] = []
    seen: set[tuple[str, str]] = set()
    for _name, (_f, _line, rows) in wlrostertbl.roster_tables().items():
        for lab in rows:
            if not lab or not lab.endswith("_anim"):
                continue
            src = where.get(lab)
            if src is None:
                continue
            key = (src.name, lab)
            if key in seen:
                continue
            seen.add(key)
            out.append((src, lab))
    return out


def _movi_anim_targets() -> list[tuple[pathlib.Path, str]]:
    """Animations ordinary code loads by name, in the non-sequence files.

    Every other path here is driven by an opcode or a table. These are
    reached by plain `movi <label>,a0` from native code -- REACT1.ASM:
    749 and :757 pick between xxx_goto_stand_anim and
    xxx_aborted_attach_anim that way -- so no table names them and the
    roster sweep does not look in their file.

    The rule is deliberately narrow: the label must be a SUBR in the
    same file, must end in `_anim`, and its body must parse as an
    animation. Anything that does not is skipped rather than guessed
    at.
    """
    # Any register, not just a0. TAKER.ASM:693 loads
    # und_2_raise_dead_anim into a14 and stashes it in
    # SPECIAL_MOVE_ADDR; which register the caller used is its own
    # business, and pinning a0 lost that animation entirely.
    movi = re.compile(
        r"^\s*movi\s+([A-Za-z_][A-Za-z0-9_]*_anim)\s*,\s*a\d+\b", re.I)
    # Where each SUBR lives, so a label loaded in one file and defined
    # in another resolves to its definition -- TAKER.ASM:693 loads
    # und_2_raise_dead_anim, which FINISEQ.ASM:1640 defines.
    where: dict[str, pathlib.Path] = {}
    for q in wlanim.linked_files():
        for line in q.read_text(errors="replace").splitlines():
            m = wlanim.SUBR_RE.match(line)
            if m and m.group(1) not in where:
                where[m.group(1)] = q

    out: list[tuple[pathlib.Path, str]] = []
    seen: set[tuple[str, str]] = set()
    for src in wlanim.linked_files():
        # Skip only what the roster sweep above already covers, which
        # is the same condition it uses. FINISEQ.ASM has "SEQ" in its
        # name and is NOT swept, so excluding it here too left its
        # raise_dead_anim reachable by nothing at all.
        if "SEQ" in src.name and not src.name.startswith("FINI"):
            continue
        for line in src.read_text(errors="replace").splitlines():
            m = movi.match(wlanim.strip_comment(line))
            if not m:
                continue
            lab = m.group(1)
            home = where.get(lab)
            if home is None:
                continue        # not a SUBR anywhere: a local, or data
            # Already emitted by the roster sweep, which covers every
            # per-wrestler sequence file. The control files (BRET.ASM,
            # TAKER.ASM and the rest) load those animations by name
            # constantly, and following each one here would just queue
            # a second copy.
            if "SEQ" in home.name and not home.name.startswith("FINI"):
                continue
            key = (home.name, lab)
            if key in seen:
                continue
            try:
                program_for(home, lab)
            except (OSError, ValueError):
                continue        # not an animation, or refused
            seen.add(key)
            out.append((home, lab))
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source")
    ap.add_argument("--label", action="append", default=[])
    # An animation names its own file, so one run can emit programs that
    # span HRTSEQ2/3/4 -- the same shape tools/wlcommands.py takes.
    ap.add_argument("--animation", nargs=2, action="append", default=[],
                    metavar=("SOURCE", "LABEL"))
    # Every animation an op can name a whole animation with: ANI_SLAVEANIM's
    # tables, ANI_CHANGEANIM_TBL's rows, and plain ANI_CHANGEANIM's operand.
    # Read out of the source (tools/wlpuppet.py) rather than listed here, so
    # the emitter and the ops cannot disagree about what may be named.
    ap.add_argument("--slave-targets", action="store_true")
    # Every animation the eight playable wrestlers own, rather than a
    # hand-picked list. The list had been growing one label at a time
    # as each new op or table turned out to name an animation nothing
    # had emitted; emitting the roster removes that whole class of
    # gap, because wm_anim_program_find can no longer miss.
    ap.add_argument("--roster", action="store_true")
    ap.add_argument("--out")
    # Emit nothing; report what the emitter SKIPPED instead. Skipping is
    # silent in the generated C, so without this there is no measured
    # answer to "how much of the animation VM is still missing" -- only a
    # hand-kept one, which is exactly how the MOTION_OPS defect shipped.
    ap.add_argument("--census", action="store_true")
    ns = ap.parse_args(argv)
    entries = [(src, lab) for src, lab in ns.animation]
    if ns.source:
        entries += [(ns.source, l) for l in ns.label]
    if ns.roster:
        for src in wlanim.linked_files():
            if "SEQ" not in src.name:
                continue
            # FINISEQ.ASM is not swept. Two reasons, and the second is
            # the one that matters. The first is historical: its finish
            # moves have no frames, the alias rule ran away, and
            # rzr_finish1_move swallowed the rest of the file -- that
            # is fixed now, since wlanim stops at a routine's own
            # ANI_END. The second still holds: a sweep here emits the
            # fourteen finishing moves that GAME.EQU:580-587 assembles
            # OUT (seven of the eight NUM_*_FINISHES are 0), plus
            # stand_table and dizzy_table, which are data, plus
            # check_roll and is_door_open and a dozen more that are
            # plain logic. Everything real in this file is reached by
            # name instead, below.
            if src.name.startswith("FINI"):
                continue
            lines = src.read_text(errors="replace").splitlines()
            seen_here = []
            for line in lines:
                m = wlanim.SUBR_RE.match(line)
                if m and m.group(1) not in seen_here:
                    seen_here.append(m.group(1))
            for lab in seen_here:
                try:
                    program_for(src, lab)
                except (OSError, ValueError):
                    continue    # not an animation, or refused
                entries.append((str(src), lab))
    if ns.slave_targets:
        entries += [(str(p), lab) for p, lab in wlpuppet.slave_targets()]
        # ANI_CHANGEANIM_TBL names whole animations too.
        entries += [(str(p), lab)
                    for p, lab in wlpuppet.changeanim_targets()]
        # And so does a plain ANI_CHANGEANIM. The roster pass above walks
        # SUBR lines, so an animation written as a bare column-0 label
        # was emitted only if some table happened to name it -- which is
        # how DNKSEQ2.ASM's dnk_combo_hammer_anim and
        # re_enter_combo_hiptoss were named by an ANI_CHANGEANIM and
        # emitted by nothing.
        entries += [(str(p), lab)
                    for p, lab in wlpuppet.changeanim_op_targets()]
        # And so do the global per-wrestler tables ordinary code indexes
        # with FACETBL/FACE24TBL. Those rows are animations wherever
        # they live, including in files the roster sweep above skips:
        # FINISEQ.ASM's stand_table and dizzy_table name every
        # wrestler's stand and fdizzy, and nothing else reaches them.
        # Naming the rows rather than sweeping the file is the point --
        # a sweep of FINISEQ would also emit its fourteen assembled-out
        # finishing moves, its data tables and its plain logic.
        entries += [(str(p), lab) for p, lab in _roster_table_targets()]
        entries += [(str(p), lab) for p, lab in _movi_anim_targets()]
    if not entries:
        ap.error("nothing to emit: pass --animation, or --source with --label")
    seen, unique = set(), []
    for src, lab in entries:
        if lab not in seen:
            seen.add(lab)
            unique.append((src, lab))
    entries = unique
    if ns.census:
        SKIPPED.clear()
        SKIPPED_WHERE.clear()
        emitted = 0
        for src, lab in entries:
            try:
                emitted += len(program_for(pathlib.Path(src), lab))
            except (OSError, ValueError):
                continue
        skipped = sum(SKIPPED.values())
        total = emitted + skipped
        print(f"{len(entries)} programs, {emitted} ops emitted, "
              f"{skipped} skipped "
              f"({100.0 * emitted / total:.1f}% of {total})")
        for name, n in SKIPPED.most_common():
            print(f"  {n:6d}  {name}")
            for where in SKIPPED_WHERE[name][:2]:
                print(f"            {where}")
        return 0
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
