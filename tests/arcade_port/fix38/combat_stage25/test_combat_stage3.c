#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_arcade_react1_core.h"

#define FX16(x) ((int32_t)((x) << 16))

typedef struct trace {
    int anim_calls, snd_calls, impact_calls, off_calls, gidd_calls, unhandled_calls;
    wm_arcade_react1_anim_group_t anim;
    wm_arcade_react1_sound_t snd;
    wm_arcade_react1_impact_t impact;
    int lex;
} trace_t;

static void t_anim(wm_arcade_actor_t *v, wm_arcade_react1_anim_group_t a, void *u)
{ (void)v; trace_t *t=u; t->anim_calls++; t->anim=a; }
static void t_snd(wm_arcade_actor_t *v, wm_arcade_react1_sound_t s, void *u)
{ (void)v; trace_t *t=u; t->snd_calls++; t->snd=s; }
static void t_impact(wm_arcade_actor_t *a, wm_arcade_actor_t *v, wm_arcade_react1_impact_t i, void *u)
{ (void)a; (void)v; trace_t *t=u; t->impact_calls++; t->impact=i; }
static int t_lex(const wm_arcade_actor_t *a, void *u)
{ (void)a; return ((trace_t*)u)->lex; }
static void t_gidd(wm_arcade_actor_t *v, void *u)
{ (void)v; ((trace_t*)u)->gidd_calls++; }
static void t_off(wm_arcade_actor_t *v, void *u)
{ (void)v; ((trace_t*)u)->off_calls++; }
static void t_unhandled(wm_arcade_actor_t *a, wm_arcade_actor_t *v, wm_arcade_reaction_id_t r, void *u)
{ (void)a; (void)v; (void)r; ((trace_t*)u)->unhandled_calls++; }

static wm_arcade_react1_callbacks_t callbacks(trace_t *t)
{
    wm_arcade_react1_callbacks_t c;
    memset(&c,0,sizeof(c));
    c.change_anim=t_anim; c.play_sound=t_snd; c.impact=t_impact;
    c.attacker_uses_lex_flykick_anim=t_lex; c.maybe_gidd_up=t_gidd;
    c.collisions_off=t_off; c.unhandled_reaction=t_unhandled; c.user=t;
    return c;
}

static void actors(wm_arcade_actor_t *a, wm_arcade_actor_t *v)
{
    memset(a,0,sizeof(*a)); memset(v,0,sizeof(*v));
    a->active=v->active=1; a->x_int=10; v->x_int=20; v->life=100;
    a->wrestler_num=0; v->wrestler_num=1; v->who_hit_me=a;
    v->anim_mode=WM_MODE_CHECKHIT; a->anim_mode=WM_MODE_CHECKHIT;
}

static void test_block(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c=callbacks(&t);
    wm_arcade_react1_context_t x; actors(&a,&v); v.player_mode=WM_PMODE_BLOCK;
    wm_arcade_react1_context_init(&x,&c);
    assert(wm_arcade_react1_apply(&a,&v,WM_RXN_PUNCH,NULL,NULL,&x));
    assert(v.x_vel==0x00048000); assert(t.anim==WM_R1_ANIM_HITBLOCK);
    assert(t.snd==WM_R1_SND_BLOCK); assert((v.anim_mode&WM_MODE_CHECKHIT)==0);
}

static void test_punch_sixth_loses_balance(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c=callbacks(&t);
    wm_arcade_react1_context_t x; actors(&a,&v); v.consecutive_hits=5; a.combo_count=0;
    wm_arcade_react1_context_init(&x,&c);
    assert(wm_arcade_react1_apply(&a,&v,WM_RXN_PUNCH,NULL,NULL,&x));
    assert(v.consecutive_hits==0); assert(t.anim==WM_R1_ANIM_LOSE_BALANCE);
}

static void test_punch_sixth_combo_keeps_headhit(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c=callbacks(&t);
    wm_arcade_react1_context_t x; actors(&a,&v); v.consecutive_hits=5; a.combo_count=2;
    wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react1_apply(&a,&v,WM_RXN_PUNCH,NULL,NULL,&x);
    assert(v.consecutive_hits==0); assert(t.anim==WM_R1_ANIM_HEAD_HIT);
}

static void test_elevated_punch_falls(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c=callbacks(&t);
    wm_arcade_react1_context_t x; actors(&a,&v); v.y_int=120; v.ground_y=100;
    wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react1_apply(&a,&v,WM_RXN_PUNCH,NULL,NULL,&x);
    assert(t.anim==WM_R1_ANIM_FALL_BACK); assert(v.x_vel==FX16(3));
}

static void test_hdbutt2_velocity(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c=callbacks(&t);
    wm_arcade_react1_context_t x; actors(&a,&v); v.x_vel=99;
    wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react1_apply(&a,&v,WM_RXN_HDBUTT2,NULL,NULL,&x);
    assert(v.y_vel==0x0003c000); assert(v.x_vel==0);
}

