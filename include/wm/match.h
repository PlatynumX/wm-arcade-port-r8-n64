#ifndef WM_MATCH_H
#define WM_MATCH_H

#include <stdbool.h>
#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wm_arcade_drone.h"
#include "wm/arcade/wm_arcade_roster.h"
#include "wm/arcade/wmania_rng.h"
#include "wm/bret_backend.h"

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
 *   - wm_arcade_move_ported_wrestler(): the generic 8-wrestler dispatcher is
 *     not used here. Only Bret (wrestler_num==WM_ROSTER_BRET) is wired,
 *     straight to wm_arcade_move_bret() with wm/bret_backend.h's adapter --
 *     see that header for exactly which BRET.ASM animations it can resolve
 *     to real wm_visual_sequence data (idle stance + 6 light/power attacks)
 *     and which still resolve to nothing. The other seven wrestlers have no
 *     backend at all yet, so an actor drawn as one of them holds real state
 *     (health, ring assignment, mutual smart_target) but never moves.
 *   - Movement input: nothing above ever gives a Bret actor a nonzero
 *     stick_val_cur (see the DRONE.ASM note), so wm_arcade_bret_callbacks_t.
 *     execute_walk (real, see wm/movement.h and wm/bret_backend.h) has
 *     nothing to act on today; it takes real human/player input, not more
 *     drone data.
 */

#define WM_MATCH_MAX_ACTORS 2

typedef struct {
    wm_arcade_actor_t actors[WM_MATCH_MAX_ACTORS];
    wm_arcade_drone_state_t drones[WM_MATCH_MAX_ACTORS];
    /* Only actors[i].wrestler_num==WM_ROSTER_BRET drives this -- see the
       file comment. Unused (left zeroed) for every other wrestler_num. */
    wm_bret_backend_actor bret_visual[WM_MATCH_MAX_ACTORS];
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

/* One source tick: steps each active drone's decision core, then -- for
 * whichever actor(s) drew WM_ROSTER_BRET -- wm_arcade_move_bret() and its
 * visual backend. See the "NOT yet translated" note above for what this
 * does and does not do for every other wrestler. */
void wm_match_tick(wm_match_state *m, const wm_arcade_drone_callbacks_t *cb);

#ifdef __cplusplus
}
#endif

#endif
