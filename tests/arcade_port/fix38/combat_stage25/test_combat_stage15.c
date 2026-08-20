#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wm_arcade_razor.h"
#include "wm_arcade_razor_tables.h"

typedef struct tr {
    int anims,sounds,secrets,bonus,reversals,kills;
    wm_arcade_razor_anim_id_t anim;
    wm_arcade_razor_sound_id_t sound;
    int bonus_id;
} tr_t;
static void an(wm_arcade_actor_t*a,wm_arcade_razor_anim_id_t x,void*u){(void)a;tr_t*t=u;t->anims++;t->anim=x;}
static void so(wm_arcade_actor_t*a,wm_arcade_razor_sound_id_t x,void*u){(void)a;tr_t*t=u;t->sounds++;t->sound=x;}
static void sec(wm_arcade_actor_t*a,const wm_arcade_razor_secret_pattern_t*p,size_t n,void*u){(void)a;(void)p;assert(n==6);((tr_t*)u)->secrets++;}
static int ign(wm_arcade_actor_t*a,void*u){(void)a;(void)u;return 0;}
static int ignrev(wm_arcade_actor_t*a,wm_arcade_actor_t*b,void*u){(void)a;(void)b;(void)u;return 0;}
static int combo_ok(wm_arcade_actor_t*a,void*u){(void)a;(void)u;return 0;}
static void bonus(wm_arcade_actor_t*a,int b,void*u){(void)a;tr_t*t=u;t->bonus++;t->bonus_id=b;}
static void rev(wm_arcade_actor_t*a,void*u){(void)a;((tr_t*)u)->reversals++;}
static void killend(wm_arcade_actor_t*a,void*u){(void)a;((tr_t*)u)->kills++;}

