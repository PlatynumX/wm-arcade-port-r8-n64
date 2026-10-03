/*
 * PAL.ASM's allocator had no owner.
 *
 * pal_getf, pal_clean, pal_set/pal_transfer, pal_find and the global
 * faders were all translated and tested, and the only wm_pal_state
 * anywhere in the tree was a file-static in this directory's
 * test_pal.c. So wm_anim_env's `pal_getf` seam was empty in the live
 * app, wm_pal_getf_by_name -- written as that seam's bridge -- had no
 * caller, and the two routines that resolve a palette by name got zero.
 */
#include <assert.h>
#include <string.h>

#include "wm_arcade_pal.h"
#include "wm_arcade_combat_defs.h"
#include "wm/app.h"
#include "wm/anim_program.h"
#include "wm/match.h"

static uint32_t RNG_TICK;
static uint32_t hc(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ffu; }
static uint32_t spf(void *u) {
    uint32_t *t = u;
    return 0x01000000u + ((*t * 7u) & 0x3fffu);
}

/* ---- the app owns one, and pal_init's DIAGP is in slot 0 --------- */

static void test_the_app_owns_one_with_diagp_as_palette_zero(void) {
    static wm_app app;
    const uint16_t *diagp;
    int slot = -1;

    wm_app_init(&app);
    assert(app.pal != NULL);

    /*
     * `movi DIAGP,a0 / callr pal_getf` at the end of pal_init
     * (PAL.ASM:116-118), whose own comment is "always start with DIAGP
     * as pal 0!". Slot 0 being permanently taken -- pal_clean starts at
     * slot 1 -- is what makes 0 safe as "no palette" for every caller
     * of the by-name bridge.
     */
    diagp = wm_palette_find("DIAGP");
    assert(diagp != NULL);
    assert(wm_pal_find(app.pal, diagp, &slot));
    assert(slot == 0);

    /* And nothing else is claimed yet. */
    assert(!wm_pal_find(app.pal, wm_palette_find("DNKBLU_P"), &slot));
    assert(!wm_pal_find(app.pal, wm_palette_find("BAMBLU_P"), &slot));
}

/* ---- the attract's WIPEOUT re-runs pal_init --------------------- */

static void test_entering_attract_mode_reinitialises_palram(void) {
    static wm_app app;
    wm_input_state in;
    unsigned t;
    int slot = -1;

    wm_app_init(&app);
    wm_rng_init(&app.rng, 0x12345678u, hc, spf, &RNG_TICK);
    memset(&in, 0, sizeof in);

    /* Claim something that pal_init would throw away. */
    assert(wm_pal_getf_by_name(app.pal, "DNKBLU_P") != 0);
    assert(wm_pal_find(app.pal, wm_palette_find("DNKBLU_P"), &slot));

    /*
     * `calla display_blank / calla WIPEOUT` are attract_mode's first two
     * instructions (ATTRACT.ASM:142-143), and WIPEOUT is where
     * `calla pal_init` lives (UTIL.ASM:130).
     */
    assert(!app.attract_started);
    for (t = 0; t <= WM_ATTRACT_BOOT_DELAY_TICKS; ++t)
        wm_app_tick(&app, &in);
    assert(app.attract_started);

    /* DIAGP is back in slot 0 and the rest of PALRAM is empty. */
    assert(wm_pal_find(app.pal, wm_palette_find("DIAGP"), &slot));
    assert(slot == 0);
    assert(!wm_pal_find(app.pal, wm_palette_find("DNKBLU_P"), &slot));
}

/* ---- the transfer queue is drained once a tick ------------------ */

