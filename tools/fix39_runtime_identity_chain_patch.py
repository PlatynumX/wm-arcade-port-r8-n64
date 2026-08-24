#!/usr/bin/env python3
from pathlib import Path
import re,sys
repo=Path(sys.argv[1])
np=repo/'src/platform/n64/main.c'
if not np.exists(): raise SystemExit('Combat2DR: N64 main.c missing')
t=np.read_text()
# 1) No silent source-id -> Bret fallback. The source roster is {0..6,8}; all must map.
old='''static unsigned fix39_frontend_slot_for_arcade(unsigned rid){\n    for(unsigned s=0;s<8u;++s)if((unsigned)wm_fix39_frontend_to_arcade_roster(s)==rid)return s;\n    return 0u;\n}'''
new='''static int fix39_frontend_slot_for_arcade(unsigned rid){\n    for(unsigned s=0;s<8u;++s)\n        if((unsigned)wm_fix39_frontend_to_arcade_roster(s)==rid)return (int)s;\n    /* ATTR.ASM never selects spare wrestler slot 7.  An unmappable source ID\n       is a hard identity error, not Bret. */\n    return -1;\n}'''
if old in t:t=t.replace(old,new,1)
elif 'return -1;' not in t[t.find('fix39_frontend_slot_for_arcade'):t.find('fix39_frontend_slot_for_arcade')+400]:
    raise SystemExit('Combat2DR: source-roster mapper seam changed')
# 2) Bind renderer-storage roster IDs to exact ATTR.ASM plan and verify the live runtime got the same IDs.
pat=re.compile(r'''if\(!wm_fix39_attract_demo_plan\(a->amode_loops,false,&plan\)\)return true;\n(?P<indent>\s*)/\* ATTR\.ASM SHOW_GAMEPLAY -> START_MATCH: one gameplay authority\.\n\s*wm_demo is renderer storage only and is never reset/ticked here\. \*/\n\s*wm_fix39_match_begin\(fix39_frontend_slot_for_arcade\(plan\.player_wrestler\),fix39_frontend_slot_for_arcade\(plan\.opponent_wrestler\)\);\n\s*wm_fix39_match_set_cpu_vs_cpu\(true\);''')
m=pat.search(t)
if m:
    repl='''if(!wm_fix39_attract_demo_plan(a->amode_loops,false,&plan))return true;\n        {\n            int p1slot=fix39_frontend_slot_for_arcade(plan.player_wrestler);\n            int p2slot=fix39_frontend_slot_for_arcade(plan.opponent_wrestler);\n            const wm_arcade_actor_t *p1; const wm_arcade_actor_t *p2;\n            if(p1slot<0 || p2slot<0)return true;\n            /* ATTR.ASM wrestler IDs are gameplay authority. wm_demo remains\n               renderer storage only, but it must carry the SAME source IDs so\n               any torso/base-frame fallback cannot silently become Bret. */\n            app->demo.p1.roster_id=plan.player_wrestler;\n            app->demo.p2.roster_id=plan.opponent_wrestler;\n            wm_fix39_match_begin((unsigned)p1slot,(unsigned)p2slot);\n            p1=wm_fix39_actor(0); p2=wm_fix39_actor(1);\n            if(!p1 || !p2 || (unsigned)p1->wrestler_num!=plan.player_wrestler ||\n               (unsigned)p2->wrestler_num!=plan.opponent_wrestler){\n                wm_fix39_match_set_cpu_vs_cpu(false); return true;\n            }\n            wm_fix39_match_set_cpu_vs_cpu(true);\n        }'''
    t=t[:m.start()]+repl+t[m.end():]
elif 'app->demo.p1.roster_id=plan.player_wrestler;' not in t:
    raise SystemExit('Combat2DR: ATTR gameplay identity seam changed')
# 3) Runtime sprite lookup must never fall through to stale wm_demo/Bret data for a live actor.
oldpref='''if(fix39_runtime_draw_index>=0){const wm_arcade_actor_t *a=wm_fix39_actor((size_t)fix39_runtime_draw_index);const char *fr=wm_fix39_actor_source_frame((size_t)fix39_runtime_draw_index);if(a&&fr){const wm_source_sprite *s=wm_character_sprite_find((uint8_t)a->wrestler_num,fr);if(s)return s;}}'''
newpref='''if(fix39_runtime_draw_index>=0){\n        const wm_arcade_actor_t *a=wm_fix39_actor((size_t)fix39_runtime_draw_index);\n        if(a){\n            const char *fr=wm_fix39_actor_source_frame((size_t)fix39_runtime_draw_index);\n            const wm_source_sprite *s=fr?wm_character_sprite_find((uint8_t)a->wrestler_num,fr):0;\n            if(s)return s;\n            /* Missing streamed frame: stay on the same wrestler. Falling\n               through to dormant wm_demo visual state can pair another\n               wrestler's CI8 pixels with the wrong TLUT and visibly garble. */\n            return wm_character_base_sprite((uint8_t)a->wrestler_num);\n        }\n    }'''
if oldpref in t:t=t.replace(oldpref,newpref,1)
elif 'Missing streamed frame: stay on the same wrestler.' not in t:
    raise SystemExit('Combat2DR: runtime fighter_sprite seam changed')
np.write_text(t)
print('Combat2DR ATTR.ASM runtime identity/renderer chain patched')
