#!/usr/bin/env python3
"""FIREWORK.ASM's data: the flight path, the flares, and the text.

do_fireworks is what runs when somebody finishes the ladder -- the
lights go down, twenty-two flares go off around the ring, the camera
leaves its match position and flies a figure of eight over the arena
while explosions pop behind it, and the congratulations message goes
up mid-flight. Nine of the file's twenty-three routines are `SUBRP`
tables rather than code, and this reads them out rather than having
them retyped.

What comes out:

  panning_points   63 waypoints, `.long ticks,[x,0],[y,0]`. The first
                   row's tick count is INIT_PAN_SPEED (= TSEC); every
                   other row is 3. The Y column is written relative to
                   EXP_FWY, which is -260.

  the flare and explosion image lists, `#flare_anim` (which runs
  straight into `#flare_anim2` -- the second label is an entry point
  five images into the first list, not a separate table) and
  `#fwexa_anim` / `#fwexb_anim`.

  `#fw_pals`, the five palettes a flare or an explosion picks from.

  the congratulations text: three message sets selected by the match
  type, each pairing a `JAM_STR` placement record from
  `#congrats_setup_tbl` with a string from `#congrats_str_tbl`. The
  pairing is positional and the two tables are different lengths in
  the source -- only the setup table is zero-terminated -- so this
  checks they line up rather than assuming it.

Nothing here is invented: a row this cannot read is refused.
"""
import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim            # noqa: E402

SOURCE = "FIREWORK.ASM"

# FIREWORK.ASM:129 `EXP_FWY .equ -260` and :498 `INIT_PAN_SPEED .equ TSEC`.
EXP_FWY = -260
TSEC = 53
INIT_PAN_SPEED = TSEC

_LABEL = re.compile(r"^(?:\s*SUBRP\s+)?([#A-Za-z_][\w]*)\s*$")
_LONG = re.compile(r"^\.long\s+(.+)$", re.I)
_STRING = re.compile(r'^\.string\s+"(.*)"\s*,\s*0\s*$', re.I)
_JAM = re.compile(r"^JAM_STR\s+(.+)$", re.I)
_NUM = re.compile(r"^(?:0?([0-9A-Fa-f]+)[Hh]|(\d+))$")
_BRACKET = re.compile(r"^\[\s*([^,\]]+)\s*,\s*([^,\]]+)\s*\]$")


class Refuse(Exception):
    """Something this reader does not understand. Never guessed at."""


def _lines():
    return (wlanim.ORIG / SOURCE).read_text(errors="replace").splitlines()


def _expr(tok):
    """A constant expression as FIREWORK.ASM writes them."""
    tok = tok.strip()
    neg = False
    if tok.startswith("-"):
        neg, tok = True, tok[1:].strip()
    # `EXP_FWY+0`, `EXP_FWY-48`, and bare numbers.
    m = re.match(r"^EXP_FWY\s*([-+])\s*(\d+)$", tok)
    if m:
        v = EXP_FWY + (int(m.group(2)) if m.group(1) == "+" else -int(m.group(2)))
        return -v if neg else v
    if tok == "EXP_FWY":
        return -EXP_FWY if neg else EXP_FWY
    if tok == "INIT_PAN_SPEED":
        return -INIT_PAN_SPEED if neg else INIT_PAN_SPEED
    m = _NUM.match(tok)
    if not m:
        raise Refuse("expression %r" % tok)
    v = int(m.group(1), 16) if m.group(1) else int(m.group(2))
    return -v if neg else v


def _split_top(s):
    """Split on commas that are not inside brackets."""
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch == "[":
            depth += 1
        elif ch == "]":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur)
            cur = ""
        else:
            cur += ch
    out.append(cur)
    return [t.strip() for t in out if t.strip() != ""]


def _label_at(line):
    m = _LABEL.match(line)
    return m.group(1) if m else None


def _index():
    """{label: [directive lines]} for every label in the file."""
    return dict(_ordered_index())


