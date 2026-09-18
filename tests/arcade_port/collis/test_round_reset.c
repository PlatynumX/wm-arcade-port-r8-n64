/*
 * WRESTLE.ASM's between-round reset and the small routines around it.
 * Every expectation is worked out from the source, not recorded from
 * the port.
 */
#include <assert.h>
#include <string.h>

#include "wm_arcade_round_reset.h"
#include "wm_arcade_combat_defs.h"
#include "wmania_ring_geometry.h"

/* ---- the starting-position table --------------------------------- */

static void test_start_table(void) {
    /* WRESTLE.ASM:2887, read off the source. Team 1's Z base is 1127
       and team 2's is 1103 -- twenty-four apart, so the two sides do
       not start on the same depth line. */
    assert(wm_round_team_starts[0][0].x == WM_RING_X_CENTER - 85);
    assert(wm_round_team_starts[0][0].z == 1127 + 93);
    assert(wm_round_team_starts[0][0].facing == WM_MOVE_UP_RIGHT);
    assert(wm_round_team_starts[0][2].facing == WM_MOVE_DOWN_RIGHT);
    assert(wm_round_team_starts[1][0].x == WM_RING_X_CENTER + 85);
    assert(wm_round_team_starts[1][0].z == 1103 + 93);
    assert(wm_round_team_starts[1][0].facing == WM_MOVE_DOWN_LEFT);
    assert(wm_round_team_starts[1][1].facing == WM_MOVE_UP_LEFT);

    /* Both sides start facing each other, which is the point of the
       facings and is easy to get wrong by one bit. */
    assert(wm_round_team_starts[0][0].facing & WM_MOVE_RIGHT);
    assert(wm_round_team_starts[1][0].facing & WM_MOVE_LEFT);
}

static void test_start_index(void) {
    wm_arcade_actor_t a, b, c;
    wm_arcade_actor_t *all[3];

    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    memset(&c, 0, sizeof c);
    all[0] = &a; all[1] = &b; all[2] = &c;
    a.active = b.active = c.active = 1;
    a.player_side = 0;
    b.player_side = 1;
    c.player_side = 0;

    /* "the number of teammates with PLYRNUM's lower than ours" -- the
       other side does not count, and neither does the man himself. */
    assert(wm_round_start_index(&a, all, 3) == 0);
    assert(wm_round_start_index(&b, all, 3) == 0);
    assert(wm_round_start_index(&c, all, 3) == 1);

    /* An inactive slot is skipped, not counted. */
    a.active = 0;
    assert(wm_round_start_index(&c, all, 3) == 0);
    a.active = 1;

    /* Somebody not in the list at all: the source would index past
       the table, so this refuses. */
    {
        wm_arcade_actor_t stranger;
        memset(&stranger, 0, sizeof stranger);
        assert(wm_round_start_index(&stranger, all, 3) == -1);
    }
    assert(wm_round_start_index(NULL, all, 3) == -1);
}

/* ---- reset_wrestle ----------------------------------------------- */

static void test_reset_wrestler(void) {
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *all[2];

    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    all[0] = &a; all[1] = &b;
    a.active = b.active = 1;
    a.player_side = 0;
    b.player_side = 1;

    /* Leave him somewhere wrong, mid-air, mid-animation, counted out. */
    a.x_int = 5; a.x_fixed = 5 << 16;
    a.z_int = 5; a.z_fixed = 5 << 16;
    a.y_int = 200; a.y_fixed = 200 << 16;
    a.x_vel = a.y_vel = a.z_vel = 0x40000;
    a.player_mode = (uint16_t)WM_PMODE_DEAD;
    a.anim_mode = 0xFFFF;
    a.plyr_dizzy = 9;
    a.in_ring = 0;
    a.facing_dir = WM_MOVE_LEFT;

    wm_round_reset_wrestler(&a, all, 2, 0x12345);

    assert(a.x_int == WM_RING_X_CENTER - 85);
    assert(a.x_fixed == (int32_t)(WM_RING_X_CENTER - 85) << 16);
    assert(a.z_int == 1127 + 93);
    assert(a.facing_dir == WM_MOVE_UP_RIGHT);
    assert(a.new_facing_dir == WM_MOVE_UP_RIGHT);

    /* The inlined veladd clamp puts him ON the mat, not above it and
       not falling toward it. */
    assert(a.ground_y == WM_MAT_Y);
    assert(a.y_fixed == (int32_t)WM_MAT_Y << 16);
    assert(a.y_int == WM_MAT_Y);
    assert(a.y_vel == 0);
    assert(a.x_vel == 0 && a.z_vel == 0);

    assert(a.player_mode == WM_PMODE_NORMAL);
    assert(a.anim_mode == 0);
    assert(a.plyr_dizzy == 0);

    /* `clr INRING` is INSIDE the ring, and this port's boolean reads
       the other way round -- so it must be SET. Getting this backwards
       starts every round with both wrestlers counted out. */
    assert(a.in_ring == 1);

    /* "Don't allow meters for the first x seconds of round", in
       60ths rather than TSEC ticks. */
    assert(a.delay_meter == 14 * 60);

    /* A WORD store, so the stamp is truncated. */
    assert(a.foot_pcnt == (uint16_t)0x2345);

    /* The second wrestler goes to the other side's row 0. */
    wm_round_reset_wrestler(&b, all, 2, 0);
    assert(b.x_int == WM_RING_X_CENTER + 85);
    assert(b.facing_dir == WM_MOVE_DOWN_LEFT);
    /* And they really are facing each other. */
    assert(a.x_int < b.x_int);
}

