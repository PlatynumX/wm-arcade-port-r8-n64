#!/usr/bin/env python3
"""Generate the Fix39 Sports background from the tracked WIMP override.

The stock port's sports_background_bundle.py intentionally takes the two
SPORTBKBMOD palettes from BGNDPAL.ASM.  The repurposed SPORTBK.IMG is different:
its MIDWAY WIMP object was round-tripped with a replacement PLATYNUMX palette.
Using the old BGNDPAL colors keeps the replacement pixels but renders them nearly
black on hardware.

This override converter preserves the stock SPORTBKBMOD image/stride/layout
semantics and takes each image's palette directly from the authoritative WIMP
container.  SPORTBK's 19-color WIMP palette is unchanged from the arcade data;
only the repurposed MIDWAY object's 29-color palette differs.
"""
from __future__ import annotations

import argparse
import pathlib
import sys

import wimpimg


def rgba5551(word: int, alpha: int = 1) -> int:
    r = (word >> 10) & 31
    g = (word >> 5) & 31
    b = word & 31
    return (r << 11) | (g << 6) | (b << 1) | (alpha & 1)


def emit_array(lines, ctype, name, vals, fmt, per):
    lines.append(f"static {ctype} {name}[] __attribute__((aligned(8))) = {{")
    for i in range(0, len(vals), per):
        lines.append("    " + ", ".join(fmt.format(v) for v in vals[i:i + per]) + ",")
    lines.append("};")
    lines.append("")


def choose_named(images, name: str, width: int, height: int):
    matches = [im for im in images if im.name.upper() == name.upper()]
    if len(matches) != 1:
        names = ", ".join(f"{im.name}:{im.width}x{im.height}" for im in images)
        raise ValueError(
            f"SPORTBK.IMG must contain exactly one {name}; found {len(matches)}. "
            f"Images: {names}"
        )
    im = matches[0]
    if im.width != width or im.height != height:
        raise ValueError(
            f"{name}: expected WIMP logical size {width}x{height}, "
            f"found {im.width}x{im.height}"
        )
    return im


def read_padded_ci8(data: bytes, image):
    """Return exact WIMP stored rows, including 4-byte row padding."""
    stride = (image.width + 3) & ~3
    size = stride * image.height
    start = image.data_offset
    end = start + size
    if start < 0 or end > len(data):
        raise ValueError(
            f"{image.name}: padded payload outside source: "
            f"off=0x{start:X} stride={stride} height={image.height}"
        )
    return bytes(data[start:end]), stride


def palette_words_for(data, image, images, palettes, expected_count: int):
    pal = wimpimg.palette_for_image(image, images, palettes)
    words = wimpimg.read_palette_words(data, pal)
    if len(words) != expected_count:
        raise ValueError(
            f"{image.name}: expected {expected_count} source palette colors, "
            f"found {len(words)} in {pal.name}"
        )
    return words, pal.name


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True, type=pathlib.Path)
    ap.add_argument("--out", required=True, type=pathlib.Path)
    ns = ap.parse_args()

    data, _header, images, palettes = wimpimg.parse_file(ns.source)

    # Exact WIMP objects behind BGNDTBL.ASM::BKHDRS.
    sportbk = choose_named(images, "SPORTBK", 100, 100)
    midway = choose_named(images, "MIDWAY", 98, 109)
    sportbk_px, sportbk_stride = read_padded_ci8(data, sportbk)
    midway_px, midway_stride = read_padded_ci8(data, midway)

    if sportbk_stride != 100 or midway_stride != 100:
        raise ValueError(
            f"BKHDRS stride mismatch: SPORTBK={sportbk_stride}, MIDWAY={midway_stride}; "
            "expected 100/100"
        )

    # The live BMOD palette pairing is source-proven by the original converter:
    # MIDWAY uses the 29-color SPORTBK palette, while the tiled SPORTBK image
    # uses the 19-color SPORTBK_P family.  For the override, read those exact
    # words from the WIMP image instead of silently substituting old BGNDPAL art.
    p0_words, p0_name = palette_words_for(data, midway, images, palettes, 29)
    p1_words, p1_name = palette_words_for(data, sportbk, images, palettes, 19)

    p0_opaque = [rgba5551(v, 1) for v in p0_words]
    p0_keyed = [0 if i == 0 else rgba5551(v, 1) for i, v in enumerate(p0_words)]
    p1_opaque = [rgba5551(v, 1) for v in p1_words]
    p1_keyed = [0 if i == 0 else rgba5551(v, 1) for i, v in enumerate(p1_words)]

    lines = [
        "/* Auto-generated from Fix39 override IMG/SPORTBK.IMG.",
        "   BGNDTBL.ASM::BKHDRS = 100x100,100x109.",
        "   MIDWAY is logically 98x109 in WIMP with a real 100-byte padded CI8 row.",
        "   Pixel rows and the override WIMP palettes are preserved verbatim. */",
        '#include "wm/sports_background.h"',
        "",
    ]
    emit_array(lines, "uint16_t", "sportbk_opaque", p0_opaque, "0x{:04X}", 12)
    emit_array(lines, "uint16_t", "sportbk_keyed", p0_keyed, "0x{:04X}", 12)
    emit_array(lines, "uint16_t", "sportbk_p_opaque", p1_opaque, "0x{:04X}", 12)
    emit_array(lines, "uint16_t", "sportbk_p_keyed", p1_keyed, "0x{:04X}", 12)
    emit_array(lines, "const uint8_t", "sportbk_px_0", list(sportbk_px), "0x{:02X}", 24)
    emit_array(lines, "const uint8_t", "sportbk_px_1", list(midway_px), "0x{:02X}", 24)
    lines += [
        "static const wm_sports_background_image images[] = {",
        "    {100, 100, sportbk_px_0},",
        "    {100, 109, sportbk_px_1},",
        "};",
        "",
        "static const wm_sports_background_palette palettes[] = {",
        '    {"SPORTBK", sportbk_opaque, sportbk_keyed, 29},',
        '    {"SPORTBK_P", sportbk_p_opaque, sportbk_p_keyed, 19},',
        "};",
        "",
        "size_t wm_sports_background_image_count(void) { return sizeof(images)/sizeof(images[0]); }",
        "const wm_sports_background_image *wm_sports_background_image_at(size_t index) {",
        "    return index < wm_sports_background_image_count() ? &images[index] : 0;",
        "}",
        "size_t wm_sports_background_palette_count(void) { return sizeof(palettes)/sizeof(palettes[0]); }",
        "const wm_sports_background_palette *wm_sports_background_palette_at(size_t index) {",
        "    return index < wm_sports_background_palette_count() ? &palettes[index] : 0;",
        "}",
        "",
    ]

    ns.out.parent.mkdir(parents=True, exist_ok=True)
    ns.out.write_text("\n".join(lines))
    print(
        "generated Fix39 SPORTBKBMOD override: "
        f"SPORTBK 100x100 palette={p1_name}/19; "
        f"MIDWAY 98x109->100 stride palette={p0_name}/29"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"fix39_sports_background_bundle: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
