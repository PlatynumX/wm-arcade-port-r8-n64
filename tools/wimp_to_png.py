#!/usr/bin/env python3
"""Decode a Midway WIMP .IMG container's images to individual PNG files.

For asset-replacement workflows: exports the original artist source frames
(CI8 pixels + RGB555 palette, before any DMA2/ROM packing) as standard RGBA
PNGs an image editor can open directly, sized/named to match the original so
replacement art can be dropped in 1:1. Does not modify or re-pack anything;
read-only against the source .IMG file.
"""
from __future__ import annotations

import argparse
import pathlib
import re
import struct
import sys
import zlib

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from wimpimg import (  # noqa: E402
    ImageEntry,
    parse_file,
    palette_for_image,
    read_ci8,
    read_palette_words,
)


def _png_chunk(tag: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + tag
        + payload
        + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)
    )


def write_png(path: pathlib.Path, width: int, height: int, rgba: bytes) -> None:
    """rgba is width*height*4 bytes, row-major, top-to-bottom."""
    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    raw = bytearray()
    stride = width * 4
    for y in range(height):
        raw.append(0)  # filter type: None
        raw += rgba[y * stride:(y + 1) * stride]
    idat = zlib.compress(bytes(raw), 9)
    path.write_bytes(
        sig
        + _png_chunk(b"IHDR", ihdr)
        + _png_chunk(b"IDAT", idat)
        + _png_chunk(b"IEND", b"")
    )


def image_to_rgba(data: bytes, image: ImageEntry, all_images: list[ImageEntry],
                   palettes) -> bytes:
    pal = palette_for_image(image, all_images, palettes)
    words = read_palette_words(data, pal)
    px = read_ci8(data, image)
    lut = []
    for i, w in enumerate(words):
        if i == 0:
            lut.append((0, 0, 0, 0))  # index 0 is transparent, matches N64 backend
            continue
        r = (w >> 10) & 0x1F
        g = (w >> 5) & 0x1F
        b = w & 0x1F
        lut.append(((r * 255 + 15) // 31, (g * 255 + 15) // 31, (b * 255 + 15) // 31, 255))
    while len(lut) < 256:
        lut.append((0, 0, 0, 0))

    out = bytearray(image.width * image.height * 4)
    for i, idx in enumerate(px):
        r, g, b, a = lut[idx]
        o = i * 4
        out[o] = r
        out[o + 1] = g
        out[o + 2] = b
        out[o + 3] = a
    return bytes(out)


def safe_name(name: str) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]", "_", name)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("file", type=pathlib.Path)
    ap.add_argument("--out", required=True, type=pathlib.Path,
                     help="output directory for this container's PNGs")
    ns = ap.parse_args()

    data, header, images, palettes = parse_file(ns.file)
    ns.out.mkdir(parents=True, exist_ok=True)
    for im in images:
        if im.width <= 0 or im.height <= 0:
            continue
        rgba = image_to_rgba(data, im, images, palettes)
        write_png(ns.out / f"{safe_name(im.name)}.png", im.width, im.height, rgba)
    print(f"{ns.file}: wrote {len(images)} PNGs -> {ns.out}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, struct.error) as exc:
        print(f"wimp_to_png: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
