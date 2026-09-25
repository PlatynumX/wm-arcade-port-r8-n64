#!/usr/bin/env python3
"""Check that the port's source citations point at the line they quote.

Every factual claim in this port is anchored by a `FILE.ASM:NNNN` citation
in a comment, and most of those citations quote the instruction they are
about in backticks. Nothing checked that the quote and the line number
agree. They are two independent statements about the same fact, so when
they disagree one of them is wrong -- and a wrong line number sends the
next reader to the wrong routine to check a value they were told was
verified.

Only the unambiguous cases are reported. A quoted snippet whose first
instruction occurs more than once in the cited file cannot be located, so
it is skipped rather than guessed at; so is a snippet too short to
identify a line (`ticks`, `.LONG`, `#skip`). What is left is the set of
citations that can be checked mechanically and exactly.
"""
from __future__ import annotations
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SRC = ROOT / "original" / "wwf-wrestlemania"

CITE = re.compile(r"\b([A-Z0-9_]+\.ASM):(\d+)\b")
TICK = re.compile(r"`([^`]{3,160})`")
# A new #define, the end of a comment, or a second citation ends the span a
# citation may claim a quote from. Without this the pairing drifts onto the
# NEXT comment's quote, which reads as a mismatch when nothing is wrong.
STOP = re.compile(r"\*/|#define|\b[A-Z0-9_]+\.ASM:\d+")

# How far above a quoted run a citation may legitimately point: the `SUBR`
# header or label that introduces it. Two lines covers a header plus the
# blank line the source usually puts under it.
SLACK = 2

# How far a cited comment-only line may sit from the run it describes.
COMMENT_REACH = 10

# A citation within a few words after a quote owns that quote.
TRAILING_CITE = re.compile(r"[\s\w]{0,12}[A-Z0-9_]+\.ASM:\d+")

_asm_cache: dict[str, list[str] | None] = {}


def asm_lines(name: str) -> list[str] | None:
    if name not in _asm_cache:
        p = SRC / name
        _asm_cache[name] = (
            p.read_text(errors="replace").splitlines() if p.exists() else None
        )
    return _asm_cache[name]


def tokens(text: str) -> list[str]:
    """Identifiers, labels and numbers, lowercased.

    TMS34010 immediates come as `80`, `0BBh`, `>0f0000` and `[1200,0]`, so
    the number pattern has to tolerate a leading `>` and a trailing `h`.
    """
    return re.findall(r"[A-Za-z_#@][A-Za-z0-9_#@.]*|>?[0-9][0-9A-Fa-f]*[Hh]?", text.lower())


def code_of(line: str) -> str:
    """The instruction part of an ASM line, with its `;` comment removed."""
    return line.split(";")[0]


def pairs(paths: list[pathlib.Path]) -> list[tuple[str, int, str, int, str]]:
    """Every (c_file, c_line, asm_file, asm_line, quoted_snippet)."""
    out = []
    for p in paths:
        lines = p.read_text(errors="replace").splitlines()
        for i, line in enumerate(lines):
            for m in CITE.finditer(line):
                name, ln = m.group(1), int(m.group(2))
                if asm_lines(name) is None:
                    continue
                # Look from just after the citation to the end of its span,
                # across at most two continuation lines of the same comment.
                span = line[m.end():]
                for nxt in lines[i + 1:i + 3]:
                    if STOP.search(span):
                        break
                    span += " " + nxt
                raw = span
                cut = STOP.search(span)
                if cut:
                    span = span[: cut.start()]
                q = TICK.search(span)
                if not q:
                    continue
                # A quote that is itself followed by "at FILE:NNN" belongs to
                # THAT citation, not this one. wm/pregame.h reads
                # "WRESTLE.ASM:160 PCNT ... `move @PCNT,a0,L / ...` at
                # WRESTLE.ASM:542" -- :160 is where PCNT is declared and :542
                # is where it is incremented, and both are right. Pairing the
                # quote with :160 reported a 380-line error in correct code.
                # Tested against the UNCUT span: STOP truncates at the next
                # citation, which is exactly the one that would own the quote,
                # so the cut span can never show it.
                if TRAILING_CITE.match(raw[q.end():]):
                    continue
                out.append((str(p.relative_to(ROOT)), i + 1, name, ln, q.group(1)))
    return out


