#include "wm_arcade_react4_core.h"

#include <stddef.h>

#define FX16(x) ((int32_t)((x) << 16))

static const wm_arcade_react1_callbacks_t *cb_of(wm_arcade_react1_context_t *ctx)
{ return ctx ? ctx->callbacks : NULL; }
static void anim(wm_arcade_actor_t *v, wm_arcade_react1_anim_group_t g, wm_arcade_react1_context_t *ctx)
{ const wm_arcade_react1_callbacks_t *cb=cb_of(ctx); if(cb&&cb->change_anim) cb->change_anim(v,g,cb->user); }
static void sound(wm_arcade_actor_t *v, wm_arcade_react1_sound_t s, wm_arcade_react1_context_t *ctx)
{ const wm_arcade_react1_callbacks_t *cb=cb_of(ctx); if(cb&&cb->play_sound) cb->play_sound(v,s,cb->user); }
static void impact(wm_arcade_actor_t *a, wm_arcade_actor_t *v, wm_arcade_react1_impact_t i, wm_arcade_react1_context_t *ctx)
{ const wm_arcade_react1_callbacks_t *cb=cb_of(ctx); if(cb&&cb->impact) cb->impact(a,v,i,cb->user); }
static void triple(wm_arcade_actor_t *v,uint16_t id,wm_arcade_react1_context_t *ctx)
{ const wm_arcade_react1_callbacks_t *cb=cb_of(ctx); if(cb&&cb->triple_sound) cb->triple_sound(v,id,cb->user); }
static void collis_off(wm_arcade_actor_t *v, wm_arcade_react1_context_t *ctx)
{ const wm_arcade_react1_callbacks_t *cb=cb_of(ctx); wm_arcade_wrestler_collisions_off(v); if(cb&&cb->collisions_off) cb->collisions_off(v,cb->user); }
static void setmode_normal(wm_arcade_actor_t *v)
{ if(v->player_mode!=WM_PMODE_DEAD) v->player_mode=WM_PMODE_NORMAL; }
static int32_t push_away(const wm_arcade_actor_t *a,const wm_arcade_actor_t *v,int32_t mag)
{ return a->x_int < v->x_int ? mag : -mag; }
static void set_getup(const wm_arcade_actor_t *a,wm_arcade_actor_t *v,wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb=cb_of(ctx); wm_arcade_combat_callbacks_t c={0};
    c.maybe_gidd_up=cb?cb->maybe_gidd_up:NULL; c.user=cb?cb->user:NULL; wm_arcade_set_getup_time(a,v,&c);
}
static void grade(wm_arcade_actor_t *a,wm_arcade_actor_t *v,wm_arcade_move_grade_t g,wm_arcade_react1_context_t *ctx)
{ const wm_arcade_react1_callbacks_t *cb=cb_of(ctx); if(cb&&cb->move_grade) cb->move_grade(a,v,g,cb->user); }
static wm_arcade_react_anim_tag_t animtag(const wm_arcade_actor_t *a,wm_arcade_react1_context_t *ctx)
{ const wm_arcade_react1_callbacks_t *cb=cb_of(ctx); return (cb&&cb->attacker_anim_tag)?cb->attacker_anim_tag(a,cb->user):WM_R_ANIMTAG_OTHER; }
static void bounce(wm_arcade_actor_t *a,wm_arcade_react1_context_t *ctx)
{
    const wm_arcade_react1_callbacks_t *cb=cb_of(ctx);
    a->y_vel=0x00050000; a->z_vel=0x00010000; a->x_vel=0;
    if(cb&&cb->shake_all_ropes) cb->shake_all_ropes(cb->user);
    if(cb&&cb->shaker2) cb->shaker2(8,cb->user);
}

