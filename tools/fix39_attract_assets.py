#!/usr/bin/env python3
from __future__ import annotations

import argparse
import importlib.util
import re
import struct
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

FONT_CONTAINERS = ("OSGEMD.IMG", "WSFNT10.IMG", "OGMD10.IMG", "TROGF7.IMG")
HINT_ART = (
    "JMSTIP", "JASMUG", "MIKTIP", "MIKMUG", "MJTTIP", "MRKMUG",
    "JOSTIP", "JSHMUG", "EUGTIP", "EUGMUG", "SHNTIP", "SHNMUG",
    "JAKTIP", "JAKMUG", "SALTIP", "SALMUG", "TONTIP", "TONMUG",
    "MUGBAK", "MUGFRNT",
    "WGSF22_0", "WGSF22_1", "WGSF22_2", "WGSF22_3", "WGSF22_4",
    "WGSF22_5", "WGSF22_6", "WGSF22_7", "WGSF22_8", "WGSF22_9",
    "SMWWF2",
)

# HSTD.ASM / STRING.ASM source symbols used by the five high-score pages.
# Font glyph spellings are taken from the exact wsf14_ascii/font18_ascii
# lookup tables; artwork names come from WHICH_NAMES/which_crouton and the
# table-entry drawing routines.
HIGH_SCORE_ART = (
    "WSF14NUM",
    "WSF14_0", "WSF14_1", "WSF14_2", "WSF14_3", "WSF14_4",
    "WSF14_5", "WSF14_6", "WSF14_7", "WSF14_8", "WSF14_9",
    "WGSF18PER",
    "WGSF18_0", "WGSF18_1", "WGSF18_2", "WGSF18_3", "WGSF18_4",
    "WGSF18_5", "WGSF18_6", "WGSF18_7", "WGSF18_8", "WGSF18_9",
    "SPEAR", "BARBUTT",
    "CRUT_BH", "CRUT_RR", "CRUT_UN", "CRUT_YK",
    "CRUT_SM", "CRUT_BM", "CRUT_DK", "CRUT_LX",
    "HART", "RAZOR", "UNDER", "YOKO", "SHAWN", "BAMBAM", "DOINK", "LEX",
)

NAMED_PALETTES = (
    "RUBYPAL", "WSF_Y_P", "WGFS_W_P", "SGMD8YEL", "SGMD8WHT",
    "GOLD", "BLUE", "WSF_W_P", "WGSF_W_P1", "DPLT_R_P",
)

@dataclass(frozen=True)
class ResolvedSprite:
    container: Path
    data: bytes
    image: object
    palette: object | None
    images: list
    source_name: str


def load_wimpimg(tool: Path):
    spec = importlib.util.spec_from_file_location("fix39_wimpimg", tool)
    if spec is None or spec.loader is None:
        raise ValueError(f"cannot import {tool}")
    mod = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = mod
    spec.loader.exec_module(mod)
    return mod


def find_casefold(root: Path, filename: str) -> Path:
    want = filename.casefold()
    for p in root.iterdir():
        if p.is_file() and p.name.casefold() == want:
            return p
    raise ValueError(f"missing source container {filename} in {root}")


def parse_number(tok: str) -> int:
    tok = tok.strip().rstrip(",")
    if not tok:
        raise ValueError("empty number")
    if tok.startswith(">"):
        return int(tok[1:], 16)
    if tok[-1:].upper() == "H":
        body = tok[:-1]
        if not body:
            raise ValueError(f"bad hex number {tok}")
        return int(body, 16)
    return int(tok, 0)


def strip_asm_comment(line: str) -> str:
    return line.split(";", 1)[0]


