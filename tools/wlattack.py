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


def trace(path: pathlib.Path, label: str):
    """Yield (frame_index, command, operands) plus the frame list."""
    active = False
    frames: list[wlanim.Frame] = []
    events: list[tuple[int, str, str]] = []
    wanted = label.upper()

    for raw in path.read_text(errors="replace").splitlines():
        line = wlanim.strip_comment(raw)
        if not line:
            continue
        sub = wlanim.SUBR_RE.match(line)
        if sub:
            if active and frames:
                break
            if active:
                continue
            active = sub.group(1).upper() == wanted
            continue
        if not active:
            continue

        frame = wlanim._frame_from_line(line)
        if frame:
            frames.append(frame)
            continue
        if wlanim.REPEAT_RE.match(line) or wlanim.END_RE.match(line):
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
    ns = ap.parse_args(argv)
    path = pathlib.Path(ns.source)

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