/* ---- reset_wrestle2 ---------------------------------------------- */

static void test_reset_wrestler2(void) {
    wm_arcade_actor_t a;

    memset(&a, 0, sizeof a);
    a.getup_time = 5;
    a.special_move_addr = 0x1234;
    a.last_hit_time = 99;
    a.last_headhold = 99;
    a.last_spunch = 99;
    a.last_skick = 99;
    a.consecutive_hits = 4;
    a.last_fling = 99;
    a.last_hiptoss = 99;
    a.last_damage = 12;
    a.ptime = 0;
    a.status_flags = 0xFFFFFFFFu;

    wm_round_reset_wrestler2(&a);

    /* Half a second of nobody moving. */
    assert(a.immobilize_time == 30);

    assert(a.getup_time == 0);
    assert(a.special_move_addr == 0);
    assert(a.last_hit_time == 0);
    assert(a.last_headhold == 0);
    assert(a.last_spunch == 0);
    assert(a.last_skick == 0);
    assert(a.consecutive_hits == 0);
    assert(a.last_fling == 0);
    assert(a.last_hiptoss == 0);
    assert(a.last_damage == 0);

    /* PLYR.EQU:443 SF_RESET_MASK -- exactly two bits survive. */
    assert(a.status_flags == (WM_STATUS_TEMP_PAL | WM_STATUS_DID_BUCKOFF));
    /* Everything a round can leave behind is gone, by name. */
    assert(!(a.status_flags & WM_STATUS_PINNED));
    assert(!(a.status_flags & WM_STATUS_DID_PIN));
    assert(!(a.status_flags & WM_STATUS_KOD));
    assert(!(a.status_flags & WM_STATUS_ZOMBIE));
    assert(!(a.status_flags & WM_STATUS_GUY_UP));
    assert(!(a.status_flags & WM_STATUS_PINABLE));

    /* "just in case they were KO'd last round". */
    assert(a.ptime == 1);

    /* The two bits that DO survive are not cleared when already set
       and not invented when not. */
    memset(&a, 0, sizeof a);
    a.status_flags = WM_STATUS_KOD;
    wm_round_reset_wrestler2(&a);
    assert(a.status_flags == 0);
}

/* ---- update_links ------------------------------------------------ */

static void test_update_links(void) {
    wm_arcade_actor_t a, b;

    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);

    /* Not attached: nothing happens. */
    wm_round_update_links(&a);
    assert(a.attach_proc == NULL);

    /* Attached both ways: the link stands. */
    a.attach_proc = &b;
    b.attach_proc = &a;
    wm_round_update_links(&a);
    assert(a.attach_proc == &b);

    /* He let go of me: my side is dropped. */
    b.attach_proc = NULL;
    wm_round_update_links(&a);
    assert(a.attach_proc == NULL);

    /* He is holding somebody ELSE: same answer. And note it only
       breaks its own side -- b keeps what b has. */
    {
        wm_arcade_actor_t c;
        memset(&c, 0, sizeof c);
        a.attach_proc = &b;
        b.attach_proc = &c;
        wm_round_update_links(&a);
        assert(a.attach_proc == NULL);
        assert(b.attach_proc == &c);
    }
    wm_round_update_links(NULL);
}

/* ---- init_scroller ----------------------------------------------- */

static void test_init_scroller(void) {
    int32_t x = 0, y = 0;

    wm_round_init_scroller(&x, &y, 1);
    /* Half a screen left of the ring's centre, so the ring is centred
       on the first frame rather than sliding in from the right. */
    assert(x == (int32_t)(WM_RING_X_CENTER - 200) << 16);
    /* A NEGATIVE Y -- the source writes it as a raw hex word. */
    assert(y == (int32_t)0xffe50000);
    assert(y < 0);
    assert((y >> 16) == -27);

    /* `cmpi 2,a14 / jrne #sety` -- exactly two opponents, not "two or
       more". 1 and 3 both take the default. */
    wm_round_init_scroller(&x, &y, 2);
    assert(y == (int32_t)0xffe90000);
    assert((y >> 16) == -23);
    wm_round_init_scroller(&x, &y, 3);
    assert(y == (int32_t)0xffe50000);

    wm_round_init_scroller(NULL, NULL, 1);
}

