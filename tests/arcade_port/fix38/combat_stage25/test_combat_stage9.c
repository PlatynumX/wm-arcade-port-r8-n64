#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wm_arcade_react7_core.h"

typedef struct trace { int anim_calls; wm_arcade_react1_anim_group_t last; } trace_t;
static void anim(wm_arcade_actor_t *v, wm_arcade_react1_anim_group_t g, void *u)
{ (void)v; trace_t *t=(trace_t*)u; t->anim_calls++; t->last=g; }
int main(void)
{
    wm_arcade_actor_t a,v; trace_t t={0}; wm_arcade_react1_callbacks_t c; wm_arcade_react1_context_t x;
    wm_arcade_react7_att30_stub(); wm_arcade_react7_att31_stub(); wm_arcade_react7_att32_stub();
    wm_arcade_react7_att33_stub(); wm_arcade_react7_att34_stub();
    memset(&a,0,sizeof(a)); memset(&v,0,sizeof(v)); memset(&c,0,sizeof(c));
    a.active=v.active=1; a.x_int=10; v.x_int=20; a.life=v.life=100; v.player_mode=WM_PMODE_BLOCK;
    c.change_anim=anim; c.user=&t; wm_arcade_react1_context_init(&x,&c);
    wm_arcade_react1234567_reaction_callback(&a,&v,WM_RXN_HAMMER,NULL,NULL,&x);
    assert(t.anim_calls==1 && t.last==WM_R1_ANIM_HITBLOCK);

    /* Full Stage 2 -> Stage 7 integration: source checks good_run_hit before
       bookkeeping, then REACT5 hit_run checks it again inside the reaction. */
    {
        wm_arcade_react_callbacks_t rc;
        wm_arcade_combat_runtime_t rt;
        wm_arcade_wrestler_hit_result_t r;
        memset(&a,0,sizeof(a)); memset(&v,0,sizeof(v)); memset(&rc,0,sizeof(rc));
        memset(&t,0,sizeof(t)); memset(&c,0,sizeof(c));
        a.active=v.active=1; a.x_int=10; v.x_int=20; a.z_fixed=v.z_fixed=0;
        a.life=v.life=100; a.wrestler_num=0; v.wrestler_num=1;
        a.attack_mode=WM_AMODE_RUN; a.player_mode=WM_PMODE_RUNNING; v.player_mode=WM_PMODE_NORMAL;
        c.change_anim=anim; c.user=&t; wm_arcade_react1_context_init(&x,&c);
        wm_arcade_combat_runtime_init(&rt); rt.pcnt=0x12345678u;
        rc.good_run_hit=wm_arcade_react5_good_run_hit_callback;
        rc.reaction=wm_arcade_react1234567_reaction_callback; rc.user=&x;
        r=wm_arcade_wrestler_hit(&a,&v,&rt,&rc);
        assert(r.status==WM_WRESTLER_HIT_OK && r.reaction==WM_RXN_RUN);
        assert(a.who_i_hit==&v && v.who_hit_me==&a && a.last_hit_time==0x12345678u);
        assert(a.player_mode==WM_PMODE_NORMAL && t.anim_calls==2);

        memset(&a,0,sizeof(a)); memset(&v,0,sizeof(v));
        a.active=v.active=1; a.wrestler_num=0; v.wrestler_num=1; a.attack_mode=WM_AMODE_RUN;
        a.player_mode=WM_PMODE_RUNNING; v.player_mode=WM_PMODE_NORMAL; v.life=100;
        v.z_fixed=(3 << 16);
        r=wm_arcade_wrestler_hit(&a,&v,&rt,&rc);
        assert(r.status==WM_WRESTLER_HIT_IGNORED_RUN);
        assert(a.who_i_hit==NULL && v.who_hit_me==NULL && a.last_hit_time==0);
    }

    puts("Stage 9 REACT7 legacy-stub/routing + cumulative integration tests: PASS");
    return 0;
}
