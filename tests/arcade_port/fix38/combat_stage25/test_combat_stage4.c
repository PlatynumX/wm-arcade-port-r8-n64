#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_arcade_react2_core.h"

#define FX16(x) ((int32_t)((x) << 16))

typedef struct trace {
    int anim_calls, snd_calls, off_calls, gidd_calls, unhandled_calls;
    int flash_calls, triple_calls, health_calls, teammate_calls, adjust_calls;
    wm_arcade_react1_anim_group_t anim;
    wm_arcade_react1_sound_t snd[8];
    uint16_t triple_id[4];
    int health;
    int live_teammates;
    int16_t adjusted_delta;
} trace_t;

static void t_anim(wm_arcade_actor_t *v, wm_arcade_react1_anim_group_t a, void *u)
{ (void)v; trace_t *t=(trace_t*)u; t->anim_calls++; t->anim=a; }
static void t_snd(wm_arcade_actor_t *v, wm_arcade_react1_sound_t s, void *u)
{ (void)v; trace_t *t=(trace_t*)u; if (t->snd_calls < 8) t->snd[t->snd_calls]=s; t->snd_calls++; }
static void t_off(wm_arcade_actor_t *v, void *u)
{ (void)v; ((trace_t*)u)->off_calls++; }
static void t_gidd(wm_arcade_actor_t *v, void *u)
{ (void)v; ((trace_t*)u)->gidd_calls++; }
static int32_t t_health(const wm_arcade_actor_t *v, void *u)
{ (void)v; trace_t *t=(trace_t*)u; t->health_calls++; return t->health; }
static int t_team(const wm_arcade_actor_t *v, void *u)
{ (void)v; trace_t *t=(trace_t*)u; t->teammate_calls++; return t->live_teammates; }
static void t_flash(wm_arcade_actor_t *v, void *u)
{ (void)v; ((trace_t*)u)->flash_calls++; }
static void t_triple(wm_arcade_actor_t *v, uint16_t id, void *u)
{ (void)v; trace_t *t=(trace_t*)u; if (t->triple_calls < 4) t->triple_id[t->triple_calls]=id; t->triple_calls++; }
static void t_unhandled(wm_arcade_actor_t *a, wm_arcade_actor_t *v, wm_arcade_reaction_id_t r, void *u)
{ (void)a; (void)v; (void)r; ((trace_t*)u)->unhandled_calls++; }
static void t_adjust(wm_arcade_actor_t *v, int16_t d, wm_arcade_actor_t *a, void *u)
{ (void)v; (void)a; trace_t *t=(trace_t*)u; t->adjust_calls++; t->adjusted_delta=d; }

static wm_arcade_react1_callbacks_t callbacks(trace_t *t)
{
    wm_arcade_react1_callbacks_t c;
    memset(&c,0,sizeof(c));
    c.change_anim=t_anim; c.play_sound=t_snd; c.maybe_gidd_up=t_gidd;
    c.get_health=t_health; c.victim_has_live_teammates=t_team;
    c.flash_white=t_flash; c.triple_sound=t_triple;
    c.collisions_off=t_off; c.unhandled_reaction=t_unhandled; c.user=t;
    return c;
}

static void actors(wm_arcade_actor_t *a, wm_arcade_actor_t *v)
{
    memset(a,0,sizeof(*a)); memset(v,0,sizeof(*v));
    a->active=v->active=1; a->x_int=10; v->x_int=20;
    a->wrestler_num=0; v->wrestler_num=1; a->player_num=0; v->player_num=1;
    v->life=100; v->who_hit_me=a;
    a->anim_mode=v->anim_mode=WM_MODE_CHECKHIT;
}

static void test_uprcut_block_flails(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; t.health=100;
    wm_arcade_react1_callbacks_t c=callbacks(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); v.player_mode=WM_PMODE_BLOCK; wm_arcade_react1_context_init(&x,&c);
    assert(wm_arcade_react2_apply(&a,&v,WM_RXN_UPRCUT,NULL,NULL,&x));
    assert(v.x_vel==0x00068000); assert(t.anim==WM_R1_ANIM_HITBLOCK_FLAIL);
    assert(t.snd_calls==1 && t.snd[0]==WM_R1_SND_BLOCK); assert(t.flash_calls==0);
}

static int32_t do_uprcut_y(int wrestler, int live)
{
    wm_arcade_actor_t a,v; trace_t t={0}; t.health=100; t.live_teammates=live;
    wm_arcade_react1_callbacks_t c=callbacks(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); a.attack_mode=WM_AMODE_UPRCUT; a.wrestler_num=wrestler;
    wm_arcade_react1_context_init(&x,&c);
    assert(wm_arcade_react2_apply(&a,&v,WM_RXN_UPRCUT,NULL,NULL,&x));
    assert(t.anim==WM_R1_ANIM_FALL_BACK); assert(t.flash_calls==1);
    assert(t.snd_calls==1 && t.snd[0]==WM_R1_SND_UPRCUT);
    assert(v.x_vel==FX16(2)); assert(v.roll_pos==0); assert(t.gidd_calls==1);
    return v.y_vel;
}

