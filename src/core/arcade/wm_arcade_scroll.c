/*
 * WRESTLE2.ASM:1681 scroll_world and WRESTLE.ASM:2238
 * update_positions. See wm/arcade/wm_arcade_scroll.h.
 */
#include "wm/arcade/wm_arcade_scroll.h"
#include "wm/match_display.h"
#include "wm/anim.h"
#include "wm/arcade/wmania_ring_geometry.h"

#include <stddef.h>

void wm_scroll_update_positions(const wm_arcade_actor_t *actor,
                                wm_scroll_point *out)
{
    int32_t y;

    if (!actor || !out) return;

    out->x = actor->x_fixed;
    out->z = actor->z_fixed;

    /*
     * `move *a13(OBJ_YPOS),a14,L` is commented out; what runs is
     * GROUND_Y shifted up sixteen, and `jrnn #ok` floors it at zero.
     * The camera follows the mat, not the man.
     */
    y = actor->ground_y << 16;
    if (y < 0) y = 0;
    out->y = y;
}

bool wm_scroll_pick_pair(const wm_arcade_actor_t *const *actors,
                         size_t count, bool humans_present,
                         size_t *out_a, size_t *out_b)
{
    size_t i;

    if (!actors || !out_a || !out_b) return false;

    if (humans_present) {
        /*
         * "two humans. track on them" / "plyr v. one drone" -- with a
         * fixed pair both land on the same answer, the two actives.
         */
        size_t found[2];
        size_t n = 0;
        for (i = 0; i < count && n < 2; ++i)
            if (actors[i] && actors[i]->active) found[n++] = i;
        if (n == 2) { *out_a = found[0]; *out_b = found[1]; return true; }
        if (n == 1) { *out_a = found[0]; *out_b = found[0]; return true; }
        return false;
    }

    /*
     * `#no_humans` -- "attract mode play. track on first live drone and
     * his closest opponent." Inactive and dead slots are skipped, and
     * the source notes the dead case "shouldn't happen".
     */
    for (i = 0; i < count; ++i) {
        const wm_arcade_actor_t *a = actors[i];
        size_t j;
        if (!a || !a->active) continue;
        if (a->player_mode == (uint16_t)WM_PMODE_DEAD) continue;
        *out_a = i;
        *out_b = i;
        /* CLOSEST_NUM. The port holds the opponent as a pointer. */
        for (j = 0; j < count; ++j)
            if (actors[j] == a->smart_target) { *out_b = j; break; }
        return true;
    }
    return false;
}

/* `mpys a0,a1` into the ODD register a1: the low 32 bits survive. */
static int32_t depth_to_screen(int32_t z_int)
{
    return (int32_t)((uint32_t)z_int * (uint32_t)WM_Y_SCALE_MULTIPLIER);
}

static void scroll_x(wm_scroll_state *s,
                     const wm_scroll_point *p1, const wm_scroll_point *p2)
{
    int32_t want = ((p1->x + p2->x) >> 1) - (WM_SCROLL_HALF_W << 16);
    int32_t at = s->worldtlx;
    int32_t delta = want - at;

    /*
     * `jrp #pos` then the two BUFFER arms. Positive: take the buffer
     * off and give up if that made it negative. Negative (or zero):
     * add the buffer on and give up if that made it positive. A delta
     * inside the buffer either way leaves the camera alone.
     */
    if (delta > 0) {
        delta -= WM_SCROLL_BUFFER;
        if (delta < 0) return;
    } else {
        delta += WM_SCROLL_BUFFER;
        if (delta > 0) return;
    }

    at += delta >> 3;                /* `sra 3,a1 / add a1,a2` */

    /*
     * `cmpi [12fh,0],a2 / jrlt #wide` and `cmpi [648h,0],a2 / jrgt
     * #wide` -- out of range means DON'T WRITE, so the camera holds
     * its last value rather than sticking to the limit.
     *
     * The file declares LIMITXL/LIMITXR/LIMITYT/LIMITYB above the
     * routine and the code uses none of them: LIMITXL happens to
     * equal the literal here, LIMITXR is 5E8h against the 648h
     * actually written, and LIMITYT is unused entirely.
     */
    if (at < WM_SCROLL_X_MIN) return;
    if (at > WM_SCROLL_X_MAX) return;
    s->worldtlx = at;
}

