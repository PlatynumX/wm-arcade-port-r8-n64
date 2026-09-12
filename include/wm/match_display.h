#ifndef WM_MATCH_DISPLAY_H
#define WM_MATCH_DISPLAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/match.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * What a match tick wants drawn, as data.
 *
 * The N64 platform layer has a renderer (src/platform/n64/main.c) and
 * the portable core has a match, and until now they were two different
 * programs: main.c drew wm_demo_fighter through a wm_visual_sequence
 * track and never mentioned wm_arcade_actor_t, wm_anim_exec or
 * wm_wrestler_backend once. Its match renderer was switched off
 * deliberately -- "the existing combat renderer is a development
 * harness only" -- so none of the translated animation reached a
 * screen.
 *
 * This is the seam between them, and it is HERE rather than in the
 * platform for a plain reason: the platform cannot be compiled without
 * libdragon, so anything living there cannot be tested. Draw ORDER,
 * which layers exist, and where the torso hangs off the body are all
 * game logic with exact answers in the source. They belong somewhere a
 * host test can reach them, leaving the platform a loop over this list.
 *
 * The projection is ANIM.ASM:4632 #set_image, the per-frame routine
 * ANIM.ASM:4612 set_images runs over every wrestler. It is two lines of
 * arithmetic and they are exact, so items now carry screen coordinates
 * as well as the world position they came from. What is still NOT here
 * is the world scroll: the source subtracts WORLDTL in DISPLAY.ASM's
 * SCRTST and the DMA does the same when it plots, and this port has no
 * translated scroller, so a screen coordinate here is in world-pixel
 * space with the camera at the origin. Subtracting the camera is one
 * subtraction the platform can do once it has one.
 */

typedef enum {
    /* Drawn first, under the wrestler. */
    WM_DRAW_SHADOW = 0,
    /* The primary animation channel -- ANIMODE, change_anim1. */
    WM_DRAW_BODY,
    /* The second channel -- ANIMODE2, change_anim2. Only present when
       the body frame carries a channel-2 attachment. */
    WM_DRAW_TORSO
} wm_draw_layer;

/*
 * RING.EQU:20 Y_SCALE_MULTIPLIER equ 3566h -- the source's own comment
 * says "= 1 / 4.794 * 2^16". Depth into the ring costs this much screen
 * height per unit of Z.
 */
#define WM_Y_SCALE_MULTIPLIER 0x3566

/*
 * #set_image's `movi Y_SCALE_MULTIPLIER,a0 / move *a13(OBJ_ZPOSINT),a1 /
 * mpyu a0,a1` -- the OYVAL every piece of the wrestler is plotted from,
 * in 16.16. a1 is an ODD destination, so the multiply keeps the LOW 32
 * bits of the product rather than the high (see wm/arcade/wmania_rng.h
 * for the other case). Z alone decides it: a wrestler's screen Y comes
 * from how far back he is standing, and his own Y position arrives
 * separately as the plot offset.
 */
int32_t wm_match_screen_y_base(int32_t z_int);

/*
 * #set_image's OBJ_PRIORITY, which is what obj_yzsort reads as OZPOS:
 *
 *     priority = z_fixed | 0x10000000
 *     if outside the ring and priority <= 0x15ac0000:
 *         priority -= 0x01e50000          ; "below mat"
 *
 * INRING is zero INSIDE the ring -- the shadow block three lines later
 * labels 013c8h "inside ring" on the `jrz` path, which settles a field
 * whose name reads backwards.
 *
 * Note this is NOT wm_arcade_actor_t::obj_priority. WRESTLE2.ASM:2385
 * calc_ground_y writes 112, 103 or 117 into the low half of the same
 * LONG field, and #set_image overwrites the whole thing every display
 * frame. The small number is not the draw order.
 */
int32_t wm_match_display_priority(int32_t z_fixed, int32_t in_ring);

/*
 * The shadow's own priority, which ignores Z entirely: 013c8h inside
 * the ring and 0106ah outside. Both sit below every wrestler -- Z in
 * the ring runs 1023..1345, so the smallest in-ring wrestler priority
 * is 0x13ff -- which is why shadows come first in obj_yzsort's
 * ascending order.
 */
int32_t wm_match_shadow_priority(int32_t in_ring);

typedef struct {
    wm_draw_layer layer;
    /* The source frame name, for the platform's sprite lookup. NULL for
       a shadow, which has no artwork of its own. */
    const char *frame;
    /* Which wrestler's artwork the frame belongs to (WRESTLERNUM), so a
       streamed per-wrestler sprite bank can be selected. */
    int32_t wrestler_num;
    /* The actor's world position, straight off wm_arcade_actor_t. */
    int32_t world_x, world_y, world_z;
    /*
     * Where to hang the image relative to that position. For a body
     * this is the frame's own animation origin; for a torso it is the
     * composite offset wm_secondary_display_offsets computes from both
     * frames' origins and the body's channel-2 attachment pair.
     */
    int16_t anchor_x, anchor_y;
    /*
     * Where the piece lands, in world pixels with the camera at the
     * origin: OXVAL/OYVAL's integer parts plus ODXOFF/ODYOFF, which is
     * exactly what #plot_object hands the DMA.
     *
     *     screen_x = x_int                    + anchor_x
     *     screen_y = z_int/4.794 + y_or_ground + anchor_y
     *
     * The Y term is the actor's own Y for a body or torso and his
     * GROUND_Y for a shadow, which is how the shadow stays on the mat
     * while he is in the air.
     */
    int32_t screen_x, screen_y;
    /* OZVAL -- obj_yzsort's primary key. See wm_match_display_priority. */
    int32_t priority;
    /* OBJ_CONTROL's B_FLIPH -- the wrestler is facing left. */
    bool flip_x;
} wm_match_draw_item;

/* Three layers per actor is the most this can emit. */
#define WM_MATCH_DRAW_MAX (WM_MATCH_MAX_ACTORS * 3)

/*
 * Fill `out` with everything to draw this tick, already in draw order,
 * and return how many items were written.
 *
 * Order is DISPLAY.ASM:1248 obj_yzsort's: ascending OZPOS, and
 * ascending OYPOS within an equal OZPOS. Those are the values
 * #set_image writes, not the actor's raw world position -- OZPOS is the
 * priority above and OYPOS is z_int/4.794. Both are monotone in Z, so
 * the tiebreak can never separate two wrestlers at the same Z, and the
 * source's strict swap test leaves them in list order. An earlier
 * version of this compared world Y as the tiebreak, which is a key the
 * source never uses.
 *
 * An actor's own three layers stay together and in layer order, so the
 * shadow is under the body and the torso over it. The source instead
 * carries the shadow as its own object with its own priority, and those
 * priorities put every shadow ahead of every wrestler; grouping them is
 * this list's simplification, and it draws the same picture whenever no
 * wrestler passes between another wrestler and that man's shadow.
 *
 * MODE_INVISIBLE drops the body and torso (#set_image jumps to #done2)
 * and MODE_NOSHADOW drops the shadow -- the source parks the shadow's X
 * at 0 rather than skipping it, which hides it the same way.
 */
size_t wm_match_build_draw_list(const wm_match_state *match,
                                wm_match_draw_item *out, size_t max);

#ifdef __cplusplus
}
#endif
#endif
