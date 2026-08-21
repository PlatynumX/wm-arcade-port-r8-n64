#include "wmania_ring_climb.h"

#include <stdlib.h>
#include <string.h>

const uint8_t wm_ring_face_turnbuckle[10] = {
    1u,1u,1u,0u,0u,1u,0u,0u,1u,0u
};

const char *const wm_ring_climb_up_anims[10] = {
    "hrt_climb_up_anim","rzr_climb_up_anim","und_climb_up_anim",
    "yok_climb_up_anim","shn_climb_up_anim","bam_climb_up_anim",
    "dnk_climb_up_anim","dnk_climb_up_anim","lex_climb_up_anim",
    "dnk_climb_up_anim"
};

const char *const wm_ring_climbthru_bot_anims[10] = {
    "hrt_climbthru_bot_anim","rzr_climbthru_bot_anim","und_climbthru_bot_anim",
    "yok_climbthru_bot_anim","shn_climbthru_bot_anim","bam_climbthru_bot_anim",
    "dnk_climbthru_bot_anim","dnk_climbthru_bot_anim","lex_climbthru_bot_anim",
    "dnk_climbthru_bot_anim"
};

const char *const wm_ring_climbthru_top_anims[10] = {
    "hrt_climbthru_top_anim","rzr_climbthru_top_anim","und_climbthru_top_anim",
    "yok_climbthru_top_anim","shn_climbthru_top_anim","bam_climbthru_top_anim",
    "dnk_climbthru_top_anim","dnk_climbthru_top_anim","lex_climbthru_top_anim",
    "dnk_climbthru_top_anim"
};

const char *const wm_ring_climbin_bot_anims[10] = {
    "hrt_climbin_bot_anim","rzr_climbin_bot_anim","und_climbin_bot_anim",
    "yok_climbin_bot_anim","shn_climbin_bot_anim","bam_climbin_bot_anim",
    "dnk_climbin_bot_anim","dnk_climbin_bot_anim","lex_climbin_bot_anim",
    "dnk_climbin_bot_anim"
};

const char *const wm_ring_climbin_top_anims[10] = {
    "hrt_climbin_top_anim","rzr_climbin_top_anim","und_climbin_top_anim",
    "yok_climbin_top_anim","shn_climbin_top_anim","bam_climbin_top_anim",
    "dnk_climbin_top_anim","dnk_climbin_top_anim","lex_climbin_top_anim",
    "dnk_climbin_top_anim"
};

const char *const wm_ring_climbthru_side_anims[10] = {
    "hrt_climbthru_side_anim","rzr_climbthru_side_anim","und_climbthru_side_anim",
    "yok_climbthru_side_anim","shn_climbthru_side_anim","bam_climbthru_side_anim",
    "dnk_climbthru_side_anim","dnk_climbthru_side_anim","lex_climbthru_side_anim",
    "dnk_climbthru_side_anim"
};

const char *const wm_ring_climbin_side_anims[10] = {
    "hrt_climbin_side_anim","rzr_climbin_side_anim","und_climbin_side_anim",
    "yok_climbin_side_anim","shn_climbin_side_anim","bam_climbin_side_anim",
    "dnk_climbin_side_anim","dnk_climbin_side_anim","lex_climbin_side_anim",
    "dnk_climbin_side_anim"
};

const char *const wm_ring_rollthru_top_anims[9] = {
    "hrt_rollthru_top_anim","rzr_rollthru_top_anim","und_rollthru_top_anim",
    "yok_rollthru_top_anim","shn_rollthru_top_anim","bam_rollthru_top_anim",
    "dnk_rollthru_top_anim",0,"lex_rollthru_top_anim"
};

static WmRingClimbResult none(void)
{
    WmRingClimbResult r;
    memset(&r, 0, sizeof(r));
    r.action = WM_RING_CLIMB_ACTION_NONE;
    return r;
}

static const char *table_anim(
    const char *const *table,
    size_t count,
    uint8_t wrestler)
{
    return wrestler < count ? table[wrestler] : 0;
}

static WmRingClimbResult start_anim(
    WmRingClimbPlayer *p,
    const char *label)
{
    WmRingClimbResult r = none();

    if (label == 0) {
        r.action = WM_RING_CLIMB_ACTION_SOURCE_NULL_ANIMATION;
        return r;
    }

    p->animbase_label = label;
    r.action = WM_RING_CLIMB_ACTION_START_ANIMATION;
    r.source_animation_label = label;
    return r;
}

