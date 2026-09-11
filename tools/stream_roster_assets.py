#!/usr/bin/env python3
"""Generate the complete current wrestler-frame payload as DragonFS files.

The authoritative frame set is src/generated/frame_geometry.c, regenerated
from the current translated animation/program corpus and the eight shipped
wrestler .LOD manifests. Every emitted payload is original WIMP CI8 artwork;
nothing is synthesized or substituted.
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


def write_payload(path: pathlib.Path, data: bytes, im, pal) -> int:
    pixels = bytes(wimpimg.read_ci8(data, im))
    expected = int(im.width) * int(im.height)
    if len(pixels) != expected:
        raise ValueError(
            f"{im.name}: CI8 size {len(pixels)} != width*height {expected}")
    words = list(wimpimg.read_palette_words(data, pal))
    if len(words) > TLUT_COLORS:
        raise ValueError(f"{im.name}: palette has {len(words)} colors (>256)")
    rgba = [rgba5551(v, i) for i, v in enumerate(words)]
    rgba.extend([0] * (TLUT_COLORS - len(rgba)))

    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as fh:
        fh.write(pixels)
        for value in rgba:
            fh.write(int(value).to_bytes(2, "big"))
    return len(pixels) + TLUT_COLORS * 2


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

    for rid, _lod_name, display in ROSTERS:
        roster_bytes = 0
        roster_frames = 0
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
            size = write_payload(out_fs / str(rid) / f"{frame}.bin",
                                 data, im, pal)
            roster_bytes += size
            roster_frames += 1
            total_bytes += size
            total_frames += 1
        reports.append(
            f"roster={rid} name={display} frames={roster_frames} bytes={roster_bytes}")

    if total_frames != len(frames):
        raise ValueError(f"generated {total_frames} frames, expected {len(frames)}")

    stamp = [
        "WrestleMania Arcade streamed roster art",
        f"frames={total_frames}",
        f"bytes={total_bytes}",
        "format=CI8 pixels followed by 256 big-endian RGBA5551 TLUT entries",
        "source=original Midway WIMP .IMG payload resolved through shipped wrestler .LOD files",
        *reports,
        "",
    ]
    (out_fs / "STREAMED_ROSTER.txt").write_text("\n".join(stamp))
    print(f"streamed roster: {total_frames} frames, {total_bytes} bytes")
    for line in reports:
        print(line)
    return total_frames, total_bytes


def self_test() -> int:
    assert len(ROSTERS) == 8
    assert [r[0] for r in ROSTERS] == [0, 1, 2, 3, 4, 5, 6, 8]
    assert rgba5551(0x7FFF, 1) == 0xFFFF
    assert rgba5551(0x0000, 0) == 0
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
