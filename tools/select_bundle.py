#!/usr/bin/env python3
"""Build exact SELECT.ASM + PROGRESS.ASM WIMP artwork and live palettes.

The championship/title-choice pass also exports the original OSGEMD prompt
glyphs and IMGPAL.ASM palette symbols used by ask_belt_question/hilight.


No replacement PNGs or hand-redrawn assets are accepted.
"""
from __future__ import annotations
import argparse
import pathlib
import sys

from wimpimg import (
    parse_file, palette_for_image, read_palette_words, read_ci8,
    rgb555_to_rgba5551, c_ident,
)

CROUTONS = ["CRUT_DK","CRUT_RR","CRUT_UN","CRUT_YK","CRUT_SM","CRUT_BM","CRUT_BH","CRUT_LX"]
CURSOR = ["CRUTPLT_B","CRUTHI_B"]
NAMES = ["NAM_BRT","NAM_RZR","NAM_UND","NAM_YOK","NAM_SHN2","NAM_BAM2","NAM_DNK","NAM_LEX"]
MUG_PREFIXES = ["BH","RR","UN","YK","SM","BM","DK","LX"]
MUGS = [f"{p}MUG_{c}" for p in MUG_PREFIXES for c in "ABCDEFGH"]
DIGITS = [f"FNT9_{n}" for n in range(10)]
# SELECT.ASM uses the font9 family for its live select/buy-in messages.  Pull
# the alphabet from the original WIMP containers as source glyphs; do not use
# a libdragon/debug font for arcade UI text.
FONT9_ALPHA = [f"FNT9_{c}" for c in "ABCDEFGHIJKLMNOPQRSTUVWXYZ"]
# PROGRESS.ASM belt_prompt_setup uses osgemd_ascii for "SELECT YOUR TITLE:".
OSGEMD_ALPHA = [f"OSGEMD_{c}" for c in "ABCDEFGHIJKLMNOPQRSTUVWXYZ"]
OSGEMD_PUNCT = ["OSGEMD_COL"]
# Live OPAL assignments / palette-cycle tables used by ask_belt_question.
SOURCE_PALETTES = [
    "DPLT_P2P", "DPLT_W_P", "WSF_Y_P", "WSF_W_P",
    "CHGLWT_P", *[f"CHGLWT{i}P" for i in range(1, 10)],
    "CHGLWB_P", *[f"CHGLWB{i}P" for i in range(1, 10)],
]
# PROGRESS.ASM::DO_LADDER_BITS live palette overrides. Most are global
# IMGPAL symbols; SWWFBW_P and DPLT_P_P are local tables at the bottom of
# PROGRESS.ASM and therefore must be parsed from that file.
PROGRESS_PALETTES = [
    "DPLT_R_P", "BARB_P", "WINFONT", "SWWFBB_P",
    "SWWFBW_P", "DPLT_P_P", "FLASHP_P", "FLASHR_P",
]
BUYIN = ["WF_INSERT", "WF_START"]
# PROGRESS.ASM::ask_belt_question live one-player title-selection art.
BELT_CHOICE = [
    "MVEBAR_R", "SHADOW01",
    "CHOGLOT_A", "CHOGLOT_B", "CHSHDT_A", "CHSHDT_B",
    "CHOGLOB_A", "CHOGLOB_B", "CHSHDB_A", "CHSHDB_B",
    "CHOICBK", "INTER", "WORLD",
]
# PROGRESS.ASM::DO_LADDER_BITS live source images required by Fix20.  The
# extra LOGO_IMAGE_TABLE images remain optional until SET_IMAGE_AND_FLASH is
# ported as its own process.
PROGRESS_UI = [
    "STATBAR", "BLUESH", "TXTBAR1", "WINS_IMG", "MATCH_IMG", "TXTPCE",
    "RCHAMP", "SWWFBLT", "LBAR_GENB",
    "FLASH01", "FLASH02", "FLASH03", "FLASH04", "FLASH05",
]
PROGRESS_UI_OPTIONAL = [
    "HRT3", "RZR3", "UND3", "YOK3", "SHN3", "BAM3", "DNK3", "LEX3", "WWFCHAL",
]
# Legacy stand-image aliases are kept optional; Fix20 wrestler visuals are
# generated instead from the exact action animation frame dependencies.
OPTIONAL = ["OSGEMD_SPC"] + PROGRESS_UI_OPTIONAL + [
    "H4ST4A02", "RAZOR_STAND", "TAKER_STAND", "YOKO_STAND", "SHAWN_STAND",
    "BAM_STAND", "DOINK_STAND", "LEX_STAND",
]
REQUIRED = CROUTONS + CURSOR + NAMES + MUGS + DIGITS + FONT9_ALPHA + OSGEMD_ALPHA + OSGEMD_PUNCT + BUYIN + BELT_CHOICE + PROGRESS_UI

