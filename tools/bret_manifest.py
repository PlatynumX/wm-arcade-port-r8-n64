#!/usr/bin/env python3
"""Parse the image-to-WIMP-container mapping emitted in BRET.LOD."""
from __future__ import annotations
import argparse
import pathlib
import re
import sys

IMG_RE = re.compile(r"^\s*([A-Za-z0-9_]+\.img)\s*$", re.I)
ARROW_RE = re.compile(r"^\s*--->\s*(.+?)\s*$")


def parse_lod(path: pathlib.Path) -> dict[str, str]:
    current: str | None = None
    out: dict[str, str] = {}
    for raw in path.read_text(errors="replace").splitlines():
        m = IMG_RE.match(raw)
        if m:
            current = m.group(1).lower()
            continue
        m = ARROW_RE.match(raw)
        if not m or not current:
            continue
        for label in m.group(1).split(","):
            label = label.strip().upper()
            if label:
                out[label] = current
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--lod", required=True, type=pathlib.Path)
    ap.add_argument("--require", action="append", default=[])
    ns = ap.parse_args()
    mapping = parse_lod(ns.lod)
    missing = []
    for name in ns.require:
        target = mapping.get(name.upper())
        if target is None:
            missing.append(name)
        else:
            print(f"{name.upper()} -> {target}")
    if missing:
        print("missing labels: " + ", ".join(missing), file=sys.stderr)
        return 2
    if not ns.require:
        for label in sorted(mapping):
            print(f"{label} -> {mapping[label]}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
