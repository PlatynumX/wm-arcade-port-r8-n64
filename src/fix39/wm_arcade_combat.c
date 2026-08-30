#include "wm_arcade_combat.h"

static int boxes_overlap_inclusive(const wm_arcade_box3_t *a,
                                   const wm_arcade_box3_t *b)
{
    /* COLLIS.ASM uses < and > rejection, so touching edges are accepted. */
    if (a->x1 > b->x2 || a->x2 < b->x1) return 0;
    if (a->y1 > b->y2 || a->y2 < b->y1) return 0;
    if (a->z1 > b->z2 || a->z2 < b->z1) return 0;
    return 1;
}

/* R37N9: PLYR.EQU lays each 16.16 position out as adjacent WORD fields
 * OBJ_*POS (fraction) + OBJ_*POSINT (signed integer).  COLLIS.ASM writes
 * OBJ_XPOSINT/OBJ_ZPOSINT directly during overlap separation.  In C these
 * aliases are represented as separate fields, so emulate the source WORD
 * write: replace only the high 16 bits and preserve the fractional low word. */
static int32_t source_write_posint_word(int32_t fixed, int32_t integer)
{
    uint32_t lo = (uint32_t)fixed & 0xffffu;
    uint32_t hi = (uint32_t)(uint16_t)(int16_t)integer << 16;
    return (int32_t)(hi | lo);
}

static void source_write_xposint(wm_arcade_actor_t *actor, int32_t integer)
{
    int16_t word;
    if (!actor) return;
    word = (int16_t)integer;
    actor->x_int = (int32_t)word;
    actor->x_fixed = source_write_posint_word(actor->x_fixed, word);
}

static void source_write_zposint(wm_arcade_actor_t *actor, int32_t integer)
{
    int16_t word;
    if (!actor) return;
    word = (int16_t)integer;
    actor->z_int = (int32_t)word;
    actor->z_fixed = source_write_posint_word(actor->z_fixed, word);
}

void wm_arcade_set_hurt_box(wm_arcade_actor_t *actor,
                            const wm_arcade_frame_box_t *frame)
{
    int32_t zoff = -30;
    int32_t zdepth = 60;
    int32_t y2;

    if (!actor || !frame) return;

    if (actor->player_mode == WM_PMODE_ONGROUND) {
        zoff = -15;
        zdepth = 30;
    } else if (actor->player_mode == WM_PMODE_RUNNING) {
        zoff = -5;
        zdepth = 10;
    }

    y2 = actor->y_int - frame->iani3y;
    actor->hurt_box.y2 = y2;
    actor->hurt_box.y1 = y2 - frame->iani3id;

    actor->hurt_box.z1 = actor->z_int + zoff;
    actor->hurt_box.z2 = actor->hurt_box.z1 + zdepth;

    if (actor->obj_control & WM_OBJ_FLIPH) {
        actor->hurt_box.x2 = actor->x_int - frame->iani3x;
        actor->hurt_box.x1 = actor->hurt_box.x2 - frame->iani3z;
    } else {
        actor->hurt_box.x1 = actor->x_int + frame->iani3x;
        actor->hurt_box.x2 = actor->hurt_box.x1 + frame->iani3z;
    }
}

void wm_arcade_set_attack_box(wm_arcade_actor_t *actor)
{
    if (!actor) return;

    actor->attack_box.y1 = actor->y_int + actor->attack_yoff;
    actor->attack_box.y2 = actor->attack_box.y1 + actor->attack_height;

    actor->attack_box.z1 = actor->z_int + actor->attack_zoff;
    actor->attack_box.z2 = actor->attack_box.z1 + actor->attack_depth;

    if (actor->obj_control & WM_OBJ_FLIPH) {
        actor->attack_box.x2 = actor->x_int - actor->attack_xoff;
        actor->attack_box.x1 = actor->attack_box.x2 - actor->attack_width;
    } else {
        actor->attack_box.x1 = actor->x_int + actor->attack_xoff;
        actor->attack_box.x2 = actor->attack_box.x1 + actor->attack_width;
    }
}

