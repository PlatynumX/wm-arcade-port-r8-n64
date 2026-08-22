#include "wmania_ring_onscreen.h"

#include <string.h>

static WmRingGetupSpawn no_spawn(void)
{
    WmRingGetupSpawn s;
    memset(&s, 0, sizeof(s));
    return s;
}

static WmRingOnscreenEvents no_events(void)
{
    WmRingOnscreenEvents e;
    memset(&e, 0, sizeof(e));
    e.p1_getup_spawn = no_spawn();
    e.p2_getup_spawn = no_spawn();
    return e;
}

/*
 * Semantic translation of ROM helper 0xFF86E4B0 -> 0xFF86E510.
 *
 * The helper is reached only after keep_onscreen stopped a RUNNING player,
 * reset PLYRMODE to NORMAL, and cleared ANIMODE.
 */
static WmRingGetupSpawn make_getup_spawn(
    const WmRingOnscreenPlayer *p)
{
    WmRingGetupSpawn s = no_spawn();

    if (p == 0) {
        return s;
    }

    /* *A13(0460) -> GETUP_TIME; zero returns */
    if (p->getup_time == 0) {
        return s;
    }

    /* *A13(0600) -> PLYR_DIZZY; nonzero returns */
    if (p->player_dizzy != 0) {
        return s;
    }

    /* *A13(0870),L -> METER_PROC; zero returns */
    if (!p->meter_proc_exists) {
        return s;
    }

    /*
     * ROM then copies PA8/PA9/PA10 from the old METER_PROC process,
     * loads A1=0x12B and A7=0xFF86D7D0, and calls the process creator.
     */
    s.create = true;
    s.pid = WM_RING_KEEP_GETUP_PID;
    s.entry_bit_address = WM_RING_KEEP_ONSCREEN_GETUP_ENTRY;
    s.a8 = p->meter_saved_a8;
    s.a9 = p->meter_saved_a9;
    s.a10 = p->meter_saved_a10;
    return s;
}

void wm_ring_keep_onscreen_bounds(
    int16_t worldtlx_int,
    int32_t *center_out,
    int32_t *left_out,
    int32_t *right_out,
    bool *source_center_gt_ring_center_branch_out)
{
    int32_t center = (int32_t)worldtlx_int +
                     WM_RING_KEEP_WORLD_HALF_WIDTH;

    /*
     * Raw ROM:
     *   CMPI 0432,A0
     *   JRGT ...
     * and BOTH paths then:
     *   SUBI 00B9,A0
     *   ADDI 00B9,A1
     *
     * Preserve the comparison as audit metadata, but not as fake behavior.
     */
    if (source_center_gt_ring_center_branch_out != 0) {
        *source_center_gt_ring_center_branch_out =
            center > WM_RING_KEEP_RING_CENTER_COMPARE;
    }

    if (center_out != 0) {
        *center_out = center;
    }
    if (left_out != 0) {
        *left_out = center - WM_RING_KEEP_SAFE_HALF_WIDTH;
    }
    if (right_out != 0) {
        *right_out = center + WM_RING_KEEP_SAFE_HALF_WIDTH;
    }
}

/*
 * ROM helper 0xFF871500.
 *
 * Returns true only when this call zeroed a nonzero outward velocity.
 * The ROM never changes X position here.
 */
static bool stop_if_moving_outward(
    WmRingOnscreenPlayer *p,
    int32_t left_limit,
    int32_t right_limit,
    WmRingGetupSpawn *spawn_out)
{
    bool outward = false;
    bool was_running;

    if (p == 0) {
        return false;
    }

    /*
     * Raw control flow:
     *
     * if x <= left:
     *     zero velocity -> return
     *     negative velocity -> stop
     *     positive velocity -> return
     *
     * else if x < right:
     *     return
     *
     * else (x >= right):
     *     zero velocity -> return
     *     negative velocity -> return
     *     positive velocity -> stop
     *
     * Equality at either boundary is therefore active.
     */
    if ((int32_t)p->x_int <= left_limit) {
        if (p->xvel_fp < 0) {
            outward = true;
        }
    } else if ((int32_t)p->x_int >= right_limit) {
        if (p->xvel_fp > 0) {
            outward = true;
        }
    }

    if (!outward) {
        return false;
    }

    /*
     * The ROM refuses to stop a player while CLIMBING_THRU is nonzero.
     * Note that this check happens before velocity is cleared.
     */
    if (p->climbing_thru != 0) {
        return false;
    }

    p->xvel_fp = 0;

    was_running = p->player_mode == WM_RING_MODE_RUNNING;
    if (!was_running) {
        return true;
    }

    /*
     * Running player special case:
     *   PLYRMODE = MODE_NORMAL
     *   ANIMODE  = 0
     *   call helper 0xFF86E4B0
     */
    p->player_mode = WM_RING_MODE_NORMAL;
    p->animode = 0u;

    if (spawn_out != 0) {
        *spawn_out = make_getup_spawn(p);
    }

    return true;
}

WmRingOnscreenEvents wm_ring_keep_onscreen_tick(
    WmRingOnscreenState *state)
{
    WmRingOnscreenEvents e = no_events();

    if (state == 0 || state->p1 == 0 || state->p2 == 0) {
        /*
         * Host safety guard only. The arcade code dereferences both process
         * pointers after OLD_PSTATUS==3.
         */
        return e;
    }

    /* MOVE @OLD_PSTATUS / CMPI 3 / JRNE return */
    if (state->old_pstatus != WM_RING_KEEP_REQUIRED_OLD_PSTATUS) {
        return e;
    }

    wm_ring_keep_onscreen_bounds(
        state->worldtlx_int,
        &e.screen_center_x,
        &e.left_limit,
        &e.right_limit,
        &e.source_center_gt_ring_center_branch);

    /*
     * allow_offscrn countdown:
     *
     * if nonzero:
     *     --allow_offscrn
     *     if still nonzero: return
     *
     * Thus 1 -> 0 and confinement happens immediately.
     */
    if (state->allow_offscrn != 0u) {
        state->allow_offscrn =
            (uint16_t)(state->allow_offscrn - 1u);

        if (state->allow_offscrn != 0u) {
            return e;
        }
    }

    /*
     * ROM only invokes both per-player helpers if at least one of the first
     * two wrestlers is outside the ring.
     */
    if (state->p1->inring == 0 &&
        state->p2->inring == 0) {
        return e;
    }

    e.stopped_p1 = stop_if_moving_outward(
        state->p1,
        e.left_limit,
        e.right_limit,
        &e.p1_getup_spawn);

    e.stopped_p2 = stop_if_moving_outward(
        state->p2,
        e.left_limit,
        e.right_limit,
        &e.p2_getup_spawn);

    return e;
}
