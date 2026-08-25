#include "wm_arcade_wrestle_core.h"

static wm_arcade_actor_t *find_player(wm_arcade_actor_t **actors, size_t n, int32_t player_num)
{
    size_t i;
    if (!actors) return 0;
    for (i=0;i<n;++i) if (actors[i] && actors[i]->active && actors[i]->player_num==player_num) return actors[i];
    return 0;
}

void wm_arcade_update_links(wm_arcade_actor_t *a)
{
    wm_arcade_actor_t *o;
    if (!a) return;
    o=a->attach_proc;
    if (o && o->attach_proc!=a) a->attach_proc=0;
}

void wm_arcade_set_wrestler_xflip(wm_arcade_actor_t *a)
{
    if (!a) return;
    if ((a->facing_dir & WM_MOVE_RIGHT)!=0) a->obj_control &= (uint16_t)~WM_OBJ_FLIPH;
    else a->obj_control |= WM_OBJ_FLIPH;
}

void wm_arcade_count_button_presses(wm_arcade_actor_t *a)
{
    uint16_t b;
    if (!a) return;
    b=a->but_val_down;
    if (b & WM_BTN_PUNCH) ++a->punchb_count;
    if (b & WM_BTN_BLOCK) ++a->blockb_count;
    if (b & WM_BTN_SPUNCH) ++a->spunchb_count;
    if (b & WM_BTN_KICK) ++a->kickb_count;
    if (b & WM_BTN_SKICK) ++a->skickb_count;
}

int wm_arcade_auto_pin_check(wm_arcade_actor_t *a, wm_arcade_actor_t *closest, const wm_arcade_auto_pin_env_t *env)
{
    if (!a) return 0;
    if (env && (env->in_finish_move || env->finish_completed || env->royal_rumble)) return 0;
    if (!closest || closest->player_mode!=WM_PMODE_DEAD) { a->auto_pin_countdown=0; return 0; }
    if ((a->status_flags & WM_STATUS_DID_PIN)!=0u) return 0;
    if ((closest->status_flags & WM_STATUS_ZOMBIE)!=0u) { a->auto_pin_countdown=0; return 0; }
    if (env && env->anyone_bucking) return 0;
    ++a->auto_pin_countdown;
    /* WRESTLE.ASM comment says four seconds; executable code compares TSEC*3. */
    if (a->auto_pin_countdown < (uint16_t)(WM_ARCADE_TSEC*3u)) return 0;
    if ((a->anim_mode & WM_ARCADE_MODE_UNINT)!=0u) return 0;
    a->player_type=WM_PTYPE_DRONE;
    return 1;
}

int wm_arcade_can_pin(wm_arcade_actor_t *a, wm_arcade_actor_t **actors, size_t actor_count)
{
    size_t i;
    wm_arcade_actor_t *o;
    if (!a || !actors) return 0;
    for (i=0;i<actor_count;++i) {
        o=actors[i];
        if (!o || !o->active || o->player_side==a->player_side) continue;
        if (o->player_mode!=WM_PMODE_DEAD || (o->status_flags & WM_STATUS_ZOMBIE)!=0u) return 0;
    }
    if (a->closest_dist>0x70 || a->closest_zdist>0x50) return 0;
    o=find_player(actors,actor_count,a->closest_num);
    if (!o || (o->status_flags & WM_STATUS_PINABLE)==0u) return 0;
    o->status_flags |= WM_STATUS_PINNED;
    o->who_pinned_me=a;
    o->x_vel=o->y_vel=o->z_vel=0;
    o->ptime=1;
    o->status_flags &= ~WM_STATUS_KOD;
    return 1;
}

void wm_arcade_hit_nearest_for_pin(wm_arcade_actor_t *a, wm_arcade_actor_t **actors, size_t actor_count)
{
    wm_arcade_actor_t *o;
    if (!a) return;
    o=find_player(actors,actor_count,a->closest_num);
    if (!o) return;
    a->who_i_hit=o;
    o->who_pinned_me=a;
    o->status_flags |= WM_STATUS_PINNED;
}

void wm_arcade_wrestler_countdown_tail(wm_arcade_actor_t *a, bool match_time_zero)
{
    uint16_t pressed;
    uint32_t old_flags,new_flags;
    if (!a) return;
    if (a->delay_butns!=0) --a->delay_butns;
    if (a->safe_time!=0) --a->safe_time;
    if (a->delay_meter!=0) --a->delay_meter;
    if (a->immobilize_time!=0) --a->immobilize_time;
    if (a->walk_fast>0) --a->walk_fast;
    if (a->getup_time==0) return;
    if (match_time_zero) { a->getup_time=0; return; }
    if (a->delay_meter!=0) { a->getup_time=0; return; }
    --a->getup_time;
    if (a->getup_time==0) goto clear_dizzy;

    pressed=(uint16_t)(a->but_val_down|a->stick_val_down);
    old_flags=a->status_flags;
    new_flags=old_flags & ~WM_STATUS_PRESS_LAST;
    if (pressed!=0) new_flags|=WM_STATUS_PRESS_LAST;
    a->status_flags=new_flags;
    if (pressed!=0 || (old_flags & WM_STATUS_PRESS_LAST)!=0u) {
        a->getup_time-=3;
        if (a->getup_time<0) a->getup_time=0;
    }
    if (a->getup_time!=0) return;
clear_dizzy:
    a->dizzy=0;
    a->stars_flag=0;
    a->delay_butns=40;
}

void wm_arcade_reset_wrestle2_state(wm_arcade_actor_t *a)
{
    if (!a) return;
    a->immobilize_time=30;
    a->dizzy_count=0;
    a->getup_time=0;
    a->auto_pin_countdown=0;
    a->special_move_addr=0;
    a->last_hit_time=0;
    a->last_fling_attempt=0;
    a->hit_gate_time=0;
    a->last_headhold=0;
    a->last_spunch=0;
    a->last_skick=0;
    a->consecutive_hits=0;
    a->last_fling=0;
    a->last_hiptoss=0;
    a->last_damage=0;
    a->status_flags &= WM_STATUS_RESET_MASK;
    a->ptime=1;
}