def _ordered_index():
    """[(label, [directive lines])] in source order."""
    out, cur = [], None
    for raw in _lines():
        line = wlanim.strip_comment(raw)
        if not line:
            continue
        if re.match(r"^[#*;]?\*+$", line) or re.match(r"^[#*;]?\*{4,}", line):
            continue
        if re.match(r"^\s*\.(even|bss|ref|file|title|width|option|mnolist|"
                    r"include|globl|if|endif|end)\b", line, re.I):
            continue
        if re.match(r"^\s*\w+\s+\.(set|equ)\b", line, re.I):
            continue
        lab = _label_at(line)
        if lab is not None and not re.match(r"^\.", line):
            cur = []
            out.append((lab, cur))
            continue
        if cur is not None:
            cur.append(line.strip())
    return out


def _longs(body):
    """The `.long` operands of a body, as raw tokens, stopping at 0."""
    out = []
    for line in body:
        m = _LONG.match(line)
        if not m:
            if _STRING.match(line) or _JAM.match(line):
                break
            raise Refuse("directive %r in a .long table" % line)
        for tok in _split_top(m.group(1)):
            out.append(tok)
    return out


def read_image_list(order, label):
    """An image-name list ending in 0; the 0 is dropped.

    A list may carry an interior label -- `#flare_anim2` is an entry
    point five images into `#flare_anim`, not a table of its own -- so
    this keeps reading past a label until it finds the terminator, and
    refuses a list that never has one.
    """
    names, running = [], False
    for lab, body in order:
        if lab == label:
            running = True
        elif not running:
            continue
        for tok in _longs(body):
            if tok == "0":
                return names
            if not re.match(r"^[A-Za-z_]\w*$", tok):
                raise Refuse("image name %r" % tok)
            names.append(tok)
    raise Refuse("image list %r has no terminator" % label)


def read_panning_points(idx):
    """[(ticks, x, y)] -- `.long ticks,[x,0],[y,0]` until a lone 0."""
    toks = _longs(idx["panning_points"])
    rows, i = [], 0
    while i < len(toks):
        if toks[i] == "0":
            break
        if i + 2 >= len(toks):
            raise Refuse("short panning_points row at %d" % i)
        ticks = _expr(toks[i])
        xs, ys = _BRACKET.match(toks[i + 1]), _BRACKET.match(toks[i + 2])
        if not xs or not ys:
            raise Refuse("panning point %r" % toks[i + 1:i + 3])
        if _expr(xs.group(2)) != 0 or _expr(ys.group(2)) != 0:
            raise Refuse("panning point with a fractional part")
        rows.append((ticks, _expr(xs.group(1)), _expr(ys.group(1))))
        i += 3
    else:
        raise Refuse("panning_points has no terminator")
    return rows


def read_jam_str(idx, label):
    """A JAM_STR placement record: font, spacing, cspace, x, y, palette."""
    body = idx[label]
    for line in body:
        m = _JAM.match(line)
        if m:
            f = _split_top(m.group(1))
            if len(f) != 7:
                raise Refuse("JAM_STR with %d fields" % len(f))
            return {
                "font": f[0], "spacing": _expr(f[1]), "cspace": _expr(f[2]),
                "x": _expr(f[3]), "y": _expr(f[4]),
                "palette": f[5], "method": f[6],
            }
    raise Refuse("no JAM_STR under %r" % label)


def read_string(idx, label):
    for line in idx[label]:
        m = _STRING.match(line)
        if m:
            return m.group(1)
    raise Refuse("no .string under %r" % label)


def read_congrats(idx):
    """The three message sets, each [(setup record, string)]."""
    setups = _longs(idx["#congrats_setup_tbl"])
    strs = _longs(idx["#congrats_str_tbl"])
    if len(setups) != 3 or len(strs) != 3:
        raise Refuse("congrats tables are not three sets")
    sets = []
    for s_lab, t_lab in zip(setups, strs):
        placements = []
        for tok in _longs(idx[s_lab]):
            if tok == "0":
                break
            placements.append(tok)
        else:
            raise Refuse("%s has no terminator" % s_lab)
        texts = _longs(idx[t_lab])
        if len(texts) != len(placements):
            raise Refuse("%s has %d strings for %s's %d placements"
                         % (t_lab, len(texts), s_lab, len(placements)))
        sets.append({
            "setup_table": s_lab,
            "str_table": t_lab,
            "lines": [(p, read_jam_str(idx, p), t, read_string(idx, t))
                      for p, t in zip(placements, texts)],
        })
    return sets


