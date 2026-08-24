#!/usr/bin/env python3
from pathlib import Path
R=Path(__file__).resolve().parents[1]
p=(R/'tools/fix39_combat_completion_patch.py').read_text(errors='replace')
a=(R/'src/fix39/wmania_attract_core.c').read_text(errors='replace')
new=p[p.find("newfn='''"):p.find("nt=nt[:sp[0]]")]
assert 'wm_demo_tick' not in new, 'wm_demo remains gameplay authority'
assert 'wm_fix39_match_begin' in new and 'wm_fix39_match_tick' in new
assert 'plan.player_wrestler' in new and 'plan.opponent_wrestler' in new
assert 'wm_fix39_match_begin' in new and 'wm_fix39_match_tick' in new
assert 'wm_fix39_actor_source_frame(0)' in p and 'fix39_crowd_source_frame_event' in p
for s in ['uint8_t temp[8]','uint8_t ladder[7][3]','opponent_wrestler']:
    assert s in a, f'attract source ladder plan missing {s}'
print('Combat2BQ ATTR.ASM start_match ownership: PASS')
