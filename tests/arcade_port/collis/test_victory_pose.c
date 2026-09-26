/*
 * The auto-pin's return path, and the victory pose.
 *
 *   drone_change_back   WRESTLE2.ASM:3416  temp drone -> human again
 *   set_raisearm_bit    WRESTLE2.ASM:4514  he raised it
 *   raisearm_check      WRESTLE2.ASM:3683  may he?
 *
 * All three seams were empty in every dispatcher, which only started
 * mattering once auto_pin_check could really hand a human to the drone
 * AI: nothing turned him back, so the loan was never repaid.
 *
 * And seven of the eight dispatchers were missing one of the two
 * drone_change_back calls the source makes. Every wrestler file calls
 * it twice -- once at the end of #pin and once at the end of #raisearm
 * ("if we're a temp drone for auto-pin purposes, turn back into a
 * normal player here") -- and six of them had only the #pin one. Razor
 * and Bret had both.
 */
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "wm/arcade/wm_arcade_auto_pin.h"
#include "wm/arcade/wm_arcade_final_battle.h"
#include "wm/wrestler_backend.h"
#include "wm/bret_backend.h"
#include "wm/match.h"
#include "wm_arcade_roster.h"

static void winner(wm_arcade_actor_t *a) {
    memset(a, 0, sizeof *a);
    a->active = 1;
    a->in_ring = 1;
    a->player_side = 0;
    a->player_num = 0;
    a->player_mode = WM_PMODE_NORMAL;
}

static void loser(wm_arcade_actor_t *a) {
    memset(a, 0, sizeof *a);
    a->active = 1;
    a->in_ring = 1;
    a->player_side = 1;
    a->player_num = 1;
    a->player_mode = WM_PMODE_DEAD;
}

/* drone_change_back: one guard, one store, and no question asked. */
static void test_control_comes_back(void) {
    wm_arcade_actor_t a;

    /* A temp drone -- what auto_pin_check made of him. */
    winner(&a);
    a.plyr_type = WM_PTYPE_DRONE;
    assert(wm_arcade_drone_change_back(&a));
    assert(a.plyr_type == WM_PTYPE_PLAYER);

    /* "don't bother checking if they're a drone or not. In either case,
       turning them human again won't hurt." */
    winner(&a);
    assert(a.plyr_type == WM_PTYPE_PLAYER);
    assert(wm_arcade_drone_change_back(&a));
    assert(a.plyr_type == WM_PTYPE_PLAYER);

    /* "don't check real drones" -- PLYRNUM 2 and up were never human,
       and turning them into players WOULD hurt. */
    winner(&a);
    a.player_num = 2;
    a.plyr_type = WM_PTYPE_DRONE;
    assert(!wm_arcade_drone_change_back(&a));
    assert(a.plyr_type == WM_PTYPE_DRONE);

    winner(&a);
    a.player_num = 1;                      /* the second credit slot */
    a.plyr_type = WM_PTYPE_DRONE;
    assert(wm_arcade_drone_change_back(&a));
    assert(a.plyr_type == WM_PTYPE_PLAYER);

    assert(!wm_arcade_drone_change_back(NULL));
}

static void test_the_raisearm_bit(void) {
    wm_arcade_actor_t a;

    winner(&a);
    a.status_flags = WM_STATUS_DID_PIN;    /* something else already set */
    wm_arcade_set_raisearm_bit(&a);
    assert(a.status_flags & WM_STATUS_DID_RAISEARM);
    assert(a.status_flags & WM_STATUS_DID_PIN);   /* an ori, not a store */
    wm_arcade_set_raisearm_bit(&a);                /* idempotent */
    assert(a.status_flags == (WM_STATUS_DID_PIN | WM_STATUS_DID_RAISEARM));
    wm_arcade_set_raisearm_bit(NULL);
}

/* raisearm_check's real test: every opponent dead and staying dead. */
static void test_every_opponent_must_be_dead_and_stay_dead(void) {
    wm_arcade_actor_t me, opp;
    wm_arcade_actor_t *roster[2];

    winner(&me); loser(&opp);
    roster[0] = &me; roster[1] = &opp;
    /* He is inside, the body is inside: no pose. See the geometry test. */
    assert(!wm_arcade_raisearm_check(&me, roster, 2u, false, true));

    /* Move the body out, and he may pose. */
    opp.in_ring = 0;
    assert(wm_arcade_raisearm_check(&me, roster, 2u, false, true));

    /* Still breathing: no. */
    opp.player_mode = WM_PMODE_NORMAL;
    assert(!wm_arcade_raisearm_check(&me, roster, 2u, false, true));

    /* Dead but a ZOMBIE -- coming back, so not beaten. */
    opp.player_mode = WM_PMODE_DEAD;
    opp.status_flags |= WM_STATUS_ZOMBIE;
    assert(!wm_arcade_raisearm_check(&me, roster, 2u, false, true));
    opp.status_flags &= (uint32_t)~WM_STATUS_ZOMBIE;
    assert(wm_arcade_raisearm_check(&me, roster, 2u, false, true));

    /* A TEAMMATE is skipped, alive or not: the sweep's one comparison
       is on PLYR_SIDE, which excludes himself and his own side. */
    {
        wm_arcade_actor_t mate;
        wm_arcade_actor_t *three[3];
        winner(&mate);
        mate.player_num = 1;
        mate.player_mode = WM_PMODE_NORMAL;   /* very much alive */
        three[0] = &me; three[1] = &mate; three[2] = &opp;
        assert(wm_arcade_raisearm_check(&me, three, 3u, false, true));
    }

    /* A NULL slot is skipped, like an inactive process. */
    {
        wm_arcade_actor_t *holes[3];
        holes[0] = &me; holes[1] = NULL; holes[2] = &opp;
        assert(wm_arcade_raisearm_check(&me, holes, 3u, false, true));
    }

    assert(!wm_arcade_raisearm_check(NULL, roster, 2u, false, true));
}

