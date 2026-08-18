#!/usr/bin/env python3
"""Bundle all Bret source frames referenced by generated visual tables.

This reads BRET.LOD to find which WIMP .IMG container owns each source frame,
then converts those CI8 pixels/palettes into N64-ready C data.  Artwork itself
stays a build product; it is not checked into the source package.
"""
from __future__ import annotations
import argparse
import pathlib
import re
import sys
from collections import OrderedDict

import bret_manifest
import wimpimg

FRAME_RE = re.compile(r'\{"([A-Z][A-Z0-9_]*[0-9]{2})"\s*,\s*[0-9]+\}')


def c_ident(s: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", s.lower())


def collect_frames(paths: list[pathlib.Path]) -> list[str]:
    out: list[str] = []
    seen: set[str] = set()
    for path in paths:
        for match in FRAME_RE.finditer(path.read_text(errors="replace")):
            name = match.group(1).upper()
            if name not in seen:
                seen.add(name)
                out.append(name)
    if not out:
        raise ValueError("no source frame references found in generated visual tables")
    return out


def resolve_case_insensitive(directory: pathlib.Path, filename: str) -> pathlib.Path:
    key = filename.lower()
    for p in directory.iterdir():
        if p.is_file() and p.name.lower() == key:
            return p
    raise ValueError(f"WIMP container not found in {directory}: {filename}")


def rgba5551(rgb555: int, index: int) -> int:
    if index == 0:
        return 0
    r = (rgb555 >> 10) & 31
    g = (rgb555 >> 5) & 31
    b = rgb555 & 31
    return (r << 11) | (g << 6) | (b << 1) | 1


def emit(out_path: pathlib.Path, lod: pathlib.Path, img_dir: pathlib.Path,
         visual_sources: list[pathlib.Path]) -> tuple[int, int]:
    frames = collect_frames(visual_sources)
    mapping = bret_manifest.parse_lod(lod)
    missing = [f for f in frames if f not in mapping]
    if missing:
        raise ValueError("BRET.LOD has no container mapping for: " + ", ".join(missing))

    # Load every needed WIMP container exactly once.
    containers: OrderedDict[str, tuple[pathlib.Path, bytes, list, list]] = OrderedDict()
    for frame in frames:
        container = mapping[frame]
        if container in containers:
            continue
        path = resolve_case_insensitive(img_dir, container)
        data, _header, images, palettes = wimpimg.parse_file(path)
        containers[container] = (path, data, images, palettes)

    # Resolve frame -> WIMP entry/palette and fail before writing partial output.
    resolved = []
    for frame in frames:
        container = mapping[frame]
        path, data, images, palettes = containers[container]
        by_name = {im.name.upper(): im for im in images}
        im = by_name.get(frame)
        if im is None:
            raise ValueError(f"{frame}: listed in BRET.LOD but missing from {path.name}")
        pal = wimpimg.palette_for_image(im, images, palettes)
        resolved.append((frame, container, data, im, pal))

    # r6h4: do not guess LOAD2's PWRD mapping.  r6h3 proved that the first
    # two words of the raw WIMP tail are not the runtime channel-2 coordinates.
    # Preserve all nine tail words and let the N64 attachment scanner select a
    # candidate pair live.  This turns the next hardware run into one bounded
    # experiment instead of another rebuild-per-offset loop.

    lines = [
        "/* Auto-generated from the original Midway Bret WIMP containers. */",
        '#include "wm/bret_sprites.h"',
        "#include <string.h>",
        "",
    ]

    # Palette identity includes container + palette directory offset to avoid name collisions.
    palette_symbols: dict[tuple[str, int], str] = {}
    for container, (_path, data, _images, palettes) in containers.items():
        for pal in palettes:
            used = any(c == container and p.directory_offset == pal.directory_offset
                       for _f, c, _d, _i, p in resolved)
            if not used:
                continue
            key = (container, pal.directory_offset)
            ident = f"pal_{c_ident(pathlib.Path(container).stem)}_{c_ident(pal.name)}_{pal.directory_offset:x}"
            palette_symbols[key] = ident
            vals = wimpimg.read_palette_words(data, pal)
            converted = [rgba5551(v, i) for i, v in enumerate(vals)]
            lines.append(f"static uint16_t {ident}[] __attribute__((aligned(8))) = {{")
            for i in range(0, len(converted), 12):
                lines.append("    " + ", ".join(f"0x{v:04X}" for v in converted[i:i+12]) + ",")
            lines.append("};")
            lines.append("")

    pixel_symbols: dict[str, str] = {}
    total_pixels = 0
    for frame, _container, data, im, _pal in resolved:
        ident = f"px_{c_ident(frame)}"
        pixel_symbols[frame] = ident
        px = wimpimg.read_ci8(data, im)
        total_pixels += len(px)
        lines.append(f"static const uint8_t {ident}[] __attribute__((aligned(8))) = {{")
        for i in range(0, len(px), 24):
            lines.append("    " + ", ".join(f"0x{v:02X}" for v in px[i:i+24]) + ",")
        lines.append("};")
        lines.append("")

    lines.append("static const wm_source_sprite sprites[] = {")
    for frame, container, _data, im, pal in resolved:
        psym = palette_symbols[(container, pal.directory_offset)]
        lines.append(
            f'    {{"{frame}", "{container}", {im.width}, {im.height}, {im.xani}, {im.yani}, '
            f'{{' + ', '.join(str(v) for v in im.tail_words) + f'}}, {pixel_symbols[frame]}, {psym}, {pal.color_count}}},'
        )
    lines += [
        "};",
        "",
        "const wm_source_sprite *wm_bret_sprite_find(const char *source_frame) {",
        "    if (!source_frame) return 0;",
        "    for (size_t i = 0; i < sizeof(sprites)/sizeof(sprites[0]); ++i)",
        "        if (strcmp(sprites[i].source_frame, source_frame) == 0) return &sprites[i];",
        "    return 0;",
        "}",
        "",
        "size_t wm_bret_sprite_count(void) {",
        "    return sizeof(sprites)/sizeof(sprites[0]);",
        "}",
        "",
    ]
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines))
    return len(resolved), total_pixels


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--lod", required=True, type=pathlib.Path)
    ap.add_argument("--img-dir", required=True, type=pathlib.Path)
    ap.add_argument("--visual-source", action="append", required=True, type=pathlib.Path)
    ap.add_argument("--out", required=True, type=pathlib.Path)
    ns = ap.parse_args()
    count, pixels = emit(ns.out, ns.lod, ns.img_dir, ns.visual_source)
    print(f"generated {ns.out}: {count} unique Bret frames, {pixels} CI8 pixels")
    # Dump the raw tail for a few source frames.  These lines are intentionally
    # visible in Actions logs so the exact WIMP metadata survives even if a
    # hardware photo is hard to read.
    try:
        mapping = bret_manifest.parse_lod(ns.lod)
        samples = ("H4ST4A02", "H2ST2A05", "H4WL4A01", "H2WL1A01",
                   "H4TW4A01", "H2TW2A01")
        cached = {}
        for frame in samples:
            container = mapping.get(frame)
            if not container:
                continue
            if container not in cached:
                path = resolve_case_insensitive(ns.img_dir, container)
                _data, _hdr, imgs, _pals = wimpimg.parse_file(path)
                cached[container] = {im.name.upper(): im for im in imgs}
            im = cached[container].get(frame)
            if im:
                vals = ",".join(str(v) for v in im.tail_words)
                print(f"wimp-tail {frame}: ani=({im.xani},{im.yani}) [{vals}]")
    except Exception as exc:
        print(f"WIMP-tail diagnostic unavailable: {exc}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"bret_bundle: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
