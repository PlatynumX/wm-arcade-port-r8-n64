#!/usr/bin/env python3
from pathlib import Path
import sys
repo=Path(sys.argv[1]); p=repo/'src/fix39/wm_fix39_runtime.c'; t=p.read_text()
if '#include <stdio.h>' not in t:
    inc='#include \"wm_arcade_movement.h\"\n'
    if inc not in t: raise SystemExit('Combat2CZ stdio include anchor missing')
    t=t.replace(inc,inc+'#include <stdio.h>\n',1)
anchor='static void source_keep_attached(wm_arcade_actor_t *a, void *user)\n'
if anchor not in t: raise SystemExit('Combat2CZ execute_walk anchor missing')
impl=r'''/* Combat2CZ: direct WRESTLE.ASM::execute_walk/change_walk_anim/set_velocities.
   The earlier BN translation only set X/Z velocity and omitted the source
   change_anim1/change_anim2 calls.  That leaves a completed attack frame on
   screen while DRONE input keeps moving the actor -- the frozen-pose glide
   seen in the attract capture. */
static const char *source_walk_prefix(const wm_arcade_actor_t *a)
{
    static const char *const p[9]={"hrt","rzr","und","yok","shn","bam","dnk","dnk","lex"};
    if(!a || a->wrestler_num<0 || a->wrestler_num>8) return "dnk";
    return p[a->wrestler_num];
}
static const char *source_prefixed_anim(const wm_arcade_actor_t *a,const char *suffix)
{
    static char slots[4][64]; static unsigned slot;
    char *b=slots[(slot++)&3u];
    (void)snprintf(b,64,"%s_%s",source_walk_prefix(a),suffix?suffix:"");
    return b;
}
static const char *source_rotate_anim_label(const wm_arcade_actor_t *a)
{
    static const char *const s[4][4]={
      {"stand2_anim","2_to_4_turn_anim","2_to_6_turn_anim","2_to_8_turn_anim"},
      {"4_to_2_turn_anim","stand4_anim","4_to_6_turn_anim","4_to_8_turn_anim"},
      {"6_to_2_turn_anim","6_to_4_turn_anim","stand6_anim","6_to_8_turn_anim"},
      {"8_to_2_turn_anim","8_to_4_turn_anim","8_to_6_turn_anim","stand8_anim"}};
    int f=(int)wm_arcade_convert_facing((uint16_t)a->facing_dir)>>1;
    int n=(int)wm_arcade_convert_facing((uint16_t)a->new_facing_dir)>>1;
    if(f<0||f>3)f=0;if(n<0||n>3)n=f;
    return source_prefixed_anim(a,s[f][n]);
}
static const char *source_leg_walk_label(const wm_arcade_actor_t *a,int mi,int fi)
{
    static const char *const s[8][8]={
      {"walk1_f2_anim","walk1_f2_anim","walk1_f4_anim","walk1_f4_anim","walk1_f4_anim","walk1_f4_anim","walk1_f2_anim","walk1_f2_anim"},
      {"walk2_f2_anim","walk2_f2_anim","walk2_f2_anim","walk2_f4_anim","walk8_f4_anim","walk8_f4_anim","walk4_f2_anim","walk4_f2_anim"},
      {"walk2_f2_anim","walk2_f2_anim","walk2_f2_anim","walk4_f4_anim","walk4_f4_anim","walk8_f4_anim","walk6_f2_anim","walk6_f2_anim"},
      {"walk2_f2_anim","walk8_f2_anim","walk4_f4_anim","walk4_f4_anim","walk2_f4_anim","walk6_f4_anim","walk2_f2_anim","walk6_f2_anim"},
      {"walk5_f2_anim","walk5_f2_anim","walk5_f4_anim","walk5_f4_anim","walk5_f4_anim","walk5_f4_anim","walk5_f2_anim","walk5_f2_anim"},
      {"walk2_f2_anim","walk6_f2_anim","walk2_f2_anim","walk6_f4_anim","walk2_f4_anim","walk4_f4_anim","walk2_f2_anim","walk8_f2_anim"},
      {"walk2_f2_anim","walk6_f2_anim","walk6_f2_anim","walk8_f4_anim","walk4_f4_anim","walk4_f4_anim","walk2_f2_anim","walk2_f2_anim"},
      {"walk2_f2_anim","walk4_f2_anim","walk6_f2_anim","walk8_f4_anim","walk6_f4_anim","walk2_f4_anim","walk2_f2_anim","walk2_f2_anim"}};
    if(mi<0||mi>7||fi<0||fi>7)return 0;
    return source_prefixed_anim(a,s[mi][fi]);
}
static const char *source_torso_walk_label(const wm_arcade_actor_t *a,int f4,int n4)
{
    static const char *const s[4][4]={
      {"torso2_anim","2_to_4_turn2_anim","2_to_6_turn2_anim","2_to_8_turn2_anim"},
      {"4_to_2_turn2_anim","torso4_anim","4_to_6_turn2_anim","4_to_8_turn2_anim"},
      {"6_to_2_turn2_anim","6_to_4_turn2_anim","torso6_anim","6_to_8_turn2_anim"},
      {"8_to_2_turn2_anim","8_to_4_turn2_anim","8_to_6_turn2_anim","torso8_anim"}};
    if(f4<0||f4>3||n4<0||n4>3)return 0;
    return source_prefixed_anim(a,s[f4][n4]);
}
static void source_execute_walk(wm_arcade_actor_t *a, void *user)
{
    static const int32_t vel[8][2] = {
        {0, -0x3a000}, {0x31000, -0x31000}, {0x3a000, 0}, {0x31000, 0x31000},
        {0, 0x3a000}, {-0x31000, 0x31000}, {-0x3a000, 0}, {-0x31000, -0x31000}
    };
    uint16_t d; int idx,fi,f4,n4,ai; int32_t xv,zv; wm_arcade_actor_t *opp;
    (void)user; if(!a)return;

    /* WRESTLE.ASM: INTURN suppresses normal walk processing, but at stick rest
       still clears stale velocity.  Secondary INTURN is independent. */
    ai=actor_index(a);
    if((a->anim_mode&WM_ARCADE_MODE_INTURN) ||
       (ai>=0 && (g.source_torso[ai].mode_shadow&WM_ARCADE_MODE_INTURN))){
        if((a->move_dir&0x0f)==0){a->x_vel=0;a->z_vel=0;}
        return;
    }
    a->attack_type=0;
    d=(uint16_t)(a->move_dir&0x0fu);
    if(d==WM_MOVE_ZIP || d==3u || d==7u || d>=11u){
        a->move_dir=WM_MOVE_ZIP;a->x_vel=0;a->z_vel=0;
        common_anim_label(a,source_rotate_anim_label(a),0);
        return;
    }
    if(d&WM_MOVE_LEFT)a->obj_control|=WM_OBJ_FLIPH;
    else if(d&WM_MOVE_RIGHT)a->obj_control&=(uint16_t)~WM_OBJ_FLIPH;
    idx=(int)wm_arcade_convert_facing(d);
    if(idx<0||idx>7){a->x_vel=0;a->z_vel=0;return;}
    xv=vel[idx][0];zv=vel[idx][1];opp=a->smart_target;
    if(!a->walk_fast&&opp&&opp->player_mode!=WM_PMODE_ONGROUND&&opp->player_mode!=WM_PMODE_DEAD){
        uint16_t xpair=(uint16_t)((d|a->facing_dir)&(WM_MOVE_LEFT|WM_MOVE_RIGHT));
        uint16_t zpair=(uint16_t)((d|a->facing_dir)&(WM_MOVE_UP|WM_MOVE_DOWN));
        if(xpair==(WM_MOVE_LEFT|WM_MOVE_RIGHT))xv=(xv*230)>>8;
        if(zpair==(WM_MOVE_UP|WM_MOVE_DOWN))zv=(zv*230)>>8;
    }else if(!a->walk_fast&&opp&&(opp->player_mode==WM_PMODE_ONGROUND||opp->player_mode==WM_PMODE_DEAD))xv=(xv*384)>>8;
    a->x_vel=xv;a->z_vel=zv;

    /* WRESTLE.ASM::change_walk_anim.  Directional label matrices are common
       across all eight characters; only the source prefix differs. */
    a->consecutive_hits=0;
    a->ani_speed=(uint16_t)((a->walk_fast || (opp&&opp->player_mode==WM_PMODE_ONGROUND))?0xcd:0x100);
    fi=(int)wm_arcade_convert_facing((uint16_t)a->facing_dir);
    f4=fi>>1;n4=((int)wm_arcade_convert_facing((uint16_t)a->new_facing_dir))>>1;
    ai=actor_index(a);
    if(ai>=0 && !(g.source_torso[ai].mode_shadow&WM_ARCADE_MODE_UNINT))
        common_torso_label(a,source_torso_walk_label(a,f4,n4),0);
    common_anim_label(a,source_leg_walk_label(a,idx,fi),0);
}

'''
# replace any previously injected execute_walk block, otherwise insert
start=t.find('/* Combat2BN: direct WRESTLE.ASM::execute_walk/set_velocities translation.')
if start<0: start=t.find('/* Combat2CZ: direct WRESTLE.ASM::execute_walk/change_walk_anim/set_velocities.')
if start>=0:
    end=t.find(anchor,start)
    if end<0: raise SystemExit('Combat2CZ existing execute_walk block malformed')
    t=t[:start]+impl+t[end:]
elif 'static void source_execute_walk(' not in t:
    t=t.replace(anchor,impl+anchor,1)
# wire all callback surfaces idempotently
for name in ['common_callbacks','bret_callbacks','razor_callbacks']:
    old='static const '
# generic table insertion
import re
for name in ['common_callbacks','bret_callbacks','razor_callbacks']:
    m=re.search(r'(static const [^{]+\b'+re.escape(name)+r'\s*=\s*\{\n)',t)
    if not m: raise SystemExit('Combat2CZ callback anchor missing '+name)
    end=t.find('};',m.end()); block=t[m.start():end]
    if '.execute_walk = source_execute_walk' not in block:
        t=t[:m.end()]+'    .execute_walk = source_execute_walk,\n'+t[m.end():]
p.write_text(t)
print('Combat2CZ WRESTLE.ASM execute_walk/change_walk_anim/set_velocities wired for all wrestlers')