static bool mode_is(
    const WmRingClimbPlayer *p,
    int16_t a, int16_t b, int16_t c)
{
    return p->player_mode == a ||
           p->player_mode == b ||
           p->player_mode == c;
}

bool wm_ring_any_opp_outside(
    const WmRingClimbPlayer *self,
    const WmRingClimbPlayer *players,
    size_t player_count,
    size_t *source_a0_next_slot)
{
    size_t i;

    if (source_a0_next_slot != 0) {
        *source_a0_next_slot = 0u;
    }

    if (self == 0 || players == 0) {
        return false;
    }

    for (i = 0u; i < player_count; ++i) {
        const WmRingClimbPlayer *p = &players[i];

        /* exact any_opp_outside skips inactive, teammate, dead */
        if (!p->active) continue;
        if (p->player_side == self->player_side) continue;
        if (p->player_mode == WM_RING_MODE_DEAD) continue;

        if (p->inring != 0) {
            /*
             * Source MOVE *a0+,a3,L has already post-incremented a0.
             */
            if (source_a0_next_slot != 0) {
                *source_a0_next_slot = i + 1u;
            }
            return true;
        }
    }

    return false;
}

bool wm_ring_idiot_check(
    WmRingClimbPlayer *player,
    uint32_t pcnt)
{
    uint32_t last_plus_one;

    if (player == 0) {
        return false;
    }

    if (player->climb_last == pcnt) {
        return false;
    }

    last_plus_one = player->climb_last + 1u;
    if (last_plus_one != pcnt) {
        player->climb_start = pcnt;
        player->climb_last = pcnt;
        return false;
    }

    player->climb_last = pcnt;

    /*
     * Source unsigned PCNT subtraction naturally wraps.
     */
    return (uint32_t)(pcnt - player->climb_start) >= WM_RING_IDIOT_COUNT;
}

WmRingClimbResult wm_ring_climb_turnbuckle(
    WmRingClimbPlayer *p,
    const WmRingClimbPlayer *players,
    size_t player_count,
    int16_t rope_x)
{
    uint16_t target;
    size_t i;
    WmRingClimbResult r = none();

    if (p == 0) return r;

    if ((p->stick_val_cur & (1u << WM_RING_MOVE_UP_BIT)) == 0u) return r;
    if (p->inring != 0) return r;
    if (p->z_int > WM_RING_TOP + 5) return r;

    if (p->x_int <= WM_RING_X_CENTER) {
        /* source accepts rope_x >= OBJ_COLLX1-5 */
        if (rope_x < p->coll_x1 - 5) return r;
        target = WM_RING_MOVE_UP_LEFT;
        if (p->stick_val_cur != WM_RING_MOVE_UP_LEFT) return r;
    } else {
        /* source accepts rope_x <= OBJ_COLLX2+5 */
        if (rope_x > p->coll_x2 + 5) return r;
        target = WM_RING_MOVE_UP_RIGHT;
        if (p->stick_val_cur != WM_RING_MOVE_UP_RIGHT) return r;
    }

    /*
     * Reject if another active process is already on/climbing the same
     * left/right turnbuckle. Exact source comparisons are strict.
     */
    if (players != 0) {
        for (i = 0u; i < player_count; ++i) {
            const WmRingClimbPlayer *q = &players[i];

            if (!q->active) continue;
            if (q == p) continue;
            if (q->player_mode != WM_RING_MODE_ONTURNBUCKLE &&
                q->player_mode != WM_RING_MODE_CLIMBTURNBUCKLE) {
                continue;
            }

            if (p->x_int <= WM_RING_X_CENTER) {
                if (q->x_int < WM_RING_X_CENTER) return r;
            } else {
                if (q->x_int > WM_RING_X_CENTER) return r;
            }
        }
    }

    /* source glitches near misses exactly to RING_TOP */
    p->z_int = WM_RING_TOP;
    p->z_fp16 = ((int32_t)WM_RING_TOP) << 16;

    if (p->wrestler_num < 10u &&
        wm_ring_face_turnbuckle[p->wrestler_num] != 0u) {
        target ^= (WM_RING_MOVE_UP |
                   WM_RING_MOVE_DOWN |
                   WM_RING_MOVE_LEFT |
                   WM_RING_MOVE_RIGHT);
    }

    p->new_facing_dir = target;
    r.target_facing = target;

    if (p->facing_dir == target) {
        r = start_anim(
            p, table_anim(wm_ring_climb_up_anims, 10u, p->wrestler_num));
        p->player_mode = WM_RING_MODE_CLIMBTURNBUCKLE;
        r.target_facing = target;
        return r;
    }

    /* set_rotate_anim/change_anim1a, CODE_ADDR=#climb, WAITANIM */
    p->player_mode = WM_RING_MODE_WAITANIM;
    r.action = WM_RING_CLIMB_ACTION_ROTATE_THEN_CONTINUE;
    r.continuation = WM_RING_CLIMB_CONT_TURNBUCKLE;
    r.target_facing = target;
    return r;
}

