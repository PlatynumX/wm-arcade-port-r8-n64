#!/usr/bin/env python3
"""Export the exact Bret WIMP frames used by the N64 build as transparent PNGs.

No Pillow dependency: PNG writing uses only the Python standard library. The same
r8 visual tables, BRET.LOD mapping, WIMP parser, CI8 pixels and RGB555 palettes
used by bret_bundle.py are used here, so this is a lossless inspection/export
path rather than a screenshot rip.
"""
from __future__ import annotations
import argparse
import csv
import pathlib
import shutil
import struct
import sys
import tempfile
import zipfile
import zlib

import bret_bundle
import bret_manifest
import wimpimg


def png_chunk(tag: bytes, payload: bytes) -> bytes:
    return (struct.pack(">I", len(payload)) + tag + payload +
            struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF))


def write_rgba_png(path: pathlib.Path, width: int, height: int, rgba: bytes) -> None:
    if len(rgba) != width * height * 4:
        raise ValueError(f"{path.name}: RGBA byte count mismatch")
    rows = bytearray()
    stride = width * 4
    for y in range(height):
        rows.append(0)  # filter: None
        rows.extend(rgba[y * stride:(y + 1) * stride])
    out = bytearray(b"\x89PNG\r\n\x1a\n")
    out += png_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    out += png_chunk(b"IDAT", zlib.compress(bytes(rows), 9))
    out += png_chunk(b"IEND", b"")
    path.write_bytes(out)


def rgb555_to_rgba8888(value: int, index: int) -> tuple[int, int, int, int]:
    # The port and original art path treat palette index 0 as transparent.
    if index == 0:
        return 0, 0, 0, 0
    r5 = (value >> 10) & 31
    g5 = (value >> 5) & 31
    b5 = value & 31
    return ((r5 << 3) | (r5 >> 2),
            (g5 << 3) | (g5 >> 2),
            (b5 << 3) | (b5 >> 2), 255)


def resolve_container(img_dir: pathlib.Path, name: str) -> pathlib.Path:
    key = name.lower()
    for p in img_dir.iterdir():
        if p.is_file() and p.name.lower() == key:
            return p
    raise ValueError(f"WIMP container not found: {name}")


def export_tree(lod: pathlib.Path, img_dir: pathlib.Path,
                visual_sources: list[pathlib.Path], out_dir: pathlib.Path) -> int:
    frames = bret_bundle.collect_frames(visual_sources)
    mapping = bret_manifest.parse_lod(lod)
    missing = [f for f in frames if f not in mapping]
    if missing:
        raise ValueError("BRET.LOD has no mapping for: " + ", ".join(missing))

    frame_dir = out_dir / "frames"
    frame_dir.mkdir(parents=True, exist_ok=True)
    cache = {}
    rows = []

    for frame in frames:
        container = mapping[frame]
        key = container.lower()
        if key not in cache:
            p = resolve_container(img_dir, container)
            data, _header, images, palettes = wimpimg.parse_file(p)
            cache[key] = (p, data, images, palettes,
                          {im.name.upper(): im for im in images})
        p, data, images, palettes, by_name = cache[key]
        im = by_name.get(frame.upper())
        if im is None:
            raise ValueError(f"{frame}: listed by BRET.LOD but absent from {p.name}")
        pal = wimpimg.palette_for_image(im, images, palettes)
        pal_words = wimpimg.read_palette_words(data, pal)
        rgba_pal = [rgb555_to_rgba8888(v, i) for i, v in enumerate(pal_words)]
        ci8 = wimpimg.read_ci8(data, im)
        rgba = bytearray(im.width * im.height * 4)
        for i, idx in enumerate(ci8):
            color = rgba_pal[idx] if idx < len(rgba_pal) else (0, 0, 0, 0)
            rgba[i * 4:i * 4 + 4] = bytes(color)
        write_rgba_png(frame_dir / f"{frame}.png", im.width, im.height, bytes(rgba))

        row = {
            "frame": frame,
            "container": p.name,
            "width": im.width,
            "height": im.height,
            "xani": im.xani,
            "yani": im.yani,
            "attach_raw_x_slot3": im.tail_words[3],
            "attach_raw_y_slot4": im.tail_words[4],
            "palette": pal.name,
            "palette_colors": pal.color_count,
        }
        for n, val in enumerate(im.tail_words):
            row[f"wimp_tail_{n}"] = val
        rows.append(row)

    fields = list(rows[0].keys())
    with (out_dir / "bret-r8-frame-metadata.csv").open("w", newline="") as f:
        wr = csv.DictWriter(f, fieldnames=fields)
        wr.writeheader()
        wr.writerows(rows)

    (out_dir / "README.txt").write_text(
        "WWF WrestleMania Arcade - Bret Hart frames used by N64 r8/r8h1\n\n"
        "frames/ contains exact WIMP CI8 source frames converted to RGBA PNG.\n"
        "Palette index 0 is transparent. No scaling, smoothing or cleanup was applied.\n"
        "bret-r8-frame-metadata.csv preserves image dimensions, animation origins,\n"
        "the hardware-tested channel-2 raw attachment pair (+38/+40 = tail 3/4),\n"
        "palette name/count, and all nine preserved WIMP tail words.\n"
        f"Frame count: {len(rows)}\n"
    )
    return len(rows)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--lod", required=True, type=pathlib.Path)
    ap.add_argument("--img-dir", required=True, type=pathlib.Path)
    ap.add_argument("--visual-source", action="append", required=True, type=pathlib.Path)
    ap.add_argument("--out-zip", required=True, type=pathlib.Path)
    ns = ap.parse_args()

    with tempfile.TemporaryDirectory(prefix="bret-pngs-") as td:
        out_dir = pathlib.Path(td) / "bret-r8-textures-png"
        out_dir.mkdir(parents=True)
        count = export_tree(ns.lod, ns.img_dir, ns.visual_source, out_dir)
        ns.out_zip.parent.mkdir(parents=True, exist_ok=True)
        if ns.out_zip.exists():
            ns.out_zip.unlink()
        with zipfile.ZipFile(ns.out_zip, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
            for p in sorted(out_dir.rglob("*")):
                if p.is_file():
                    zf.write(p, p.relative_to(out_dir))
    print(f"exported {count} Bret PNG frames -> {ns.out_zip}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"export_bret_pngs: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
