#!/usr/bin/env python3
from pathlib import Path

root=Path(__file__).resolve().parents[1]
g=(root/"tools/fix39_anim_vm_program.py").read_text(errors="ignore")

required=[
    "def _r37n17_local_labels(lines):",
    "def _r37n17_branch_targets(lines):",
    "SUBR is an entry label, not a hard end-of-program boundary",
    "missing=_r37n17_branch_targets(body)-_r37n17_local_labels(body)",
    "next_labels=_r37n17_local_labels(next_body)",
]
for s in required:
    assert s in g, f"missing R37N17 translator contract: {s}"
    print("PASS:",s)

assert (
    "if not (missing & next_labels): break" in g
    or "if not falls_through and not (missing & next_labels): break" in g
), "missing R37N17 shared-tail extension semantics"
print("PASS: R37N17 shared-tail extension retained through R37N18 fallthrough-aware condition")

rt=(root/"src/fix39/wm_arcade_source_animation_runtime.c").read_text(errors="ignore")
assert "if(x->kind==WM_SRC_ARG_LOCAL_PC)" in rt
assert "if(!branch_n(s,a,i,0))s->fault=45" in "".join(rt.split())
print("PASS: runtime LOCAL_PC/fault semantics retained; fix is translator-side")

fix=(root/"src/fix39/wm_fix39_runtime.c").read_text(errors="ignore")
assert "if(label[0]=='#')label++;" in "".join(fix.split())
print("PASS: R37N16 ANI_CODE local-label fix retained")

print("R37N17/R37N18 structural audit: PASS")