WmRingClimbResult wm_ring_ck_climb_out_bot(
    WmRingClimbPlayer *p,
    const WmRingClimbPlayer *players,
    size_t player_count,
    uint32_t pcnt)
{
    WmRingClimbResult r = none();

    if (p == 0) return r;
    if (mode_is(p, WM_RING_MODE_ATTACHED, WM_RING_MODE_RUNNING,
                WM_RING_MODE_OPPOVERHEAD)) return r;
    if (!wm_ring_any_opp_outside(p, players, player_count, 0)) return r;
    if (abs((int)p->x_int - WM_RING_X_CENTER) > 0x10d) return r;
    if ((p->anim_mode & WM_RING_ANIM_MODE_UNINT) != 0u) return r;
    if (p->z_fp16 != (((int32_t)WM_RING_BOT) << 16)) return r;

    if (!wm_ring_idiot_check(p, pcnt) && p->but_val_cur == 0u) return r;

    r = start_anim(
        p, table_anim(wm_ring_climbthru_bot_anims, 10u, p->wrestler_num));
    p->climbing_thru = 2;
    return r;
}

WmRingClimbResult wm_ring_ck_climb_in_top(
    WmRingClimbPlayer *p,
    uint32_t pcnt)
{
    WmRingClimbResult r = none();

    if (p == 0) return r;

    if (p->player_mode == WM_RING_MODE_ATTACHED ||
        p->player_mode == WM_RING_MODE_DEAD ||
        p->player_mode == WM_RING_MODE_RUNNING ||
        p->player_mode == WM_RING_MODE_INAIR ||
        p->player_mode == WM_RING_MODE_INAIR2 ||
        p->player_mode == WM_RING_MODE_OPPOVERHEAD) return r;

    if (p->climbing_thru != 0) return r;
    if (abs((int)p->x_int - WM_RING_X_CENTER) > 0x0c0) return r;
    if ((p->anim_mode & WM_RING_ANIM_MODE_UNINT) != 0u) return r;
    if (p->z_fp16 != (((int32_t)(WM_MAT_TOP - 5)) << 16)) return r;
    if ((p->move_dir & (1u << WM_RING_MOVE_DOWN_BIT)) == 0u) return r;

    if (!wm_ring_idiot_check(p, pcnt) && p->but_val_cur == 0u) return r;

    r = start_anim(
        p, table_anim(wm_ring_climbin_top_anims, 10u, p->wrestler_num));
    p->climbing_thru = 2;
    return r;
}

WmRingClimbResult wm_ring_ck_climb_out_top(
    WmRingClimbPlayer *p,
    const WmRingClimbPlayer *players,
    size_t player_count,
    uint32_t pcnt)
{
    WmRingClimbResult r = none();
    const char *roll;

    if (p == 0) return r;

    /*
     * Source zombie branch occurs before every normal eligibility check.
     */
    if ((p->status_flags & WM_RING_STATUS_ZOMBIE) != 0u) {
        roll = table_anim(
            wm_ring_rollthru_top_anims, 9u, p->wrestler_num);

        if (roll != 0 && p->animbase_label != 0 &&
            strcmp(roll, p->animbase_label) == 0) {
            return r;
        }

        r = start_anim(p, roll);
        if (r.action == WM_RING_CLIMB_ACTION_START_ANIMATION) {
            p->climbing_thru = 2;
        }
        return r;
    }

    if (mode_is(p, WM_RING_MODE_ATTACHED, WM_RING_MODE_RUNNING,
                WM_RING_MODE_OPPOVERHEAD)) return r;
    if (!wm_ring_any_opp_outside(p, players, player_count, 0)) return r;
    if (abs((int)p->x_int - WM_RING_X_CENTER) > 0x0c0) return r;
    if ((p->anim_mode & WM_RING_ANIM_MODE_UNINT) != 0u) return r;
    if (p->z_fp16 != (((int32_t)WM_RING_TOP) << 16)) return r;

    if (!wm_ring_idiot_check(p, pcnt) && p->but_val_cur == 0u) return r;

    r = start_anim(
        p, table_anim(wm_ring_climbthru_top_anims, 10u, p->wrestler_num));
    p->climbing_thru = 2;
    return r;
}