static void hit_stomp(wm_arcade_actor_t *a,wm_arcade_actor_t *v,int16_t *pending,wm_arcade_react1_context_t *ctx)
{
    wm_arcade_react_anim_tag_t tag;
    if(v->player_mode==WM_PMODE_NORMAL || v->player_mode==WM_PMODE_BLOCK) {
        if(pending) *pending=0;
        collis_off(v,ctx);
        return;
    }
    collis_off(v,ctx);
    if(v->player_mode!=WM_PMODE_ONGROUND && v->player_mode!=WM_PMODE_DEAD) {
        collis_off(v,ctx);
        return;
    }
    anim(v,WM_R1_ANIM_HIT_ON_GROUND,ctx);
    tag=animtag(a,ctx);
    if(tag==WM_R_ANIMTAG_SHN_COMBO_RUN_STOMP || tag==WM_R_ANIMTAG_SHN_RUN_STOMP) {
        sound(v,WM_R1_SND_SCREAM,ctx);
    } else {
        sound(v,WM_R1_SND_LBOWDROP,ctx);
        triple(v,0x43u,ctx);
    }
    if(tag==WM_R_ANIMTAG_DNK_BELLY || tag==WM_R_ANIMTAG_UND_FLYING_BUTT_DROP) {
        bounce(a,ctx);
        return;
    }
    if(tag==WM_R_ANIMTAG_LEX_FLYING_GROUND_PUNCH) {
        bounce(a,ctx); a->y_vel=0x00040000; return;
    }
    collis_off(v,ctx);
}

static void hit_bstomp(wm_arcade_actor_t *a,wm_arcade_actor_t *v,wm_arcade_react1_context_t *ctx)
{
    sound(v,WM_R1_SND_SCREAM,ctx); collis_off(v,ctx);
    if(v->player_mode==WM_PMODE_ONGROUND || v->player_mode==WM_PMODE_DEAD) {
        anim(v,WM_R1_ANIM_HIT_ON_GROUND,ctx); collis_off(v,ctx); return;
    }
    sound(v,WM_R1_SND_SCREAM,ctx); triple(v,0x43u,ctx); set_getup(a,v,ctx); setmode_normal(v);
    if((v->y_int-v->ground_y)<20 || a->wrestler_num==5) {
        anim(v,WM_R1_ANIM_KNOCKDOWN,ctx); collis_off(v,ctx); return;
    }
    anim(v,WM_R1_ANIM_FALL_BACK,ctx); collis_off(v,ctx);
    v->x_vel=push_away(a,v,FX16(2)); v->y_vel=-0x00040000;
}

static void hit_bstomp2(wm_arcade_actor_t *a,wm_arcade_actor_t *v,wm_arcade_react1_context_t *ctx)
{
    if(v->player_mode==WM_PMODE_BLOCK) { wm_arcade_react1_block_hit_flail(a,v,ctx); return; }
    sound(v,WM_R1_SND_FLYKICK,ctx); collis_off(v,ctx);
    if(a->combo_count==0) {
        if(v->player_mode==WM_PMODE_ONGROUND || v->player_mode==WM_PMODE_DEAD) {
            /* REACT4 jumps back to the preceding #isdead collision-off tail. */
            collis_off(v,ctx);
            return;
        }
    } else {
        v->obj_control ^= WM_OBJ_FLIPH; v->y_vel=0; v->y_int=v->ground_y;
    }
    set_getup(a,v,ctx); sound(v,WM_R1_SND_SCREAM,ctx); triple(v,0x43u,ctx); grade(a,v,WM_R_MOVE_NASTY,ctx);
    anim(v,WM_R1_ANIM_KNOCKDOWN,ctx); collis_off(v,ctx);
}

static void hit_hammer(wm_arcade_actor_t *a,wm_arcade_actor_t *v,wm_arcade_react1_context_t *ctx)
{
    triple(v,0x45u,ctx); grade(a,v,WM_R_MOVE_NASTY,ctx);
    if(v->player_mode==WM_PMODE_BLOCK) { wm_arcade_react1_block_hit(a,v,ctx); return; }
    hit_bstomp(a,v,ctx);
}

static void hit_spinkick(wm_arcade_actor_t *a,wm_arcade_actor_t *v,wm_arcade_react1_context_t *ctx)
{
    if(v->player_mode==WM_PMODE_BLOCK) { wm_arcade_react1_block_hit_flail(a,v,ctx); return; }
    triple(v,0x43u,ctx); grade(a,v,WM_R_MOVE_AVERAGE,ctx);
    if(v->life==0) { collis_off(v,ctx); return; }
    sound(v,WM_R1_SND_KICK,ctx); setmode_normal(v);
    if((v->y_int-v->ground_y)<20) { anim(v,WM_R1_ANIM_SPINKICK_HEAD_HIT,ctx); collis_off(v,ctx); return; }
    anim(v,WM_R1_ANIM_FALL_BACK,ctx); collis_off(v,ctx); v->x_vel=push_away(a,v,FX16(3));
}

