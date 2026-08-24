#!/usr/bin/env python3
from pathlib import Path
import sys
repo=Path(sys.argv[1])
p=repo/'src/fix39/wm_fix39_runtime.c'
vm=repo/'src/fix39/wm_arcade_source_animation_runtime.c'
t=p.read_text()
v=vm.read_text()

# 1) ANIM.ASM opcode 0 is ANI_ZIP/no-op, not ANI_END.
v=v.replace('case 0: a->anim_mode|=WM_ARCADE_MODE_END; return 1; /* ZIP is a return address in source; portable VM ends this stream. */',
            'case 0: NEXT(); /* ANIM.ASM _ani_zip: no-op, continue at the next command. */')
# ANIM.ASM _ani_waitroll: I_WILL_DIE is resolved instead of becoming a
# permanent HOLD state.  Ordinary knockdowns remain MODE_ONGROUND until the
# source recovery timers expire and do_roll returns zero.
v=v.replace('case 84: if(a->player_mode!=WM_PMODE_DEAD&&!a->i_will_die){a->player_mode=WM_PMODE_ONGROUND;if(a->immobilize_time||a->getup_time)HOLD();a->stars_flag=0;if(s->services&&s->services->do_roll&&s->services->do_roll(a,s->services->user))HOLD();NEXT();}HOLD();',
'''case 84:
        if(a->i_will_die){
            if(a->immobilize_time)HOLD();
            a->i_will_die=0; a->player_mode=WM_PMODE_DEAD;
        }
        if(a->player_mode==WM_PMODE_DEAD){
            if(change_program(s,a,"xxx_dead_anim",0))return 0;
            a->anim_mode|=WM_ARCADE_MODE_END; return 1;
        }
        a->player_mode=WM_PMODE_ONGROUND;
        if(a->immobilize_time||a->getup_time)HOLD();
        a->stars_flag=0;
        if(s->services&&s->services->do_roll&&s->services->do_roll(a,s->services->user))HOLD();
        NEXT();''')
vm.write_text(v)

# 2) Keep one bound service table with the live runtime.  Earlier CE/CH generated
# a complete VM but never bound wm_source_anim_services_t, making ANI_CODE,
# RNG, DAMAGEOPP, change-other-animation, rope, and timing services silent no-ops.
state_anchor='    wm_source_anim_runtime_t source_torso[WM_FIX39_ACTOR_COUNT];\n'
if 'wm_source_anim_services_t source_anim_services;' not in t:
    if state_anchor not in t: raise SystemExit('source animation state anchor missing')
    t=t.replace(state_anchor,state_anchor+'    wm_source_anim_services_t source_anim_services;\n',1)