WmRingClimbResult wm_ring_ck_climb_in_bot(
    WmRingClimbPlayer *p,
    uint32_t pcnt)
{
    WmRingClimbResult r = none();

    if (p == 0) return r;

    if (p->player_mode == WM_RING_MODE_ATTACHED ||
        p->player_mode == WM_RING_MODE_DEAD ||
        p->player_mode == WM_RING_MODE_RUNNING ||
        p->player_mode == WM_RING_MODE_INAIR ||
        p->player_mode == WM_RING_MODE_INAIR2 ||
        p->player_mode == WM_RING_MODE_OPPOVERHEAD) return r;

    if (p->climbing_thru != 0) return r;
    if (abs((int)p->x_int - WM_RING_X_CENTER) > 0x10d) return r;
    if ((p->anim_mode & WM_RING_ANIM_MODE_UNINT) != 0u) return r;
    if (p->z_fp16 != (((int32_t)(WM_MAT_BOT + 5)) << 16)) return r;
    if ((p->move_dir & (1u << WM_RING_MOVE_UP_BIT)) == 0u) return r;

    if (!wm_ring_idiot_check(p, pcnt) && p->but_val_cur == 0u) return r;

    r = start_anim(
        p, table_anim(wm_ring_climbin_bot_anims, 10u, p->wrestler_num));
    p->climbing_thru = 2;
    return r;
}

WmRingClimbResult wm_ring_ck_climb_out_side(
    WmRingClimbPlayer *p,
    const WmRingClimbPlayer *players,
    size_t player_count,
    uint32_t pcnt,
    int16_t rope_x,
    WmRingSourceQuirkReadFn source_quirk_read,
    void *source_quirk_user)
{
    WmRingClimbResult r = none();
    size_t next_slot = 0u;
    int16_t quirk_word = 0;
    uint16_t target;
    bool idiot;

    if (p == 0) return r;

    if (mode_is(p, WM_RING_MODE_ATTACHED, WM_RING_MODE_RUNNING,
                WM_RING_MODE_OPPOVERHEAD)) return r;

    if (!wm_ring_any_opp_outside(
            p, players, player_count, &next_slot)) return r;

    /*
     * Preserve the current source's post-any_opp_outside a0 bug literally.
     */
    if (source_quirk_read == 0 ||
        !source_quirk_read(source_quirk_user, next_slot, &quirk_word)) {
        r.action = WM_RING_CLIMB_ACTION_SOURCE_QUIRK_INPUT_REQUIRED;
        return r;
    }
    if (quirk_word == 1) return r;

    if (abs((int)p->z_int - WM_RING_Z_CENTER) > 0x48) return r;
    if ((p->anim_mode & WM_RING_ANIM_MODE_UNINT) != 0u) return r;

    if (rope_x <= WM_RING_X_CENTER) {
        if ((p->stick_val_cur & (1u << WM_RING_MOVE_LEFT_BIT)) == 0u) return r;
        /* source JRLE after (OBJ_COLLX1 - rope_x) */
        if (p->coll_x1 > rope_x) return r;
        target = WM_RING_MOVE_DOWN_LEFT;
    } else {
        if ((p->stick_val_cur & (1u << WM_RING_MOVE_RIGHT_BIT)) == 0u) return r;
        /* source JRGE after (OBJ_COLLX2 - rope_x) */
        if (p->coll_x2 < rope_x) return r;
        target = WM_RING_MOVE_DOWN_RIGHT;
    }

    idiot = wm_ring_idiot_check(p, pcnt);
    if (idiot) {
        /* #special_face: WAITANIM means do nothing; otherwise face. */
        if (p->player_mode == WM_RING_MODE_WAITANIM) return r;
    } else if (p->but_val_cur == 0u) {
        return r;
    }

    p->new_facing_dir = target;
    r.target_facing = target;

    if (p->facing_dir == target) {
        r = start_anim(
            p, table_anim(
                wm_ring_climbthru_side_anims, 10u, p->wrestler_num));
        p->player_mode = WM_RING_MODE_NORMAL;
        p->climbing_thru = 1;
        r.target_facing = target;
        return r;
    }

    /* rotate -> CODE_ADDR=#climb -> WAITANIM */
    p->player_mode = WM_RING_MODE_WAITANIM;
    p->climbing_thru = 1;
    r.action = WM_RING_CLIMB_ACTION_ROTATE_THEN_CONTINUE;
    r.continuation = WM_RING_CLIMB_CONT_OUT_SIDE;
    r.target_facing = target;
    return r;
}