int wm_arcade_resolve_overlap(wm_arcade_actor_t *mover,
                              const wm_arcade_actor_t *other)
{
    int32_t rox, lox, boz, toz;
    int32_t min_x, min_z;

    if (!mover || !other || mover == other) return 0;

    if (mover->status_flags & WM_STATUS_ZOMBIE) return 0;
    if (other->player_mode == WM_PMODE_DEAD) return 0;
    if (other->anim_mode & WM_ARCADE_MODE_OVERLAP) return 0;
    if (other->attach_proc == mover) return 0;
    if (mover->player_mode == WM_PMODE_ONGROUND ||
        mover->player_mode == WM_PMODE_DEAD) return 0;

    if ((mover->player_mode == WM_PMODE_RUNNING ||
         mover->player_mode == WM_PMODE_BOUNCING) &&
        other->player_mode == WM_PMODE_ONGROUND) return 0;

    if (mover->anim_mode & WM_ARCADE_MODE_OVERLAP) return 0;

    rox = mover->hurt_box.x2 - other->hurt_box.x1;
    if (rox <= 0) return 0;
    lox = other->hurt_box.x2 - mover->hurt_box.x1;
    if (lox <= 0) return 0;
    boz = mover->hurt_box.z2 - other->hurt_box.z1;
    if (boz <= 0) return 0;
    toz = other->hurt_box.z2 - mover->hurt_box.z1;
    if (toz <= 0) return 0;

    /* Y is tested for overlap but never used to separate the wrestlers. */
    if (mover->hurt_box.y2 - other->hurt_box.y1 <= 0) return 0;
    if (other->hurt_box.y2 - mover->hurt_box.y1 <= 0) return 0;

    if (mover->player_mode != WM_PMODE_RUNNING) {
        min_x = (rox < lox) ? rox : lox;
        min_z = (boz < toz) ? boz : toz;

        /* Exact source behavior: halve X penetration if the other guy is down. */
        if (other->player_mode == WM_PMODE_ONGROUND) min_x >>= 1;

        /* Resolve the smaller axis, except the source forces Z for a large Z glitch. */
        if (!(min_x > min_z || min_z > 0x3d)) {
            if (rox > lox) source_write_xposint(mover, mover->x_int + lox);
            else source_write_xposint(mover, mover->x_int - rox);

            if (mover->move_dir != 0 &&
                (mover->move_dir & (WM_MOVE_UP | WM_MOVE_DOWN)) == 0) {
                source_write_zposint(mover, mover->z_int + ((boz > toz) ? 3 : -3));
            }
            return 1;
        }
    }

    if (boz > toz) source_write_zposint(mover, mover->z_int + toz);
    else source_write_zposint(mover, mover->z_int - boz);

    if (mover->move_dir != 0 &&
        (mover->move_dir & (WM_MOVE_LEFT | WM_MOVE_RIGHT)) == 0) {
        source_write_xposint(mover, mover->x_int + ((rox > lox) ? 3 : -3));
    }
    return 1;
}

wm_arcade_hit_result_t wm_arcade_try_attack_hit(
    wm_arcade_actor_t *attacker,
    wm_arcade_actor_t *victim,
    const wm_arcade_combat_callbacks_t *callbacks)
{
    int hit_side;

    if (!attacker || !victim || attacker == victim) return WM_HIT_REJECTED;

    if (victim->anim_mode & WM_ARCADE_MODE_NOCOLLIS) return WM_HIT_REJECTED;
    if (attacker->immobilize_time != 0) return WM_HIT_REJECTED;
    if (!boxes_overlap_inclusive(&attacker->attack_box, &victim->hurt_box))
        return WM_HIT_REJECTED;

    if ((attacker->status_flags & WM_STATUS_SMART_ATTACK) &&
        attacker->smart_target != victim) return WM_HIT_REJECTED;

    if (attacker->combo_count != 0 && attacker->who_i_hit != victim)
        return WM_HIT_REJECTED;

    if (victim->player_mode == WM_PMODE_DEAD) {
        if (callbacks && callbacks->victim_has_live_teammates &&
            callbacks->victim_has_live_teammates(victim, callbacks->user))
            return WM_HIT_REJECTED;

        if (attacker->attack_mode == WM_AMODE_PUPPET ||
            attacker->attack_mode == WM_AMODE_PUPPET2)
            return WM_HIT_REJECTED;
    }

    if (victim->status_flags & WM_STATUS_ZOMBIE) return WM_HIT_REJECTED;

    if (!(attacker->status_flags & WM_STATUS_DID_PIN) &&
        (victim->status_flags & WM_STATUS_PINNED))
        return WM_HIT_REJECTED;

    if (attacker->in_ring != victim->in_ring) return WM_HIT_REJECTED;

    if (attacker->attack_mode == WM_AMODE_PUSH &&
        (victim->player_mode == WM_PMODE_INAIR ||
         victim->player_mode == WM_PMODE_INAIR2))
        return WM_HIT_REJECTED;

    if (victim->status_flags & WM_STATUS_PUSH) {
        if (attacker->attack_mode != WM_AMODE_FLYKICK &&
            attacker->attack_mode != WM_AMODE_BSTOMP &&
            attacker->attack_mode != WM_AMODE_BLBOWDROP)
            return WM_HIT_REJECTED;
    }

    if (attacker->anim_mode & WM_ARCADE_MODE_WAITHITOPP) {
        attacker->anim_mode &= (uint16_t)~WM_ARCADE_MODE_WAITHITOPP;
        attacker->ani_count = 0;
        attacker->ani_count2 = 0;
    }

    attacker->hit_blocker = (victim->player_mode == WM_PMODE_BLOCK) ? 1 : 0;

    hit_side = (attacker->x_int > victim->x_int) ? WM_MOVE_RIGHT : WM_MOVE_LEFT;
    hit_side |= (attacker->z_fixed > victim->z_fixed) ? WM_MOVE_DOWN : WM_MOVE_UP;
    attacker->hit_side = hit_side;
    victim->hit_side = hit_side;

    attacker->anim_mode |= WM_ARCADE_MODE_STATUS;

    if (callbacks && callbacks->wrestler_hit)
        callbacks->wrestler_hit(attacker, victim, callbacks->user);

    return WM_HIT_ACCEPTED;
}

