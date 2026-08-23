#!/usr/bin/env python3
from pathlib import Path
R=Path(__file__).resolve().parents[1]
base=(R/'tools/apply_fix39.py').read_text(errors='replace')
cp=(R/'tools/fix39_combat_completion_patch.py').read_text(errors='replace')
rt=(R/'src/fix39/wm_fix39_runtime.c').read_text(errors='replace')
for name,text in [('apply_fix39',base),('completion',cp)]:
    assert 'wm_demo_tick(&app->demo' not in text, name+' resurrects wm_demo gameplay'
    assert 'wm_fix39_match_sync_presenter_pose(0, app->demo' not in text
    assert 'wm_fix39_match_sync_presenter_pose(1, app->demo' not in text
start=cp.index("newfn='''")
end=cp.index("'''",start+9)+3
seg=cp[start:end]
assert 'wm_fix39_match_begin' in seg and 'wm_fix39_match_tick' in seg
assert 'wm_fix39_match_begin' in seg and 'wm_fix39_match_tick' in seg
assert 'wm_demo_reset_match' not in seg and 'wm_demo_set_roster' not in seg
assert 'app->demo.p1.health' not in seg and 'app->demo.p2.health' not in seg
fn=rt[rt.index('void wm_fix39_match_sync_presenter_pose'):rt.index('void wm_fix39_match_bind_source_frame_attack')]
for forbidden in ['a->player_mode =','a->x_int =','a->z_int =','a->facing_dir =']:
    assert forbidden not in fn, forbidden
print('Combat2CS ATTR.ASM single-authority ownership: PASS')