WmRingClimbResult wm_ring_ck_climb_in_side(
    WmRingClimbPlayer *p,
    uint32_t pcnt,
    int16_t calc_line_x_result)
{
    WmRingClimbResult r = none();
    uint16_t move_bit;
    uint16_t target;
    int16_t collision_edge;
    bool running;
    bool idiot;

    if (p == 0) return r;

    if (p->player_mode == WM_RING_MODE_ATTACHED ||
        p->player_mode == WM_RING_MODE_DEAD ||
        p->player_mode == WM_RING_MODE_OPPOVERHEAD) return r;

    if (p->climbing_thru != 0) return r;
    if (abs((int)p->z_int - WM_RING_Z_CENTER) > 0x0d8) return r;

    if (p->x_int < WM_RING_X_CENTER) {
        move_bit = WM_RING_MOVE_RIGHT_BIT;
        target = WM_RING_MOVE_DOWN_RIGHT;
        collision_edge = p->coll_x2;
    } else {
        move_bit = WM_RING_MOVE_LEFT_BIT;
        target = WM_RING_MOVE_DOWN_LEFT;
        collision_edge = p->coll_x1;
    }

    running = p->player_mode == WM_RING_MODE_RUNNING;
    if (running) {
        if (p->getup_time != 0) return r;
    } else {
        if ((p->anim_mode & WM_RING_ANIM_MODE_UNINT) != 0u) return r;

        if (p->player_mode == WM_RING_MODE_HEADHELD ||
            p->player_mode == WM_RING_MODE_HEADHOLD ||
            p->player_mode == WM_RING_MODE_INAIR ||
            p->player_mode == WM_RING_MODE_INAIR2) return r;
    }

    /* exact calc_line_x result must be supplied by translated collision code */
    if (abs((int)calc_line_x_result - (int)collision_edge) > 10) return r;

    if ((p->move_dir & (1u << move_bit)) == 0u) return r;

    idiot = wm_ring_idiot_check(p, pcnt);
    if (!idiot && !running && p->but_val_cur == 0u) return r;

    p->new_facing_dir = target;
    r.target_facing = target;

    if (p->facing_dir == target) {
        r = start_anim(
            p, table_anim(wm_ring_climbin_side_anims, 10u, p->wrestler_num));
        p->player_mode = WM_RING_MODE_NORMAL;
        p->climbing_thru = 1;
        r.target_facing = target;
        return r;
    }

    /* rotate -> CODE_ADDR=#jump_in -> WAITANIM */
    p->player_mode = WM_RING_MODE_WAITANIM;
    p->climbing_thru = 1;
    r.action = WM_RING_CLIMB_ACTION_ROTATE_THEN_CONTINUE;
    r.continuation = WM_RING_CLIMB_CONT_IN_SIDE;
    r.target_facing = target;
    return r;
}

WmRingClimbResult wm_ring_climb_continue(
    WmRingClimbPlayer *p,
    WmRingClimbContinuation continuation)
{
    WmRingClimbResult r = none();

    if (p == 0) return r;

    switch (continuation) {
    case WM_RING_CLIMB_CONT_TURNBUCKLE:
        r = start_anim(
            p, table_anim(wm_ring_climb_up_anims, 10u, p->wrestler_num));
        p->player_mode = WM_RING_MODE_CLIMBTURNBUCKLE;
        return r;

    case WM_RING_CLIMB_CONT_OUT_SIDE:
        r = start_anim(
            p, table_anim(
                wm_ring_climbthru_side_anims, 10u, p->wrestler_num));
        p->player_mode = WM_RING_MODE_NORMAL;
        p->climbing_thru = 1;
        return r;

    case WM_RING_CLIMB_CONT_IN_SIDE:
        r = start_anim(
            p, table_anim(wm_ring_climbin_side_anims, 10u, p->wrestler_num));
        p->player_mode = WM_RING_MODE_NORMAL;
        p->climbing_thru = 1;
        return r;

    case WM_RING_CLIMB_CONT_NONE:
    default:
        return r;
    }
}
