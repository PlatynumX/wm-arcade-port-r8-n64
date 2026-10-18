#!/usr/bin/env python3
"""Extract DCSSOUND.ASM's per-wrestler sound tables -- SOUND.H's WRSNDX.

Every wrestler grunts, yells and gets hurt in his own voice, and the
mechanism is three layers deep:

  1. WRSNDX (SOUND.H:184) looks a move up in MASTER_SOUND_TABLE, a
     9 x (LAST_MOVE+1) word grid indexed [WRESTLERNUM][move]. The source's
     own comment on the layout: "each row contains the triple_sound_table
     indices of the four sound calls associated with a given move. the
     first two are the noises a wrestler makes when he throws the move,
     and the second two are the noises he makes when he's hit with the
     move."

  2. A NEGATIVE entry means DEFLT (8000h) -- "if the value is DEFLT, the
     value will be read from DEFAULT_SOUND_TABLE instead of the
     wrestler's custom table". A zero means no sound at all, on either
     table. The macro tests them in that order: `jrz DONE?` then
     `jrp OKAY?`.

  3. What comes out goes to table_sound (DCSSOUND.ASM:1330), which is NOT
     just triple_sound: `btst 12,a0` -- an index with bit 12 set is an
     index into #random_sound_tables instead, and the real sound is drawn
     from that sub-table with RNDRNG0. Almost every per-wrestler entry has
     that bit set, so the voices are randomised per hit rather than fixed.

Each random sub-table is `.word count, v0, v1, ... vcount` where count is
RNDRNG0's INCLUSIVE maximum, exactly like the announcer tables -- so a
sub-table written `.word 3,1Bh,1Bh,1Bh,0B2h` has four entries, not three.
"""
from __future__ import annotations
import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim  # noqa: E402

SRC = wlanim.ORIG / "DCSSOUND.ASM"
SOUND_H = wlanim.ORIG / "SOUND.H"

# SOUND.H:124 LAST_MOVE equ YELL_THROW, and WRSNDXI strides by LAST_MOVE+1.
MOVES_PER_WRESTLER = 59
WRESTLERS = 9
# DCSSOUND.ASM:1083 `DEFLT .equ 8000h` -- a negative word, which is how the
# macro's `jrp` tells it apart from a real index.
DEFLT = 0x8000
# table_sound's `btst 12,a0`.
RANDOM_BIT = 0x1000

WORD_RE = re.compile(r"^\s*\.WORD\s+(.+)$", re.I)
LONG_RE = re.compile(r"^\s*\.LONG\s+(.+)$", re.I)
EQU_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*)\s+equ\s+(\S+)\s*$", re.I)


def _lines() -> list[str]:
    return [wlanim.strip_comment(r).rstrip()
            for r in SRC.read_text(errors="replace").splitlines()]


def _num(tok: str) -> int:
    t = tok.strip()
    if t.upper() == "DEFLT":
        return DEFLT
    if re.fullmatch(r"-?[0-9]+", t):
        return int(t)
    if re.fullmatch(r"[0-9A-Fa-f]+[hH]", t):
        return int(t[:-1], 16)
    raise ValueError(f"unresolved sound-table value {tok!r}")


def _label_index(lines: list[str], name: str) -> int:
    for i, l in enumerate(lines):
        if l.strip() == name:
            return i
    raise ValueError(f"no label {name} in DCSSOUND.ASM")


LABEL_RE = re.compile(r"^[#A-Za-z_][A-Za-z0-9_]*\s*$")


def _words_after(lines: list[str], start: int, want: int | None) -> list[int]:
    """Read `.WORD` operands after a label, stopping at the next label.

    Labels ALIAS here: `#doink_kick_a`, `#doink_knee_a` and `#doink_slam_a`
    are stacked on one another with a single `.word` line under all three,
    so Doink's kick, knee and slam draw from the same three sounds. A run
    of bare labels before the data is therefore skipped rather than
    treated as the end of the table.
    """
    out: list[int] = []
    for q in range(start + 1, len(lines)):
        l = lines[q]
        if not l.strip():
            continue
        m = WORD_RE.match(l)
        if not m:
            if not out and LABEL_RE.match(l):
                continue                # an alias of the same table
            break                       # a real end
        for part in m.group(1).split(","):
            out.append(_num(part))
            if want is not None and len(out) == want:
                return out
    return out


def move_names() -> dict[int, str]:
    """SOUND.H's move equates, so the generated table is readable.

    Only the move block counts. SOUND.H also carries WRESTLERNUM values
    (`W_BRET equ 0`) and other small constants, and taking every equate
    with a small value would label move 0 "W_BRET". The block runs from
    PUNCH_T1 to YELL_THROW, which is what LAST_MOVE is defined as.

    The four BIGBOOT_* equates inside it are commented out -- nothing
    refers to them by name -- but their rows are still in both tables, so
    the gap is left unnamed rather than closed up.
    """
    out: dict[int, str] = {}
    started = False
    for raw in SOUND_H.read_text(errors="replace").splitlines():
        m = EQU_RE.match(wlanim.strip_comment(raw))
        if not m:
            continue
        name, val = m.group(1), m.group(2)
        if name.upper() == "PUNCH_T1":
            started = True
        if not started:
            continue
        if not re.fullmatch(r"[0-9]+", val):
            continue
        v = int(val)
        if 0 <= v < MOVES_PER_WRESTLER and v not in out:
            out[v] = name
        if name.upper() == "YELL_THROW":
            break
    return out


