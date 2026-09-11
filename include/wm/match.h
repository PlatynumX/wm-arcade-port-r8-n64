#ifndef WM_MATCH_H
#define WM_MATCH_H

#include <stdbool.h>
#include "wm/arcade/wm_arcade_closest.h"
#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wm_arcade_coffin.h"
#include "wm/arcade/wm_arcade_pin.h"
#include "wm/award.h"
#include "wm/arcade/wm_arcade_match_end.h"
#include "wm/arcade/wm_arcade_round_reset.h"
#include "wm/arcade/wm_arcade_scroll.h"
#include "wm/arcade/wm_arcade_smove.h"
#include "wm/arcade/wm_arcade_confine.h"
#include "wm/arcade/wm_arcade_drone.h"
#include "wm/arcade/wm_arcade_lifebar.h"
#include "wm/arcade/wm_arcade_react.h"
#include "wm/arcade/wm_arcade_roster.h"
#include "wm/arcade/wm_arcade_match_clock.h"
#include "wm/arcade/wm_arcade_round.h"
#include "wm/arcade/wm_arcade_round_announce.h"
#include "wm/arcade/wm_arcade_target.h"
#include "wm/arcade/wm_arcade_shake.h"
#include "wm/arcade/wm_arcade_debris.h"
#include "wm/arcade/wmania_rng.h"
#include "wm/arcade/wmania_rope_runtime.h"
#include "wm/arcade/wm_arcade_announcer.h"
#include "wm/arcade/wm_arcade_wrestler_port.h"
#include "wm/bret_backend.h"
#include "wm/human_input.h"
#include "wm/wrestler_backend.h"

