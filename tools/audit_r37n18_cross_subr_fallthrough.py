#!/usr/bin/env python3
from pathlib import Path

root=Path(__file__).resolve().parents[1]
g=(root/"tools/fix39_anim_vm_program.py").read_text(errors="ignore")

required=[
    "def _r37n18_falls_through(lines):",
    "terminal={'END','REPEAT','GOTO','CHANGEANIM','CHANGEANIM_TBL'}",
    "falls_through=_r37n18_falls_through(body)",
    "if not falls_through and not (missing & next_labels): break",
]
for s in required:
    assert s in g, f"missing R37N18 translator contract: {s}"
    print("PASS:",s)

assert "missing=_r37n17_branch_targets(body)-_r37n17_local_labels(body)" in g
print("PASS: R37N17 cross-SUBR branch preservation retained")

rt=(root/"src/fix39/wm_arcade_source_animation_runtime.c").read_text(errors="ignore")
assert "if(x->kind==WM_SRC_ARG_LOCAL_PC)" in rt
fix=(root/"src/fix39/wm_fix39_runtime.c").read_text(errors="ignore")
assert "if(label[0]=='#')label++;" in "".join(fix.split())
print("PASS: runtime unchanged; R37N16 ANI_CODE fix retained")

print("R37N18 structural audit: PASS")
