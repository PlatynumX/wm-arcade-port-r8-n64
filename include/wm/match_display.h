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
 * What this does NOT do is project world coordinates onto the screen.
 * The source scrolls a world through WORLDTLX/WORLDTLY and the port has
 * no translated projection yet, so inventing one here would be a guess
 * dressed as a translation. Items carry the actor's own world position;
 * placing it is the platform's job until obj_addworldxy and friends are
 * translated.
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
    /* OBJ_CONTROL's B_FLIPH -- the wrestler is facing left. */
    bool flip_x;
} wm_match_draw_item;

/* Three layers per actor is the most this can emit. */
#define WM_MATCH_DRAW_MAX (WM_MATCH_MAX_ACTORS * 3)

/*
 * Fill `out` with everything to draw this tick, already in draw order,
 * and return how many items were written.
 *
 * Order is DISPLAY.ASM:1248 obj_yzsort's: ascending Z, and ascending Y
 * within equal Z. The source sorts the whole object list that way every
 * frame so a wrestler further back is drawn first and the one in front
 * overlaps him. The demo renderer this replaces compared screen Y
 * alone, which is neither of those keys.
 *
 * An actor's own three layers stay together and in layer order, so the
 * shadow is under the body and the torso over it.
 */
size_t wm_match_build_draw_list(const wm_match_state *match,
                                wm_match_draw_item *out, size_t max);

#ifdef __cplusplus
}
#endif
#endif