def default_table() -> list[int]:
    lines = _lines()
    rows = _words_after(lines, _label_index(lines, "DEFAULT_SOUND_TABLE"),
                        MOVES_PER_WRESTLER)
    if len(rows) != MOVES_PER_WRESTLER:
        raise ValueError(f"DEFAULT_SOUND_TABLE: {len(rows)} words, "
                         f"expected {MOVES_PER_WRESTLER}")
    return rows


def master_table() -> list[list[int]]:
    lines = _lines()
    flat = _words_after(lines, _label_index(lines, "MASTER_SOUND_TABLE"),
                        MOVES_PER_WRESTLER * WRESTLERS)
    if len(flat) != MOVES_PER_WRESTLER * WRESTLERS:
        raise ValueError(f"MASTER_SOUND_TABLE: {len(flat)} words, expected "
                         f"{MOVES_PER_WRESTLER * WRESTLERS}")
    return [flat[i * MOVES_PER_WRESTLER:(i + 1) * MOVES_PER_WRESTLER]
            for i in range(WRESTLERS)]


def random_tables() -> list[dict]:
    """table_sound's #random_sound_tables, in pointer order."""
    lines = _lines()
    start = _label_index(lines, "#random_sound_tables")
    names: list[str] = []
    for q in range(start + 1, len(lines)):
        l = lines[q]
        if not l.strip():
            continue
        m = LONG_RE.match(l)
        if not m:
            break
        names.append(m.group(1).strip())

    out = []
    for n in names:
        i = _label_index(lines, n)
        words = _words_after(lines, i, None)
        if not words:
            raise ValueError(f"{n}: empty random sound table")
        last_index, entries = words[0], words[1:]
        # The count is RNDRNG0's inclusive maximum, so it must be inside
        # the entries that follow it.
        if last_index < 0 or last_index >= len(entries):
            raise ValueError(f"{n}: count {last_index} but {len(entries)} "
                             f"entries follow it")
        out.append({"name": n, "last_index": last_index, "entries": entries})
    return out


def render_c() -> str:
    names = move_names()
    default = default_table()
    master = master_table()
    rnd = random_tables()

    def w(v: int) -> str:
        return f"0x{v:04X}"

    out = ["/* Auto-generated by tools/wlwrsnd.py from the original",
           "   DCSSOUND.ASM per-wrestler sound tables -- do not edit. */",
           '#include "wm/wrestler_sound_tables.h"',
           ""]

    out.append("/* DEFAULT_SOUND_TABLE: the row used wherever a wrestler's")
    out.append("   own entry is DEFLT (8000h). */")
    out.append("const uint16_t wm_wrsnd_default[WM_WRSND_MOVES] = {")
    for i, v in enumerate(default):
        out.append(f"    {w(v)},   /* {i:2d} {names.get(i, '(unnamed)')} */")
    out += ["};", ""]

    out.append("/* MASTER_SOUND_TABLE, in WRESTLERNUM order. Slot 7 is Adam")
    out.append("   Bomb, the wrestler who was cut. */")
    out.append("const uint16_t wm_wrsnd_master[WM_WRSND_WRESTLERS]"
               "[WM_WRSND_MOVES] = {")
    for wi, row in enumerate(master):
        out.append(f"    {{ /* wrestler {wi} */")
        for i in range(0, MOVES_PER_WRESTLER, 8):
            out.append("        " +
                       " ".join(w(v) + "," for v in row[i:i + 8]))
        out.append("    },")
    out += ["};", ""]

    out.append("/* table_sound's #random_sound_tables: an index with bit 12")
    out.append("   set names one of these, and RNDRNG0 draws a row from it. */")
    for t in rnd:
        ident = t["name"].lstrip("#")
        out.append(f"static const uint16_t rnd_{ident}[] = {{")
        out.append("    " + " ".join(w(v) + "," for v in t["entries"]))
        out += ["};"]
    out.append("")
    out.append("const wm_wrsnd_random wm_wrsnd_random_tables[] = {")
    for i, t in enumerate(rnd):
        ident = t["name"].lstrip("#")
        out.append(f'    {{ "{t["name"]}", rnd_{ident}, {t["last_index"]}, '
                   f"sizeof(rnd_{ident}) / sizeof(rnd_{ident}[0]) }},"
                   f"   /* 0x{i:02X} */")
    out += ["};", "const size_t wm_wrsnd_random_count =",
            "    sizeof(wm_wrsnd_random_tables) / "
            "sizeof(wm_wrsnd_random_tables[0]);",
            ""]
    return "\n".join(out) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out")
    ap.add_argument("--list", action="store_true")
    args = ap.parse_args()
    if args.list:
        names = move_names()
        master = master_table()
        default = default_table()
        rnd = random_tables()
        print(f"{MOVES_PER_WRESTLER} moves x {WRESTLERS} wrestlers, "
              f"{len(rnd)} random sub-tables")
        for i in range(MOVES_PER_WRESTLER):
            row = " ".join(f"{master[wnum][i]:04X}" for wnum in range(WRESTLERS))
            print(f"  {i:2d} {names.get(i, '(unnamed)'):16s} "
                  f"dflt {default[i]:04X}  {row}")
        return 0
    text = render_c()
    if args.out:
        pathlib.Path(args.out).write_text(text)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
