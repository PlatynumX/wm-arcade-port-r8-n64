#!/usr/bin/env python3
"""IMGPAL.ASM's palettes -- the colour data PAL.ASM hands to the hardware.

A palette in this game is one contiguous block: a count word, then that
many 16-bit colours.  Every routine in PAL.ASM reads it that way --

    move  *a1+,a14      ; # Colors
    move  a14,*a0+
    sll   32-9,a14      ; Remove any flags
    srl   32-9,a14      ; 9 bits needed for # colors

-- so the count word's top bits are flags and only its low nine are the
length.  pal_getf does the same thing when it sets up a transfer
(`move *a0+,a2  ;Get # colors in pal`).

Colours are RGB555 packed the way pal_fade unpacks them: blue in bits
0-4, green in 5-9, red in 10-14.  Bit 15 is not touched by any of the
fade code, which masks each channel to five bits and reassembles with
`sll 10` / `sll 5` / `or` -- so a palette that has bit 15 set loses it
the first time it is faded.  That is the arcade's behaviour and the data
is emitted here exactly as written, bit 15 included.
"""
from __future__ import annotations
import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim  # noqa: E402

SRC = wlanim.ORIG / "IMGPAL.ASM"
WRESPAL = wlanim.ORIG / "WRESPAL.ASM"

# Both files are linked (WRESTLE.CMD lists wrespal.obj and imgpal.obj), and
# they share names -- BAMBLU_P is declared in each. WRESPAL.ASM's copy is
# inside `.if 0`, so the assembler only ever sees IMGPAL.ASM's. Getting the
# conditionals right is therefore not optional here.
SOURCES = (SRC, WRESPAL)

LABEL_RE = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*):\s*$")
# WRESPAL.ASM labels its palettes with the SUBR macro instead of a colon.
SUBR_RE = re.compile(r"^\s*SUBRP?\s+([A-Za-z_][A-Za-z0-9_]*)\s*$", re.I)
WORD_RE = re.compile(r"^\s*\.word\s+(.+)$", re.I)
IF_RE = re.compile(r"^\s*\.if\s+(.+)$", re.I)
ELSE_RE = re.compile(r"^\s*\.else\s*$", re.I)
ENDIF_RE = re.compile(r"^\s*\.endif\s*$", re.I)

# PAL.ASM's 9-bit mask on the count word.
COUNT_MASK = 0x1FF


def _num(tok: str) -> int:
    tok = tok.strip()
    if not tok:
        raise ValueError("empty numeric token")
    if tok[-1] in "hH":
        return int(tok[:-1], 16)
    return int(tok, 10)


def _one_file(path: pathlib.Path, out: dict[str, list[int]],
              aliases: dict[str, str]) -> None:
    """Read one file's palettes into `out`, honouring `.if`.

    Only constant conditions are understood. A `.if` on anything else is
    refused rather than assumed true, because guessing wrong here would
    silently pull in a disabled duplicate of a live palette.

    Several labels can share one block -- WRESPAL.ASM stacks UNDGRN_P and
    UNDBLU_P on the same data -- so each extra name is recorded as an
    alias and gets the same colours.
    """
    lines = [wlanim.strip_comment(r)
             for r in path.read_text(errors="replace").splitlines()]

    names: list[str] = []
    words: list[int] = []
    # A stack of "is this region assembled", so a nested .if inside a
    # disabled one stays disabled.
    stack: list[bool] = []

    def live() -> bool:
        return all(stack)

    def close() -> None:
        nonlocal names, words
        if not names:
            return
        if not words:
            # A label with no data of its own is not an error; it is a
            # second name for whatever block comes next.
            return
        want = words[0] & COUNT_MASK
        if len(words) - 1 != want:
            raise ValueError(
                f"{path.name}: {names[0]}: count word {words[0]:#x} says "
                f"{want} colours but the block holds {len(words) - 1}")
        for i, name in enumerate(names):
            if name in out:
                raise ValueError(f"{name}: defined twice")
            out[name] = words
            if i:
                aliases[name] = names[0]
        names, words = [], []

    for line in lines:
        m = IF_RE.match(line)
        if m:
            cond = m.group(1).strip()
            if cond not in ("0", "1"):
                raise ValueError(
                    f"{path.name}: `.if {cond}` is not a constant; this "
                    f"reader cannot tell whether the block assembles")
            close()
            stack.append(cond == "1")
            continue
        if ELSE_RE.match(line):
            close()
            if not stack:
                raise ValueError(f"{path.name}: .else with no .if")
            stack[-1] = not stack[-1]
            continue
        if ENDIF_RE.match(line):
            close()
            if not stack:
                raise ValueError(f"{path.name}: .endif with no .if")
            stack.pop()
            continue
        if not live():
            continue

        m = LABEL_RE.match(line) or SUBR_RE.match(line)
        if m:
            if words:
                close()         # a new label after data starts a new block
            names.append(m.group(1))
            continue
        m = WORD_RE.match(line)
        if m:
            if not names:
                continue        # data before the first label
            words += [_num(tok) for tok in m.group(1).split(",") if tok.strip()]
            continue
        if line.strip() and not line.lstrip().startswith("."):
            close()
    close()
    if stack:
        raise ValueError(f"{path.name}: {len(stack)} unterminated .if")


