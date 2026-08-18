#!/usr/bin/env python3
"""Convert original Midway BLIMP .BDD background data into portable C.

BIGWWF.BDB supplies the source block placement/palette metadata, BIGWWF.BDD
supplies the raw CI8 block pixels and RGB555 palettes, and BGNDTBL.ASM supplies
the compiled BMOD records used by the game.  The converter keeps those pieces
separate so the N64 backend can execute the original BMOD composition instead
of flattening/recreating the screen.
"""
from __future__ import annotations

import argparse
import pathlib
import struct
import sys
from dataclasses import dataclass

TOOLS_DIR = pathlib.Path(__file__).resolve().parent
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))
import bmod_source


@dataclass(frozen=True)
class BddImage:
    source_id: int
    width: int
    height: int
    source_flag: int
    pixels: bytes


@dataclass(frozen=True)
class BddPalette:
    name: str
    words_rgb555: tuple[int, ...]


@dataclass(frozen=True)
class BdbBlock:
    source_control: int
    source_x: int
    source_y: int
    source_id: int
    palette: int


@dataclass(frozen=True)
class BdbRegion:
    source_name: str
    canvas_width: int
    canvas_height: int
    region_name: str
    region_bounds: tuple[int, int, int, int]
    blocks: tuple[BdbBlock, ...]


def _read_ascii_line(data: bytes, pos: int, what: str) -> tuple[str, int]:
    try:
        end = data.index(b"\n", pos)
    except ValueError as exc:
        raise ValueError(f"BDD: unterminated {what} at 0x{pos:X}") from exc
    try:
        line = data[pos:end].decode("ascii")
    except UnicodeDecodeError as exc:
        raise ValueError(f"BDD: non-ASCII {what} at 0x{pos:X}") from exc
    return line.rstrip("\r"), end + 1


def parse_bdd(path: pathlib.Path) -> tuple[list[BddImage], list[BddPalette]]:
    data = path.read_bytes()
    count_line, pos = _read_ascii_line(data, 0, "image count")
    try:
        image_count = int(count_line, 10)
    except ValueError as exc:
        raise ValueError(f"{path.name}: invalid image count {count_line!r}") from exc
    if image_count <= 0:
        raise ValueError(f"{path.name}: image count must be positive")

    images: list[BddImage] = []
    for index in range(image_count):
        line, pos = _read_ascii_line(data, pos, f"image header {index}")
        parts = line.split()
        if len(parts) != 4:
            raise ValueError(f"{path.name}: malformed image header {index}: {line!r}")
        try:
            source_id = int(parts[0], 16)
            width = int(parts[1], 10)
            height = int(parts[2], 10)
            source_flag = int(parts[3], 10)
        except ValueError as exc:
            raise ValueError(f"{path.name}: malformed image header {index}: {line!r}") from exc
        if width <= 0 or height <= 0:
            raise ValueError(f"{path.name}: invalid image size {width}x{height} at {index}")
        size = width * height
        end = pos + size
        if end > len(data):
            raise ValueError(
                f"{path.name}: image {index} payload overruns file: need {size} bytes"
            )
        images.append(BddImage(source_id, width, height, source_flag, data[pos:end]))
        pos = end

    palettes: list[BddPalette] = []
    while pos < len(data):
        line, pos = _read_ascii_line(data, pos, f"palette header {len(palettes)}")
        if not line:
            continue
        parts = line.split()
        if len(parts) != 2:
            raise ValueError(f"{path.name}: malformed palette header: {line!r}")
        name = parts[0]
        try:
            count = int(parts[1], 10)
        except ValueError as exc:
            raise ValueError(f"{path.name}: malformed palette count: {line!r}") from exc
        if count <= 0 or count > 256:
            raise ValueError(f"{path.name}: invalid palette size {count} for {name}")
        end = pos + count * 2
        if end > len(data):
            raise ValueError(f"{path.name}: palette {name} overruns file")
        words = struct.unpack_from(f"<{count}H", data, pos)
        palettes.append(BddPalette(name, tuple(words)))
        pos = end

    if not palettes:
        raise ValueError(f"{path.name}: no palettes found after {image_count} images")
    return images, palettes


