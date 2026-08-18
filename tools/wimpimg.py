#!/usr/bin/env python3
"""Read the subset of Midway/Williams WIMP .IMG containers needed by the port.

The format observations used here are deliberately narrow and bounds checked:
- 0x32-byte image directory entries
- 0x1A-byte palette directory entries immediately after the image directory
- palette entries contain a 16-bit color count at +0x0C and a data offset at +0x0E
- source image pixels are CI8, one byte per pixel, rows padded to 4-byte boundaries
- palette words are RGB555 (bit 15 unused): RRRRR GGGGG BBBBB

The WIMP artist container is *before* LOAD2/DMA2 packing. DMA2 packing/compression
belongs to the later graphics-ROM build step, so it is not required to recover the
source pixels from these .IMG files.
"""
from __future__ import annotations
import argparse
import json
import pathlib
import re
import struct
import sys
from dataclasses import asdict, dataclass

HEADER_MIN = 28
IMAGE_ENTRY_SIZE = 0x32
PALETTE_ENTRY_SIZE = 0x1A

@dataclass(frozen=True)
class WimpHeader:
    image_count: int
    unknown_02: int
    directory_offset: int
    version_raw: int
    anim_sequence_count: int
    anim_script_count: int
    unknown_0e: int

@dataclass(frozen=True)
class ImageEntry:
    name: str
    xani: int
    yani: int
    width: int
    height: int
    palette_index_raw: int
    data_offset: int
    tail_words: tuple[int, ...]
    directory_offset: int

@dataclass(frozen=True)
class PaletteEntry:
    name: str
    color_count: int
    data_offset: int
    directory_offset: int


def u16(data: bytes, off: int) -> int:
    return struct.unpack_from("<H", data, off)[0]


def u32(data: bytes, off: int) -> int:
    return struct.unpack_from("<I", data, off)[0]


def s16(data: bytes, off: int) -> int:
    return struct.unpack_from("<h", data, off)[0]


def parse_header(data: bytes) -> WimpHeader:
    if len(data) < HEADER_MIN:
        raise ValueError(f"file too small for WIMP header: {len(data)} bytes")
    h = WimpHeader(
        image_count=u16(data, 0),
        unknown_02=u16(data, 2),
        directory_offset=u32(data, 4),
        version_raw=u16(data, 8),
        anim_sequence_count=u16(data, 10),
        anim_script_count=u16(data, 12),
        unknown_0e=u16(data, 14),
    )
    end = h.directory_offset + h.image_count * IMAGE_ENTRY_SIZE
    if not h.image_count:
        raise ValueError("WIMP container has no images")
    if h.directory_offset < HEADER_MIN or end > len(data):
        raise ValueError(
            f"image directory out of bounds: off=0x{h.directory_offset:X} "
            f"count={h.image_count} entry=0x{IMAGE_ENTRY_SIZE:X} file=0x{len(data):X}"
        )
    return h


def parse_name(raw: bytes) -> str:
    raw = raw.split(b"\0", 1)[0]
    try:
        text = raw.decode("ascii")
    except UnicodeDecodeError as exc:
        raise ValueError("non-ASCII directory name") from exc
    if not text or any(ord(c) < 0x20 or ord(c) > 0x7E for c in text):
        raise ValueError(f"invalid directory name: {text!r}")
    return text


def parse_images(data: bytes, header: WimpHeader) -> list[ImageEntry]:
    out: list[ImageEntry] = []
    for i in range(header.image_count):
        off = header.directory_offset + i * IMAGE_ENTRY_SIZE
        entry = ImageEntry(
            name=parse_name(data[off:off+8]),
            xani=s16(data, off + 18),
            yani=s16(data, off + 20),
            width=u16(data, off + 22),
            height=u16(data, off + 24),
            palette_index_raw=u16(data, off + 26),
            data_offset=u32(data, off + 28),
            # Preserve the complete unknown 18-byte image-entry tail.  r6h3
            # incorrectly assumed its first three words were LOAD2 PWRD1/2/3.
            # They are not: real HRT_WLK data disproved that assumption.
            tail_words=tuple(s16(data, off + 32 + j * 2) for j in range(9)),
            directory_offset=off,
        )
        if entry.width == 0 or entry.height == 0:
            raise ValueError(f"{entry.name}: zero image dimension")
        stride = (entry.width + 3) & ~3
        if entry.data_offset < HEADER_MIN or entry.data_offset + stride * entry.height > len(data):
            raise ValueError(
                f"{entry.name}: CI8 payload outside file: off=0x{entry.data_offset:X} "
                f"stride={stride} h={entry.height} file=0x{len(data):X}"
            )
        out.append(entry)
    return out


