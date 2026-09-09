#!/usr/bin/env python3
"""An INDEPENDENT reader and interpreter of the animation streams.

Everything else in tools/ shares wlanim.py -- its comment stripping, its
label patterns, its routine-span logic. That makes the existing
cross-check between the flat extractor and the program emitter only
partly independent: a mistake in the shared layer is invisible to both.

This file deliberately shares NOTHING. It re-reads the .ASM files with
its own lexer, finds routines with its own patterns, resolves branches
with its own scoping, and then RUNS the result -- so what it produces is
the sequence of frames an animation actually plays, tick by tick, which
is the thing the C interpreter also produces. Comparing the two checks
the emitter and the interpreter together, end to end.

It is deliberately narrow. It handles only the frame-and-control subset:

    WL   <ticks>,<IMAGE>+FR<n>     show a frame for n ticks
    .word ANI_END                  stop
    WL   ANI_GOTO,<label>          jump
    .word ANI_REPEAT               back to the top
    .word ANI_SET_RPTCOUNT,<n>     load the repeat counter
    WL   ANI_IF_RPTCOUNT,<label>   branch while it is non-zero
    .word ANI_PAUSE,<n>            hold the current frame

An animation whose body contains anything else is REPORTED AS
UNVERIFIABLE rather than guessed at, so the number this prints is an
honest count of what was actually checked.
"""
from __future__ import annotations
import argparse
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
ASM = ROOT / "original" / "wwf-wrestlemania"

# --- its own lexer -----------------------------------------------------
def strip(line: str) -> str:
    """Drop a ';' comment and trailing whitespace/CR. Quotes never appear
    in these streams, so no string handling is needed."""
    i = line.find(";")
    if i >= 0:
        line = line[:i]
    return line.replace("\r", "").rstrip()


ROUTINE = re.compile(r"^\s+SUBRP?\s+(\S+)\s*$", re.I)
LOCAL = re.compile(r"^(#\S+)")
FRAME = re.compile(r"^\s+WL\s+([0-9]+)\s*,\s*([A-Za-z0-9_]+)\s*\+\s*FR([0-9]+)\s*$", re.I)
END = re.compile(r"^\s+\.word\s+ANI_END\s*$", re.I)
REPEAT = re.compile(r"^\s+\.word\s+ANI_REPEAT\s*$", re.I)
GOTO = re.compile(r"^\s+WL\s+ANI_GOTO\s*,\s*(\S+)\s*$", re.I)
SETRPT = re.compile(r"^\s+\.word\s+ANI_SET_RPTCOUNT\s*,\s*([0-9]+)\s*$", re.I)
IFRPT = re.compile(r"^\s+WL\s+ANI_IF_RPTCOUNT\s*,\s*(\S+)\s*$", re.I)
PAUSE = re.compile(r"^\s+\.word\s+ANI_PAUSE\s*,\s*([0-9]+)\s*$", re.I)
ANYCMD = re.compile(r"\bANI_[A-Z0-9_]+\b", re.I)

# Ops that decide WHICH frame plays and for HOW LONG. Any of these in a
# body makes it unverifiable here -- they are the interesting half, and
# guessing at them would defeat the point.
STREAM_OPS = {
    "ANI_IFSTATUS", "ANI_IFNOTSTATUS", "ANI_IFBLOCKED", "ANI_IFBUTTONS",
    "ANI_IFOPP", "ANI_IFROPE", "ANI_IFNOTROPE", "ANI_IF_BUTCOUNT_GE",
    "ANI_IF_BUTCOUNT_LT", "ANI_IFNOT_RPTCOUNT", "ANI_CHANGEANIM",
    "ANI_CHANGEANIM_TBL", "ANI_SLIDE_BACK", "ANI_WAITHITGND",
    "ANI_WAITRELEASE", "ANI_WAITROLL", "ANI_WAITHITOPP", "ANI_HMBWAIT",
    "ANI_LEAPATOPP", "ANI_LEAPATPOS", "ANI_SLAVEANIM", "ANI_SUPERSLAVE2",
    "ANI_LOOP", "ANI_SINGLESTEP", "ANI_DEC_RPTCOUNT", "ANI_START_DIZZY",
    "ANI_SLIDEATOPP", "ANI_CHECKWORD", "ANI_BOUNCE", "ANI_GETUP",
}

