#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
p=root/'src/platform/n64/main.c'
s=p.read_text()
need=[
 'return -1;',
 'app->demo.p1.roster_id=plan.player_wrestler;',
 'app->demo.p2.roster_id=plan.opponent_wrestler;',
 '(unsigned)p1->wrestler_num!=plan.player_wrestler',
 '(unsigned)p2->wrestler_num!=plan.opponent_wrestler',
 'return wm_character_base_sprite((uint8_t)a->wrestler_num);',
]
missing=[x for x in need if x not in s]
assert not missing,missing
assert 'return 0u;' not in s[s.index('fix39_frontend_slot_for_arcade'):s.index('fix39_frontend_slot_for_arcade')+420]
print('Combat2DR source-ID -> actor -> streamed-renderer identity chain: PASS')
