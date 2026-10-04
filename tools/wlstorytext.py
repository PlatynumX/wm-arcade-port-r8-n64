#!/usr/bin/env python3
"""The compressed text: AWARD.ASM's MK3 tips and the ending stories.

AWARD.ASM:3056 decompress_string is eleven instructions and unpacks
every piece of long text in the game:

    setf 6,0,0          ; field size = SIX BITS
  #loop
    move *a0+,a2        ; one 6-bit field, bit pointer advanced by 6
    jrz  #done          ; a zero field ends the string
    addi 01fh,a2        ; and every other value is the character
    movb a2,*a1         ;   minus 1Fh
    addk 8,a1
    jruc #loop

So the alphabet is 0x20-0x5F -- space through underscore, which is
uppercase ASCII and punctuation and nothing else -- packed four
characters to every three bytes, least significant bits first. That
is the whole compressor, and it buys about 25%.

What it unpacks is worth having: the two Mortal Kombat 3 tips
WrestleMania hides behind a win streak, and the eight wrestlers'
ending stories -- 1,889 lines of source across BAMST.H, BRETST.H,
DOINKST.H, LEXST.H, RAZORST.H, SHAWNST.H, TAKERST.H and YOKOST.H,
none of it readable without running the decompressor.

Every blob in those files carries its own plaintext in a comment
directly above it, put there by whatever tool Midway compressed them
with. This reader decodes the bytes and CHECKS the answer against
that comment, refusing on any mismatch -- so the decompressor is
verified against 100-odd known answers rather than asserted.
"""
import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim            # noqa: E402

STORY_FILES = ("BAMST.H", "BRETST.H", "DOINKST.H", "LEXST.H",
               "RAZORST.H", "SHAWNST.H", "TAKERST.H", "YOKOST.H")
STORY_DISPATCH_FILE = "STORIES.ASM"
STORY_DISPATCH = "#wrestler_stories_table"

_COMMENT = re.compile(r'^;\s*Compressed string\s+"(.*)"\s*$', re.I)
_LABEL = re.compile(r"^(#?[A-Za-z_]\w*)\s*$")
_BYTES = re.compile(r"^\.byte\s+(.+)$", re.I)
_SUBR = re.compile(r"^SUBRP?\s+(#?[A-Za-z_]\w*)\s*$", re.I)
_LONG = re.compile(r"^\.long\s+(.+)$", re.I)
_BYTE_VAL = re.compile(r"^0?([0-9A-Fa-f]+)h$|^(\d+)$")


class Refuse(Exception):
    """Something this reader does not understand. Never guessed at."""


def decompress(data):
    """AWARD.ASM:3056 decompress_string, exactly.

    A little-endian bit stream read six bits at a time; a zero field
    terminates and every other value is the character less 1Fh.
    """
    bits = 0
    have = 0
    out = []
    for b in data:
        bits |= b << have
        have += 8
        while have >= 6:
            v = bits & 0x3f
            bits >>= 6
            have -= 6
            if v == 0:
                return "".join(out)
            out.append(chr(v + 0x1f))
    # `jrz #decompress_done` never fired: the blob has no terminator.
    return "".join(out)


def _byte(tok):
    m = _BYTE_VAL.match(tok.strip())
    if not m:
        raise Refuse("byte %r" % tok)
    return int(m.group(1), 16) if m.group(1) else int(m.group(2))


