#!/usr/bin/env python3
"""Extract the attract screens' own text literals from ATTRACT.ASM.

The port already carries these screens' layout and timing, extracted and
correct -- copyright's FIRST_Y 110, X 200, LINE_STEP 12, its 9-then-10 line
split, aama's six lines. It carries the per-line LABELS too
(wm_attract_copyright_page1_labels, wm_attract_aama_labels), in the source's
own order. What it never carried is the TEXT those labels stand for, so a
screen with correct geometry and timing had no words to put in it.

Two extraction mechanisms, because the screens index their lines two ways.

(1) BY ROUTINE EXTENT, for the `#ln`-numbered screens. show_copyright and
aama_message number their lines `#ln1`, `#ln2`, ... and `#` makes the label
local to its routine: aama_message has #ln1..#ln5 plus #ln2b, show_copyright
has #ln1..#ln19. Resolving "copyright_ln1" by searching ATTRACT.ASM for `#ln1`
finds aama's first -- "AAMA PARENTAL ADVISORY" would appear at the top of the
copyright page. So extraction is scoped to each routine's extent, and the
extent is bounded by the next routine header or column-0 label, not by the
next `.string`.

(2) BY POINTER TABLE, for the rest. show_gen_tips, show_wres_tips, show_bios
and DO_HINTS do not number their lines; they reach them through a table
(#gen_tip_table, wrestler_tips, #bio_data, WHICH_HINT) and the table's order
IS the screen's order. So these walk the table the source walks.

WHY MECHANISM (2) RESOLVES BY UNIQUE DEFINITION RATHER THAN BY SCOPE. The
assembler's rule for `#` labels cannot be established from this dump. `#` is
narrower than the file -- the #ln1 pair above proves that -- but show_bios
references #bio_data at ATTRACT.ASM:2174 and #bio_data is defined at :2679,
past the column-0 label `no_attr` (:2292), past `SUBRP show_wres_tips` (:2588)
and past `SUBR MAKE_UP_LOGO` (:2614). No reset rule that explains the #ln1
pair also lets that reference resolve, and the file carries no .newblock and
exactly one .text (:133), so there is nothing to infer it from. Rather than
guess, mechanism (2) resolves a name only when ATTRACT.ASM defines it exactly
once and raises otherwise. Of the file's 226 `#` definitions only eight names
repeat -- #ln1..#ln5, #lp, #x, #title_mod -- and none is reachable from these
four tables, so nothing here is ambiguous today; if a re-dump makes one
ambiguous the tool stops instead of picking a side.

CONTROL BYTE 1 IS A NEWLINE. STRING.ASM:515 `cmpi 1,a0 / jrne #no_newline`
adds @mess_line_spacing to @mess_cursy, so a `.byte "A",1,"B",0` is two lines
in one literal. It is emitted as `\\n`, and the renderer splits on it.
"""
from __future__ import annotations
import argparse
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SRC = ROOT / "original" / "wwf-wrestlemania" / "ATTRACT.ASM"

# A routine header (` SUBR name` / ` SUBRP name`) or a column-0 label. Both end
# the routine above them: aama_message's extent ends at `do_the_grad_thang`,
# which is a bare label and not a SUBR, so matching SUBR alone would run the
# extent on into the next screen's strings.
BOUNDARY = re.compile(r"^\s*SUBRP?\s+([A-Za-z_]\w*)|^([A-Za-z_]\w*)\s*$")
# `#ln3   .string "(C) 1995 ASSIGNED TO ACCLAIM ENTERTAINMENT, INC.",0`
STRING = re.compile(r'^#(\w+)\s+\.string\s+"([^"]*)"\s*,\s*0\s*$')

# Which routine each port label group comes out of, and the port prefix its
# labels carry. The COUNT is asserted, not discovered: these arrays are
# declared with fixed sizes in wm/arcade/wmania_attract_data.h, and a source
# that stopped matching them is a finding rather than something to absorb.
GROUPS = [
    ("show_copyright", "copyright_", 19),
    ("aama_message", "aama_", 6),
]

# Mechanism (2)'s own asserted shapes, for the same reason.
GEN_TIP_SLOTS = 11      # #gen_tip_table, 4 tips x 2 lines + 3 `blank` spacers
WRESTLERS = 8           # wrestler_tips / #bio_data, in roster order
WRES_TIP_LINES = 8      # each xxx_tips: 3 tips x 2 lines + 2 `blank` spacers
HINTS = 10              # WHICH_HINT, = NUM_HINTS (ATTRACT.ASM:3386)
HINT_FIELDS = 4         # title, body, tip-name object, mugshot object
HINT_MAX_LINES = 8      # #HNT_7 carries eight pointers; see build_hints()


