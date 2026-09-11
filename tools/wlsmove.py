#!/usr/bin/env python3
"""The special-move watchdogs, read out of the source.

WRESTLE2.ASM:4058 init_smoves creates one SMOVE_PID process per entry
in a wrestler's smove table, and 79 entries across the eight tables
name 65 distinct routines. Three were translated by hand
(und_finish_move1, std_walk_fast, std_taunt, in
src/core/arcade/wm_arcade_smove.c); this reads the rest.

They are not 62 unrelated routines. 53 of them are ONE routine written
out 53 times with different constants -- the "head-hold" family, whose
shape is:

    #lp0  SLEEPK 1
    #lp   PLYRMODE is MODE_HEADHOLD  -> #cont
          PLYRMODE is MODE_HEADHELD  -> #cont
          otherwise                  -> #lp0
    #cont clr a11
          WAITSWITCH_DWN  <a>            -- no deadline
          movi #TIMEOUT,a11
          WAITSWITCH_DWN  <b>
          WAITSWITCH_DWN  <c>            -- always a button
          PLYRMODE is MODE_HEADHOLD  -> #slam   (I am doing it)
          PLYRMODE is MODE_HEADHELD  -> reversal (he is doing it to me)
          otherwise                  -> #lp0
          [I_WILL_DIE and IMMOBILIZE_TIME refuse either way]
    reversal  DO_REVERSAL, DO_REVERSAL_MESS, target WHOHITME
    #slam     a bonus message with its own index, target WHOIHIT
    both      the target's IMMOBILIZE_TIME = 15, FIND_AND_KILL_ENDLESS,
              SPECIAL_MOVE_ADDR = <the move's animation>, SLEEPK 20,
              back to #lp

So what varies between them is six things: the three switch steps, the
TIMEOUT, the bonus index, the animation, and whether the routine has a
reversal path at all. Those are what this extracts. Transcribing 53
copies of the same routine by hand is exactly the kind of work this
tree generates instead, and a hand copy could not be checked against
the source afterwards.

A routine that does not match the shape is REFUSED rather than
approximated -- see the `refused` list in `--census`. Twenty are
refused, in four groups, and each is a different routine rather than a
variant of this one:

  6  the "charge" monitors (hrt_charge_flying_kick and friends), which
     count a HELD button up to a threshold and then check a list of
     mode guards on both wrestlers -- no input sequence at all.
  8  the grab_toss_air family, one per wrestler. They pick their
     animation with two FACE24 macros behind a distance test, so the
     move is a four-way choice (near/far x facing) rather than the one
     animation, or one facing pair, this row can hold.
  2  und_spirit_pull / und_spirit_push, which live in SPECIAL.ASM and
     are a different thing entirely.
  4  und_choke_slide, hrt_roll_uppercut, shn_hdhold_butts and
     bam_hdhold_pogo, each of which departs from the shape in its own
     way.

One thing to be careful of, and it cost two wrong readings here: these
routines are full of COMMENTED-OUT gates. YOKO.ASM:759 has a
GETUP_TIME test commented out for two of Yokozuna's moves, and
SHAWN.ASM:1538 has CHECK_COMBO_GO commented out for one of Shawn's.
Reading the raw text rather than the stripped text counts all three as
live, and the result is three moves that refuse in this port and work
in the arcade. Everything below reads stripped lines.
"""
import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim            # noqa: E402
import wlwrestlertbl     # noqa: E402

HAND_WRITTEN = ("und_finish_move1", "std_walk_fast", "std_taunt")

_SUBR = re.compile(r"^\s+SUBRP?\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:;.*)?$")
_COL0 = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)\s*(?:;.*)?$")
_WAIT = re.compile(
    r"^\s*WAITSWITCH_DWN\s+([^,]+),([^,]*),\s*(\S+)", re.I)
_TIMEOUT = re.compile(r"^#TIMEOUT\s+\.equ\s+(\d+)", re.I)
# NOTE: wlanim.strip_comment also strips LEADING whitespace, so the
# instruction patterns above anchor on \s* rather than \s+. Anchoring
# on \s+ silently matches nothing and every routine is refused.
_BONUS = re.compile(r"^\s*mov[ki]\s+(\w+),a10\b", re.I)
_SMADDR = re.compile(
    r"^\s*movi\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*a\d+\s*$", re.I)
