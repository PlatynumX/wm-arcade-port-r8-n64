#!/usr/bin/env python3
from pathlib import Path

root=Path(__file__).resolve().parents[1]
main=(root/"src/platform/n64/main.c").read_text(errors="ignore")
rt=(root/"src/fix39/wm_fix39_runtime.c").read_text(errors="ignore")
hdr=(root/"src/fix39/wm_fix39_runtime.h").read_text(errors="ignore")

checks=[
 ("flight marker","R37N19 COMBAT-ROPE-FLIGHT-RECORDER" in main),
 ("pipeline counters","R37N19 PIPE" in main and "combat_full_overlap_ticks" in main and "combat_accepted_hits" in main),
 ("fighter boxes","R37N19 FIGHT" in main and "attack_box.x1" in main and "hurt_box.x1" in main),
 ("bounce begin/end","R37N19 BOUNCE-BEGIN" in main and "R37N19 BOUNCE-END" in main),
 ("bounce stall","R37N19 BOUNCE-STALL" in main),
 ("read-only anim pc","wm_fix39_actor_source_anim_pc(size_t index)" in rt and "wm_fix39_actor_source_anim_pc(size_t index);" in hdr),
 ("read-only instruction count","wm_fix39_actor_source_anim_instructions(size_t index)" in rt),
 ("existing N15 diag retained","R37N15 FREEZE-DIAG" in main and "wm_r37n15_diag_tick();" in main),
 ("R37N16 callback fix retained","if(label[0]=='#')label++;" in "".join(rt.split())),
]
for name,ok in checks:
    if not ok:
        raise SystemExit("FAIL: "+name)
    print("PASS:",name)

flight=main[main.index("static void wm_r37n19_diag_tick"):main.index("static void wm_r37n15_diag_tick")]
for bad in ("->x_int=","->player_mode=","->anim_mode=","->attack_mode=","->attach_proc=","->in_ring="):
    assert bad not in flight, "diagnostic mutates game state: "+bad
print("PASS: flight recorder contains no actor-state assignments")
print("R37N19 flight recorder audit: PASS")