def routine_extents(lines: list[str]) -> dict[str, tuple[int, int]]:
    """name -> (first_line, last_line), 1-based and inclusive."""
    starts: list[tuple[int, str]] = []
    for i, line in enumerate(lines):
        m = BOUNDARY.match(line)
        if m:
            starts.append((i + 1, m.group(1) or m.group(2)))
    out: dict[str, tuple[int, int]] = {}
    for idx, (ln, name) in enumerate(starts):
        end = starts[idx + 1][0] - 1 if idx + 1 < len(starts) else len(lines)
        # A name can repeat (ATTR.ASM-style duplicates inside one file do not
        # occur here, but a label reused later would silently win). Keep the
        # FIRST, and say so if a second turns up.
        out.setdefault(name, (ln, end))
    return out


def strings_in(lines: list[str], extent: tuple[int, int]) -> list[tuple[str, str, int]]:
    """(label, text, line_no) for each `#label .string "...",0` in extent."""
    lo, hi = extent
    found = []
    for n in range(lo, hi + 1):
        m = STRING.match(lines[n - 1])
        if m:
            found.append((m.group(1), m.group(2), n))
    return found


def c_quote(s: str) -> str:
    return ('"' + s.replace("\\", "\\\\").replace('"', '\\"')
                   .replace("\n", "\\n") + '"')


# --------------------------------------------------------------------------
# Mechanism (2): a small reader for ATTRACT.ASM's data directives.
# --------------------------------------------------------------------------

# `#HNTT_1	.byte	"OUT OF RING",0,0` puts the directive on the label's own
# line; `#gen_tip1` / `.string "..."` puts it on the next. Both occur.
LABEL_DEF = re.compile(r"^([#A-Za-z_][\w#$?]*)(?:\s+(.*))?$")


def strip_comment(s: str) -> str:
    """Drop a trailing `;` comment, respecting quotes."""
    out, quoted = [], False
    for ch in s:
        if ch == '"':
            quoted = not quoted
        if ch == ";" and not quoted:
            break
        out.append(ch)
    return "".join(out).rstrip()


def label_on(raw: str) -> re.Match | None:
    """The column-0 label this line defines, if any."""
    if not raw or raw[0] in " \t;*":
        return None
    # `#****...` is a banner comment, not a label.
    if raw.startswith("#") and len(raw) > 1 and not (raw[1].isalnum() or raw[1] == "_"):
        return None
    return LABEL_DEF.match(strip_comment(raw))


def split_operands(s: str) -> list[str]:
    out, cur, quoted = [], [], False
    for ch in s:
        if ch == '"':
            quoted = not quoted
            cur.append(ch)
        elif ch == "," and not quoted:
            out.append("".join(cur).strip())
            cur = []
        else:
            cur.append(ch)
        continue
    if "".join(cur).strip():
        out.append("".join(cur).strip())
    return out


def split_mnemonic(s: str) -> tuple[str, str]:
    head, _, rest = (s.partition("\t") if "\t" in s else s.partition(" "))
    return head.strip(), rest.strip()


