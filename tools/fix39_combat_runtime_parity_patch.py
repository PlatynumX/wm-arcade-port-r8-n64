#!/usr/bin/env python3
from pathlib import Path
import sys,re
repo=Path(sys.argv[1])
rc=repo/'src/fix39/wm_fix39_runtime.c'; rh=repo/'src/fix39/wm_fix39_runtime.h'; np=repo/'src/platform/n64/main.c'
t=rc.read_text(); h=rh.read_text(); n=np.read_text()
anchor='static void source_keep_attached(wm_arcade_actor_t *a, void *user)\n'
helper='''/* Combat2CX: source callback wrapper is injected before the runtime's
   concrete drone callback definition, so provide the C prototype first. */
static int drone_check_combo_go(wm_arcade_actor_t *actor, void *user);

static void source_count_button_presses(wm_arcade_actor_t *a)\n{\n    uint16_t d;\n    if (!a) return;\n    d=a->but_val_down;\n    if (d & WM_BTN_PUNCH)  ++a->punchb_count;\n    if (d & WM_BTN_BLOCK)  ++a->blockb_count;\n    if (d & WM_BTN_SPUNCH) ++a->spunchb_count;\n    if (d & WM_BTN_KICK)   ++a->kickb_count;\n    if (d & WM_BTN_SKICK)  ++a->skickb_count;\n}\nstatic int source_check_combo_go_port(wm_arcade_actor_t *a, void *user)\n{\n    return drone_check_combo_go(a,user);\n}\nstatic void source_adjust_health_port(wm_arcade_actor_t *a, int delta, void *user)\n{\n    int32_t v; (void)user; if(!a)return;\n    v=a->life+delta; if(v<0)v=0; if(v>100)v=100; a->life=v;\n}\nstatic void source_set_raisearm_bit_port(wm_arcade_actor_t *a, void *user)\n{\n    (void)user; if(a)a->status_flags|=WM_STATUS_DID_RAISEARM;\n}\n\n'''
if 'static void source_count_button_presses(' not in t:
    if anchor not in t: raise SystemExit('CU runtime callback anchor missing')
    t=t.replace(anchor,helper+anchor,1)
for name in ['common_callbacks','bret_callbacks','razor_callbacks']:
    pat=re.compile(r'(static const [^{]+\b'+re.escape(name)+r'\s*=\s*\{\n)')
    m=pat.search(t)
    if not m: raise SystemExit('CU callback table missing '+name)
    end=t.find('};',m.end())
    block=t[m.start():end]
    additions=[]
    if '.check_combo_go =' not in block: additions.append('    .check_combo_go = source_check_combo_go_port,\n')
    if '.adjust_health =' not in block: additions.append('    .adjust_health = source_adjust_health_port,\n')
    if '.set_raisearm_bit =' not in block: additions.append('    .set_raisearm_bit = source_set_raisearm_bit_port,\n')
    if additions:t=t[:m.end()]+''.join(additions)+t[m.end():]
needle='    /* ARE_WE_IN_RING / ring-out timing and health are already ported; keep\n'
insert='''    /* WRESTLE.ASM count_button_presses: live per-wrestler counters consumed\n       by animation/native sequence logic. */\n    for (i=0u;i<WM_FIX39_ACTOR_COUNT;++i) source_count_button_presses(&g.actors[i]);\n\n'''
if 'source_count_button_presses(&g.actors[i])' not in t:
    if needle not in t: raise SystemExit('CU button-count anchor missing')
    t=t.replace(needle,insert+needle,1)
needle='    /* WRESTLE.ASM set_wrestler_xflip, skipped only for MODE_NOAUTOFLIP. */\n'
insert='''    /* WRESTLE.ASM master_keep_attached after overlap_collision. */\n    for (i=0u;i<WM_FIX39_ACTOR_COUNT;++i) {\n        wm_arcade_actor_t *a=&g.actors[i];\n        if (a->anim_mode & WM_ARCADE_MODE_KEEPATTACHED)\n            (void)wm_arcade_master_keep_attached(a);\n    }\n\n'''
if 'master_keep_attached after overlap_collision' not in t:
    if needle not in t: raise SystemExit('CU keepattached anchor missing')
    t=t.replace(needle,insert+needle,1)
anchor='const char *wm_fix39_actor_source_anim(size_t index)\n'
if 'wm_fix39_actor_source_torso_frame' not in t:
    pos=t.find(anchor)
    if pos<0: raise SystemExit('CU torso getter anchor missing')
    getter='''const char *wm_fix39_actor_source_torso_frame(size_t index)\n{\n    if(!g.status.initialized)wm_fix39_runtime_init();\n    if(index>=WM_FIX39_ACTOR_COUNT)return 0;\n    return wm_source_anim_runtime_frame(&g.source_torso[index]);\n}\n\n'''
    t=t[:pos]+getter+t[pos:]
