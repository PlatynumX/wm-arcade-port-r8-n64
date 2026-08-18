#!/usr/bin/env python3
"""Recover a full-screen background directly from the original artist source.

The Williams/Midway background compiler's .BDB text file keeps source-image
coordinates for every block. BGNDTBL.ASM keeps the final BMOD dimensions. For a
screen whose source lives in one WIMP image, the exact visible composite can be
recovered by cropping the source image at the minimum block X/Y with the BMOD
width/height. This preserves the original artwork and avoids recreating the
background from screenshots or hand placement.
"""
from __future__ import annotations
import argparse
import pathlib
import re
import sys

import wimpimg


def parse_int(text: str, base: int = 10) -> int:
    return int(text.rstrip('Hh'), 16 if text.lower().endswith('h') else base)


def parse_bdb(path: pathlib.Path, region: str):
    lines = [ln.strip() for ln in path.read_text(errors="replace").splitlines() if ln.strip()]
    if len(lines) < 3:
        raise ValueError(f"{path.name}: BDB too short")
    header = lines[0].split()
    if len(header) < 3:
        raise ValueError(f"{path.name}: malformed BDB header")
    source_name = header[0]
    canvas_w, canvas_h = int(header[1]), int(header[2])

    region_i = None
    region_bounds = None
    for i, line in enumerate(lines[1:], start=1):
        parts = line.split()
        if parts and parts[0].upper() == region.upper():
            if len(parts) != 5:
                raise ValueError(f"{path.name}: malformed {region} region line")
            region_i = i
            region_bounds = tuple(int(v) for v in parts[1:5])
            break
    if region_i is None or region_bounds is None:
        raise ValueError(f"{path.name}: region {region} not found")

    records = []
    for line in lines[region_i + 1:]:
        parts = line.split()
        if len(parts) != 5:
            break
        # First field is a hexadecimal flags word. X/Y are decimal source coords.
        try:
            int(parts[0], 16)
            x = int(parts[1], 10)
            y = int(parts[2], 10)
            int(parts[3], 16)
            int(parts[4], 16)
        except ValueError:
            break
        records.append((x, y))
    if not records:
        raise ValueError(f"{path.name}: no block records after {region}")
    return source_name, canvas_w, canvas_h, region_bounds, records


def parse_bmod_size(path: pathlib.Path, module: str) -> tuple[int, int, int]:
    text = path.read_text(errors="replace")
    pat = re.compile(
        rf"^\s*{re.escape(module)}:\s*\n\s*\.word\s+([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)",
        re.I | re.M,
    )
    m = pat.search(text)
    if not m:
        raise ValueError(f"{path.name}: module {module} size not found")
    return tuple(int(v) for v in m.groups())


def choose_source_image(images, source_name: str, region: str,
                        canvas_w: int, canvas_h: int,
                        abs_x: int, abs_y: int, width: int, height: int,
                        region_bounds: tuple[int, int, int, int]):
    by_name = {im.name.upper(): im for im in images}
    for name in (source_name, region):
        im = by_name.get(name.upper())
        if im is not None:
            return im

    # Deterministic fallback: a single image that can contain the absolute crop,
    # preferably matching the BDB canvas dimensions.
    fits = [im for im in images if im.width >= abs_x + width and im.height >= abs_y + height]
    exact_canvas = [im for im in fits if im.width == canvas_w and im.height == canvas_h]
    if len(exact_canvas) == 1:
        return exact_canvas[0]
    if len(fits) == 1:
        return fits[0]

    # Some artist containers store the named region rather than the full canvas.
    x0, x1, y0, y1 = region_bounds
    rel_x, rel_y = abs_x - x0, abs_y - y0
    rel_fits = [im for im in images if im.width >= rel_x + width and im.height >= rel_y + height]
    if len(rel_fits) == 1:
        return rel_fits[0]
    raise ValueError(
        f"cannot uniquely select WIMP image for {region}; source={source_name} "
        f"images={len(images)} abs_fits={len(fits)} rel_fits={len(rel_fits)}"
    )


def crop_ci8(pixels: bytes, src_w: int, src_h: int,
             x: int, y: int, w: int, h: int) -> bytes:
    if x < 0 or y < 0 or x + w > src_w or y + h > src_h:
        raise ValueError(f"crop {x},{y} {w}x{h} outside {src_w}x{src_h}")
    out = bytearray(w * h)
    for row in range(h):
        s = (y + row) * src_w + x
        d = row * w
        out[d:d+w] = pixels[s:s+w]
    return bytes(out)