int main(void){
    wm_arcade_actor_t a,o;
    wm_arcade_razor_env_t e={1000,0,0,0,0};
    wm_arcade_razor_callbacks_t c;
    tr_t t;
    int32_t x,z;
    memset(&a,0,sizeof(a));memset(&o,0,sizeof(o));memset(&c,0,sizeof(c));memset(&t,0,sizeof(t));
    c.change_anim=an;c.sound=so;c.check_secret_moves=sec;c.ck_ignore=ign;c.ck_ignore_reversed=ignrev;
    c.check_combo_go=combo_ok;c.bonus_message=bonus;c.do_reversal=rev;c.do_reversal_message=rev;c.find_and_kill_endless=killend;c.user=&t;

    a.player_mode=WM_PMODE_NORMAL;a.facing_dir=WM_MOVE_UP_RIGHT;a.new_facing_dir=WM_MOVE_RIGHT;
    a.closest_xdist=40;a.closest_zdist=20;a.but_val_down=WM_BTN_PUNCH;o.player_mode=WM_PMODE_NORMAL;
    assert(wm_arcade_move_razor(&a,&o,&e,&c)==WM_RZR_STEP_ACTION);
    assert(t.anim==WM_RZR_ANIM_BUTT2&&t.sound==WM_RZR_SND_HDBUTT&&t.secrets==1);

    memset(&t,0,sizeof(t));a.player_mode=WM_PMODE_NORMAL;a.but_val_down=WM_BTN_SPUNCH;a.stick_val_cur=WM_MOVE_DOWN;a.closest_xdist=30;a.closest_zdist=20;
    wm_arcade_move_razor(&a,&o,&e,&c);assert(t.anim==WM_RZR_ANIM_UPPERCUT4&&t.sound==WM_RZR_SND_UPRCUT);

    memset(&t,0,sizeof(t));a.player_mode=WM_PMODE_NORMAL;a.but_val_down=WM_BTN_SPUNCH;a.stick_val_cur=0;a.closest_xdist=100;a.closest_zdist=20;
    wm_arcade_move_razor(&a,&o,&e,&c);assert(t.anim==WM_RZR_ANIM_USLASH3);

    memset(&t,0,sizeof(t));a.player_mode=WM_PMODE_NORMAL;a.but_val_down=WM_BTN_SKICK;a.closest_xdist=20;a.closest_zdist=20;a.new_facing_dir=WM_MOVE_DOWN_RIGHT;a.stick_val_cur=WM_MOVE_RIGHT;
    wm_arcade_move_razor(&a,&o,&e,&c);assert(t.anim==WM_RZR_ANIM_KNEE_FALL4&&t.sound==WM_RZR_SND_GRABHOLD);

    memset(&t,0,sizeof(t));a.player_mode=WM_PMODE_RUNNING;a.but_val_down=WM_BTN_PUNCH;a.getup_time=0;a.delay_butns=0;a.usr_var1=0;a.move_dir=WM_MOVE_RIGHT;a.facing_dir=WM_MOVE_RIGHT;a.new_facing_dir=WM_MOVE_RIGHT;a.stick_val_cur=0;a.closest_xdist=80;o.player_mode=WM_PMODE_NORMAL;
    wm_arcade_move_razor(&a,&o,&e,&c);assert(a.x_vel==WM_RZR_XRUN&&a.run_time==0&&a.player_mode==WM_PMODE_NORMAL&&t.anim==WM_RZR_ANIM_USLASH3&&t.sound==WM_RZR_SND_GRABHOLD);

    memset(&t,0,sizeof(t));a.player_mode=WM_PMODE_RUNNING;a.but_val_down=WM_BTN_KICK;a.move_dir=WM_MOVE_RIGHT;a.facing_dir=WM_MOVE_RIGHT;a.new_facing_dir=WM_MOVE_RIGHT;a.closest_xdist=80;o.player_mode=WM_PMODE_NORMAL;
    wm_arcade_move_razor(&a,&o,&e,&c);assert(a.player_mode==WM_PMODE_INAIR&&t.anim==WM_RZR_ANIM_FLYING_KICK);

    memset(&t,0,sizeof(t));memset(&a,0,sizeof(a));memset(&o,0,sizeof(o));a.player_mode=WM_PMODE_NORMAL;o.player_mode=WM_PMODE_NORMAL;
    assert(wm_arcade_razor_release_charge_flying_kick(&a,&o,84,&c)==0);
    assert(wm_arcade_razor_release_charge_flying_kick(&a,&o,85,&c)==1);assert(a.player_mode==WM_PMODE_INAIR&&a.x_vel==1&&t.anim==WM_RZR_ANIM_FLYING_KICK);

    /* RAZOR.ASM reloads GETUP_TIME into a0 before comparing a0 to MODE_ONGROUND/DEAD.
       With GETUP_TIME==0 those two source comparisons are ineffective; preserve that quirk. */
    memset(&t,0,sizeof(t));memset(&a,0,sizeof(a));a.player_mode=WM_PMODE_ONGROUND;a.getup_time=0;o.player_mode=WM_PMODE_NORMAL;
    assert(wm_arcade_razor_release_charge_flying_kick(&a,&o,85,&c)==1);assert(a.player_mode==WM_PMODE_INAIR);

    memset(&t,0,sizeof(t));memset(&a,0,sizeof(a));a.player_mode=WM_PMODE_NORMAL;
    assert(wm_arcade_razor_release_charge_slashes(&a,99,&c)==0);assert(wm_arcade_razor_release_charge_slashes(&a,100,&c)==1);assert(a.special_move_addr==(uintptr_t)WM_RZR_ANIM_REPEAT_SLASH&&t.sound==WM_RZR_SND_KICK_T2);

    memset(&t,0,sizeof(t));memset(&a,0,sizeof(a));memset(&o,0,sizeof(o));a.player_mode=WM_PMODE_NORMAL;a.last_headhold=950;a.closest_xdist=20;o.player_mode=WM_PMODE_NORMAL;
    assert(wm_arcade_razor_fire_secret(&a,&o,WM_RZR_SECRET_NECK_GRAB,1000,&c)==1);assert(t.anim==WM_RZR_ANIM_FAKE_HOLD3);
    memset(&t,0,sizeof(t));a.last_headhold=0;a.closest_xdist=80;assert(wm_arcade_razor_fire_secret(&a,&o,WM_RZR_SECRET_NECK_GRAB,1000,&c)==1);assert(t.anim==WM_RZR_ANIM_HEAD_HOLD2_3);

    memset(&t,0,sizeof(t));memset(&a,0,sizeof(a));memset(&o,0,sizeof(o));a.player_mode=WM_PMODE_HEADHOLD;a.who_i_hit=&o;
    assert(wm_arcade_razor_fire_monitor(&a,&o,WM_RZR_MON_HEADHOLD_EDGE,&e,0,&c)==1);assert(a.smart_target==&o&&o.immobilize_time==15&&a.special_move_addr==(uintptr_t)WM_RZR_ANIM_RAZORS_EDGE&&t.bonus_id==33);
    o.immobilize_time=0;memset(&t,0,sizeof(t));a.player_mode=WM_PMODE_HEADHOLD;a.immobilize_time=0;a.who_i_hit=&o;
    assert(wm_arcade_razor_fire_monitor(&a,&o,WM_RZR_MON_HEADHOLD_PILE,&e,0,&c)==1);assert(t.bonus_id==7&&a.special_move_addr==(uintptr_t)WM_RZR_ANIM_PILE_DRIVER3);
    o.immobilize_time=0;memset(&t,0,sizeof(t));a.player_mode=WM_PMODE_HEADHOLD;a.immobilize_time=0;
    assert(wm_arcade_razor_fire_monitor(&a,&o,WM_RZR_MON_HEADHOLD_RUG,&e,0,&c)==1);assert(t.bonus_id==6&&a.special_move_addr==(uintptr_t)WM_RZR_ANIM_RUGSHAKE2);

    memset(&t,0,sizeof(t));memset(&a,0,sizeof(a));memset(&o,0,sizeof(o));a.player_mode=WM_PMODE_HEADHELD;a.who_hit_me=&o;
    assert(wm_arcade_razor_fire_monitor(&a,&o,WM_RZR_MON_HEADHOLD_EDGE,&e,0,&c)==1);assert(a.smart_target==&o&&o.immobilize_time==15&&t.reversals==2);

    memset(&a,0,sizeof(a));memset(&o,0,sizeof(o));a.player_mode=WM_PMODE_NORMAL;a.closest_dist=0x90;o.player_mode=WM_PMODE_INAIR;
    assert(wm_arcade_razor_fire_monitor(&a,&o,WM_RZR_MON_GRAB_TOSS_AIR,&e,0,&c)==1);assert(a.special_move_addr==(uintptr_t)WM_RZR_ANIM_HIPTOSS2_4||a.special_move_addr==(uintptr_t)WM_RZR_ANIM_HIPTOSS2_2);

    assert(wm_arcade_razor_secret_patterns[0].id==WM_RZR_SECRET_NECK_GRAB&&wm_arcade_razor_secret_patterns[0].max_ticks==30);
    assert(wm_arcade_razor_secret_patterns[5].id==WM_RZR_SECRET_DOWN_SLASH&&wm_arcade_razor_secret_patterns[5].max_ticks==50);
    assert(wm_arcade_razor_monitor_patterns[4].id==WM_RZR_MON_GRAB_TOSS_AIR&&wm_arcade_razor_monitor_patterns[4].max_ticks==40);
    wm_arcade_razor_velocity_for_dir(7,&x,&z);assert(x==-WM_RZR_WALK_DVEL&&z==-WM_RZR_WALK_DVEL);
    assert(strcmp(wm_arcade_razor_rotate_anim_labels[0][1],"rzr_2_to_4_turn_anim")==0);
    assert(strcmp(wm_arcade_razor_leg_anim_labels[7][4],"rzr_walk6_f4_anim")==0);
    assert(strcmp(wm_arcade_razor_torso_anim_labels[3][3],"rzr_torso8_anim")==0);

    puts("Stage 15 Razor-specific move/input tests: PASS");
    return 0;
}