# A routine header or a branch label: ` SUBR name`, ` SUBRP name`, or a label
# in column 0. Citing one of these and quoting an instruction inside it is a
# convention this port uses widely and deliberately -- wm_arcade_combo.h cites
# `LIFEBAR.ASM:5291 SUBR halve_combo_meter` and quotes the `movk 10,a2` eleven
# lines into it -- so it is accepted rather than reported.
HEADER = re.compile(r"^\s*SUBRP?\s+\S|^[A-Za-z_#@][A-Za-z0-9_#@]*")
# A comment-only line. The port sometimes cites the source's OWN commentary
# rather than an instruction, because that comment is the specification:
# app.h cites WRESTLE.ASM:1116, ";The only time we return from start_match is
# when the match is over", for the `JSRP start_match` six lines above it.
COMMENT_ONLY = re.compile(r"^\s*;")


def enclosing_header(name: str, run_start: int) -> int | None:
    """Line of the nearest routine header or label at/above `run_start`."""
    lines = asm_lines(name) or []
    for j in range(min(run_start, len(lines)) - 1, -1, -1):
        if HEADER.match(lines[j]):
            return j + 1
    return None


def is_comment_only(name: str, line_no: int) -> bool:
    lines = asm_lines(name) or []
    if not 1 <= line_no <= len(lines):
        return False
    return bool(COMMENT_ONLY.match(lines[line_no - 1]))


def locate(name: str, snippet: str) -> list[tuple[int, int]]:
    """Where the snippet's quoted run sits, as (first_line, last_line) spans.

    A citation quotes a RUN of instructions (`movk 2,a14 / move a14,@DAM_MULT`)
    and may legitimately point at any line of it, or at the `SUBR` header just
    above -- DCSSOUND.ASM:4265 is `SUBR BLOCK_WOOSH` and the two instructions
    it names follow it. So the run's extent is what has to be found, not one
    line of it.

    Anchoring on the first instruction alone reported thirty-odd citations
    that were pointing perfectly well at the second one. That is the failure
    mode this function exists to avoid: a check that is technically right
    about a convention nobody follows produces noise, and noise gets
    exemption rows written for it until it measures nothing.
    """
    parts = [q for q in (t.strip() for t in snippet.split("/")) if q]
    want = [tokens(q) for q in parts]
    want = [w for w in want if len(w) >= 2]
    if not want:
        return []  # nothing in the quote identifies a line
    lines = asm_lines(name) or []

    def matches(j: int, w: list[str]) -> bool:
        have = set(tokens(code_of(lines[j])))
        return all(t in have for t in w)

    spans = []
    for j in range(len(lines)):
        if not matches(j, want[0]):
            continue
        last = j
        # Each later instruction of the run must follow within a short window;
        # the source interleaves `.ref` directives and blank lines.
        ok = True
        for w in want[1:]:
            nxt = next((k for k in range(last + 1, min(last + 8, len(lines)))
                        if matches(k, w)), None)
            if nxt is None:
                ok = False
                break
            last = nxt
        if ok:
            spans.append((j + 1, last + 1))
    return spans


def findings(paths: list[pathlib.Path] | None = None):
    """Citations that point outside the run of instructions they quote."""
    if paths is None:
        paths = [
            p
            for d in ("src", "include")
            for p in (ROOT / d).rglob("*")
            if p.suffix in (".c", ".h")
        ]
    out = []
    for c_file, c_line, name, ln, snip in pairs(sorted(paths)):
        spans = locate(name, snip)
        if len(spans) != 1:
            continue  # not uniquely locatable -- skipped, never guessed at
        lo, hi = spans[0]
        # SLACK covers citing the blank line or label immediately above.
        if lo - SLACK <= ln <= hi + SLACK:
            continue
        # The routine or label that encloses the run.
        hdr = enclosing_header(name, lo)
        if hdr is not None and hdr - SLACK <= ln <= hdr + SLACK:
            continue
        # The source's own comment about the run, near it on either side.
        if is_comment_only(name, ln) and abs(ln - lo) <= COMMENT_REACH:
            continue
        out.append(
            {
                "c_file": c_file,
                "c_line": c_line,
                "asm_file": name,
                "cited": ln,
                "actual": lo,
                "span": f"{lo}-{hi}" if hi != lo else str(lo),
                "snippet": snip,
            }
        )
    return out


def main() -> int:
    got = findings()
    for f in sorted(got, key=lambda d: -abs(d["actual"] - d["cited"])):
        print(
            f'{f["c_file"]}:{f["c_line"]}  cites {f["asm_file"]}:{f["cited"]}'
            f'  -> quoted run is at :{f["span"]}'
            f'  (off by {abs(f["actual"] - f["cited"])})'
        )
        print(f'    `{f["snippet"][:90]}`')
    print(f"\n{len(got)} citation(s) disagree with the line they quote")
    return 0


if __name__ == "__main__":
    sys.exit(main())