def read_strings(path):
    """{label: (plaintext, decoded)} for one file's compressed blobs.

    The two files put the plaintext comment on opposite sides of the
    label -- the story headers write it above, AWARD.ASM writes it
    below the label and its JAM_STR -- so the comment arms the blob
    in hand whichever side it falls on, and a blob is only closed
    when bytes have actually been collected.
    """
    out = {}
    label = None
    pending = None
    data = []

    def flush():
        nonlocal label, pending, data
        if label is not None and data:
            got = decompress(bytes(data))
            if pending is not None and got != pending:
                raise Refuse("%s: decoded %r, source says %r"
                             % (label, got, pending))
            out[label] = (pending, got)
            label, pending, data = None, None, []
            return True
        return False

    for raw in path.read_text(errors="replace").splitlines():
        m = _COMMENT.match(raw.strip())
        if m:
            flush()
            pending = m.group(1)
            continue
        line = wlanim.strip_comment(raw)
        if not line:
            continue
        m = _LABEL.match(line) or _SUBR.match(line)
        if m:
            flush()
            label, data = m.group(1), []
            continue
        m = _BYTES.match(line)
        if m:
            # Only a blob a plaintext comment has armed is a
            # compressed string. AWARD.ASM is full of other .byte
            # tables -- icon lists with symbolic operands -- and
            # this is what keeps them out rather than a guess about
            # which labels look like text.
            if label is None or pending is None:
                continue
            for tok in m.group(1).split(","):
                data.append(_byte(tok))
            continue
        if line.lower().startswith(".even"):
            continue
        # A .long table or another directive ends a blob that has
        # started; anything before the bytes (a JAM_STR header) does
        # not, because it is part of the label's own preamble.
        flush()
    flush()
    return out


def _long_list(lines, at):
    """The `.long` rows following index `at`, to a 0 or a non-.long."""
    out = []
    for raw in lines[at + 1:]:
        line = wlanim.strip_comment(raw)
        if not line:
            continue
        m = _LONG.match(line)
        if not m:
            break
        name = m.group(1).split(";")[0].strip()
        if name == "0":
            break
        out.append(name)
    return out


def _find_label(lines, name):
    """The index of `name`'s definition, bare or behind SUBR/SUBRP."""
    for i, raw in enumerate(lines):
        line = wlanim.strip_comment(raw)
        if not line:
            continue
        m = _LABEL.match(line) or _SUBR.match(line)
        if m and m.group(1) == name:
            return i
    raise Refuse("no label %r" % name)


def read_stories():
    """The nine roster slots' stories, as decoded lines.

    STORIES.ASM's `#story_tbl` is nine longs -- one per roster slot,
    with Adam Bomb given Doink's, the same hand-me-down the animation
    dispatch tables make. Each names a table of stories, each story a
    table of lines, each line a compressed blob.
    """
    text = {}
    lines_by_file = {}
    for name in STORY_FILES:
        path = wlanim.ORIG / name
        text.update(read_strings(path))
        lines_by_file[name] = path.read_text(errors="replace").splitlines()

    disp_lines = (wlanim.ORIG / STORY_DISPATCH_FILE).read_text(
        errors="replace").splitlines()
    at = _find_label(disp_lines, STORY_DISPATCH)
    slots = _long_list(disp_lines, at)
    if len(slots) != 9:
        raise Refuse("%d story slots, expected 9" % len(slots))

    def resolve(label):
        for name, ls in lines_by_file.items():
            try:
                return ls, _find_label(ls, label)
            except Refuse:
                continue
        raise Refuse("no definition for %r" % label)

    out = []
    for slot, tbl in enumerate(slots):
        ls, at2 = resolve(tbl)
        stories = []
        for story_tbl in _long_list(ls, at2):
            ls3, at3 = resolve(story_tbl)
            story = []
            for line_label in _long_list(ls3, at3):
                if line_label not in text:
                    raise Refuse("line %r has no text" % line_label)
                story.append((line_label, text[line_label][1]))
            stories.append((story_tbl, story))
        out.append((slot, tbl, stories))
    return out


def read_mk3_tips():
    """AWARD.ASM's two Mortal Kombat 3 tips, three lines each."""
    text = read_strings(wlanim.ORIG / "AWARD.ASM")
    tips = []
    for n in (1, 2):
        lines = []
        for k in (1, 2, 3):
            label = "#mk3_tip%d_str%d" % (n, k)
            if label not in text:
                raise Refuse("no %s" % label)
            lines.append(text[label][1])
        tips.append(lines)
    return tips


_MK_IMAGE = re.compile(r"^\.long\s+(I_\w+)", re.I)