/* ---- show_most_damage -------------------------------------------- */

static void test_most_damage(void) {
    int32_t dmg[2], type[2], pct;

    /* Two humans, player 1 well ahead. */
    dmg[0] = 75; dmg[1] = 25;
    type[0] = type[1] = 0;
    assert(wm_round_most_damage(dmg, 2, type, &pct) == 1);
    assert(pct == 75);

    /* Player 2 ahead. */
    dmg[0] = 25; dmg[1] = 75;
    assert(wm_round_most_damage(dmg, 2, type, &pct) == 2);
    assert(pct == 75);

    /* A tie goes to player 2: `cmp a8,a9 / jrlt #p1_most` takes
       player 1 only on strictly more. */
    dmg[0] = 50; dmg[1] = 50;
    assert(wm_round_most_damage(dmg, 2, type, &pct) == 2);
    assert(pct == 50);

    /* Drones are skipped, and are NOT in the total either -- so a
       human fighting a drone did 100% of "the damage" however hard
       the drone hit him. That is the source's own arithmetic. */
    dmg[0] = 30; dmg[1] = 900;
    type[0] = 0; type[1] = 1;
    assert(wm_round_most_damage(dmg, 2, type, &pct) == 1);
    assert(pct == 100);

    /* The divide truncates: 1 of 3 is 33, not 34. */
    dmg[0] = 1; dmg[1] = 2;
    type[0] = type[1] = 0;
    assert(wm_round_most_damage(dmg, 2, type, &pct) == 2);
    assert(pct == 66);

    /* Nobody did anything: the source would divide by zero here. */
    dmg[0] = dmg[1] = 0;
    assert(wm_round_most_damage(dmg, 2, type, &pct) == 0);
    assert(pct == 0);
    assert(wm_round_most_damage(NULL, 2, type, &pct) == 0);
}

/* ---- loser_snd --------------------------------------------------- */

static uint32_t hc(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ff; }
static uint32_t spf(void *u) { uint32_t *t = u; return 0x01000000u + ((*t * 7u) & 0x3fff); }

static void test_loser_snd(void) {
    WmRng rng;
    uint32_t t = 1;
    int32_t streak[2];
    int i;
    int saw[3] = { 0, 0, 0 };

    wm_rng_init(&rng, 0x12345678u, hc, spf, &t);
    streak[0] = 4;
    streak[1] = 0;

    /* One human: nothing is said however long the streak was. */
    assert(wm_round_loser_snd(1, 2, streak, &rng) == 0);

    /* Two humans, player 2 won -- so the LOSER is player 1, who had a
       streak. `NOT 2 & 3` is 1, minus 1 is 0. */
    assert(wm_round_loser_snd(3, 2, streak, &rng) != 0);

    /* Player 1 won: the loser is player 2, whose streak was zero. */
    assert(wm_round_loser_snd(3, 1, streak, &rng) == 0);
    streak[1] = 2;
    assert(wm_round_loser_snd(3, 1, streak, &rng) != 0);

    /* All three lines come up, and nothing else does. */
    for (i = 0; i < 600; ++i) {
        uint16_t s = wm_round_loser_snd(3, 2, streak, &rng);
        if (s == WM_SND_SOMEHOW_I_DONT_THINK) saw[0] = 1;
        else if (s == WM_SND_L_BACK_TO_SANDBOX) saw[1] = 1;
        else if (s == WM_SND_ARE_YOU_TOUGH_ENOUGH) saw[2] = 1;
        else assert(0 && "loser_snd returned something not in the table");
    }
    assert(saw[0] && saw[1] && saw[2]);

    assert(wm_round_loser_snd(3, 2, NULL, &rng) == 0);
}

/* ---- maybe_do_flashes -------------------------------------------- */

static void test_flashes(void) {
    /* reduce_bog kills it outright -- the first thing to go when the
       machine is struggling. */
    assert(!wm_round_maybe_do_flashes((int32_t)0xff900000, 1));

    /* A SIGNED compare against a negative value: it fires only while
       the camera is scrolled above the line. */
    assert(wm_round_maybe_do_flashes((int32_t)0xff900000, 0));
    assert(!wm_round_maybe_do_flashes((int32_t)0xff980000, 0));
    assert(!wm_round_maybe_do_flashes(0, 0));
    /* Exactly the line passes: `JRGT` is strictly greater. */
    assert(wm_round_maybe_do_flashes(WM_FLASHES_Y_LIMIT, 0));
}

int main(void) {
    test_start_table();
    test_start_index();
    test_reset_wrestler();
    test_reset_wrestler2();
    test_update_links();
    test_init_scroller();
    test_most_damage();
    test_loser_snd();
    test_flashes();
    return 0;
}
