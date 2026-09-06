#ifndef WM_MATCH_H
#define WM_MATCH_H

#include <stdbool.h>
#include "wm/arcade/wm_arcade_closest.h"
#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wm_arcade_confine.h"
#include "wm/arcade/wm_arcade_drone.h"
#include "wm/arcade/wm_arcade_lifebar.h"
#include "wm/arcade/wm_arcade_react.h"
#include "wm/arcade/wm_arcade_roster.h"
#include "wm/arcade/wm_arcade_round.h"
#include "wm/arcade/wmania_rng.h"
#include "wm/arcade/wm_arcade_wrestler_port.h"
#include "wm/bret_backend.h"
#include "wm/human_input.h"
#include "wm/wrestler_backend.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * WRESTLE.ASM::start_match: the PSTATUS==0 (attract/0-player, #0plyr) path
 * via wm_match_start_attract, and the PSTATUS!=0 single-human (#1plyr) path
 * via wm_match_start_selected. #2plyr (two humans) is not covered.
 *
 * Source flow being translated (WRESTLE.ASM:1568-1797):
 *   start_match -> (PSTATUS==0) #amode_battle -> init_life_data -> #0plyr
 *   or #1plyr wrestler-process creation loop.
 *
 * NOT yet translated, on purpose:
 *   - INIT_LADDER_TABLE / CURRENT_LADDER / NUM_OPPS multi-drone team
 *     selection: the ladder matchup table is not ported, so this only ever
 *     creates a single opponent instead of a full NUM_OPPS-sized team, for
 *     either start path.
 *   - wm_arcade_move_ported_wrestler(): the generic 8-wrestler dispatcher is
 *     not used here. Only Bret (wrestler_num==WM_ROSTER_BRET) is wired,
 *     straight to wm_arcade_move_bret() with wm/bret_backend.h's adapter --
 *     see that header for exactly which BRET.ASM animations it can resolve
 *     to real wm_visual_sequence data (idle stance + 6 light/power attacks)
 *     and which still resolve to nothing. The other seven wrestlers have no
 *     backend at all yet, so an actor drawn as one of them holds real state
 *     (health, ring assignment, mutual smart_target) but never moves, even
 *     though the drone AI driving it (see wm/arcade/wm_arcade_bret_drone.h)
 *     does press real buttons/stick for it.
 *
 * DRONE.ASM's own AI decision core and script interpreter
 * (wm_arcade_drone_main()/wm_arcade_drone_script_step(), wm/arcade/
 * wm_arcade_drone.h) are real and wired every tick; wm/arcade/
 * wm_arcade_bret_drone.h supplies the real data and DS_CODE-block callback
 * bodies they need (an earlier claim that the source's SKLM macro wasn't
 * present in the checked-in tree was wrong -- it is, and DRONE.ASM's own
 * skill tables and Bret's short/medium/long-range script lists are
 * transcribed from it). Only Bret has a real move/animation backend, so the
 * per-wrestler branches (range_script_list/drn_combo/drone_chrg) only ever
 * hand out real attack content when the drone-controlled actor is itself
 * Bret; the shared, wrestler-agnostic majority of the engine (positioning,
 * block detection, getup timing, run/roll/turnbuckle/in-air handling) runs
 * for any wrestler. wm_match_tick computes wm_arcade_calc_closest for every
 * actor every tick (not just whichever one carries a real Bret backend), so
 * a drone's own decisions read fresh distances too.
 */

#define WM_MATCH_MAX_ACTORS 2

typedef struct {
    wm_arcade_actor_t actors[WM_MATCH_MAX_ACTORS];
    wm_arcade_drone_state_t drones[WM_MATCH_MAX_ACTORS];
    /* Only actors[i].wrestler_num==WM_ROSTER_BRET drives this -- see the
       file comment. Unused (left zeroed) for every other wrestler_num. */
    wm_bret_backend_actor bret_visual[WM_MATCH_MAX_ACTORS];

    /* The shared, animation-free backend state every other wrestler runs on
       (wm/wrestler_backend.h): real execute_walk/adjust_health/mode_dead,
       no artwork. Bret ignores it in favour of bret_visual above. */
    wm_wrestler_backend_actor wrestler_visual[WM_MATCH_MAX_ACTORS];
    unsigned actor_count;

    /* @index1: source global set by ATTRACT.ASM::show_gameplay before
       CREATE-ing start_match; #0plyr reads it back for the P1 drone. */
    unsigned index1;

    /* Placeholder single opponent. Not @index2/ladder-derived -- see the
       file comment above. Drawn with the same RNDRNG0(7)-skip-7 rule as
       index1 only because no ladder table exists yet to draw it properly. */
    unsigned opponent_wrestler;

    /* wm_match_start_selected only: which actor (if any) is the #1plyr
       PTYPE_PLAYER human, and its committed-input edge-detection state.
       Always false/unused after wm_match_start_attract. */
    bool has_human;
    unsigned human_actor_index;
    wm_human_input_state human_input_state;

    bool active;
    unsigned tick_count;

    /* REACT1.ASM combat state (pcnt/any_hits/dam_mult) carried across ticks
       for wm_arcade_wrestler_hit -- see wm_match_tick's own comment for what
       the hit path around it does and does not translate. */
    wm_arcade_combat_runtime_t combat_runtime;

    /* WRESTLE2.ASM::match_timer's KO-detection state (get_live_bits + the
       5-second pin-idiot-check countdown) -- see wm/arcade/wm_arcade_round.h
       for exactly what this does and does not decide. */
    wm_arcade_round_state_t round_state;

    /* LIFEBAR.ASM::set_winner's real best-of-3 round/match tracking,
       awarded once on round_state.decided's false-to-true edge -- see
       wm_arcade_match_score_award_round's own comment. */
    wm_arcade_match_score_t score;
    /*
     * Services this match's ANI_CODE routines reach for
     * (wm/anim_program.h's wm_anim_env). Set by the caller -- the app owns
     * both the RNG and the audio queue -- and copied into each wrestler's
     * own env every tick. Left NULL, the routines that need them simply do
     * nothing, which is what the host tests run with.
     */
    WmRng *anim_rng;
    void *anim_sound_user;
    void (*anim_sound)(void *user, uint16_t call);

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

/*
 * WRESTLE.ASM::start_match's PSTATUS!=0, #1plyr path (WRESTLE.ASM:1713-1731):
 * a real PTYPE_PLAYER human at PLYRNUM 0 using p1_source_wrestler (the
 * wrestler @index1 would hold -- i.e. whatever the select screen actually
 * chose, wm_select_screen_state::selected_source_wrestler, NOT a random
 * draw), plus one placeholder opponent drone at PLYRNUM 2 (source draws a
 * full NUM_OPPS team from the ladder table here too; see the file comment).
 * p1_source_wrestler must be a real wm_arcade_roster_id_t value (0-6 or 8).
 */
void wm_match_start_selected(wm_match_state *m, WmRng *rng,
                             uint8_t p1_source_wrestler);

/* One source tick: for the human actor (if wm_match_start_selected was
 * used), commits human_input through wm_human_input_commit; every other
 * actor steps its drone decision core as usual. Then -- for whichever
 * actor(s) drew WM_ROSTER_BRET -- wm_arcade_move_bret() and its visual
 * backend, which now also sets a real per-frame hurt_box every tick (see
 * wm_bret_hurt_box_for_frame). See the file comment for what this does and
 * does not do for every other wrestler. human_input is ignored (may be
 * NULL) unless has_human is set.
 *
 * After every actor has moved, this calls the real, ctest-verified
 * wm_arcade_check_wrestler_collisions()/wm_arcade_wrestler_hit() (REACT1.ASM)
 * so a wired attack that overlaps a real hurt_box actually registers a hit
 * and calls the real, shared wm_arcade_adjust_health (LIFEBAR.ASM:1429-1670):
 * its own DAM_MULT/COMBO_COUNT damage-shaping step (a real DAM_MULT=2/4
 * first-hit/high-risk bonus from wm_arcade_wrestler_hit now genuinely
 * inflates damage instead of being silently discarded), its damage_mod_
 * table scaling (this port's fixed 1-on-1 matches always resolve to a flat
 * _85PCT) and speed_adjustment scaling (the arcade's own factory-default
 * ADJSPEED setting, a real 1.0x identity today, wired for a live value once
 * this port has an operator-settings system), the life range check (clamped
 * to [0, LIFE_MAX]), its "attract mode never dies" / "20+ pt near-death
 * fudge" rules, a genuine WM_PMODE_DEAD transition once life actually
 * reaches 0, and a real LAST_DAMAGE stamp feeding wm_arcade_wrestler_hit's
 * own rapid-hit reduced-damage window. Everything past SETMODE DEAD (death
 * animation) is NOT translated -- see wm_arcade_adjust_health's own
 * comment.
 *
 * Finally, this steps round_state (wm/arcade/wm_arcade_round.h): once one
 * side has no live wrestler left, a real 5-second countdown starts, and
 * round_state.decided/decided_winner_side become real once it elapses --
 * WRESTLE2.ASM::match_timer's actual knockout trigger, not just its round
 * clock running out. On the exact tick that happens, wm_match_tick awards
 * the real LIFEBAR.ASM::set_winner round to m->score (best-of-3
 * p1rounds/p2rounds, match_winner set once either reaches 2) -- see
 * wm_arcade_match_score_award_round's own comment for the pin-search/timer-
 * expiry parts of set_winner this doesn't reach. wm_match_tick keeps
 * ticking exactly as before even after match_winner becomes nonzero: there
 * is still no round-2 restart, announcer, or match-over transition. */
void wm_match_tick(wm_match_state *m, const wm_arcade_drone_callbacks_t *cb,
                   const wm_input_state *human_input);

#ifdef __cplusplus
}
#endif

#endif
