#!/usr/bin/env python3
"""Generate the Midway Sports logo directly from the original WIMP container."""
from __future__ import annotations
import argparse
import pathlib
import sys

import wimpimg

SPORTS_LOGO_NAMES = [f"SPRTLG{i:02d}" for i in range(1, 18)]


def emit(source: pathlib.Path, out: pathlib.Path, required: list[str] | None = None) -> tuple[int, int]:
    required = list(required or SPORTS_LOGO_NAMES)
    data, _header, images, palettes = wimpimg.parse_file(source)
    by_name = {im.name.upper(): im for im in images}
    missing = [name for name in required if name.upper() not in by_name]
    if missing:
        raise ValueError("SPORTLO8 missing source logo pieces: " + ", ".join(missing))

    # Keep the exact LOGO_LIST order from ATTRACT.ASM.  BEGINOBJ uses one object
    # anchor for every piece; each WIMP entry's xani/yani is the private hotspot
    # that positions the piece relative to that common anchor.
    wimpimg.emit_c(
        out, data, images, palettes, required, source.name,
        header_include="wm/sports_logo.h",
        api_prefix="wm_sports_logo_sprite",
    )
    pixels = sum(by_name[name.upper()].width * by_name[name.upper()].height for name in required)
    return len(required), pixels


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True, type=pathlib.Path)
    ap.add_argument("--out", required=True, type=pathlib.Path)
    ns = ap.parse_args()
    count, pixels = emit(ns.source, ns.out)
    print(f"generated {count} Midway Sports logo pieces ({pixels} CI8 pixels) from {ns.source}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"frontend_bundle: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
