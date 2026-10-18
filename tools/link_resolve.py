#!/usr/bin/env python3
"""Resolve WRESTLE.CMD's link line the way the linker would.

The historical dump carries variants the shipped game does not build:
ATTR.ASM beside ATTRACT.ASM, DNK.ASM beside DOINK.ASM, RING.ASM, REF.ASM.
They define the SAME labels, so a citation into one looks plausible and the
values it names are usually right -- until they are not. DNK.ASM carries
the oldest of three walk speeds and this port shipped it as Doink's.

WRESTLE.CMD is the only authority on what is in the game, so everything
rests on it being the shipped link line. That was carried as an open
question because vln_right_rope and vln_left_rope looked as though they
were defined ONLY in the unlinked RING.ASM while linked files referenced
them -- and a shipped ROM cannot have unresolved symbols.

This settles it mechanically instead of by argument. TMS34010 assembly
declares what it exports with `.def` (which the SUBR and BSSX macros in
MACROS.H expand to, per MACROS.H:241 and :246) or `.globl`, and what it
imports from elsewhere with `.ref`. A link resolves when every `.ref` in a
linked file has a matching export in a linked file. That is the whole
check, and it is the same check that would have caught the Doink walk
speed: a symbol resolvable only from an unlinked file means the link line
is not the one that built the ROM.
"""
from __future__ import annotations
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SRC = ROOT / "original" / "wwf-wrestlemania"

# `.ref a,b,c` and `.def a,b,c` both take comma-separated lists, and the
# operand field ends at a comment (`;`) or end of line.
DIRECTIVE = re.compile(r"^\s*\.(ref|def|globl|global)\s+([^;\n]+)", re.I)
# SUBR/BSSX are macros that expand to `.def`, so their operand is an export
# too. SUBRP deliberately does NOT emit `.def` (MACROS.H:252) -- it is the
# file-local form -- so it is not counted.
MACRO_DEF = re.compile(r"^\s*(SUBR|BSSX)\s+([A-Za-z_?$][A-Za-z0-9_?$]*)", re.I)
NAME = re.compile(r"^[A-Za-z_?$][A-Za-z0-9_?$]*$")


def linked_objs(cmd: pathlib.Path) -> list[str]:
    """The .obj names WRESTLE.CMD links, in link order.

    Lines inside a C-style comment are skipped: the file comments out
    `robo.obj` and `coll2.obj` that way, which is itself evidence that it
    was maintained by hand rather than generated.
    """
    text = cmd.read_text(errors="replace")
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return [m.lower() for m in re.findall(r"^\s*([A-Za-z0-9_]+)\.obj", text,
                                          re.I | re.M)]


def asm_for(obj: str) -> pathlib.Path | None:
    """The .ASM that assembles to this .obj, if the dump has it.

    Some .obj files in the link line are image or palette data assembled
    from generated sources the dump does not carry; those are reported
    rather than guessed at.
    """
    p = SRC / (obj.upper() + ".ASM")
    return p if p.exists() else None


INCLUDE = re.compile(r"^\s*\.include\s+\"?([A-Za-z0-9_.]+)\"?", re.I)


def resolve_include(name: str) -> pathlib.Path | None:
    """The dump's copy of an `.include` operand, if it has one.

    Case is not significant to the assembler and the dump is upper-case,
    so `bretimg.tbl` is looked for as BRETIMG.TBL. Many includes are the
    art pipeline's generated output (.tbl, .seq, .glo) and are simply not
    in this dump -- BRETIMG.ASM includes bretimg.tbl and bret.seq, neither
    of which exists here. That is the same gap as the zero-byte FONTS.LOD,
    and it is reported rather than silently treated as "undefined".
    """
    p = SRC / name.upper()
    return p if p.exists() and p.is_file() else None


