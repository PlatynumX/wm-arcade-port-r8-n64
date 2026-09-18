#!/usr/bin/env python3
"""The special-move watchdogs, read out of the source.

WRESTLE2.ASM:4058 init_smoves creates one SMOVE_PID process per entry
in a wrestler's smove table, and 79 entries across the eight tables
name 65 distinct routines. Five are translated by hand in
src/core/arcade/wm_arcade_smove.c; this reads the other sixty.

They are not sixty unrelated routines. Forty of them are ONE routine
written out forty times with different constants -- the "head-hold"
family, whose shape is:

    #lp0  SLEEPK 1
    #lp   PLYRMODE is MODE_HEADHOLD  -> #cont
          PLYRMODE is MODE_HEADHELD  -> #cont
          otherwise                  -> #lp0
    #cont clr a11
          WAITSWITCH_DWN  <a>            -- no deadline
          movi #TIMEOUT,a11
          WAITSWITCH_DWN  <b>
          WAITSWITCH_DWN  <c>            -- always a button
          PLYRMODE decides what the input just did
    reversal  DO_REVERSAL, DO_REVERSAL_MESS, target WHOHITME
    #slam     a bonus message with its own index, target WHOIHIT
    both      the target's IMMOBILIZE_TIME, FIND_AND_KILL_ENDLESS,
              SPECIAL_MOVE_ADDR = <the move's animation>, SLEEPK 20,
              back to #lp

and what varies is the three switch steps, the TIMEOUT, the bonus
index, the animation, the victim's IMMOBILIZE_TIME, which of the two
modes the gate admits, what a wrestler whose head is HELD gets, two
extra gates, and whether the routine targets anybody at all. Every
one of those is a column here because every one of them really does
differ between rows -- IMMOBILIZE_TIME alone takes five values, one
of which is "the write is commented out, so nobody is pinned".
Transcribing forty copies of one routine by hand is exactly the kind
of work this tree generates instead, and a hand copy could not be
checked against the source afterwards.

A routine that does not match the shape is handed to the three other
family readers below, and only refused if none of them recognises it
either. Nothing is approximated: an instruction no reader knows
refuses the routine rather than being dropped, because a test that
quietly failed to translate is a move that fires in this port and
would not in the arcade.

The four families, and what makes each one its own routine:

  40  the head-hold family above.
   6  the "charge" monitors (hrt_charge_flying_kick and friends),
      which have no input sequence at all -- they count a HELD button
      up to a threshold on the MONITOR's own process and then run a
      list of mode guards.
   8  the grab_toss_air family, one per wrestler: the same three
      steps, and then a two-way choice between an airborne opponent
      (caught wherever he is) and a grounded one (who has to be
      inside a per-wrestler distance threshold).
   6  the "free" moves, which are the head-hold shape with the gate
      taken OFF -- and four of them actively refuse while anyone has
      a head hold.

Two more are read by hand in src/core/arcade/wm_arcade_smove.c,
because they have a THIRD outcome: shn_flipslam and
shn_swirl_speedkick do one thing while holding a head, another while
held, and a third from a neutral stance.

The gate's DIRECTION is the thing to get right, and it is what the
first version of this reader got wrong. Both kinds of gate contain
the strings MODE_HEADHOLD and MODE_HEADHELD:

    head-hold  cmpi MODE_HEADHOLD,a0 / jrz  #cont   (enter)
    free       cmpi MODE_HEADHOLD,a0 / jrz  #lp0    (leave)

so matching on the names read four free moves as head-hold monitors
and gated them on the exact opposite of their real condition. The
branches are simulated per mode now.

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


def home_files():
    """{routine: the file whose smove table names it}.

    This decides a real name collision in the source. SPECIAL.ASM:1536
    defines a GLOBAL `und_spirit_pull` -- the spirit object process
    UNDSEQ4.ASM's animation CREATE0s -- and TAKER.ASM:1092 defines a
    LOCAL one, the stick monitor. `.long und_spirit_pull` inside
    TAKER.ASM's own smove table binds to TAKER.ASM's local, so the
    monitor is the TAKER.ASM routine; taking the first definition in
    link order takes SPECIAL.ASM's object process instead and refuses
    both of the Undertaker's spirit moves for the wrong reason.
    """
    out = {}
    for entry in wlwrestlertbl.smove_tables():
        if not entry:
            continue
        file = wlwrestlertbl._defining_file(entry[0]).name
        for label in entry[1]:
            out.setdefault(label, file)
    return out


_EQU_DIR = re.compile(r"^#[A-Za-z_][A-Za-z0-9_]*\s+\.equ\b", re.I)
_BANNER = re.compile(r"^[#*;]?[*]{4,}")


def _trim(body):
    """Drop the next routine's prologue off the end of a body.

    A body runs to the next definition, but six of these routines are
    followed by a charge monitor, and a charge monitor's `#CHARGE_TIME
    .equ SM_USRW1` and its STRUCTPD comment sit BEFORE its label. Those
    lines land at the end of the PREVIOUS routine's slice, and
    `shn_hdhold_butts` and `bam_hdhold_pogo` -- two ordinary head-hold
    monitors -- were refused because the next routine's CHARGE_TIME
    appeared to be theirs.

    So trim back over trailing blanks, comment banners and `#SYM .equ`
    directives. A routine's own `#TIMEOUT .equ` is never trimmed: it
    has the routine's instructions after it.
    """
    end = len(body)
    while end > 0:
        line = wlanim.strip_comment(body[end - 1])
        if not line or _BANNER.match(line) or _EQU_DIR.match(line):
            end -= 1
            continue
        break
    return body[:end]


def bodies():
    """{name: (file, line, [lines])} for every smove-table routine.

    A body runs from its definition to the next NON-LOCAL definition in
    the same file. Local labels (`#lp0`) are not boundaries: they are
    the routine's own structure, and treating them as boundaries cuts
    every one of these routines off after two lines.
    """
    wanted = set(smove_names())
    home = home_files()
    seen = {}
    for path in wlanim.linked_files():
        lines = path.read_text(errors="replace").splitlines()
        marks = []
        for i, line in enumerate(lines):
            m = _SUBR.match(line) or _COL0.match(line)
            if m:
                marks.append((i, m.group(1)))
        for k, (i, name) in enumerate(marks):
            if name not in wanted:
                continue
            j = marks[k + 1][0] if k + 1 < len(marks) else len(lines)
            seen.setdefault(name, []).append(
                (path.name, i + 1, _trim(lines[i:j])))
    found = {}
    for name, defs in seen.items():
        pick = next((d for d in defs if d[0] == home.get(name)), defs[0])
        found[name] = pick
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


_LABEL = re.compile(r"^(#?[A-Za-z_][A-Za-z0-9_]*)\s*(?:;.*)?$")
_CMPI_MODE = re.compile(r"^cmpi\s+(MODE_[A-Z0-9_]+)\s*,\s*a0\b", re.I)
_JUMP = re.compile(r"^(jr[a-z]{1,3})\s+(#?[A-Za-z_][A-Za-z0-9_]*)", re.I)


def _labels(stripped):
    """{label: index} for every label this body defines."""
    out = {}
    for i, raw in enumerate(stripped):
        m = _LABEL.match(raw)
        if m:
            out.setdefault(m.group(1), i)
    return out


def _is_hdhold_gate(stripped, head_len):
    """Does the gate let the monitor IN on a head hold, or throw it OUT?

    This is the one test that separates the head-hold family from the
    Undertaker's two spirit moves, and the strings alone do not: both
    gates read PLYRMODE and name MODE_HEADHOLD and MODE_HEADHELD. The
    difference is the direction of the branch.

        head-hold  cmpi MODE_HEADHOLD,a0 / jrz  #cont   (enter, forward)
                   cmpi MODE_HEADHOLD,a0 / jrnz #lp0    (leave, back)
        spirit     cmpi MODE_HEADHOLD,a0 / jrz  #lp0    (leave, back)

    so a `jrz` to a label defined EARLIER in the body is a refusal --
    the move is unavailable while anyone has a head hold -- and
    treating it as the family's gate inverts the move.
    """
    labels = _labels(stripped)
    for i in range(head_len):
        m = _CMPI_MODE.match(stripped[i])
        if not m or m.group(1) != "MODE_HEADHOLD":
            continue
        for j in range(i + 1, min(i + 3, head_len)):
            j2 = _JUMP.match(stripped[j])
            if not j2:
                continue
            op, target = j2.group(1).lower(), j2.group(2)
            at = labels.get(target)
            if at is None:
                return False
            forward = at > j
            if op in ("jrz", "jreq"):
                return forward
            if op in ("jrnz", "jrne"):
                return not forward
            return False
    return False



_CMPI_A0_MODE = re.compile(r"^cmpi\s+(MODE_\w+)\s*,\s*a0\s*$", re.I)


def _gate_modes(live, upto):
    """Which of the two head-hold modes reach the switch sequence.

    Most of them admit both, but some are written `cmpi
    MODE_HEADHOLD,a0 / jrnz #lp0` and run only for the man doing the
    holding. Treating those as admitting both makes a move available
    to the wrong wrestler, so the gate's branches are simulated here
    rather than assumed: for each mode, walk the compares in order,
    take the branch the flags would take, and see whether the walk
    falls off the end (in) or lands on a label defined earlier (out).
    """
    labels = _labels(live)
    chain = []
    for i in range(upto):
        m = _CMPI_A0_MODE.match(live[i])
        if not m:
            continue
        j = _JUMP.match(live[i + 1]) if i + 1 < upto else None
        if not j:
            continue
        chain.append((i, m.group(1), j.group(1).lower(), j.group(2)))

    out = {}
    for mode in ("MODE_HEADHOLD", "MODE_HEADHELD"):
        at = 0
        reached = True
        for _ in range(len(chain) + 1):
            step = next((c for c in chain if c[0] >= at), None)
            if step is None:
                break
            i, cmode, op, target = step
            equal = (cmode == mode)
            if op in ("jrz", "jreq"):
                taken = equal
            elif op in ("jrnz", "jrne"):
                taken = not equal
            else:
                return {"MODE_HEADHOLD": True, "MODE_HEADHELD": True}
            if not taken:
                at = i + 2
                continue
            dest = labels.get(target)
            if dest is None or dest <= i:
                reached = False      # back to the top: refused
                break
            at = dest + 1
        out[mode] = reached
    return out


def _post_outcome(live, after, mode):
    """What a completed sequence does for a wrestler in `mode`.

    Three answers, and the reading this replaces had two. `cmpi
    MODE_HEADHELD,a0 / jrnz #lp0` with the slam label right after it
    means a wrestler whose head is HELD does the same move -- fifteen
    combo moves work that way -- while the same test with a reversal
    block in between means he reverses it, and yok_salt_throw tests
    nothing at all after the sequence and fires either way.

    So the branches are walked, exactly as the gate's are, and
    whichever instructions this mode actually lands on decide it.
    """
    labels = _labels(live)
    chain = []
    for i in range(after, len(live)):
        m = _CMPI_A0_MODE.match(live[i])
        if not m or m.group(1) not in ("MODE_HEADHOLD", "MODE_HEADHELD"):
            continue
        j = _JUMP.match(live[i + 1]) if i + 1 < len(live) else None
        if j:
            chain.append((i, m.group(1), j.group(1).lower(), j.group(2)))

    at = after
    for _ in range(len(chain) + 1):
        step = next((c for c in chain if c[0] >= at), None)
        if step is None:
            break
        i, cmode, op, target = step
        equal = (cmode == mode)
        if op in ("jrz", "jreq"):
            taken = equal
        elif op in ("jrnz", "jrne"):
            taken = not equal
        else:
            return "refuse"
        if not taken:
            at = i + 2
            continue
        dest = labels.get(target)
        if dest is None or dest <= i:
            return "refuse"
        at = dest + 1

    for k in range(at, len(live)):
        if "DO_REVERSAL" in live[k]:
            return "reversal"
        if "SPECIAL_MOVE_ADDR" in live[k]:
            break
        j = _JUMP.match(live[k])
        if j and j.group(1).lower() == "jruc":
            break
    return "slam"


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
    # `_live` also splits a local label off the front of an
    # instruction, which no head-hold body needs today -- only the
    # eight grab_toss_air and std_walk_fast write `#doit2 movi ...`
    # on one line -- but the gate and tail walkers below look labels
    # up by name, so relying on that staying true would be luck.
    live = _live(body)
    head_len = next((i for i, l in enumerate(live)
                     if _WAIT.match(l)), len(live))
    if not _is_hdhold_gate(live, head_len):
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

    # `movk 15,a14 / move a14,*a0(IMMOBILIZE_TIME)` -- how long the
    # move pins its target in place. Forty of the forty-one say 15 and
    # shn_hdhold_butts says 32, so it is a column: baking the 15 in
    # gets one of them wrong for the length of a head-butt flurry.
    victim_immobilize = None
    for i, raw in enumerate(stripped):
        if not re.match(r"^move\s+a\d+\s*,\s*\*a0\(IMMOBILIZE_TIME\)",
                        raw, re.I):
            continue
        for j in range(i - 1, max(i - 3, -1), -1):
            m = re.match(r"^mov[ki]\s+(\d+)\s*,\s*a\d+\s*$",
                         stripped[j], re.I)
            if m:
                victim_immobilize = int(m.group(1))
                break
        break
    if victim_immobilize is None:
        # Seventeen of them have `movk 15,a0 / move a0,*a14(...)`
        # COMMENTED OUT -- every combo move does -- so the move pins
        # nobody. Zero is the answer, not a missing reading.
        victim_immobilize = 0

    gate = _gate_modes(live, head_len)
    seq_end = max(i for i, l in enumerate(live) if _WAIT.match(l)) + 1
    headhold = _post_outcome(live, seq_end, "MODE_HEADHOLD")
    headheld = _post_outcome(live, seq_end, "MODE_HEADHELD")
    # `SMRTTGT a8,WHOIHIT` -- two of them target nobody at all, and
    # aiming their move at the nearest wrestler is an invention.
    smrttgt = any("SMRTTGT" in l for l in live)
    if not gate["MODE_HEADHOLD"] and not gate["MODE_HEADHELD"]:
        return None

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
        "victim_immobilize": victim_immobilize,
        "gate_headhold": gate["MODE_HEADHOLD"],
        "gate_headheld": gate["MODE_HEADHELD"],
        "headhold": headhold,
        "headheld": headheld,
        "smrttgt": smrttgt,
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



# ==================================================================
# The three families the head-hold parser refuses
# ==================================================================
#
# Eighteen of the sixty-two are not head-hold monitors, and they are
# not eighteen unrelated routines either:
#
#   charge (6)         no input sequence at all. Hold one button and
#                      count the ticks; on release, if the count
#                      reached the threshold, run a list of mode
#                      guards and fire.
#   grab_toss_air (8)  one skeleton per wrestler. Three switch steps,
#                      then a two-way choice: the opponent airborne or
#                      leaping takes one animation, an opponent inside
#                      the distance threshold takes the other.
#   free (6)           the head-hold shape's three switch steps with
#                      the head-hold GATE removed. These are done from
#                      a neutral stance and four of them actively
#                      REFUSE while anyone has a head hold -- which is
#                      how they were being read before: the old parser
#                      matched on the mode names alone, saw
#                      MODE_HEADHOLD and MODE_HEADHELD in the body,
#                      and gated four moves on the opposite of their
#                      real condition.
#
# All three end in a list of refusals, and that list is what varies
# most between them, so it is extracted as data rather than as
# columns. An instruction the reader does not recognise REFUSES the
# whole routine: a silently dropped test is a move that fires in this
# port and would not in the arcade.

# DISPLAY.EQU:46 `TSEC equ 53`. Two of these spell a count that way.
_EQU_VALUES = {"TSEC": 53}

_MOVE_FIELD = re.compile(
    r"^move\s+\*a8\((\w+)\)\s*,\s*[aA]\d+(?:\s*,\s*[WL])?\s*$", re.I)
_PTR_FIELD = re.compile(r"^move\s+\*a0\((\w+)\)\s*,\s*a0\s*$", re.I)
_BTST = re.compile(r"^btst\s+(\w+)\s*,\s*[aA]\d+\s*$", re.I)
_CALL = re.compile(r"^calla?\s+(\w+)\s*$", re.I)
_CMPI_VAL = re.compile(r"^cmpi\s+(\w+)\s*,\s*[aA]\d+\s*$", re.I)
_MOVI_NAME = re.compile(r"^mov[ki]\s+([A-Za-z_]\w*)\s*,\s*[aA]\d+\s*$", re.I)
_SETMODE = re.compile(r"^SETMODE\s+(\w+)", re.I)
_SLEEP_LINE = re.compile(r"^SLEEPK?\s+(\S.*)$", re.I)
_TIMEOUT_SYM = re.compile(r"^#TIMEOUT\s+\.equ\s+(\w+)", re.I)
_BARE_LABEL = re.compile(r"^#?[A-Za-z_]\w*\s*$")

# Lines with no behaviour this port models: sound, register shuffling,
# the pointer walk get_opp_plyrmode does inline, assembler directives.
_IGNORABLE = (
    re.compile(r"^SWAP\s", re.I),
    re.compile(r"^WRSND\s", re.I),
    re.compile(r"^X32\s", re.I),
    re.compile(r"^addi\s+process_ptrs\s*,", re.I),
    re.compile(r"^move\s+\*a0\s*,\s*a0\s*,\s*L\s*$", re.I),
    re.compile(r"^move\s+\*a8\(CLOSEST_NUM\)\s*,\s*a0\s*$", re.I),
    re.compile(r"^\.(ref|def|globl|global)\b", re.I),
    _BARE_LABEL,
)

# WRESTLE.ASM:4489 get_opp_plyrmode IS
# `process_ptrs[CLOSEST_NUM]->PLYRMODE`, so the charge monitors'
# inline walk and the call the others make are one guard.
_OPP_CALLS = ("get_opp_plyrmode",)
# WRESTLE.ASM:6016 ck_ignore and :6044 ck_ignore_a8 are the same test
# on a different register -- "is he moving away from his opponent" --
# and both refuse on carry.
_IGNORE_CALLS = ("ck_ignore", "ck_ignore_a8")
# Not a guard: it kills a looping process before the move starts.
_NEUTRAL_CALLS = ("FIND_AND_KILL_ENDLESS",)


class Refuse(Exception):
    """This routine is not the shape being read. Never repaired."""


def _num(text):
    text = text.strip()
    if text in _EQU_VALUES:
        return _EQU_VALUES[text]
    if re.fullmatch(r"[0-9A-Fa-f]+h", text):
        return int(text[:-1], 16)
    if re.fullmatch(r"\d+", text):
        return int(text)
    raise Refuse("not a number: %r" % text)


def _jmp(live, i):
    if i >= len(live):
        return None
    m = _JUMP.match(live[i])
    return (m.group(1).lower(), m.group(2)) if m else None


def read_ops(live, start, end, select=()):
    """The tests between `start` and `end`, as a list of tuples.

    Ops are ("GUARD", subject, mode) for a refusal, ("SELECT", label,
    subject, mode) for a compare that branches to one of `select`
    instead of refusing, ("DIST", n) for the proximity test, and
    ("FLAG", name) for the fieldless ones.

    Anything unrecognised raises Refuse.
    """
    ops = []
    subject = "SELF"
    i = start
    while i < end:
        line = live[i]
        if any(p.match(line) for p in _IGNORABLE):
            i += 1
            continue

        m = _CALL.match(line)
        if m:
            call = m.group(1)
            if call in _OPP_CALLS:
                subject = "OPP"
                i += 1
                continue
            if call in _NEUTRAL_CALLS:
                i += 1
                continue
            if call in _IGNORE_CALLS:
                # `jrc #lp` refuses on carry; rzr and shn spell it
                # `jrnc #norm` with the refusal falling through. Either
                # way carry means "moving away", and refuses.
                j = _jmp(live, i + 1)
                if not j or j[0] not in ("jrc", "jrnc"):
                    raise Refuse("%s without a carry test" % call)
                ops.append(("FLAG", "CK_IGNORE"))
                if j[0] == "jrc":
                    i += 2
                    continue
                # The `jrnc #norm` spelling puts the refusal in the
                # fall-through: `jrnc #norm / SWAP / jruc #lp / #norm`.
                # Step over that branch; the guard is the same.
                k = i + 2
                while k < end:
                    if any(p.match(live[k]) for p in _IGNORABLE):
                        k += 1
                        continue
                    jj = _jmp(live, k)
                    if jj and jj[0] == "jruc":
                        k += 1
                        break
                    raise Refuse("ck_ignore branch has %r" % live[k])
                i = k
                continue
            raise Refuse("unrecognised call %s" % call)

        m = _MOVE_FIELD.match(line)
        if m:
            field = m.group(1).upper()
            if field == "PLYRMODE":
                subject = "SELF"
                i += 1
                continue
            if field == "ANIMODE":
                b = _BTST.match(live[i + 1]) if i + 1 < end else None
                j = _jmp(live, i + 2)
                if not b or b.group(1).upper() != "MODE_UNINT_BIT":
                    raise Refuse("ANIMODE read that is not the UNINT bit")
                if not j or j[0] not in ("jrnz", "jrne"):
                    raise Refuse("UNINT test without jrnz")
                ops.append(("FLAG", "UNINT"))
                i += 3
                continue
            if field == "CLOSEST_DIST":
                c = _CMPI_VAL.match(live[i + 1]) if i + 1 < end else None
                j = _jmp(live, i + 2)
                if not c or not j or j[0] != "jrgt":
                    raise Refuse("CLOSEST_DIST test is not `cmpi n / jrgt`")
                ops.append(("DIST", _num(c.group(1))))
                i += 3
                continue
            simple = {"GETUP_TIME": "GETUP",
                      "IMMOBILIZE_TIME": "IMMOBILIZE",
                      "I_WILL_DIE": "I_WILL_DIE"}
            if field in simple:
                j = _jmp(live, i + 1)
                if not j or j[0] not in ("jrnz", "jrne"):
                    raise Refuse("%s test without jrnz" % field)
                ops.append(("FLAG", simple[field]))
                i += 2
                continue
            raise Refuse("unrecognised field %s" % field)

        m = _PTR_FIELD.match(line)
        if m:
            # The tail of the inline get_opp_plyrmode walk, or the
            # same walk reading the opponent's ATTACK_TYPE.
            field = m.group(1).upper()
            if field == "PLYRMODE":
                subject = "OPP"
            elif field == "ATTACK_TYPE":
                subject = "OPP_ATTACK"
            else:
                raise Refuse("pointer walk to %s" % field)
            i += 1
            continue

        m = _CMPI_VAL.match(line)
        if m:
            value = m.group(1)
            j = _jmp(live, i + 1)
            if not j:
                raise Refuse("compare without a jump")
            if j[0] in ("jrz", "jreq") and j[1] in select:
                ops.append(("SELECT", j[1], subject, value))
            elif j[0] in ("jrz", "jreq"):
                ops.append(("GUARD", subject, value))
            else:
                # `jrnz #slam` picks a path rather than refusing one.
                # The two three-way monitors do that; they are not
                # these families and are translated by hand.
                raise Refuse("compare branches %s" % j[0])
            i += 2
            continue

        raise Refuse("unrecognised line %r" % line)
    return ops


def _steps(live):
    out = []
    for raw in live:
        m = _WAIT.match(raw)
        if m:
            out.append((_operand(m.group(1)), _operand(m.group(2))))
    return out


def _timeout_of(live):
    for raw in live:
        m = _TIMEOUT_SYM.match(raw)
        if m:
            return _num(m.group(1))
    raise Refuse("no #TIMEOUT")


def _sleep_ticks(line):
    """`SLEEPK 20`, `SLEEP 3*60`, `SLEEP TSEC*3` -> ticks."""
    total = 1
    for part in line.split(None, 1)[1].split("*"):
        total *= _num(part)
    return total


def _tail_effects(live, at, stop_labels):
    """What a routine does after writing SPECIAL_MOVE_ADDR.

    Only the four effects these routines have; anything else refuses
    rather than being dropped on the floor.
    """
    eff = {"clear_run_time": False, "clear_attach": False,
           "set_mode": None, "cooldown": 0}
    i = at + 1
    while i < len(live):
        line = live[i]
        j = _jmp(live, i)
        if j and j[0] == "jruc":
            break
        if _BARE_LABEL.match(line) and line in stop_labels:
            break
        if re.match(r"^clr\s+a0\s*$", line, re.I) and i + 1 < len(live):
            nxt = live[i + 1]
            if re.match(r"^move\s+a0\s*,\s*\*a8\(RUN_TIME\)", nxt, re.I):
                eff["clear_run_time"] = True
                i += 2
                continue
            if re.match(r"^move\s+a0\s*,\s*\*a8\(ATTACH_PROC\)", nxt, re.I):
                eff["clear_attach"] = True
                i += 2
                continue
        m = _SETMODE.match(line)
        if m:
            eff["set_mode"] = m.group(1).upper()
            i += 1
            continue
        if _SLEEP_LINE.match(line):
            eff["cooldown"] = _sleep_ticks(line)
            i += 1
            continue
        if any(p.match(line) for p in _IGNORABLE):
            i += 1
            continue
        raise Refuse("unrecognised tail line %r" % line)
    return eff


def _anim_before(live, store):
    """The animation the store writes: a name, or a FACE24 pair."""
    for j in range(store - 1, max(store - 4, -1), -1):
        m = _MOVI_NAME.match(live[j])
        if m:
            return (m.group(1), None)
        m = _FACE24.match(live[j])
        if m:
            return ("%s_2_%s" % (m.group(1), m.group(2)),
                    "%s_4_%s" % (m.group(1), m.group(2)))
        if _BARE_LABEL.match(live[j]):
            continue
        # The Undertaker's near path loads the animation and jumps to
        # the shared store: `FACE24 und,snapmirror_anim / jruc #cont`.
        jj = _JUMP.match(live[j])
        if jj and jj.group(1).lower() == "jruc":
            continue
        break
    raise Refuse("no animation before the store")


# Lines that are a routine's scaffolding rather than one of its
# tests: the definition itself, its `#TIMEOUT .equ`, the SLEEPK the
# loop turns on, and the two writes to the WAITSWITCH countdown.
_PROLOGUE_OK = (
    re.compile(r"^\s*SUBRP?\s+[A-Za-z_]\w*\s*$", re.I),
    re.compile(r"^#\w+\s+\.equ\b", re.I),
    re.compile(r"^SLEEPK?\s", re.I),
    re.compile(r"^clr\s+a11\s*$", re.I),
    re.compile(r"^mov[ki]\s+#TIMEOUT\s*,\s*a11\s*$", re.I),
)

_LABEL_THEN_OP = re.compile(r"^(#[A-Za-z_]\w*)\s+([A-Za-z_].*)$")


def _live(body):
    """The body's real instructions, one per element, labels split off.

    Six of these routines write a local label and an instruction on
    ONE line -- `#doit2	FACE24 yok,hiptoss2_anim` -- and every
    instruction pattern here anchors at the start of the line, so the
    label has to come off or the instruction is invisible. A `.equ`
    is never split: `#TIMEOUT	.equ	40` is one directive.
    """
    out = []
    for raw in body:
        line = wlanim.strip_comment(raw)
        if not line:
            continue
        m = _LABEL_THEN_OP.match(line)
        if m and not m.group(2).startswith("."):
            out.append(m.group(1))
            out.append(m.group(2))
        else:
            out.append(line)
    return out


def _wait_indices(live):
    return [i for i, l in enumerate(live) if _WAIT.match(l)]


def _store_indices(live):
    return [i for i, l in enumerate(live)
            if "SPECIAL_MOVE_ADDR" in l and l.lower().startswith("move")]


def _anim_load_index(live, after):
    for i in range(after, len(live)):
        if _MOVI_NAME.match(live[i]) or _FACE24.match(live[i]):
            return i
    raise Refuse("no animation load")


def _read_gate(live, end):
    """The tests before the first switch step."""
    keep = []
    for i in range(end):
        if any(p.match(live[i]) for p in _PROLOGUE_OK):
            continue
        keep.append(live[i])
    ops = read_ops(keep, 0, len(keep))
    for op in ops:
        if op[0] == "SELECT":
            raise Refuse("a selecting gate")
    return ops


# ---- the charge family -------------------------------------------

_BUT_CUR = re.compile(r"^move\s+\*a8\(BUT_VAL_CUR\)\s*,\s*a0\s*$", re.I)
_BUT_BIT = re.compile(r"^btst\s+PLAYER_(\w+)_BIT\s*,\s*a0\s*$", re.I)
_RUNNING_PICK = re.compile(r"^cmpi\s+MODE_RUNNING\s*,\s*a0\s*$", re.I)


def parse_charge(name, file, line, body):
    """BRET.ASM:543 hrt_charge_flying_kick and five others.

    `#CHARGE_TIME .equ SM_USRW1` is a counter on the monitor's own
    process, not the wrestler's: the routine zeroes it, increments it
    once per tick for as long as the button is HELD, and on release
    compares it with a threshold. There is no WAITSWITCH_DWN anywhere
    in them.
    """
    live = _live(body)
    text = "\n".join(live)
    if "CHARGE_TIME" not in text:
        raise Refuse("no CHARGE_TIME")
    if _wait_indices(live):
        raise Refuse("a charge monitor with switch steps")

    button = None
    for i, l in enumerate(live):
        if _BUT_CUR.match(l):
            m = _BUT_BIT.match(live[i + 1]) if i + 1 < len(live) else None
            if not m:
                raise Refuse("BUT_VAL_CUR read without a button bit")
            button = m.group(1).upper()
            break
    if button is None:
        raise Refuse("no held button")

    threshold = None
    guard_start = None
    for i, l in enumerate(live):
        m = _CMPI_VAL.match(l)
        if not m:
            continue
        j = _jmp(live, i + 1)
        if j and j[0] == "jrlt":
            threshold = _num(m.group(1))
            guard_start = i + 2
            break
    if threshold is None:
        raise Refuse("no charge threshold")

    stores = _store_indices(live)
    if len(stores) != 1:
        raise Refuse("%d SPECIAL_MOVE_ADDR stores" % len(stores))
    store = stores[0]

    # `movi <anim>,a14 / move *a8(PLYRMODE),a0 / cmpi MODE_RUNNING,a0
    #  / jrnz #cont / movi <anim>_run,a14 / #cont / move ...` -- two of
    # the six have a second animation for a wrestler already running.
    first = None
    for k in range(store - 1, max(store - 8, -1), -1):
        if _BARE_LABEL.match(live[k]):
            continue
        if _MOVI_NAME.match(live[k]):
            first = k
            break
        break
    if first is None:
        raise Refuse("no animation before the store")
    anim = _MOVI_NAME.match(live[first]).group(1)
    anim_running = None
    anim_start = first
    if (first >= 3 and _RUNNING_PICK.match(live[first - 2])
            and _MOVI_NAME.match(live[first - 4] if first >= 4 else "")):
        anim_running = anim
        anim = _MOVI_NAME.match(live[first - 4]).group(1)
        anim_start = first - 4

    guards = read_ops(live, guard_start, anim_start)
    for op in guards:
        if op[0] == "SELECT":
            raise Refuse("a selecting guard")
    tail = _tail_effects(live, store, set())
    return {
        "name": name, "file": file, "line": line,
        "button": button, "threshold": threshold,
        "guards": guards, "anim": anim, "anim_running": anim_running,
        "set_mode": tail["set_mode"], "cooldown": tail["cooldown"],
    }


# ---- the grab_toss_air family -------------------------------------

# The skeleton all eight share, checked rather than assumed. Order
# included: these are refusals and selections in sequence, and a
# routine that tests the same things in another order is a different
# routine until someone has read it.
_GRAB_SHAPE = (
    ("FLAG", "UNINT"),
    ("GUARD", "SELF", "MODE_HEADHOLD"),
    ("GUARD", "OPP", "MODE_ONGROUND"),
    ("GUARD", "OPP", "MODE_DEAD"),
    ("SELECT", "#doit2", "OPP", "MODE_INAIR"),
    ("SELECT", "#doit2", "OPP", "MODE_INAIR2"),
    ("SELECT", "#doit2", "OPP_ATTACK", "AT_LEAPING"),
)


def parse_grab(name, file, line, body):
    """TAKER.ASM:1149 und_grab_toss_air and seven others.

    Three switch steps -- away, away, punch -- and then a two-way
    choice the head-hold row cannot hold: an opponent who is airborne,
    in a second air mode, or in the middle of a leaping attack takes
    the `2` animation wherever he is; anyone else has to be inside the
    wrestler's own distance threshold, and takes the `1` animation.

    The Undertaker writes it with both halves joining at one store and
    the other seven with a store each, so both are read here.
    """
    live = _live(body)
    waits = _wait_indices(live)
    if len(waits) != 3:
        raise Refuse("%d switch steps" % len(waits))
    if _read_gate(live, waits[0]):
        raise Refuse("a gate before the sequence")

    anim0 = _anim_load_index(live, waits[2] + 1)
    ops = read_ops(live, waits[2] + 1, anim0, select=("#doit2",))
    if len(ops) != len(_GRAB_SHAPE) + 1 or ops[-1][0] != "DIST":
        raise Refuse("skeleton is %r" % (ops,))
    if tuple(ops[:-1]) != _GRAB_SHAPE:
        raise Refuse("skeleton is %r" % (ops[:-1],))
    near_dist = ops[-1][1]

    stores = _store_indices(live)
    try:
        doit2 = live.index("#doit2")
    except ValueError:
        raise Refuse("no #doit2")

    if len(stores) == 1:
        # The Undertaker: `FACE24 und,snapmirror_anim / jruc #cont`,
        # `#doit2 FACE24 und,snapmirror2_anim`, `#cont move ...`.
        near = _anim_before(live, doit2)
        air = _anim_before(live, stores[0])
        tail = _tail_effects(live, stores[0], set())
        air_tail = tail
    elif len(stores) == 2:
        near, air = (_anim_before(live, stores[0]),
                     _anim_before(live, stores[1]))
        tail = _tail_effects(live, stores[0], {"#doit2"})
        air_tail = _tail_effects(live, stores[1], set())
    else:
        raise Refuse("%d SPECIAL_MOVE_ADDR stores" % len(stores))
    if tail != air_tail:
        raise Refuse("the two paths differ after the store")

    return {
        "name": name, "file": file, "line": line,
        "steps": _steps(live), "timeout": _timeout_of(live),
        "near_dist": near_dist, "near": near, "air": air,
        "clear_attach": tail["clear_attach"],
        "set_mode": tail["set_mode"], "cooldown": tail["cooldown"],
    }


# ---- the free-move family -----------------------------------------

def parse_free(name, file, line, body):
    """The head-hold shape with the head-hold gate taken off.

    Three switch steps, one animation, and a list of refusals -- but
    no head hold anywhere in the gate, and for four of the six the
    mode tests at the end REFUSE on a head hold rather than requiring
    one. Reading these as head-hold monitors, which is what matching
    on the mode names alone does, inverts them.
    """
    live = _live(body)
    waits = _wait_indices(live)
    if len(waits) != 3:
        raise Refuse("%d switch steps" % len(waits))
    stores = _store_indices(live)
    if len(stores) != 1:
        raise Refuse("%d SPECIAL_MOVE_ADDR stores" % len(stores))
    store = stores[0]

    gate = _read_gate(live, waits[0])
    anim0 = _anim_load_index(live, waits[2] + 1)
    guards = read_ops(live, waits[2] + 1, anim0)
    for op in guards + gate:
        if op[0] in ("SELECT", "DIST"):
            raise Refuse("%s in a free move" % op[0])
    anim = _anim_before(live, store)
    if anim[1]:
        raise Refuse("a FACE24 pair in a free move")
    tail = _tail_effects(live, store, set())
    return {
        "name": name, "file": file, "line": line,
        "steps": _steps(live), "timeout": _timeout_of(live),
        "gate": gate, "guards": guards, "anim": anim[0],
        "clear_run_time": tail["clear_run_time"],
        "set_mode": tail["set_mode"], "cooldown": tail["cooldown"],
    }


# Read by hand rather than generated, and why. These two are the only
# monitors with a THREE-way outcome: the same input reverses if he is
# held, slams with a bonus if he is holding, and still does something
# from a neutral stance. The tail differs on each path -- Shawn's
# swirl kick sets DELAY_METER and SPCDMG and goes into the air on the
# head-hold path only -- so there is nothing shared to parametrise.
HAND_WRITTEN_THREEWAY = ("shn_flipslam", "shn_swirl_speedkick")


def extract():
    """Every smove routine, sorted into the families that read it."""
    out = {"hdhold": [], "charge": [], "grab": [], "free": [],
           "refused": []}
    found = bodies()
    for name in smove_names():
        if name in HAND_WRITTEN or name in HAND_WRITTEN_THREEWAY:
            continue
        if name not in found:
            out["refused"].append((name, "?", 0, "no body"))
            continue
        file, line, body = found[name]
        row = parse_hdhold(name, file, line, body)
        if row:
            out["hdhold"].append(row)
            continue
        why = []
        for key, fn in (("charge", parse_charge), ("grab", parse_grab),
                        ("free", parse_free)):
            try:
                out[key].append(fn(name, file, line, body))
                break
            except Refuse as e:
                why.append("%s: %s" % (key, e))
        else:
            out["refused"].append((name, file, line, "; ".join(why)))
    return out


_HH_RESULT = {"slam": "WM_SMOVE_HH_SLAM",
              "reversal": "WM_SMOVE_HH_REVERSAL",
              "refuse": "WM_SMOVE_HH_NOTHING"}


def _bool(v):
    return "true" if v else "false"


def _mode_c(name):
    return "WM_PMODE_%s" % name if name else "-1"


def _switch_word(parts):
    return " | ".join("WM_" + p for p in parts) if parts else "0"


def _steps_c(steps):
    return ", ".join("{ %s, %s }" % (_switch_word(s), _switch_word(m))
                     for s, m in steps)


def _guard_c(op):
    if op[0] == "FLAG":
        return "{ WM_SMOVE_G_%s, 0 }" % op[1]
    subject = {"SELF": "SELF_MODE", "OPP": "OPP_MODE"}[op[1]]
    if not op[2].startswith("MODE_"):
        raise Refuse("guard on %r" % (op[2],))
    return "{ WM_SMOVE_G_%s, WM_PMODE_%s }" % (subject, op[2][len("MODE_"):])


def _guards_c(ops):
    return "{ %s }" % (", ".join(_guard_c(o) for o in ops) or "{ 0, 0 }")


def _str(s):
    return '"%s"' % s if s else "0"


_HEAD = """/* Auto-generated by tools/wlsmove.py: the special-move watchdogs,
   read out of the source rather than transcribed.

   WRESTLE2.ASM:4058 init_smoves creates one process per entry in a
   wrestler's smove table. The 79 entries name 65 distinct routines,
   and those routines are four families plus five one-offs, so what
   is generated here is each family's varying columns around a body
   they share. See wm/arcade/wm_arcade_smove.h. */