def source_symbol_name(data: bytes, im) -> str:
    """Recover the full source symbol stored in the WIMP directory.

    The shared wimpimg.py reader intentionally used an 8-byte name slice while
    reverse-engineering the first frontend assets.  SELECT.ASM exposes a case
    that proves the directory symbol is longer: CRUTPLT_B / CRUTPLT_R share the
    same first eight bytes ("CRUTPLT").  XANI starts at +18, so the source-name
    field has room for the longer symbol before the animation metadata.

    For select extraction only, read a conservative 16-byte symbol, require a
    NUL terminator and printable ASCII, and otherwise fall back to the proven
    8-byte parser name.  This avoids changing the already-working global parser
    underneath title/DCS/sports assets.
    """
    off = im.directory_offset
    raw = data[off:off + 16]
    nul = raw.find(b"\0")
    if nul <= 0:
        return im.name
    raw = raw[:nul]
    try:
        text = raw.decode("ascii")
    except UnicodeDecodeError:
        return im.name
    if not text or any(ord(c) < 0x20 or ord(c) > 0x7E for c in text):
        return im.name
    return text

def asm_num(token: str) -> int:
    token = token.strip().rstrip(',')
    if token.lower().endswith('h'):
        return int(token[:-1], 16)
    if token.lower().startswith('0x'):
        return int(token, 16)
    return int(token, 10)


def asm_code_lines(text: str):
    for raw in text.splitlines():
        yield raw.split(';', 1)[0].strip()