def parse_imgpal_named(path: Path, wanted: tuple[str, ...]) -> dict[str, list[int]]:
    lines = path.read_text(errors="replace").splitlines()
    labels: dict[str, int] = {}
    for i, raw in enumerate(lines):
        line = strip_asm_comment(raw).strip()
        m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*):\s*$", line)
        if m:
            labels[m.group(1).upper()] = i

    out: dict[str, list[int]] = {}
    for name in wanted:
        key = name.upper()
        if key not in labels:
            raise ValueError(f"IMGPAL.ASM missing palette {name}")
        i = labels[key] + 1
        tokens: list[str] = []
        while i < len(lines) and not tokens:
            line = strip_asm_comment(lines[i]).strip()
            i += 1
            m = re.match(r"^\.word\s+(.+)$", line, re.I)
            if m:
                tokens.extend(x.strip() for x in m.group(1).split(",") if x.strip())
        if not tokens:
            raise ValueError(f"palette {name} has no count")
        count = parse_number(tokens.pop(0))
        vals: list[int] = []
        for t in tokens:
            vals.append(parse_number(t) & 0x7FFF)
        while len(vals) < count and i < len(lines):
            line = strip_asm_comment(lines[i]).strip()
            i += 1
            if re.match(r"^[A-Za-z_][A-Za-z0-9_]*:\s*$", line):
                break
            m = re.match(r"^\.word\s+(.+)$", line, re.I)
            if not m:
                continue
            vals.extend(parse_number(x.strip()) & 0x7FFF
                        for x in m.group(1).split(",") if x.strip())
        if len(vals) < count:
            raise ValueError(f"palette {name} expected {count} colors, got {len(vals)}")
        out[key] = vals[:count]
    return out


def rgba5551(v: int, palette_index: int) -> int:
    """Translate Midway RGB555 to N64 RGBA5551 using DMAWNZ semantics.

    The arcade object renderer's DMAWNZ mode keys transparency from the CI8
    *pixel index*, not from the RGB555 value stored in that palette slot.
    Therefore palette index 0 is transparent even when IMGPAL.ASM gives it a
    non-zero matte color (WGFS_W_P/WSF_W_P use 0x318C), while a non-zero
    palette entry whose RGB value is 0 remains an opaque black entry.
    """
    v &= 0x7FFF
    if palette_index == 0:
        return 0
    r = (v >> 10) & 31
    g = (v >> 5) & 31
    b = v & 31
    return (r << 11) | (g << 6) | (b << 1) | 1


