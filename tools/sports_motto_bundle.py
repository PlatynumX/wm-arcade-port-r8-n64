#!/usr/bin/env python3
"""Generate the exact SGMD8 glyph subset used by the Midway Sports motto.

Source-faithful palette rule:
- ATTR.ASM references the global SGMD8WHT symbol.
- IMGPAL.ASM defines that palette explicitly.
- Do not try to choose a "unique" SGMD8WHT entry from the WIMP palette
  directory; SGMD8.IMG contains repeated palette-directory names.
"""
from __future__ import annotations
import argparse
import pathlib
import sys
import wimpimg

TEXT = "WE MAKE THE GAMES THAT MAKE THE INDUSTRY"
NEEDED = sorted(set(TEXT.replace(" ", "")))

# IMGPAL.ASM::SGMD8WHT, verbatim RGB555 words.
SGMD8WHT_RGB555 = [
    0x212B, 0x7F25, 0x0000, 0x7FFF,
    0x7BDE, 0x77BD, 0x739C, 0x6F7B,
    0x6B5A, 0x6318, 0x5EF7, 0x5AD6,
]

def exact_glyph_map(images):
    """Resolve osgmd8_ascii symbols using WIMP directory order.

    SGMD8.IMG contains duplicate directory names. Converting the directory to
    a Python dict was wrong because it silently replaced earlier headers with
    later duplicates. LOAD2 consumes a named image from the active WIMP file,
    so preserve source directory order and select the first exact match.
    """
    mapping = {}
    missing = []
    duplicate_counts = {}

    for ch in NEEDED:
        key = f"osgmd8_{ch.lower()}"
        matches = [im for im in images if im.name.lower() == key]
        if not matches:
            missing.append(f"osgmd8_{ch}")
            continue

        mapping[ch] = matches[0]
        if len(matches) > 1:
            duplicate_counts[ch] = len(matches)

    if missing:
        available = ", ".join(im.name for im in images)
        raise ValueError(
            "missing required original SGMD8 motto glyph(s): "
            + ", ".join(missing)
            + ". Available image names: "
            + available
        )

    if duplicate_counts:
        detail = ", ".join(
            f"{ch}={count}" for ch, count in sorted(duplicate_counts.items())
        )
        print(
            "SGMD8 duplicate WIMP names detected; "
            "using first source-directory occurrence: " + detail
        )

    return mapping

def emit_array(lines, ctype, name, vals, fmt, per):
    lines.append(f"static {ctype} {name}[] __attribute__((aligned(8))) = {{")
    for i in range(0, len(vals), per):
        lines.append("    " + ", ".join(fmt.format(v) for v in vals[i:i+per]) + ",")
    lines.append("};")
    lines.append("")

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True, type=pathlib.Path)
    ap.add_argument("--out", required=True, type=pathlib.Path)
    ns = ap.parse_args()

    data, _header, images, _palettes = wimpimg.parse_file(ns.source)
    mapping = exact_glyph_map(images)

    # Preserve the exact global source palette. Index 0 remains transparent,
    # matching the port's WIMP CI8 renderer convention.
    rgba = [
        wimpimg.rgb555_to_rgba5551(v, i)
        for i, v in enumerate(SGMD8WHT_RGB555)
    ]

    lines = [
        "/* Auto-generated from original IMG/SGMD8.IMG glyph pixels.",
        "   Palette is exact IMGPAL.ASM::SGMD8WHT (12 RGB555 words).",
        "   ATTR.ASM::rule_str:",
        "     JAM_STR osgmd8_ascii,6,0,200,225,SGMD8WHT,print_string_C2",
        "     WE MAKE THE GAMES THAT MAKE THE INDUSTRY",
        "   Glyphs are required by exact original names osgmd8_<CHAR>. */",
        '#include "wm/sports_motto.h"',
        "",
    ]
    emit_array(lines, "uint16_t", "sgmd8wht", rgba, "0x{:04X}", 12)

    ordered = [(ch, mapping[ch]) for ch in NEEDED]
    for ch, im in ordered:
        px = list(wimpimg.read_ci8(data, im))
        emit_array(lines, "const uint8_t", f"glyph_{ord(ch):02x}", px, "0x{:02X}", 24)

    lines.append("static const char glyph_chars[] = {")
    lines.append("    " + ", ".join("'" + ch + "'" for ch, _ in ordered) + ",")
    lines.append("};")
    lines.append("")
    lines.append("static const wm_source_sprite glyphs[] = {")
    for ch, im in ordered:
        tail = ", ".join(str(v) for v in im.tail_words)
        lines.append(
            f'    {{"{im.name}", "SGMD8.IMG", {im.width}, {im.height}, '
            f'{im.xani}, {im.yani}, {{{tail}}}, glyph_{ord(ch):02x}, '
            f'sgmd8wht, 12}},'
        )
    lines += [
        "};",
        "",
        'static const char motto[] = "WE MAKE THE GAMES THAT MAKE THE INDUSTRY";',
        "",
        "const wm_source_sprite *wm_sports_motto_glyph(char ascii) {",
        "    for (size_t i = 0; i < sizeof(glyph_chars)/sizeof(glyph_chars[0]); ++i)",
        "        if (glyph_chars[i] == ascii) return &glyphs[i];",
        "    return 0;",
        "}",
        "const char *wm_sports_motto_text(void) { return motto; }",
        "",
    ]

    ns.out.parent.mkdir(parents=True, exist_ok=True)
    ns.out.write_text("\n".join(lines))
    print(
        "generated exact SGMD8 motto: "
        f"{len(ordered)} distinct osgmd8_<CHAR> glyphs; "
        "palette=IMGPAL.ASM::SGMD8WHT(12)"
    )
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"sports_motto_bundle: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