# Ops that change actor or world state but not the frame stream. Listed
# explicitly rather than assumed: an op NOT named here is unverifiable,
# so a newly-introduced one cannot silently be treated as harmless.
#
# ANI_SETSPEED is in here with a caveat. It only stores ANI_SPEED in this
# port and the interpreter's tick counting never reads it, so it is
# stream-neutral HERE by construction -- which means this check cannot
# say whether the arcade scaled frame durations by it. That is a separate
# open question, not something this file verifies.
NEUTRAL_OPS = {
    "ANI_SETMODE", "ANI_SETPLYRMODE", "ANI_SETSPEED", "ANI_SETFACING",
    "ANI_XFLIP", "ANI_SET_WRESTLER_XFLIP", "ANI_ZEROVELS",
    "ANI_ZERO_XZVELS", "ANI_SET_XVEL", "ANI_SET_YVEL", "ANI_SET_ZVEL",
    "ANI_MIN_YVEL", "ANI_FRICTION", "ANI_OFFSET", "ANI_ATTACK_ON",
    "ANI_ATTACK_ON_Z", "ANI_ATTACK_OFF", "ANI_STARTATTACK", "ANI_CODE",
    "ANI_SOUND", "ANI_FACEUP", "ANI_FACEDOWN", "ANI_FACE", "ANI_DAMAGE",
    "ANI_DAMAGEOPP", "ANI_CLEAR_COMBO", "ANI_INC_COMBO", "ANI_ADD_MOVE",
    "ANI_SETLONG", "ANI_SETWORD", "ANI_CLR_BUTCOUNT", "ANI_IMMOBILIZE",
    "ANI_SHAKER", "ANI_SHAKEALL", "ANI_SHAKECORNER", "ANI_SHAKEROPES",
    "ANI_DEBRIS", "ANI_DEBRISAT", "ANI_ATTCHIMAGE", "ANI_ATTCHIMAGE2",
    "ANI_SHADOWTRAIL", "ANI_DRAW_NAME", "ANI_CREATEPROC", "ANI_TARGET",
    "ANI_SCROLL_CTRL", "ANI_SET_IDIOT", "ANI_DETACH", "ANI_SET_ATTACH",
    "ANI_ATTACHZ", "ANI_SETOPPVELS", "ANI_SETOPPMODE", "ANI_SETOPPFACING",
    "ANI_SET_OPP_XVEL", "ANI_XFLIP_OPP", "ANI_OPP_GETUP", "ANI_ATTACHVEL",
    "ANI_SETOPP_PLYRMODE", "ANI_BOUNCEROPE", "ANI_BENDROPE", "ANI_ROPE_Z",
    "ANI_CLEAR_CLIMB", "ANI_SET_RPTCOUNT", "ANI_NOP", "ANI_PAUSE",
    "ANI_RESET_GRAVITY", "ANI_SET_GRAVITY", "ANI_OPPOFFSET",
    "ANI_XFLIP_TBL", "ANI_SETPAL", "ANI_RESTOREPAL",
}


def linked() -> set:
    """The .ASM files the linker actually builds, read here rather than
    borrowed, so this file keeps its independence. ADMSEQ*.ASM (Adam
    Bomb, the wrestler who was cut) and the superseded HRTSEQ/YOKSEQ are
    absent from it -- their animations are real and playable but are not
    in the game, so they are not expected in the emitted set either."""
    out = set()
    for line in (ASM / "WRESTLE.CMD").read_text(errors="replace").splitlines():
        m = re.match(r"^([A-Za-z0-9_]+)\.obj\b", line.strip(), re.I)
        if m:
            out.add(m.group(1).upper() + ".ASM")
    return out


def routines(path: pathlib.Path):
    """label -> list of stripped body lines, ending at the next routine."""
    lines = [strip(l) for l in path.read_text(errors="replace").splitlines()]
    starts = [(i, m.group(1)) for i, l in enumerate(lines)
              if (m := ROUTINE.match(l))]
    out = {}
    for n, (i, name) in enumerate(starts):
        stop = starts[n + 1][0] if n + 1 < len(starts) else len(lines)
        out.setdefault(name, lines[i + 1:stop])
    return out


class Unverifiable(Exception):
    pass


def play(body, max_frames=400):
    """Run the body and return the (frame, ticks) sequence it plays."""
    # Labels first: a local label may sit alone or share its line.
    labels = {}
    steps = []
    for line in body:
        if not line.strip():
            continue
        m = LOCAL.match(line)
        if m:
            labels[m.group(1)] = len(steps)
            rest = line[m.end():]
            if not rest.strip():
                continue
            line = rest
        steps.append(line)

    played = []
    pc = 0
    rpt = 0
    guard = 0
    while pc < len(steps):
        guard += 1
        if guard > 20000:
            raise Unverifiable("did not settle")
        line = steps[pc]

        if (m := FRAME.match(line)):
            played.append((f"{m.group(2).upper()}{int(m.group(3)):02d}",
                           int(m.group(1))))
            if len(played) >= max_frames:
                return played, "capped"
            pc += 1
            continue
        if END.match(line):
            return played, "end"
        if (m := GOTO.match(line)):
            t = m.group(1)
            if t not in labels:
                raise Unverifiable(f"goto {t} outside body")
            pc = labels[t]
            continue
        if REPEAT.match(line):
            pc = 0
            if len(played) >= max_frames:
                return played, "capped"
            continue
        if (m := SETRPT.match(line)):
            rpt = int(m.group(1))
            pc += 1
            continue
        if (m := IFRPT.match(line)):
            t = m.group(1)
            if t not in labels:
                raise Unverifiable(f"if_rptcount {t} outside body")
            if rpt > 0:
                rpt -= 1
                pc = labels[t]
            else:
                pc += 1
            continue
        if (m := PAUSE.match(line)):
            if played:
                played.append((played[-1][0], int(m.group(1))))
            pc += 1
            continue
        m = ANYCMD.search(line)
        if m:
            op = m.group(0).upper()
            if op in STREAM_OPS:
                raise Unverifiable(op)
            if op not in NEUTRAL_OPS:
                raise Unverifiable("unknown " + op)
            pc += 1
            continue
        # Data, directives, stray labels: not part of the stream.
        pc += 1
    return played, "ranout"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out")
    ns = ap.parse_args()

    verified, skipped = {}, {}
    live = linked()
    for path in sorted(ASM.glob("*SEQ*.ASM")):
        if "'" in path.name or path.name.upper() not in live:
            continue
        for label, body in routines(path).items():
            if not label.endswith("_anim"):
                continue
            try:
                seq, why = play(body)
            except Unverifiable as exc:
                skipped[label] = str(exc)
                continue
            if not seq or why != "end":
                skipped[label] = f"no clean end ({why})"
                continue
            verified[label] = seq

    print(f"independently played: {len(verified)}   "
          f"unverifiable: {len(skipped)}")
    if ns.out:
        pathlib.Path(ns.out).write_text(json.dumps(verified))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
