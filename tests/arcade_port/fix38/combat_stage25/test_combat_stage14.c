#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wm_arcade_bret.h"
#include "wm_arcade_bret_tables.h"

typedef struct tr { int anims,sounds,walks,secrets; wm_arcade_bret_anim_id_t anim; wm_arcade_bret_sound_id_t sound; } tr_t;
static void an(wm_arcade_actor_t*a,wm_arcade_bret_anim_id_t x,void*u){(void)a;tr_t*t=u;t->anims++;t->anim=x;}
static void so(wm_arcade_actor_t*a,wm_arcade_bret_sound_id_t x,void*u){(void)a;tr_t*t=u;t->sounds++;t->sound=x;}
static void wa(wm_arcade_actor_t*a,void*u){(void)a;((tr_t*)u)->walks++;}
static void sec(wm_arcade_actor_t*a,const wm_arcade_bret_secret_pattern_t*p,size_t n,void*u){(void)a;(void)p;assert(n==8);((tr_t*)u)->secrets++;}
static int ign(wm_arcade_actor_t*a,void*u){(void)a;(void)u;return 0;}
static int combo_ok(wm_arcade_actor_t*a,void*u){(void)a;(void)u;return 0;}
int main(void){
 wm_arcade_actor_t a,o; tr_t t; wm_arcade_bret_env_t e={1000,0,0,0,0}; wm_arcade_bret_callbacks_t c;
 memset(&a,0,sizeof(a));memset(&o,0,sizeof(o));memset(&t,0,sizeof(t));memset(&c,0,sizeof(c)); c.change_anim=an;c.sound=so;c.execute_walk=wa;c.check_secret_moves=sec;c.ck_ignore=ign;c.check_combo_go=combo_ok;c.user=&t;
 a.player_mode=WM_PMODE_NORMAL;a.facing_dir=WM_MOVE_UP_RIGHT;a.new_facing_dir=WM_MOVE_RIGHT;a.closest_xdist=40;a.closest_zdist=20;a.but_val_down=WM_BTN_PUNCH;o.player_mode=WM_PMODE_NORMAL;
 assert(wm_arcade_move_bret(&a,&o,&e,&c)==WM_BRET_STEP_ACTION);assert(t.anim==WM_BRET_ANIM_BUTT2&&t.sound==WM_BRET_SND_HDBUTT&&t.secrets==1);
 memset(&t,0,sizeof(t));a.player_mode=WM_PMODE_NORMAL;a.but_val_down=WM_BTN_SPUNCH;a.stick_val_cur=WM_MOVE_DOWN;a.closest_xdist=30;a.closest_zdist=20;
 wm_arcade_move_bret(&a,&o,&e,&c);assert(t.anim==WM_BRET_ANIM_UPPERCUT4&&t.sound==WM_BRET_SND_UPRCUT);
 memset(&t,0,sizeof(t));a.player_mode=WM_PMODE_RUNNING;a.but_val_down=WM_BTN_KICK;a.getup_time=0;a.delay_butns=0;a.usr_var1=0;a.move_dir=WM_MOVE_RIGHT;a.facing_dir=WM_MOVE_RIGHT;a.new_facing_dir=WM_MOVE_RIGHT;a.stick_val_cur=0;
 wm_arcade_move_bret(&a,&o,&e,&c);assert(a.run_time==1&&a.x_vel==WM_BRET_XRUN&&a.player_mode==WM_PMODE_INAIR&&t.anim==WM_BRET_ANIM_FLYING_KICK);
 memset(&t,0,sizeof(t));a.player_mode=WM_PMODE_BLOCK;a.block_time=159;a.but_val_down=0;a.closest_xdist=50;a.closest_zdist=20;o.player_mode=WM_PMODE_BLOCK;
 wm_arcade_move_bret(&a,&o,&e,&c);assert(a.player_mode==WM_PMODE_NORMAL&&t.anim==WM_BRET_ANIM_PUSH4&&t.sound==WM_BRET_SND_PUSH);
 memset(&t,0,sizeof(t));a.player_mode=WM_PMODE_NORMAL;a.anim_mode=0;a.last_headhold=950;a.closest_xdist=20;o.player_mode=WM_PMODE_NORMAL;
 assert(wm_arcade_bret_fire_secret(&a,&o,WM_BRET_SECRET_NECK_GRAB,1000,&c)==1);assert(t.anim==WM_BRET_ANIM_FAKE_HOLD3);
 memset(&t,0,sizeof(t));a.player_mode=WM_PMODE_NORMAL;a.getup_time=0;a.anim_mode=0;o.player_mode=WM_PMODE_NORMAL;
 assert(wm_arcade_bret_release_charge_flying_kick(&a,&o,99,&c)==0);assert(wm_arcade_bret_release_charge_flying_kick(&a,&o,100,&c)==1);assert(a.player_mode==WM_PMODE_INAIR&&a.special_move_addr==(uintptr_t)WM_BRET_ANIM_FLYING_KICK);
 int32_t x,z;wm_arcade_bret_velocity_for_dir(7,&x,&z);assert(x==-WM_BRET_WALK_DVEL&&z==-WM_BRET_WALK_DVEL);
 assert(wm_arcade_bret_secret_patterns[7].id==WM_BRET_SECRET_SUPERCUT&&wm_arcade_bret_secret_patterns[7].max_ticks==16);
 a.but_val_up=WM_BTN_SPUNCH;assert(wm_arcade_bret_try_charge_ddt(&a,&o,99,&c)==0);
 assert(strcmp(wm_arcade_bret_rotate_anim_labels[0][1],"hrt_2_to_4_turn_anim")==0);
 assert(strcmp(wm_arcade_bret_leg_anim_labels[7][4],"hrt_walk6_f4_anim")==0);
 assert(strcmp(wm_arcade_bret_torso_anim_labels[3][3],"hrt_torso8_anim")==0);
 assert(wm_arcade_bret_monitor_patterns[0].id==WM_BRET_MON_ROLL_UPPERCUT);
 assert(wm_arcade_bret_monitor_patterns[6].max_ticks==40);
 memset(&a,0,sizeof(a));memset(&o,0,sizeof(o));a.player_mode=WM_PMODE_NORMAL;a.run_time=77;
 assert(wm_arcade_bret_fire_monitor(&a,&o,WM_BRET_MON_ROLL_UPPERCUT,&e,0,&c)==1);
 assert(a.special_move_addr==(uintptr_t)WM_BRET_ANIM_ROLL_UPPERCUT&&a.run_time==0);
 memset(&a,0,sizeof(a));memset(&o,0,sizeof(o));a.player_mode=WM_PMODE_HEADHOLD;a.who_i_hit=&o;
 assert(wm_arcade_bret_fire_monitor(&a,&o,WM_BRET_MON_HEADHOLD_PILE,&e,0,&c)==1);
 assert(a.smart_target==&o&&o.immobilize_time==15&&a.special_move_addr==(uintptr_t)WM_BRET_ANIM_PILE_DRIVER3);
 memset(&a,0,sizeof(a));memset(&o,0,sizeof(o));a.player_mode=WM_PMODE_NORMAL;a.closest_dist=0x80;o.player_mode=WM_PMODE_INAIR;
 assert(wm_arcade_bret_fire_monitor(&a,&o,WM_BRET_MON_GRAB_TOSS_AIR,&e,0,&c)==1);
 assert(a.special_move_addr==(uintptr_t)WM_BRET_ANIM_HIPTOSS2);
 memset(&a,0,sizeof(a));e.p1rounds=2;e.p2rounds=0;
 assert(wm_arcade_bret_fire_monitor(&a,&o,WM_BRET_MON_FINISH1,&e,0,&c)==1);
 assert(a.special_move_addr==(uintptr_t)WM_BRET_ANIM_FINISH1);
 puts("Stage 14 Bret-specific move/input tests: PASS");return 0;
}