static void test_uprcut_launch_matrix(void)
{
    assert(do_uprcut_y(3,0)==FX16(15));
    assert(do_uprcut_y(0,0)==FX16(13));
    assert(do_uprcut_y(2,1)==FX16(11));
    assert(do_uprcut_y(2,0)==FX16(18));
}

static void test_combo_uprcut(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; t.health=100;
    wm_arcade_react1_callbacks_t c=callbacks(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); a.attack_mode=WM_AMODE_UPRCUT2; a.rpt_count=1;
    wm_arcade_react1_context_init(&x,&c);
    assert(wm_arcade_react2_apply(&a,&v,WM_RXN_COMBO_UPRCUT,NULL,NULL,&x));
    assert(v.y_vel==FX16(7)); assert(v.x_vel==0x00018000);
    assert(v.anim_mode==(WM_MODE_UNINT|WM_MODE_NOAUTOFLIP|WM_MODE_OVERLAP));
    assert(t.anim==WM_R1_ANIM_FALL_BACK && t.flash_calls==1 && t.gidd_calls==1);

    actors(&a,&v); memset(&t,0,sizeof(t)); t.health=100; c=callbacks(&t);
    a.attack_mode=WM_AMODE_UPRCUT2; a.rpt_count=2; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react2_apply(&a,&v,WM_RXN_COMBO_UPRCUT,NULL,NULL,&x);
    assert(v.y_vel==FX16(3));
}

static void test_combo_uprcut_block_is_nonflail(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; t.health=100;
    wm_arcade_react1_callbacks_t c=callbacks(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); v.player_mode=WM_PMODE_BLOCK; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react2_apply(&a,&v,WM_RXN_COMBO_UPRCUT,NULL,NULL,&x);
    assert(v.x_vel==0x00048000); assert(t.anim==WM_R1_ANIM_HITBLOCK);
}

static void test_lbowdrop_ignore_and_ground(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; t.health=100; int16_t pending=-9;
    wm_arcade_react1_callbacks_t c=callbacks(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); v.player_mode=WM_PMODE_NORMAL; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react2_apply(&a,&v,WM_RXN_LBOWDROP,&pending,NULL,&x);
    assert(pending==0); assert(t.snd_calls==0); assert(t.off_calls==1);

    actors(&a,&v); memset(&t,0,sizeof(t)); t.health=100; c=callbacks(&t); pending=-9;
    v.player_mode=WM_PMODE_ONGROUND; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react2_apply(&a,&v,WM_RXN_LBOWDROP,&pending,NULL,&x);
    assert(pending==-9); assert(t.snd_calls==1 && t.snd[0]==WM_R1_SND_LBOWDROP);
    assert(t.triple_calls==1 && t.triple_id[0]==0x33); assert(t.off_calls==2);
    assert(t.anim==WM_R1_ANIM_HIT_ON_GROUND);
}

static void test_blbowdrop_ground(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; t.health=100;
    wm_arcade_react1_callbacks_t c=callbacks(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); v.player_mode=WM_PMODE_ONGROUND; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react2_apply(&a,&v,WM_RXN_BLBOWDROP,NULL,NULL,&x);
    assert(t.snd_calls==3); assert(t.snd[0]==WM_R1_SND_SCREAM);
    assert(t.snd[1]==WM_R1_SND_FLYKICK); assert(t.snd[2]==WM_R1_SND_LBOWDROP);
    assert(t.anim==WM_R1_ANIM_HIT_ON_GROUND); assert(t.off_calls==2);
}

static void test_blbowdrop_near_ground_knockdown(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; t.health=100;
    wm_arcade_react1_callbacks_t c=callbacks(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); a.attack_mode=WM_AMODE_BLBOWDROP; v.player_mode=WM_PMODE_INAIR;
    v.ground_y=100; v.y_int=119; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react2_apply(&a,&v,WM_RXN_BLBOWDROP,NULL,NULL,&x);
    assert(v.player_mode==WM_PMODE_NORMAL); assert(t.anim==WM_R1_ANIM_KNOCKDOWN);
    assert(t.gidd_calls==1); assert(t.triple_calls==0); assert(t.off_calls==2);
}

static void test_blbowdrop_elevated_fallback(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; t.health=100;
    wm_arcade_react1_callbacks_t c=callbacks(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); v.player_mode=WM_PMODE_INAIR; v.ground_y=100; v.y_int=120;
    wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react2_apply(&a,&v,WM_RXN_BLBOWDROP,NULL,NULL,&x);
    assert(t.anim==WM_R1_ANIM_FALL_BACK); assert(t.triple_calls==1 && t.triple_id[0]==0x43);
    assert(v.x_vel==FX16(3)); assert(v.y_vel==-0x00030000); assert(t.off_calls==2);
}

