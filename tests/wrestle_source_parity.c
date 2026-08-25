#include <assert.h>
#include <string.h>
#include "wm_arcade_wrestle_core.h"
#include "wm_arcade_match_lifecycle.h"
#include "wm_arcade_getup_process.h"

static void test_auto_pin(void){
 wm_arcade_actor_t a,o; wm_arcade_auto_pin_env_t e; memset(&a,0,sizeof(a));memset(&o,0,sizeof(o));memset(&e,0,sizeof(e));
 a.active=o.active=1;o.player_mode=WM_PMODE_DEAD;
 for(unsigned i=0;i<WM_ARCADE_TSEC*3u-1u;i++) assert(!wm_arcade_auto_pin_check(&a,&o,&e));
 assert(a.player_type==0); assert(wm_arcade_auto_pin_check(&a,&o,&e)); assert(a.player_type==WM_PTYPE_DRONE);
 a.auto_pin_countdown=55;o.status_flags|=WM_STATUS_ZOMBIE;assert(!wm_arcade_auto_pin_check(&a,&o,&e));assert(a.auto_pin_countdown==0);
}
static void test_tail(void){
 wm_arcade_actor_t a;memset(&a,0,sizeof(a));a.getup_time=10;a.but_val_down=WM_BTN_PUNCH;
 wm_arcade_wrestler_countdown_tail(&a,false);assert(a.getup_time==6);assert(a.status_flags&WM_STATUS_PRESS_LAST);
 a.but_val_down=0;wm_arcade_wrestler_countdown_tail(&a,false);assert(a.getup_time==2);assert(!(a.status_flags&WM_STATUS_PRESS_LAST));
 wm_arcade_wrestler_countdown_tail(&a,false);assert(a.getup_time==1);
 wm_arcade_wrestler_countdown_tail(&a,false);assert(a.getup_time==0&&a.delay_butns==40);
}
static void test_reset(void){wm_arcade_actor_t a;memset(&a,0xff,sizeof(a));a.status_flags=WM_STATUS_TEMP_PAL|WM_STATUS_DID_BUCKOFF|WM_STATUS_KOD|WM_STATUS_PUSH;wm_arcade_reset_wrestle2_state(&a);assert(a.immobilize_time==30);assert(a.ptime==1);assert(a.status_flags==(WM_STATUS_TEMP_PAL|WM_STATUS_DID_BUCKOFF));assert(!a.last_fling&&!a.last_hiptoss&&!a.last_spunch&&!a.last_skick);}
static void test_pin(void){wm_arcade_actor_t a,o;wm_arcade_actor_t *p[2]={&a,&o};memset(&a,0,sizeof(a));memset(&o,0,sizeof(o));a.active=o.active=1;a.player_side=0;o.player_side=1;a.closest_num=1;o.player_num=1;a.closest_dist=0x70;a.closest_zdist=0x50;o.player_mode=WM_PMODE_DEAD;o.status_flags=WM_STATUS_PINABLE|WM_STATUS_KOD;assert(wm_arcade_can_pin(&a,p,2));assert(o.who_pinned_me==&a);assert(o.status_flags&WM_STATUS_PINNED);assert(!(o.status_flags&WM_STATUS_KOD));assert(o.ptime==1);}
static void test_timer(void){wm_arcade_match_lifecycle_config c;memset(&c,0,sizeof(c));c.adjust_speed=1;assert(wm_arcade_match_timer_step(&c)==1050);c.adjust_speed=5;assert(wm_arcade_match_timer_step(&c)==1950);c.adjust_speed=3;c.royal_rumble=true;assert(wm_arcade_match_timer_step(&c)==499);c.royal_rumble=false;c.pstatus=1;c.num_opps=3;assert(wm_arcade_match_timer_step(&c)==999);c.final_match=true;assert(wm_arcade_match_timer_step(&c)==499);}
static void test_getup(void){wm_arcade_actor_t a,o;wm_arcade_actor_t *p[2]={&a,&o};wm_arcade_getup_process m;memset(&a,0,sizeof(a));memset(&o,0,sizeof(o));a.active=o.active=1;a.player_type=WM_PTYPE_PLAYER;a.player_side=0;o.player_side=1;a.life=100;a.getup_time=120;wm_arcade_getup_process_begin(&m,&a,p,2,false,1,false,0);wm_arcade_getup_process_tick(&m,&a);assert(a.meter_proc==0);wm_arcade_getup_process_tick(&m,&a);assert(a.meter_proc==&m);assert(a.delay_meter==1080);a.delay_meter=0;for(unsigned i=0;i<10u;i++)wm_arcade_getup_process_tick(&m,&a);assert(m.phase==WM_GETUP_OFFSCREEN);wm_arcade_getup_process_tick(&m,&a);assert(m.phase==WM_GETUP_ONSCREEN);a.getup_time=60;wm_arcade_getup_process_tick(&m,&a);assert(m.display_value==60);wm_arcade_inc_getup_time(&a,20);assert(a.getup_time==80);wm_arcade_getup_process_ditch(&m,&a);assert(m.phase==WM_GETUP_OFFSCREEN&&a.delay_meter==1080);}
int main(void){test_auto_pin();test_tail();test_reset();test_pin();test_timer();test_getup();return 0;}