#include "wm/arcade/wm_arcade_smove.h"

"""


def emit_c(data):
    out = [_HEAD]

    out.append("const wm_smove_hdhold_t wm_smove_hdhold[] = {\n")
    for r in sorted(data["hdhold"], key=lambda x: x["name"]):
        out.append(
            '    { "%s", "%s", { %s }, %d, %d, %s, %s, %s, %s, %s, %s,\n'
            '      %d, %s, %s },   /* %s:%d */\n'
            % (r["name"], r["file"], _steps_c(r["steps"]), r["timeout"],
               -1 if r["bonus"] is None else r["bonus"],
               _bool(r["gate_headhold"]), _bool(r["gate_headheld"]),
               _HH_RESULT[r["headheld"]], _bool(r["smrttgt"]),
               _bool(r["needs_combo"]), _bool(r["needs_up"]),
               r["victim_immobilize"],
               _str(r["anim"] or r["facing_pair"][0]),
               _str(r["facing_pair"][1] if r["facing_pair"] else None),
               r["file"], r["line"]))
    out.append("};\n\nconst size_t wm_smove_hdhold_count =\n"
               "    sizeof(wm_smove_hdhold) / sizeof(wm_smove_hdhold[0]);\n\n")

    out.append("const wm_smove_charge_t wm_smove_charge[] = {\n")
    for r in sorted(data["charge"], key=lambda x: x["name"]):
        out.append(
            '    { "%s", "%s", WM_BTN_%s, %d,\n      %s, %d,\n'
            '      %s, %s, %s, %d },   /* %s:%d */\n'
            % (r["name"], r["file"], r["button"], r["threshold"],
               _guards_c(r["guards"]), len(r["guards"]),
               _str(r["anim"]), _str(r["anim_running"]),
               _mode_c(r["set_mode"]), r["cooldown"], r["file"], r["line"]))
    out.append("};\n\nconst size_t wm_smove_charge_count =\n"
               "    sizeof(wm_smove_charge) / sizeof(wm_smove_charge[0]);\n\n")

    out.append("const wm_smove_grab_t wm_smove_grab[] = {\n")
    for r in sorted(data["grab"], key=lambda x: x["name"]):
        out.append(
            '    { "%s", "%s", { %s }, %d, 0x%x,\n      %s, %s, %s, %s,\n'
            '      %s, %s, %d },   /* %s:%d */\n'
            % (r["name"], r["file"], _steps_c(r["steps"]), r["timeout"],
               r["near_dist"], _str(r["near"][0]), _str(r["near"][1]),
               _str(r["air"][0]), _str(r["air"][1]),
               _bool(r["clear_attach"]), _mode_c(r["set_mode"]),
               r["cooldown"], r["file"], r["line"]))
    out.append("};\n\nconst size_t wm_smove_grab_count =\n"
               "    sizeof(wm_smove_grab) / sizeof(wm_smove_grab[0]);\n\n")

    out.append("const wm_smove_free_t wm_smove_free[] = {\n")
    for r in sorted(data["free"], key=lambda x: x["name"]):
        out.append(
            '    { "%s", "%s", { %s }, %d,\n      %s, %d,\n      %s, %d,\n'
            '      %s, %s, %s, %d },   /* %s:%d */\n'
            % (r["name"], r["file"], _steps_c(r["steps"]), r["timeout"],
               _guards_c(r["gate"]), len(r["gate"]),
               _guards_c(r["guards"]), len(r["guards"]),
               _str(r["anim"]), _bool(r["clear_run_time"]),
               _mode_c(r["set_mode"]), r["cooldown"],
               r["file"], r["line"]))
    out.append("};\n\nconst size_t wm_smove_free_count =\n"
               "    sizeof(wm_smove_free) / sizeof(wm_smove_free[0]);\n")
    return "".join(out)


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("--out-c")
    ap.add_argument("--census", action="store_true")
    ns = ap.parse_args(argv)

    data = extract()
    if ns.census:
        print("%d head-hold, %d charge, %d grab_toss_air, %d free, "
              "%d refused"
              % (len(data["hdhold"]), len(data["charge"]),
                 len(data["grab"]), len(data["free"]),
                 len(data["refused"])))
        for name, file, line, why in data["refused"]:
            print("  refused  %-26s %s:%d  %s" % (name, file, line, why))
        return 0
    if ns.out_c:
        pathlib.Path(ns.out_c).write_text(emit_c(data))
        print("wrote %d head-hold, %d charge, %d grab, %d free -> %s"
              % (len(data["hdhold"]), len(data["charge"]),
                 len(data["grab"]), len(data["free"]), ns.out_c))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