def parse_palettes(data: bytes, header: WimpHeader, images: list[ImageEntry]) -> list[PaletteEntry]:
    """Scan the palette directory immediately following image directory entries.

    WIMP does not expose a palette count in the understood part of its header. We
    therefore stop at the first entry that fails conservative palette validation.
    The pixel-data area begins before the image directory, so a palette's data must
    live before the first image pixel offset.
    """
    first_pixel = min(i.data_offset for i in images)
    off = header.directory_offset + header.image_count * IMAGE_ENTRY_SIZE
    out: list[PaletteEntry] = []
    while off + PALETTE_ENTRY_SIZE <= len(data):
        try:
            name = parse_name(data[off:off+8])
        except ValueError:
            break
        count = u16(data, off + 12)
        data_off = u32(data, off + 14)
        if not (1 <= count <= 256):
            break
        if data_off < HEADER_MIN or data_off + count * 2 > first_pixel:
            break
        out.append(PaletteEntry(name, count, data_off, off))
        off += PALETTE_ENTRY_SIZE
    if not out:
        raise ValueError("no valid WIMP palette directory entries found")
    return out


def palette_for_image(image: ImageEntry, images: list[ImageEntry], palettes: list[PaletteEntry]) -> PaletteEntry:
    ids = sorted({i.palette_index_raw for i in images})
    base = ids[0]
    idx = image.palette_index_raw - base
    if idx < 0 or idx >= len(palettes):
        raise ValueError(
            f"{image.name}: palette id {image.palette_index_raw} cannot map to "
            f"{len(palettes)} scanned palettes (base={base})"
        )
    return palettes[idx]


def read_palette_words(data: bytes, pal: PaletteEntry) -> list[int]:
    return [u16(data, pal.data_offset + i * 2) & 0x7FFF for i in range(pal.color_count)]


def read_ci8(data: bytes, image: ImageEntry) -> bytes:
    stride = (image.width + 3) & ~3
    out = bytearray(image.width * image.height)
    for y in range(image.height):
        src = image.data_offset + y * stride
        dst = y * image.width
        out[dst:dst + image.width] = data[src:src + image.width]
    return bytes(out)


def parse_file(path: pathlib.Path):
    data = path.read_bytes()
    header = parse_header(data)
    images = parse_images(data, header)
    palettes = parse_palettes(data, header, images)
    return data, header, images, palettes