static void scroll_y(wm_scroll_state *s,
                     const wm_scroll_point *p1, const wm_scroll_point *p2,
                     const wm_arcade_actor_t *const *actors, size_t count)
{
    /* `sra 1+16,a1` -- the mean Z, as an integer, in one shift. */
    int32_t mean_z = (p1->z + p2->z) >> (1 + 16);
    int32_t want = depth_to_screen(mean_z);
    int32_t mean_y = (p1->y + p2->y) >> 1;
    int32_t at;
    size_t i;

    want -= mean_y;
    want -= (WM_SCROLL_HALF_H << 16);

    /*
     * "Check for SCROLL_CTRL bits on active wrestlers." An actor that
     * has one, and is inside the horizontal window, offers its own
     * high point; the SMALLEST wins, so whoever is highest on screen
     * drags the camera up.
     */
    for (i = 0; i < count; ++i) {
        const wm_arcade_actor_t *a = actors ? actors[i] : NULL;
        int32_t hi;
        if (!a || !a->active) continue;
        if (!(a->status_flags & WM_STATUS_SCROLL_CTRL)) continue;

        /* `subi [60,0]` below and `addi [400+120,0]` above. */
        if (a->x_fixed < s->worldtlx - (60 << 16)) continue;
        if (a->x_fixed > s->worldtlx - (60 << 16) + ((400 + 120) << 16))
            continue;

        hi = depth_to_screen(a->z_int) - a->y_fixed - (a->scroll_y << 16);
        if (hi < want) want = hi;    /* `cmp a2,a1 / jrge #nxt4` */
    }

    at = s->worldtly;
    at += (want - at) >> 2;          /* a quarter of the error */

    /* `cmpi [97h,0],a1 / jrgt #low` -- past the fence, don't write. */
    if (at > WM_SCROLL_Y_MAX) return;
    s->worldtly = at;
}

void wm_scroll_world(wm_scroll_state *s,
                     const wm_scroll_point *p1, const wm_scroll_point *p2,
                     const wm_arcade_actor_t *const *actors, size_t count)
{
    if (!s || !p1 || !p2) return;
    scroll_x(s, p1, p2);
    scroll_y(s, p1, p2, actors, count);
}

/*
 * WRESTLE2.ASM:2270 #do_check -- one wrestler against the window. See
 * the header for the four gates the caller applies first.
 */
wm_keep_onscreen_result wm_keep_onscreen_one(wm_arcade_actor_t *actor,
                                             int32_t left, int32_t right)
{
    wm_keep_onscreen_result r;

    r.stopped = false;
    r.dropped_run = false;
    if (!actor) return r;

    /*
     * `cmp a0,a14 / jrgt #ok1` -- inside the left edge, go and check
     * the right one. Past it, only a velocity still carrying him
     * further left counts; zero or rightward is left alone.
     */
    if (actor->x_int <= left) {
        if (actor->x_vel == 0) return r;
        if (actor->x_vel >= 0) return r;       /* heading back in */
    } else if (actor->x_int >= right) {
        if (actor->x_vel == 0) return r;
        if (actor->x_vel < 0) return r;        /* heading back in */
    } else {
        return r;                              /* inside the window */
    }

    /* A man climbing through the ropes is exempt. */
    if (actor->climbing_thru) return r;

    actor->x_vel = 0;
    r.stopped = true;

    /*
     * A run that is stopped is over: PLYRMODE and ANIMODE both go to
     * NORMAL, and the getup meter goes with them. Note the source
     * writes MODE_NORMAL into ANIMODE as a whole word, clearing every
     * other animation-mode bit with it.
     */
    if (actor->player_mode == (uint16_t)WM_PMODE_RUNNING) {
        actor->player_mode = (uint16_t)WM_PMODE_NORMAL;
        actor->anim_mode = (uint16_t)WM_MODE_NORMAL;
        r.dropped_run = true;
    }
    return r;
}

/* WRESTLE2.ASM:2180 keep_onscreen -- the gate and the two checks. */
void wm_keep_onscreen(wm_arcade_actor_t *p1, wm_arcade_actor_t *p2,
                      int32_t worldtlx, bool two_player,
                      int32_t *allow_offscrn)
{
    int32_t centre, left, right;

    if (!two_player || !p1 || !p2) return;     /* `jrne #no_2_player` */

    centre = (worldtlx >> 16) + 200;           /* WORLDTLX+16, then +200 */

    /*
     * Which buffer goes on which side flips at the ring's centre.
     * Both are 185 in the shipped build -- #BUFF2 carries `;140` as
     * the value it used to be -- so this currently makes no
     * difference, and the code that would make it one is here.
     */
    if (centre > WM_RING_X_CENTER) {
        left = centre - WM_KEEP_ONSCREEN_BUFF1;
        right = centre + WM_KEEP_ONSCREEN_BUFF2;
    } else {
        left = centre - WM_KEEP_ONSCREEN_BUFF2;
        right = centre + WM_KEEP_ONSCREEN_BUFF1;
    }

    /*
     * `allow_offscrn` is a countdown. Nonzero means skip the check,
     * and it is decremented on the way past -- so ANI_SET_IDIOT buys
     * exactly that many ticks of freedom, not a latch.
     */
    if (allow_offscrn && *allow_offscrn != 0) {
        --*allow_offscrn;
        if (*allow_offscrn != 0) return;
    }

    /* At least one of them has to be out of the ring. INRING is zero
       INSIDE, so `jrnz #outside` fires on the one who is out. */
    if (p1->in_ring == 0 && p2->in_ring == 0) return;

    (void)wm_keep_onscreen_one(p1, left, right);
    (void)wm_keep_onscreen_one(p2, left, right);
}
