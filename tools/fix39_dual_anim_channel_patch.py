#!/usr/bin/env python3
from pathlib import Path
import sys
repo=Path(sys.argv[1])
h=repo/'src/fix39/wm_arcade_source_animation_runtime.h'
c=repo/'src/fix39/wm_arcade_source_animation_runtime.c'
r=repo/'src/fix39/wm_fix39_runtime.c'
ht=h.read_text(); ct=c.read_text(); rt=r.read_text()

# WRESTLE/ANIM source has two independent animation channels: ANIMODE/ANICNT
# for legs/body and ANIMODE2/ANICNT2 for torso.  The previous portable runtime
# let both VM instances mutate actor->anim_mode/ani_count, so the torso stream
# could end/reset the primary attack stream (and vice versa).
old='''struct wm_source_anim_runtime {\n    const wm_source_anim_program_t *program;\n    uint16_t pc;\n    const char *current_frame;\n    const wm_source_anim_services_t *services;\n    uint32_t instructions_executed;\n    int fault;\n};'''
new='''struct wm_source_anim_runtime {\n    const wm_source_anim_program_t *program;\n    uint16_t pc;\n    const char *current_frame;\n    const wm_source_anim_services_t *services;\n    uint32_t instructions_executed;\n    int fault;\n    /* Combat2CZ: ANIM.ASM has independent primary/secondary state. */\n    uint16_t mode_shadow;\n    int32_t count_shadow;\n    uint8_t secondary;\n};'''
if old in ht:
    ht=ht.replace(old,new,1)
elif 'uint8_t secondary;' not in ht:
    raise SystemExit('Combat2CZ animation runtime struct anchor missing')

anchor='void wm_source_anim_runtime_bind(wm_source_anim_runtime_t *s,const wm_source_anim_services_t *services);\n'
if 'wm_source_anim_runtime_set_secondary' not in ht:
    if anchor not in ht: raise SystemExit('Combat2CZ runtime header API anchor missing')
    ht=ht.replace(anchor,anchor+'void wm_source_anim_runtime_set_secondary(wm_source_anim_runtime_t *s,bool secondary);\n',1)

old='void wm_source_anim_runtime_bind(wm_source_anim_runtime_t*s,const wm_source_anim_services_t*v){if(s)s->services=v;}\n'
new=old+'void wm_source_anim_runtime_set_secondary(wm_source_anim_runtime_t*s,bool secondary){if(s){s->secondary=secondary?1u:0u;s->mode_shadow=0;s->count_shadow=1;}}\n'
if 'wm_source_anim_runtime_set_secondary' not in ct:
    if old not in ct: raise SystemExit('Combat2CZ bind implementation anchor missing')
    ct=ct.replace(old,new,1)

# Match ANIM.ASM change_anim1/change_anim2 behavior: same script is a no-op
# unless MODE_END is set.  Secondary changes do not reset OBJ_GRAVITY.
import re
pat=re.compile(r'bool wm_source_anim_runtime_change\(wm_source_anim_runtime_t\*s,wm_arcade_actor_t\*a,uint8_t roster,const char\*label\)\{.*?return true;\}',re.S)
m=pat.search(ct)
replacement=r'''bool wm_source_anim_runtime_change(wm_source_anim_runtime_t*s,wm_arcade_actor_t*a,uint8_t roster,const char*label){
    const wm_source_anim_services_t*v;
    const wm_source_anim_program_t*p;
    uint16_t mode;
    if(!s||!a||!label)return false;
    p=wm_source_anim_program_find(roster,label);
    if(!p)return false;
    mode=s->secondary?s->mode_shadow:a->anim_mode;
    /* ANIM.ASM change_anim1/2: do not restart an already-running identical script. */
    if(s->program==p && !(mode&WM_ARCADE_MODE_END))return true;
    v=s->services;
    s->program=p;s->pc=0;s->current_frame=0;s->instructions_executed=0;s->fault=0;s->services=v;
    if(s->secondary){s->mode_shadow=0;s->count_shadow=1;}
    else{a->anim_mode=0;a->ani_count=1;a->gravity=SRC_GRAVITY;}
    return true;
}'''
if 's->program==p && !(mode&WM_ARCADE_MODE_END)' not in ct:
    if not m: raise SystemExit('Combat2CZ runtime change function missing')
    ct=ct[:m.start()]+replacement+ct[m.end():]

# Keep the existing VM executor unchanged.  For ANIMODE2, temporarily present
# its own mode/count state to the executor, then restore the actor's primary
# channel.  All other actor side effects remain shared, matching source.
old_sig='void wm_source_anim_runtime_tick(wm_source_anim_runtime_t*s,wm_arcade_actor_t*a){unsigned budget=512;'
if old_sig in ct:
    ct=ct.replace(old_sig,'static void wm_source_anim_runtime_tick_impl(wm_source_anim_runtime_t*s,wm_arcade_actor_t*a){unsigned budget=512;',1)
elif 'wm_source_anim_runtime_tick_impl' not in ct:
    raise SystemExit('Combat2CZ runtime tick function anchor missing')
getter='const char *wm_source_anim_runtime_frame(const wm_source_anim_runtime_t*s){return s?s->current_frame:0;}\n'
wrapper=r'''void wm_source_anim_runtime_tick(wm_source_anim_runtime_t*s,wm_arcade_actor_t*a){
    uint16_t primary_mode;
    int32_t primary_count;
    if(!s||!a)return;
    if(!s->secondary){wm_source_anim_runtime_tick_impl(s,a);return;}
    primary_mode=a->anim_mode;
    primary_count=a->ani_count;
    a->anim_mode=s->mode_shadow;
    a->ani_count=s->count_shadow;
    wm_source_anim_runtime_tick_impl(s,a);
    s->mode_shadow=a->anim_mode;
    s->count_shadow=a->ani_count;
    a->anim_mode=primary_mode;
    a->ani_count=primary_count;
}
'''
if 'void wm_source_anim_runtime_tick(wm_source_anim_runtime_t*s,wm_arcade_actor_t*a){\n    uint16_t primary_mode;' not in ct:
    if getter not in ct: raise SystemExit('Combat2CZ runtime getter anchor missing')
    ct=ct.replace(getter,wrapper+getter,1)

# Mark the two torso VMs as the secondary ANIMODE2 channel at match init.
needle='''        wm_source_anim_runtime_init(&g.source_anim[i]);\n        wm_source_anim_runtime_init(&g.source_torso[i]);\n'''
repl=needle+'        wm_source_anim_runtime_set_secondary(&g.source_torso[i], true);\n'
if 'wm_source_anim_runtime_set_secondary(&g.source_torso[i], true);' not in rt:
    if needle not in rt: raise SystemExit('Combat2CZ runtime init loop anchor missing')
    rt=rt.replace(needle,repl,1)

h.write_text(ht); c.write_text(ct); r.write_text(rt)
print('Combat2CZ ANIM.ASM primary/secondary channel separation applied')
