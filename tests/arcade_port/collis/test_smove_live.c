/*
 * The chain end to end, in a real match: init_smoves makes the
 * Undertaker's watchdogs, a pin prompt moves @p1pins, UP-DOWN-PUNCH
 * reaches und_finish_move1's guards, and the coffin animation starts.
 *
 * Every link in that chain was missing before: nothing called
 * init_smoves, nothing supplied can_pin, nothing bumped the pin
 * counters, and nothing wired change_opp_anim -- so the finishing
 * move could not be reached however completely it was translated.
 */
#include <assert.h>
#include <string.h>

#include "wm/match.h"
#include "wm/anim_program.h"
#include "wm_arcade_roster.h"
#include "wm_arcade_smove.h"
#include "wm_arcade_coffin.h"

static uint32_t hcount(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ff; }
static uint32_t sp(void *u) { uint32_t *t = u; return 0x01000000u + ((*t * 7u) & 0x3fff); }

static wm_match_state M;

/*
 * und_finish_move1's seven guards, held steady for the run: the
 * opponent dead and pinable, WHOIHIT pointing at him, the Undertaker
 * in the ring, and this his second pin attempt. A real match reaches
 * all of it by knocking the man out twice; the wiring is what is under
 * test here, not the fight.
 *
 * Positions are deliberately NOT re-applied. Both start left of
 * RING_X_CENTER+100 -- the only position rule the finish has -- and
 * far enough apart that can_pin refuses, which matters: otherwise the
 * PUNCH that completes the sequence also lands as an ordinary pin and
 * sets B_DID_PIN, and the finish then refuses on that guard. The race
 * is the source's own; a player steps around it by being too far away
 * to pin, and so does this.
 */
static void hold_guards(wm_arcade_actor_t *taker, wm_arcade_actor_t *victim) {
    victim->player_mode = WM_PMODE_DEAD;
    victim->status_flags |= WM_STATUS_PINABLE;
    taker->who_i_hit = victim;
    taker->in_ring = 1;
    M.pins.p1pins = 2;
}

static void start_taker_match(WmRng *rng) {
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    wm_match_start_selected(&M, rng, (uint8_t)WM_ROSTER_TAKER);
    assert(M.actors[0].wrestler_num == WM_ROSTER_TAKER);
}

/* init_smoves ran, and it made exactly what the tables say it can. */
static void test_init_smoves_ran(void) {
    WmRng rng;
    uint32_t t = 1;
    wm_rng_init(&rng, 0x12345678u, hcount, sp, &t);
    start_taker_match(&rng);

    /* The Undertaker is the human here, so he keeps std_taunt. */
    assert(M.smove_count[0] == 3);
    assert(M.smove_unported[0] == 9);
    /* And the counters start clear, as WRESTLE.ASM:1578 leaves them. */
    assert(M.pins.p1pins == 0 && M.pins.p2pins == 0);
    assert(!M.in_finish_move);
    assert(M.coffin.close_the_door == 0);
}

/*
 * The whole thing. The guards are set up by hand -- a real match would
 * reach them by knocking the opponent out twice -- because what is
 * being checked here is the WIRING, not the fight.
 */