# `FACE24 und,snapmirror_anim` -- the macro picks between the wrestler's
# `_2_` and `_4_` forms by facing and leaves the answer in a0, so the
# routine stores a0 rather than a name it loaded. The eight
# grab_toss_air monitors all select their animation this way, and the
# pair is the answer, not one of them.
_FACE24 = re.compile(
    r"^\s*FACE24\s+([A-Za-z_][A-Za-z0-9_]*)\s*,\s*"
    r"([A-Za-z_][A-Za-z0-9_]*)", re.I)


def smove_names():
    """Every distinct routine an smove table names, table order."""
    out = []
    for entry in wlwrestlertbl.smove_tables():
        if not entry:
            continue
        for label in entry[1]:
            if label not in out:
                out.append(label)
    return out


def bodies():
    """{name: (file, line, [lines])} for every smove-table routine.

    A body runs from its definition to the next NON-LOCAL definition in
    the same file. Local labels (`#lp0`) are not boundaries: they are
    the routine's own structure, and treating them as boundaries cuts
    every one of these routines off after two lines.
    """
    wanted = set(smove_names())
    found = {}
    for path in wlanim.linked_files():
        lines = path.read_text(errors="replace").splitlines()
        marks = []
        for i, line in enumerate(lines):
            m = _SUBR.match(line) or _COL0.match(line)
            if m:
                marks.append((i, m.group(1)))
        for k, (i, name) in enumerate(marks):
            if name in wanted and name not in found:
                j = marks[k + 1][0] if k + 1 < len(marks) else len(lines)
                found[name] = (path.name, i + 1, lines[i:j])
    return found


def _operand(text):
    """A WAITSWITCH_DWN operand as the assembler sees it.

    They are GAME.EQU names, sometimes ORed: `J_DOWN|J_UP`. An empty
    mask is the literal 0 the macro is given.
    """
    text = text.split(";")[0].strip()
    if not text or text == "0":
        return []
    return [p.strip() for p in text.split("|") if p.strip()]


def parse_hdhold(name, file, line, body):
    """The head-hold family, or None when this is not one of them.

    Every check here is a refusal, not a repair: a routine that is
    nearly this shape is still not this shape.
    """
    raw = "\n".join(body)
    stripped = [wlanim.strip_comment(l) for l in body]
    # EVERY test below reads the stripped text, never the raw. These
    # routines are full of commented-out gates -- YOKO.ASM:759's
    # `;	move *a8(GETUP_TIME),a0` is one, and reading it as live
    # makes two of Yokozuna's moves refuse when the shipped game
    # allows them. `raw` is kept only so a caller can see the
    # original if it wants to.
    text = "\n".join(l for l in stripped if l)
    del raw

    # The gate. Both modes, and the one that decides who is doing what
    # to whom at the end.
    if "MODE_HEADHOLD" not in text or "MODE_HEADHELD" not in text:
        return None
    # A charge monitor guards on the same modes but counts a held
    # button instead of reading a sequence.
    if "CHARGE_TIME" in text:
        return None

    waits = []
    for raw in stripped:
        m = _WAIT.match(raw)
        if m:
            waits.append((_operand(m.group(1)), _operand(m.group(2))))
    if len(waits) != 3:
        return None

    # The third step is always a button: the two stick inputs charge
    # it up and the button fires it.
    if not any(s.startswith("B_") for s in waits[2][0]):
        return None

    timeout = None
    for raw in stripped:
        m = _TIMEOUT.match(raw)
        if m:
            timeout = int(m.group(1))
            break
    if timeout is None:
        return None

    # `movi <anim>,a14 / move a14,*a8(SPECIAL_MOVE_ADDR),L` -- the move
    # itself. Take the load that immediately precedes the store.
    anim = None
    facing_pair = None
    for i, raw in enumerate(stripped):
        if "SPECIAL_MOVE_ADDR" not in raw or "move" not in raw.lower():
            continue
        for j in range(i - 1, max(i - 8, -1), -1):
            m = _SMADDR.match(stripped[j])
            if m:
                anim = m.group(1)
                break
            m = _FACE24.match(stripped[j])
            if m:
                # FACE24's two halves, spelled the way the roster
                # tables already name them.
                facing_pair = ("%s_2_%s" % (m.group(1), m.group(2)),
                               "%s_4_%s" % (m.group(1), m.group(2)))
                break
        if anim or facing_pair:
            break
    if not anim and not facing_pair:
        return None

    # `movk <n>,a10 / CREATE MESSAGE_PID,BONUS_MESS` -- the bonus
    # message's own index. Absent in the few that award none.
    bonus = None
    for i, raw in enumerate(stripped):
        if "BONUS_MESS" not in raw:
            continue
        for j in range(i - 1, max(i - 4, -1), -1):
            m = _BONUS.match(stripped[j])
            if m:
                try:
                    bonus = int(m.group(1), 0)
                except ValueError:
                    bonus = None
                break
        break

    reversal = "DO_REVERSAL" in text

    # Two extra gates appear before the sequence on some of them, and
    # both refuse the whole monitor rather than one step. Missing
    # either would let 19 of these fire when the source would not.
    #
    #   `calla CHECK_COMBO_GO / jrlt #lp0` -- fifteen combo moves are
    #   only available while a combo is running.
    #   `move *a8(GETUP_TIME),a0 / jrnz #lp0` -- four of Shawn's
    #   refuse while he is getting up.
    head = text.split("WAITSWITCH_DWN")[0]
    needs_combo = "CHECK_COMBO_GO" in head
    needs_up = "GETUP_TIME" in head

    return {
        "name": name,
        "file": file,
        "line": line,
        "steps": waits,
        "timeout": timeout,
        "anim": anim,
        "facing_pair": facing_pair,
        "bonus": bonus,
        "reversal": reversal,
        "needs_combo": needs_combo,
        "needs_up": needs_up,
    }


