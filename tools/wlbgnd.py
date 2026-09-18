#!/usr/bin/env python3
"""The background modules the scroller culls, read out of BGNDTBL.ASM.

BAKGND.ASM's BGND_UD1 keeps the display list in step with the camera:
it deletes the background blocks that have scrolled off and adds the
ones that have scrolled on. What it walks is a MODULE LIST, and
WRESTLE.ASM:1615 hands it exactly one for a match --

    ring_mod
        .long   ringBMOD        ;wrestling ring
        .word   105,-450        ;x,y
        .long   0

-- so the arena is one module of 211 blocks placed at (105,-450).

Three levels of table, all here:

  the MODULE    `.word xsize,ysize,#blocks` then `.long BLKS,HDRS,PALS`;
  the BLOCKS    four words each -- palette and flags and Z packed into
                the first, X, Y, and a header index in the fourth
                whose top nibble carries four more bits of palette;
  the HEADERS   `.word w,h / .long address / .word dmactrl`, and it is
                the W and H the scroller needs, because a block's own
                record has no size in it. That indirection is the
                reason the two scan phases in bgnd_addmod look
                different: the cheap one avoids touching the header
                at all.

src/generated/bmod_tables.c already carries the block words for five
FRONTEND modules, emitted by tools/bmod_source.py through the asset
pipeline. It does not carry the ring, and it does not carry header
sizes at all. This is the source-data half the scroller needs, kept
separate rather than bolted onto the asset path.
"""
import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim            # noqa: E402

SOURCE = "BGNDTBL.ASM"
# WRESTLE.ASM:1615 `movi ring_mod,a0 / move a0,@BAKMODS,L`.
LIST_FILE = "WRESTLE.ASM"
LIST_LABEL = "ring_mod"

_LABEL = re.compile(r"^([A-Za-z_]\w*)\s*:?\s*$")
_WORD = re.compile(r"^\.word\s+(.+)$", re.I)
_LONG = re.compile(r"^\.long\s+(.+)$", re.I)
_NUM = re.compile(r"^-?(?:0?([0-9A-Fa-f]+)[Hh]|(\d+))$")

BLOCK_END = 0xFFFF


class Refuse(Exception):
    """Something this reader does not understand. Never guessed at."""


def _num(tok):
    tok = tok.split(";")[0].strip()
    neg = tok.startswith("-")
    if neg:
        tok = tok[1:]
    m = _NUM.match(tok)
    if not m:
        raise Refuse("number %r" % tok)
    v = int(m.group(1), 16) if m.group(1) else int(m.group(2))
    return -v if neg else v


def _lines(name):
    return (wlanim.ORIG / name).read_text(errors="replace").splitlines()


def _section(lines, label):
    """The directive lines from `label` to the next top-level label."""
    at = None
    for i, raw in enumerate(lines):
        line = wlanim.strip_comment(raw)
        if not line:
            continue
        m = _LABEL.match(line)
        if m and m.group(1) == label:
            at = i
            break
    if at is None:
        raise Refuse("no label %r" % label)
    out = []
    for raw in lines[at + 1:]:
        line = wlanim.strip_comment(raw)
        if not line:
            continue
        if _LABEL.match(line):
            break
        # A banner of asterisks between tables is not a directive.
        if re.match(r"^[#*;]?\*{4,}", line):
            break
        out.append(line)
    return out


def _values(section):
    """(kind, value) for each operand, kind being 'w' or 'l'."""
    out = []
    for line in section:
        m = _WORD.match(line)
        if m:
            for tok in m.group(1).split(","):
                out.append(("w", _num(tok)))
            continue
        m = _LONG.match(line)
        if m:
            for tok in m.group(1).split(","):
                tok = tok.split(";")[0].strip()
                try:
                    out.append(("l", _num(tok)))
                except Refuse:
                    out.append(("s", tok))     # a label
            continue
        raise Refuse("directive %r" % line)
    return out


def read_headers(label):
    """[(w, h)] -- `.word w,h / .long address / .word dmactrl`."""
    vals = _values(_section(_lines(SOURCE), label))
    out = []
    i = 0
    while i + 3 < len(vals) + 1:
        if i + 4 > len(vals):
            break
        w, h, addr, ctrl = vals[i], vals[i + 1], vals[i + 2], vals[i + 3]
        if w[0] != "w" or h[0] != "w" or addr[0] not in "ls" or ctrl[0] != "w":
            raise Refuse("%s: entry %d is %r" % (label, len(out),
                                                 (w, h, addr, ctrl)))
        out.append((w[1], h[1]))
        i += 4
    if i != len(vals):
        raise Refuse("%s: %d operands left over" % (label, len(vals) - i))
    return out


def read_blocks(label, count):
    """[(flags_word, x, y, hdr_word)] -- four words each."""
    vals = _values(_section(_lines(SOURCE), label))
    words = []
    for kind, v in vals:
        if kind != "w":
            raise Refuse("%s: a .long inside the block table" % label)
        words.append(v)
    out = []
    i = 0
    while i + 4 <= len(words):
        if words[i] == BLOCK_END:
            break
        out.append(tuple(words[i:i + 4]))
        i += 4
    if len(out) != count:
        raise Refuse("%s: %d blocks, the module says %d"
                     % (label, len(out), count))
    return out


