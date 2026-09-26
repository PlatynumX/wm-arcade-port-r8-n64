#!/usr/bin/env python3
"""RD7FONT's 93 glyph slots and the width of each, from TEXT.ASM + TROGF7.IMG.

UTIL.ASM's string engine indexes its font table by `char - 33`: stringr1_1
does `subk 32,a1` (and treats <= 0 as a space), then `subk 1,a1`, `sll 5,a1`
for the long stride, and adds the table base. RD7FONT's first entry is
FONT7excla, which is '!' -- character 33 -- so the derivation and the data
agree.

WIDTH IS THE ONE NUMBER THE ENGINE READS out of a glyph: both stringr1_1
(`move *a1,a1` after the header deref) and STRNGLEN (`move *a14,a14  ;Get
ISIZEX`) take the image header's first word and nothing else.

SIX SLOTS ARE NOT MEASURABLE HERE, and they are emitted as -1 rather than
guessed. WIMP image names are truncated to eight characters, so
FONT7parenl, FONT7parenr, FONT7paren2l and FONT7paren2r all become
`FONT7par`, and FONT7percen and FONT7period both become `FONT7per`.
TROGF7.IMG holds four `FONT7par` images as two adjacent pairs -- (3x10,
3x10) and (4x10, 4x10) -- so one pair is the parens and the other the
brace-slot paren2 variants, and the container does not say which. Telling
them apart needs FONTS.LOD's packing order, and that file is zero bytes in
this tree; tools/wlstring.py records the same blocker for 46 other symbols.
Picking 3 or 4 would silently move every centred line containing a
parenthesis.
"""
from __future__ import annotations
import argparse
import collections
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wimpimg  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parents[1]
SRC = ROOT / "original" / "wwf-wrestlemania"
NAME_FIELD = 8  # WIMP truncates a symbol to eight characters

# UTIL.ASM's index base: RD7FONT[0] is character 33.
FIRST_CHAR = 33


def rd7font_slots() -> list[str]:
    """RD7FONT's entries, in table order, from the shipped TEXT.ASM.

    text.obj is in WRESTLE.CMD, so TEXT.ASM is the file the game builds.
    BACKUP/TEXT.ASM is a second copy in this tree and is not read here.

    Their RD7FONT tables are IDENTICAL today, all 93 slots, so swapping the
    two changes nothing and no test can tell them apart -- checked, rather
    than assumed. The path is still spelled explicitly and the guard still
    asserts text.obj is linked, because that is what makes this the right
    file if the two ever diverge, the way DNK.ASM diverged from DOINK.ASM
    and cost Doink his walk speed.
    """
    lines = (SRC / "TEXT.ASM").read_text(errors="replace").splitlines()
    start = next(i for i, l in enumerate(lines) if re.match(r"^RD7FONT\s*$", l))
    names: list[str] = []
    for line in lines[start + 1:]:
        if re.match(r"^[A-Za-z_]\w*\s*$", line):  # RD15FONT ends the table
            break
        m = re.match(r"\s*\.long\s+(.*)", line)
        if m:
            body = m.group(1).split(";")[0]
            names += [x.strip() for x in body.split(",") if x.strip()]
    return names


def container_widths() -> dict[str, set[int]]:
    """Truncated image name -> every width seen under it, across IMG/."""
    out: dict[str, set[int]] = collections.defaultdict(set)
    for p in sorted(SRC.glob("IMG/*")):
        try:
            _d, _h, images, _pals = wimpimg.parse_file(p)
        except Exception:
            continue
        for im in images:
            out[im.name].add(im.width)
    return out



def _pixels(data: bytes, image) -> "list[list[int]]":
    """The image's CI8 rows, as 0/1 ink."""
    raw = wimpimg.read_ci8(data, image)
    return [[1 if b else 0 for b in raw[y * image.width:(y + 1) * image.width]]
            for y in range(image.height)]


def _art_map(rows: "list[list[int]]") -> "list[str]":
    return ["".join("#" if v else "." for v in r) for r in rows]


