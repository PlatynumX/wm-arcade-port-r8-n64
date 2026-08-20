#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wm_arcade_react3_core.h"
#define FX16(x) ((int32_t)((x)<<16))

typedef struct trace {
 int anim_calls,snd_calls,impact_calls,off_calls,gidd_calls,rnd_calls,unhandled_calls;
 wm_arcade_react1_anim_group_t anim; wm_arcade_react1_sound_t snd[8]; wm_arcade_react1_impact_t impact;
 int rnd_hi; uint16_t rnd_arg;
} trace_t;
static void t_anim(wm_arcade_actor_t*v,wm_arcade_react1_anim_group_t g,void*u){(void)v;trace_t*t=u;t->anim_calls++;t->anim=g;}
static void t_snd(wm_arcade_actor_t*v,wm_arcade_react1_sound_t s,void*u){(void)v;trace_t*t=u;if(t->snd_calls<8)t->snd[t->snd_calls]=s;t->snd_calls++;}
static void t_imp(wm_arcade_actor_t*a,wm_arcade_actor_t*v,wm_arcade_react1_impact_t i,void*u){(void)a;(void)v;trace_t*t=u;t->impact_calls++;t->impact=i;}
static void t_off(wm_arcade_actor_t*v,void*u){(void)v;((trace_t*)u)->off_calls++;}
static void t_gidd(wm_arcade_actor_t*v,void*u){(void)v;((trace_t*)u)->gidd_calls++;}
static int t_rnd(uint16_t arg,void*u){trace_t*t=u;t->rnd_calls++;t->rnd_arg=arg;return t->rnd_hi;}
static void t_un(wm_arcade_actor_t*a,wm_arcade_actor_t*v,wm_arcade_reaction_id_t r,void*u){(void)a;(void)v;(void)r;((trace_t*)u)->unhandled_calls++;}
static wm_arcade_react1_callbacks_t cbs(trace_t*t){wm_arcade_react1_callbacks_t c;memset(&c,0,sizeof(c));c.change_anim=t_anim;c.play_sound=t_snd;c.impact=t_imp;c.collisions_off=t_off;c.maybe_gidd_up=t_gidd;c.rndper_hi=t_rnd;c.unhandled_reaction=t_un;c.user=t;return c;}
static void actors(wm_arcade_actor_t*a,wm_arcade_actor_t*v){memset(a,0,sizeof(*a));memset(v,0,sizeof(*v));a->active=v->active=1;a->x_int=10;v->x_int=20;a->life=v->life=100;a->attack_mode=WM_AMODE_BIGBOOT;v->player_mode=WM_PMODE_NORMAL;a->anim_mode=v->anim_mode=WM_MODE_CHECKHIT;}
static void test_bigboot_face(void){wm_arcade_actor_t a,v;trace_t t={0};wm_arcade_react1_callbacks_t c=cbs(&t);wm_arcade_react1_context_t x;actors(&a,&v);wm_arcade_react1_context_init(&x,&c);assert(wm_arcade_react3_apply(&a,&v,WM_RXN_BIGBOOT,NULL,NULL,&x));assert(t.impact==WM_R1_IMPACT_FACE);assert(t.snd_calls==2&&t.snd[0]==WM_R1_SND_FLYKICK&&t.snd[1]==WM_R1_SND_SCREAM);assert(t.anim==WM_R1_ANIM_HEAD_HIT2);assert(t.off_calls==1);}
static void test_bigboot_inair(void){wm_arcade_actor_t a,v;trace_t t={0};wm_arcade_react1_callbacks_t c=cbs(&t);wm_arcade_react1_context_t x;actors(&a,&v);v.player_mode=WM_PMODE_INAIR;wm_arcade_react1_context_init(&x,&c);wm_arcade_react3_apply(&a,&v,WM_RXN_BIGBOOT,NULL,NULL,&x);assert(t.impact==WM_R1_IMPACT_DROP_KICK);assert(t.snd_calls==1&&t.snd[0]==WM_R1_SND_LBOWDROP);assert(t.anim==WM_R1_ANIM_FALL_BACK);assert(v.roll_pos==0);assert(v.x_vel==FX16(3));assert(v.getup_time>0);}
static void test_bigboot_running_rng(void){wm_arcade_actor_t a,v;trace_t t={0};wm_arcade_react1_callbacks_t c=cbs(&t);wm_arcade_react1_context_t x;actors(&a,&v);v.player_mode=WM_PMODE_RUNNING;t.rnd_hi=1;wm_arcade_react1_context_init(&x,&c);wm_arcade_react3_apply(&a,&v,WM_RXN_BIGBOOT,NULL,NULL,&x);assert(t.rnd_calls==1&&t.rnd_arg==100&&t.impact==WM_R1_IMPACT_FACE);actors(&a,&v);memset(&t,0,sizeof(t));c=cbs(&t);v.player_mode=WM_PMODE_RUNNING;t.rnd_hi=0;wm_arcade_react1_context_init(&x,&c);wm_arcade_react3_apply(&a,&v,WM_RXN_BIGBOOT,NULL,NULL,&x);assert(t.impact==WM_R1_IMPACT_DROP_KICK);}
static void test_knee(void){wm_arcade_actor_t a,v;trace_t t={0};wm_arcade_react1_callbacks_t c=cbs(&t);wm_arcade_react1_context_t x;actors(&a,&v);a.x_vel=-FX16(8);wm_arcade_react1_context_init(&x,&c);wm_arcade_react3_apply(&a,&v,WM_RXN_KNEE,NULL,NULL,&x);assert(t.impact==WM_R1_IMPACT_MID);assert(t.snd[0]==WM_R1_SND_KICK);assert(t.anim==WM_R1_ANIM_KNEE_HIT);assert(a.x_vel==-FX16(1));}
static void test_headknees(void){wm_arcade_actor_t a,v;trace_t t={0};wm_arcade_react1_callbacks_t c=cbs(&t);wm_arcade_react1_context_t x;actors(&a,&v);wm_arcade_react1_context_init(&x,&c);wm_arcade_react3_apply(&a,&v,WM_RXN_HEADKNEES,NULL,NULL,&x);assert(v.y_vel==FX16(4));assert(t.anim==WM_R1_ANIM_QUICK_KNEE_HIT);assert(t.snd[0]==WM_R1_SND_KICK);}
static void test_boxpunch(void){wm_arcade_actor_t a,v;trace_t t={0};wm_arcade_react1_callbacks_t c=cbs(&t);wm_arcade_react1_context_t x;actors(&a,&v);v.life=0;v.player_mode=WM_PMODE_DEAD;wm_arcade_react1_context_init(&x,&c);wm_arcade_react3_apply(&a,&v,WM_RXN_BOXPUNCH,NULL,NULL,&x);assert(t.impact==WM_R1_IMPACT_FACE);assert(t.snd[0]==WM_R1_SND_FLYKICK);assert(v.player_mode==WM_PMODE_DEAD);assert(v.getup_time==300);assert(t.anim==WM_R1_ANIM_FALL_BACK);assert(v.x_vel==FX16(4));}
int main(void){test_bigboot_face();test_bigboot_inair();test_bigboot_running_rng();test_knee();test_headknees();test_boxpunch();puts("Stage 5 REACT3 core tests: PASS");return 0;}