_MOVK = re.compile(r"^mov[ki]\s+(\w+)\s*,\s*a10$", re.I)
_MOVI_A9 = re.compile(r"^movi\s+\[([^,\]]+),([^,\]]+)\]\s*,\s*a9$", re.I)
_ADDI_A9 = re.compile(r"^addi\s+\[([^,\]]+),([^,\]]+)\]\s*,\s*a9$", re.I)
_CREATE_FLARE = re.compile(r"^CREATE\s+FIREWRK_PID\s*,\s*firework_flare$", re.I)
_DSJS = re.compile(r"^dsjs\s+a10\s*,\s*(#?\w+)$", re.I)


def read_flare_positions():
    """The [x,y] every `CREATE FIREWRK_PID,firework_flare` goes off at.

    do_fireworks spawns twenty-two flares: twelve across the back of
    the ring from a loop, then five down each side written out one at
    a time. a9 carries the position as one packed long -- X in the top
    word, Y in the bottom -- which is how firework_flare unpacks it
    (`move a9,a0 / srl 16,a0 / sll 16,a0` for X, `move a9,a1 /
    sll 16,a1` for Y).

    Rather than retyping the coordinates, this runs the four
    instructions that produce them: the counter, the seed, the step,
    and the branch back. Anything else in the body is skipped, and a
    loop that does not terminate is refused.
    """
    lines = [wlanim.strip_comment(r) for r in _lines()]
    start = None
    for i, line in enumerate(lines):
        if re.match(r"^\s*SUBR\s+do_fireworks\s*$", line):
            start = i + 1
            break
    if start is None:
        raise Refuse("no do_fireworks")
    body = []
    for line in lines[start:]:
        if re.match(r"^\s*SUBRP?\s+\w", line):
            break
        body.append(line.strip())

    # Where each local label sits, for the one backward branch.
    at = {}
    for i, line in enumerate(body):
        lab = _label_at(line)
        if lab is not None:
            at[lab] = i

    out, a9, a10, pc, steps = [], None, None, 0, 0
    while pc < len(body):
        line = body[pc]
        pc += 1
        steps += 1
        if steps > 100000:
            raise Refuse("do_fireworks flare loop does not terminate")
        m = _MOVK.match(line)
        if m:
            a10 = _expr(m.group(1))
            continue
        m = _MOVI_A9.match(line)
        if m:
            a9 = (_expr(m.group(1)), _expr(m.group(2)))
            continue
        m = _ADDI_A9.match(line)
        if m:
            if a9 is None:
                raise Refuse("addi to a9 before it was set")
            a9 = (a9[0] + _expr(m.group(1)), a9[1] + _expr(m.group(2)))
            continue
        if _CREATE_FLARE.match(line):
            if a9 is None:
                raise Refuse("a flare with no position")
            out.append(a9)
            continue
        m = _DSJS.match(line)
        if m:
            if a10 is None:
                raise Refuse("dsjs on an unset counter")
            a10 -= 1
            if a10 != 0:
                target = m.group(1)
                if target not in at:
                    raise Refuse("dsjs to %r" % target)
                pc = at[target] + 1
            continue
    if not out:
        raise Refuse("do_fireworks created no flares")
    return out


def _cname(label):
    return label.lstrip("#")


