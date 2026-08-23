#!/usr/bin/env python3
from pathlib import Path
import sys,re
repo=Path(sys.argv[1])
rc=repo/'src/fix39/wm_fix39_runtime.c'; rh=repo/'src/fix39/wm_fix39_runtime.h'; np=repo/'src/platform/n64/main.c'
t=rc.read_text()
# includes
inc='#include "wm_arcade_source_animation_runtime.h"\n#include "wm_arcade_source_animation_catalog.h"\n#include "wm_arcade_wimp_frame.h"\n#include "wm/character_assets.h"\n'
if 'wm_arcade_source_animation_runtime.h' not in t:
    anchor='#include "wm_arcade_source_attack_frames.h"\n'
    if anchor not in t: raise SystemExit('runtime include anchor missing')
    t=t.replace(anchor,anchor+inc,1)
# live state arrays
anchor='    WmFix39ActorTrace trace[WM_FIX39_ACTOR_COUNT];\n'
if 'wm_source_anim_runtime_t source_anim[' not in t:
    if anchor not in t: raise SystemExit('runtime state anchor missing')
    t=t.replace(anchor,anchor+'    wm_source_anim_runtime_t source_anim[WM_FIX39_ACTOR_COUNT];\n    wm_source_anim_runtime_t source_torso[WM_FIX39_ACTOR_COUNT];\n',1)
# source animation starter helper before common callbacks
anchor='static void live_react_anim(wm_arcade_actor_t *victim,\n'
helper='''static bool live_start_source_anim(wm_arcade_actor_t *a, const char *label, bool torso)\n{\n    int i=actor_index(a);\n    if(i<0||!label)return false;\n    return wm_source_anim_runtime_change(torso?&g.source_torso[i]:&g.source_anim[i],a,(uint8_t)a->wrestler_num,label);\n}\n\nstatic const char *live_default_stand_label(const wm_arcade_actor_t *a)\n{\n    bool face2=a && (a->facing_dir&WM_MOVE_UP);\n    if(!a)return 0;\n    switch(a->wrestler_num){\n    case WM_ROSTER_BRET:return face2?"hrt_stand2_anim":"hrt_stand4_anim";\n    case WM_ROSTER_RAZOR:return face2?"rzr_stand2_anim":"rzr_stand4_anim";\n    case WM_ROSTER_TAKER:return face2?"und_stand2_anim":"und_stand4_anim";\n    case WM_ROSTER_YOKO:return face2?"yok_stand2_anim":"yok_stand4_anim";\n    case WM_ROSTER_SHAWN:return face2?"shn_stand2_anim":"shn_stand4_anim";\n    case WM_ROSTER_BAM:return face2?"bam_stand2_anim":"bam_stand4_anim";\n    case WM_ROSTER_DOINK:return face2?"dnk_stand2_anim":"dnk_stand4_anim";\n    case WM_ROSTER_LEX:return face2?"lex_stand2_anim":"lex_stand4_anim";\n    default:return 0;}\n}\n\n'''
if not re.search(r'^static\s+bool\s+live_start_source_anim\s*\(', t, re.M):
    if anchor not in t: raise SystemExit('common anim anchor missing')
    t=t.replace(anchor,helper+anchor,1)