def parse_bdb(path: pathlib.Path, region_name: str) -> BdbRegion:
    lines = [ln.strip() for ln in path.read_text(errors="replace").splitlines() if ln.strip()]
    if len(lines) < 3:
        raise ValueError(f"{path.name}: BDB too short")
    hp = lines[0].split()
    if len(hp) < 7:
        raise ValueError(f"{path.name}: malformed BDB header")
    try:
        source_name = hp[0]
        canvas_w = int(hp[1], 10)
        canvas_h = int(hp[2], 10)
        block_count = int(hp[-1], 10)
    except ValueError as exc:
        raise ValueError(f"{path.name}: malformed BDB header") from exc

    region_index = -1
    bounds: tuple[int, int, int, int] | None = None
    for i, line in enumerate(lines[1:], start=1):
        p = line.split()
        if p and p[0].upper() == region_name.upper():
            if len(p) != 5:
                raise ValueError(f"{path.name}: malformed {region_name} region header")
            try:
                bounds = tuple(int(v, 10) for v in p[1:5])  # type: ignore[assignment]
            except ValueError as exc:
                raise ValueError(f"{path.name}: malformed {region_name} bounds") from exc
            region_index = i
            break
    if region_index < 0 or bounds is None:
        raise ValueError(f"{path.name}: region {region_name} not found")

    raw = lines[region_index + 1:region_index + 1 + block_count]
    if len(raw) != block_count:
        raise ValueError(
            f"{path.name}: {region_name} has {len(raw)} block rows; expected {block_count}"
        )
    blocks: list[BdbBlock] = []
    for i, line in enumerate(raw):
        p = line.split()
        if len(p) != 5:
            raise ValueError(f"{path.name}: malformed block {i}: {line!r}")
        try:
            blocks.append(BdbBlock(
                source_control=int(p[0], 16),
                source_x=int(p[1], 10),
                source_y=int(p[2], 10),
                source_id=int(p[3], 16),
                palette=int(p[4], 16),
            ))
        except ValueError as exc:
            raise ValueError(f"{path.name}: malformed block {i}: {line!r}") from exc

    return BdbRegion(source_name, canvas_w, canvas_h, region_name, bounds, tuple(blocks))


def _s16(v: int) -> int:
    return v - 0x10000 if v & 0x8000 else v


def validate_sources(images: list[BddImage], palettes: list[BddPalette], region: BdbRegion,
                     bgndtbl: pathlib.Path, module_name: str) -> dict:
    if len(images) != len(region.blocks):
        raise ValueError(
            f"image/block count mismatch: BDD={len(images)} BDB={len(region.blocks)}"
        )

    min_x = min(b.source_x for b in region.blocks)
    min_y = min(b.source_y for b in region.blocks)
    max_x = max(b.source_x + im.width for b, im in zip(region.blocks, images))
    max_y = max(b.source_y + im.height for b, im in zip(region.blocks, images))
    footprint = (max_x - min_x, max_y - min_y)

    text = bgndtbl.read_text(errors="replace")
    module = bmod_source.parse_module(text, module_name)
    if module["count"] != len(images):
        raise ValueError(
            f"{module_name}: BMOD count {module['count']} != BDD count {len(images)}"
        )
    if (module["width"], module["height"]) != footprint:
        raise ValueError(
            f"{module_name}: BMOD size {module['width']}x{module['height']} "
            f"!= BDB/BDD footprint {footprint[0]}x{footprint[1]}"
        )

    words = module["words"]
    for i, (desc, im) in enumerate(zip(region.blocks, images)):
        if desc.source_id != im.source_id:
            raise ValueError(
                f"block {i}: BDB source id {desc.source_id:X} != BDD id {im.source_id:X}"
            )
        if desc.palette >= len(palettes):
            raise ValueError(
                f"block {i}: palette {desc.palette} outside BDD palette table ({len(palettes)})"
            )
        if im.pixels and max(im.pixels) >= len(palettes[desc.palette].words_rgb555):
            raise ValueError(
                f"block {i}: CI8 index {max(im.pixels)} exceeds palette "
                f"{desc.palette} size {len(palettes[desc.palette].words_rgb555)}"
            )

        a, bx, by, hdr = words[i * 4:i * 4 + 4]
        bpal = (a & 0x000F) | ((hdr >> 8) & 0x00F0)
        bz = (a >> 8) & 0xFF
        hidx = hdr & 0x0FFF
        expected_x = desc.source_x - min_x
        expected_y = desc.source_y - min_y
        if _s16(bx) != expected_x or _s16(by) != expected_y:
            raise ValueError(
                f"block {i}: BMOD pos {_s16(bx)},{_s16(by)} != "
                f"BDB-relative {expected_x},{expected_y}"
            )
        if hidx != i:
            raise ValueError(f"block {i}: BMOD header index {hidx} != BDD record index {i}")
        if bpal != desc.palette:
            raise ValueError(f"block {i}: BMOD palette {bpal} != BDB palette {desc.palette}")
        if bz != ((desc.source_control >> 8) & 0xFF):
            raise ValueError(
                f"block {i}: BMOD Z {bz:X} != BDB Z {(desc.source_control >> 8) & 0xFF:X}"
            )

    return {
        "module": module,
        "min_x": min_x,
        "min_y": min_y,
        "footprint": footprint,
    }


def rgb555_to_rgba5551(word: int, alpha: int = 1) -> int:
    r = (word >> 10) & 31
    g = (word >> 5) & 31
    b = word & 31
    return (r << 11) | (g << 6) | (b << 1) | (alpha & 1)


