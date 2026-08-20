#include <assert.h>
#include "wm_arcade_damage.h"
#include <stdio.h>
#include <string.h>
#include "wm_arcade_react5_core.h"

#define FX16(x) ((int32_t)((x) << 16))

typedef struct trace {
    int anim_calls, snd_calls, off_calls, triple_calls, grade_calls, slide_calls;
    wm_arcade_react1_anim_group_t anims[16];
    wm_arcade_react1_sound_t snd[16];
    uint16_t triple[16];
    wm_arcade_move_grade_t grade;
} trace_t;

static void t_anim(wm_arcade_actor_t *v, wm_arcade_react1_anim_group_t g, void *u)
{ (void)v; trace_t *t=(trace_t*)u; if(t->anim_calls<16)t->anims[t->anim_calls]=g; t->anim_calls++; }
static void t_snd(wm_arcade_actor_t *v, wm_arcade_react1_sound_t s, void *u)
{ (void)v; trace_t *t=(trace_t*)u; if(t->snd_calls<16)t->snd[t->snd_calls]=s; t->snd_calls++; }
static void t_off(wm_arcade_actor_t *v, void *u)
{ (void)v; ((trace_t*)u)->off_calls++; }
static void t_tri(wm_arcade_actor_t *v, uint16_t id, void *u)
{ (void)v; trace_t *t=(trace_t*)u; if(t->triple_calls<16)t->triple[t->triple_calls]=id; t->triple_calls++; }
static void t_grade(wm_arcade_actor_t *a, wm_arcade_actor_t *v, wm_arcade_move_grade_t g, void *u)
{ (void)a;(void)v; trace_t *t=(trace_t*)u; t->grade_calls++; t->grade=g; }
static void t_slide(wm_arcade_actor_t *a, void *u)
{ (void)a; ((trace_t*)u)->slide_calls++; }

static wm_arcade_react1_callbacks_t cbs(trace_t *t)
{
    wm_arcade_react1_callbacks_t c;
    memset(&c,0,sizeof(c));
    c.change_anim=t_anim; c.play_sound=t_snd; c.collisions_off=t_off;
    c.triple_sound=t_tri; c.move_grade=t_grade; c.slide_getup_meter=t_slide; c.user=t;
    return c;
}

static void actors(wm_arcade_actor_t *a, wm_arcade_actor_t *v)
{
    memset(a,0,sizeof(*a)); memset(v,0,sizeof(*v));
    a->active=v->active=1; a->x_int=10; v->x_int=20;
    a->life=v->life=100; a->z_fixed=v->z_fixed=FX16(5);
    a->anim_mode=v->anim_mode=WM_MODE_CHECKHIT|WM_MODE_STATUS;
    a->player_mode=WM_PMODE_RUNNING; v->player_mode=WM_PMODE_NORMAL;
}

static void test_good_run_hit(void)
{
    wm_arcade_actor_t a,v; actors(&a,&v);
    v.z_fixed=a.z_fixed+FX16(2); assert(wm_arcade_react5_good_run_hit(&a,&v));
    v.z_fixed=a.z_fixed+FX16(3); assert(!wm_arcade_react5_good_run_hit(&a,&v));
    a.wrestler_num=3; a.getup_time=0; v.z_fixed=a.z_fixed+FX16(4); assert(wm_arcade_react5_good_run_hit(&a,&v));
    v.z_fixed=a.z_fixed+FX16(5); assert(!wm_arcade_react5_good_run_hit(&a,&v));
    v.z_fixed=a.z_fixed; a.getup_time=1; assert(!wm_arcade_react5_good_run_hit(&a,&v));
}

static void test_run_normal_and_meter(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; int16_t p=-7, nd=0;
    wm_arcade_react1_callbacks_t c=cbs(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); a.getup_time=99; a.meter_proc=(void*)1; a.run_time=12; v.run_time=7;
    wm_arcade_react1_context_init(&x,&c);
    assert(wm_arcade_react5_apply(&a,&v,WM_RXN_RUN,&p,&nd,&x));
    assert(p==-7 && nd==0); assert(a.player_mode==WM_PMODE_NORMAL && a.y_vel==FX16(3));
    assert(a.x_vel==-FX16(3) && a.run_time==0 && v.run_time==0 && a.getup_time==0);
    assert(t.slide_calls==1 && t.anim_calls==2);
    assert(t.anims[0]==WM_R1_ANIM_LOSE_BALANCE && t.anims[1]==WM_R1_ANIM_BOUNCE_OFF);
}

static void test_run_yoko_and_skip(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; int16_t p=-1, nd=0;
    wm_arcade_react1_callbacks_t c=cbs(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); a.wrestler_num=3; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react5_apply(&a,&v,WM_RXN_RUN,&p,&nd,&x);
    assert(p==-WM_D_GUTPUSH && v.x_vel==FX16(3));
    assert(t.anims[0]==WM_R1_ANIM_FALL_BACK && t.anims[1]==WM_R1_ANIM_BOUNCE_OFF);

    actors(&a,&v); memset(&t,0,sizeof(t)); c=cbs(&t); wm_arcade_react1_context_init(&x,&c);
    v.z_fixed=a.z_fixed+FX16(3); v.move_dir=WM_MOVE_DOWN_RIGHT; nd=0;
    wm_arcade_react5_apply(&a,&v,WM_RXN_RUN,&p,&nd,&x);
    assert(nd==WM_MOVE_DOWN_RIGHT && a.player_mode==WM_PMODE_RUNNING && t.anim_calls==0 && t.off_calls==1);
}

