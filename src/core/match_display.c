/*
 * The seam between the translated match and whatever draws it.
 * See wm/match_display.h for why this lives in the portable core.
 */
#include "wm/match_display.h"

#include "wm/anim_program.h"
#include "wm/bret_backend.h"
#include "wm/composite.h"
#include "wm/frame_geometry.h"
#include "wm/arcade/wm_arcade_roster.h"
#include "wm/visual.h"
#include "wm/wrestler_backend.h"
#include "wm/arcade/wm_arcade_combat_defs.h"

#include <stddef.h>
#include <stdint.h>

/* The frame each channel is showing, whichever backend the actor runs on.
   Bret has a hand-built wm_visual_sequence track; the other seven are
   program-driven and answer through the animation VM. */
static void frames_for(const wm_match_state *m, size_t i,
                       const char **body, const char **torso) {
    *body = NULL;
    *torso = NULL;
    if (m->actors[i].wrestler_num == (int32_t)WM_ROSTER_BRET) {
        const wm_visual_frame *f = wm_visual_current(&m->bret_visual[i].visual);
        const wm_visual_frame *t =
            wm_visual_current(&m->bret_visual[i].torso_visual);
        if (f) *body = f->source_frame;
        if (t) *torso = t->source_frame;
        return;
    }
    *body = wm_anim_exec_frame(&m->wrestler_visual[i].prog);
    *torso = wm_wrestler_backend_torso_frame(&m->wrestler_visual[i]);
}

/*
 * ANIM.ASM:4645 -- `move *a13(OBJ_ZPOS),a0,L / ori [01000h,0],a0`, then
 * the out-of-ring drop. INRING is zero inside the ring.
 */
int32_t wm_match_display_priority(int32_t z_fixed, int32_t in_ring) {
    int32_t p = (int32_t)((uint32_t)z_fixed | 0x10000000u);
    if (in_ring != 0 && p <= 0x15ac0000) p -= 0x01e50000;
    return p;
}

/* ANIM.ASM:4694 -- the shadow's own, with no Z in it at all. */
int32_t wm_match_shadow_priority(int32_t in_ring) {
    return in_ring == 0 ? 0x13c80000 : 0x106a0000;
}

/*
 * ANIM.ASM:4651 -- `mpyu a0,a1` into the ODD register a1, so the low 32
 * bits of the product are what survives.
 */
int32_t wm_match_screen_y_base(int32_t z_int) {
    return (int32_t)((uint32_t)z_int * (uint32_t)WM_Y_SCALE_MULTIPLIER);
}

/*
 * #plot_object (ANIM.ASM:4924): OXVAL and OYVAL are the 16.16 bases and
 * ODXOFF/ODYOFF the frame's own IANIOFFX/IANIOFFY plus the caller's a10
 * and a11. `y_base_off` is a11 -- GROUND_Y for the shadow, OBJ_YPOSINT
 * for the body and torso.
 */
static void fill(wm_match_draw_item *it, wm_draw_layer layer,
                 const wm_arcade_actor_t *a, const char *frame,
                 int16_t ax, int16_t ay, int32_t y_base_off,
                 int32_t priority) {
    it->layer = layer;
    it->frame = frame;
    it->wrestler_num = a->wrestler_num;
    it->world_x = a->x_int;
    it->world_y = a->y_int;
    it->world_z = a->z_int;
    it->anchor_x = ax;
    it->anchor_y = ay;
    it->screen_x = a->x_int + ax;
    it->screen_y = (wm_match_screen_y_base(a->z_int) >> 16) + y_base_off + ay;
    it->priority = priority;
    it->flip_x = (a->obj_control & WM_OBJ_FLIPH) != 0;
}

/*
 * DISPLAY.ASM:1248 obj_yzsort's comparison: OZPOS first, then OYPOS
 * within an equal OZPOS, both ascending. Those are the integer halves
 * of what #set_image wrote -- the display priority and z_int/4.794 --
 * not the actor's raw world position, and the tiebreak is therefore
 * degenerate for two wrestlers standing at the same Z.
 */
static int behind(const wm_arcade_actor_t *a, const wm_arcade_actor_t *b) {
    int32_t pa = wm_match_display_priority(a->z_fixed, a->in_ring) >> 16;
    int32_t pb = wm_match_display_priority(b->z_fixed, b->in_ring) >> 16;
    if (pa != pb) return pa < pb;
    return (wm_match_screen_y_base(a->z_int) >> 16) <
           (wm_match_screen_y_base(b->z_int) >> 16);
}

size_t wm_match_build_draw_list(const wm_match_state *match,
                                wm_match_draw_item *out, size_t max) {
    size_t order[WM_MATCH_MAX_ACTORS];
    size_t n = 0, i, j, count = 0;

    if (!match || !out || max == 0) return 0;

    for (i = 0; i < match->actor_count && i < WM_MATCH_MAX_ACTORS; ++i) {
        if (!match->actors[i].active) continue;
        order[n++] = i;
    }

    /* Insertion sort into obj_yzsort's order. Two actors today, and the
       source's own sort is a bubble sort over the object list, so the
       algorithm is not what matters -- the comparison is. */
    for (i = 1; i < n; ++i) {
        size_t key = order[i];
        j = i;
        while (j > 0 && behind(&match->actors[key],
                               &match->actors[order[j - 1]])) {
            order[j] = order[j - 1];
            --j;
        }
        order[j] = key;
    }

    for (i = 0; i < n; ++i) {
        size_t idx = order[i];
        const wm_arcade_actor_t *a = &match->actors[idx];
        const wm_frame_geometry_t *bg;
        const char *body, *torso;

        frames_for(match, idx, &body, &torso);

        /* The shadow goes down first whether or not the wrestler has a
           frame this tick -- he is standing there either way. It hangs
           off GROUND_Y, so it stays on the mat while he is airborne. */
        if (!(a->anim_mode & WM_MODE_NOSHADOW)) {
            if (count >= max) return count;
            fill(&out[count++], WM_DRAW_SHADOW, a, NULL, 0, 0,
                 a->ground_y, wm_match_shadow_priority(a->in_ring));
        }

        /* MODE_INVISIBLE: #set_image jumps straight to #done2. */
        if (a->anim_mode & WM_MODE_INVISIBLE) continue;
        if (!body) continue;
        bg = wm_frame_geometry_find(body);
        if (!bg) continue;      /* no geometry, nothing to anchor by */

        if (count >= max) return count;
        fill(&out[count++], WM_DRAW_BODY, a, body, bg->xani, bg->yani,
             a->y_int, wm_match_display_priority(a->z_fixed, a->in_ring));

        /*
         * The second channel hangs off the body frame's channel-2
         * attachment pair, and only 748 of the roster's 5,126 frames
         * carry one: (-1, -1) means this pose has nowhere to hang a
         * torso, and the source draws none.
         */
        if (!torso) continue;
        if (bg->attach_x == -1 && bg->attach_y == -1) continue;
        {
            const wm_frame_geometry_t *tg = wm_frame_geometry_find(torso);
            int xoff = 0, yoff = 0;
            if (!tg) continue;
            wm_secondary_display_offsets(bg->xani, bg->yani,
                                         bg->attach_x, bg->attach_y,
                                         tg->xani, tg->yani, &xoff, &yoff);
            if (count >= max) return count;
            fill(&out[count++], WM_DRAW_TORSO, a, torso,
                 (int16_t)xoff, (int16_t)yoff, a->y_int,
                 wm_match_display_priority(a->z_fixed, a->in_ring));
        }
    }
    return count;
}