def c_ident(s: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", s.lower())


def parse_container(wimp, path: Path, *, allow_palette_less: bool = False):
    """Parse one WIMP container.

    The historical WWF checkout contains several *font* artist containers whose
    image directory is valid but whose palette is supplied by the game at draw
    time (for example RUBYPAL / WSF_Y_P / SGMD8YEL).  tools/wimpimg.py is
    intentionally strict and rejects those files because it expects at least one
    embedded palette directory entry.  For font containers only, recover the
    image directory directly and leave palette selection to the source draw call.
    Real artwork containers remain strict: if their embedded palette cannot be
    parsed we still stop rather than inventing colors.
    """
    try:
        data, _header, images, palettes = wimp.parse_file(path)
        return data, images, palettes
    except ValueError as exc:
        if not allow_palette_less or "no valid WIMP palette directory entries found" not in str(exc):
            raise
        data = path.read_bytes()
        header = wimp.parse_header(data)
        images = wimp.parse_images(data, header)
        return data, images, []


def source_symbol_name(data: bytes, im) -> str:
    """Recover the full WIMP directory symbol when it exceeds 8 bytes.

    The shared target reader intentionally parses only the first eight bytes.
    Several Midway font glyph symbols are longer, while XANI begins at +18.
    Match the proven select_bundle.py recovery rule: inspect at most 16 bytes,
    require a NUL terminator and printable ASCII, otherwise keep im.name.
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


def add_container_all(wimp, path: Path, resolved: dict[str, ResolvedSprite]) -> None:
    # Font WIMPs are allowed to omit embedded palette directories because the
    # original draw routines supply their palettes externally.
    data, images, palettes = parse_container(wimp, path, allow_palette_less=True)
    for im in images:
        pal = wimp.palette_for_image(im, images, palettes) if palettes else None
        source_name = source_symbol_name(data, im)
        key = source_name.upper()
        if key in resolved:
            prev = resolved[key]
            if prev.container == path:
                continue

            # Separate WIMP font containers are effectively separate linker
            # namespaces in the arcade build, so harmless local symbols such as
            # BLANK may legitimately occur in more than one file.  A flat C
            # lookup table cannot represent two identical string keys.  Keep
            # the first source spelling intact and give later cross-container
            # collisions a deterministic container-qualified alias.  The
            # sprite pixels/metadata remain source-exact; only our generated C
            # lookup key is disambiguated.
            alias = f"{path.stem}_{source_name}"
            alias_key = alias.upper()
            if alias_key in resolved:
                alias = f"{path.stem}_{source_name}_{im.directory_offset:X}"
                alias_key = alias.upper()
            while alias_key in resolved:
                alias += "_DUP"
                alias_key = alias.upper()
            source_name = alias
            key = alias_key

        resolved[key] = ResolvedSprite(path, data, im, pal, images, source_name)


def add_required_from_container(wimp, path: Path, needed: set[str],
                                resolved: dict[str, ResolvedSprite]) -> None:
    # Required font glyphs can come from palette-less historical font WIMPs;
    # artwork remains protected at emit time because only known source font
    # symbol families receive external-palette fallback.
    data, images, palettes = parse_container(wimp, path, allow_palette_less=True)
    by_name = {source_symbol_name(data, im).upper(): im for im in images}
    for key in sorted(needed & set(by_name)):
        im = by_name[key]
        pal = wimp.palette_for_image(im, images, palettes) if palettes else None
        source_name = source_symbol_name(data, im)
        if key in resolved and resolved[key].container != path:
            raise ValueError(f"duplicate required sprite {source_name}")
        resolved[key] = ResolvedSprite(path, data, im, pal, images, source_name)


def resolve_assets(img_dir: Path, wimp) -> dict[str, ResolvedSprite]:
    resolved: dict[str, ResolvedSprite] = {}
    for fn in FONT_CONTAINERS:
        add_container_all(wimp, find_casefold(img_dir, fn), resolved)

    needed = {x.upper() for x in HINT_ART + HIGH_SCORE_ART}
    # Fast-path the source files known to contain attract/logo art, then scan the
    # remaining original WIMP containers only if a symbol is still unresolved.
    for fn in ("ATTRACT.IMG", "LILLOGO.IMG", "CRUT2.IMG", "BARBUTT.IMG"):
        try:
            p = find_casefold(img_dir, fn)
        except ValueError:
            continue
        add_required_from_container(wimp, p, needed, resolved)
        needed -= set(resolved)
        if not needed:
            break

    if needed:
        for p in sorted(img_dir.iterdir(), key=lambda x: x.name.casefold()):
            if not p.is_file() or p.suffix.casefold() != ".img":
                continue
            if p.name.casefold() in {x.casefold() for x in FONT_CONTAINERS} | {"attract.img", "lillogo.img", "crut2.img", "barbutt.img"}:
                continue
            try:
                add_required_from_container(wimp, p, needed, resolved)
            except (OSError, ValueError, struct.error):
                continue
            needed -= set(resolved)
            if not needed:
                break
    if needed:
        raise ValueError("missing required attract/HSTD source images: " + ", ".join(sorted(needed)))
    return resolved


def emit_header(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text('''/* Auto-generated Fix39 V12 attract WIMP interface. */
#ifndef FIX39_ATTRACT_ASSETS_GENERATED_H
#define FIX39_ATTRACT_ASSETS_GENERATED_H
#include <stddef.h>
#include <stdint.h>
#include "wm/bret_sprites.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    const char *source_name;
    uint16_t *rgba5551;
    uint16_t color_count;
} wm_fix39_attract_palette;
const wm_source_sprite *wm_fix39_attract_sprite_find(const char *source_frame);
size_t wm_fix39_attract_sprite_count(void);
const wm_fix39_attract_palette *wm_fix39_attract_palette_find(const char *source_name);
#ifdef __cplusplus
}
#endif
#endif
''')


FONT_DRAW_PALETTE = {
    # Directly from ATTRACT.ASM / message setup usage.  These are not guessed
    # palettes from the WIMP: the arcade code supplies them at draw time.
    "OSGEMD.IMG": "RUBYPAL",
    "WSFNT10.IMG": "WSF_Y_P",
    "OGMD10.IMG": "SGMD8YEL",
    # RD7 is rendered by STRCNRMO_2 in constant-color mode; SGMD8WHT is used
    # only as a valid CI8 coverage palette for the N64 sprite bridge.  The
    # renderer still owns the source text/color behavior.
    "TROGF7.IMG": "SGMD8WHT",
}


def source_draw_palette(r: ResolvedSprite) -> str | None:
    by_container = FONT_DRAW_PALETTE.get(r.container.name.upper())
    if by_container is not None:
        return by_container
    name = r.source_name.upper()
    if name.startswith("WSF14"):
        return "WSF_W_P"
    if name.startswith("WGSF18"):
        return "WGSF_W_P1"
    return None


def emit_c(path: Path, header_name: str, resolved: dict[str, ResolvedSprite],
           named_pals: dict[str, list[int]], wimp) -> None:
    # Emit font containers wholesale plus only the specifically required art.
    font_keys = {k for k, r in resolved.items()
                 if r.container.name.casefold() in {x.casefold() for x in FONT_CONTAINERS}}
    wanted_keys = sorted(font_keys | {x.upper() for x in HINT_ART + HIGH_SCORE_ART})

    # Native WIMP palettes are keyed by container + palette directory name so
    # identically-named palettes in different files never alias.
    native: dict[tuple[str, str], tuple[str, list[int]]] = {}
    for key in wanted_keys:
        r = resolved[key]
        if r.palette is None:
            continue
        pk = (r.container.name.upper(), r.palette.name.upper())
        if pk not in native:
            vals = wimp.read_palette_words(r.data, r.palette)
            conv = [wimp.rgb555_to_rgba5551(v, i) for i, v in enumerate(vals)]
            native[pk] = (f"pal_{c_ident(r.container.stem)}_{c_ident(r.palette.name)}", conv)

    lines = [
        "/* Auto-generated from original WWF WrestleMania arcade WIMP data. */",
        f'#include "{header_name}"',
        "#include <string.h>",
        "",
    ]
    for (_pk, (ident, vals)) in sorted(native.items()):
        lines.append(f"static uint16_t {ident}[] __attribute__((aligned(8))) = {{")
        for i in range(0, len(vals), 12):
            lines.append("    " + ", ".join(f"0x{v:04X}" for v in vals[i:i+12]) + ",")
        lines += ["};", ""]

    for name in NAMED_PALETTES:
        vals = [rgba5551(v, i) for i, v in enumerate(named_pals[name])]
        ident = f"srcpal_{c_ident(name)}"
        lines.append(f"static uint16_t {ident}[] __attribute__((aligned(8))) = {{")
        for i in range(0, len(vals), 12):
            lines.append("    " + ", ".join(f"0x{v:04X}" for v in vals[i:i+12]) + ",")
        lines += ["};", ""]

    for key in wanted_keys:
        r = resolved[key]
        px = wimp.read_ci8(r.data, r.image)
        ident = f"px_{c_ident(r.container.stem)}_{c_ident(r.source_name)}_{r.image.directory_offset:x}"
        lines.append(f"static const uint8_t {ident}[] __attribute__((aligned(8))) = {{")
        for i in range(0, len(px), 24):
            lines.append("    " + ", ".join(f"0x{v:02X}" for v in px[i:i+24]) + ",")
        lines += ["};", ""]

    lines.append("static const wm_source_sprite sprites[] = {")
    for key in wanted_keys:
        r = resolved[key]
        if r.palette is not None:
            pident = native[(r.container.name.upper(), r.palette.name.upper())][0]
            pcount = r.palette.color_count
        else:
            fallback = source_draw_palette(r)
            if fallback is None or fallback not in named_pals:
                raise ValueError(f"palette-less non-font source sprite {r.source_name} in {r.container.name}")
            pident = f"srcpal_{c_ident(fallback)}"
            pcount = len(named_pals[fallback])
        pxident = f"px_{c_ident(r.container.stem)}_{c_ident(r.source_name)}_{r.image.directory_offset:x}"
        tail = ", ".join(str(v) for v in r.image.tail_words)
        lines.append(
            f'    {{"{r.source_name}", "{r.container.name}", {r.image.width}, {r.image.height}, '
            f'{r.image.xani}, {r.image.yani}, {{{tail}}}, {pxident}, {pident}, {pcount}}},'
        )
    lines += ["};", ""]

    # V12f hardware hardening: do not scan the pointer-bearing source_frame
    # member of the large generated wm_source_sprite table.  The V12e second
    # attract-loop exception occurred inside this lookup while resolving hint
    # art.  Keep a second, tiny index whose names are stored inline, so both
    # sides of the runtime name comparison are backed by directly-addressable
    # character arrays.  The returned sprite, pixels, palette and WIMP metadata
    # remain exactly the same generated source objects.
    lookup_width = max(len(resolved[key].source_name) for key in wanted_keys) + 1
    lines += [
        "typedef struct {",
        f"    char source_frame[{lookup_width}];",
        "    size_t sprite_index;",
        "} wm_fix39_attract_sprite_lookup;",
        "static const wm_fix39_attract_sprite_lookup sprite_lookup[] = {",
    ]
    for index, key in enumerate(wanted_keys):
        lines.append(f'    {{"{resolved[key].source_name}", {index}u}},')
    lines += ["};", ""]

    lines.append("static const wm_fix39_attract_palette source_palettes[] = {")
    for name in NAMED_PALETTES:
        lines.append(f'    {{"{name}", srcpal_{c_ident(name)}, {len(named_pals[name])}}},')
    lines += ["};", ""]
    lines += [
        "static int fix39_source_name_equal(const char *a, const char *b) {",
        "    if (!a || !b) return 0;",
        "    while (*a && *b) {",
        "        unsigned char ca = (unsigned char)*a++;",
        "        unsigned char cb = (unsigned char)*b++;",
        "        if (ca >= 'a' && ca <= 'z') ca = (unsigned char)(ca - ('a' - 'A'));",
        "        if (cb >= 'a' && cb <= 'z') cb = (unsigned char)(cb - ('a' - 'A'));",
        "        if (ca != cb) return 0;",
        "    }",
        "    return *a == *b;",
        "}", "",
        "const wm_source_sprite *wm_fix39_attract_sprite_find(const char *source_frame) {",
        "    if (!source_frame) return 0;",
        "    for (size_t i = 0; i < sizeof(sprite_lookup)/sizeof(sprite_lookup[0]); ++i)",
        "        if (fix39_source_name_equal(sprite_lookup[i].source_frame, source_frame))",
        "            return &sprites[sprite_lookup[i].sprite_index];",
        "    return 0;",
        "}", "",
        "size_t wm_fix39_attract_sprite_count(void) {",
        "    return sizeof(sprites)/sizeof(sprites[0]);",
        "}", "",
        "const wm_fix39_attract_palette *wm_fix39_attract_palette_find(const char *source_name) {",
        "    if (!source_name) return 0;",
        "    for (size_t i = 0; i < sizeof(source_palettes)/sizeof(source_palettes[0]); ++i)",
        "        if (strcmp(source_palettes[i].source_name, source_name) == 0) return &source_palettes[i];",
        "    return 0;",
        "}", "",
    ]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines))


def self_test() -> None:
    assert parse_number("07FE0H") == 0x7FE0
    assert parse_number(">1111") == 0x1111
    # DMAWNZ keys on CI8 index zero, not on the palette RGB value.  This is
    # the hardware regression behind the gray matte around TIP-name art.
    assert rgba5551(0x318C, 0) == 0
    assert rgba5551(0x0000, 1) == 0x0001
    assert rgba5551(0x7FFF, 1) == 0xFFFF
    class _DummyImage:
        directory_offset = 0
        name = "FONT7per"
    assert source_symbol_name(b"FONT7period\0" + b"\0" * 8, _DummyImage()) == "FONT7period"
    # Do not hard-code /tmp: Android/Termux may expose a non-writable system
    # /tmp.  tempfile honors TMPDIR and falls back to a writable app-private
    # location.
    with tempfile.TemporaryDirectory(prefix="fix39_imgpal_selftest_") as td:
        sample = Path(td) / "imgpal_selftest.asm"
        sample.write_text("RUBYPAL:\n .word 3\n .word 00H, 07FFFH, 01234H\nWSF_Y_P:\n .word 1\n .word 07FE0H\nWGFS_W_P:\n .word 1\n .word 0318CH\nSGMD8YEL:\n .word 1\n .word 07F25H\n")
        # Include the V12c RD7 coverage palette in the synthetic table too.
        sample.write_text(sample.read_text() + "SGMD8WHT:\n .word 3\n .word 00H, 07FFFH, 05AD6H\n")
        sample.write_text(sample.read_text() +
            "GOLD:\n .word 1\n .word 07FE0H\n"
            "BLUE:\n .word 1\n .word 001FH\n"
            "WSF_W_P:\n .word 1\n .word 07FFFH\n"
            "WGSF_W_P1:\n .word 1\n .word 07FFFH\n"
            "DPLT_R_P:\n .word 1\n .word 07C00H\n")
        got = parse_imgpal_named(sample, NAMED_PALETTES)
        assert got["RUBYPAL"] == [0, 0x7FFF, 0x1234]
        assert got["WSF_Y_P"] == [0x7FE0]
        assert got["SGMD8WHT"] == [0, 0x7FFF, 0x5AD6]

        # Regression for the exact Termux failure: strict wimpimg rejects a
        # palette-less font, but the V12c wrapper must still recover images.
        class _FakeHeader:
            pass
        class _FakeWimp:
            def parse_file(self, _path):
                raise ValueError("no valid WIMP palette directory entries found")
            def parse_header(self, _data):
                return _FakeHeader()
            def parse_images(self, _data, _header):
                return [object()]
        raw = Path(td) / "font.img"
        raw.write_bytes(b"x" * 64)
        data, images, palettes = parse_container(_FakeWimp(), raw, allow_palette_less=True)
        assert len(data) == 64 and len(images) == 1 and palettes == []

        # Regression for the real historical checkout: local font symbols such
        # as BLANK can appear in more than one WIMP container.  Cross-container
        # duplicates must be namespaced instead of aborting generation.
        class _Img:
            def __init__(self, off):
                self.directory_offset = off
                self.name = "BLANK"
        class _DupWimp:
            def parse_file(self, path):
                return (b"BLANK\0" + b"\0" * 64, object(), [_Img(0)], [])
            def palette_for_image(self, *_args):
                return None
        dups = {}
        add_container_all(_DupWimp(), Path(td) / "OSGEMD.IMG", dups)
        add_container_all(_DupWimp(), Path(td) / "OGMD10.IMG", dups)
        assert "BLANK" in dups
        assert "OGMD10_BLANK" in dups
        assert dups["OGMD10_BLANK"].source_name == "OGMD10_BLANK"

        # V12f regression: emitted runtime name lookup must use inline names,
        # not dereference sprites[i].source_frame while scanning.
        class _EmitImage:
            def __init__(self, off):
                self.directory_offset = off
                self.width = 1
                self.height = 1
                self.xani = 0
                self.yani = 0
                self.tail_words = (0,) * 9
        class _EmitPalette:
            name = "P0"
            color_count = 1
        class _EmitWimp:
            def read_palette_words(self, _data, _palette):
                return [0]
            def rgb555_to_rgba5551(self, _v, _i):
                return 0
            def read_ci8(self, _data, _image):
                return bytes([0])
        fake_container = Path(td) / "ATTRACT.IMG"
        fake_container.write_bytes(b"x")
        fake_pal = _EmitPalette()
        fake_resolved = {
            name.upper(): ResolvedSprite(
                fake_container, b"x", _EmitImage(i), fake_pal, [], name
            )
            for i, name in enumerate(HINT_ART + HIGH_SCORE_ART)
        }
        fake_named = {name: [0] for name in NAMED_PALETTES}
        emitted = Path(td) / "attract_assets.c"
        emit_c(emitted, "fix39_attract_assets_generated.h",
               fake_resolved, fake_named, _EmitWimp())
        emitted_text = emitted.read_text()
        assert "wm_fix39_attract_sprite_lookup" in emitted_text
        assert "sprite_lookup[i].source_frame" in emitted_text
        assert "sprites[i].source_frame, source_frame" not in emitted_text
        assert 'return &sprites[sprite_lookup[i].sprite_index];' in emitted_text
    print("Fix39 V12i attract/HSTD asset parser self-test: PASS")


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate source-exact Fix39 ATTRACT WIMP assets")
    ap.add_argument("--img-dir", type=Path)
    ap.add_argument("--imgpal", type=Path)
    ap.add_argument("--wimpimg", type=Path, default=Path("tools/wimpimg.py"))
    ap.add_argument("--out-c", type=Path)
    ap.add_argument("--out-h", type=Path)
    ap.add_argument("--self-test", action="store_true")
    ns = ap.parse_args()
    if ns.self_test:
        self_test()
        return 0
    for name in ("img_dir", "imgpal", "out_c", "out_h"):
        if getattr(ns, name) is None:
            ap.error(f"--{name.replace('_', '-')} is required unless --self-test is used")
    wimp = load_wimpimg(ns.wimpimg)
    resolved = resolve_assets(ns.img_dir, wimp)
    named = parse_imgpal_named(ns.imgpal, NAMED_PALETTES)
    emit_header(ns.out_h)
    emit_c(ns.out_c, ns.out_h.name, resolved, named, wimp)
    print(f"generated {ns.out_c} ({len(resolved)} resolved source sprites)")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, struct.error) as exc:
        print(f"fix39_attract_assets: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