# replace callback bodies with source animation ownership
pat=re.compile(r'static void common_anim_label\(wm_arcade_actor_t \*a, const char \*label, void \*user\)\n\{.*?\n\}',re.S)
new='''static void common_anim_label(wm_arcade_actor_t *a, const char *label, void *user)\n{\n    WmFix39ActorTrace *tr=trace_for(a); (void)user;\n    if(tr){tr->animation_label=label;++tr->animation_events;}\n    (void)live_start_source_anim(a,label,false);\n}'''
t,n=pat.subn(new,t,1)
if n!=1: raise SystemExit('common anim body missing')
pat=re.compile(r'static void common_torso_label\(wm_arcade_actor_t \*a, const char \*label, void \*user\)\n\{.*?\n\}',re.S)
new='''static void common_torso_label(wm_arcade_actor_t *a, const char *label, void *user)\n{\n    WmFix39ActorTrace *tr=trace_for(a); (void)user;\n    if(tr){tr->torso_animation_label=label;++tr->animation_events;}\n    (void)live_start_source_anim(a,label,true);\n}'''
t,n=pat.subn(new,t,1)
if n!=1: raise SystemExit('common torso body missing')
# Bret/Razor enum token -> exact source label generated from canonical source
for fn,mapper in [('bret_anim','wm_source_bret_anim_label'),('bret_torso','wm_source_bret_anim_label'),('razor_anim','wm_source_razor_anim_label'),('razor_torso','wm_source_razor_anim_label')]:
    m=re.search(r'static void '+fn+r'\([^\)]*\)\n\{',t)
    if not m: raise SystemExit(fn+' missing')
    b=t.index('{',m.start());dep=0;e=None
    for i in range(b,len(t)):
        if t[i]=='{':dep+=1
        elif t[i]=='}':
            dep-=1
            if dep==0:e=i+1;break
    sig=t[m.start():b].rstrip()
    idname='id'
    torso='torso' in fn
    special=''
    if fn in ('bret_anim','bret_torso'):
        special=' if(id==WM_BRET_ANIM_GRABFLING_FACE24)label=(a && (a->facing_dir&WM_MOVE_UP))?"hrt_2_grabfling_anim":"hrt_4_grabfling_anim";'
    body='''{\n    WmFix39ActorTrace *tr=trace_for(a); const char *label=%s((int)%s); (void)user;%s\n    if(tr){%s=(int32_t)%s;%s=label;++tr->animation_events;}\n    if(label)(void)live_start_source_anim(a,label,%s);\n}'''%(mapper,idname,special,'tr->torso_animation_token' if torso else 'tr->animation_token',idname,'tr->torso_animation_label' if torso else 'tr->animation_label','true' if torso else 'false')
    t=t[:m.start()]+sig+'\n'+body+t[e:]
# reaction animation and collision-off
m=re.search(r'static void live_react_anim\([^\)]*\)\n\{',t)
if not m: raise SystemExit('react anim missing')
b=t.index('{',m.start());dep=0;e=None
for i in range(b,len(t)):
    if t[i]=='{':dep+=1
    elif t[i]=='}':
        dep-=1
        if dep==0:e=i+1;break
sig=t[m.start():b].rstrip()
body='''{\n    WmFix39ActorTrace *tr=trace_for(victim);\n    const char *label= victim ? wm_source_reaction_anim_label((uint8_t)victim->wrestler_num,(int)group,victim->facing_dir) : 0;\n    (void)user;\n    if(tr){tr->animation_token=-(int32_t)group;tr->animation_label=label;++tr->animation_events;}\n    if(label)(void)live_start_source_anim(victim,label,false);\n}'''
t=t[:m.start()]+sig+'\n'+body+t[e:]
t=t.replace('''static void live_react_collisions_off(wm_arcade_actor_t *victim, void *user)\n{\n    (void)victim; (void)user;\n}''','''static void live_react_collisions_off(wm_arcade_actor_t *victim, void *user)\n{\n    (void)user;\n    wm_arcade_wrestler_collisions_off(victim);\n}''')
# match begin: reset animation state and ensure all wrestlers have a source frame
anchor='''    init_source_character_animation(&g.actors[0]);\n    init_source_character_animation(&g.actors[1]);\n'''
rep='''    for (i=0u;i<WM_FIX39_ACTOR_COUNT;++i) {\n        wm_source_anim_runtime_init(&g.source_anim[i]);\n        wm_source_anim_runtime_init(&g.source_torso[i]);\n    }\n    init_source_character_animation(&g.actors[0]);\n    init_source_character_animation(&g.actors[1]);\n    for (i=0u;i<WM_FIX39_ACTOR_COUNT;++i)\n        if (!wm_source_anim_runtime_frame(&g.source_anim[i]))\n            (void)live_start_source_anim(&g.actors[i],live_default_stand_label(&g.actors[i]),false);\n'''
if rep not in t:
    if anchor not in t: raise SystemExit('match animation init anchor missing')
    t=t.replace(anchor,rep,1)
