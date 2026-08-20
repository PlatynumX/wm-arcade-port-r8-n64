#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wm_arcade_react9_core.h"
#define FX16(x) ((int32_t)((x)<<16))
typedef struct tr{int an,sn,im,tri,off;wm_arcade_react1_anim_group_t ag;wm_arcade_react1_sound_t sg;uint16_t tid;}tr_t;
static void ca(wm_arcade_actor_t*v,wm_arcade_react1_anim_group_t g,void*u){(void)v;tr_t*t=u;t->an++;t->ag=g;}static void cs(wm_arcade_actor_t*v,wm_arcade_react1_sound_t s,void*u){(void)v;tr_t*t=u;t->sn++;t->sg=s;}static void ci(wm_arcade_actor_t*a,wm_arcade_actor_t*v,wm_arcade_react1_impact_t i,void*u){(void)a;(void)v;(void)i;((tr_t*)u)->im++;}static void ct(wm_arcade_actor_t*v,uint16_t id,void*u){(void)v;tr_t*t=u;t->tri++;t->tid=id;}static void co(wm_arcade_actor_t*v,void*u){(void)v;((tr_t*)u)->off++;}
static wm_arcade_react1_callbacks_t cb(tr_t*t){wm_arcade_react1_callbacks_t c;memset(&c,0,sizeof(c));c.change_anim=ca;c.play_sound=cs;c.impact=ci;c.triple_sound=ct;c.collisions_off=co;c.user=t;return c;}static void init(wm_arcade_actor_t*a,wm_arcade_actor_t*v){memset(a,0,sizeof(*a));memset(v,0,sizeof(*v));a->x_int=10;v->x_int=20;a->life=v->life=100;v->player_mode=WM_PMODE_NORMAL;}
int main(void){wm_arcade_actor_t a,v;tr_t t={0};wm_arcade_react1_callbacks_t c=cb(&t);wm_arcade_react1_context_t x;int16_t p=-7;
 init(&a,&v);wm_arcade_react1_context_init(&x,&c);wm_arcade_react9_apply(&a,&v,WM_RXN_RSLASH,&p,NULL,&x);assert(t.im==1&&t.sg==WM_R1_SND_RSLASH&&t.ag==WM_R1_ANIM_HEAD_HIT2);
 init(&a,&v);memset(&t,0,sizeof(t));c=cb(&t);wm_arcade_react1_context_init(&x,&c);v.y_int=30;v.ground_y=0;wm_arcade_react9_apply(&a,&v,WM_RXN_RSLASH,&p,NULL,&x);assert(t.ag==WM_R1_ANIM_FALL_BACK&&v.x_vel==FX16(3));
 init(&a,&v);memset(&t,0,sizeof(t));c=cb(&t);wm_arcade_react1_context_init(&x,&c);wm_arcade_react9_apply(&a,&v,WM_RXN_HEADDSLASH,&p,NULL,&x);assert(v.y_vel==0x2c000&&v.x_vel==0&&v.x_int==25&&t.ag==WM_R1_ANIM_KNEE_HIT);
 init(&a,&v);memset(&t,0,sizeof(t));c=cb(&t);wm_arcade_react1_context_init(&x,&c);wm_arcade_react9_apply(&a,&v,WM_RXN_HEADUSLASH,&p,NULL,&x);assert(v.y_vel==0x40000&&v.x_int==20);
 init(&a,&v);memset(&t,0,sizeof(t));c=cb(&t);wm_arcade_react1_context_init(&x,&c);v.player_mode=WM_PMODE_NORMAL;p=-9;wm_arcade_react9_apply(&a,&v,WM_RXN_NAPALM,&p,NULL,&x);assert(p==0&&t.an==0&&t.off==1);
 init(&a,&v);memset(&t,0,sizeof(t));c=cb(&t);wm_arcade_react1_context_init(&x,&c);v.player_mode=WM_PMODE_ONGROUND;p=-9;wm_arcade_react9_apply(&a,&v,WM_RXN_NAPALM,&p,NULL,&x);assert(p==-9&&t.ag==WM_R1_ANIM_BURN&&(v.status_flags&WM_STATUS_DEAD_ANIM)&&t.sg==WM_R1_SND_LBOWDROP&&t.tri==1&&t.tid==0x43&&t.off==2);
 puts("Stage 11 REACT9 core tests: PASS");return 0;}