if 'wm_fix39_actor_source_torso_frame(size_t index);' not in h:
    ha='const char *wm_fix39_actor_source_frame(size_t index);\n'
    if ha not in h: raise SystemExit('CU header torso getter anchor missing')
    h=h.replace(ha,ha+'const char *wm_fix39_actor_source_torso_frame(size_t index);\n',1)
if 'fix39_runtime_draw_index' not in n: raise SystemExit('CW renderer runtime override missing')
def patch_torso_gate(text: str) -> str:
    pat = re.compile(r'static\s+bool\s+fighter_uses_torso_layer\s*\([^;{}]*\)\s*\{', re.S)
    m = pat.search(text)
    if not m:
        raise SystemExit('CW torso gate function missing')
    brace = text.find('{', m.start(), m.end()+1)
    depth = 0
    close = -1
    for i in range(brace, len(text)):
        if text[i] == '{': depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0:
                close = i
                break
    if close < 0:
        raise SystemExit('CW torso gate function malformed')
    block = text[m.start():close+1]
    if 'wm_fix39_actor_source_torso_frame((size_t)fix39_runtime_draw_index)' in block:
        return text
    line_start = text.rfind('\n', 0, m.start()) + 1
    indent = text[line_start:m.start()]
    new = (
        'static bool fighter_uses_torso_layer(const wm_demo_fighter *f) {\n'
        '    if (fix39_runtime_draw_index >= 0)\n'
        '        return wm_fix39_actor_source_torso_frame((size_t)fix39_runtime_draw_index) != NULL;\n'
        '    return f->action == WM_DEMO_IDLE || f->action == WM_DEMO_WALK ||\n'
        '           f->action == WM_DEMO_BLOCK;\n'
        '}'
    )
    if indent:
        new = new.replace('\n', '\n' + indent)
    return text[:m.start()] + new + text[close+1:]

n = patch_torso_gate(n)
# Combat2CW: apply_fix39.py upgrades the fallback torso lookup to the roster-aware
# wm_character_sprite_find() before this patcher runs. Combat2CV still required
# the obsolete exact wm_bret_sprite_find() text, so it aborted before compile.
# Find and replace the torso declaration structurally, accepting either form.
def patch_torso_lookup(text: str) -> str:
    if 'if(a&&tf) torso=wm_character_sprite_find' in text:
        return text

    frame_pat = re.compile(
        r'(?m)^(?P<indent>[ \t]*)const\s+wm_visual_frame\s*\*torso_frame\s*=\s*'
        r'wm_visual_current\s*\(\s*&f->torso_visual\s*\)\s*;'
    )
    fm = frame_pat.search(text)
    if not fm:
        raise SystemExit('CW torso frame declaration not found')

    decl_pat = re.compile(r'\bconst\s+wm_source_sprite\s*\*torso\s*=', re.M)
    dm = decl_pat.search(text, fm.end())
    if not dm:
        raise SystemExit('CW torso sprite declaration not found')
    semi = text.find(';', dm.end())
    if semi < 0:
        raise SystemExit('CW torso sprite declaration is unterminated')
    between = text[fm.end():dm.start()]
    if between.count('\n') > 6 or 'torso_frame' in between:
        raise SystemExit('CW torso sprite declaration is not adjacent to torso_frame')

    indent = fm.group('indent')
    replacement = (
        f'{indent}const wm_visual_frame *torso_frame = wm_visual_current(&f->torso_visual);\n'
        f'{indent}const wm_source_sprite *torso = NULL;\n'
        f'{indent}if (fix39_runtime_draw_index >= 0) {{\n'
        f'{indent}    const wm_arcade_actor_t *a=wm_fix39_actor((size_t)fix39_runtime_draw_index);\n'
        f'{indent}    const char *tf=wm_fix39_actor_source_torso_frame((size_t)fix39_runtime_draw_index);\n'
        f'{indent}    if(a&&tf) torso=wm_character_sprite_find((uint8_t)a->wrestler_num,tf);\n'
        f'{indent}}} else if (torso_frame) {{\n'
        f'{indent}    torso=wm_character_sprite_find(f->roster_id,torso_frame->source_frame);\n'
        f'{indent}}}'
    )
    return text[:fm.start()] + replacement + text[semi + 1:]

n = patch_torso_lookup(n)
rc.write_text(t); rh.write_text(h); np.write_text(n)
print('Combat2CW WRESTLE.ASM runtime/renderer parity patch applied (structural torso fallback)')