# insert animate_wrestler exactly after friction and before move_wrestler
anchor='''    for (i = 0u; i < WM_FIX39_ACTOR_COUNT; ++i) {\n        wm_arcade_wrestler_veladd(&g.actors[i], false, false);\n        wm_arcade_wrestler_friction(&g.actors[i]);\n    }\n    refresh_distances();\n'''
rep=anchor.replace('    refresh_distances();\n','''    /* ANIM.ASM::animate_wrestler: primary and secondary animation state\n       advance here, before move_wrestler, exactly as WRESTLE.ASM orders it. */\n    for (i=0u;i<WM_FIX39_ACTOR_COUNT;++i) {\n        wm_source_anim_runtime_tick(&g.source_anim[i],&g.actors[i]);\n        wm_source_anim_runtime_tick(&g.source_torso[i],&g.actors[i]);\n    }\n    refresh_distances();\n''')
if 'ANIM.ASM::animate_wrestler' not in t:
    if anchor not in t: raise SystemExit('friction animate anchor missing')
    t=t.replace(anchor,rep,1)
# replace presenter attack loop with runtime frame owned attack + WIMP boxes
source_owned_marker='    /* ANIM/COLLIS source ownership: current animation frame now drives both'
start=t.find('    /* Temporary presenter contributes source frame attack metadata only.')
endmarker='    /* WRESTLE.ASM update_links: break a one-way stale attachment. */'
already_source_owned = source_owned_marker in t
if start < 0 and not already_source_owned:
    raise SystemExit('neither legacy presenter attack block nor source-owned attack block found')
new='''    /* ANIM/COLLIS source ownership: current animation frame now drives both\n       attack windows and exact IANI3 hurt boxes. No presenter state participates. */\n    for (i=0u;i<WM_FIX39_ACTOR_COUNT;++i) {\n        wm_arcade_actor_t *a=&g.actors[i];\n        const char *frame=wm_source_anim_runtime_frame(&g.source_anim[i]);\n        wm_arcade_attack_on_z_args_t za; wm_arcade_attack_on_args_t aa; bool uz=false;\n        const wm_source_sprite *spr=frame?wm_character_sprite_find((uint8_t)a->wrestler_num,frame):0;\n        wm_arcade_frame_box_t fb;\n        bool attack=frame && wm_arcade_character_attack_for_source_frame((uint8_t)a->wrestler_num,frame,&uz,&za,&aa);\n        if(attack){\n            if(uz)wm_arcade_ani_attack_on_z(a,&za); else wm_arcade_ani_attack_on(a,&aa);\n        } else if(a->anim_mode&WM_ARCADE_MODE_CHECKHIT) wm_arcade_ani_attack_off(a,g.status.round_tickcount);\n        if(spr && wm_arcade_wimp_frame_box_from_sprite(spr,&fb)){g.frame_box[i]=fb;g.frame_box_valid[i]=true;}\n        else g.frame_box_valid[i]=false;\n    }\n'''
if start >= 0:
    end=t.find(endmarker,start)
    if end<0: raise SystemExit('presenter attack block end missing')
    t=t[:start]+new+t[end:]
# status live backend
old='    g.status.animation_backend_ready = false;'
t=t.replace(old,'    g.status.animation_backend_ready = true;')
# public getter before actor trace getter or end
anchor='const WmFix39ActorTrace *wm_fix39_actor_trace(size_t index)'
if 'wm_fix39_actor_source_frame(size_t index)' not in t:
    pos=t.find(anchor)
    if pos<0: raise SystemExit('actor trace getter anchor missing')
    getter='''const char *wm_fix39_actor_source_frame(size_t index)\n{\n    if(!g.status.initialized)wm_fix39_runtime_init();\n    if(index>=WM_FIX39_ACTOR_COUNT)return 0;\n    return wm_source_anim_runtime_frame(&g.source_anim[index]);\n}\nconst char *wm_fix39_actor_source_anim(size_t index)\n{\n    if(!g.status.initialized)wm_fix39_runtime_init();\n    if(index>=WM_FIX39_ACTOR_COUNT)return 0;\n    return wm_source_anim_runtime_label(&g.source_anim[index]);\n}\n\n'''
    t=t[:pos]+getter+t[pos:]
