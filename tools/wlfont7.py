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



def facing(rows: "list[list[int]]") -> int:
    """-1 when the glyph faces left, +1 when right, 0 when it is symmetric.

    A bracket, parenthesis or brace is told apart from its mirror by where its
    ENDS sit relative to its MIDDLE, not by total ink. `(` anchors its ends to
    the right and pushes its middle left; `)` is the reverse. Centre-of-mass
    does not work for this: the 4x10 pair holds its extreme for only two of ten
    rows, so the mean follows the broad part and points the wrong way.
    """
    h = len(rows)
    if h < 4:
        return 0

    def mean_x(sel: "list[int]") -> float:
        xs = [x for r in sel for x, v in enumerate(r) if v]
        return sum(xs) / len(xs) if xs else 0.0

    ends = mean_x([rows[0], rows[h - 1]])
    mid = mean_x(rows[h // 2 - 1:h // 2 + 1])
    if abs(ends - mid) < 0.25:
        return 0
    return -1 if ends > mid else 1


def resolve_mirror_pair(data: bytes, candidates: list, left_name: str,
                        right_name: str) -> "dict[str, object]":
    """Assign a left/right name to each of two mirrored images, by facing.

    FONT7bracl and FONT7bracr both truncate to `FONT7bra` and are both 3x10,
    so the WIDTH was never ambiguous but which image is `[` and which is `]`
    was. The artwork answers it: a left bracket's top and bottom bars reach
    right of its upright, a right bracket's reach left.
    """
    if len(candidates) != 2:
        return {}
    sided = {}
    for im in candidates:
        f = facing(_pixels(data, im))
        if f < 0:
            sided.setdefault("left", []).append(im)
        elif f > 0:
            sided.setdefault("right", []).append(im)
    if len(sided.get("left", [])) != 1 or len(sided.get("right", [])) != 1:
        return {}
    return {left_name: sided["left"][0], right_name: sided["right"][0]}


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



def glyph_images() -> "dict[str, object]":
    """The image backing each RD7FONT symbol, where it can be established.

    Unambiguous stem -> the one image. FONT7bra and FONT7par are mirrored
    pairs and resolve by facing. FONT7per resolves by size. What is left
    unresolved is only WHICH PAIR of FONT7par is () and which is {} -- both
    pairs' left and right are known, the pair assignment is not, so all four
    are omitted rather than half-guessed.
    """
    data, _h, images, _p = wimpimg.parse_file(SRC / "IMG" / "TROGF7.IMG")
    by_stem: dict[str, list] = collections.defaultdict(list)
    for im in images:
        by_stem[im.name].append(im)

    out: dict[str, object] = {}
    for sym in set(rd7font_slots()):
        cands = by_stem.get(sym[:NAME_FIELD], [])
        if len(cands) == 1:
            out[sym] = cands[0]

    out.update(resolve_mirror_pair(data, by_stem.get("FONT7bra", []),
                                   "FONT7bracl", "FONT7bracr"))

    per = by_stem.get("FONT7per", [])
    if len(per) == 2:
        small = [im for im in per if im.width <= 3 and im.height <= 3]
        big = [im for im in per if im not in small]
        if len(small) == 1 and len(big) == 1:
            out["FONT7period"] = small[0]
            out["FONT7percen"] = big[0]
    return data, out


def build_glyphs() -> "tuple[list[str], int, list[str]]":
    """Emit each slot's ink mask: one byte per pixel, 1 where the glyph draws.

    A mask is all the renderer needs. SYS.EQU:217 is
    `DMACNZ .equ 8008h ;WRITE CONSTANT ON NON-ZERO DATA`, and STRCNRMO_2 sets
    exactly that -- so a glyph is stencilled in ONE colour, the constant
    show_copyright passes as `movi [>1111,0000],a6  ;pal 0, color 17`. The
    glyph's own palette never reaches the screen on this path, which is why
    the mask, not the CI8 pixels, is the artwork this needs.
    """
    slots = rd7font_slots()
    data, backing = glyph_images()

    body: list[str] = []
    table: list[str] = []
    missing: list[str] = []
    emitted: dict[str, str] = {}

    for i, sym in enumerate(slots):
        im = backing.get(sym)
        if im is None:
            if sym not in missing:
                missing.append(sym)
            table.append(f"    {{ 0, 0, 0 }},  /* [{i:2}] {sym}: unresolved */")
            continue
        if sym not in emitted:
            rows = _pixels(data, im)
            flat = [v for r in rows for v in r]
            name = f"s_ink_{sym}"
            emitted[sym] = name
            body.append(f"/* {sym}  {im.width}x{im.height} */")
            body.append(f"static const uint8_t {name}[{len(flat)}] = {{")
            for r in rows:
                body.append("    " + ", ".join(str(v) for v in r) + ",")
            body.append("};")
            body.append("")
        table.append(
            f"    {{ {im.width:2}, {im.height:2}, {emitted[sym]} }},"
            f"  /* [{i:2}] '{chr(i + FIRST_CHAR) if chr(i + FIRST_CHAR) not in chr(34) + chr(92) else '?'}' {sym} */"
        )

    out = [
        "/* Generated by tools/wlfont7.py from TEXT.ASM and IMG/TROGF7.IMG.",
        " * Do not edit.",
        " *",
        " * RD7FONT's glyphs as INK MASKS -- one byte per pixel, 1 where the",
        " * glyph draws. A mask is all this path needs: SYS.EQU:217 is",
        " * `DMACNZ .equ 8008h ;WRITE CONSTANT ON NON-ZERO DATA` and STRCNRMO_2",
        " * sets exactly those flags, so every glyph is stencilled in the ONE",
        " * colour its caller passes -- show_copyright's",
        " * `movi [>1111,0000],a6  ;pal 0, color 17`. The glyph's own palette",
        " * never reaches the screen here.",
        " *",
        " * A row of { 0, 0, 0 } is a slot whose backing image could not be",
        " * established, not a blank glyph. Callers must skip it rather than",
        " * draw nothing and advance as if they had.",
        " */",
        '#include "wm/arcade/wm_arcade_rd7font.h"',
        "",
    ] + body + [
        "const wm_rd7font_glyph wm_rd7font_glyph_table[WM_RD7FONT_SLOTS] = {",
    ] + table + [
        "};",
        "",
    ]
    return out, len(emitted), missing


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=pathlib.Path)
    ap.add_argument("--out-glyphs", type=pathlib.Path)
    ns = ap.parse_args()
    out, unmeasured, measured = build()
    text = "\n".join(out)
    if ns.out:
        ns.out.write_text(text)
        print(f"RD7FONT: 93 slots, {len(measured)} distinct glyphs measured, "
              f"{len(unmeasured)} not measurable ({', '.join(unmeasured)})")
    elif not ns.out_glyphs:
        sys.stdout.write(text)
    if ns.out_glyphs:
        g, n, missing = build_glyphs()
        ns.out_glyphs.write_text("\n".join(g))
        print(f"RD7FONT masks: {n} distinct glyphs, "
              f"{len(missing)} unresolved ({', '.join(missing)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