class Asm:
    """ATTRACT.ASM, indexed by column-0 label."""

    def __init__(self, text: str) -> None:
        self.lines = text.splitlines()
        self.defs: dict[str, list[int]] = {}
        for i, raw in enumerate(self.lines, 1):
            m = label_on(raw)
            if m:
                self.defs.setdefault(m.group(1), []).append(i)

    def unique(self, name: str) -> int:
        at = self.defs.get(name)
        if not at:
            raise SystemExit(f"ATTRACT.ASM defines no {name}")
        if len(at) > 1:
            raise SystemExit(
                f"ATTRACT.ASM defines {name} {len(at)} times (lines {at}); "
                "the assembler's `#` scoping rule is not established from this "
                "dump, so this tool refuses to choose one")
        return at[0]

    def fields(self, name: str) -> list[tuple[str, list[str], int]]:
        """(mnemonic, operands, line_no) from the label to the next label."""
        start = self.unique(name)
        out: list[tuple[str, list[str], int]] = []
        m = LABEL_DEF.match(strip_comment(self.lines[start - 1]))
        rest = (m.group(2) or "").strip() if m else ""
        if rest:
            mn, ops = split_mnemonic(rest)
            out.append((mn, split_operands(ops), start))
        n = start
        while n < len(self.lines):
            n += 1
            raw = self.lines[n - 1]
            if label_on(raw):
                break
            body = strip_comment(raw).strip()
            if not body or body.startswith("*"):
                continue
            mn, ops = split_mnemonic(body)
            out.append((mn, split_operands(ops), n))
        return out

    def text_at(self, name: str) -> tuple[str, int]:
        """The first .string/.byte payload at or after `name`.

        A JAM_STR header sits between the label and the text for the two
        title blocks (print_message prints the string that FOLLOWS the
        setup block), so intervening directives are skipped rather than
        treated as the end of the search.
        """
        for mnemonic, operands, line_no in self.fields(name):
            if mnemonic.lower() not in (".string", ".byte"):
                continue
            parts: list[str] = []
            for op in operands:
                if op.startswith('"') and op.endswith('"'):
                    parts.append(op[1:-1])
                elif op == "1":
                    parts.append("\n")   # STRING.ASM:515
                elif op == "0":
                    return "".join(parts), line_no   # terminator
                else:
                    raise SystemExit(
                        f"{name}: ATTRACT.ASM:{line_no} has operand {op!r}, "
                        "which is neither a literal, a newline (1) nor the "
                        "terminator (0)")
            # `.string "SPECIAL MOVES"` is terminated by a following
            # `.byte 0,0` rather than by an operand of its own.
            return "".join(parts), line_no
        raise SystemExit(f"{name}: no .string/.byte before the next label")

    def longs(self, name: str) -> list[str]:
        out: list[str] = []
        for mnemonic, operands, _ in self.fields(name):
            if mnemonic.lower() == ".long":
                out.extend(operands)
        return out

    def words(self, name: str) -> list[str]:
        out: list[str] = []
        for mnemonic, operands, _ in self.fields(name):
            if mnemonic.lower() == ".word":
                out.extend(operands)
        return out

    def walk(self, table: str, expect: int) -> list[str]:
        """A null-terminated pointer table's entries, count asserted."""
        got: list[str] = []
        for entry in self.longs(table):
            if entry == "0":
                break
            got.append(entry)
        if len(got) != expect:
            raise SystemExit(
                f"{table} holds {len(got)} entries, expected {expect}: {got}")
        return got


# --------------------------------------------------------------------------
# The four table-driven screens.
# --------------------------------------------------------------------------

def build_gen_tips(asm: Asm, rows: list) -> None:
    """print_gen_tips (ATTRACT.ASM:1314), drawn by show_gen_tips (:1416).

    The title comes from #gen_tip_mes, whose JAM_STR header precedes the
    text print_message puts up. The lines come from #gen_tip_table, which
    the `#sgt_loop` at :1344 walks until it reads a 0, printing each at
    a3 and stepping a3 by 15 -- so a `blank` slot still consumes a line.
    """
    text, line_no = asm.text_at("#gen_tip_mes")
    rows.append(("gen_tip_mes", text, line_no))
    for entry in asm.walk("#gen_tip_table", GEN_TIP_SLOTS):
        text, line_no = asm.text_at(entry)
        label = entry.lstrip("#")
        if not any(have == label for have, _, _ in rows):
            rows.append((label, text, line_no))


def build_wres_tips(asm: Asm, rows: list) -> list[list[str]]:
    """show_wres_tips (ATTRACT.ASM:2588), show_bios' bios_type!=0 branch.

    `swt_loop` (:2600) walks the chosen wrestler's xxx_tips table the same
    way #sgt_loop does, from a3 = 120 in steps of 15.
    """
    text, line_no = asm.text_at("wt_title_setup")
    rows.append(("wt_title_setup", text, line_no))
    per: list[list[str]] = []
    tables = asm.longs("wrestler_tips")
    if len(tables) != WRESTLERS:
        raise SystemExit(f"wrestler_tips holds {len(tables)}, expected {WRESTLERS}")
    for table in tables:
        lines: list[str] = []
        for entry in asm.walk(table, WRES_TIP_LINES):
            text, line_no = asm.text_at(entry)
            label = entry.lstrip("#")
            if not any(have == label for have, _, _ in rows):
                rows.append((label, text, line_no))
            lines.append(label)
        per.append(lines)
    return per