def read_module(name):
    """One BMOD: its size, its blocks and its header sizes."""
    vals = _values(_section(_lines(SOURCE), name))
    if len(vals) < 6:
        raise Refuse("%s: short module record" % name)
    xs, ys, nb = (v for _k, v in vals[:3])
    labels = [v for k, v in vals[3:6] if k == "s"]
    if len(labels) != 3:
        raise Refuse("%s: expected BLKS, HDRS, PALS" % name)
    blocks = read_blocks(labels[0], nb)
    headers = read_headers(labels[1])
    for _f, _x, _y, hdr in blocks:
        idx = hdr & 0x0fff
        if idx >= len(headers):
            raise Refuse("%s: block header index %d, only %d headers"
                         % (name, idx, len(headers)))
    return {"name": name, "width": xs, "height": ys,
            "blocks": blocks, "headers": headers,
            "labels": labels}


def read_module_list():
    """WRESTLE.ASM's ring_mod: [(module name, x, y)], zero-terminated."""
    vals = _values(_section(_lines(LIST_FILE), LIST_LABEL))
    out = []
    i = 0
    while i < len(vals):
        kind, v = vals[i]
        if kind == "l" and v == 0:
            break
        if kind != "s":
            raise Refuse("%s: entry %d is not a module label" % (LIST_LABEL, i))
        if i + 2 >= len(vals):
            raise Refuse("%s: module %r has no position" % (LIST_LABEL, v))
        x, y = vals[i + 1], vals[i + 2]
        if x[0] != "w" or y[0] != "w":
            raise Refuse("%s: %r's position is not two words" % (LIST_LABEL, v))
        out.append((v, x[1], y[1]))
        i += 3
    if not out:
        raise Refuse("%s is empty" % LIST_LABEL)
    return out


_HEAD = """/* Auto-generated by tools/wlbgnd.py from BGNDTBL.ASM -- do not
   edit.

   The background modules BAKGND.ASM's scroller culls, and the header
   sizes it needs to cull them: a block's own record carries no width
   or height, only an index into its module's header table. See
   wm/arcade/wm_arcade_bgnd.h. */
#include "wm/arcade/wm_arcade_bgnd.h"

"""


def emit_c(modules, mlist):
    out = [_HEAD]
    for m in modules:
        out.append("/* %s: %d blocks, %d headers, %dx%d */\n"
                   % (m["name"], len(m["blocks"]), len(m["headers"]),
                      m["width"], m["height"]))
        out.append("static const wm_bgnd_block_t %s_blocks[] = {\n"
                   % m["name"])
        for f, x, y, hdr in m["blocks"]:
            out.append("    { 0x%04X, %5d, %5d, 0x%04X },\n" % (f, x, y, hdr))
        out.append("};\n\n")
        out.append("static const wm_bgnd_header_t %s_headers[] = {\n"
                   % m["name"])
        for w, h in m["headers"]:
            out.append("    { %4d, %4d },\n" % (w, h))
        out.append("};\n\n")

    out.append("const wm_bgnd_module_t wm_bgnd_modules[] = {\n")
    for m in modules:
        out.append('    { "%s", %d, %d, %s_blocks, %d, %s_headers, %d },\n'
                   % (m["name"], m["width"], m["height"], m["name"],
                      len(m["blocks"]), m["name"], len(m["headers"])))
    out.append("};\n\nconst size_t wm_bgnd_module_count =\n"
               "    sizeof(wm_bgnd_modules) / sizeof(wm_bgnd_modules[0]);\n\n")

    out.append("/* WRESTLE.ASM:1615 `movi ring_mod,a0 / move a0,@BAKMODS,L`\n"
               "   -- the one module list a match ever uses. */\n")
    out.append("const wm_bgnd_placed_t wm_bgnd_ring_mod[] = {\n")
    for name, x, y in mlist:
        idx = next(i for i, m in enumerate(modules) if m["name"] == name)
        out.append("    { &wm_bgnd_modules[%d], %d, %d },\n" % (idx, x, y))
    out.append("};\n\nconst size_t wm_bgnd_ring_mod_count =\n"
               "    sizeof(wm_bgnd_ring_mod) / sizeof(wm_bgnd_ring_mod[0]);\n")
    return "".join(out)


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("--out-c")
    ap.add_argument("--census", action="store_true")
    ns = ap.parse_args(argv)

    mlist = read_module_list()
    modules = [read_module(name) for name, _x, _y in mlist]

    if ns.census:
        for (name, x, y), m in zip(mlist, modules):
            print("%s at (%d,%d): %dx%d, %d blocks, %d headers"
                  % (name, x, y, m["width"], m["height"],
                     len(m["blocks"]), len(m["headers"])))
            xs = [b[1] for b in m["blocks"]]
            print("   block x range %d..%d, sorted=%s"
                  % (min(xs), max(xs), xs == sorted(xs)))
        return 0
    if ns.out_c:
        pathlib.Path(ns.out_c).write_text(emit_c(modules, mlist))
        print("wrote %d background module(s) -> %s" % (len(modules), ns.out_c))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