def palettes(paths=None) -> dict[str, list[int]]:
    """Every assembled palette across IMGPAL.ASM and WRESPAL.ASM."""
    return palettes_with_aliases(paths)[0]


def palettes_with_aliases(paths=None):
    out: dict[str, list[int]] = {}
    aliases: dict[str, str] = {}
    for path in (paths or SOURCES):
        if path.exists():
            _one_file(path, out, aliases)
    return out, aliases


def render_c() -> str:
    pals, aliases = palettes_with_aliases()
    out = [
        "/* Generated by tools/wlpal.py from IMGPAL.ASM and WRESPAL.ASM.",
        " * Do not edit.",
        " *",
        " * Each palette is a count word followed by that many RGB555",
        " * colours, which is exactly how PAL.ASM reads one. The count",
        " * word's low nine bits are the length; the rest are flags.",
        " *",
        " * Both files are linked and they share names, so the `.if`",
        " * blocks decide which copy exists. Seven of WRESPAL.ASM's",
        " * palettes -- BAMBLU_P among them -- sit inside `.if 0` and",
        " * are not here; IMGPAL.ASM's BAMBLU_P is.",
        " */",
        '#include "wm/arcade/wm_arcade_pal.h"',
        "",
    ]
    emitted: set[str] = set()
    for name in pals:
        if name in aliases:
            continue            # a second name for a block already emitted
        words = pals[name]
        emitted.add(name)
        out.append("static const uint16_t pal_%s[] = {" % name)
        for base in range(0, len(words), 8):
            out.append("    " + ", ".join("0x%04X" % w
                                          for w in words[base:base + 8]) + ",")
        out.append("};")
    out.append("")
    out.append("/* Sorted by name so the lookup can bisect. */")
    out.append("const wm_palette wm_palettes[] = {")
    for name in sorted(pals):
        target = aliases.get(name, name)
        note = "  /* the same block as %s */" % target if name in aliases else ""
        out.append('    { "%s", pal_%s },%s' % (name, target, note))
    out.append("};")
    out.append("")
    out.append("const int wm_palette_count = %d;" % len(pals))
    out.append("")
    return "\n".join(out)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out")
    ap.add_argument("--list", action="store_true")
    ns = ap.parse_args()
    pals, aliases = palettes_with_aliases()
    if ns.list:
        for name, words in pals.items():
            note = "  (same block as %s)" % aliases[name] if name in aliases else ""
            print("%-16s %3d colours  flags=%#06x%s"
                  % (name, words[0] & COUNT_MASK, words[0] & ~COUNT_MASK, note))
        print("\n%d palettes, %d of them extra names on a shared block"
              % (len(pals), len(aliases)))
        return 0
    text = render_c()
    if ns.out:
        pathlib.Path(ns.out).write_text(text)
        print("palettes: %d from IMGPAL.ASM and WRESPAL.ASM" % len(pals))
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
