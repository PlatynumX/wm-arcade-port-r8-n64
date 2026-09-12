/*
 * WRESTLE2.ASM's roster predicates.
 */
#include "wm/arcade/wm_arcade_teammates.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static wm_arcade_actor_t g[4];
static wm_arcade_actor_t *list[4];

static void setup(void)
{
    int i;
    memset(g, 0, sizeof(g));
    for (i = 0; i < 4; ++i) {
        list[i] = &g[i];
        g[i].player_side = (i & 1);
        g[i].player_mode = WM_PMODE_NORMAL;
    }
}

static void test_teammate_pin(void)
{
    setup();
    /* g[0] and g[2] are side 0; g[1] and g[3] are side 1. */
    assert(!wm_ck_teammate_pin(&g[0], list, 4));

    /* My own pin does not count -- `cmp a3,a13 / jreq #nxt`. */
    g[0].status_flags |= WM_STATUS_DID_PIN;
    assert(!wm_ck_teammate_pin(&g[0], list, 4));

    /* An enemy's does not either. */
    g[1].status_flags |= WM_STATUS_DID_PIN;
    assert(!wm_ck_teammate_pin(&g[0], list, 4));

    /* A teammate's does, and the relation is symmetric: g[0] and
     * g[2] are each other's only teammate. */
    setup();
    g[2].status_flags |= WM_STATUS_DID_PIN;
    assert(wm_ck_teammate_pin(&g[0], list, 4));
    assert(!wm_ck_teammate_pin(&g[2], list, 4));
    g[0].status_flags |= WM_STATUS_DID_PIN;
    assert(wm_ck_teammate_pin(&g[2], list, 4));

    /* An empty slot is skipped, not dereferenced. */
    setup();
    g[2].status_flags |= WM_STATUS_DID_PIN;
    list[1] = NULL;
    assert(wm_ck_teammate_pin(&g[0], list, 4));
    list[2] = NULL;
    assert(!wm_ck_teammate_pin(&g[0], list, 4));
}

static void test_live_and_any_teammates(void)
{
    setup();
    assert(wm_ck_live_teammates(&g[0], list, 4));
    assert(wm_ck_any_teammates(&g[0], list, 4));

    /* A dead teammate still counts as a teammate -- that is the whole
     * difference between the two routines. */
    g[2].player_mode = WM_PMODE_DEAD;
    assert(!wm_ck_live_teammates(&g[0], list, 4));
    assert(wm_ck_any_teammates(&g[0], list, 4));

    /* Alone on my side. */
    setup();
    g[2].player_side = 1;
    assert(!wm_ck_live_teammates(&g[0], list, 4));
    assert(!wm_ck_any_teammates(&g[0], list, 4));

    /* And I am never my own teammate, dead or alive. */
    setup();
    list[2] = NULL;
    g[0].player_mode = WM_PMODE_DEAD;
    assert(!wm_ck_any_teammates(&g[0], list, 4));
}

static void test_is_behind(void)
{
    setup();
    g[0].x_int = 100;

    /* Opponent to my right (bigger X). */
    g[1].x_int = 200;
    g[0].facing_dir = WM_MOVE_RIGHT;
    assert(!wm_is_behind(&g[0], &g[1]));
    g[0].facing_dir = WM_MOVE_LEFT;
    assert(wm_is_behind(&g[0], &g[1]));

    /* Opponent to my left. */
    g[1].x_int = 50;
    g[0].facing_dir = WM_MOVE_LEFT;
    assert(!wm_is_behind(&g[0], &g[1]));
    g[0].facing_dir = WM_MOVE_RIGHT;
    assert(wm_is_behind(&g[0], &g[1]));

    /* A diagonal facing still carries the horizontal bit, so it
     * answers the same way. */
    g[1].x_int = 200;
    g[0].facing_dir = WM_MOVE_UP_RIGHT;
    assert(!wm_is_behind(&g[0], &g[1]));
    g[0].facing_dir = WM_MOVE_DOWN_LEFT;
    assert(wm_is_behind(&g[0], &g[1]));

    /* Facing straight up or down has neither bit, so the opponent is
     * behind whichever side he is on. */
    g[0].facing_dir = WM_MOVE_UP;
    assert(wm_is_behind(&g[0], &g[1]));
    g[1].x_int = 50;
    assert(wm_is_behind(&g[0], &g[1]));

    /* Exactly level counts as "on my left": `jrn` needs a strictly
     * negative difference to take the right-hand branch. */
    g[1].x_int = 100;
    g[0].facing_dir = WM_MOVE_LEFT;
    assert(!wm_is_behind(&g[0], &g[1]));
    g[0].facing_dir = WM_MOVE_RIGHT;
    assert(wm_is_behind(&g[0], &g[1]));
}

static void test_clr_climb(void)
{
    wm_arcade_actor_t a;
    memset(&a, 0, sizeof(a));
    a.climbing_thru = 1;
    a.safe_time = 0;

    wm_clr_climb(&a);
    assert(a.climbing_thru == 0);
    /* `inc a0` before the store -- one tick of grace, not zero. */
    assert(a.safe_time == 1);
}

int main(void)
{
    test_teammate_pin();
    test_live_and_any_teammates();
    test_is_behind();
    test_clr_climb();
    printf("WRESTLE2.ASM roster predicates: all checks passed\n");
    return 0;
}