/*
 * The geometry, which is the part worth reading twice: outside the ring
 * he may pose over anything; inside, a dead opponent in there with him
 * stops it.
 */
static void test_the_inside_outside_rule(void) {
    wm_arcade_actor_t me, opp;
    wm_arcade_actor_t *roster[2];

    winner(&me); loser(&opp);
    roster[0] = &me; roster[1] = &opp;

    /* Both inside: no. */
    assert(!wm_arcade_raisearm_check(&me, roster, 2u, false, true));

    /* He steps out: yes, and the body's position stops mattering --
       `jrnz #setc` skips the a4 test entirely. */
    me.in_ring = 0;
    assert(wm_arcade_raisearm_check(&me, roster, 2u, false, true));

    /* He is inside and the body is outside: yes. */
    me.in_ring = 1;
    opp.in_ring = 0;
    assert(wm_arcade_raisearm_check(&me, roster, 2u, false, true));

    /* Two bodies, one of them inside with him: no. */
    {
        wm_arcade_actor_t opp2;
        wm_arcade_actor_t *three[3];
        loser(&opp2);
        opp2.player_num = 2;
        three[0] = &me; three[1] = &opp; three[2] = &opp2;
        assert(!wm_arcade_raisearm_check(&me, three, 3u, false, true));
        opp2.in_ring = 0;
        assert(wm_arcade_raisearm_check(&me, three, 3u, false, true));
    }
}

/* The royal-rumble hack, which the source labels as one. */
static void test_the_royal_rumble_hack(void) {
    wm_arcade_actor_t me, opp;
    wm_arcade_actor_t *roster[2];

    winner(&me); loser(&opp);
    opp.in_ring = 0;
    roster[0] = &me; roster[1] = &opp;

    /* Not a rumble: the hack is skipped whatever the queue says. */
    assert(wm_arcade_raisearm_check(&me, roster, 2u, false, false));
    assert(wm_arcade_raisearm_check(&me, roster, 2u, false, true));

    /* A rumble, and he is HUMAN: only once the queue is empty. */
    assert(!wm_arcade_raisearm_check(&me, roster, 2u, true, false));
    assert(wm_arcade_raisearm_check(&me, roster, 2u, true, true));

    /* A rumble, and he is a DRONE: `PLAYER=0`, so a nonzero PLYR_TYPE
       jumps straight past the hack and the queue is not consulted. */
    me.plyr_type = WM_PTYPE_DRONE;
    assert(wm_arcade_raisearm_check(&me, roster, 2u, true, false));

    /* And the queue-empty answer really is wm_final_queue_empty's: a
       NULL state reads as empty, which is what a match with no final
       battle has. */
    assert(wm_final_queue_empty(NULL));
}

/* The three seams, on both backends. */
static void test_the_seams_are_wired(void) {
    wm_wrestler_backend_actor st;
    wm_bret_backend_actor bva;
    wm_arcade_roster_callbacks_t roster_cb;
    wm_arcade_razor_callbacks_t razor_cb;
    wm_arcade_bret_callbacks_t bret_cb;
    wm_arcade_actor_t me, opp;
    wm_arcade_actor_t *roster[2];

    memset(&st, 0, sizeof st);
    roster_cb = wm_wrestler_roster_callbacks(&st);
    razor_cb = wm_wrestler_razor_callbacks(&st);
    wm_bret_backend_init(&bva);
    bret_cb = wm_bret_backend_callbacks(&bva);

    assert(roster_cb.set_raisearm_bit && roster_cb.drone_change_back);
    assert(razor_cb.set_raisearm_bit && razor_cb.drone_change_back);
    assert(bret_cb.set_raisearm_bit && bret_cb.drone_change_back);

    /*
     * And raisearm_check's, in all three. Wiring this one needed the
     * announcer's own tail first: the #raisearm branch plays
     * xxx_N_raise_arm_anim, whose FIFTH op is ANI_CODE win_announce --
     * so the pose really does end the round, exactly as the arcade's
     * does, and it was announce_rnd_winner stopping at CALL_MATCH_OVER
     * without ever reaching WRESTLERS_RESET that stranded the match.
     * See wm/arcade/wm_arcade_round_announce.h.
     */
    assert(roster_cb.raisearm_check != NULL);
    assert(razor_cb.raisearm_check != NULL);
    assert(bret_cb.raisearm_check != NULL);

    winner(&me); loser(&opp);
    opp.in_ring = 0;
    roster[0] = &me; roster[1] = &opp;
    st.all_actors = roster;
    st.all_actor_count = 2u;

    roster_cb.set_raisearm_bit(&me, roster_cb.user);
    assert(me.status_flags & WM_STATUS_DID_RAISEARM);
    me.plyr_type = WM_PTYPE_DRONE;
    roster_cb.drone_change_back(&me, roster_cb.user);
    assert(me.plyr_type == WM_PTYPE_PLAYER);

    /* Bret's, which read his own roster. */
    bva.all_actors = roster;
    bva.all_actor_count = 2u;
    me.plyr_type = WM_PTYPE_DRONE;
    bret_cb.drone_change_back(&me, bret_cb.user);
    assert(me.plyr_type == WM_PTYPE_PLAYER);
}

