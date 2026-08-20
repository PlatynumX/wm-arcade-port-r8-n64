#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wm_arcade_react8_core.h"

#define FX16(x) ((int32_t)((x) << 16))

typedef struct tr { int anim_n,snd_n,off_n; wm_arcade_react1_anim_group_t anim; wm_arcade_react1_sound_t snd; } tr_t;
static void ca(wm_arcade_actor_t*v,wm_arcade_react1_anim_group_t g,void*u){(void)v;tr_t*t=u;t->anim_n++;t->anim=g;}
static void cs(wm_arcade_actor_t*v,wm_arcade_react1_sound_t s,void*u){(void)v;tr_t*t=u;t->snd_n++;t->snd=s;}
static void co(wm_arcade_actor_t*v,void*u){(void)v;((tr_t*)u)->off_n++;}
static wm_arcade_react1_callbacks_t cbs(tr_t*t){wm_arcade_react1_callbacks_t c;memset(&c,0,sizeof(c));c.change_anim=ca;c.play_sound=cs;c.collisions_off=co;c.user=t;return c;}
static void init(wm_arcade_actor_t*a,wm_arcade_actor_t*v){memset(a,0,sizeof(*a));memset(v,0,sizeof(*v));a->life=v->life=100;a->x_int=10;v->x_int=20;a->attack_mode=WM_AMODE_SHNSPDKIK2;v->player_mode=WM_PMODE_NORMAL;}
int main(void){
 wm_arcade_actor_t a,v; tr_t t={0}; wm_arcade_react1_callbacks_t c=cbs(&t); wm_arcade_react1_context_t x; int16_t p=-9;
 init(&a,&v); wm_arcade_react1_context_init(&x,&c); a.x_vel=FX16(8); wm_arcade_react8_apply(&a,&v,WM_RXN_SHNBFKIK,&p,NULL,&x); assert(a.x_vel==-FX16(4)); assert(v.x_vel==FX16(2));
 init(&a,&v); memset(&t,0,sizeof(t)); c=cbs(&t); wm_arcade_react1_context_init(&x,&c); wm_arcade_react8_apply(&a,&v,WM_RXN_SHNSPDKIK,&p,NULL,&x); assert(t.anim==WM_R1_ANIM_HEAD_HIT && t.snd==WM_R1_SND_KICK);
 init(&a,&v); memset(&t,0,sizeof(t)); c=cbs(&t); wm_arcade_react1_context_init(&x,&c); v.player_mode=WM_PMODE_BLOCK; wm_arcade_react8_apply(&a,&v,WM_RXN_SHNSPDKIK,&p,NULL,&x); assert(t.anim==WM_R1_ANIM_HITBLOCK_FLAIL);
 init(&a,&v); memset(&t,0,sizeof(t)); c=cbs(&t); wm_arcade_react1_context_init(&x,&c); v.z_vel=123;v.roll_pos=8;wm_arcade_react8_apply(&a,&v,WM_RXN_SHNSPDKIK2,&p,NULL,&x);assert(v.z_vel==0&&v.roll_pos==0&&v.x_vel==FX16(4)&&t.anim==WM_R1_ANIM_FALL_BACK);
 init(&a,&v); memset(&t,0,sizeof(t)); c=cbs(&t); wm_arcade_react1_context_init(&x,&c); p=-12;wm_arcade_react8_apply(&a,&v,WM_RXN_HITCHECK,&p,NULL,&x);assert(p==0&&t.off_n==1);
 init(&a,&v); memset(&t,0,sizeof(t)); c=cbs(&t); wm_arcade_react1_context_init(&x,&c); a.x_vel=FX16(8);wm_arcade_react8_legacy_flyelbow(&a,&v,&x);assert(a.x_vel==FX16(4));
 puts("Stage 10 REACT8 core tests: PASS"); return 0;
}
