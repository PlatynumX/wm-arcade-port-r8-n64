#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
runtime_p = ROOT / "src/fix39/wm_arcade_source_animation_runtime.c"
cmake_p = ROOT / "CMakeLists.txt"
report_p = ROOT / "reports" / "anim_attachment_source_parity.md"

failures: list[str] = []
runtime = runtime_p.read_text(encoding="utf-8", errors="replace") if runtime_p.exists() else ""
cmake = cmake_p.read_text(encoding="utf-8", errors="replace") if cmake_p.exists() else ""

def must_have(s: str, why: str) -> None:
    if s not in runtime:
        failures.append(f"missing {why}: {s}")

def must_not_have(s: str, why: str) -> None:
    if s in runtime:
        failures.append(f"still present {why}: {s}")

# ANI_ATTACH 9: source reads one long X offset only.
must_have("case 9: a->attach_xoff=av(i,0);NEXT();", "ANI_ATTACH X-only write")
must_not_have("case 9: a->attach_xoff=av(i,0);a->attach_yoff=av(i,1);NEXT();",
              "ANI_ATTACH illegal Y write")

# ANI_WAITHITGND 11: caller must be descending/not-positive before any ground probe.
must_have("case 11: if(a->y_vel>0)HOLD();", "ANI_WAITHITGND caller YVEL gate")
must_not_have("o->y_vel<=0&&o->y_int<=o->ground_y", "ANI_WAITHITGND puppet YVEL gate absent in source")

# ANI_ATTACHZ 18: X long + Z word, no Y.
must_have("case 18: a->attach_xoff=av(i,0);a->attach_zoff=av(i,1);NEXT();",
          "ANI_ATTACHZ X/Z mapping")
must_not_have("case 18: a->attach_xoff=av(i,0);a->attach_yoff=av(i,1);a->attach_zoff=av(i,2);NEXT();",
              "ANI_ATTACHZ shifted Y/Z mapping")

# Commands 24/82/85 only require both attachment pointers non-null; source does not compare reciprocal equality.
must_have("case 24: o=a->attach_proc;if(o&&o->attach_proc){", "ANI_ATTACHVEL non-null chain")
must_have("case 82: o=a->attach_proc;if(o&&o->attach_proc){", "ANI_OPPOFFSET non-null chain")
must_have("case 85: o=a->attach_proc;if(o&&o->attach_proc)o->facing_dir=o->new_facing_dir;NEXT();",
          "ANI_SETOPPFACING non-null chain")

# ANI_OPP_GETUP/CLEAR_COMBO_COUNT select ATTACH_PROC else WHOIHIT, never SMART_TARGET.
must_have("case 76: o=a->attach_proc?a->attach_proc:a->who_i_hit;", "ANI_OPP_GETUP source victim selection")
must_have("case 115: o=a->attach_proc?a->attach_proc:a->who_i_hit;", "ANI_CLEAR_COMBO_COUNT source victim selection")
must_not_have("case 76: o=opp(a);", "ANI_OPP_GETUP SMART_TARGET fallback")
must_not_have("case 115: o=opp(a);", "ANI_CLEAR_COMBO_COUNT SMART_TARGET fallback")

# ANI_IMMOBILIZE tests caller PLYR_DIZZY and victim PLYRMODE==MODE_BLOCK.
must_have("case 108: if(!a->dizzy){o=a->who_i_hit;if(o&&o->player_mode!=WM_PMODE_BLOCK){",
          "ANI_IMMOBILIZE source actor-role predicates")
must_not_have("!a->hit_blocker&&!o->dizzy", "ANI_IMMOBILIZE wrong blocker/dizzy roles")

# ANI_WAITHITGND2 mirrors the caller YVEL gate before puppet/self ground checks.
must_have("case 111: v=av(i,0);if(a->y_vel>0)HOLD();", "ANI_WAITHITGND2 caller YVEL gate")
must_not_have("o->y_vel<=0&&o->y_int<=o->ground_y+v", "ANI_WAITHITGND2 puppet YVEL gate absent in source")

# R37N7 closes the ANI_SUPERSLAVE2 frontier: raw table X/Y are not final
# placement values; exact logical WIMP geometry must participate.
must_have("superslave2_geometry", "ANI_SUPERSLAVE2 geometry service use")
must_have("dx=(int32_t)dw-dx", "ANI_SUPERSLAVE2 defender X-size correction")
must_have("if(!match)x=-x", "ANI_SUPERSLAVE2 mismatch mirror")
must_not_have("a->attach_xoff=st->entries[off+1].value;a->attach_yoff=st->entries[off+2].value;",
              "ANI_SUPERSLAVE2 raw-offset shortcut")

if "wm_anim_attachment_source_parity_audit" not in cmake:
    failures.append("CMake does not register the R37N5 ANIM attachment audit")

report_p.parent.mkdir(parents=True, exist_ok=True)
status = "FAIL" if failures else "PASS"
body = [
    "# ANIM.ASM attachment/paired-state source-parity guard",
    "",
    f"- status: {status}",
    "- scope: corrected attachment/paired-state opcode subset only",
    "- source: historicalsource/wwf-wrestlemania/ANIM.ASM",
    "- ANI_SUPERSLAVE2 (opcode 79): OPEN — requires exact get_mpart_offsets/get_mpart_xsize translation",
    "",
]
if failures:
    body += ["## Failures", ""] + [f"- {x}" for x in failures]
else:
    body += [
        "## Guarded corrections",
        "",
        "- ANI_ATTACH (9): X-only attachment offset write",
        "- ANI_WAITHITGND (11): caller Y-velocity gate before paired/self ground probes",
        "- ANI_ATTACHZ (18): X/Z argument mapping; no Y write",
        "- ANI_ATTACHVEL (24): source non-null attachment-chain predicate",
        "- ANI_OPP_GETUP (76): ATTACH_PROC else WHOIHIT only",
        "- ANI_OPPOFFSET (82): source non-null attachment-chain predicate",
        "- ANI_SETOPPFACING (85): source non-null attachment-chain predicate",
        "- ANI_IMMOBILIZE (108): caller dizzy and victim block predicates",
        "- ANI_WAITHITGND2 (111): caller Y-velocity gate before paired/self ground probes",
        "- ANI_CLEAR_COMBO_COUNT (115): ATTACH_PROC else WHOIHIT only",
        "",
        "## Open frontier",
        "",
        "ANI_SUPERSLAVE2 still writes raw slave-table X/Y offsets in the current port. "
        "Midway additionally calls get_mpart_offsets and get_mpart_xsize for both "
        "images, then performs flip-sensitive multipart math. R37N5 does not guess "
        "that metadata mapping.",
    ]
report_p.write_text("\n".join(body) + "\n", encoding="utf-8")

if failures:
    print("ANIM attachment source-parity audit: FAIL", file=sys.stderr)
    for x in failures:
        print(f" - {x}", file=sys.stderr)
    sys.exit(1)

print("ANIM attachment source-parity audit: PASS (opcode 79 remains explicitly open)")
