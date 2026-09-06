#!/usr/bin/env python3
"""Extract Midway `WL ticks,FRAME+FRn` visual animation streams.

Two modes are supported:
- strict sequences: intended for pure frame streams such as stands/walks. Unknown
  executable lines after the first frame are rejected.
- visual slices: collect only frame-bearing WL rows while ignoring gameplay/control
  opcodes. This is useful for N64 visual bring-up of attacks before their full
  animation VM semantics are translated. A slice stops at ANI_END, ANI_REPEAT,
  an obvious terminal local GOTO, or the next SUBR.
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys
from dataclasses import dataclass

SUBR_RE = re.compile(r"^\s*SUBR(?:P)?\s+#?([A-Za-z_][A-Za-z0-9_]*)\b", re.I)
WL_RE = re.compile(r"^\s*WL\s+([^,]+),\s*([A-Za-z_][A-Za-z0-9_]*)\s*\+\s*FR([0-9]+)\s*$", re.I)
WAIT_FRAME_RE = re.compile(r"^\s*WWL\s+ANI_WAITHITOPP\s*,\s*([^,]+),\s*([A-Za-z_][A-Za-z0-9_]*)\s*\+\s*FR([0-9]+)\s*$", re.I)
REPEAT_RE = re.compile(r"^\s*\.word\s+ANI_REPEAT\s*$", re.I)
END_RE = re.compile(r"^\s*\.word\s+ANI_END\s*$", re.I)
GOTO_RE = re.compile(r"^\s*WL\s+ANI_GOTO\s*,\s*#?[A-Za-z_][A-Za-z0-9_]*\s*$", re.I)
PREFIX_WORD_RE = re.compile(
    r"^\s*\.word\s+(ANI_SETMODE|ANI_SETSPEED|ANI_SETFACING|ANI_XFLIP)\b", re.I)
HEX_SUFFIX_RE = re.compile(r"\b([0-9A-Fa-f]+)h\b")

@dataclass(frozen=True)
class Frame:
    name: str
    ticks: int

@dataclass(frozen=True)
class Sequence:
    label: str
    frames: tuple[Frame, ...]
    repeat: bool
    # ANI_CHANGEANIM target, when the routine ends by becoming another
    # animation rather than by ANI_END.
    next_label: str | None = None
    # ANIM.ASM RPT_COUNT loop, as frame indices into `frames`; loop_count 0
    # means the routine has no such loop.
    loop_first: int = 0
    loop_last: int = 0
    loop_count: int = 0


def strip_comment(s: str) -> str:
    return s.split(";", 1)[0].strip()


def load_equ(path: pathlib.Path, prefix: str) -> dict[str, int]:
    """`NAME .equ VALUE` constants from one of the original .EQU files."""
    out: dict[str, int] = {}
    if not path.exists():
        return out
    for raw in path.read_text(errors="replace").splitlines():
        line = raw.split(";", 1)[0]
        # Values are written either decimal or with the assembler's own
        # trailing-h hex (MODE_UNINT is 04h, MODE_STATUS 200h).
        m = re.match(rf"^({prefix}[A-Z0-9_]*)\s+\.?equ\s+"
                     rf"([0-9A-Fa-f]+h|[0-9]+)\s*$", line.strip(), re.I)
        if m:
            val = m.group(2)
            out[m.group(1).upper()] = (int(val[:-1], 16) if val[-1] in "hH"
                                       else int(val))
    return out


ORIG = pathlib.Path(__file__).resolve().parents[1] / "original" / "wwf-wrestlemania"

# Plain global constants an operand or a tick count can be written in terms
# of -- TSEC (DISPLAY.EQU:46, ticks per second) shows up as `TSEC*60`, the
# one-minute hold at the head of every wrestler's `*_zip_anim`.
GLOBAL_EQU: dict[str, int] = {}
for _f in ("DISPLAY.EQU", "GAME.EQU", "PLYR.EQU", "ANIM.EQU", "DAMAGE.EQU"):
    GLOBAL_EQU.update(load_equ(ORIG / _f, ""))


def eval_ticks(expr: str) -> int:
    expr = expr.strip()
    # A tick count is usually a literal, but the long attract-mode holds are
    # written as products -- `60*60` and `TSEC*60` are both a minute.
    if re.search(r"[A-Za-z_]", expr):
        for name in set(re.findall(r"\b[A-Za-z_][A-Za-z0-9_]*\b", expr)):
            if name.upper() in GLOBAL_EQU and not re.fullmatch(
                    r"[0-9A-Fa-f]+h", name, re.I):
                expr = re.sub(r"\b" + re.escape(name) + r"\b",
                              str(GLOBAL_EQU[name.upper()]), expr)
    expr = HEX_SUFFIX_RE.sub(lambda m: "0x" + m.group(1), expr)
    if not re.fullmatch(r"[0-9xXa-fA-F+*() \t-]+", expr):
        raise ValueError(f"unsupported WL tick expression: {expr!r}")
    value = eval(expr, {"__builtins__": {}}, {})
    if not isinstance(value, int) or not 1 <= value <= 65535:
        raise ValueError(f"WL tick count out of range: {expr!r} -> {value!r}")
    return value


# A local label can sit on the same line as the frame it names
# (`#cont\tWL\t5,H4SL4C+FR1`). The label is an address, not part of the
# instruction, so it is stripped before matching -- otherwise that frame is
# silently dropped, which matters as soon as a chained continuation lands
# on one.
LEADING_LOCAL_LABEL_RE = re.compile(r"^\s*#[A-Za-z_][A-Za-z0-9_]*\s+(?=\S)")


def _frame_from_line(line: str) -> Frame | None:
    body = LEADING_LOCAL_LABEL_RE.sub("", line)
    m = WL_RE.match(body) or WAIT_FRAME_RE.match(body)
    if not m:
        return None
    return Frame(f"{m.group(2).upper()}{int(m.group(3)):02d}", eval_ticks(m.group(1)))


def extract(path: pathlib.Path, label: str) -> Sequence:
    """Strict extraction for data-like WL streams."""
    active = False
    frames: list[Frame] = []
    repeat = False
    wanted = label.upper()

    for lineno, raw in enumerate(path.read_text(errors="replace").splitlines(), 1):
        line = strip_comment(raw)
        if not line:
            continue
        sub = SUBR_RE.match(line)
        if sub:
            if active:
                if frames:
                    break
                continue
            active = sub.group(1).upper() == wanted
            continue
        if not active:
            continue
        if line.startswith("#") or line.startswith("*"):
            if frames:
                break
            continue
        if PREFIX_WORD_RE.match(line):
            continue
        frame = _frame_from_line(line)
        if frame:
            frames.append(frame)
            continue
        if REPEAT_RE.match(line):
            repeat = True
            break
        if END_RE.match(line):
            break
        if frames:
            raise ValueError(f"{path}:{lineno}: unsupported line in {label}: {line!r}")

    if not frames:
        raise ValueError(f"no WL frames found for {label}")
    return Sequence(label=label, frames=tuple(frames), repeat=repeat)


GOTO_TARGET_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_GOTO\s*,\s*(#?[A-Za-z_][A-Za-z0-9_]*)\s*$", re.I)
LOCAL_LABEL_RE = re.compile(r"^\s*(#[A-Za-z_][A-Za-z0-9_]*)\b")

# A branch target is not always a `#local`. The sequence files also place
# plain file-scope labels in column 0 on a line of their own -- SHNSEQ2's
# `getup_in_4`, RZRSEQ3's `missed_rug`, SHNSEQ3's `last_hitx`,
# BAMSEQ3's `START_OF_BREAKER` -- and branch to them from routines that do
# not otherwise contain them. To the assembler these resolve exactly like a
# local does; the only difference is the sigil.
GLOBAL_LABEL_RE = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)[ \t]*$")


def label_def(line: str) -> str | None:
    """The label this line defines, `#local` or file-scope, else None."""
    m = LOCAL_LABEL_RE.match(line)
    if m:
        return m.group(1)
    m = GLOBAL_LABEL_RE.match(line)
    return m.group(1) if m else None


def _body_stop(lines: list[str], start: int) -> int:
    """End of the body beginning at `start`: the next SUBR that follows at
    least one frame. A SUBR reached before any frame is an alias for the
    routine after it (HRTSEQ2.ASM:1334-1335 hrt_2/4_super_kick_anim), so the
    body runs on through it."""
    saw_frame = False
    for j in range(start, len(lines)):
        if SUBR_RE.match(lines[j]):
            if saw_frame:
                return j
            continue
        if _frame_from_line(lines[j]):
            saw_frame = True
    return len(lines)


def _routine_span(lines: list[str], label: str) -> tuple[int, int] | None:
    wanted = label.upper()
    for i, line in enumerate(lines):
        sub = SUBR_RE.match(line)
        if sub and sub.group(1).upper() == wanted:
            return (i + 1, _body_stop(lines, i + 1))
    return None


def _routine_terminates(lines: list[str], span: tuple[int, int]) -> bool:
    """Does this body actually end, rather than run on? ANI_END/ANI_REPEAT
    is a real terminator; a body with neither does not stop where its text
    stops, because execution simply continues into the words that follow."""
    body = lines[span[0]:span[1]]
    if any(END_RE.match(l) or REPEAT_RE.match(l) for l in body):
        return True
    # An ANI_CHANGEANIM terminates only when it genuinely REPLACES the
    # terminator -- i.e. nothing in the body ends it any other way. That is
    # the shape the source writes with the `.word ANI_END` after it
    # commented out (16 places across HRTSEQ2-4). When the body also has a
    # real ANI_END, the CHANGEANIM is one exit among several and the
    # routine's own frame list runs on to that ANI_END; see
    # extract_visual_slice.
    return any(CHANGEANIM_RE.match(l) for l in body)


def _chain_target(lines: list[str], span: tuple[int, int]) -> int | None:
    """Where a non-terminating body's execution actually continues: at its
    own trailing unconditional ANI_GOTO's label (nothing after that GOTO is
    reachable any other way, e.g. hrt_2_raise_arm_anim's `WL ANI_GOTO,#cont`
    into the middle of hrt_4_raise_arm_anim), else by falling off the end
    into the next SUBR (hrt_2_hair_pickup_anim into hrt_4_hair_pickup_anim,
    HRTSEQ3.ASM:855/866)."""
    last_goto = None
    for i in range(span[0], span[1]):
        m = GOTO_TARGET_RE.match(lines[i])
        if m:
            last_goto = m.group(1)
    if last_goto is None:
        return span[1]
    # Local labels are scoped, and the same name (#cont, #hit, #missed) is
    # reused all over these files. Resolve forward from this routine's own
    # start so the match is either later in this body or in the routine it
    # runs on into -- never some unrelated earlier routine's label.
    for i in range(span[0], len(lines)):
        m = LOCAL_LABEL_RE.match(lines[i])
        if m and m.group(1) == last_goto:
            return i
    return None


CHANGEANIM_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_CHANGEANIM\s*,\s*([A-Za-z_][A-Za-z0-9_]*)", re.I)


SET_RPT_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_SET_RPTCOUNT\s*,\s*(-?\d+)\s*$", re.I)
IF_RPT_RE = re.compile(
    r"^\s*(?:\.word|W+L+W*)\s+ANI_IF_RPTCOUNT\s*,\s*(#?[A-Za-z_][A-Za-z0-9_]*)\s*$",
    re.I)


def _find_rpt_loop(lines, order, frame_at):
    """Locate an ANI_SET_RPTCOUNT / ANI_IF_RPTCOUNT span in the walked stream.

    Returns (loop_first, loop_last, count) as frame indices plus the literal
    iteration count, or None when there is no loop. Raises when the loop is
    real but cannot be represented statically, rather than guessing:

      - a negative ANI_SET_RPTCOUNT means RNDRNG0(-n) (ANIM.ASM:3538), a
        count drawn fresh at runtime
      - a count large enough to be an "until something else stops it" loop
        (hrt_4_raise_arm_anim's own 1000) is not a fixed-length animation
    """
    count = None
    for idx in order:
        m = SET_RPT_RE.match(lines[idx])
        if m:
            if count is not None:
                raise ValueError(
                    "more than one ANI_SET_RPTCOUNT in the walked stream; "
                    "nested or re-seeded repeat loops are not represented")
            count = int(m.group(1))
            continue
        m = IF_RPT_RE.match(lines[idx])
        if m:
            if count is None:
                raise ValueError("ANI_IF_RPTCOUNT with no ANI_SET_RPTCOUNT")
            if count < 0:
                raise ValueError(
                    "ANI_SET_RPTCOUNT,%d is negative, i.e. RNDRNG0(%d) drawn "
                    "at runtime (ANIM.ASM:3538) -- the iteration count is not "
                    "fixed and cannot be tabled" % (count, -count))
            if count > 64:
                raise ValueError(
                    "ANI_SET_RPTCOUNT,%d is an effectively endless loop, not "
                    "a fixed-length animation" % count)
            target = m.group(1)
            first_line = None
            for j in order:
                lm = LOCAL_LABEL_RE.match(lines[j])
                if lm and lm.group(1) == target:
                    first_line = j
                    break
            if first_line is None:
                raise ValueError(
                    "ANI_IF_RPTCOUNT,%s: label not in the walked stream" % target)
            firsts = [frame_at[j] for j in order
                      if j >= first_line and j in frame_at]
            lasts = [frame_at[j] for j in order if j <= idx and j in frame_at]
            if not firsts or not lasts:
                raise ValueError("repeat loop span contains no frames")
            if firsts[0] > lasts[-1]:
                # A FORWARD ANI_IF_RPTCOUNT: the label sits after the branch,
                # so this is not one span played N times but a first pass
                # followed by a different repeated block, sharing one
                # RPT_COUNT (hrt_uppercuts_to_head_anim, HRTSEQ2.ASM:2310).
                # wm_visual_sequence carries a single span, so refuse rather
                # than emit an inverted or invented one.
                raise ValueError(
                    "ANI_IF_RPTCOUNT,%s branches FORWARD: a first pass plus a "
                    "separate repeated block sharing one RPT_COUNT, which a "
                    "single loop span cannot represent" % target)
            branches = sum(1 for j in order if IF_RPT_RE.match(lines[j]))
            if branches != 1:
                raise ValueError(
                    "%d ANI_IF_RPTCOUNT branches share one ANI_SET_RPTCOUNT; "
                    "only a single repeated span is represented" % branches)
            return (firsts[0], lasts[-1], count)
    return None


def slice_line_order(lines: list[str], label: str) -> list[int]:
    """The real line order a visual slice of `label` walks.

    Shared with tools/wlattack.py so the audit judges exactly the stream
    that would be extracted, chained continuations included, instead of
    only the text under the routine's own label.
    """
    span = _routine_span(lines, label)
    if span is None:
        raise ValueError(f"no visual WL frames found for {label}")

    order: list[int] = []
    walked: set[int] = set()
    cur = span
    while True:
        order.extend(range(cur[0], cur[1]))
        walked.update(range(cur[0], cur[1]))
        if _routine_terminates(lines, cur):
            break
        nxt = _chain_target(lines, cur)
        if nxt is None:
            raise ValueError(
                f"{label}: ends in an ANI_GOTO to a label that is not "
                f"defined in this file; the animation continues where this "
                f"extractor cannot follow")
        if nxt >= len(lines):
            break
        if nxt in walked:
            # A backward jump into ground already covered is a repeat loop,
            # not a continuation into new artwork -- hrt_run_anim's own
            # endless `ANI_GOTO #lp1`. Stop here and let the frame walk
            # below apply its existing repeat handling.
            break
        cur = (nxt, _body_stop(lines, nxt))
    return order


def extract_visual_slice(path: pathlib.Path, label: str, repeat: bool | None = None) -> Sequence:
    """Extract only visible WL frame rows from a gameplay-heavy routine.

    This deliberately ignores non-image opcodes. It is a visual bring-up aid, not a
    claim that the gameplay routine itself has been translated.

    A routine whose body carries no ANI_END/ANI_REPEAT does not end where
    its text ends: execution runs on into the next SUBR, or to the label of
    its own trailing unconditional ANI_GOTO. Those continuations are
    followed, so what comes out is the stream the machine really plays
    rather than the fragment that happens to sit under one label. Routines
    that do terminate are walked exactly as before.
    """
    lines = [strip_comment(raw)
             for raw in path.read_text(errors="replace").splitlines()]

    order = slice_line_order(lines, label)

    frames: list[Frame] = []
    inferred_repeat = False
    next_label = None
    frame_at: dict[int, int] = {}
    walked: list[int] = []

    for idx in order:
        walked.append(idx)
        line = lines[idx]
        if not line or SUBR_RE.match(line):
            continue
        frame = _frame_from_line(line)
        if frame:
            frame_at[idx] = len(frames)
            frames.append(frame)
            continue
        if REPEAT_RE.match(line):
            inferred_repeat = True
            break
        if END_RE.match(line):
            break
        # ANIM.ASM:1301 _ani_changeanim overwrites OANIPC *and* OANIBASE
        # with the target animation and never comes back, so this routine
        # genuinely ends here and the target begins. The source says so
        # itself: the `.word ANI_END` after an ANI_CHANGEANIM is commented
        # out in 16 places across HRTSEQ2-4. Everything after it in the text
        # belongs to some other path that branched past it.
        cm = CHANGEANIM_RE.match(line)
        if cm:
            # Terminal only when nothing after it ends the routine any
            # other way. hrt_fall_back_anim / hrt_flying_kick_anim end
            # exactly here, their `.word ANI_END` commented out right
            # below. But hrt_2_butts_anim reaches its ANI_CHANGEANIM when
            # its repeat loop runs out and still has a real ANI_END on the
            # button-mash #ex path, and hrt_facedown_getup_anim's sits
            # inside an ANI_IFNOTSTATUS free-toss branch with the ordinary
            # ending below it -- in both, the CHANGEANIM is one exit among
            # several, so the flat list keeps walking to the ANI_END, the
            # same "longest real path" rule every forward branch already
            # gets.
            if any(END_RE.match(lines[j]) for j in order[order.index(idx) + 1:]):
                continue
            next_label = cm.group(1)
            break
        # The run routine is an endless local loop ending in ANI_GOTO #lp1.
        if frames and GOTO_RE.match(line):
            # A local GOTO is only a loop terminator for auto/repeating slices.
            # One-shot attacks can branch to a shared recovery label (for example
            # Bret's super kick), so explicit repeat=false must keep scanning until
            # ANI_END and collect that recovery artwork.
            if repeat is not False:
                inferred_repeat = True
                break
            continue
        # Everything else (attack boxes, velocity changes, branches, labels, etc.)
        # is intentionally ignored in visual-slice mode.

    if not frames:
        raise ValueError(f"no visual WL frames found for {label}")

    loop = _find_rpt_loop(lines, walked, frame_at)
    loop_first, loop_last, loop_count = loop if loop else (0, 0, 0)

    return Sequence(label=label, frames=tuple(frames),
                    repeat=inferred_repeat if repeat is None else repeat,
                    loop_first=loop_first, loop_last=loop_last,
                    loop_count=loop_count, next_label=next_label)


def render(entries: list[tuple[str, str, str, Sequence]]) -> str:
    out = [
        "/* Auto-generated by tools/wlanim.py from selected WL streams. */",
        '#include "wm/bret_visuals.h"',
        "",
    ]
    for source_name, public_symbol, array_symbol, seq in entries:
        out.append(f"static const wm_visual_frame {array_symbol}[] = {{")
        for f in seq.frames:
            out.append(f'    {{"{f.name}", {f.ticks}}},')
        out += [
            "};",
            "",
            f"const wm_visual_sequence {public_symbol} = {{",
            f'    .source_file = "{source_name}",',
            f'    .source_label = "{seq.label}",',
            f"    .frames = {array_symbol},",
            f"    .frame_count = sizeof({array_symbol}) / sizeof({array_symbol}[0]),",
            f"    .repeat = {'true' if seq.repeat else 'false'},",
        ]
        if seq.loop_count:
            out += [
                f"    /* ANI_SET_RPTCOUNT,{seq.loop_count}: frames "
                f"[{seq.loop_first}..{seq.loop_last}] play {seq.loop_count} "
                f"times before the stream continues. */",
                f"    .loop_first = {seq.loop_first},",
                f"    .loop_last = {seq.loop_last},",
                f"    .loop_count = {seq.loop_count},",
            ]
        out += [
            "};",
            "",
        ]
    return "\n".join(out)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True, type=pathlib.Path)
    ap.add_argument("--sequence", action="append", nargs=3,
                    metavar=("LABEL", "PUBLIC_SYMBOL", "ARRAY_SYMBOL"), default=[])
    ap.add_argument("--slice", action="append", nargs=4,
                    metavar=("LABEL", "PUBLIC_SYMBOL", "ARRAY_SYMBOL", "REPEAT"), default=[])
    ap.add_argument("--out", required=True, type=pathlib.Path)
    ns = ap.parse_args()
    if not ns.sequence and not ns.slice:
        ap.error("at least one --sequence or --slice is required")

    entries: list[tuple[str, str, str, Sequence]] = []
    for label, public_symbol, array_symbol in ns.sequence:
        entries.append((ns.source.name, public_symbol, array_symbol, extract(ns.source, label)))
    for label, public_symbol, array_symbol, repeat_s in ns.slice:
        rs = repeat_s.lower()
        if rs not in ("true", "false", "auto"):
            raise ValueError(f"REPEAT must be true, false, or auto (got {repeat_s!r})")
        repeat = None if rs == "auto" else rs == "true"
        entries.append((ns.source.name, public_symbol, array_symbol,
                        extract_visual_slice(ns.source, label, repeat)))
    ns.out.parent.mkdir(parents=True, exist_ok=True)
    ns.out.write_text(render(entries))
    print(f"wrote {len(entries)} visual sequences -> {ns.out}")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, SyntaxError) as exc:
        print(f"wlanim: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