# source-native service surface. Keep gameplay semantics source-backed; visual/audio
# callbacks that do not yet have N64 backends only record their source event.
insert_anchor='static bool live_start_source_anim(wm_arcade_actor_t *a, const char *label, bool torso)\n'
if 'static void source_anim_code(' not in t:
    pos=t.find(insert_anchor)
    if pos<0: raise SystemExit('live_start_source_anim anchor missing')
    helper=r'''static uint16_t source_anim_round_tick(void *user)
{
    (void)user; return g.status.round_tickcount;
}
static uint32_t source_anim_pcnt(void *user)
{
    (void)user; return g.status.pcnt;
}
static uint32_t source_anim_rndrng0(uint32_t max_inclusive, void *user)
{
    (void)user; return wm_rng_rndrng0(&g.rng,max_inclusive);
}
static int source_anim_rndper_hi(uint16_t probability, void *user)
{
    (void)user; return live_rndper_hi(probability,&g.rng);
}
static void source_anim_sound(wm_arcade_actor_t *a,const char *token,int32_t raw,void *user)
{
    WmFix39ActorTrace *tr=trace_for(a); (void)user;
    if(tr){tr->sound_label=token;tr->sound_token=raw;++tr->sound_events;}
}
static void source_anim_native_am_i_dead(wm_arcade_actor_t *a)
{
    if(!a)return;
    a->anim_mode &= (uint16_t)~WM_ARCADE_MODE_STATUS;
    if(a->life<=0){a->anim_mode|=WM_ARCADE_MODE_STATUS;a->player_mode=WM_PMODE_DEAD;}
    else if(a->player_mode==WM_PMODE_DEAD)a->anim_mode|=WM_ARCADE_MODE_STATUS;
}
static void source_anim_native_ckzpos(wm_arcade_actor_t *a)
{
    if(!a)return;
    /* DNKSEQ2/HRT/RZR/... ckzpos: slide toward ring middle near front/rear ropes. */
    if(a->z_int>0x510)a->z_vel=-0x24000;
    else if(a->z_int<=0x442)a->z_vel=0x24000;
}
static void source_anim_native_no_bk_xvel(wm_arcade_actor_t *a)
{
    int32_t x; if(!a)return; x=a->x_vel;
    if(!(a->facing_dir&WM_MOVE_RIGHT))x=-x;
    if(x<0)a->x_vel=0;
}
static void source_anim_native_choose_2or4(wm_arcade_actor_t *a)
{
    if(!a) return;
    a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;
    if(!(a->new_facing_dir&WM_MOVE_UP))a->anim_mode|=WM_ARCADE_MODE_STATUS;
}
static void source_anim_native_make_norm(wm_arcade_actor_t *a)
{
    if(!a)return;
    /* Source make_norm restores normal drawable object mode.  Preserve FLIPH and
       combat-visible flags; clear temporary invisible/ghost animation state. */
    a->anim_mode&=(uint16_t)~(WM_ARCADE_MODE_INVISIBLE|WM_ARCADE_MODE_GHOST);
}
static void source_anim_code(wm_arcade_actor_t *a,const char *label,void *user)
{
    (void)user; if(!a||!label)return;
    /* State-affecting ANI_CODE callbacks used by the eight shipped wrestlers.
       Audio/visual-only callbacks are recorded by source_anim_sound elsewhere. */
    if(!strcmp(label,"am_I_dead")){source_anim_native_am_i_dead(a);return;}
    if(!strcmp(label,"ckzpos")){source_anim_native_ckzpos(a);return;}
    if(!strcmp(label,"no_bk_xvel")){source_anim_native_no_bk_xvel(a);return;}
    if(!strcmp(label,"choose_2or4")){source_anim_native_choose_2or4(a);return;}
    if(!strcmp(label,"make_norm")){source_anim_native_make_norm(a);return;}
    if(!strcmp(label,"DO_CROWD_CHEER")){WmFix39ActorTrace *tr=trace_for(a);if(tr)++tr->external_special_events;return;}
    if(!strcmp(label,"HIT_THE_MAT")||!strcmp(label,"SMALL_BOUNCE")||
       !strcmp(label,"SMALL_RUN")||!strcmp(label,"impact_sound")||
       !strcmp(label,"CALL_MISSES")||!strcmp(label,"DO_GRUNT")||
       !strcmp(label,"DO_WAIL")||!strcmp(label,"DO_SCREAM")||
       !strcmp(label,"MAKE_HIM_SCREAM")){
        source_anim_sound(a,label,0,0); return;
    }
    /* Preserve the native label for diagnostics instead of silently pretending
       the call executed.  Native routines outside ANIM.ASM remain separately
       auditable source services. */
    {WmFix39ActorTrace *tr=trace_for(a);if(tr){tr->external_special_label=label;++tr->external_special_events;}}
}
static bool source_anim_change_other(wm_arcade_actor_t *a,wm_arcade_actor_t *o,const char *label,void *user)
{
    int i=actor_index(o);(void)a;(void)user;if(i<0||!label)return false;
    if(!wm_source_anim_runtime_change(&g.source_anim[i],o,(uint8_t)o->wrestler_num,label))return false;
    wm_source_anim_runtime_tick(&g.source_anim[i],o); return true;
}
static bool source_anim_force_other(wm_arcade_actor_t *a,wm_arcade_actor_t *o,const char *frame,void *user)
{
    int i=actor_index(o);(void)a;(void)user;if(i<0||!frame)return false;
    wm_source_anim_runtime_force_frame(&g.source_anim[i],frame);return true;
}
static int source_anim_do_roll(wm_arcade_actor_t *a,void *user)
{
    uint16_t d;(void)user;if(!a)return 0;d=a->stick_val_cur&(WM_MOVE_UP|WM_MOVE_DOWN);
    if(!d){a->z_vel=0;return 0;}
    /* WRESTLE2.ASM do_roll: direction controls sign.  Character-specific roll
       image progression remains owned by the animation frame table. */
    a->z_vel=(d&WM_MOVE_DOWN)?0x18000:-0x18000;return 1;
}
static int source_anim_buttons_down(wm_arcade_actor_t *a,void *user)
{
    (void)user;return a?(int)(a->but_val_down|a->stick_val_down):0;
}
static void source_anim_allow_offscreen(int ticks,void *user)
{
    (void)user;g.allow_offscreen=(uint16_t)(ticks<0?0:ticks);
}
static void init_source_anim_services(void)
{
    memset(&g.source_anim_services,0,sizeof(g.source_anim_services));
    g.source_anim_services.round_tick=source_anim_round_tick;
    g.source_anim_services.pcnt=source_anim_pcnt;
    g.source_anim_services.rndrng0=source_anim_rndrng0;
    g.source_anim_services.rndper_hi=source_anim_rndper_hi;
    g.source_anim_services.sound=source_anim_sound;
    g.source_anim_services.code=source_anim_code;
    g.source_anim_services.change_other_anim=source_anim_change_other;
    g.source_anim_services.force_other_frame=source_anim_force_other;
    g.source_anim_services.do_roll=source_anim_do_roll;
    g.source_anim_services.buttons_down=source_anim_buttons_down;
    g.source_anim_services.set_allow_offscreen=source_anim_allow_offscreen;
    g.source_anim_services.combat_runtime=&g.combat_runtime;
    g.source_anim_services.react=&g.react_callbacks;
    g.source_anim_services.user=&g;
}

'''
    t=t[:pos]+helper+t[pos:]