def resolve_by_artwork(unmeasured: "list[str]") -> "tuple[dict[str, int], dict[str, list[str]]]":
    """Identify what a truncated name cannot, by reading the glyph itself.

    The slot POSITION already says which character a slot is -- RD7FONT slot 4
    is '%' and slot 13 is '.', because the table is indexed by char - 33. What
    the eight-character name cannot say is which IMAGE is which, since
    FONT7percen and FONT7period both truncate to `FONT7per`. The artwork
    settles that, and reading the artwork is reading the source, not guessing
    at it.

    ONLY the FONT7per pair is resolved here, and only because it is not a
    judgement call: TROGF7.IMG holds an 11x8 figure of two rings joined by a
    diagonal, and a 3x2 solid block. One is a percent sign and the other is a
    full stop, and no one would order them the other way.

    THE FOUR FONT7par SLOTS ARE DELIBERATELY LEFT UNRESOLVED. They are two
    mirrored pairs, 3x10 and 4x10, and one pair is (), the other {}. A left
    parenthesis and a left brace have the SAME gross outline at this size --
    stroke at the outer columns top and bottom, at the inner column across the
    middle. What separates them in a real typeface is that a brace carries a
    pointed spur where a parenthesis is a smooth arc, and at four pixels wide
    with a two-pixel stroke that distinction is not present in the data to be
    read. Container order was checked as a second source and does not help:
    across the 78 glyphs whose pairing is already certain, container order and
    table order disagree 1517 times, so packing order carries no information
    here. Both maps are recorded below so the next reader can judge for
    themselves rather than take this on trust.
    """
    data, _h, images, _p = wimpimg.parse_file(SRC / "IMG" / "TROGF7.IMG")
    by_stem: dict[str, list] = collections.defaultdict(list)
    for im in images:
        by_stem[im.name].append(im)

    resolved: dict[str, int] = {}
    evidence: dict[str, list[str]] = {}

    per = by_stem.get("FONT7per", [])
    if len(per) == 2:
        # SIZE is the whole argument, and it is sufficient: a 3x2 image cannot
        # be a percent sign and an 11x8 one cannot be a full stop. An ink test
        # was here as well and was removed -- mutation testing showed it could
        # not fail, because the two sizes already separate the pair, and a
        # check that cannot fail is decoration that later reads as evidence.
        def is_dot(im) -> bool:
            return im.width <= 3 and im.height <= 3

        dots = [im for im in per if is_dot(im)]
        others = [im for im in per if not is_dot(im)]
        if len(dots) == 1 and len(others) == 1:
            resolved["FONT7period"] = dots[0].width
            resolved["FONT7percen"] = others[0].width
            evidence["FONT7period"] = _art_map(_pixels(data, dots[0]))
            evidence["FONT7percen"] = _art_map(_pixels(data, others[0]))

    for im in by_stem.get("FONT7par", []):
        key = f"FONT7par {im.width}x{im.height} #{images.index(im)}"
        evidence[key] = _art_map(_pixels(data, im))

    return resolved, evidence


def build() -> tuple[list[str], list[str], dict[str, int]]:
    slots = rd7font_slots()
    if len(slots) != 93:
        raise SystemExit(f"RD7FONT has {len(slots)} entries, expected 93")
    widths = container_widths()

    measured: dict[str, int] = {}
    unmeasured: list[str] = []
    for sym in sorted(set(slots)):
        seen = widths.get(sym[:NAME_FIELD], set())
        if len(seen) == 1:
            measured[sym] = next(iter(seen))
        else:
            unmeasured.append(sym)

    # What the truncated name could not say, the artwork can -- for the two
    # glyphs where it is not a judgement call. See resolve_by_artwork.
    by_art, evidence = resolve_by_artwork(unmeasured)
    for sym, w in by_art.items():
        measured[sym] = w
        if sym in unmeasured:
            unmeasured.remove(sym)

    out = [
        "/* Generated by tools/wlfont7.py from TEXT.ASM and IMG/TROGF7.IMG.",
        " * Do not edit.",
        " *",
        " * RD7FONT, the font UTIL.ASM's string engine draws the copyright and",
        " * AAMA screens with. Slot i is character i + 33: the table's first",
        " * entry is FONT7excla, and stringr1_1 indexes it by `char - 33`.",
        " *",
        " * width == -1 means the width is NOT KNOWN, not zero. Six slots share",
        " * an eight-character WIMP name with another glyph (FONT7parenl vs",
        " * FONT7paren2l, FONT7percen vs FONT7period) and FONTS.LOD, which would",
        " * give the packing order that separates them, is zero bytes here.",
        " *",
        " * IDENTIFIED BY ARTWORK, because the truncated name could not say",
        " * which image was which. The slot position gives the character; these",
        " * maps give which glyph is that character. Judge them yourself:",
    ]
    for name in sorted(k for k in evidence if not k.startswith("FONT7par ")):
        out.append(f" *   {name} -> width {measured[name]}")
        for row in evidence[name]:
            out.append(f" *     {row}")
    out += [
        " *",
        " * STILL UNRESOLVED, and left at -1 on purpose. Two mirrored pairs; one",
        " * pair is () and the other {}. At this size a left parenthesis and a",
        " * left brace have the same outline, and the spur that separates them",
        " * is not in the data:",
    ]
    for name in sorted(k for k in evidence if k.startswith("FONT7par ")):
        out.append(f" *   {name}")
        for row in evidence[name]:
            out.append(f" *     {row}")
    out += [
        " */",
        '#include "wm/arcade/wm_arcade_rd7font.h"',
        "",
        "const int16_t wm_rd7font_width[WM_RD7FONT_SLOTS] = {",
    ]
    for i, sym in enumerate(slots):
        w = measured.get(sym, -1)
        ch = chr(i + FIRST_CHAR)
        shown = ch if ch not in ('"', "\\") else "\\" + ch
        out.append(f"    {w:>3},  /* [{i:2}] '{shown}'  {sym} */")
    out += [
        "};",
        "",
    ]
    return out, unmeasured, measured


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=pathlib.Path)
    ns = ap.parse_args()
    out, unmeasured, measured = build()
    text = "\n".join(out)
    if ns.out:
        ns.out.write_text(text)
        print(f"RD7FONT: 93 slots, {len(measured)} distinct glyphs measured, "
              f"{len(unmeasured)} not measurable ({', '.join(unmeasured)})")
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