def emit(source_img: pathlib.Path, bdb: pathlib.Path, bgndtbl: pathlib.Path,
         region: str, module: str, out: pathlib.Path) -> tuple[int, int, int, int]:
    source_name, canvas_w, canvas_h, bounds, records = parse_bdb(bdb, region)
    width, height, _blocks = parse_bmod_size(bgndtbl, module)
    abs_x = min(x for x, _ in records)
    abs_y = min(y for _, y in records)

    data, _header, images, palettes = wimpimg.parse_file(source_img)
    im = choose_source_image(images, source_name, region, canvas_w, canvas_h,
                             abs_x, abs_y, width, height, bounds)
    full = wimpimg.read_ci8(data, im)

    # Absolute canvas crop for full-canvas images; region-relative crop for
    # region-only images. Exact-size region images naturally use 0,0.
    if im.width >= abs_x + width and im.height >= abs_y + height:
        crop_x, crop_y = abs_x, abs_y
    else:
        x0, _x1, y0, _y1 = bounds
        crop_x, crop_y = abs_x - x0, abs_y - y0
        if im.width == width and im.height == height:
            crop_x = crop_y = 0
    px = crop_ci8(full, im.width, im.height, crop_x, crop_y, width, height)
    pal = wimpimg.palette_for_image(im, images, palettes)
    # BMOD artwork is an opaque background, not a keyed sprite. WIMP sprite
    # conversion makes palette index 0 transparent, but doing that here would
    # incorrectly punch holes in source backgrounds. Preserve every RGB555
    # entry and force RGBA5551 alpha on.
    vals = []
    for v in wimpimg.read_palette_words(data, pal):
        r = (v >> 10) & 31
        g = (v >> 5) & 31
        b = v & 31
        vals.append((r << 11) | (g << 6) | (b << 1) | 1)

    lines = [
        "/* Auto-generated crop from original Midway background artist source. */",
        '#include "wm/title_screen.h"',
        "",
        "static uint16_t title_pal[] __attribute__((aligned(8))) = {",
    ]
    for i in range(0, len(vals), 12):
        lines.append("    " + ", ".join(f"0x{v:04X}" for v in vals[i:i+12]) + ",")
    lines += ["};", "", "static const uint8_t title_px[] __attribute__((aligned(8))) = {"]
    for i in range(0, len(px), 24):
        lines.append("    " + ", ".join(f"0x{v:02X}" for v in px[i:i+24]) + ",")
    lines += [
        "};", "",
        "static const wm_source_sprite title_sprite = {",
        f'    "{region}", "{source_img.name}:{bdb.name}", {width}, {height}, 0, 0,',
        "    {0,0,0,0,0,0,0,0,0}, title_px, title_pal,",
        f"    {len(vals)}",
        "};", "",
        "const wm_source_sprite *wm_title_screen_sprite(void) { return &title_sprite; }", "",
    ]
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines))
    return width, height, crop_x, crop_y


def find_bdb(img_dir: pathlib.Path, region: str) -> pathlib.Path:
    matches = []
    for p in sorted(img_dir.glob("*.BDB")):
        try:
            text = p.read_text(errors="replace")
        except OSError:
            continue
        if re.search(rf"^\s*{re.escape(region)}\s+", text, re.I | re.M):
            matches.append(p)
    if len(matches) != 1:
        raise ValueError(f"expected one BDB containing {region}, found {[p.name for p in matches]}")
    return matches[0]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--img-dir", required=True, type=pathlib.Path)
    ap.add_argument("--bgndtbl", required=True, type=pathlib.Path)
    ap.add_argument("--region", required=True)
    ap.add_argument("--module", required=True)
    ap.add_argument("--out", required=True, type=pathlib.Path)
    ns = ap.parse_args()
    bdb = find_bdb(ns.img_dir, ns.region)
    source_img = bdb.with_suffix(".IMG")
    if not source_img.is_file():
        raise ValueError(f"artist source missing beside {bdb.name}: {source_img.name}")
    w, h, x, y = emit(source_img, bdb, ns.bgndtbl, ns.region, ns.module, ns.out)
    print(f"generated {ns.region} {w}x{h} from {source_img.name}/{bdb.name} crop=({x},{y})")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"background_crop: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
