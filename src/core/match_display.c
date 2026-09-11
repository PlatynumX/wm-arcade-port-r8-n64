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

static void fill(wm_match_draw_item *it, wm_draw_layer layer,
                 const wm_arcade_actor_t *a, const char *frame,
                 int16_t ax, int16_t ay) {
    it->layer = layer;
    it->frame = frame;
    it->wrestler_num = a->wrestler_num;
    it->world_x = a->x_int;
    it->world_y = a->y_int;
    it->world_z = a->z_int;
    it->anchor_x = ax;
    it->anchor_y = ay;
    it->flip_x = (a->obj_control & WM_OBJ_FLIPH) != 0;
}

/*
 * DISPLAY.ASM:1248 obj_yzsort's comparison: Z first, then Y within an
 * equal Z, both ascending. The source bubble-sorts the whole object
 * list into that order every frame.
 */
static int behind(const wm_arcade_actor_t *a, const wm_arcade_actor_t *b) {
    if (a->z_int != b->z_int) return a->z_int < b->z_int;
    return a->y_int < b->y_int;
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
           frame this tick -- he is standing there either way. */
        if (count >= max) return count;
        fill(&out[count++], WM_DRAW_SHADOW, a, NULL, 0, 0);

        if (!body) continue;
        bg = wm_frame_geometry_find(body);
        if (!bg) continue;      /* no geometry, nothing to anchor by */

        if (count >= max) return count;
        fill(&out[count++], WM_DRAW_BODY, a, body, bg->xani, bg->yani);

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
                 (int16_t)xoff, (int16_t)yoff);
        }
    }
    return count;
}
