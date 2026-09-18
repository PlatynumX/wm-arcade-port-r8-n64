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
     * raisearm_check's seam is deliberately NULL, and this asserts the
     * decision so it cannot be undone silently. src/core/wrestler_
     * backend.c carries the reasoning; the short version is that the
     * routine is right and wiring it strands the match. Measured on the
     * live loop: at tick 0 the dead opponent is still in the ring, the
     * check says no, the winner pins and the round ends; by tick 2 his
     * death animation has rolled him out, the check correctly says yes,
     * and the winner poses forever, because this port's round only ends
     * on the pin. The arcade ends it from the DEAD man's own path
     * (win_announce -> announce_rnd_winner, which
     * wm_arcade_round_announce_tick translates and nothing drives when
     * nobody pins). That path is the blocker, and the two have to land
     * together.
     */
    assert(roster_cb.raisearm_check == NULL);
    assert(razor_cb.raisearm_check == NULL);
    assert(bret_cb.raisearm_check == NULL);

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

int main(void) {
    test_control_comes_back();
    test_the_raisearm_bit();
    test_every_opponent_must_be_dead_and_stay_dead();
    test_the_inside_outside_rule();
    test_the_royal_rumble_hack();
    test_the_seams_are_wired();
    printf("victory pose ok\n");
    return 0;
}