/* ROPES.ASM keeps one process per bank: front, back, left, right. */
#define WM_MATCH_ROPE_BANKS 4

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
    /*
     * WRESTLE2.ASM:4098 match_timer's @match_time, the round clock --
     * see wm/arcade/wm_arcade_match_clock.h. Reset to 99 by
     * match_reset_for_round and ticked once a source tick, and when
     * it reaches zero the round is decided by LIFEBAR.ASM's #tmout
     * rule instead of by a knockout.
     */
    wm_match_clock_t clock;
    /*
     * #dec_timer's `movk 10,a0 / calla triple_sound`, true for the
     * ONE tick it fires -- once a second from ten seconds down. It is
     * a flag rather than a call because nothing in this port plays a
     * sound yet; the crowd channel beside it is the crowd's, and
     * putting the clock's warning on it would be a different sound in
     * a different place.
     */
    bool clock_warning;
    /*
     * ROPES.ASM's four rope banks. The whole subsystem -- rope_command's
     * table routing, the runtime's new_command_wake, and the complete
     * static script corpus -- has been translated for a long time and
     * nothing outside those files ever called it. The animation VM's
     * ANI_BOUNCEROPE, ANI_BENDROPE and ANI_ROPE_Z are what drive it in the
     * original, so they drive it here.
     */
    WmRopeRuntimeBank ropes[WM_MATCH_ROPE_BANKS];
    /* DCSSOUND.ASM's announcer voice queue, drained one line a tick the
       way ANNOUNCE_VOICE drains it. */
    wm_announcer_state announcer;
    /* LIFEBAR.ASM:2642 announce_rnd_winner: the round-ending process a
       pin starts, ticked below round_state's own KO countdown because a
       pin ends the round instead of waiting for it. */
    wm_arcade_round_announce_t round_announce;
    /* set_rope_z's second-half Z per bank (ANIM.ASM:41's RZ_HIGH/RZ_NORM). */
    uint16_t rope_second_half_z[WM_MATCH_ROPE_BANKS];

    /*
     * What CROWD.ASM's crowd_cheer is told, for as long as there is no
     * CROWD.ASM port to tell. The flags and percentages are the real
     * extracted ones -- DO_CROWD_CHEER's C_OVERIDE|C_LONG, and whatever
     * row DO_CROWD_ANYWAY drew out of the announcer table's crowd
     * `.LONG` -- so the subsystem that eventually reads them gets the
     * source's own arguments rather than a reconstruction.
     *
     * `sound_ticks` doubles as the source's crowd_dummy_exists: CROWD_DUMMY
     * is a process that sleeps for exactly the drawn row's duration, and
     * while it lives DO_CROWD_ANYWAY skips the SNDSND. Counted down by
     * wm_match_tick.
     */
    /*
     * WRESTLE.ASM:255-256 no_debris / reduce_bog, and what the debris
     * routines were told. The particle processes themselves belong to a
     * SPECIAL.ASM port; these are the decisions in front of them, which
     * are real: `no_debris` is set and cleared by the routines that
     * suppress each other, and reduce_bog is init_reduce_bog's own
     * "active wrestlers minus two" (WRESTLE.ASM:4552), so a 1-on-1 match
     * has it at 0 and debris on.
     */
    /* UTIL.ASM:2406 SHAKER2's oscillator and the WORLDTLY it moves. */
    wm_shake_state shake;
    /*
     * WRESTLE.ASM:257 allow_offscrn -- ANI_SET_IDIOT's 80 ticks of "let
     * them off screen on toss outs". WRESTLE2.ASM:2214 counts it down once
     * a tick and skips the ring-out check entirely while it is set.
     */
    int32_t allow_offscrn;
    /* What SPECIAL.ASM's create_dizzy_proc was asked for, for a star
       sprite a renderer would make. */
    struct {
        int32_t xoff, yoff;
        uint32_t created;
    } dizzy;

    /* LIFEBAR.ASM's MOVE_NAME_ANNC state: which names have been shown
       and whether one is up on either side. */
    wm_move_name_state move_names;
    /* What the last move-name draw asked for, for a renderer to read. */
    struct {
        int side;
        int index;
        const char *image;
        uint32_t drawn;
    } move_name;

    /* SPECIAL.ASM:3415 react_debris's global @debris_count, and what
       the last burst was told to be. */
    wm_debris_state debris_runtime;
    wm_debris_burst last_burst;

    struct {
        bool no_debris;
        int32_t reduce_bog;
        const char *last_effect;
        int last_count;
        int32_t last_yoff;
        uint32_t created;      /* how many DEBRIS_PID processes were asked */
    } debris;

    struct {
        int flags;            /* WM_CROWD_* */
        int percent;          /* per mille, 0 unless WM_CROWD_RANDOM */
        int sound;            /* SOUND.EQU CROWD_* id, 0 if none yet */
        uint16_t sound_ticks; /* CROWD_DUMMY's remaining life */
        uint32_t cheers;      /* how many times crowd_cheer was reached */
        uint32_t sounds;      /* how many times SNDSND was */
    } crowd;

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
    /*
     * AWARD.ASM round_award, through JJXM.H's RND_AWARD macro. The award
     * arrays are per credit rather than per match, so the app owns them;
     * the match hands the seam to the animation VM (DO_COMBO_MESS) and to
     * the hit path (ANIM.ASM:2253's first hit of the round), both of
     * which are where the source itself calls RND_AWARD.
     */
    void *anim_award_user;
    void (*anim_round_award)(void *user, int player_num, unsigned award_index);
    /*
     * FINISEQ.ASM's @close_the_door / @close_the_floor / @guy_in --
     * the Undertaker's coffin sequence. Globals in the source because
     * only one finish can run at a time; the match owns them here and
     * lends the struct to every wrestler's env, so the ANI_CODE
     * routines in und_2_raise_dead_anim and push_in_anim can talk to
     * each other the way the source's globals let them.
     * wm_match_init clears it, as und_coffin_up's own first two
     * instructions do.
     */
    wm_coffin_state_t coffin;
    /*
     * WRESTLE2.ASM:4058 init_smoves' watchdogs, one set per wrestler.
     * The source gives each its own SMOVE_PID process, made once per
     * MATCH; these are the same state machines ticked from here.
     *
     * `smove_unported` is how many of that wrestler's table entries
     * this port has no monitor for -- carried rather than discarded so
     * the gap is a number somebody can read, not a silence.
     */
    wm_smove_run_t smoves[WM_MATCH_MAX_ACTORS][WM_SMOVE_MAX_PER_WRESTLER];
    size_t smove_count[WM_MATCH_MAX_ACTORS];
    size_t smove_unported[WM_MATCH_MAX_ACTORS];

    /* @p1pins / @p2pins (AWARD.ASM:208), cleared at match start by
       WRESTLE.ASM:1578 and bumped by pin_prompt. und_finish_move1
       counts them. */
    wm_arcade_pins_t pins;

    /*
     * @in_finish_move. TAKER.ASM:676 raises it before the coffin
     * animation and :703 clears it once @finish_completed lands. Two
     * things read it: scroll_world stops scrolling while it is set,
     * and announce_rnd_winner holds off calling the round.
     */
    bool in_finish_move;

    /* WRESTLE2.ASM:1681 scroll_world's @WORLDTLX / @WORLDTLY. */
    wm_scroll_state scroll;

    /*
     * The two processes FINISEQ.ASM's coffin finish creates through
     * ANI_CREATEPROC -- und_coffin_up (with do_up_coffin and
     * hover_coffin inside it) and raise_dead. The VM has had a
     * create_proc seam since it was written and nothing supplied one,
     * so neither ran: @finish_completed was never set, TAKER.ASM's
     * `#fdone_wait` never ended, and @in_finish_move would have stuck
     * raised for the rest of the match.
     *
     * `raise_dead_delay` is that routine's whole body bar one line --
     * `SLEEP TSEC/2`, then change_anim1a on @dead_wrestler, then DIE.
     * One at a time, which is all the source can have.
     */
    wm_coffin_driver_t coffin_driver;
    int32_t raise_dead_delay;
    wm_arcade_actor_t *raise_dead_target;

    /*
     * LIFEBAR.ASM's WRESTLERS_RESET is owed but has not run yet.
     *
     * The source reaches it from deep inside announce_rnd_winner,
     * long after the round is called -- past `#fini_wait`, the
     * buckoff window, the victory sound, CALL_MATCH_OVER, the dufus
     * messages and a 105-tick button wait. This port models almost
     * none of that presentation, so it cannot reproduce the delay
     * honestly; what it DOES reproduce is the one gate that changes
     * behaviour rather than timing, LIFEBAR.ASM:2649's `#fini_wait`:
     * while @in_finish_move is set the announcement waits, and so
     * does the reset. Without that the Undertaker's coffin sequence
     * is wiped half-way through by the reset for the round it just
     * won.
     *
     * Set on the tick the round is decided and consumed on a later
     * one, so the deciding tick's own state is still observable.
     */
    bool round_reset_pending;

    /*
     * LIFEBAR.ASM:2852 DO_WAIT, the end-of-match path. Started on the
     * tick match_winner is set and run out over the following few
     * hundred, ending with @match_over = 2
     * (wm/arcade/wm_arcade_match_end.h).
     *
     * `streaks` is @p1winstreak / @p2winstreak and the `old` pair
     * beside them, which increment_wincount writes and
     * WRESTLE2.ASM's loser_snd reads back.
     */
    wm_match_end_t match_end;
    /*
     * WRESTLE.ASM:221 `BSSX match_over, 16 ;0=not over, !0=over`, and
     * LIFEBAR.ASM:2985's `MOVK 2,A0 / move a0,@match_over` at the end
     * of DO_WAIT. The main loop at WRESTLE.ASM:2087 polls it and RETPs
     * out of game_loop when it is set, which is how the arcade leaves
     * a finished match; wm_app_tick does the same with this.
     */
    int32_t match_over;
    wm_match_streaks_t streaks;
    /*
     * AWARD.ASM's per-player award arrays. They are per CREDIT rather
     * than per match in the source, which is why the match has always
     * reached round_award through a callback the app owns -- but the
     * end-of-match path calls five award routines directly and needs
     * somewhere to put the answers. This is that, for a match played
     * without an app around it; an app that owns its own
     * wm_award_state can copy it in and out.
     */
    wm_award_state awards;
    /* @match_cnt, for the tip rule. The cabinet counts matches across
       credits, so the app owns it; the match reads it. */
    int32_t match_cnt;

    /* @current_round (LIFEBAR.ASM), incremented by reset_for_round.
       The round-2/round-3 music picks off it, and so does
       VINCE_START_ROUND2_3; this port has neither yet, but the count
       is the reset's own and is kept rather than dropped. */
    int32_t current_round;

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
 * wm_hurt_box_for_frame). See the file comment for what this does and
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