int wm_arcade_check_wrestler_collisions(
    wm_arcade_actor_t **actors,
    size_t actor_count,
    uint32_t round_tick,
    const wm_arcade_combat_callbacks_t *callbacks)
{
    size_t outer;

    if (!actors || actor_count == 0) return 0;

    for (outer = 0; outer < actor_count; ++outer) {
        size_t ai = (round_tick & 1u) ? (actor_count - 1u - outer) : outer;
        wm_arcade_actor_t *attacker = actors[ai];
        size_t di;

        if (!attacker || !attacker->active) continue;
        if (!(attacker->anim_mode & WM_ARCADE_MODE_CHECKHIT)) continue;

        wm_arcade_set_attack_box(attacker);

        for (di = 0; di < actor_count; ++di) {
            wm_arcade_actor_t *victim = actors[di];
            if (!victim || !victim->active || victim == attacker) continue;

            if (wm_arcade_try_attack_hit(attacker, victim, callbacks) ==
                WM_HIT_ACCEPTED) {
                /* Original check_collisions exits after first successful hit. */
                return 1;
            }
        }
    }
    return 0;
}

void wm_arcade_wrestler_collisions_off(wm_arcade_actor_t *actor)
{
    if (actor) actor->anim_mode &= (uint16_t)~WM_ARCADE_MODE_CHECKHIT;
}

static int getup_table_value(uint16_t attack_mode)
{
    switch (attack_mode) {
        case WM_AMODE_FLYKICK:
        case WM_AMODE_URN:      /* GETUP.ASM labels entry 10 as _hiptoss. */
        case WM_AMODE_BIGBOOT:
        case WM_AMODE_BIGKNEE:
            return WM_STAY_TIME;
        default:
            return 0;
    }
}

static int getup_calls_maybe_gidd_up(uint16_t attack_mode)
{
    switch (attack_mode) {
        case WM_AMODE_KICK:
        case WM_AMODE_FLYKICK:
        case WM_AMODE_UPRCUT:
        case WM_AMODE_URN:
        case WM_AMODE_BIGBOOT:
        case WM_AMODE_CLINE:
        case WM_AMODE_BLBOWDROP:
        case WM_AMODE_BSTOMP:
        case WM_AMODE_HAMMER:
        case WM_AMODE_BIGKNEE:
        case WM_AMODE_SHNSPDKIK2:
        case WM_AMODE_UPRCUT2:
            return 1;
        default:
            return 0;
    }
}

void wm_arcade_set_getup_time(
    const wm_arcade_actor_t *attacker,
    wm_arcade_actor_t *victim,
    const wm_arcade_combat_callbacks_t *callbacks)
{
    uint16_t attack_mode;

    if (!attacker || !victim) return;
    if (victim->getup_time != 0) return;

    attack_mode = attacker->attack_mode;
    victim->getup_time = getup_table_value(attack_mode);

    if (victim->delay_meter != 0) return;
    if (!getup_calls_maybe_gidd_up(attack_mode)) return;

    if (callbacks && callbacks->maybe_gidd_up)
        callbacks->maybe_gidd_up(victim, callbacks->user);
}