def c_ident(s: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", s.lower())


def rgb555_to_rgba5551(value: int, index: int) -> int:
    """Convert Midway RGB555 to N64 RGBA5551; palette index 0 stays transparent."""
    if index == 0:
        return 0
    r = (value >> 10) & 31
    g = (value >> 5) & 31
    b = value & 31
    return (r << 11) | (g << 6) | (b << 1) | 1


def emit_c(path: pathlib.Path, data: bytes, images: list[ImageEntry], palettes: list[PaletteEntry],
           required: list[str], source_container: str = "source.img",
           header_include: str = "wm/bret_sprites.h",
           api_prefix: str = "wm_bret_sprite") -> None:
    by_name = {i.name.upper(): i for i in images}
    wanted: list[ImageEntry] = []
    seen: set[str] = set()
    for name in required:
        key = name.upper()
        if key in seen:
            continue
        if key not in by_name:
            raise ValueError(f"required image missing: {name}")
        seen.add(key)
        wanted.append(by_name[key])

    used_pals: list[PaletteEntry] = []
    for im in wanted:
        pal = palette_for_image(im, images, palettes)
        if pal not in used_pals:
            used_pals.append(pal)

    lines = [
        "/* Auto-generated from an original Midway WIMP container. */",
        f'#include "{header_include}"',
        "#include <string.h>",
        "",
    ]
    for pal in used_pals:
        vals = read_palette_words(data, pal)
        vals = [rgb555_to_rgba5551(v, i) for i, v in enumerate(vals)]
        ident = c_ident(pal.name)
        lines.append(f"static uint16_t pal_{ident}[] __attribute__((aligned(8))) = {{")
        for i in range(0, len(vals), 12):
            lines.append("    " + ", ".join(f"0x{v:04X}" for v in vals[i:i+12]) + ",")
        lines.append("};")
        lines.append("")

    for im in wanted:
        px = read_ci8(data, im)
        ident = c_ident(im.name)
        lines.append(f"static const uint8_t px_{ident}[] __attribute__((aligned(8))) = {{")
        for i in range(0, len(px), 24):
            lines.append("    " + ", ".join(f"0x{v:02X}" for v in px[i:i+24]) + ",")
        lines.append("};")
        lines.append("")

    lines.append("static const wm_source_sprite sprites[] = {")
    for im in wanted:
        pal = palette_for_image(im, images, palettes)
        tail = ", ".join(str(v) for v in im.tail_words)
        lines.append(
            f'    {{"{im.name}", "{source_container}", {im.width}, {im.height}, {im.xani}, {im.yani}, '
            f'{{{tail}}}, px_{c_ident(im.name)}, pal_{c_ident(pal.name)}, {pal.color_count}}},'
        )
    lines += [
        "};",
        "",
        f"const wm_source_sprite *{api_prefix}_find(const char *source_frame) {{",
        "    if (!source_frame) return 0;",
        "    for (size_t i = 0; i < sizeof(sprites)/sizeof(sprites[0]); ++i)",
        "        if (strcmp(sprites[i].source_frame, source_frame) == 0) return &sprites[i];",
        "    return 0;",
        "}",
        "",
        f"const wm_source_sprite *{api_prefix}_at(size_t index) {{",
        "    if (index >= sizeof(sprites)/sizeof(sprites[0])) return 0;",
        "    return &sprites[index];",
        "}",
        "",
        f"size_t {api_prefix}_count(void) {{",
        "    return sizeof(sprites)/sizeof(sprites[0]);",
        "}",
        "",
    ]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines))

def main() -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="command", required=True)
    p = sub.add_parser("probe")
    p.add_argument("file", type=pathlib.Path)
    p.add_argument("--require", action="append", default=[])
    p.add_argument("--json", action="store_true")
    e = sub.add_parser("emit-c")
    e.add_argument("file", type=pathlib.Path)
    e.add_argument("--require", action="append", default=[])
    e.add_argument("--out", required=True, type=pathlib.Path)
    ns = ap.parse_args()

    data, header, images, palettes = parse_file(ns.file)
    names = {e.name.upper() for e in images}
    missing = [n for n in ns.require if n.upper() not in names]
    if missing:
        raise ValueError("required image names missing: " + ", ".join(missing))

    if ns.command == "emit-c":
        if not ns.require:
            raise ValueError("emit-c requires at least one --require image")
        emit_c(ns.out, data, images, palettes, ns.require, ns.file.name)
        print(f"generated {ns.out} from {len(ns.require)} requested source frames")
        return 0

    if ns.json:
        print(json.dumps({
            "header": asdict(header),
            "images": [asdict(e) for e in images],
            "palettes": [asdict(e) for e in palettes],
        }, indent=2))
    else:
        print(
            f"{ns.file}: images={header.image_count} palettes_scanned={len(palettes)} "
            f"dir=0x{header.directory_offset:X} wimp_version_raw=0x{header.version_raw:04X}"
        )
        shown = images
        if ns.require:
            required = {n.upper() for n in ns.require}
            shown = [e for e in images if e.name.upper() in required]
        for e in shown:
            pal = palette_for_image(e, images, palettes)
            print(
                f"{e.name:8s} {e.width:4d}x{e.height:<4d} ani=({e.xani},{e.yani}) "
                f"tail=[{' '.join(str(v) for v in e.tail_words)}] "
                f"pal={pal.name}({pal.color_count}) data=0x{e.data_offset:X}"
            )
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, struct.error) as exc:
        print(f"wimpimg: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
