#!/usr/bin/env python3
"""Generate the complete current wrestler-frame payload as DragonFS files.

The authoritative frame set is src/generated/frame_geometry.c, regenerated
from the current translated animation/program corpus and the eight shipped
wrestler .LOD manifests. Every emitted payload is original WIMP CI8 artwork;
nothing is synthesized or substituted.

Palettes are stored ONCE, not once per frame. A wrestler has a handful
of TLUTs and several hundred frames -- Bret has 619 frames and two
distinct palettes, the Undertaker 608 and seven -- so the whole roster
is 5,126 frames sharing 23 palettes between them. Writing 512 bytes of
TLUT into every frame file spent 2.5 MiB of cartridge storing those 23
palettes 5,126 times, which matters on a part where the art is already
most of the ROM. Each frame now carries a four-byte header naming its
palette, and the palettes sit beside the frames as pN.pal.
"""
from __future__ import annotations

import argparse
import pathlib
import re
import shutil
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import bret_bundle  # noqa: E402
import bret_manifest  # noqa: E402
import bret_geometry_bundle  # noqa: E402
import wimpimg  # noqa: E402

ROSTERS = (
    (0, "BRET.LOD", "Bret Hart"),
    (1, "RAZOR.LOD", "Razor Ramon"),
    (2, "TAKER.LOD", "Undertaker"),
    (3, "YOKO.LOD", "Yokozuna"),
    (4, "SHAWN.LOD", "Shawn Michaels"),
    (5, "BAM.LOD", "Bam Bam Bigelow"),
    (6, "DOINK.LOD", "Doink"),
    (8, "LEX.LOD", "Lex Luger"),
)
FRAME_RE = re.compile(r'^\s*\{"([^"]+)"\s*,', re.M)
SAFE_FRAME_RE = re.compile(r'^[A-Za-z0-9_]+$')
TLUT_COLORS = 256
TLUT_BYTES = TLUT_COLORS * 2
# Two magic bytes then a big-endian palette index.
PAYLOAD_MAGIC = b"WM"
PAYLOAD_HEADER = 4


def rgba5551(rgb555: int, index: int) -> int:
    if index == 0:
        return 0
    return (((rgb555 >> 10) & 31) << 11) | (((rgb555 >> 5) & 31) << 6) | ((rgb555 & 31) << 1) | 1


def geometry_frames(path: pathlib.Path) -> list[str]:
    text = path.read_text(errors="strict")
    out: list[str] = []
    seen: set[str] = set()
    for raw in FRAME_RE.findall(text):
        name = raw.upper()
        if name not in seen:
            seen.add(name)
            out.append(name)
    if not out:
        raise ValueError(f"no frame rows found in {path}")
    return out


def load_roster_maps(img_dir: pathlib.Path, wanted: set[str]):
    owner: dict[str, tuple[int, str, str]] = {}
    order: dict[tuple[int, str], list[str]] = {}
    per_roster: dict[int, list[str]] = {rid: [] for rid, _lod, _name in ROSTERS}

    for rid, lod_name, display in ROSTERS:
        lod = img_dir / lod_name
        if not lod.is_file():
            raise ValueError(f"missing roster manifest: {lod}")
        mapping = bret_manifest.parse_lod(lod)
        for raw_name, raw_container in mapping.items():
            frame = raw_name.upper()
            container = raw_container
            order.setdefault((rid, container), []).append(frame)
            if frame not in wanted:
                continue
            prev = owner.get(frame)
            here = (rid, container, display)
            if prev is not None and prev[:2] != here[:2]:
                raise ValueError(
                    f"{frame}: ambiguous roster ownership: "
                    f"{prev[2]}:{prev[1]} vs {display}:{container}"
                )
            if prev is None:
                owner[frame] = here
                per_roster[rid].append(frame)
    return owner, order, per_roster