/* ------------------------------------------------------------------
 * The pose on the live loop, end to end.
 *
 * This is the test the whole change exists for. The #raisearm branch
 * plays xxx_N_raise_arm_anim, and that animation's fifth op is
 * `ANI_CODE,win_announce` (DNKSEQ2.ASM:5159 and its seven siblings) --
 * so the victory pose starts announce_rnd_winner and ends the round,
 * with no pin anywhere in it.
 *
 * What it did NOT do until now is start the NEXT round.
 * WRESTLERS_RESET lives inside announce_rnd_winner and nowhere else
 * (LIFEBAR.ASM:3071), and this port's translation of that process
 * stopped at CALL_MATCH_OVER. The reset was owed off the KO countdown
 * instead, which carried every live round only because nothing had
 * ever reached win_announce in the match loop.
 * ------------------------------------------------------------------ */

static wm_match_state MS;
static uint32_t hc_stub(void *u) { (void)u; return 0; }
static uint32_t spf_stub(void *u) { (void)u; return 1; }

static void test_the_pose_ends_the_round_and_the_next_one_starts(void) {
    WmRng r;
    uint32_t t = 1;
    wm_input_state in;
    int i;
    int first_award = -1, first_reset = -1;
    int32_t round0;
    int posed = 0, pinned = 0;

    wm_rng_init(&r, 0x12345678u, hc_stub, spf_stub, &t);
    memset(&MS, 0, sizeof MS);
    wm_match_init(&MS);
    wm_match_start_selected(&MS, &r, (uint8_t)WM_ROSTER_TAKER);
    memset(&in, 0, sizeof in);
    round0 = (int32_t)MS.current_round;

    for (i = 0; i < 4000 && MS.match_end.match_over == 0; ++i) {
        unsigned k;
        /* Hold side 1 dead, and never press a button: there is no pin
           on this path, which is the point. */
        for (k = 0; k < MS.actor_count; ++k)
            if (MS.actors[k].player_side == 1) {
                MS.actors[k].player_mode = (uint16_t)WM_PMODE_DEAD;
                MS.actors[k].life = 0;
            }
        wm_match_tick(&MS, NULL, &in);
        if (first_award < 0 && MS.score.p1rounds == 1) first_award = i;
        if (first_award >= 0 && first_reset < 0 && MS.current_round != round0)
            first_reset = i;
        if (MS.actors[0].status_flags & WM_STATUS_DID_RAISEARM) posed = 1;
        if (MS.actors[0].status_flags & WM_STATUS_DID_PIN) pinned = 1;
    }

    /*
     * The round is ended by the pose, and quickly: the opponent's death
     * animation rolls him out of the ring within a couple of ticks, and
     * from there raisearm_check says yes. The KO countdown that used to
     * be the only thing ending a round takes 264.
     */
    assert(first_award >= 0 && first_award < 30);
    /* It really was the announcer, and it really was win_announce that
       started it -- KILL_PIN_HIM is reached nowhere else. */
    assert(MS.round_announce.pin_him_kills >= 1u);
    /* Nobody pinned; he posed. */
    assert(posed);
    assert(!pinned);

    /*
     * And the next round starts -- SLEEPK 30 then SLEEPK 20 after the
     * award, which is where the reset lands. Before the announcer owed
     * its own WRESTLERS_RESET this stuck at one round forever.
     */
    assert(first_reset >= 0);
    assert(first_reset - first_award >= WM_ARW_PRE_TOKEN_SLEEP);
    assert(first_reset - first_award <=
           WM_ARW_PRE_TOKEN_SLEEP + WM_ARW_POST_TOKEN_SLEEP + 4);

    /* The whole match plays out, as it does on the pin path. */
    assert(MS.match_end.match_over == 2);
    assert(MS.score.p1rounds == 2);
}

int main(void) {
    test_control_comes_back();
    test_the_raisearm_bit();
    test_every_opponent_must_be_dead_and_stay_dead();
    test_the_inside_outside_rule();
    test_the_royal_rumble_hack();
    test_the_seams_are_wired();
    test_the_pose_ends_the_round_and_the_next_one_starts();
    printf("victory pose ok\n");
    return 0;
}