def parse_source_palette(text: str, label: str) -> list[int]:
    """Parse one IMGPAL.ASM `label: .word size; .word colors...` block."""
    import re
    lines = list(asm_code_lines(text))
    rx = re.compile(rf'^{re.escape(label)}\s*:?\s*$', re.I)
    start = None
    for i, line in enumerate(lines):
        if rx.match(line):
            start = i + 1
            break
    if start is None:
        raise ValueError(f'IMGPAL palette not found: {label}')

    def word_values(line: str):
        m = re.match(r'(?i)^\.word\s+(.+)$', line)
        if not m:
            return None
        return [asm_num(x) for x in m.group(1).split(',') if x.strip()]

    i = start
    while i < len(lines) and not lines[i]:
        i += 1
    size_vals = word_values(lines[i]) if i < len(lines) else None
    if not size_vals or len(size_vals) != 1:
        raise ValueError(f'{label}: missing palette size')
    size = size_vals[0]
    i += 1
    vals: list[int] = []
    label_rx = re.compile(r'^[A-Za-z_.$][A-Za-z0-9_.$]*\s*:\s*$')
    while i < len(lines) and len(vals) < size:
        line = lines[i]
        i += 1
        if not line:
            continue
        if label_rx.match(line):
            break
        w = word_values(line)
        if w:
            vals.extend(w)
    if len(vals) < size:
        raise ValueError(f'{label}: expected {size} colors, parsed {len(vals)}')
    return vals[:size]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--img-dir", required=True, type=pathlib.Path)
    ap.add_argument("--imgpal", required=True, type=pathlib.Path)
    ap.add_argument("--progress", type=pathlib.Path, help="PROGRESS.ASM for local progression palettes")
    ap.add_argument("--out", required=True, type=pathlib.Path)
    ns = ap.parse_args()
    imgpal_text = ns.imgpal.read_text(errors="replace")
    progress_text = ns.progress.read_text(errors="replace") if ns.progress else ""
    named_source_palettes = {name: parse_source_palette(imgpal_text, name) for name in SOURCE_PALETTES}
    for name in PROGRESS_PALETTES:
        try:
            named_source_palettes[name] = parse_source_palette(imgpal_text, name)
        except ValueError:
            if not progress_text:
                raise ValueError(f"progress palette {name} requires --progress PROGRESS.ASM")
            named_source_palettes[name] = parse_source_palette(progress_text, name)
    all_source_palettes = SOURCE_PALETTES + PROGRESS_PALETTES

    wanted = {n.upper() for n in REQUIRED + OPTIONAL}
    found = {}
    scanned = 0
    rejected = 0

    for path in sorted(ns.img_dir.glob("*.IMG")):
        try:
            data, header, images, palettes = parse_file(path)
        except Exception:
            rejected += 1
            continue
        scanned += 1
        for im in images:
            source_name = source_symbol_name(data, im)
            key = source_name.upper()
            if key in wanted and key not in found:
                pal = palette_for_image(im, images, palettes)
                found[key] = (path, data, images, palettes, im, pal, source_name)

    missing = [n for n in REQUIRED if n.upper() not in found]
    if missing:
        raise ValueError(
            "required SELECT.ASM WIMP images not found in original IMG set: "
            + ", ".join(missing)
        )

    emitted = REQUIRED + [n for n in OPTIONAL if n.upper() in found]

    lines = [
        "/* Auto-generated from original WWF WIMP .IMG containers for SELECT.ASM. */",
        '#include "wm/select_sprites.h"',
        "#include <string.h>",
        "",
    ]

    palette_names = {}
    for req in emitted:
        path, data, images, palettes, im, pal, source_name = found[req.upper()]
        pkey = (path.name.upper(), pal.directory_offset)
        if pkey in palette_names:
            continue
        ident = c_ident(path.stem + "_" + pal.name + "_" + f"{pal.directory_offset:x}")
        palette_names[pkey] = ident
        vals = [rgb555_to_rgba5551(v, i) for i, v in enumerate(read_palette_words(data, pal))]
        lines.append(f"static uint16_t pal_{ident}[] __attribute__((aligned(8))) = {{")
        for i in range(0, len(vals), 12):
            lines.append("    " + ", ".join(f"0x{v:04X}" for v in vals[i:i+12]) + ",")
        lines += ["};", ""]

    sprite_idents = {}
    for req in emitted:
        path, data, images, palettes, im, pal, source_name = found[req.upper()]
        ident = c_ident(path.stem + "_" + source_name + "_" + f"{im.directory_offset:x}")
        sprite_idents[req.upper()] = ident
        px = read_ci8(data, im)
        lines.append(f"static const uint8_t px_{ident}[] __attribute__((aligned(8))) = {{")
        for i in range(0, len(px), 24):
            lines.append("    " + ", ".join(f"0x{v:02X}" for v in px[i:i+24]) + ",")
        lines += ["};", ""]

    # SELECT.ASM initial one-player challenger text uses FNT9YEL_P.
    # Source palette from IMGPAL.ASM: .word 02D6Bh,0000h,07FE0h.
    yel_vals = [rgb555_to_rgba5551(v, i) for i, v in enumerate((0x2D6B, 0x0000, 0x7FE0))]
    lines.append("static uint16_t pal_FNT9YEL_P[] __attribute__((aligned(8))) = {")
    lines.append("    " + ", ".join(f"0x{v:04X}" for v in yel_vals) + ",")
    lines += ["};", ""]

    source_palette_idents = {}
    for pname in all_source_palettes:
        ident = c_ident(pname)
        source_palette_idents[pname] = ident
        vals = [rgb555_to_rgba5551(v, i) for i, v in enumerate(named_source_palettes[pname])]
        lines.append(f"static uint16_t srcpal_{ident}[] __attribute__((aligned(8))) = {{")
        for i in range(0, len(vals), 12):
            lines.append("    " + ", ".join(f"0x{v:04X}" for v in vals[i:i+12]) + ",")
        lines += ["};", ""]

    lines.append("static const wm_source_sprite sprites[] = {")
    for req in emitted:
        path, data, images, palettes, im, pal, source_name = found[req.upper()]
        pkey = (path.name.upper(), pal.directory_offset)
        ident = sprite_idents[req.upper()]
        pident = palette_names[pkey]
        tail = ", ".join(str(v) for v in im.tail_words)
        lines.append(
            f'    {{"{source_name}", "{path.name}", {im.width}, {im.height}, '
            f'{im.xani}, {im.yani}, {{{tail}}}, px_{ident}, pal_{pident}, {pal.color_count}}},'
        )

    # Same original FNT9 pixels under the exact FNT9YEL_P source palette.
    for req in DIGITS + FONT9_ALPHA:
        path, data, images, palettes, im, pal, source_name = found[req.upper()]
        ident = sprite_idents[req.upper()]
        tail = ", ".join(str(v) for v in im.tail_words)
        yname = "Y" + source_name
        lines.append(
            f'    {{"{yname}", "{path.name}", {im.width}, {im.height}, '
            f'{im.xani}, {im.yani}, {{{tail}}}, px_{ident}, pal_FNT9YEL_P, 3}},'
        )
    lines += [
        "};",
        "",
        "static const wm_select_palette source_palettes[] = {",
    ]
    for pname in all_source_palettes:
        ident = source_palette_idents[pname]
        lines.append(f'    {{"{pname}", srcpal_{ident}, {len(named_source_palettes[pname])}}},')
    lines += [
        "};",
        "",
        "const wm_select_palette *wm_select_palette_find(const char *source_name) {",
        "    if (!source_name) return 0;",
        "    for (size_t i=0;i<sizeof(source_palettes)/sizeof(source_palettes[0]);++i)",
        "        if (!strcmp(source_palettes[i].source_name, source_name)) return &source_palettes[i];",
        "    return 0;",
        "}",
        "",
        "const wm_source_sprite *wm_select_sprite_find(const char *source_frame) {",
        "    if (!source_frame) return 0;",
        "    for (size_t i=0;i<sizeof(sprites)/sizeof(sprites[0]);++i)",
        "        if (!strcmp(sprites[i].source_frame, source_frame)) return &sprites[i];",
        "    return 0;",
        "}",
        "",
        "const wm_source_sprite *wm_select_sprite_at(size_t index) {",
        "    return index < sizeof(sprites)/sizeof(sprites[0]) ? &sprites[index] : 0;",
        "}",
        "",
        "size_t wm_select_sprite_count(void) {",
        "    return sizeof(sprites)/sizeof(sprites[0]);",
        "}",
        "",
    ]
    ns.out.parent.mkdir(parents=True, exist_ok=True)
    ns.out.write_text("\n".join(lines))
    print(
        f"select/pregame foreground: {len(emitted)} exact images + {len(all_source_palettes)} live palettes ({len(REQUIRED)} required) from {scanned} WIMP containers "
        f"({rejected} non-WIMP/unsupported skipped) -> {ns.out}"
    )
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"select_bundle: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