def resolve_image(frame: str, container: str, lod_order: list[str], parsed):
    data, images, palettes = parsed
    exact = [im for im in images if im.name.upper() == frame]
    if len(exact) == 1:
        im = exact[0]
    elif len(exact) > 1:
        raise ValueError(f"{frame}: duplicate exact WIMP names in {container}")
    else:
        by_name = {im.name.upper(): im for im in images}
        im = bret_geometry_bundle._by_truncated_name(
            frame, lod_order, images, by_name)
    if im is None:
        raise ValueError(f"{frame}: listed in .LOD but missing from {container}")
    pal = wimpimg.palette_for_image(im, images, palettes)
    return data, im, pal


def tlut_bytes(data: bytes, im, pal) -> bytes:
    """One frame's 256-entry big-endian RGBA5551 TLUT."""
    words = list(wimpimg.read_palette_words(data, pal))
    if len(words) > TLUT_COLORS:
        raise ValueError(f"{im.name}: palette has {len(words)} colors (>256)")
    rgba = [rgba5551(v, i) for i, v in enumerate(words)]
    rgba.extend([0] * (TLUT_COLORS - len(rgba)))
    return b"".join(int(v).to_bytes(2, "big") for v in rgba)


def write_payload(path: pathlib.Path, data: bytes, im,
                  palette_index: int) -> int:
    """One frame: a four-byte header then the CI8 pixels.

    The TLUT is NOT here. A wrestler has a handful of palettes and
    several hundred frames -- Bret has 619 frames and two distinct
    TLUTs -- so storing 512 bytes with every frame spent 2.5 MiB of
    cartridge on 23 palettes written 5,126 times. They live in their
    own files beside the frames now and the header says which one.
    """
    pixels = bytes(wimpimg.read_ci8(data, im))
    expected = int(im.width) * int(im.height)
    if len(pixels) != expected:
        raise ValueError(
            f"{im.name}: CI8 size {len(pixels)} != width*height {expected}")
    if palette_index < 0 or palette_index > 0xFFFF:
        raise ValueError(f"{im.name}: palette index {palette_index} out of range")

    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as fh:
        fh.write(PAYLOAD_MAGIC)
        fh.write(int(palette_index).to_bytes(2, "big"))
        fh.write(pixels)
    return PAYLOAD_HEADER + len(pixels)


def emit(geometry: pathlib.Path, img_dir: pathlib.Path,
         out_fs: pathlib.Path) -> tuple[int, int]:
    frames = geometry_frames(geometry)
    wanted = set(frames)
    owner, order, per_roster = load_roster_maps(img_dir, wanted)

    missing = [f for f in frames if f not in owner]
    if missing:
        raise ValueError(
            f"{len(missing)} current geometry frame(s) have no roster .LOD owner: "
            + ", ".join(missing[:24]))

    if out_fs.exists():
        shutil.rmtree(out_fs)
    out_fs.mkdir(parents=True, exist_ok=True)

    parsed_containers: dict[tuple[int, str], tuple[bytes, list, list]] = {}
    total_bytes = 0
    total_frames = 0
    reports: list[str] = []

    total_palettes = 0
    for rid, _lod_name, display in ROSTERS:
        roster_bytes = 0
        roster_frames = 0
        # This wrestler's distinct TLUTs, in first-seen order. They are
        # written once each rather than once per frame.
        palettes_seen: dict[bytes, int] = {}
        for frame in per_roster[rid]:
            if not SAFE_FRAME_RE.fullmatch(frame):
                raise ValueError(f"unsafe frame name for DragonFS path: {frame!r}")
            _owner_id, container, _display = owner[frame]
            key = (rid, container)
            if key not in parsed_containers:
                source = bret_bundle.resolve_case_insensitive(img_dir, container)
                data, _hdr, images, palettes = wimpimg.parse_file(source)
                parsed_containers[key] = (data, images, palettes)
            data, im, pal = resolve_image(
                frame, container, order[key], parsed_containers[key])
            tlut = tlut_bytes(data, im, pal)
            index = palettes_seen.get(tlut)
            if index is None:
                index = len(palettes_seen)
                palettes_seen[tlut] = index
            size = write_payload(out_fs / str(rid) / f"{frame}.bin",
                                 data, im, index)
            roster_bytes += size
            roster_frames += 1
            total_bytes += size
            total_frames += 1
        for tlut, index in palettes_seen.items():
            pal_path = out_fs / str(rid) / f"p{index}.pal"
            pal_path.parent.mkdir(parents=True, exist_ok=True)
            pal_path.write_bytes(tlut)
            roster_bytes += len(tlut)
            total_bytes += len(tlut)
        total_palettes += len(palettes_seen)
        reports.append(
            f"roster={rid} name={display} frames={roster_frames} "
            f"palettes={len(palettes_seen)} bytes={roster_bytes}")

    if total_frames != len(frames):
        raise ValueError(f"generated {total_frames} frames, expected {len(frames)}")

    stamp = [
        "WrestleMania Arcade streamed roster art",
        f"frames={total_frames}",
        f"bytes={total_bytes}",
        f"palettes={total_palettes}",
        "format=frame is 'WM' + big-endian u16 palette index + CI8 pixels;"
        " pN.pal is 256 big-endian RGBA5551 entries",
        "source=original Midway WIMP .IMG payload resolved through shipped wrestler .LOD files",
        *reports,
        "",
    ]
    (out_fs / "STREAMED_ROSTER.txt").write_text("\n".join(stamp))
    print(f"streamed roster: {total_frames} frames, {total_palettes} "
          f"palettes, {total_bytes} bytes")
    for line in reports:
        print(line)
    return total_frames, total_bytes


