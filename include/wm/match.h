#ifndef WM_MATCH_H
#define WM_MATCH_H

#include <stdbool.h>
#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wm_arcade_drone.h"
#include "wm/arcade/wmania_rng.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * WRESTLE.ASM::start_match, PSTATUS==0 (attract/0-player) path only.
 *
 * Source flow being translated (WRESTLE.ASM:1568-1797):
 *   start_match -> (PSTATUS==0) #amode_battle -> init_life_data -> #0plyr
 *   wrestler-process creation loop.
 *
 * NOT yet translated, on purpose:
 *   - INIT_LADDER_TABLE / CURRENT_LADDER / NUM_OPPS multi-drone team
 *     selection: the ladder matchup table is not ported, so this only ever
 *     creates a single opponent instead of a full NUM_OPPS-sized team.
 *   - The wrestler_main process body itself: DRONE.ASM's script/range data
 *     (wnshort_t/wnmed_t/wnlong_t mode lists, named scripts, and the
 *     blkbase_t/blkatk_t/sklhhdly_t/sklhrdly_t skill tables) is emitted by a
 *     SKLM macro that is not present in the checked-in historicalsource
 *     tree, so it cannot be materialized without guessing its expansion.
 *     wm_arcade_drone_main() is still called every tick with the callbacks
 *     that ARE real (rndrng0_upto) and safely no-ops (WM_DRONE_STEP_IDLE)
 *     for the rest -- it does not invent AI behavior.
 *   - wm_arcade_move_ported_wrestler(): the per-character control/animation
 *     layer needs a full wm_arcade_wrestler_port_bindings_t per wrestler,
 *     which itself needs a real anim/sound backend. Only Bret has one today
 *     (see README). Actors therefore hold real state (health, ring
 *     assignment, mutual smart_target) but do not yet move or animate.
 */

#define WM_MATCH_MAX_ACTORS 2

typedef struct {
    wm_arcade_actor_t actors[WM_MATCH_MAX_ACTORS];
    wm_arcade_drone_state_t drones[WM_MATCH_MAX_ACTORS];
    unsigned actor_count;

    /* @index1: source global set by ATTRACT.ASM::show_gameplay before
       CREATE-ing start_match; #0plyr reads it back for the P1 drone. */
    unsigned index1;

    /* Placeholder single opponent. Not @index2/ladder-derived -- see the
       file comment above. Drawn with the same RNDRNG0(7)-skip-7 rule as
       index1 only because no ladder table exists yet to draw it properly. */
    unsigned opponent_wrestler;

    bool active;
    unsigned tick_count;
} wm_match_state;

void wm_match_init(wm_match_state *m);

/* ATTRACT.ASM::show_gameplay's own draw:
 *   movk 7,a0 / calla RNDRNG0 / cmpi 7,a0 / jrnz #bug / inc a0 / #bug
 * i.e. RNDRNG0(7) (0..7 inclusive), and if the draw lands exactly on 7 it is
 * bumped to 8. wm_arcade_roster_id_t skips 7 for exactly this reason. */
unsigned wm_match_draw_wrestler_index(WmRng *rng);

/* Runs the #0plyr creation sequence: draws index1, inits both actors' life
 * to LIFE_MAX (LIFEBAR.ASM:135, =163) via LIFEBAR.ASM::init_life_data,
 * assigns PLYRNUM 2/3 and PSIDE_PLYR1/PSIDE_PLYR2 exactly as WRESTLE.ASM's
 * #0plyr loop does, and points each actor's smart_target at the other. */
void wm_match_start_attract(wm_match_state *m, WmRng *rng);

/* One source tick: steps each active drone's decision core. See the
 * "NOT yet translated" note above for what this does and does not do. */
void wm_match_tick(wm_match_state *m, const wm_arcade_drone_callbacks_t *cb);

#ifdef __cplusplus
}
#endif

#endif