def extract():
    """{parsed: [...], refused: [(name, file, line)]}"""
    parsed, refused = [], []
    found = bodies()
    for name in smove_names():
        if name in HAND_WRITTEN:
            continue
        if name not in found:
            refused.append((name, "?", 0))
            continue
        file, line, body = found[name]
        row = parse_hdhold(name, file, line, body)
        if row:
            parsed.append(row)
        else:
            refused.append((name, file, line))
    return {"parsed": parsed, "refused": refused}


_C_HEAD = """/* Auto-generated by tools/wlsmove.py: the head-hold special-move
   watchdogs, read out of the source rather than transcribed.

   Each row is one routine's six variable parts -- three switch steps,
   the timeout, the bonus-message index and the animation -- around a
   body they all share. See wm/arcade/wm_arcade_smove.h. */
#include "wm/arcade/wm_arcade_smove.h"

const wm_smove_hdhold_t wm_smove_hdhold[] = {
"""


def emit_c(data):
    out = [_C_HEAD]
    for r in sorted(data["parsed"], key=lambda x: x["name"]):
        def word(parts):
            return " | ".join("WM_" + p for p in parts) if parts else "0"
        steps = ", ".join(
            "{ %s, %s }" % (word(s), word(m)) for s, m in r["steps"])
        out.append(
            '    { "%s", "%s", { %s }, %d, %d, %s, %s, %s, %s, %s },'
            '   /* %s:%d */\n'
            % (r["name"], r["file"], steps, r["timeout"],
               -1 if r["bonus"] is None else r["bonus"],
               "true" if r["reversal"] else "false",
               "true" if r["needs_combo"] else "false",
               "true" if r["needs_up"] else "false",
               ('"%s"' % r["anim"]) if r["anim"]
               else ('"%s"' % r["facing_pair"][0]),
               ('"%s"' % r["facing_pair"][1]) if r["facing_pair"] else "0",
               r["file"], r["line"]))
    out.append("};\n\n")
    out.append("const size_t wm_smove_hdhold_count =\n"
               "    sizeof(wm_smove_hdhold) / sizeof(wm_smove_hdhold[0]);\n")
    return "".join(out)


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("--out-c")
    ap.add_argument("--census", action="store_true")
    ns = ap.parse_args(argv)

    data = extract()
    if ns.census:
        print("%d head-hold monitors read, %d refused"
              % (len(data["parsed"]), len(data["refused"])))
        for name, file, line in data["refused"]:
            print("  refused  %-26s %s:%d" % (name, file, line))
        return 0
    if ns.out_c:
        pathlib.Path(ns.out_c).write_text(emit_c(data))
        print("wrote %d head-hold monitors -> %s"
              % (len(data["parsed"]), ns.out_c))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
