#!/usr/bin/env python3
"""Bundle real per-frame WIMP geometry (width/height/xani/yani) for every
Bret source frame referenced by the generated visual tables -- no pixel or
palette data, unlike tools/bret_bundle.py's wm_source_sprite output.

This is the engine-side counterpart to bret_bundle.py: hit-box derivation
(wm_hurt_box_for_frame) needs real per-frame image bounds on every
target, including the portable host build that never links asset pixel
data, so this emits a small standalone table instead of depending on
wm_bret_sprite_find.
"""
from __future__ import annotations
import argparse
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import bret_bundle  # noqa: E402
import bret_manifest  # noqa: E402
import wimpimg  # noqa: E402


# A WIMP image carries an eight-character name, so a longer one is stored
# truncated: BAM.LOD's BURNBODY01..BURNBODY05 are five images in
# bam_jms.img all called `BURNBODY`.
NAME_FIELD = 8


def _by_truncated_name(frame: str, lod_order: list[str], images: list,
                       by_name: dict):
    """The image a container holds under a name too long to spell.

    The .LOD lists a container's frames in packing order and the container
    stores them in that same order, so the nth full name that truncates to
    a stem is the nth image carrying that stem. That only holds if the two
    counts agree, so this resolves nothing unless they do.
    """
    stem = frame[:NAME_FIELD]
    if stem == frame:
        return None
    wanted = [n for n in lod_order
              if n[:NAME_FIELD] == stem and n not in by_name]
    have = [im for im in images if im.name.upper() == stem]
    if len(wanted) != len(have) or frame not in wanted:
        return None
    return have[wanted.index(frame)]


def emit(out_path: pathlib.Path, lods: list[pathlib.Path], img_dir: pathlib.Path,
         visual_sources: list[pathlib.Path]) -> int:
    frames = bret_bundle.collect_frames(visual_sources)
    # One mapping across every wrestler's .LOD. Frame names carry the
    # wrestler's own letter, so they do not collide -- but a name defined
    # in two LODs would silently take whichever came first, so that is
    # refused rather than resolved by luck.
    mapping: dict[str, str] = {}
    order: dict[str, list[str]] = {}
    for lod in lods:
        for name, container in bret_manifest.parse_lod(lod).items():
            if name in mapping and mapping[name] != container:
                raise ValueError(
                    f"{name} is mapped to {mapping[name]} and to {container}: "
                    f"two .LOD files disagree about the same frame")
            mapping[name] = container
            order.setdefault(container, []).append(name)
    missing = [f for f in frames if f not in mapping]
    if missing:
        raise ValueError("no .LOD container mapping for: " + ", ".join(missing))

    containers: dict[str, tuple[bytes, list]] = {}
    resolved = []
    for frame in frames:
        container = mapping[frame]
        if container not in containers:
            path = bret_bundle.resolve_case_insensitive(img_dir, container)
            data, _header, images, _palettes = wimpimg.parse_file(path)
            containers[container] = (data, images)
        _data, images = containers[container]
        by_name = {im.name.upper(): im for im in images}
        im = by_name.get(frame)
        if im is None:
            im = _by_truncated_name(frame, order[container], images, by_name)
        if im is None:
            raise ValueError(f"{frame}: listed in a .LOD but missing from {container}")
        resolved.append((frame, im))

    lines = [
        "/* Auto-generated from the original Midway wrestlers' WIMP",
        "   containers -- geometry only, no pixel/palette data. */",
        '#include "wm/frame_geometry.h"',
        "#include <string.h>",
        "",
        "static const wm_frame_geometry_t frames[] = {",
    ]
    for frame, im in resolved:
        lines.append(f'    {{"{frame}", {im.width}, {im.height}, {im.xani}, {im.yani}}},')
    lines += [
        "};",
        "",
        "const wm_frame_geometry_t *wm_frame_geometry_find(const char *source_frame) {",
        "    if (!source_frame) return 0;",
        "    for (size_t i = 0; i < sizeof(frames)/sizeof(frames[0]); ++i)",
        "        if (strcmp(frames[i].source_frame, source_frame) == 0) return &frames[i];",
        "    return 0;",
        "}",
        "",
        "size_t wm_frame_geometry_count(void) {",
        "    return sizeof(frames)/sizeof(frames[0]);",
        "}",
        "",
    ]
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines))
    return len(resolved)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--lod", required=True, action="append", type=pathlib.Path)
    ap.add_argument("--img-dir", required=True, type=pathlib.Path)
    ap.add_argument("--visual-source", action="append", required=True, type=pathlib.Path)
    ap.add_argument("--out", required=True, type=pathlib.Path)
    ns = ap.parse_args()
    count = emit(ns.out, ns.lod, ns.img_dir, ns.visual_source)
    print(f"generated {ns.out}: {count} unique frame geometries "
          f"from {len(ns.lod)} .LOD files")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"bret_geometry_bundle: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