static void test_pal_transfer_drains_the_queue_every_tick(void) {
    static wm_app app;
    wm_input_state in;
    int32_t dma;
    int slot = -1;
    int cell;
    int live;
    const uint16_t *pal;

    wm_app_init(&app);
    wm_rng_init(&app.rng, 0x12345678u, hc, spf, &RNG_TICK);
    memset(&in, 0, sizeof in);

    /*
     * Allocating queues the upload: pal_getf claims a slot only if
     * pal_set found a free transfer CELL, and there are WM_NUMPALT of
     * them. A queue nobody empties fills up, and then every later
     * allocation fails for good -- so draining it is not cosmetic.
     */
    /* pal_init's own DIAGP upload is already sitting in the queue,
       unsent, exactly as it is in the arcade between pal_init and the
       next display interrupt. So count the DELTA. */
    live = 0;
    for (cell = 0; cell < WM_NUMPALT; ++cell)
        if (app.pal->xfer[cell].count != 0u) ++live;
    assert(live == 1);                  /* DIAGP */

    dma = wm_pal_getf_by_name(app.pal, "DNKBLU_P");
    assert(dma != 0);
    assert(wm_pal_find(app.pal, wm_palette_find("DNKBLU_P"), &slot));
    assert(slot == 1);                  /* 0 is DIAGP's, permanently */

    live = 0;
    for (cell = 0; cell < WM_NUMPALT; ++cell)
        if (app.pal->xfer[cell].count != 0u) ++live;
    assert(live == 2);                  /* DIAGP and DNKBLU_P */

    /*
     * `calla pal_transfer ;Copy new PALs` -- MAIN.ASM:719, inside DIRQ
     * (:592), once a frame. The port runs it beside snd_update, which
     * the same interrupt runs.
     */
    wm_app_tick(&app, &in);
    live = 0;
    for (cell = 0; cell < WM_NUMPALT; ++cell)
        if (app.pal->xfer[cell].count != 0u) ++live;
    assert(live == 0);

    /* And the colours really landed in COLRAM at the slot's own
       address, which is `palette << 8 | start colour`. */
    pal = wm_palette_find("DNKBLU_P");
    assert(app.pal->colram[slot * WM_PAL_MAP_STRIDE] == pal[1]);
    /* ...and DIAGP's did on the same pass, at slot 0's own address. */
    pal = wm_palette_find("DIAGP");
    assert(app.pal->colram[0] == pal[1]);

    /*
     * Which also means the allocator does not run itself out. Sixty
     * cells and no drain would stop at sixty; with the drain, many
     * more generations of PALRAM come and go.
     */
    {
        int round;
        for (round = 0; round < WM_NUMPALT * 3; ++round) {
            (void)wm_pal_init_with_diagp(app.pal);
            assert(wm_pal_getf_by_name(app.pal, "BAMBLU_P") != 0);
            wm_app_tick(&app, &in);
        }
        assert(wm_pal_getf_by_name(app.pal, "DNKBLU_P") != 0);
    }
}

/* ---- the seam is filled on a live match ------------------------- */

/* Run the app into the attract's gameplay demo, which is the one place
   a match is started and bound without a select/pregame walkthrough. */
static wm_anim_env *live_env(wm_app *app) {
    wm_input_state in;
    int i;
    memset(&in, 0, sizeof in);
    for (i = 0; i < 70000; ++i) {
        wm_audio_event ev;
        wm_app_tick(app, &in);
        while (wm_audio_pop_event(&app->audio, &ev)) {}
        if (app->attract.call == WM_ATTRACT_SHOW_GAMEPLAY &&
            app->match.active && app->match.actor_count > 0)
            return &app->match.wrestler_visual[0].anim_env;
    }
    return NULL;
}

static void test_a_live_match_fills_the_pal_getf_seam(void) {
    static wm_app app;
    wm_anim_env *env;

    wm_app_init(&app);
    wm_rng_init(&app.rng, 0x12345678u, hc, spf, &RNG_TICK);

    env = live_env(&app);
    assert(env != NULL);
    /* The seam, and the state behind it. Before this both were empty on
       every live path and only tests/test_core.c set either. */
    assert(env->pal_getf != NULL);
    assert(app.match.palettes == app.pal);
    /* Bret's own backend gets it too, not just the generic one. */
    assert(app.match.bret_visual[0].anim_env.pal_getf == env->pal_getf);

    /* And it resolves, out of the app's PALRAM. */
    assert(env->pal_getf(env->screen_user, "DNKBLU_P") != 0);
    assert(env->pal_getf(env->screen_user, "BAMBLU_P") != 0);
    /* A name IMGPAL.ASM does not carry is 0, not a crash. */
    assert(env->pal_getf(env->screen_user, "NO_SUCH_PAL") == 0);
}

/* ---- Doink's buzzer: the whole three-routine swap ---------------- */