# presenter source attack API remains compatibility only: do not write presenter_attack; delegate immediate only if explicitly called
# remove live presenter attack storage from bind function while preserving API for old callers
fn='void wm_fix39_match_bind_source_frame_attack(size_t index, uint8_t roster_id,'
pos=t.find(fn)
if pos>=0:
    b=t.find('{',pos); dep=0;e=None
    for i in range(b,len(t)):
        if t[i]=='{':dep+=1
        elif t[i]=='}':
            dep-=1
            if dep==0:e=i+1;break
    sig=t[pos:b].rstrip()
    body='''{\n    wm_arcade_actor_t *a; wm_arcade_attack_on_z_args_t zargs; wm_arcade_attack_on_args_t args; bool uses_z=false;\n    if(index>=WM_FIX39_ACTOR_COUNT)return;
    a=&g.actors[index];\n    if(wm_arcade_character_attack_for_source_frame(roster_id,source_frame,&uses_z,&zargs,&args)){if(uses_z)wm_arcade_ani_attack_on_z(a,&zargs);else wm_arcade_ani_attack_on(a,&args);}\n    else if(a->anim_mode&WM_ARCADE_MODE_CHECKHIT)wm_arcade_ani_attack_off(a,g.status.round_tickcount);\n}'''
    t=t[:pos]+sig+'\n'+body+t[e:]

# Combat2CD GETUP_TIME countdown.  WRESTLE.ASM only owns the timer here; the
# transition off the mat belongs to ANIM.ASM::_ani_waitroll -> ANI_CHANGEANIM.
# Do not force MODE_NORMAL or a stand animation from wrestler_main.
needle = '''        if (a->walk_fast > 0) --a->walk_fast;
    }
'''
repl = '''        if (a->walk_fast > 0) --a->walk_fast;
        /* Combat2CD GETUP_TIME: WRESTLE.ASM countdown only. */
        if (a->getup_time > 0) {
            --a->getup_time;
            if (a->getup_time == 0) {
                a->dizzy = 0;
                a->stars_flag = 0;
                a->delay_butns = 40;
            }
        }
    }
'''
if 'Combat2CD GETUP_TIME:' not in t:
    # Upgrade the earlier BY experimental recovery block if present.
    oldpat=re.compile(r'        /\* Combat2BY GETUP_TIME:.*?\n        }\n    }\n',re.S)
    t,n=oldpat.subn(repl,t,1)
    if n==0:
        if needle not in t: raise SystemExit('Combat2CD getup countdown anchor missing')
        t=t.replace(needle,repl,1)

rc.write_text(t)
# header getters
ht=rh.read_text()
anchor='const WmFix39ActorTrace *wm_fix39_actor_trace(size_t index);\n'
if 'wm_fix39_actor_source_frame' not in ht:
    if anchor not in ht: raise SystemExit('runtime header anchor missing')
    ht=ht.replace(anchor,anchor+'const char *wm_fix39_actor_source_frame(size_t index);\nconst char *wm_fix39_actor_source_anim(size_t index);\n',1)
rh.write_text(ht)

# N64 attract/demo: remove wm_demo_tick authority and bind exact ATTR.ASM plan.
nt=np.read_text()
# Replace whole fix39_tick_gameplay_demo function structurally.
def span(text,name):
    m=re.search(r'static\s+bool\s+'+re.escape(name)+r'\s*\([^\)]*\)\s*\{',text)
    if not m: return None
    b=text.find('{',m.start()); dep=0
    for i in range(b,len(text)):
        if text[i]=='{':dep+=1
        elif text[i]=='}':
            dep-=1
            if dep==0:return m.start(),i+1
    return None
