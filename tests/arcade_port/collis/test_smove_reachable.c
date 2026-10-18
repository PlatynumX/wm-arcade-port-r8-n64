/*
 * The twelve special moves the process-order fix made reachable, and
 * the type collision that was waiting for them.
 *
 * Every monitor carrying WM_SMOVE_G_UNINT -- six charge moves and six
 * free moves -- was refused on its own guard before that fix, because
 * this port ticked the monitors after the wrestler's dispatcher instead
 * of before it. So this is twelve code paths that had never executed.
 *
 * Nine of them worked first time. Bret's three did not, and the reason
 * was one field with two meanings. `SPECIAL_MOVE_ADDR` is where a
 * monitor queues an animation for the wrestler's own process to pick up
 * (WRESTLE.ASM:3843). Seven wrestlers are label-driven, so the label
 * pointer IS the address. Bret is not: his backend selects by a typed
 * id and reads the field back as one. Handing him a pointer made him
 * decode it as an id, and a garbage id looked like a secret move --
 * which latched MODE_UNINT|MODE_NOAUTOFLIP on him with nothing to
 * clear it, so every later charge and free move was refused by its own
 * G_UNINT guard for the rest of the match.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/anim_program.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wm_arcade_roster.h"
#include "wm/arcade/wm_arcade_smove.h"
#include "wm/bret_backend.h"
#include "wm/match.h"

static wm_match_state M;

static uint32_t hc(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ff; }
static uint32_t spf(void *u) { uint32_t *t = u; return 0x01000000u + ((*t * 7u) & 0x3fffu); }

static void start(int wrestler)
{
    WmRng rng;
    static uint32_t t;
    wm_input_state in;
    int i;
    t = 1;
    wm_rng_init(&rng, 0x2468ACE0u, hc, spf, &t);
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    wm_match_start_selected(&M, &rng, (uint8_t)wrestler);
    memset(&in, 0, sizeof in);
    for (i = 0; i < 8; ++i) wm_match_tick(&M, NULL, &in);
}

static bool has_monitor(const char *name)
{
    size_t i;
    for (i = 0; i < M.smove_count[0]; ++i)
        if (M.smoves[0][i].monitor &&
            strcmp(M.smoves[0][i].monitor->name, name) == 0)
            return true;
    return false;
}

/* Did `anim` start, on either backend? Bret's program path leaves
   current_id alone deliberately, and his dispatcher consumes
   special_move_addr in the same tick, so neither alone is a signal. */
static bool started(const char *anim)
{
    const wm_anim_program *p;
    if (M.actors[0].wrestler_num == WM_ROSTER_BRET) {
        int want = wm_bret_anim_id_for_label(anim);
        p = M.bret_visual[0].prog.program;
        if (want >= 0 && (int)M.bret_visual[0].current_id == want) return true;
        return p && p->source_label && strcmp(p->source_label, anim) == 0;
    }
    p = M.wrestler_visual[0].prog.program;
    if (M.wrestler_visual[0].current_label &&
        strcmp(M.wrestler_visual[0].current_label, anim) == 0)
        return true;
    return p && p->source_label && strcmp(p->source_label, anim) == 0;
}

static void press(wm_input_state *in, int which)
{
    if (which == 0) in->light_punch = true;
    else if (which == 1) in->power_punch = true;
    else if (which == 2) in->light_kick = true;
    else in->power_kick = true;
}

/*
 * A charge: one button held past its threshold and then RELEASED. The
 * count goes up while held and the decision happens on the tick the
 * button comes up -- `move *a8(BUT_VAL_CUR),a0 / btst ... / jrz #p1`,
 * with every refusal landing on `#start_over`, which zeroes the count.
 * So a charge that is turned down is spent.
 */
static bool do_charge(int which, int hold)
{
    wm_input_state in;
    int i;
    for (i = 0; i < hold; ++i) {
        memset(&in, 0, sizeof in);
        press(&in, which);
        wm_match_tick(&M, NULL, &in);
    }
    memset(&in, 0, sizeof in);
    for (i = 0; i < 6; ++i) wm_match_tick(&M, NULL, &in);
    return true;
}

/* A three-step sequence, each input held one tick and released the
   next: STICK_REL_NEW only reads on a tick the stick moved. */
static void do_seq(int dy1, int dx1, int dy2, int dx2, int which)
{
    wm_input_state in;
    int i;
    memset(&in, 0, sizeof in);
    in.stick_y = (int8_t)dy1; in.stick_x = (int8_t)dx1;
    wm_match_tick(&M, NULL, &in);
    memset(&in, 0, sizeof in); wm_match_tick(&M, NULL, &in);
    in.stick_y = (int8_t)dy2; in.stick_x = (int8_t)dx2;
    wm_match_tick(&M, NULL, &in);
    memset(&in, 0, sizeof in); wm_match_tick(&M, NULL, &in);
    memset(&in, 0, sizeof in);
    press(&in, which);
    wm_match_tick(&M, NULL, &in);
    memset(&in, 0, sizeof in);
    for (i = 0; i < 6; ++i) wm_match_tick(&M, NULL, &in);
}

/* ---- the six charge moves ---------------------------------------- */