class _FakeImage:
    """Just enough of a WIMP image record for the payload writer."""

    def __init__(self, name, width, height):
        self.name = name
        self.width = width
        self.height = height


def self_test() -> int:
    import tempfile

    assert len(ROSTERS) == 8
    assert [r[0] for r in ROSTERS] == [0, 1, 2, 3, 4, 5, 6, 8]
    assert rgba5551(0x7FFF, 1) == 0xFFFF
    assert rgba5551(0x0000, 0) == 0
    # Index 0 is transparent whatever colour the source gives it, and
    # every other entry gets the alpha bit set.
    assert rgba5551(0x7FFF, 0) == 0
    assert PAYLOAD_HEADER == len(PAYLOAD_MAGIC) + 2

    # The payload format, round-tripped: a frame is the magic, a
    # big-endian palette index and then width*height CI8 bytes, with
    # NO palette in it. Getting this wrong silently feeds the loader
    # pixels offset by four bytes, which is not a crash -- it is a
    # smeared sprite -- so it is checked rather than assumed.
    original_read_ci8 = wimpimg.read_ci8
    try:
        pixels = bytes(range(64))
        wimpimg.read_ci8 = lambda data, im: pixels
        im = _FakeImage("TESTFRAME", 8, 8)
        with tempfile.TemporaryDirectory() as tmp:
            out = pathlib.Path(tmp) / "TESTFRAME.bin"
            size = write_payload(out, b"", im, 0x0102)
            blob = out.read_bytes()
            assert size == len(blob) == PAYLOAD_HEADER + 64, (size, len(blob))
            assert blob[:2] == PAYLOAD_MAGIC
            assert blob[2] == 0x01 and blob[3] == 0x02
            assert blob[PAYLOAD_HEADER:] == pixels
            # A mismatched width*height is refused, not padded.
            try:
                write_payload(out, b"", _FakeImage("BAD", 9, 9), 0)
            except ValueError:
                pass
            else:
                raise AssertionError("write_payload accepted a size mismatch")
    finally:
        wimpimg.read_ci8 = original_read_ci8

    print("stream_roster_assets self-test: PASS")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--geometry", type=pathlib.Path)
    ap.add_argument("--img-dir", type=pathlib.Path)
    ap.add_argument("--out-fs", type=pathlib.Path)
    ap.add_argument("--self-test", action="store_true")
    ns = ap.parse_args()
    if ns.self_test:
        return self_test()
    if not ns.geometry or not ns.img_dir or not ns.out_fs:
        ap.error("--geometry --img-dir --out-fs are required")
    emit(ns.geometry, ns.img_dir, ns.out_fs)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as exc:
        print(f"stream_roster_assets: error: {exc}", file=sys.stderr)
        raise SystemExit(2)