static void test_push_and_one_health_suppression(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; t.health=1; int16_t pending=-4;
    wm_arcade_react1_callbacks_t c=callbacks(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); a.x_vel=FX16(5); v.player_mode=WM_PMODE_INAIR;
    wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react2_apply(&a,&v,WM_RXN_PUSH,&pending,NULL,&x);
    assert(a.x_vel==0); assert(v.player_mode==WM_PMODE_NORMAL); assert(v.x_vel==FX16(8));
    assert(t.anim==WM_R1_ANIM_LOSE_BALANCE); assert(t.snd_calls==1 && t.snd[0]==WM_R1_SND_PUSH);
    assert(pending==0); assert(t.health_calls==1); assert(t.off_calls==1);
}

static void test_push_setmode_preserves_dead(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; t.health=10; int16_t pending=-4;
    wm_arcade_react1_callbacks_t c=callbacks(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); v.player_mode=WM_PMODE_DEAD; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react2_apply(&a,&v,WM_RXN_PUSH,&pending,NULL,&x);
    assert(v.player_mode==WM_PMODE_DEAD); assert(pending==-4);
}

static void test_gutpush_block(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; t.health=100; int16_t pending=-3;
    wm_arcade_react1_callbacks_t c=callbacks(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); v.player_mode=WM_PMODE_BLOCK; v.x_vel=FX16(9);
    wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react2_apply(&a,&v,WM_RXN_GUTPUSH,&pending,NULL,&x);
    assert(v.x_vel==0x00068000); assert(t.anim==WM_R1_ANIM_HITBLOCK_FLAIL);
    assert(t.snd_calls==1 && t.snd[0]==WM_R1_SND_BLOCK); assert(pending==-3);
}

static void test_grab_reactions_are_noops(void)
{
    wm_arcade_actor_t a,v,save; trace_t t={0}; t.health=100;
    wm_arcade_react1_callbacks_t c=callbacks(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); wm_arcade_react1_context_init(&x,&c); save=v;
    assert(wm_arcade_react2_apply(&a,&v,WM_RXN_GRABHOLD,NULL,NULL,&x));
    assert(memcmp(&v,&save,sizeof(v))==0);
    assert(wm_arcade_react2_apply(&a,&v,WM_RXN_GRABFLING,NULL,NULL,&x));
    assert(memcmp(&v,&save,sizeof(v))==0);
}

static void test_cumulative_router(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; t.health=100;
    wm_arcade_react1_callbacks_t c=callbacks(&t); wm_arcade_react1_context_t x;
    actors(&a,&v); wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react12_reaction_callback(&a,&v,WM_RXN_PUNCH,NULL,NULL,&x);
    assert(t.anim==WM_R1_ANIM_HEAD_HIT); assert(t.unhandled_calls==0);

    actors(&a,&v); memset(&t,0,sizeof(t)); t.health=100; c=callbacks(&t);
    wm_arcade_react1_context_init(&x,&c); a.attack_mode=WM_AMODE_UPRCUT;
    wm_arcade_react12_reaction_callback(&a,&v,WM_RXN_UPRCUT,NULL,NULL,&x);
    assert(t.anim==WM_R1_ANIM_FALL_BACK); assert(t.unhandled_calls==0);
}

static void test_stage2_dispatch_push_suppresses_health_write(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; t.health=1;
    wm_arcade_react1_callbacks_t c=callbacks(&t); wm_arcade_react1_context_t x;
    wm_arcade_react_callbacks_t rc; wm_arcade_combat_runtime_t rt;
    wm_arcade_wrestler_hit_result_t result;
    actors(&a,&v); a.attack_mode=WM_AMODE_PUSH;
    wm_arcade_react1_context_init(&x,&c);
    memset(&rc,0,sizeof(rc)); rc.reaction=wm_arcade_react12_reaction_callback;
    rc.adjust_health=t_adjust; rc.user=&x;
    /* adjust callback needs trace, so use a tiny bridge via context callbacks user. */
    /* t_adjust receives rc.user; avoid it because expected no call in this case. */
    wm_arcade_combat_runtime_init(&rt); rt.pcnt=100;
    result=wm_arcade_wrestler_hit(&a,&v,&rt,&rc);
    assert(result.status==WM_WRESTLER_HIT_OK);
    assert(result.reaction==WM_RXN_PUSH); assert(result.damage_before_reaction!=0);
    assert(result.damage_after_reaction==0); assert(result.health_hook_called==0);
}

int main(void)
{
    test_uprcut_block_flails(); test_uprcut_launch_matrix();
    test_combo_uprcut(); test_combo_uprcut_block_is_nonflail();
    test_lbowdrop_ignore_and_ground(); test_blbowdrop_ground();
    test_blbowdrop_near_ground_knockdown(); test_blbowdrop_elevated_fallback();
    test_push_and_one_health_suppression(); test_push_setmode_preserves_dead();
    test_gutpush_block(); test_grab_reactions_are_noops(); test_cumulative_router();
    test_stage2_dispatch_push_suppresses_health_write();
    puts("Stage 4 REACT2 core tests: PASS");
    return 0;
}