# Source change_anim1a/change_anim2a execute animate immediately on restart.
old='''static bool live_start_source_anim(wm_arcade_actor_t *a, const char *label, bool torso)
{
    int i=actor_index(a);
    if(i<0||!label)return false;
    return wm_source_anim_runtime_change(torso?&g.source_torso[i]:&g.source_anim[i],a,(uint8_t)a->wrestler_num,label);
}
'''
new='''static bool live_start_source_anim(wm_arcade_actor_t *a, const char *label, bool torso)
{
    int i=actor_index(a); wm_source_anim_runtime_t *rt; bool ok;
    if(i<0||!label)return false;
    rt=torso?&g.source_torso[i]:&g.source_anim[i];
    ok=wm_source_anim_runtime_change(rt,a,(uint8_t)a->wrestler_num,label);
    if(ok)wm_source_anim_runtime_tick(rt,a); /* ANIM.ASM change_anim1a/2a */
    return ok;
}
'''
if old in t:t=t.replace(old,new,1)
elif 'ok=wm_source_anim_runtime_change' not in t: raise SystemExit('live_start_source_anim body mismatch')

# Initialize service table after combat/reaction callback surfaces exist.
init_anchor='    g.special_callbacks.react1 = &g.react1_context;\n'
if 'init_source_anim_services();' not in t:
    if init_anchor not in t: raise SystemExit('combat callback init anchor missing')
    t=t.replace(init_anchor,init_anchor+'    init_source_anim_services();\n',1)

# Bind every primary/secondary VM after reset, before ani_init/change_anim.
bind_anchor='''        wm_source_anim_runtime_init(&g.source_anim[i]);
        wm_source_anim_runtime_init(&g.source_torso[i]);
'''
if 'wm_source_anim_runtime_bind(&g.source_anim[i],&g.source_anim_services);' not in t:
    repl=bind_anchor+'''        wm_source_anim_runtime_bind(&g.source_anim[i],&g.source_anim_services);
        wm_source_anim_runtime_bind(&g.source_torso[i],&g.source_anim_services);
'''
    if bind_anchor not in t: raise SystemExit('match source anim init anchor missing')
    t=t.replace(bind_anchor,repl,1)