static void hit_cline(wm_arcade_actor_t *a,wm_arcade_actor_t *v,wm_arcade_react1_context_t *ctx)
{
    if(v->player_mode==WM_PMODE_BLOCK) { wm_arcade_react1_block_hit_flail(a,v,ctx); return; }
    impact(a,v,WM_R1_IMPACT_DROP_KICK,ctx); sound(v,WM_R1_SND_FLYKICK,ctx);
    if(v->life!=0) setmode_normal(v);
    a->z_vel=0; v->z_fixed=a->z_fixed-FX16(1); v->roll_pos=v->z_fixed; set_getup(a,v,ctx);
    anim(v,WM_R1_ANIM_FALL_BACK2,ctx);
    v->x_vel=(a->x_vel>0)?FX16(3):-FX16(3);
    collis_off(v,ctx);
}

static void hit_jumpkick(wm_arcade_actor_t *a,wm_arcade_actor_t *v,wm_arcade_react1_context_t *ctx)
{
    if(v->player_mode==WM_PMODE_BLOCK) { wm_arcade_react1_block_hit_flail(a,v,ctx); return; }
    if(v->life==0) { collis_off(v,ctx); return; }
    sound(v,WM_R1_SND_KICK,ctx); setmode_normal(v);
    if((v->y_int-v->ground_y)<20) { anim(v,WM_R1_ANIM_JUMPKICK_HEAD_HIT,ctx); collis_off(v,ctx); return; }
    anim(v,WM_R1_ANIM_FALL_BACK,ctx); collis_off(v,ctx); v->x_vel=push_away(a,v,FX16(3));
}

int wm_arcade_react4_supports(wm_arcade_reaction_id_t r)
{
    switch(r) {
    case WM_RXN_STOMP: case WM_RXN_BUTTSTOMP: case WM_RXN_BSTOMP: case WM_RXN_BSTOMP2:
    case WM_RXN_HAMMER: case WM_RXN_SPINKICK: case WM_RXN_CLINE: case WM_RXN_HEADHOLD: case WM_RXN_JUMPKICK:
        return 1; default: return 0;
    }
}

int wm_arcade_react4_apply(wm_arcade_actor_t *a,wm_arcade_actor_t *v,wm_arcade_reaction_id_t r,
                           int16_t *pending,int16_t *newdir,wm_arcade_react1_context_t *ctx)
{
    (void)newdir;
    if(!a||!v||!wm_arcade_react4_supports(r)) return 0;
    if(ctx){ctx->last_reaction=r;ctx->last_supported=1;ctx->last_flykick_aborted=0;}
    switch(r) {
    case WM_RXN_STOMP: case WM_RXN_BUTTSTOMP: hit_stomp(a,v,pending,ctx); break;
    case WM_RXN_BSTOMP: hit_bstomp(a,v,ctx); break;
    case WM_RXN_BSTOMP2: hit_bstomp2(a,v,ctx); break;
    case WM_RXN_HAMMER: hit_hammer(a,v,ctx); break;
    case WM_RXN_SPINKICK: hit_spinkick(a,v,ctx); break;
    case WM_RXN_CLINE: hit_cline(a,v,ctx); break;
    case WM_RXN_HEADHOLD: break; /* exact bare RETS */
    case WM_RXN_JUMPKICK: hit_jumpkick(a,v,ctx); break;
    default: return 0;
    }
    return 1;
}

void wm_arcade_react1234_reaction_callback(wm_arcade_actor_t *a,wm_arcade_actor_t *v,wm_arcade_reaction_id_t r,
                                           int16_t *pending,int16_t *newdir,void *user)
{
    wm_arcade_react1_context_t *ctx=(wm_arcade_react1_context_t*)user;
    if(wm_arcade_react4_supports(r)){(void)wm_arcade_react4_apply(a,v,r,pending,newdir,ctx);return;}
    wm_arcade_react123_reaction_callback(a,v,r,pending,newdir,user);
}
