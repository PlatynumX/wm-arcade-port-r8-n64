#!/usr/bin/env python3
"""DCSSOUND.ASM's triple_sndtab, read out of the source.

Every sound in the game is an index into one table. WRSND picks an
index out of a wrestler's own sound table (already generated, see
tools/wlwrsnd.py), the animation VM's sound opcodes carry one, and
every `calla triple_sound` in the tree passes one. This reads what
those indices MEAN.

DCSSOUND.ASM:235 `triple_sndtab` is 836 lines of

    .word   sp_smack|17,>80         ;  1 = face hit #0

and each row is two words:

  word 0  the PRIORITY in the high byte and the DURATION in the low
          one, written as one OR because the sp_* names at
          DCSSOUND.ASM:209-221 are all `N << 8`. So `sp_smack|17` is
          priority 16, duration 17 -- and a row of 0 is the "zero
          entry = skip" triple_sound tests for.
  word 1  the DCS sound call for CHANNEL ONE. Channels two, three and
          four use the next three consecutive values, which is why
          triple_sound does `inc a3` / `addk 2,a3` / `addk 3,a3` on
          its way to the board rather than looking anything up.

The table also carries six labels inside it, and they are not
decoration: ANNOUNCE_VOICE decides WHICH ANNOUNCER is speaking purely
by where the index falls between them, so the boundaries are data the
runtime needs.
"""
import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim            # noqa: E402

SOUND_FILE = "DCSSOUND.ASM"
START = "triple_sndtab"
END = "triple_end"

# The labels sitting inside the rows, in the order the table puts
# them. ANNOUNCE_VOICE compares an index against these and nothing
# else, so this order is the announcer layout.
LABELS = ("triple_sndtab", "announcer_start", "vince_end", "randy_end",
          "howards_end", "more_jerry", "wrestle_end", "triple_end")

_EQU = re.compile(r"^(sp_\w+)\s+equ\s+(\d+)\s*<<\s*8\b", re.I)
_LABEL = re.compile(r"^([A-Za-z_]\w*)\s*$")
_WORD = re.compile(r"^\.word\s+(.+)$", re.I)


class Refuse(Exception):
    """A row this reader does not understand. Never guessed at."""


def _lines():
    path = wlanim.ORIG / SOUND_FILE
    return path.read_text(errors="replace").splitlines()


def priorities():
    """{sp_name: group value}. DCSSOUND.ASM:209-221."""
    out = {}
    for raw in _lines():
        m = _EQU.match(wlanim.strip_comment(raw))
        if m:
            out[m.group(1)] = int(m.group(2)) << 8
    if not out:
        raise Refuse("no sp_* priority groups")
    return out


def _atom(part, groups):
    part = part.strip()
    if not part:
        raise Refuse("empty operand")
    if part in groups:
        return groups[part]
    if part.startswith(">"):
        return int(part[1:], 16)
    if re.fullmatch(r"\d+", part):
        return int(part)
    if re.fullmatch(r"[0-9A-Fa-f]+h", part):
        return int(part[:-1], 16)
    raise Refuse("operand %r" % part)


def _sum(text, groups):
    """`75-25` and friends -- a run of + and - over atoms."""
    tokens = re.split(r"([+-])", text)
    total = _atom(tokens[0], groups)
    for i in range(1, len(tokens) - 1, 2):
        term = _atom(tokens[i + 1], groups)
        total = total + term if tokens[i] == "+" else total - term
    return total


def _value(text, groups):
    """One .word operand: names and numbers ORed, with + and - inside.

    Seven rows are written `sp_losmack|75-25`, which raises a real
    question of precedence -- sp|(75-25), or (sp|75)-25? Both are
    computed and compared rather than one being assumed. They agree
    on every row in this table, because 75-25 never borrows across
    the byte boundary the OR is there to respect, and a row where
    they diverged would refuse instead of quietly picking one.
    """
    tight = 0
    for part in text.split("|"):
        tight |= _sum(part, groups)
    if "|" not in text or not re.search(r"[+-]", text):
        return tight

    head, _, tail = text.partition("|")
    parts = re.split(r"([+-])", tail)
    loose = _atom(head, groups) | _atom(parts[0], groups)
    for i in range(1, len(parts) - 1, 2):
        term = _atom(parts[i + 1], groups)
        loose = loose + term if parts[i] == "+" else loose - term
    if loose != tight:
        raise Refuse("precedence matters in %r: %#x vs %#x"
                     % (text, tight, loose))
    return tight