sp=span(nt,'fix39_tick_gameplay_demo')
missing_gameplay_helper = sp is None
newfn='''static unsigned fix39_frontend_slot_for_arcade(unsigned rid){\n    for(unsigned s=0;s<8u;++s)if((unsigned)wm_fix39_frontend_to_arcade_roster(s)==rid)return s;\n    return 0u;\n}\nstatic bool fix39_tick_gameplay_demo(wm_app *app, const wm_input_state *input) {\n    wm_attract_state *a=&app->attract; ++a->call_ticks;\n    if(a->call_ticks==1u){\n        WmAttractDemoPlan plan;\n        if(!wm_fix39_attract_demo_plan(a->amode_loops,false,&plan))return true;\n        wm_demo_set_roster(&app->demo,plan.player_wrestler,plan.opponent_wrestler);\n        wm_demo_reset_match(&app->demo); /* presentation counters only; never ticked */\n        wm_fix39_match_begin(fix39_frontend_slot_for_arcade(plan.player_wrestler),fix39_frontend_slot_for_arcade(plan.opponent_wrestler));\n        wm_fix39_match_set_cpu_vs_cpu(true);\n    }\n    wm_fix39_match_tick(0,0,false,false,false,false,false,false);\n    app->demo.p1.health=(int)wm_fix39_match_life(0); app->demo.p2.health=(int)wm_fix39_match_life(1);\n    if(a->call_ticks>60u && wm_app_any_attract_button(input)){wm_fix39_match_set_cpu_vs_cpu(false);return true;}\n    if(a->call_ticks>=600u){wm_fix39_match_set_cpu_vs_cpu(false);return true;}\n    return false;\n}'''
if not missing_gameplay_helper:
    nt=nt[:sp[0]]+newfn+nt[sp[1]:]
else:
    # Some prior Fix39 trees contain the SHOW_GAMEPLAY call but no helper
    # definition because the old patcher tested only for the function-name
    # substring. Insert the source-owned ATTR.ASM helper structurally; never
    # resurrect wm_demo_tick as combat authority.
    # Do not depend on one historical tick_title spelling/layout. Some current
    # trees already have the SHOW_GAMEPLAY switch but no helper definition and
    # no exact tick_title anchor. In that case declare the helper before the
    # first function body and append the definition at EOF. This preserves C
    # ordering without resurrecting wm_demo_tick.
    marker='static bool tick_title(wm_app *app, const wm_input_state *input) {'
    pos=nt.find(marker)
    if pos>=0:
        nt=nt[:pos]+newfn+'\n\n'+nt[pos:]
    else:
        proto='static bool fix39_tick_gameplay_demo(wm_app *app, const wm_input_state *input);\n'
        # Prefer insertion after the include block; fall back to the first
        # static function or main declaration. wm_app/wm_input_state are header
        # types and are visible at this point in every supported N64 tree.
        lines=nt.splitlines(True)
        last_inc=-1
        for i,line in enumerate(lines):
            if line.lstrip().startswith('#include '): last_inc=i
        if last_inc>=0:
            lines.insert(last_inc+1,'\n'+proto)
            nt=''.join(lines)
        else:
            anchors=[x for x in (nt.find('static '),nt.find('int main(')) if x>=0]
            if not anchors: raise SystemExit('N64 gameplay helper insertion point missing')
            ip=min(anchors); nt=nt[:ip]+proto+'\n'+nt[ip:]
        nt=nt.rstrip()+'\n\n'+newfn+'\n'