static void test_the_buzz_palette_swap_end_to_end(void) {
    static wm_app app;
    wm_anim_env *env;
    wm_arcade_actor_t *a;
    int32_t dnkblu;
    int32_t before;

    wm_app_init(&app);
    wm_rng_init(&app.rng, 0x12345678u, hc, spf, &RNG_TICK);
    env = live_env(&app);
    assert(env != NULL);
    a = &app.match.actors[0];

    a->obj_pal = 42;
    a->my_pal = 0;
    a->skeleton_pal = 0;
    a->status_flags &= (uint32_t)~WM_STATUS_TEMP_PAL;
    before = a->obj_pal;

    /*
     * DNKSEQ3.ASM:476 set_position. ONE definition, `SUBR` and so
     * global, .ref'd by eight other wrestlers' SEQ files -- every
     * wrestler's get_buzz_anim calls it, and it asks for DNKBLU_P in
     * all of them, because the buzzer is Doink's and the victim turns
     * Doink-blue.
     *
     * Its name is a lie: every position write in it is commented out
     * (:486, :489, :492). What is live is the palette pair, and this
     * port had only the MY_PAL half -- the pal_getf at :493 was not
     * attempted at all, so skeleton_pal was read-never-written and a
     * filled seam alone would not have changed that.
     */
    assert(wm_anim_code_run(a, env, "set_position", "DNKSEQ3.ASM", 0));
    dnkblu = wm_pal_getf_by_name(app.pal, "DNKBLU_P");
    assert(dnkblu != 0);
    assert(a->skeleton_pal == dnkblu);   /* `move a0,*a13(SKELETON_PAL)` */
    assert(a->my_pal == before);         /* `move *a13(OBJ_PAL),a0 / MY_PAL` */
    assert(a->obj_pal == before);        /* set_position swaps nothing */

    /* DNKSEQ3.ASM:501 set_skeleton_pal: OBJ_PAL = SKELETON_PAL, and the
       TEMP_PAL bit on. */
    assert(wm_anim_code_run(a, env, "set_skeleton_pal", "DNKSEQ3.ASM", 0));
    assert(a->obj_pal == dnkblu);
    assert((a->status_flags & (uint32_t)WM_STATUS_TEMP_PAL) != 0u);

    /* DNKSEQ3.ASM:512 set_my_pal: back, and the bit off. */
    assert(wm_anim_code_run(a, env, "set_my_pal", "DNKSEQ3.ASM", 0));
    assert(a->obj_pal == before);
    assert((a->status_flags & (uint32_t)WM_STATUS_TEMP_PAL) == 0u);
}

/* ---- Bam Bam burning: the other by-name resolve ----------------- */

static void test_bam_bam_turns_blue_as_he_burns(void) {
    static wm_app app;
    wm_anim_env *env;
    wm_arcade_actor_t *a;
    int32_t bamblu;

    wm_app_init(&app);
    wm_rng_init(&app.rng, 0x12345678u, hc, spf, &RNG_TICK);
    env = live_env(&app);
    assert(env != NULL);
    a = &app.match.actors[0];

    a->obj_pal = 7;
    a->my_pal = 0;
    a->status_flags &= (uint32_t)~WM_STATUS_TEMP_PAL;

    /* BAMSEQ2.ASM:1351 #set_pal: `movi BAMBLU_P,a0 / calla pal_getf`,
       MY_PAL saved, OBJ_PAL swapped, TEMP_PAL set. */
    assert(wm_anim_code_run(a, env, "#set_pal", "BAMSEQ2.ASM", 1351));
    bamblu = wm_pal_getf_by_name(app.pal, "BAMBLU_P");
    assert(bamblu != 0);
    assert(a->my_pal == 7);
    assert(a->obj_pal == bamblu);
    assert((a->status_flags & (uint32_t)WM_STATUS_TEMP_PAL) != 0u);

    /* And the two palettes are different colour maps -- reading one
       table with the other's name would be invisible otherwise. */
    assert(bamblu != wm_pal_getf_by_name(app.pal, "DNKBLU_P"));

    /* BAMSEQ2.ASM:1364 #restore_pal. */
    assert(wm_anim_code_run(a, env, "#restore_pal", "BAMSEQ2.ASM", 1364));
    assert(a->obj_pal == 7);
    assert((a->status_flags & (uint32_t)WM_STATUS_TEMP_PAL) == 0u);
}

/* ---- asking twice reuses the slot ------------------------------- */

static void test_a_second_request_reuses_the_colour_map(void) {
    static wm_app app;
    int32_t first, second;

    wm_app_init(&app);
    /* pal_getf's own first test: `callr pal_find / jrnz` -- a palette
       already in colour ram keeps the slot it has rather than taking a
       second one. Doink can buzz twice in a round. */
    first = wm_pal_getf_by_name(app.pal, "DNKBLU_P");
    second = wm_pal_getf_by_name(app.pal, "DNKBLU_P");
    assert(first != 0);
    assert(first == second);
}

int main(void) {
    test_the_app_owns_one_with_diagp_as_palette_zero();
    test_entering_attract_mode_reinitialises_palram();
    test_pal_transfer_drains_the_queue_every_tick();
    test_a_live_match_fills_the_pal_getf_seam();
    test_the_buzz_palette_swap_end_to_end();
    test_bam_bam_turns_blue_as_he_burns();
    test_a_second_request_reuses_the_colour_map();
    return 0;
}