def symbols(path: pathlib.Path, _seen: set[str] | None = None,
            absent: set[str] | None = None) -> tuple[set[str], set[str]]:
    """(exported, imported) for one assembly file, following `.include`.

    An .obj is everything the assembler saw, so a symbol defined in an
    included file is exported by the including .obj. Not following the
    includes made 211 image frame labels look undefined when they are
    only defined in a file this dump does not carry.
    """
    exports: set[str] = set()
    imports: set[str] = set()
    if _seen is None:
        _seen = set()
    key = str(path).upper()
    if key in _seen:
        return exports, imports
    _seen.add(key)
    for line in path.read_text(errors="replace").splitlines():
        m = INCLUDE.match(line)
        if m:
            inc = resolve_include(m.group(1))
            if inc is None:
                if absent is not None:
                    absent.add(m.group(1).upper())
            else:
                e, i = symbols(inc, _seen, absent)
                exports |= e
                imports |= i
            continue
        m = MACRO_DEF.match(line)
        if m:
            exports.add(m.group(2))
            continue
        m = DIRECTIVE.match(line)
        if not m:
            continue
        kind = m.group(1).lower()
        names = {n.strip() for n in m.group(2).split(",")}
        names = {n for n in names if NAME.match(n)}
        if kind == "ref":
            imports |= names
        else:
            exports |= names
    return exports, imports


def report():
    cmd = SRC / "WRESTLE.CMD"
    if not cmd.exists():
        return None
    objs = linked_objs(cmd)
    present, missing_asm = {}, []
    for o in objs:
        p = asm_for(o)
        if p is None:
            missing_asm.append(o)
        else:
            present[o] = p

    exports: dict[str, list[str]] = {}
    imports: dict[str, set[str]] = {}
    absent: set[str] = set()
    for o, p in present.items():
        e, i = symbols(p, None, absent)
        imports[o] = i
        for s in e:
            exports.setdefault(s, []).append(o)

    # Where else in the dump could an unresolved symbol come from? Every
    # .ASM the link line does NOT name, which is the set of variants the
    # shipped game did not build.
    unlinked = {}
    for p in sorted(SRC.glob("*.ASM")):
        if p.stem.lower() in present:
            continue
        e, _ = symbols(p)
        for s in e:
            unlinked.setdefault(s, []).append(p.name)

    unresolved = []
    for o in present:
        for s in sorted(imports[o]):
            if s in exports:
                continue
            unresolved.append((o, s, unlinked.get(s, [])))
    return {
        "objs": objs,
        "linked_asm": present,
        "obj_without_asm": missing_asm,
        "exports": exports,
        "unresolved": unresolved,
        "absent_includes": sorted(absent),
    }


def main() -> int:
    r = report()
    if r is None:
        print("WRESTLE.CMD not present; nothing to resolve")
        return 0
    print(f"{len(r['objs'])} .obj in the link line, "
          f"{len(r['linked_asm'])} with a .ASM in this dump")
    if r["obj_without_asm"]:
        print("  .obj with no .ASM here (image/palette data): "
              + ", ".join(r["obj_without_asm"]))
    print(f"{len(r['exports'])} exported symbols across the linked files")
    if r["absent_includes"]:
        print(f"{len(r['absent_includes'])} `.include` files the linked "
              f"sources name and this dump does not carry (art pipeline "
              f"output): e.g. " + ", ".join(r["absent_includes"][:6]))

    supplied_by_unlinked = [u for u in r["unresolved"] if u[2]]
    truly_absent = [u for u in r["unresolved"] if not u[2]]

    print(f"\n{len(r['unresolved'])} `.ref` symbols unresolved within the "
          f"link line:")
    print(f"  {len(supplied_by_unlinked)} resolvable ONLY from an unlinked "
          f".ASM  <-- these would disprove the link line")
    print(f"  {len(truly_absent)} not defined by any .ASM in the dump "
          f"(assembled data, or a library)")

    for o, s, where in supplied_by_unlinked[:40]:
        print(f"    {o}.obj refs {s} -- defined in {', '.join(where)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