def read_table():
    """{'rows': [(pri, dur, call)], 'labels': {name: row index}}."""
    groups = priorities()
    lines = _lines()
    at = next((i for i, l in enumerate(lines)
               if _LABEL.match(wlanim.strip_comment(l) or "")
               and wlanim.strip_comment(l) == START), None)
    if at is None:
        raise Refuse("no %s" % START)

    rows = []
    labels = {}
    for raw in lines[at:]:
        line = wlanim.strip_comment(raw)
        if not line:
            continue
        m = _LABEL.match(line)
        if m:
            name = m.group(1)
            if name not in LABELS:
                raise Refuse("unexpected label %r inside the table" % name)
            # A label's position is the row it precedes.
            labels[name] = len(rows)
            if name == END:
                break
            continue
        m = _WORD.match(line)
        if not m:
            raise Refuse("line %r is neither a label nor a .word" % line)
        parts = [p.strip() for p in m.group(1).split(",")]
        if len(parts) != 2:
            raise Refuse("%d operands in %r" % (len(parts), line))
        head = _value(parts[0], groups)
        call = _value(parts[1], groups)
        pri = (head >> 8) & 0xff
        dur = head & 0xff
        # The source writes duration with `|`, so a value that did not
        # fit in the low byte would silently corrupt the priority. It
        # never does; this is the check that says so.
        if head and head >> 16:
            raise Refuse("row %d head %#x is wider than a word"
                         % (len(rows), head))
        rows.append((pri, dur, call))

    missing = [l for l in LABELS if l not in labels]
    if missing:
        raise Refuse("missing labels %r" % missing)
    return {"rows": rows, "labels": labels}


_HEAD = """/* Auto-generated by tools/wlsound.py from DCSSOUND.ASM:235
   triple_sndtab -- do not edit.

   One row per sound index: the priority and duration triple_sound
   arbitrates on, and the DCS call for channel one (channels two to
   four are the next three consecutive values). A row of all zeroes
   is the source's own "zero entry = skip". */
#include "wm/arcade/wm_arcade_sound.h"

const wm_sound_entry_t wm_sound_table[] = {
"""


def emit_c(data):
    out = [_HEAD]
    by_row = {}
    for name, row in data["labels"].items():
        by_row.setdefault(row, []).append(name)
    for i, (pri, dur, call) in enumerate(data["rows"]):
        for name in by_row.get(i, []):
            out.append("    /* --- %s --- */\n" % name)
        out.append("    { %3d, %3d, %5d },   /* %#05x */\n"
                   % (pri, dur, call, i))
    out.append("};\n\nconst size_t wm_sound_table_count =\n"
               "    sizeof(wm_sound_table) / sizeof(wm_sound_table[0]);\n\n")
    out.append("/* The announcer boundaries ANNOUNCE_VOICE compares "
               "against.\n   Row indices, in the order the table puts "
               "them. */\n")
    for name in LABELS:
        out.append("const size_t wm_sound_%s = %d;\n"
                   % (name, data["labels"][name]))
    return "".join(out)


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("--out-c")
    ap.add_argument("--census", action="store_true")
    ns = ap.parse_args(argv)

    data = read_table()
    if ns.census:
        print("%d sound entries" % len(data["rows"]))
        for name in LABELS:
            print("  %-16s row %d" % (name, data["labels"][name]))
        return 0
    if ns.out_c:
        pathlib.Path(ns.out_c).write_text(emit_c(data))
        print("wrote %d sound entries -> %s"
              % (len(data["rows"]), ns.out_c))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
