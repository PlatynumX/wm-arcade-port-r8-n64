#include "wm_arcade_react3_core.h"

#include <stddef.h>

#define FX16(x) ((int32_t)((x) << 16))
#define WM_ARCADE_TSEC 60

static const wm_arcade_react1_callbacks_t *cb_of(wm_arcade_react1_context_t *ctx)
{ return ctx ? ctx->callbacks : NULL; }

static void anim(wm_arcade_actor_t *v, wm_arcade_react1_anim_group_t g,
                 wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb=cb_of(ctx);
    if (cb && cb->change_anim) cb->change_anim(v,g,cb->user);
}
static void sound(wm_arcade_actor_t *v, wm_arcade_react1_sound_t s,
                  wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb=cb_of(ctx);
    if (cb && cb->play_sound) cb->play_sound(v,s,cb->user);
}
static void impact(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                   wm_arcade_react1_impact_t i, wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb=cb_of(ctx);
    if (cb && cb->impact) cb->impact(a,v,i,cb->user);
}
static void collis_off(wm_arcade_actor_t *v, wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb=cb_of(ctx);
    wm_arcade_wrestler_collisions_off(v);
    if (cb && cb->collisions_off) cb->collisions_off(v,cb->user);
}
static void setmode_normal(wm_arcade_actor_t *v)
{
    if (v->player_mode != WM_PMODE_DEAD) v->player_mode=WM_PMODE_NORMAL;
}
static int32_t push_away(const wm_arcade_actor_t *a,const wm_arcade_actor_t *v,int32_t mag)
{ return a->x_int < v->x_int ? mag : -mag; }
static void set_getup(const wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                      wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb=cb_of(ctx);
    wm_arcade_combat_callbacks_t c={0};
    c.maybe_gidd_up=cb ? cb->maybe_gidd_up : NULL;
    c.user=cb ? cb->user : NULL;
    wm_arcade_set_getup_time(a,v,&c);
}

static int hit_bigboot(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                       wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb=cb_of(ctx);
    int use_face=0;
    if (v->player_mode == WM_PMODE_RUNNING) {
        if (!cb || !cb->rndper_hi) {
            if (ctx) ctx->last_supported=0;
            return 0;
        }
        use_face = cb->rndper_hi(100,cb->user) != 0;
    } else if (v->player_mode != WM_PMODE_INAIR) {
        use_face=1;
    }

    if (use_face) {
        impact(a,v,WM_R1_IMPACT_FACE,ctx);
        if (v->life != 0) {
            sound(v,WM_R1_SND_FLYKICK,ctx);
            sound(v,WM_R1_SND_SCREAM,ctx);
            setmode_normal(v);
            anim(v,WM_R1_ANIM_HEAD_HIT2,ctx);
        }
        collis_off(v,ctx);
        return 1;
    }

    impact(a,v,WM_R1_IMPACT_DROP_KICK,ctx);
    sound(v,WM_R1_SND_LBOWDROP,ctx);
    if (v->life != 0) setmode_normal(v);
    v->roll_pos=0;
    set_getup(a,v,ctx);
    anim(v,WM_R1_ANIM_FALL_BACK,ctx);
    v->x_vel=push_away(a,v,FX16(3));
    collis_off(v,ctx);
    return 1;
}

static void hit_knee(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                     wm_arcade_react1_context_t *ctx)
{
    if (v->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit(a,v,ctx);
        return;
    }
    impact(a,v,WM_R1_IMPACT_MID,ctx);
    if (v->life != 0) {
        sound(v,WM_R1_SND_KICK,ctx);
        setmode_normal(v);
        anim(v,WM_R1_ANIM_KNEE_HIT,ctx);
        a->x_vel >>= 3;
    }
    collis_off(v,ctx);
}

static void hit_headknees(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                          wm_arcade_react1_context_t *ctx)
{
    (void)a;
    sound(v,WM_R1_SND_KICK,ctx);
    v->y_vel=0x00040000;
    anim(v,WM_R1_ANIM_QUICK_KNEE_HIT,ctx);
    collis_off(v,ctx);
}

static void hit_boxpunch(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                         wm_arcade_react1_context_t *ctx)
{
    if (v->player_mode == WM_PMODE_BLOCK) {
        wm_arcade_react1_block_hit_flail(a,v,ctx);
        return;
    }
    impact(a,v,WM_R1_IMPACT_FACE,ctx);
    sound(v,WM_R1_SND_FLYKICK,ctx);
    if (v->life != 0) setmode_normal(v);
    v->getup_time=5*WM_ARCADE_TSEC;
    anim(v,WM_R1_ANIM_FALL_BACK,ctx);
    v->x_vel=push_away(a,v,FX16(4));
    collis_off(v,ctx);
}

int wm_arcade_react3_supports(wm_arcade_reaction_id_t reaction)
{
    switch (reaction) {
    case WM_RXN_BIGBOOT:
    case WM_RXN_KNEE:
    case WM_RXN_HEADKNEES:
    case WM_RXN_BOXPUNCH:
        return 1;
    default: return 0;
    }
}

int wm_arcade_react3_apply(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                           wm_arcade_reaction_id_t reaction,
                           int16_t *hit_damage_pending,
                           int16_t *new_victim_movedir,
                           wm_arcade_react1_context_t *ctx)
{
    (void)hit_damage_pending; (void)new_victim_movedir;
    if (!a || !v || !wm_arcade_react3_supports(reaction)) return 0;
    if (ctx) { ctx->last_reaction=reaction; ctx->last_supported=1; ctx->last_flykick_aborted=0; }
    switch (reaction) {
    case WM_RXN_BIGBOOT: return hit_bigboot(a,v,ctx);
    case WM_RXN_KNEE: hit_knee(a,v,ctx); break;
    case WM_RXN_HEADKNEES: hit_headknees(a,v,ctx); break;
    case WM_RXN_BOXPUNCH: hit_boxpunch(a,v,ctx); break;
    default: return 0;
    }
    return 1;
}

void wm_arcade_react123_reaction_callback(wm_arcade_actor_t *a,
                                          wm_arcade_actor_t *v,
                                          wm_arcade_reaction_id_t reaction,
                                          int16_t *hit_damage_pending,
                                          int16_t *new_victim_movedir,
                                          void *user)
{
    wm_arcade_react1_context_t *ctx=(wm_arcade_react1_context_t*)user;
    if (wm_arcade_react3_supports(reaction)) {
        (void)wm_arcade_react3_apply(a,v,reaction,hit_damage_pending,new_victim_movedir,ctx);
        return;
    }
    wm_arcade_react12_reaction_callback(a,v,reaction,hit_damage_pending,new_victim_movedir,user);
}