static void charge_case(int wrestler, const char *monitor,
                        const char *anim, int which)
{
    start(wrestler);
    assert(has_monitor(monitor));
    (void)do_charge(which, 110);
    assert(started(anim));
}

static void test_the_six_charge_moves_fire(void)
{
    charge_case(WM_ROSTER_BAM,   "bam_charge_neckbreaker",
                "bam_neckbreaker2_anim", 1);
    charge_case(WM_ROSTER_DOINK, "dnk_charge_flykick",
                "dnk_flying_kick_anim", 3);
    charge_case(WM_ROSTER_BRET,  "hrt_charge_face_rake",
                "hrt_rake_face_anim", 0);
    charge_case(WM_ROSTER_BRET,  "hrt_charge_flying_kick",
                "hrt_flying_kick_anim", 3);
    charge_case(WM_ROSTER_RAZOR, "rzr_charge_slashes",
                "rzr_repeat_slash_anim", 0);
    charge_case(WM_ROSTER_SHAWN, "shn_charge_suplex",
                "shn_gsuplex_anim", 0);
}

/* A charge shorter than its threshold is no charge at all --
   `cmpi 100,a14 / jrlt #start_over`. */
static void test_a_short_press_is_not_a_charge(void)
{
    start(WM_ROSTER_RAZOR);
    assert(has_monitor("rzr_charge_slashes"));
    (void)do_charge(0, 30);
    assert(!started("rzr_repeat_slash_anim"));
}

/* ---- the six free moves ------------------------------------------ */

static void test_the_six_free_moves_fire(void)
{
    /* Bret faces right at the start, so TOWARD is +x and AWAY is -x. */
    start(WM_ROSTER_BRET);
    assert(has_monitor("hrt_roll_uppercut"));
    do_seq(-100, 0, 0, 100, 1);                 /* DOWN, TOWARD, SPUNCH */
    assert(started("hrt_roll_uppercut_anim"));

    start(WM_ROSTER_RAZOR);
    do_seq(0, 100, 0, 100, 2);                  /* TOWARD, TOWARD, KICK */
    assert(started("rzr_sliding_rug_anim"));

    start(WM_ROSTER_SHAWN);
    do_seq(0, 100, 0, 100, 2);
    assert(started("shn_sliding_kicktoss_anim"));

    start(WM_ROSTER_TAKER);
    do_seq(-100, 0, 0, 100, 0);                 /* DOWN, TOWARD, PUNCH */
    assert(started("und_sliding_choke_anim"));

    start(WM_ROSTER_TAKER);
    do_seq(-100, 0, 0, -100, 2);                /* DOWN, AWAY, KICK */
    assert(started("und_spirit_pull_anim"));

    start(WM_ROSTER_TAKER);
    do_seq(-100, 0, 0, 100, 2);                 /* DOWN, TOWARD, KICK */
    assert(started("und_spirit_push_anim"));
}

/* ---- the field with two meanings --------------------------------- */

/*
 * The regression that matters. Before the fix Bret's first charge wrote
 * a label POINTER into a field his own backend reads as a typed id; he
 * decoded it as a garbage id, that looked like a secret move, and
 * MODE_UNINT latched with nothing to clear it. A second charge was then
 * refused by its own G_UNINT guard -- and so was every one after it,
 * for the rest of the match.
 *
 * So: charge twice. The second one is the assertion.
 */
static void test_bret_can_charge_twice(void)
{
    start(WM_ROSTER_BRET);
    (void)do_charge(0, 110);
    assert(started("hrt_rake_face_anim"));

    /* Let whatever is playing finish, then do it again. */
    {
        wm_input_state in;
        int i;
        memset(&in, 0, sizeof in);
        for (i = 0; i < 80; ++i) wm_match_tick(&M, NULL, &in);
    }
    /* MODE_UNINT is not latched: it came off when the animation ended. */
    assert((M.actors[0].anim_mode & WM_MODE_UNINT) == 0);

    (void)do_charge(0, 110);
    assert(started("hrt_rake_face_anim"));
}

/*
 * And the mapping the fix rests on: Bret's label -> id resolver is
 * derived from his own id -> sequence table, so the two cannot drift.
 * A label he has no sequence for answers -1 rather than 0, which
 * matters because 0 is a real id.
 */
static void test_bret_label_to_id_is_honest(void)
{
    int id = wm_bret_anim_id_for_label("hrt_flying_kick_anim");
    assert(id >= 0);
    assert(wm_bret_anim_id_for_label("no_such_anim") == -1);
    assert(wm_bret_anim_id_for_label(NULL) == -1);
    /* start_run_anim has no WL frames of its own and so no sequence to
       be found by; it is named explicitly. */
    assert(wm_bret_anim_id_for_label("start_run_anim") ==
           (int)WM_BRET_ANIM_START_RUN);
}

int main(void)
{
    test_bret_label_to_id_is_honest();
    test_the_six_charge_moves_fire();
    test_a_short_press_is_not_a_charge();
    test_the_six_free_moves_fire();
    test_bret_can_charge_twice();
    printf("reachable special-move tests passed\n");
    return 0;
}
