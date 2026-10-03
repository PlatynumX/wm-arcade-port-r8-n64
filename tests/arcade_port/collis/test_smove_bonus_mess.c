/*
 * TAKER.ASM:958, inside und_hdhold_pile:
 *
 *      movk    29,a10
 *      CREATE  MESSAGE_PID,BONUS_MESS
 *
 * Twenty-three live pairs of that shape sit in the eight wrestler
 * files -- twenty-two `CREATE MESSAGE_PID,BONUS_MESS` and YOKO.ASM:803's
 * `CREATE0 BONUS_MESS`; a raw grep says twenty-six because three are
 * commented out -- one per special that awards a message. In port
 * terms that is the twenty-two generated head-hold rows whose `bonus`
 * column is not -1, plus SHAWN.ASM:1606 flipslam, which is written by
 * hand. The port read every one of them -- tools/wlsmove.py puts the
 * number in the generated row's
 * `bonus` column, and wm_smove_hdhold_fire hands it back on
 * wm_smove_fire_t -- and then the live loop dropped it on the floor.
 *
 * That mattered because BONUS_MESS is not display. LIFEBAR.ASM:3302
 * sets DAM_MULT to 2 and scores HIGH_RISK_AWD on every path past its
 * first test, so a head-hold special is meant to hit for x1.5 (the
 * table at LIFEBAR.ASM:1451) and to score. Only the words are gated on
 * the first-time flag.
 *
 * The seam was already wired, but only on the REACT1.ASM:550 and
 * ANIM.ASM:2237 paths, and both of those pass A10 = -1 -- the taunt's
 * high risk. No secret move could reach it.
 *
 * So this drives one through a real match and asks for the two
 * non-display effects by name.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/match.h"
#include "wm/award.h"
#include "wm_arcade_roster.h"
#include "wm_arcade_smove.h"

static uint32_t hcount(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ff; }
static uint32_t sp(void *u) { uint32_t *t = u; return 0x01000000u + ((*t * 7u) & 0x3fff); }

static wm_match_state M;

/* What AWARD.ASM's RND_AWARD reaches. A bare match leaves
   anim_round_award NULL -- only app.c binds it -- so the test binds its
   own and counts. */
static int awarded[4][WM_AWARD_COUNT];
static void record_award(void *user, int player_num, unsigned award_index) {
    (void)user;
    if (player_num < 0 || player_num >= 4) return;
    if (award_index >= (unsigned)WM_AWARD_COUNT) return;
    awarded[player_num][award_index]++;
}

/*
 * MODE_HEADHOLD with a victim, re-applied every tick. The Taker's own
 * dispatcher runs mode_headhold while this is set and would otherwise
 * walk him out of it; what is under test is the monitor's fire, not
 * how a real match gets a man into a head hold.
 */
static void hold_head(wm_arcade_actor_t *taker, wm_arcade_actor_t *victim) {
    taker->player_mode = WM_PMODE_HEADHOLD;
    taker->who_i_hit = victim;
    taker->immobilize_time = 0;
    taker->getup_time = 0;
    victim->player_mode = WM_PMODE_HEADHELD;
    victim->who_hit_me = taker;
}

static void tick(wm_input_state *in, wm_arcade_actor_t *t, wm_arcade_actor_t *v) {
    hold_head(t, v);
    wm_match_tick(&M, NULL, in);
}

/*
 * und_hdhold_pile is DOWN, DOWN, strong kick (generated from
 * TAKER.ASM:958 `movk 29,a10` and the CREATE on the line after),
 * bonus 29, and it is the row with the largest
 * message number of the Undertaker's three. Its steps are both plain
 * directions, so nothing here depends on which way he is facing.
 */
static bool run_the_pile_driver(void) {
    WmRng rng;
    uint32_t seed = 1;
    wm_input_state in;
    wm_arcade_actor_t *taker, *victim;
    size_t si;
    int i;
    bool found = false;

    memset(awarded, 0, sizeof awarded);
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    M.anim_round_award = record_award;
    M.anim_award_user = &M;
    wm_rng_init(&rng, 0x0BADF00Du, hcount, sp, &seed);
    wm_match_start_selected(&M, &rng, (uint8_t)WM_ROSTER_TAKER);
    assert(M.actors[0].wrestler_num == WM_ROSTER_TAKER);

    taker = &M.actors[0];
    victim = &M.actors[1];

    /* Let both wrestlers play a frame before anything is done to them
       -- test_smove_live.c's comment explains why the hurt box has to
       exist first. */
    memset(&in, 0, sizeof in);
    for (i = 0; i < 4; ++i) tick(&in, taker, victim);

    for (si = 0; si < M.smove_count[0]; ++si)
        if (M.smoves[0][si].monitor &&
            strcmp(M.smoves[0][si].monitor->name, "und_hdhold_pile") == 0)
            found = true;
    if (!found) return false;

    /*
     * Nothing has scored yet, and DAM_MULT is at its resting value.
     * That value is ONE, not zero (wm_arcade_react.c's runtime init),
     * because adjust_health scales by (delta*(1+mult))>>1 and one is
     * the identity there -- zero would halve every hit. So the
     * assertion this test needs is that BONUS_MESS moves it OFF the
     * identity, which is why the before-value is pinned too.
     */
    assert(awarded[0][WM_AWARD_HIGH_RISK] == 0);
    assert(M.combat_runtime.dam_mult == 1);

    /*
     * DOWN, DOWN, KICK. STICK_REL_NEW only reads on a tick the stick
     * moved, so the two DOWNs need a neutral between them or the
     * second one never registers.
     */
    memset(&in, 0, sizeof in);
    tick(&in, taker, victim);            /* arm */

    in.stick_y = -100;                   /* DOWN */
    tick(&in, taker, victim);
    memset(&in, 0, sizeof in);
    tick(&in, taker, victim);

    in.stick_y = -100;                   /* DOWN again */
    tick(&in, taker, victim);
    memset(&in, 0, sizeof in);
    tick(&in, taker, victim);

    in.power_kick = true;                /* WM_B_SKICK */
    tick(&in, taker, victim);

    return true;
}

int main(void) {
    if (!run_the_pile_driver()) {
        /* The monitor itself not existing is a different failure from
           the one this test is about, and saying so beats an assert
           that reads like the wiring broke. */
        puts("und_hdhold_pile is not among the Undertaker's monitors: FAIL");
        return 1;
    }

    /*
     * `RND_AWARD a8,HIGH_RISK_AWD` at LIFEBAR.ASM:3316 and :3387 --
     * both paths, which is why it is asserted rather than the message.
     */
    assert(awarded[0][WM_AWARD_HIGH_RISK] == 1);
    /* And nobody else scored for it. */
    assert(awarded[1][WM_AWARD_HIGH_RISK] == 0);

    /*
     * `movk 2,a1 / move a1,@DAM_MULT` at LIFEBAR.ASM:3385. Two, not
     * four: four is ANIM.ASM:2234's taunt value on the #tag path,
     * which this one is not. adjust_health then scales the next hit by
     * (delta*(1+mult))>>1 -- x1.5 -- and clears it.
     */
    assert(M.combat_runtime.dam_mult == 2);

    /* The move really did start, so the multiplier is not being
       credited to a monitor that fired nothing. */
    assert(M.actors[0].special_move_addr != 0);

    puts("smove BONUS_MESS wiring tests: PASS");
    return 0;
}