static void test_hdbutt_stay(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c=callbacks(&t);
    wm_arcade_react1_context_t x; actors(&a,&v); v.x_vel=FX16(9);
    wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react1_apply(&a,&v,WM_RXN_HDBUTT_STAY,NULL,NULL,&x);
    assert(v.delay_meter==360); assert(v.x_vel==0); assert(v.player_mode==WM_PMODE_NORMAL);
}

static void test_tomb_ground(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c=callbacks(&t);
    wm_arcade_react1_context_t x; actors(&a,&v); v.player_mode=WM_PMODE_ONGROUND;
    wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react1_apply(&a,&v,WM_RXN_TOMB,NULL,NULL,&x);
    assert(t.anim==WM_R1_ANIM_HIT_ON_GROUND); assert(t.snd==WM_R1_SND_SCREAM);
}

static void test_superkick_block_flails(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c=callbacks(&t);
    wm_arcade_react1_context_t x; actors(&a,&v); v.player_mode=WM_PMODE_BLOCK;
    wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react1_apply(&a,&v,WM_RXN_SUPER_KICK,NULL,NULL,&x);
    assert(t.anim==WM_R1_ANIM_HITBLOCK_FLAIL); assert(v.x_vel==0x00068000);
}

static void test_flykick_velocity_and_getup(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c=callbacks(&t);
    wm_arcade_react1_context_t x; actors(&a,&v); a.attack_mode=WM_AMODE_FLYKICK;
    a.x_vel=FX16(6); v.getup_time=0; v.delay_meter=0;
    wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react1_apply(&a,&v,WM_RXN_FLYKICK,NULL,NULL,&x);
    assert(a.x_vel==-FX16(4)); assert(a.y_vel==FX16(4));
    assert(v.x_vel==FX16(2)); assert(v.roll_pos==0); assert(v.getup_time==WM_STAY_TIME);
    assert(t.gidd_calls==1); assert(t.impact==WM_R1_IMPACT_DROP_KICK);
}

static void test_flykick_lex_mid_hit(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c=callbacks(&t);
    wm_arcade_react1_context_t x; actors(&a,&v); t.lex=1; a.attack_mode=WM_AMODE_FLYKICK;
    wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react1_apply(&a,&v,WM_RXN_FLYKICK,NULL,NULL,&x);
    assert(t.impact==WM_R1_IMPACT_MID);
}

static void test_flykick_block_still_changes_attacker_velocity(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c=callbacks(&t);
    wm_arcade_react1_context_t x; actors(&a,&v); v.player_mode=WM_PMODE_BLOCK; a.x_vel=-FX16(2);
    wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react1_apply(&a,&v,WM_RXN_FLYKICK,NULL,NULL,&x);
    assert(a.x_vel==FX16(1)); assert(a.y_vel==FX16(4)); assert(x.last_flykick_aborted==1);
}

static void test_bigknee(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c=callbacks(&t);
    wm_arcade_react1_context_t x; actors(&a,&v); a.attack_mode=WM_AMODE_BIGKNEE;
    wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react1_apply(&a,&v,WM_RXN_BIGKNEE,NULL,NULL,&x);
    assert(v.x_vel==FX16(4)); assert(v.getup_time==WM_STAY_TIME);
    assert(t.anim==WM_R1_ANIM_FALL_BACK); assert(t.impact==WM_R1_IMPACT_DROP_KICK);
}

static void test_turnbuckle(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c=callbacks(&t);
    wm_arcade_react1_context_t x; actors(&a,&v);
    wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react1_apply(&a,&v,WM_RXN_ONTURNBUCKLE,NULL,NULL,&x);
    assert(v.player_mode==WM_PMODE_INAIR); assert(v.status_flags&WM_STATUS_DEAD_ANIM);
    assert(v.x_vel==FX16(4)); assert(v.y_vel==FX16(6)); assert(t.anim==WM_R1_ANIM_FALL_BACK_TBUKL);
}

static void test_grabthrow_noop_and_unhandled(void)
{
    wm_arcade_actor_t a,v,save; trace_t t={0}; wm_arcade_react1_callbacks_t c=callbacks(&t);
    wm_arcade_react1_context_t x; actors(&a,&v); save=v;
    wm_arcade_react1_context_init(&x,&c);
    assert(wm_arcade_react1_apply(&a,&v,WM_RXN_GRABTHROW,NULL,NULL,&x));
    assert(memcmp(&v,&save,sizeof(v))==0);
    assert(!wm_arcade_react1_apply(&a,&v,WM_RXN_UPRCUT,NULL,NULL,&x));
    assert(t.unhandled_calls==1);
}

int main(void)
{
    test_block(); test_punch_sixth_loses_balance(); test_punch_sixth_combo_keeps_headhit();
    test_elevated_punch_falls(); test_hdbutt2_velocity(); test_hdbutt_stay();
    test_tomb_ground(); test_superkick_block_flails(); test_flykick_velocity_and_getup();
    test_flykick_lex_mid_hit(); test_flykick_block_still_changes_attacker_velocity();
    test_bigknee(); test_turnbuckle(); test_grabthrow_noop_and_unhandled();
    puts("Stage 3 REACT1 core tests: PASS");
    return 0;
}
