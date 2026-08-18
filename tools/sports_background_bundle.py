#!/usr/bin/env python3
"""Generate exact SPORTBKBMOD source images and BKPALS palettes.

Important source-format detail:
- BGNDTBL.ASM::BKHDRS declares 100x100 and 100x109 DMA/header images.
- The WIMP artist file stores those as SPORTBK 100x100 and MIDWAY 98x109.
- WIMP CI8 rows are padded to a 4-byte boundary, so MIDWAY's real stored row
  stride is 100 bytes. LOAD2/BGNDTBL keeps that padded width in BKHDRS.

Therefore this generator preserves the original WIMP row padding bytes instead
of stripping them and then inventing replacements.
"""
from __future__ import annotations
import argparse
import pathlib
import sys
import wimpimg

# BGNDPAL.ASM::SPORTBK and SPORTBK_P, verbatim RGB555.
SPORTBK_PAL = [
    0x1108,0x0401,0x0C04,0x0803,0x1006,0x1408,0x0C05,0x1007,0x1409,0x0402,
    0x0804,0x0C06,0x1008,0x0C07,0x0805,0x0403,0x0404,0x0001,0x0002,0x28CD,
    0x24AC,0x20AB,0x1C8A,0x1869,0x1468,0x1047,0x0C46,0x0C25,0x0824,
]
SPORTBK_P_PAL = [
    0x0000,0x0401,0x0C04,0x0803,0x1006,0x1408,0x0C05,0x1007,0x1409,0x0402,
    0x0804,0x0C06,0x1008,0x0C07,0x0805,0x0403,0x0404,0x0001,0x0002,
]

def rgba5551(word: int, alpha: int = 1) -> int:
    r = (word >> 10) & 31
    g = (word >> 5) & 31
    b = word & 31
    return (r << 11) | (g << 6) | (b << 1) | (alpha & 1)

def emit_array(lines, ctype, name, vals, fmt, per):
    lines.append(f"static {ctype} {name}[] __attribute__((aligned(8))) = {{")
    for i in range(0, len(vals), per):
        lines.append("    " + ", ".join(fmt.format(v) for v in vals[i:i+per]) + ",")
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
    """Return the exact WIMP stored rows, including 4-byte row padding."""
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

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True, type=pathlib.Path)
    ap.add_argument("--out", required=True, type=pathlib.Path)
    ns = ap.parse_args()

    data, _header, images, _palettes = wimpimg.parse_file(ns.source)

    # These are the actual WIMP source objects behind BGNDTBL.ASM::BKHDRS.
    sportbk = choose_named(images, "SPORTBK", 100, 100)
    midway = choose_named(images, "MIDWAY", 98, 109)

    sportbk_px, sportbk_stride = read_padded_ci8(data, sportbk)
    midway_px, midway_stride = read_padded_ci8(data, midway)

    # BKHDRS is 100x100 and 100x109. Both WIMP row strides must therefore be 100.
    if sportbk_stride != 100 or midway_stride != 100:
        raise ValueError(
            f"BKHDRS stride mismatch: SPORTBK={sportbk_stride}, MIDWAY={midway_stride}; "
            "expected 100/100"
        )

    p0_opaque = [rgba5551(v, 1) for v in SPORTBK_PAL]
    p0_keyed  = [0 if i == 0 else rgba5551(v, 1) for i, v in enumerate(SPORTBK_PAL)]
    p1_opaque = [rgba5551(v, 1) for v in SPORTBK_P_PAL]
    p1_keyed  = [0 if i == 0 else rgba5551(v, 1) for i, v in enumerate(SPORTBK_P_PAL)]

    lines = [
        "/* Auto-generated from original IMG/SPORTBK.IMG.",
        "   BGNDTBL.ASM::BKHDRS = 100x100,100x109.",
        "   MIDWAY is logically 98x109 in WIMP but has a real 100-byte padded CI8 row.",
        "   The two original padding bytes per row are preserved verbatim. */",
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
        "generated SPORTBKBMOD art: "
        "SPORTBK logical/DMA 100x100; "
        "MIDWAY logical 98x109 -> preserved DMA stride 100x109; "
        "BKPALS 29/19"
    )
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"sports_background_bundle: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