def _ident(text: str) -> str:
    out = "".join(ch.lower() if ch.isalnum() else "_" for ch in text)
    if not out or out[0].isdigit():
        out = "p_" + out
    return out


def emit(out: pathlib.Path, images: list[BddImage], palettes: list[BddPalette],
         region: BdbRegion, validation: dict, source_bdd: pathlib.Path,
         source_bdb: pathlib.Path) -> None:
    lines = [
        "/* Auto-generated from original Midway BLIMP BDD/BDB background data. */",
        '#include "wm/title_screen.h"',
        "",
    ]

    for i, im in enumerate(images):
        lines.append(f"static const uint8_t title_px_{i:02d}[] __attribute__((aligned(8))) = {{")
        for j in range(0, len(im.pixels), 24):
            lines.append("    " + ", ".join(f"0x{x:02X}" for x in im.pixels[j:j + 24]) + ",")
        lines += ["};", ""]

    for i, pal in enumerate(palettes):
        name = _ident(pal.name)
        opaque = [rgb555_to_rgba5551(v, 1) for v in pal.words_rgb555]
        keyed = [rgb555_to_rgba5551(v, 0 if j == 0 else 1)
                 for j, v in enumerate(pal.words_rgb555)]
        lines.append(f"static uint16_t title_pal_{i:02d}_{name}_opaque[] __attribute__((aligned(8))) = {{")
        for j in range(0, len(opaque), 12):
            lines.append("    " + ", ".join(f"0x{x:04X}" for x in opaque[j:j + 12]) + ",")
        lines += ["};", ""]
        lines.append(f"static uint16_t title_pal_{i:02d}_{name}_keyed[] __attribute__((aligned(8))) = {{")
        for j in range(0, len(keyed), 12):
            lines.append("    " + ", ".join(f"0x{x:04X}" for x in keyed[j:j + 12]) + ",")
        lines += ["};", ""]

    lines += ["static const wm_title_background_image title_images[] = {"]
    for i, im in enumerate(images):
        lines.append(
            f"    {{0x{im.source_id:04X}, {im.width}, {im.height}, {im.source_flag}, title_px_{i:02d}}},"
        )
    lines += ["};", "", "static const wm_title_background_palette title_palettes[] = {"]
    for i, pal in enumerate(palettes):
        name = _ident(pal.name)
        lines.append(
            f'    {{"{pal.name}", title_pal_{i:02d}_{name}_opaque, '
            f"title_pal_{i:02d}_{name}_keyed, {len(pal.words_rgb555)}}},"
        )
    lines += [
        "};", "",
        f'static const char title_source_name[] = "{region.region_name}:{source_bdd.name}:{source_bdb.name}";',
        f"static const uint16_t title_source_origin_x = {validation['min_x']};",
        f"static const uint16_t title_source_origin_y = {validation['min_y']};",
        "",
        "size_t wm_title_background_image_count(void) {",
        "    return sizeof(title_images) / sizeof(title_images[0]);",
        "}",
        "const wm_title_background_image *wm_title_background_image_at(size_t index) {",
        "    return index < wm_title_background_image_count() ? &title_images[index] : 0;",
        "}",
        "size_t wm_title_background_palette_count(void) {",
        "    return sizeof(title_palettes) / sizeof(title_palettes[0]);",
        "}",
        "const wm_title_background_palette *wm_title_background_palette_at(size_t index) {",
        "    return index < wm_title_background_palette_count() ? &title_palettes[index] : 0;",
        "}",
        "const char *wm_title_background_source_name(void) { return title_source_name; }",
        "uint16_t wm_title_background_source_origin_x(void) { return title_source_origin_x; }",
        "uint16_t wm_title_background_source_origin_y(void) { return title_source_origin_y; }",
        "",
    ]
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--bdd", required=True, type=pathlib.Path)
    ap.add_argument("--bdb", required=True, type=pathlib.Path)
    ap.add_argument("--bgndtbl", required=True, type=pathlib.Path)
    ap.add_argument("--region", required=True)
    ap.add_argument("--module", required=True)
    ap.add_argument("--out", required=True, type=pathlib.Path)
    ns = ap.parse_args()

    images, palettes = parse_bdd(ns.bdd)
    region = parse_bdb(ns.bdb, ns.region)
    validation = validate_sources(images, palettes, region, ns.bgndtbl, ns.module)
    emit(ns.out, images, palettes, region, validation, ns.bdd, ns.bdb)
    w, h = validation["footprint"]
    print(
        f"BDD background: {ns.region}={w}x{h}, {len(images)} blocks, "
        f"{len(palettes)} palettes, source-origin="
        f"({validation['min_x']},{validation['min_y']})"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, struct.error) as exc:
        print(f"bdd_bundle: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
