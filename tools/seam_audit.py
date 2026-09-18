#!/usr/bin/env python3
"""Every callback seam in the port, and whether anything fills it.

WHY THIS EXISTS. This port routes the source's cross-module calls through
function-pointer fields named after the routines they stand for. That
reads well and has one failure mode, which has now produced findings six
times running: a seam gets declared, its call sites get written with a
NULL check in front of them, and nothing ever assigns it. The call
compiles, runs, does nothing, and looks exactly like working code.

Found that way so far: the REACT reaction dispatch, good_run_hit, the
match-award tally, bounce_off_ropes, auto_pin_check, drone_change_back,
set_raisearm_bit, REACT4's shake_all_ropes and shaker2, REACT5's
slide_getup_meter, bozo_check, find_and_kill_endless, and sound_label --
which was dropping fifty calls across six files.

So the number is measured here rather than rediscovered. Each seam that
is CALLED but never ASSIGNED needs a row in port/seam_ledger.json giving
a verdict; an unledgered one fails the source-tools test. The point is
not that every seam must be wired -- plenty are display this port does
not have -- but that leaving one empty has to be a decision somebody
wrote down.
"""

import json
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import wlanim  # noqa: E402
import port_coverage  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parents[1]
LEDGER = ROOT / "port" / "seam_ledger.json"
GENERATED = "src/generated"

# `.field = something` or `->field = something`. An assignment through
# either is what fills a seam; a comparison (`== `) is not, which is why
# the right-hand side must start a name or an address-of.
ASSIGN = re.compile(r"(?:\.|->)\s*([A-Za-z_]\w*)\s*=\s*[&A-Za-z_]")
# `.field(` or `->field(` -- a call through the seam.
CALL = re.compile(r"(?:\.|->)\s*([A-Za-z_]\w*)\s*\(")

VERDICTS = ("wired", "display", "hardware", "unreached", "fallback",
            "deferred")


def scan() -> tuple[dict[str, str], set[str], set[str],
                    dict[str, int], dict[str, int]]:
    """(seam -> declaring file, assigned, called, decl counts, assign counts).

    The two counts exist because of a BLIND SPOT this tool had and was
    caught by: it keys on the seam's NAME, so a seam declared in several
    callback structs and filled in only one of them reads as filled.
    check_secret_moves was exactly that -- declared in
    wm_arcade_roster.h, wm_arcade_razor.h and wm_arcade_bret.h, assigned
    only for Bret, and therefore invisible here while seven wrestlers
    had no button-sequence secret moves at all.

    Resolving it properly means knowing which struct each assignment
    targets, which needs a real C parser rather than a regex. The counts
    are the cheap signal instead: a name declared in more structs than
    it has assignment sites is worth a look. It is reported, not
    enforced -- one adapter legitimately serving two structs is normal,
    and a heuristic that failed the build on it would be noise.
    """
    decl: dict[str, str] = {}
    assigned: set[str] = set()
    called: set[str] = set()
    decl_n: dict[str, int] = {}
    assign_n: dict[str, int] = {}
    for d in port_coverage.PORT_DIRS:
        for p in sorted((ROOT / d).rglob("*")):
            if p.suffix not in (".c", ".h"):
                continue
            rel = str(p.relative_to(ROOT))
            if rel.startswith(GENERATED):
                continue
            code, _prose = port_coverage._split_code_and_prose(
                p.read_text(errors="replace"))
            for n in port_coverage.CALLBACK_DECL.findall(code):
                decl.setdefault(n, rel)
                decl_n[n] = decl_n.get(n, 0) + 1
            for n in ASSIGN.findall(code):
                assigned.add(n)
                assign_n[n] = assign_n.get(n, 0) + 1
            called |= set(CALL.findall(code))
    return decl, assigned, called, decl_n, assign_n