# The VM's ANI_ATTACK_ON/OFF commands are authoritative.  Keep only exact WIMP
# hurt-box extraction here; frame-name attack reconstruction duplicates/overrides
# the source command stream and can hold CHECKHIT across reactions.
start=t.find('    /* ANIM/COLLIS source ownership: current animation frame now drives both')
end=t.find('    /* WRESTLE.ASM update_links:',start)
if start<0 or end<0: raise SystemExit('source frame attack block missing')
newblock='''    /* ANIM.ASM owns attack windows through ANI_ATTACK_ON/OFF.  The current
       WIMP frame contributes only exact IANI3 hurt-box geometry. */
    for (i=0u;i<WM_FIX39_ACTOR_COUNT;++i) {
        wm_arcade_actor_t *a=&g.actors[i];
        const char *frame=wm_source_anim_runtime_frame(&g.source_anim[i]);
        const wm_source_sprite *spr=frame?wm_character_sprite_find((uint8_t)a->wrestler_num,frame):0;
        wm_arcade_frame_box_t fb;
        if(spr && wm_arcade_wimp_frame_box_from_sprite(spr,&fb)){g.frame_box[i]=fb;g.frame_box_valid[i]=true;}
        else g.frame_box_valid[i]=false;
    }
'''
t=t[:start]+newblock+t[end:]

# Replace the BY forced-stand shortcut with WRESTLE.ASM's actual GETUP_TIME tail.
old_start=t.find('    /* wrestler_main per-tick countdown tail.')
old_end=t.find('    /* SPECIAL.ASM process state',old_start)
if old_start<0 or old_end<0: raise SystemExit('wrestler countdown block missing')
countdown=r'''    /* WRESTLE.ASM wrestler_main countdown tail.  GETUP_TIME is recovery
       state only; ANI_WAITROLL/ANI_GETUP_WAIT and ANI_CHANGEANIM own the actual
       get-up animation.  Do not force PLYRMODE/stand from this loop. */
    for (i = 0u; i < WM_FIX39_ACTOR_COUNT; ++i) {
        wm_arcade_actor_t *a = &g.actors[i];
        if (a->delay_butns > 0) --a->delay_butns;
        if (a->safe_time > 0) --a->safe_time;
        if (a->delay_meter > 0) --a->delay_meter;
        if (a->immobilize_time > 0) --a->immobilize_time;
        if (a->walk_fast > 0) --a->walk_fast;
        if (a->getup_time > 0) {
            /* Source rejects a newly-set recovery while DELAY_METER is active. */
            if (a->delay_meter > 0) a->getup_time = 0;
            else {
                --a->getup_time;
                if (a->getup_time > 0) {
                    uint16_t pressed=(uint16_t)(a->but_val_down|a->stick_val_down);
                    uint32_t sf=a->status_flags;
                    bool press_last=(sf&WM_STATUS_PRESS_LAST)!=0;
                    if(pressed)sf|=WM_STATUS_PRESS_LAST;else sf&=~WM_STATUS_PRESS_LAST;
                    a->status_flags=sf;
                    if(pressed||press_last){a->getup_time-=3;if(a->getup_time<0)a->getup_time=0;}
                }
            }
            if(a->getup_time==0){a->dizzy=0;a->stars_flag=0;a->delay_butns=40;}
        }
    }

'''
t=t[:old_start]+countdown+t[old_end:]

# Update stale seam comment.
t=t.replace('the complete ANIM command stream and platform presentation/\n       audio backends.',
            'native ANI_CODE routines outside ANIM.ASM and platform presentation/\n       audio backends.')
p.write_text(t)
print('Combat2CI source-vs-port combat audit corrections applied')
