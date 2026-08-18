#!/usr/bin/env python3
"""Generate ATTRACT.ASM::dcslogo from the original DCSLOGO.IMG WIMP container."""
from __future__ import annotations
import argparse
import pathlib
import sys

import wimpimg


def pick_logo(images: list[wimpimg.ImageEntry]) -> wimpimg.ImageEntry:
    for im in images:
        if im.name.upper() == "DCSLOGO":
            return im
    # The source container is dedicated to this logo.  Some WIMP revisions use
    # a shorter internal symbol; accept a single-image container without
    # inventing a substitute asset name.
    if len(images) == 1:
        return images[0]
    raise ValueError("DCSLOGO.IMG has no DCSLOGO entry and is not single-image")


def emit(source: pathlib.Path, out: pathlib.Path) -> tuple[str, int]:
    data, _header, images, palettes = wimpimg.parse_file(source)
    logo = pick_logo(images)
    wimpimg.emit_c(
        out, data, images, palettes, [logo.name], source.name,
        header_include="wm/dcs_logo.h",
        api_prefix="wm_dcs_logo_raw",
    )
    text = out.read_text()
    # The generic WIMP emitter exposes find/at/count.  Keep those functions
    # private to the generated TU and export the source routine's one object.
    text = text.replace("const wm_source_sprite *wm_dcs_logo_raw_find", "static const wm_source_sprite *wm_dcs_logo_raw_find")
    text = text.replace("const wm_source_sprite *wm_dcs_logo_raw_at", "static const wm_source_sprite *wm_dcs_logo_raw_at")
    text = text.replace("size_t wm_dcs_logo_raw_count", "static size_t wm_dcs_logo_raw_count")
    text += "\nconst wm_source_sprite *wm_dcs_logo_sprite(void) {\n    return wm_dcs_logo_raw_at(0);\n}\n"
    # Avoid -Wunused-function under the CI -Werror compile: the generated
    # find/count helpers are not part of the dedicated single-logo API.
    text = text.replace("static const wm_source_sprite *wm_dcs_logo_raw_find", "const wm_source_sprite *wm_dcs_logo_raw_find")
    text = text.replace("static size_t wm_dcs_logo_raw_count", "size_t wm_dcs_logo_raw_count")
    out.write_text(text)
    return logo.name, logo.width * logo.height


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True, type=pathlib.Path)
    ap.add_argument("--out", required=True, type=pathlib.Path)
    ns = ap.parse_args()
    name, pixels = emit(ns.source, ns.out)
    print(f"generated DCS logo {name} ({pixels} CI8 pixels) from {ns.source}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"dcs_bundle: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