static void test_run_dizzy(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c=cbs(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); a.dizzy=1; a.getup_time=44; a.meter_proc=(void*)1;
    wm_arcade_react1_context_init(&x,&c); wm_arcade_react5_apply(&a,&v,WM_RXN_RUN,NULL,NULL,&x);
    assert(a.getup_time==44 && t.slide_calls==0);
    assert(t.anims[1]==WM_R1_ANIM_BOUNCE_OFF_DIZZY);
}

static void test_puppet_variants(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c=cbs(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); v.player_mode=WM_PMODE_DEAD; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react5_apply(&a,&v,WM_RXN_PUPPET,NULL,NULL,&x);
    assert(v.player_mode==WM_PMODE_DEAD && v.attach_proc==&a && a.attach_proc==&v && t.anims[0]==WM_R1_ANIM_WRES_SLAVE);

    actors(&a,&v); memset(&t,0,sizeof(t)); c=cbs(&t); v.player_mode=WM_PMODE_DEAD; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react5_apply(&a,&v,WM_RXN_PUPPET_NOFLAIL,NULL,NULL,&x);
    assert(v.attach_proc==NULL && a.attach_proc==NULL && t.anim_calls==0);

    actors(&a,&v); memset(&t,0,sizeof(t)); c=cbs(&t); v.getup_time=0; a.anim_mode=WM_MODE_CHECKHIT|WM_MODE_STATUS; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react5_apply(&a,&v,WM_RXN_PUPPET2,NULL,NULL,&x);
    assert((a.anim_mode & WM_MODE_STATUS)==0 && v.attach_proc==NULL);
}

static void test_hdgrab_and_toss(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c=cbs(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); v.safe_time=10; v.player_mode=WM_PMODE_BLOCK; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react5_apply(&a,&v,WM_RXN_PUPPET_HDGRAB,NULL,NULL,&x);
    assert(v.attach_proc==NULL && t.anims[0]==WM_R1_ANIM_HITBLOCK_FLAIL);

    actors(&a,&v); memset(&t,0,sizeof(t)); c=cbs(&t); a.last_hit_time=0x12345678u; a.hit_blocker=9; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react5_apply(&a,&v,WM_RXN_PUPPET_HDGRAB,NULL,NULL,&x);
    assert(a.hit_blocker==0 && v.head_grab_time==0x12345678u && v.player_mode==WM_PMODE_PUPPET && v.attach_proc==&a);

    actors(&a,&v); memset(&t,0,sizeof(t)); c=cbs(&t); v.player_mode=WM_PMODE_BLOCK; v.safe_time=0;
    v.stick_val_cur=WM_MOVE_DOWN_LEFT; v.new_facing_dir=WM_MOVE_RIGHT; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react5_apply(&a,&v,WM_RXN_PUPPET_TOSS,NULL,NULL,&x);
    assert(v.attach_proc==NULL && t.anims[0]==WM_R1_ANIM_HITBLOCK);

    actors(&a,&v); memset(&t,0,sizeof(t)); c=cbs(&t); v.player_mode=WM_PMODE_BLOCK; v.safe_time=0;
    v.stick_val_cur=WM_MOVE_DOWN_RIGHT; v.new_facing_dir=WM_MOVE_RIGHT; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react5_apply(&a,&v,WM_RXN_PUPPET_TOSS,NULL,NULL,&x);
    assert(v.attach_proc==&a && v.player_mode==WM_PMODE_PUPPET);
}

static void test_strikes(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c=cbs(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); a.wrestler_num=4; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react5_apply(&a,&v,WM_RXN_BACKHAND,NULL,NULL,&x);
    assert(t.triple[0]==0x33 && t.grade==WM_R_MOVE_AVERAGE && t.snd[0]==WM_R1_SND_UPRCUT);
    assert(t.anims[0]==WM_R1_ANIM_BACKHAND_HEAD_HIT);

    actors(&a,&v); memset(&t,0,sizeof(t)); c=cbs(&t); wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react5_apply(&a,&v,WM_RXN_EARSLAP,NULL,NULL,&x);
    assert(t.triple[0]==0x43 && t.snd[0]==WM_R1_SND_HDBUTT && t.anims[0]==WM_R1_ANIM_EARSLAP_HEAD_HIT);

    actors(&a,&v); memset(&t,0,sizeof(t)); c=cbs(&t); v.life=0; v.player_mode=WM_PMODE_DEAD; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react5_apply(&a,&v,WM_RXN_BUZZ,NULL,NULL,&x);
    assert(v.player_mode==WM_PMODE_DEAD && v.attach_proc==&a && a.attach_proc==&v);
    assert(t.snd[0]==WM_R1_SND_PUNCH && t.anims[0]==WM_R1_ANIM_GET_BUZZ);

    actors(&a,&v); memset(&t,0,sizeof(t)); c=cbs(&t); v.life=0; v.player_mode=WM_PMODE_DEAD; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react5_apply(&a,&v,WM_RXN_HAYMAKER,NULL,NULL,&x);
    assert(v.player_mode==WM_PMODE_DEAD && v.x_vel==FX16(4));
    assert(t.snd[0]==WM_R1_SND_FLYKICK && t.anims[0]==WM_R1_ANIM_FALL_BACK);
}

int main(void)
{
    test_good_run_hit(); test_run_normal_and_meter(); test_run_yoko_and_skip(); test_run_dizzy();
    test_puppet_variants(); test_hdgrab_and_toss(); test_strikes();
    puts("Stage 7 REACT5 core tests: PASS");
    return 0;
}
