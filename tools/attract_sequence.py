#!/usr/bin/env python3
"""Extract ATTRACT.ASM's active attract_mode JSRP order into portable C."""
from __future__ import annotations
import argparse
import pathlib
import re
import sys

JSRP_RE = re.compile(r"\bJSRP\s+([A-Za-z0-9_#]+)", re.IGNORECASE)


def extract(source: pathlib.Path) -> list[str]:
    lines = source.read_text(errors="replace").splitlines()
    in_attract = False
    in_loop = False
    out: list[str] = []
    for raw in lines:
        active = raw.split(";", 1)[0]
        if not in_attract:
            if re.search(r"\bSUBR\s+attract_mode\b", active, re.IGNORECASE):
                in_attract = True
            continue
        if not in_loop:
            if re.match(r"^\s*#loop\b", active, re.IGNORECASE):
                in_loop = True
            continue
        if re.search(r"\bcalla\s+RemapIO\b", active, re.IGNORECASE):
            break
        m = JSRP_RE.search(active)
        if m:
            out.append(m.group(1))
    if not in_attract:
        raise ValueError("ATTRACT.ASM attract_mode not found")
    if not in_loop:
        raise ValueError("ATTRACT.ASM attract_mode #loop label not found")
    if not out:
        raise ValueError("no active JSRP calls found before RemapIO")
    return out


def enum_name(label: str) -> str:
    return "WM_ATTRACT_" + re.sub(r"[^A-Za-z0-9]+", "_", label).upper().strip("_")


def emit(source: pathlib.Path, out: pathlib.Path) -> list[str]:
    labels = extract(source)
    lines = [
        "/* Auto-generated from ATTRACT.ASM::attract_mode active JSRP calls. */",
        '#include "wm/attract.h"',
        "",
        "const wm_attract_call wm_source_attract_loop[] = {",
    ]
    lines.extend(f"    {enum_name(label)}," for label in labels)
    lines += [
        "};",
        "",
        "const size_t wm_source_attract_loop_count =",
        "    sizeof(wm_source_attract_loop) / sizeof(wm_source_attract_loop[0]);",
        "",
    ]
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines))
    return labels


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True, type=pathlib.Path)
    ap.add_argument("--out", required=True, type=pathlib.Path)
    ns = ap.parse_args()
    labels = emit(ns.source, ns.out)
    print("attract sequence: " + " -> ".join(labels))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"attract_sequence: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