static void test_coffin_finish_fires_in_a_match(void) {
    WmRng rng;
    uint32_t t = 1;
    wm_input_state in;
    wm_arcade_actor_t *taker, *victim;
    size_t si;
    int i;
    bool found = false;

    wm_rng_init(&rng, 0x2468ACE0u, hcount, sp, &t);
    start_taker_match(&rng);
    taker = &M.actors[0];
    victim = &M.actors[1];

    /*
     * Let the match settle first. Both wrestlers have to have played a
     * frame before anything else is done to them: until one does his
     * hurt box is still [0,0], and confine_wrestler then measures the
     * left rope against a zero OBJ_COLLX1 and shoves him ~830 pixels a
     * tick. That is the source's own arithmetic on a box this port has
     * not filled in yet, not a pushout bug, and it only happens to a
     * wrestler who has never been drawn.
     */
    memset(&in, 0, sizeof in);
    for (i = 0; i < 4; ++i) wm_match_tick(&M, NULL, &in);
    assert(M.actors[1].hurt_frame_valid);

    /* Both left of RING_X_CENTER+100 and out of pin range. */
    assert(taker->x_int <= 1174 && victim->x_int <= 1174);
    assert(taker->closest_dist > 0x70);

    /* Find his finishing-move watchdog. */
    for (si = 0; si < M.smove_count[0]; ++si)
        if (M.smoves[0][si].monitor &&
            strcmp(M.smoves[0][si].monitor->name, "und_finish_move1") == 0)
            found = true;
    assert(found);

    /*
     * UP, then DOWN, then PUNCH, each held for one tick and released
     * the next -- STICK_REL_NEW only reads on a tick the stick moved,
     * so a held direction would count once and then go quiet.
     *
     * The guard state is re-applied before every tick. A corpse with
     * no fight around it still gets moved by the backends, and this
     * test is about the WIRING reaching und_finish_move1's guards, not
     * about a real knockout putting him there.
     */
    memset(&in, 0, sizeof in);
    hold_guards(taker, victim);
    wm_match_tick(&M, NULL, &in);      /* settle, and arm the monitor */

    in.stick_y = 100;                  /* UP */
    hold_guards(taker, victim);
    wm_match_tick(&M, NULL, &in);
    memset(&in, 0, sizeof in);
    hold_guards(taker, victim);
    wm_match_tick(&M, NULL, &in);

    in.stick_y = -100;                 /* DOWN */
    hold_guards(taker, victim);
    wm_match_tick(&M, NULL, &in);
    memset(&in, 0, sizeof in);
    hold_guards(taker, victim);
    wm_match_tick(&M, NULL, &in);

    in.light_punch = true;             /* PUNCH */
    hold_guards(taker, victim);
    wm_match_tick(&M, NULL, &in);

    /* @in_finish_move is up, which is the flag the scroller and the
       round announcer both read. */
    assert(M.in_finish_move);
    /* And the victim cannot buck off. */
    assert(victim->status_flags & WM_STATUS_NO_BUCKOFF);
    assert(!(victim->status_flags & WM_STATUS_DO_BUCKOFF));
    /* SPECIAL_MOVE_ADDR is set, which is also what stops every other
       watchdog: WAITSWITCH_DWN fails outright while it is non-zero. */
    assert(taker->special_move_addr != 0);

    /* The animation really is running on him. */
    assert(M.wrestler_visual[0].current_label);
    assert(strcmp(M.wrestler_visual[0].current_label,
                  "und_2_raise_dead_anim") == 0);
    assert(M.wrestler_visual[0].prog.program);

    /* ANI_CREATEPROC started both of the sequence's processes. */
    assert(wm_coffin_driver_busy(&M.coffin_driver));
    assert(M.raise_dead_delay > 0);
    assert(M.raise_dead_target == victim);

    /*
     * Now let it play. Everything from here is the sequence talking to
     * itself: raise_dead stands the dead man up, the Undertaker's
     * animation polls is_guy_up and is_door_open, push_to_coffin and
     * make_wres_disappear reach the dead man through change_opp_anim,
     * and the driver walks the mat, the coffin and the tombstone.
     */
    memset(&in, 0, sizeof in);
    for (i = 0; i < 250 && !(victim->status_flags & WM_STATUS_GUY_UP); ++i) {
        hold_guards(taker, victim);
        wm_match_tick(&M, NULL, &in);
    }
    /* raise_dead fired and stand_wrestler ran guy_is_up on him. */
    assert(victim->status_flags & WM_STATUS_GUY_UP);

    /* The coffin comes up and the door opens: @close_the_door moves
       off zero, which is what is_door_open answers on. */
    for (i = 0; i < 1200 && M.coffin.close_the_door == 0; ++i) {
        hold_guards(taker, victim);
        wm_match_tick(&M, NULL, &in);
    }
    assert(M.coffin.close_the_door >= 1);

    /*
     * And the whole thing finishes. Without the driver, TAKER.ASM's
     * `#fdone_wait` would spin forever: @finish_completed would never
     * be set, @in_finish_move would stay raised, and the scroller and
     * the round announcer would both be stuck for the rest of the
     * match. That it clears is the point of running the processes at
     * all.
     */
    for (i = 0; i < 4000 && M.in_finish_move; ++i) {
        hold_guards(taker, victim);
        wm_match_tick(&M, NULL, &in);
    }
    assert(!M.in_finish_move);
    assert(M.coffin.close_the_floor == 3);   /* the tombstone is up */
    assert(!wm_coffin_driver_busy(&M.coffin_driver));
}

/*
 * The driver on its own, with no match around it: the two processes
 * reach every state in the source's order and stop.
 */
static void test_coffin_driver(void) {
    wm_coffin_driver_t d;
    wm_coffin_state_t st;
    int i, puffs = 0;
    int saw_door_open = 0, saw_door_shut = 0, saw_floor_close = 0;

    wm_coffin_reset(&st);
    wm_coffin_driver_start(&d, &st);
    assert(wm_coffin_driver_busy(&d));

    for (i = 0; i < 5000 && wm_coffin_driver_busy(&d); ++i) {
        /* The Undertaker's animation is what asks for the door to
           shut; here that is done by hand the moment it opens. */
        if (st.close_the_door == 1) wm_coffin_close_door(&st);
        wm_coffin_driver_tick(&d, &st);
        puffs += d.puffs;
        if (st.close_the_door >= 1) saw_door_open = 1;
        if (st.close_the_door == 3) saw_door_shut = 1;
        if (st.close_the_floor >= 1) saw_floor_close = 1;
    }
    assert(!wm_coffin_driver_busy(&d));
    assert(saw_door_open && saw_door_shut && saw_floor_close);
    assert(st.close_the_floor == 3);
    assert(st.finish_completed == 3);
    /* Two 25-puff bursts, one a tick down the 34-step fall, plus the
       hover's eights -- so comfortably more than the bursts alone. */
    assert(puffs > 2 * 25 + 34);
}

/*
 * pin_prompt, through the round loop: when a side is wiped out the
 * counter moves, and it moves ONCE rather than every tick of the
 * five-second countdown.
 */
static void test_pin_counter(void) {
    WmRng rng;
    uint32_t t = 1;
    wm_input_state in;
    int i;

    wm_rng_init(&rng, 0x13572468u, hcount, sp, &t);
    start_taker_match(&rng);
    memset(&in, 0, sizeof in);

    M.actors[0].player_mode = WM_PMODE_NORMAL;
    M.actors[0].in_ring = 1;
    M.actors[1].player_mode = WM_PMODE_DEAD;
    M.actors[1].in_ring = 1;

    for (i = 0; i < 20; ++i) {
        /* Keep him dead: the backends would otherwise move him on. */
        M.actors[1].player_mode = WM_PMODE_DEAD;
        wm_match_tick(&M, NULL, &in);
    }
    assert(M.pins.p1pins == 1);
    assert(M.pins.p2pins == 0);
}

int main(void) {
    test_init_smoves_ran();
    test_coffin_finish_fires_in_a_match();
    test_coffin_driver();
    test_pin_counter();
    return 0;
}