def read_mk3_codes():
    """The two Mortal Kombat 3 button codes the tips tell you to enter.

    `#mk_code1_1` to `_6` and `#mk_code2_1` to `_6` are object
    records -- x, y, z, image, DMA flags, class -- laid out
    twenty-four pixels apart across the screen. The IMAGE in each is
    the icon, and the six of them in order are the code.
    """
    lines = (wlanim.ORIG / "AWARD.ASM").read_text(
        errors="replace").splitlines()
    out = []
    for n in (1, 2):
        code = []
        for k in range(1, 7):
            at = _find_label(lines, "#mk_code%d_%d" % (n, k))
            image = None
            for raw in lines[at + 1:at + 8]:
                line = wlanim.strip_comment(raw)
                if not line:
                    continue
                m = _MK_IMAGE.match(line)
                if m:
                    image = m.group(1)
                    break
                if _LABEL.match(line) or _SUBR.match(line):
                    break
            if image is None:
                raise Refuse("#mk_code%d_%d has no image" % (n, k))
            code.append(image)
        out.append(code)
    return out


def _c_string(s):
    return '"%s"' % s.replace("\\", "\\\\").replace('"', '\\"')


_HEAD = """/* Auto-generated by tools/wlstring.py -- do not edit.

   AWARD.ASM:3056 decompress_string unpacks every piece of long text
   in the game from a six-bit stream. This is what it unpacks: the
   two Mortal Kombat 3 tips, and the eight wrestlers' ending stories
   out of BAMST.H and its seven siblings.

   Each blob carries its own plaintext in a source comment, and the
   reader checks the decode against it, so these strings are
   verified rather than transcribed. */
#include "wm/arcade/wm_arcade_story.h"

"""


def emit_c(stories, tips, codes):
    out = [_HEAD]

    out.append("const char *const wm_mk3_tip_lines[2]"
               "[WM_MK3_TIP_LINES] = {\n")
    for tip in tips:
        out.append("    { %s },\n" % ", ".join(_c_string(l) for l in tip))
    out.append("};\n\n")

    out.append("/* The icon sequence each tip tells you to enter. */\n")
    out.append("const char *const wm_mk3_code_icons[2]"
               "[WM_MK3_CODE_ICONS] = {\n")
    for code in codes:
        out.append("    { %s },\n" % ", ".join(_c_string(c) for c in code))
    out.append("};\n\n")

    # Slot 7 (Adam Bomb) shares Doink's table, exactly as the
    # animation dispatch lists hand him Doink's, so each distinct
    # table is emitted once and referenced twice.
    seen = []
    for _slot, tbl, sts in stories:
        if tbl in seen:
            continue
        seen.append(tbl)
        for si, (_name, lines) in enumerate(sts):
            out.append("static const char *const %s_%d[] = {\n" % (tbl, si))
            for _label, txt in lines:
                out.append("    %s,\n" % _c_string(txt))
            out.append("};\n")
    out.append("\n")

    for tbl in seen:
        sts = next(s for _sl, t, s in stories if t == tbl)
        if not sts:
            continue
        out.append("static const wm_story_t %s_list[] = {\n" % tbl)
        for si, (_name, lines) in enumerate(sts):
            out.append("    { %s_%d, %d },\n" % (tbl, si, len(lines)))
        out.append("};\n")
    out.append("\n")

    out.append("const wm_story_set_t wm_wrestler_stories"
               "[WM_STORY_SLOTS] = {\n")
    for slot, tbl, sts in stories:
        if sts:
            out.append("    { \"%s\", %s_list, %d },\n"
                       % (tbl, tbl, len(sts)))
        else:
            out.append("    { \"%s\", 0, 0 },\n" % tbl)
    out.append("};\n")
    return "".join(out)


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("--out-c")
    ap.add_argument("--census", action="store_true")
    ns = ap.parse_args(argv)

    stories = read_stories()
    tips = read_mk3_tips()
    codes = read_mk3_codes()

    if ns.census:
        for tip, code in zip(tips, codes):
            print("MK3 tip: %s" % " ".join(tip))
            print("    code: %s" % " ".join(code))
        total = 0
        for slot, tbl, sts in stories:
            n = sum(len(lines) for _n, lines in sts)
            total += n
            print("slot %d  %-16s %d stor%s, %d lines"
                  % (slot, tbl, len(sts), "y" if len(sts) == 1 else "ies", n))
        print("%d story lines in total" % total)
        return 0
    if ns.out_c:
        pathlib.Path(ns.out_c).write_text(emit_c(stories, tips, codes))
        print("wrote the stories and the MK3 tips -> %s" % ns.out_c)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