def build_bios(asm: Asm, rows: list) -> list[dict]:
    """show_bios' text half (ATTRACT.ASM:2067).

    #bio_data (:2679) is eight pointers, one per wrestler in roster order.
    Each record is `.word halfwidth / .long fromstr / .word pounds,feet,
    inches / .long quote`, and show_bios reads them in exactly that order
    with four `move *a10+` steps.

    HALFWIDTH IS READ AND NOT USED. :2176 `move *a10+,a11,W` loads it and
    the comment above says "a11 becomes the halfwidth for current
    wrestler", but the only code that consumed it -- the `#bio_center`
    minus a11 at :2184 and :2192 -- is commented out, and the live path at
    :2204 writes #bio_center straight into mess_cursx. So the word has to
    be read to advance the pointer and must NOT be applied to the x. It is
    carried here because it is in the data, flagged because applying it
    would move every bio line off the arcade's own position.
    """
    for name in ("#pounds", "#feet", "#inches"):
        text, line_no = asm.text_at(name)
        rows.append((name.lstrip("#"), text, line_no))

    records = asm.longs("#bio_data")
    if len(records) != WRESTLERS:
        raise SystemExit(f"#bio_data holds {len(records)}, expected {WRESTLERS}")
    out: list[dict] = []
    for w, record in enumerate(records):
        numbers = asm.words(record)
        pointers = asm.longs(record)
        if len(numbers) != 4 or len(pointers) != 2:
            raise SystemExit(
                f"{record} has {len(numbers)} .word and {len(pointers)} .long, "
                "expected 4 and 2 (halfwidth; pounds,feet,inches; from, quote)")
        for pointer in pointers:
            text, line_no = asm.text_at(pointer)
            rows.append((pointer.lstrip("#"), text, line_no))
        out.append({
            "record": record,
            "from_label": pointers[0].lstrip("#"),
            "quote_label": pointers[1].lstrip("#"),
            "halfwidth": int(numbers[0]),
            "pounds": int(numbers[1]),
            "feet": int(numbers[2]),
            "inches": int(numbers[3]),
        })
    return out


def build_hints(asm: Asm, rows: list) -> list[dict]:
    """DO_HINTS (ATTRACT.ASM:3390).

    WHICH_HINT (:3625) is ten four-long records: title, body, the tip
    author's name object, and the author's mugshot. `last_hint` steps
    through them, so the table's order is the on-screen order, which is
    NOT the #HNT_n numbering -- record 0 is #HNT_2.

    Each body is `.long count, line, line, ...` and NEXT_HINT (:3545)
    draws `count` of them with a DSJS. #HNT_7 (hint 4, "SECOND WIND")
    declares 6 and lists EIGHT pointers: the arcade never draws
    #HNT_7G/#HNT_7H, and the second of those even reads "...WORK IF / IF
    YOU DIE OUTSIDE THE RING.", a doubled IF that suggests nobody saw it
    on a screen. Both counts are carried so the port draws what the
    arcade draws and the dead lines stay visible as data.
    """
    table = asm.longs("WHICH_HINT")
    if len(table) != HINTS * HINT_FIELDS:
        raise SystemExit(
            f"WHICH_HINT holds {len(table)} longs, expected "
            f"{HINTS * HINT_FIELDS} ({HINTS} hints x {HINT_FIELDS})")
    out: list[dict] = []
    for h in range(HINTS):
        title_ptr, body_ptr, tip_obj, mug_obj = table[h * HINT_FIELDS:(h + 1) * HINT_FIELDS]
        text, line_no = asm.text_at(title_ptr)
        rows.append((title_ptr.lstrip("#"), text, line_no))
        body = asm.longs(body_ptr)
        count = int(body[0])
        pointers = body[1:]
        if not 1 <= count <= len(pointers) <= HINT_MAX_LINES:
            raise SystemExit(
                f"{body_ptr} declares {count} lines and lists {len(pointers)} "
                f"pointers (cap {HINT_MAX_LINES})")
        lines: list[str] = []
        for entry in pointers:
            text, line_no = asm.text_at(entry)
            rows.append((entry.lstrip("#"), text, line_no))
            lines.append(entry.lstrip("#"))
        out.append({
            "title_label": title_ptr.lstrip("#"),
            "body": body_ptr,
            "lines": lines,
            "count": count,
            "total": len(pointers),
            "tip_object": tip_obj,
            "mug_object": mug_obj,
        })
    return out