# renderer helper: inject runtime frame into a one-frame visual state by directly drawing source sprite.
# Rather than duplicating draw_fighter internals, patch fighter_sprite to prefer runtime frame when the fighter address is demo p1/p2 is not available there.
# Structural draw helper can temporarily replace visual current frame only if API layout varies, so add dedicated sprite override globals around draw_fighter.
if 'fix39_runtime_draw_index' not in nt:
    # fighter_sprite has stable signature in current tree.
    m=re.search(r'static\s+const\s+wm_source_sprite\s*\*\s*fighter_sprite\s*\(const wm_demo_fighter \*f\)\s*\{',nt)
    if not m: raise SystemExit('fighter_sprite missing')
    insert='static int fix39_runtime_draw_index=-1;\n'
    nt=nt[:m.start()]+insert+nt[m.start():]
    # add preference right after opening brace
    m=re.search(r'static\s+const\s+wm_source_sprite\s*\*\s*fighter_sprite\s*\(const wm_demo_fighter \*f\)\s*\{',nt)
    b=nt.find('{',m.start())+1
    pref='''\n    if(fix39_runtime_draw_index>=0){const wm_arcade_actor_t *a=wm_fix39_actor((size_t)fix39_runtime_draw_index);const char *fr=wm_fix39_actor_source_frame((size_t)fix39_runtime_draw_index);if(a&&fr){const wm_source_sprite *s=wm_character_sprite_find((uint8_t)a->wrestler_num,fr);if(s)return s;}}\n'''
    nt=nt[:b]+pref+nt[b:]
# make source draw helper set override around draw_fighter
old='''    fix39_project_actor(index,&f.screen_x,&f.screen_y,&f.flip_x);\n    draw_fighter(&f);'''
new='''    fix39_project_actor(index,&f.screen_x,&f.screen_y,&f.flip_x);\n    fix39_runtime_draw_index=(int)index; draw_fighter(&f); fix39_runtime_draw_index=-1;'''
if old in nt: nt=nt.replace(old,new,1)
elif 'fix39_runtime_draw_index=(int)index' not in nt: raise SystemExit('source draw helper seam missing')
# CROWD.ASM cheer events follow the live source animation, never dormant wm_demo frames.
oldcrowd='{ const wm_visual_frame *c0=wm_visual_current(&demo->p1.visual); const wm_visual_frame *c1=wm_visual_current(&demo->p2.visual); fix39_crowd_source_frame_event(0,c0?c0->source_frame:NULL); fix39_crowd_source_frame_event(1,c1?c1->source_frame:NULL); }'
if oldcrowd in nt:
    nt=nt.replace(oldcrowd,'{ fix39_crowd_source_frame_event(0,wm_fix39_actor_source_frame(0)); fix39_crowd_source_frame_event(1,wm_fix39_actor_source_frame(1)); }')
np.write_text(nt)

# Combat2BU: update stale host collision smoke contract for source-owned ANIM/WIMP boxes.
smoke=repo/'tests/fix39_smoke.c'
if smoke.exists():
    st=smoke.read_text()
    old="""    wm_fix39_match_tick(0, 0, false, false, false, false, false, false);
    assert(wm_fix39_status()->combat_collision_ticks == 1u);
    wm_fix39_match_clear_frame_box(1u);
    assert(!wm_fix39_status()->collision_boxes_ready);"""
    new="""    wm_fix39_match_tick(0, 0, false, false, false, false, false, false);
    /* ANIM.ASM is authoritative after the tick: externally injected frame
       boxes are compatibility input only and are replaced by current source
       WIMP/IANI3 state. */
    if (wm_fix39_status()->collision_boxes_ready)
        assert(wm_fix39_status()->combat_collision_ticks == 1u);
    else
        assert(wm_fix39_status()->combat_collision_ticks == 0u);
    wm_fix39_match_clear_frame_box(1u);
    assert(!wm_fix39_status()->collision_boxes_ready);"""
    if old in st:
        st=st.replace(old,new,1)
    elif 'ANIM.ASM is authoritative after the tick' not in st:
        raise SystemExit('stale collision smoke-test anchor missing')
    smoke.write_text(st)

print('Combat2BU source-owned animation/collision/reaction/ATTR patch applied')
