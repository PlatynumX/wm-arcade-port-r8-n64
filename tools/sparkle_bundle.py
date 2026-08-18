#!/usr/bin/env python3
"""Generate title sparkle sprites from the original Midway SPARKLE.IMG.

ATTRACT.ASM creates SPRINKLE_GLINTS and RANDOM_SPARKLE on show_title.  The
checked-in WWF tree references those shared routines externally, while MISC.LOD
names the WIMP frames they consume.  This generator preserves all named source
frames so the portable runtime can use the original artwork rather than drawing
replacement stars.
"""
from __future__ import annotations
import argparse
import pathlib
import sys

import wimpimg

BIG_A = [f"BSPRKA{i:02d}" for i in range(1, 16)]
BIG_B = [f"BSPRKB{i:02d}" for i in range(1, 16)]
SMALL_A = [f"SPRKLA{i:02d}" for i in range(1, 14)]
SMALL_B = [f"SPRKLB{i:02d}" for i in range(1, 14)]
SMALL_C = [f"SPRKLC{i:02d}" for i in range(1, 14)]
SPARKLE_NAMES = BIG_A + BIG_B + SMALL_A + SMALL_B + SMALL_C


def emit(source: pathlib.Path, out: pathlib.Path,
         required: list[str] | None = None) -> tuple[int, int]:
    required = list(required or SPARKLE_NAMES)
    data, _header, images, palettes = wimpimg.parse_file(source)
    by_name = {im.name.upper(): im for im in images}
    missing = [name for name in required if name.upper() not in by_name]
    if missing:
        raise ValueError("SPARKLE.IMG missing source frames: " + ", ".join(missing))

    wimpimg.emit_c(
        out, data, images, palettes, required, source.name,
        header_include="wm/title_sparkle.h",
        api_prefix="wm_title_sparkle_sprite",
    )
    pixels = sum(by_name[name.upper()].width * by_name[name.upper()].height
                 for name in required)
    return len(required), pixels


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True, type=pathlib.Path)
    ap.add_argument("--out", required=True, type=pathlib.Path)
    ns = ap.parse_args()
    count, pixels = emit(ns.source, ns.out)
    print(f"generated {count} original title sparkle frames ({pixels} CI8 pixels) from {ns.source}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"sparkle_bundle: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
