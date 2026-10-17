#!/usr/bin/env python3
"""Stream the shipped NEWRINGB arena art into DragonFS.

The shipped match background is not a flattened bitmap. ringBMOD contains
211 placements that reuse 95 BDD image headers with 23 palettes, plus the
source Z/flip/transparency bits. This keeps those identities intact.

No approximation is made here:
- image header index is proven from BDB source_id / 3 against every BMOD row
- BMOD palette index is proven against the BDB palette code on every row
- positions and Z are cross-checked against BDB
- the module footprint is cross-checked against the source images
"""
from __future__ import annotations

import argparse
import pathlib
import shutil
import sys

TOOLS = pathlib.Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

import bdd_bundle
import bmod_source

MAGIC = b"WR"
TLUT_COLORS = 256


def s16(v: int) -> int:
    return v - 0x10000 if v & 0x8000 else v


def rgba5551(word: int, alpha: int) -> int:
    r = (word >> 10) & 31
    g = (word >> 5) & 31
    b = word & 31
    return (r << 11) | (g << 6) | (b << 1) | (alpha & 1)


def pal_bytes(words: tuple[int, ...], transparent: bool) -> bytes:
    if len(words) > TLUT_COLORS:
        raise ValueError(f"palette has {len(words)} colors (>256)")
    out: list[int] = []
    for i, word in enumerate(words):
        out.append(rgba5551(word, 0 if transparent and i == 0 else 1))
    out.extend([0] * (TLUT_COLORS - len(out)))
    return b"".join(v.to_bytes(2, "big") for v in out)


def bdb_palette_code(index: int) -> int:
    """BDB writes palette numbers as digit pairs: 10 is stored as hex 0x10."""
    return ((index // 10) << 4) | (index % 10)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--bdd", required=True, type=pathlib.Path)
    ap.add_argument("--bdb", required=True, type=pathlib.Path)
    ap.add_argument("--bgndtbl", required=True, type=pathlib.Path)
    ap.add_argument("--region", default="ring")
    ap.add_argument("--module", default="ringBMOD")
    ap.add_argument("--out-fs", required=True, type=pathlib.Path)
    ns = ap.parse_args()

    images, palettes = bdd_bundle.parse_bdd(ns.bdd)
    region = bdd_bundle.parse_bdb(ns.bdb, ns.region)
    module = bmod_source.parse_module(
        ns.bgndtbl.read_text(errors="replace"), ns.module)

    if module["count"] != len(region.blocks):
        raise ValueError(
            f"{ns.module}: {module['count']} BMOD rows != "
            f"{len(region.blocks)} BDB placements")

    image_by_header: dict[int, bdd_bundle.BddImage] = {}
    for image in images:
        if image.source_id % 3:
            raise ValueError(
                f"BDD source id {image.source_id:X} is not divisible by 3")
        hi = image.source_id // 3
        if hi in image_by_header:
            raise ValueError(f"duplicate BDD header identity {hi}")
        image_by_header[hi] = image

    if set(image_by_header) != set(range(len(images))):
        raise ValueError("BDD source ids do not form dense source_id/3 headers")

    min_x = min(b.source_x for b in region.blocks)
    min_y = min(b.source_y for b in region.blocks)
    max_x = min_x
    max_y = min_y
    words = module["words"]

    for i, desc in enumerate(region.blocks):
        a, bx, by, hdr = words[i * 4:i * 4 + 4]
        hidx = hdr & 0x0FFF
        pal_index = (a & 0x000F) | ((hdr >> 8) & 0x00F0)
        z = (a >> 8) & 0xFF

        if desc.source_id % 3 or hidx != desc.source_id // 3:
            raise ValueError(
                f"block {i}: BMOD header {hidx} != "
                f"BDB source_id/3 ({desc.source_id:X}/3)")

        image = image_by_header.get(hidx)
        if image is None:
            raise ValueError(f"block {i}: BDD image for header {hidx} missing")

        expected_x = desc.source_x - min_x
        expected_y = desc.source_y - min_y
        if s16(bx) != expected_x or s16(by) != expected_y:
            raise ValueError(
                f"block {i}: BMOD pos {s16(bx)},{s16(by)} != "
                f"BDB-relative {expected_x},{expected_y}")

        if pal_index >= len(palettes):
            raise ValueError(
                f"block {i}: BMOD palette {pal_index} outside "
                f"{len(palettes)} BDD palettes")

        expected_bdb_pal = bdb_palette_code(pal_index)
        if desc.palette != expected_bdb_pal:
            raise ValueError(
                f"block {i}: BDB palette code {desc.palette:X} != "
                f"compiled palette {pal_index} (code {expected_bdb_pal:X})")

        if image.pixels and max(image.pixels) >= len(
                palettes[pal_index].words_rgb555):
            raise ValueError(
                f"block {i}: CI8 index {max(image.pixels)} exceeds "
                f"palette {pal_index} size "
                f"{len(palettes[pal_index].words_rgb555)}")

        if z != ((desc.source_control >> 8) & 0xFF):
            raise ValueError(
                f"block {i}: BMOD Z {z:X} != "
                f"BDB Z {(desc.source_control >> 8) & 0xFF:X}")

        max_x = max(max_x, desc.source_x + image.width)
        max_y = max(max_y, desc.source_y + image.height)

    footprint = (max_x - min_x, max_y - min_y)
    if (module["width"], module["height"]) != footprint:
        raise ValueError(
            f"{ns.module}: {module['width']}x{module['height']} != "
            f"BDD/BDB footprint {footprint[0]}x{footprint[1]}")

    if ns.out_fs.exists():
        shutil.rmtree(ns.out_fs)
    ns.out_fs.mkdir(parents=True, exist_ok=True)

    image_bytes = 0
    for hi in range(len(images)):
        image = image_by_header[hi]
        payload = (
            MAGIC
            + int(image.width).to_bytes(2, "big")
            + int(image.height).to_bytes(2, "big")
            + bytes(image.pixels)
        )
        (ns.out_fs / f"h{hi:03d}.bin").write_bytes(payload)
        image_bytes += len(payload)

    palette_bytes = 0
    for i, pal in enumerate(palettes):
        for transparent, suffix in ((False, "o"), (True, "t")):
            payload = pal_bytes(pal.words_rgb555, transparent)
            (ns.out_fs / f"p{i}_{suffix}.pal").write_bytes(payload)
            palette_bytes += len(payload)

    stamp = [
        "WrestleMania arcade ring background (source streamed)",
        f"module={ns.module}",
        f"region={ns.region}",
        f"blocks={len(region.blocks)}",
        f"images={len(images)}",
        f"palettes={len(palettes)}",
        f"image_bytes={image_bytes}",
        f"palette_bytes={palette_bytes}",
        f"footprint={footprint[0]}x{footprint[1]}",
        "format=image is 'WR' + big-endian u16 width + u16 height + CI8 pixels",
        "format=palette is 256 big-endian RGBA5551 entries; _t makes index 0 transparent",
        f"source={ns.bdd.name}+{ns.bdb.name}+{ns.bgndtbl.name}",
        "",
    ]
    (ns.out_fs / "STREAMED_RING.txt").write_text("\n".join(stamp))

    print(
        f"ring assets: {len(region.blocks)} blocks -> {len(images)} images, "
        f"{len(palettes)} palettes, {image_bytes + palette_bytes} bytes")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"stream_ring_assets: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