def audit() -> dict:
    decl, assigned, called, decl_n, assign_n = scan()
    rows = []
    for name in sorted(decl):
        rows.append({
            "name": name,
            "declared_in": decl[name],
            "filled": name in assigned,
            "called": name in called,
        })
    open_rows = [r for r in rows if not r["filled"] and r["called"]]
    # See scan(): declared in more structs than it has assignment sites.
    thin = sorted(n for n in decl
                  if decl_n.get(n, 0) > 1
                  and assign_n.get(n, 0) < decl_n.get(n, 0)
                  and n in assigned and n in called)
    return {
        "total": len(rows),
        "filled": sum(1 for r in rows if r["filled"]),
        "called_but_empty": len(open_rows),
        "partially_filled": thin,
        "rows": rows,
    }


def load_ledger() -> dict:
    if not LEDGER.exists():
        return {}
    raw = json.loads(LEDGER.read_text())
    return {k: v for k, v in raw.items() if not k.startswith("_")}


def write_md(path: pathlib.Path, a: dict, ledger: dict) -> None:
    empty = [r for r in a["rows"] if not r["filled"] and r["called"]]
    by: dict[str, list[dict]] = {}
    for r in empty:
        v = ledger.get(r["name"], {}).get("verdict", "UNLEDGERED")
        by.setdefault(v, []).append(r)
    out = ["# Callback seams", "",
           "Generated by `tools/seam_audit.py`. A seam is a",
           "function-pointer field this port uses to route one of the",
           "source's cross-module calls. The failure mode it exists to",
           "catch: a seam declared, called behind a NULL check, and",
           "assigned by nobody -- which compiles, runs, does nothing, and",
           "reads like working code.", "",
           "| | count |", "|---|---|",
           "| seams declared | %d |" % a["total"],
           "| filled by something | %d |" % a["filled"],
           "| **called but empty** | **%d** |" % a["called_but_empty"],
           "",
           "Every empty one carries a verdict in `port/seam_ledger.json`,",
           "and a source-tools test refuses both an unledgered empty seam",
           "and a ledger row for a seam that is no longer empty.", ""]
    for v in sorted(by):
        out.append("## %s (%d)" % (v, len(by[v])))
        out.append("")
        out.append("| seam | declared in |")
        out.append("|---|---|")
        for r in sorted(by[v], key=lambda x: x["name"]):
            out.append("| `%s` | `%s` |" % (r["name"], r["declared_in"]))
        out.append("")
    path.write_text("\n".join(out) + "\n")


def main() -> int:
    a = audit()
    ledger = load_ledger()
    for i, arg in enumerate(sys.argv):
        if arg == "--out-md" and i + 1 < len(sys.argv):
            write_md(pathlib.Path(sys.argv[i + 1]), a, ledger)
    empty = [r["name"] for r in a["rows"]
             if not r["filled"] and r["called"]]
    print("callback seams:      %d" % a["total"])
    print("filled by something: %d" % a["filled"])
    print("called but empty:    %d" % a["called_but_empty"])
    if a["partially_filled"]:
        print("\nfilled in fewer structs than declare them (%d) --"
              % len(a["partially_filled"]))
        print("a heuristic, not a verdict; see scan()'s docstring:")
        for n in a["partially_filled"]:
            print("   ", n)
    missing = [n for n in empty if n not in ledger]
    stale = [n for n in ledger if n not in empty]
    if missing:
        print("\nempty and unledgered (%d):" % len(missing))
        for n in missing:
            print("   ", n)
    if stale:
        print("\nledgered but no longer empty (%d):" % len(stale))
        for n in sorted(stale):
            print("   ", n)
    by = {}
    for n in empty:
        by[ledger.get(n, {}).get("verdict", "UNLEDGERED")] = \
            by.get(ledger.get(n, {}).get("verdict", "UNLEDGERED"), 0) + 1
    if by:
        print("\nby verdict:")
        for k in sorted(by):
            print("   %-12s %d" % (k, by[k]))
    return 1 if (missing or stale) else 0


if __name__ == "__main__":
    raise SystemExit(main())