def emit_c(d):
    o = []
    o.append("/* Generated by tools/wlfirework.py from FIREWORK.ASM."
             " Do not edit. */\n")
    o.append('#include "wm/arcade/wm_arcade_firework.h"\n\n')

    o.append("/* `panning_points` -- the figure of eight, `.long ticks,"
             "[x,0],[y,0]`.\n"
             "   Y is written as EXP_FWY (%d) plus an offset. */\n" % EXP_FWY)
    o.append("const wm_fw_waypoint_t wm_fw_panning_points[] = {\n")
    for t, x, y in d["panning_points"]:
        o.append("    { %3d, %4d, %5d },\n" % (t, x, y))
    o.append("};\n")
    o.append("const size_t wm_fw_panning_point_count =\n"
             "    sizeof(wm_fw_panning_points) / "
             "sizeof(wm_fw_panning_points[0]);\n\n")

    for key, label in (("flare", "#flare_anim"),
                       ("fwexa", "#fwexa_anim"),
                       ("fwexb", "#fwexb_anim")):
        names = d[key]
        o.append("/* `%s` -- %d images. */\n" % (label, len(names)))
        o.append("const char *const wm_fw_%s_anim[] = {\n" % key)
        for n in names:
            o.append('    "%s",\n' % n)
        o.append("};\n")
        o.append("const size_t wm_fw_%s_anim_count =\n"
                 "    sizeof(wm_fw_%s_anim) / sizeof(wm_fw_%s_anim[0]);\n\n"
                 % (key, key, key))

    fp = d["flare_positions"]
    o.append("/* Every `CREATE FIREWRK_PID,firework_flare` in do_fireworks,\n"
             "   with the a9 the source had loaded at that point: twelve\n"
             "   across the back of the ring then five down each side. */\n")
    o.append("const wm_fw_point_t wm_fw_flare_positions[WM_FW_FLARES] = {\n")
    for x, y in fp:
        o.append("    { %4d, %3d },\n" % (x, y))
    o.append("};\n\n")

    o.append("/* `#fw_pals`. */\n")
    o.append("const char *const wm_fw_palettes[WM_FW_PALETTES] = {\n")
    for n in d["fw_pals"]:
        o.append('    "%s",\n' % n)
    o.append("};\n\n")

    for i, s in enumerate(d["congrats"]):
        o.append("/* %s paired with %s. */\n"
                 % (s["setup_table"], s["str_table"]))
        o.append("static const wm_fw_congrat_line_t congrats_%d[] = {\n" % i)
        for lab, jam, tlab, text in s["lines"]:
            o.append('    { "%s", %d, %d, "%s", "%s", "%s" },\n'
                     % (text, jam["x"], jam["y"], jam["font"],
                        jam["palette"], _cname(lab)))
        o.append("};\n")
    o.append("\nconst wm_fw_congrats_t wm_fw_congrats[WM_FW_CONGRATS_SETS] "
             "= {\n")
    for i, s in enumerate(d["congrats"]):
        o.append('    { "%s", congrats_%d, %d },\n'
                 % (_cname(s["setup_table"]), i, len(s["lines"])))
    o.append("};\n")
    return "".join(o)


def read_all():
    order = _ordered_index()
    idx = dict(order)
    flare = read_image_list(order, "#flare_anim")
    flare2 = read_image_list(order, "#flare_anim2")
    # #flare_anim2 is a label INSIDE #flare_anim's list, so the second
    # list has to be a suffix of the first or this reading is wrong.
    if flare[len(flare) - len(flare2):] != flare2:
        raise Refuse("#flare_anim2 is not a suffix of #flare_anim")
    return {
        "panning_points": read_panning_points(idx),
        "flare": flare,
        "flare2_at": len(flare) - len(flare2),
        "fwexa": read_image_list(order, "#fwexa_anim"),
        "fwexb": read_image_list(order, "#fwexb_anim"),
        # #fw_pals has no terminator -- it is exactly five entries and
        # the next label follows -- so it is taken as written.
        "fw_pals": _longs(idx["#fw_pals"]),
        "congrats": read_congrats(idx),
        "flare_positions": read_flare_positions(),
    }


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("--out-c")
    ap.add_argument("--census", action="store_true")
    ns = ap.parse_args(argv)

    d = read_all()
    if ns.census:
        print("panning_points: %d waypoints, ticks %s"
              % (len(d["panning_points"]),
                 sorted({t for t, _x, _y in d["panning_points"]})))
        xs = [x for _t, x, _y in d["panning_points"]]
        ys = [y for _t, _x, y in d["panning_points"]]
        print("   x %d..%d   y %d..%d" % (min(xs), max(xs), min(ys), max(ys)))
        print("flare_anim: %d images, flare_anim2 starts at %d"
              % (len(d["flare"]), d["flare2_at"]))
        print("explosions: %d + %d images" % (len(d["fwexa"]), len(d["fwexb"])))
        print("palettes: %s" % ", ".join(d["fw_pals"]))
        fp = d["flare_positions"]
        print("flares: %d, x %d..%d, y %d..%d"
              % (len(fp), min(x for x, _y in fp), max(x for x, _y in fp),
                 min(y for _x, y in fp), max(y for _x, y in fp)))
        for s in d["congrats"]:
            print("%s: %d lines at y %s"
                  % (s["setup_table"], len(s["lines"]),
                     [j["y"] for _l, j, _t, _x in s["lines"]]))
        return 0
    if ns.out_c:
        pathlib.Path(ns.out_c).write_text(emit_c(d))
        print("wrote firework tables -> %s" % ns.out_c)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