def build() -> tuple[list[str], list[tuple[str, str, int]]]:
    raw = SRC.read_text(errors="replace")
    lines = raw.splitlines()
    extents = routine_extents(lines)
    rows: list[tuple[str, str, int]] = []

    # (1) the #ln-numbered screens, scoped to their routine.
    for routine, prefix, expect in GROUPS:
        if routine not in extents:
            raise SystemExit(f"ATTRACT.ASM has no routine {routine}")
        got = strings_in(lines, extents[routine])
        if len(got) != expect:
            raise SystemExit(
                f"{routine} ({extents[routine][0]}-{extents[routine][1]}) has "
                f"{len(got)} .string lines, expected {expect}: "
                f"{[g[0] for g in got]}"
            )
        for label, text, line_no in got:
            rows.append((prefix + label, text, line_no))

    # (2) the table-driven screens.
    asm = Asm(raw)
    build_gen_tips(asm, rows)
    wres = build_wres_tips(asm, rows)
    build_bios(asm, rows)
    hints = build_hints(asm, rows)

    seen = {}
    for label, _text, line_no in rows:
        if label in seen:
            raise SystemExit(f"duplicate port label {label} (lines {seen[label]}, {line_no})")
        seen[label] = line_no

    def text_for(label: str) -> str:
        for have, text, _ in rows:
            if have == label:
                return c_quote(text)
        raise SystemExit(f"no row for {label}")

    out = [
        "/* Generated by tools/wlattracttext.py from ATTRACT.ASM. Do not edit.",
        "",
        "   The attract screens' own text literals, keyed by the port label",
        "   wm/arcade/wmania_attract_data.h already uses for each line.",
        "",
        "   show_copyright and aama_message number their lines #ln1.. and `#`",
        "   makes those local, so their text is scoped to each routine's",
        "   extent -- resolving by name alone would put aama_message's first",
        "   line atop the copyright page. show_gen_tips, show_wres_tips,",
        "   show_bios and DO_HINTS instead reach their lines through a pointer",
        "   table, so theirs is read by walking that table in its own order.",
        "",
        "   A `\\n` inside a line is ATTRACT.ASM's control byte 1, which",
        "   STRING.ASM:515 turns into a cursy step of @mess_line_spacing.",
        "*/",
        '#include "wm/arcade/wmania_attract_text.h"',
        "",
        '#include "wm/arcade/wmania_attract_data.h"',
        "",
        "#include <stddef.h>",
        "#include <string.h>",
        "",
        "static const wm_attract_text_entry s_entries[] = {",
    ]
    for label, text, line_no in rows:
        out.append(f"    {{ {c_quote(label)}, {c_quote(text)} }},"
                   f"  /* ATTRACT.ASM:{line_no} */")
    out += [
        "};",
        "",
        "const size_t wm_attract_text_count = "
        "sizeof s_entries / sizeof s_entries[0];",
        "",
        "const wm_attract_text_entry *wm_attract_text_entries(void) {",
        "    return s_entries;",
        "}",
        "",
        "const char *wm_attract_text(const char *label) {",
        "    size_t i;",
        "    if (!label) return NULL;",
        "    for (i = 0; i < wm_attract_text_count; ++i) {",
        "        if (strcmp(s_entries[i].label, label) == 0) {",
        "            return s_entries[i].text;",
        "        }",
        "    }",
        "    return NULL;",
        "}",
        "",
        "/* ---- show_wres_tips (ATTRACT.ASM:2588) ---- */",
        "",
        "/* The eight xxx_tips tables of wrestler_tips, in roster order. A",
        "   `blank` slot is \"\" and not NULL: swt_loop still steps y by 15 for",
        "   it, so it is a gap in the column rather than a missing line. The",
        "   general tips go through wm_attract_general_tip_labels instead,",
        "   which wmania_attract_data.c already holds. */",
        "const char *const wm_attract_wres_tip_lines"
        "[WM_ATTRACT_WRESTLERS][WM_ATTRACT_WRES_TIP_LINES] = {",
    ]
    for w, labels in enumerate(wres):
        out.append(f"    {{  /* {w} */")
        for label in labels:
            out.append(f"        {text_for(label)},")
        out.append("    },")
    out += [
        "};",
        "",
        "/* ---- DO_HINTS (ATTRACT.ASM:3390, WHICH_HINT at :3625) ---- */",
        "",
        "/* Parallel to wm_attract_hints, which carries the same ten records'",
        "   image symbols; this carries their words and their two counts. */",
        "const wm_attract_hint_text "
        "wm_attract_hint_texts[WM_ATTRACT_ACTIVE_HINTS] = {",
    ]
    for h, hint in enumerate(hints):
        out.append(f"    {{  /* {h}, {hint['title_label']} / {hint['body']},"
                   f" {hint['tip_object']} / {hint['mug_object']} */")
        out.append(f"        {text_for(hint['title_label'])},")
        out.append("        {")
        for label in hint["lines"]:
            out.append(f"            {text_for(label)},")
        for _ in range(hint["total"], HINT_MAX_LINES):
            out.append("            NULL,")
        out.append("        },")
        out.append(f"        {hint['count']}, {hint['total']}")
        out.append("    },")
    out += [
        "};",
        "",
    ]
    return out, rows


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=pathlib.Path)
    ns = ap.parse_args()
    out, rows = build()
    text = "\n".join(out)
    if ns.out:
        ns.out.write_text(text)
        print(f"generated {len(rows)} attract text lines from ATTRACT.ASM")
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
