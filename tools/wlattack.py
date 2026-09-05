#!/usr/bin/env python3
"""Locate a Midway animation routine's inline ANIM.ASM commands relative to
the WL frame stream tools/wlanim.py --slice extracts from the same routine.

An extracted wm_visual_sequence is a flat frame list; the gameplay commands
that sit *between* those WL rows (ANI_ATTACK_ON/ATTACK_ON_Z/ATTACK_OFF,
ANI_SETPLYRMODE, ANI_WAITRELEASE, ANI_SETFACING, ANI_XFLIP, ...) carry the
frame index a backend has to fire them at. That index used to be worked out
by reading the .ASM by hand for each animation, which does not scale to a
wrestler's full animation set and leaves nothing to re-check against when
the extraction changes.

This reports, for each command, the 0-based index of the WL frame it
immediately precedes -- i.e. exactly the `active_frame_index` a backend
window table needs -- using the same routine-scanning and stop rules as
wlanim.py's --slice mode so the two always agree on what "frame N" means.
A command after the final WL row is reported as index == frame_count.

--audit additionally answers a question the frame indices alone cannot: is a
flat `--slice` extraction of this routine actually faithful to any single
real playthrough? `--slice` linearly concatenates whatever WL rows it walks
past, so it is only honest for routines whose control flow is a straight
line plus forward skips. A routine that loops backwards, changes into a
different animation, or falls through into the next SUBR produces a flat
frame list that no real playthrough ever plays. Wiring one of those is
inventing an animation, not porting one, so --audit names the specific
construct instead of leaving it to be spotted by hand.

This is a reading aid over the original source. It does not decide what any
command means, and it does not emit code.
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim  # noqa: E402

# Commands worth reporting: everything a backend currently has to place, plus
# the mode/facing ones already hand-placed for the wired attacks.
COMMAND_RE = re.compile(
    r"^\s*(?:\.word|WL|WWL)\s+(ANI_ATTACK_ON_Z|ANI_ATTACK_ON|ANI_ATTACK_OFF|"
    r"ANI_SETPLYRMODE|ANI_WAITRELEASE|ANI_SETFACING|ANI_XFLIP|ANI_SETMODE|"
    r"ANI_STARTATTACK|ANI_ZEROVELS|ANI_ZERO_XZVELS|ANI_ADD_MOVE|ANI_OFFSET)\b"
    r"\s*,?\s*(.*)$",
    re.I)


# Control-flow constructs a flat slice cannot represent. Each entry is
# (regex, severity, explanation). "blocking" means a flat extraction is not a
# faithful rendering of any single playthrough; "tolerated" means the flat
# list is a real playthrough (the longest one), which is the precedent the
# already-wired attacks set -- hrt_2_punch_anim's own ANI_IFBUTTONS early
# exits and its ANI_SLIDE_BACK forward skip are both of this kind.
FLOW_RULES = (
    # blocking: a flat list is not any single real playthrough.
    (re.compile(r"^\s*W+L+W*\s+ANI_SUPERSLAVE2?\b", re.I), "blocking",
     "ANI_SUPERSLAVE: its WWLLW rows carry this actor's own frames in a "
     "shape wlanim.py's WL matcher does not parse, so those frames are "
     "silently missing from the extraction"),

    # unported: the frame list is faithful, but the routine carries real
    # gameplay commands this port does not translate. Worth knowing before
    # wiring; not a reason the frame data itself would be wrong.
    (re.compile(r"^\s*LEAPATOPP\b", re.I), "unported",
     "LEAPATOPP: real launch-at-opponent trajectory, not translated (the "
     "frames still play; the movement does not happen)"),
    (re.compile(r"^\s*(?:\.word|WL|WWL)\s+ANI_ATTACHZ?\b", re.I), "unported",
     "ANI_ATTACH: grapples the opponent onto this actor (paired-actor state "
     "this port does not model)"),
    (re.compile(r"^\s*(?:\.word|WL|WWL)\s+ANI_SETOPPMODE\b", re.I), "unported",
     "ANI_SETOPPMODE: writes the opponent's player_mode"),
    (re.compile(r"^\s*(?:\.word|WL|WWL)\s+ANI_ADD_MOVE\b", re.I), "unported",
     "ANI_ADD_MOVE: real per-frame velocity impulse, not translated"),

    # tolerated: a forward skip or fork. The flat list keeps the longest real
    # path, which is the precedent every already-wired attack sets --
    # hrt_2_punch_anim carries both of the first two and is wired.
    (re.compile(r"^\s*(?:\.word|WL|WWL)\s+ANI_IFBUTTONS\b", re.I), "tolerated",
     "ANI_IFBUTTONS: forward early-exit into another animation"),
    (re.compile(r"^\s*(?:\.word|WL|WWL)\s+ANI_SLIDE_BACK\b", re.I), "tolerated",
     "ANI_SLIDE_BACK: forward skip over the connected-hit frames"),
    (re.compile(r"^\s*(?:\.word|WL|WWL)\s+ANI_IFNOTSTATUS\b", re.I), "tolerated",
     "ANI_IFNOTSTATUS: hit/miss fork; the flat list keeps the hit path"),
    (re.compile(r"^\s*(?:\.word|WL|WWL)\s+ANI_IFBLOCKED\b", re.I), "tolerated",
     "ANI_IFBLOCKED: blocked fork; the flat list keeps the unblocked path"),
    (re.compile(r"^\s*(?:\.word|WL|WWL)\s+ANI_IF_BUTCOUNT_LT\b", re.I), "tolerated",
     "ANI_IF_BUTCOUNT_LT: forward skip on a button-mash count"),
)

LOCAL_LABEL_RE = re.compile(r"^\s*(#[A-Za-z_][A-Za-z0-9_]*)\b")
GOTO_TARGET_RE = re.compile(
    r"^\s*(?:\.word|WL|WWL)\s+ANI_GOTO\s*,\s*(#?[A-Za-z_][A-Za-z0-9_]*)", re.I)


def _routine_local_labels(path: pathlib.Path, label: str) -> set[str]:
    """Every local (#foo) label defined between this SUBR and the next."""
    active = False
    saw_frame = False
    labels: set[str] = set()
    wanted = label.upper()
    for raw in path.read_text(errors="replace").splitlines():
        line = wlanim.strip_comment(raw)
        if not line:
            continue
        sub = wlanim.SUBR_RE.match(line)
        if sub:
            # Same rule wlanim.py's own slice scan uses: a SUBR reached
            # before any frame is an alias for the routine that follows
            # (HRTSEQ2.ASM:1334-1335 hrt_2/4_super_kick_anim), so keep
            # scanning; a SUBR after frames genuinely ends the routine.
            if active and saw_frame:
                break
            if active:
                continue
            active = sub.group(1).upper() == wanted
            continue
        if not active:
            continue
        if wlanim._frame_from_line(line):
            saw_frame = True
        m = LOCAL_LABEL_RE.match(line)
        if m:
            labels.add(m.group(1))
    return labels


def audit(path: pathlib.Path, label: str):
    """Report why (or whether) a flat --slice of this routine is faithful.

    Walks exactly the line order wlanim.slice_line_order walks, so a routine
    that runs on into another is judged on the whole stream that would be
    extracted -- it is only as sliceable as everything it chains through.
    """
    lines = [wlanim.strip_comment(raw)
             for raw in path.read_text(errors="replace").splitlines()]
    try:
        order = wlanim.slice_line_order(lines, label)
    except ValueError as exc:
        return [("blocking", str(exc))], None

    # A deterministic ANI_SET_RPTCOUNT / ANI_IF_RPTCOUNT span is representable
    # now (wm_visual_sequence carries loop_first/loop_last/loop_count and the
    # runtime re-fires any attack window inside it once per pass, as the
    # source does). One that is not -- a negative count, i.e. RNDRNG0 drawn
    # at runtime, or an effectively endless one -- still blocks, and
    # extract_visual_slice says exactly why.
    loop_note = None
    seq_next = None
    try:
        seq = wlanim.extract_visual_slice(path, label, False)
        seq_next = seq.next_label
        if seq.loop_count:
            loop_note = ("looped",
                         f"ANI_SET_RPTCOUNT,{seq.loop_count}: frames "
                         f"[{seq.loop_first}..{seq.loop_last}] play "
                         f"{seq.loop_count} times; carried as the sequence's "
                         f"own loop fields, not flattened")
    except ValueError as exc:
        return [("blocking", str(exc))], None

    # Local labels reachable anywhere in the walked stream: a GOTO forward
    # into a chained-in routine is a forward skip, not a jump out.
    own_labels = {m.group(1) for m in
                  (LOCAL_LABEL_RE.match(lines[i]) for i in order) if m}

    seen_labels: set[str] = set()
    findings: list[tuple[str, str]] = []
    terminator = None
    saw_frame = False
    chained: list[str] = []

    for idx in order:
        line = lines[idx]
        if not line:
            continue
        sub = wlanim.SUBR_RE.match(line)
        if sub:
            if saw_frame:
                chained.append(sub.group(1))
            continue

        lbl = LOCAL_LABEL_RE.match(line)
        if lbl:
            seen_labels.add(lbl.group(1))

        if wlanim._frame_from_line(line):
            saw_frame = True
            continue
        if wlanim.REPEAT_RE.match(line):
            terminator = ("ANI_REPEAT", None)
            break
        if wlanim.END_RE.match(line):
            terminator = ("ANI_END", None)
            break

        goto = GOTO_TARGET_RE.match(line)
        if goto:
            target = goto.group(1)
            if target in seen_labels:
                sev = "blocking"
                why = (f"ANI_GOTO,{target}: backward jump to a label already "
                       f"passed -- a loop with no ANI_SET_RPTCOUNT bound, so "
                       f"the real frame stream has no fixed length")
            elif target in own_labels:
                sev = "tolerated"
                why = (f"ANI_GOTO,{target}: forward skip to a later label in "
                       f"the walked stream; --slice keeps the skipped frames, "
                       f"i.e. the longer path")
            else:
                sev = "blocking"
                why = (f"ANI_GOTO,{target}: {target} is not reached anywhere "
                       f"in the walked stream, so the animation continues "
                       f"where --slice cannot follow")
            if (sev, why) not in findings:
                findings.append((sev, why))
            continue

        for rx, severity, why in FLOW_RULES:
            if rx.match(line):
                if (severity, why) not in findings:
                    findings.append((severity, why))
                break

    if not saw_frame:
        raise ValueError(f"no WL frames found for {label}")
    if seq_next:
        findings.insert(0, ("becomes",
                            f"ends with ANI_CHANGEANIM,{seq_next} -- ANIM.ASM:"
                            f"1301 overwrites OANIPC/OANIBASE and never "
                            f"returns, so this routine really ends here and "
                            f"{seq_next} begins (the source's own `.word "
                            f"ANI_END` after it is commented out). The frame "
                            f"list is complete for THIS animation; the "
                            f"transition target is its own"))
    if loop_note:
        findings.insert(0, loop_note)
    if chained:
        findings.insert(0, ("chained",
                            "runs on into " + ", ".join(chained) +
                            " (no ANI_END of its own), so those routines' "
                            "frames and commands are part of this animation "
                            "and are judged here too"))
    return findings, terminator


def trace(path: pathlib.Path, label: str):
    """Yield (frame_index, command, operands) plus the frame list.

    Walks wlanim.slice_line_order, the same order extract_visual_slice
    walks, so a reported index always refers to the same frame the
    generated wm_visual_sequence has at that position -- chained
    continuations included.
    """
    lines = [wlanim.strip_comment(raw)
             for raw in path.read_text(errors="replace").splitlines()]
    order = wlanim.slice_line_order(lines, label)

    frames: list[wlanim.Frame] = []
    events: list[tuple[int, str, str]] = []

    for idx in order:
        line = lines[idx]
        if not line or wlanim.SUBR_RE.match(line):
            continue
        frame = wlanim._frame_from_line(line)
        if frame:
            frames.append(frame)
            continue
        if wlanim.REPEAT_RE.match(line) or wlanim.END_RE.match(line):
            break
        # Same terminator extract_visual_slice uses: ANI_CHANGEANIM ends the
        # routine (ANIM.ASM:1301), so indices must not run past it either.
        if wlanim.CHANGEANIM_RE.match(line):
            break
        cmd = COMMAND_RE.match(line)
        if cmd:
            events.append((len(frames), cmd.group(1).upper(), cmd.group(2).strip()))

    if not frames:
        raise ValueError(f"no WL frames found for {label}")
    return frames, events


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True)
    ap.add_argument("--label", required=True, action="append")
    ap.add_argument("--audit", action="store_true",
                    help="report whether a flat --slice of each routine is "
                         "faithful, instead of listing frames and commands")
    ns = ap.parse_args(argv)
    path = pathlib.Path(ns.source)

    if ns.audit:
        unsafe = 0
        for label in ns.label:
            findings, terminator = audit(path, label)
            blocking = [f for f in findings if f[0] == "blocking"]
            end = terminator[0] if terminator else "end of file"
            verdict = "NOT SAFELY SLICEABLE" if blocking else "sliceable"
            print(f"{label}: {verdict}  (slice ends at {end})")
            for sev in ("chained", "becomes", "looped", "blocking",
                        "unported", "tolerated"):
                tag = {"blocking": "BLOCKING ", "unported": "unported ",
                       "tolerated": "tolerated", "chained": "chained  ",
                       "looped": "looped   ", "becomes": "becomes  "}[sev]
                for s_, why in findings:
                    if s_ == sev:
                        print(f"    {tag} {why}")
            if blocking:
                unsafe += 1
        return 1 if unsafe else 0

    for label in ns.label:
        frames, events = trace(path, label)
        print(f"{label}  ({len(frames)} frames from {path.name})")
        for i, f in enumerate(frames):
            print(f"    [{i}] {f.name} x{f.ticks}")
        for idx, cmd, ops in events:
            where = f"before frame [{idx}]" if idx < len(frames) else "after last frame"
            print(f"    {cmd:<18} {where}" + (f"  {ops}" if ops else ""))
        print()
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError) as exc:
        print(f"wlattack: error: {exc}", file=sys.stderr)
        sys.exit(1)
