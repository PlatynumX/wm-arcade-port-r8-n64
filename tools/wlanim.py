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


def strip_comment(s: str) -> str:
    return s.split(";", 1)[0].strip()


def eval_ticks(expr: str) -> int:
    expr = HEX_SUFFIX_RE.sub(lambda m: "0x" + m.group(1), expr.strip())
    if not re.fullmatch(r"[0-9xXa-fA-F+() \t-]+", expr):
        raise ValueError(f"unsupported WL tick expression: {expr!r}")
    value = eval(expr, {"__builtins__": {}}, {})
    if not isinstance(value, int) or not 1 <= value <= 65535:
        raise ValueError(f"WL tick count out of range: {expr!r} -> {value!r}")
    return value


def _frame_from_line(line: str) -> Frame | None:
    m = WL_RE.match(line) or WAIT_FRAME_RE.match(line)
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


def extract_visual_slice(path: pathlib.Path, label: str, repeat: bool | None = None) -> Sequence:
    """Extract only visible WL frame rows from a gameplay-heavy routine.

    This deliberately ignores non-image opcodes. It is a visual bring-up aid, not a
    claim that the gameplay routine itself has been translated.
    """
    active = False
    frames: list[Frame] = []
    inferred_repeat = False
    wanted = label.upper()

    for raw in path.read_text(errors="replace").splitlines():
        line = strip_comment(raw)
        if not line:
            continue
        sub = SUBR_RE.match(line)
        if sub:
            if active and frames:
                break
            if active:
                continue
            active = sub.group(1).upper() == wanted
            continue
        if not active:
            continue
        frame = _frame_from_line(line)
        if frame:
            frames.append(frame)
            continue
        if REPEAT_RE.match(line):
            inferred_repeat = True
            break
        if END_RE.match(line):
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
    return Sequence(label=label, frames=tuple(frames),
                    repeat=inferred_repeat if repeat is None else repeat)


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
