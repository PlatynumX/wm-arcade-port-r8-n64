/*
 * Two results a translated routine computed and its caller threw away.
 *
 * wm_smove_fire_t lost its BONUS_MESS number that way, and the shape is
 * general: a routine that cannot perform a side effect reports it on a
 * struct, and the report is only worth as much as the caller reading it.
 * A sweep of every `*_result_t` in include/wm found thirty fields no
 * consumer reads. Most are genuine observations. These two were not.
 *
 * ONE: the Undertaker's choke loop.
 *
 * TAKER.ASM:3110 mode_choking holds the victim while the attachment is
 * mutual and lets go the moment it is not -- and its #fall_out arm runs
 * `CALLA FIND_AND_KILL_ENDLESS` (DCSSOUND.ASM:4223) before SETMODE
 * NORMAL. Both backends called the routine as `(void)`, under a comment
 * saying this backend "has no sound queue to send it to". It needs none:
 * the routine takes no arguments, and the port has had it since the
 * animation opcodes landed. So the choke sound went on looping after the
 * victim was loose, and nothing else on that path clears the global.
 *
 * TWO: the buckoff's two animations.
 *
 * DOINK.ASM #dobuck ends with `FACETBL hitonground_tbl / calla
 * change_anim1a` on the man who mashed his way back to life, and
 * `FACETBL #buckoff_tbl / move a0,*a8(SPECIAL_MOVE_ADDR),L` on whoever
 * had pinned him. match.c reported both and played neither, on two
 * grounds that had since stopped being true: "this port has no
 * hitonground table for every wrestler" and "only some of those
 * animations are extracted". Both tables are in the generated registry
 * and all sixteen animations exist, which is what the second half of
 * this file measures -- the data the wiring now depends on.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/match.h"
#include "wm/anim_program.h"
#include "wm_arcade_roster.h"
#include "wm_arcade_roster_anims.h"

static uint32_t hcount(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ff; }
static uint32_t sp(void *u) { uint32_t *t = u; return 0x01000000u + ((*t * 7u) & 0x3fff); }

static wm_match_state M;

/*
 * A choke in progress, then the choker lets go.
 *
 * The victim is put in MODE_CHOKING pointing at the Undertaker, who does
 * NOT point back -- which is exactly the state mode_choking's #fall_out
 * exists for, and the one a real match reaches when the choker is hit,
 * dies, or is pulled away mid-hold.
 */
static void test_the_choke_loop_stops_when_the_victim_falls_out(void) {
    WmRng rng;
    uint32_t seed = 1;
    wm_input_state in;
    wm_anim_env env;
    wm_arcade_actor_t *taker, *victim;
    int i;

    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    wm_rng_init(&rng, 0x5EED1234u, hcount, sp, &seed);
    wm_match_start_selected(&M, &rng, (uint8_t)WM_ROSTER_TAKER);
    taker = &M.actors[0];
    victim = &M.actors[1];

    memset(&in, 0, sizeof in);
    for (i = 0; i < 4; ++i) wm_match_tick(&M, NULL, &in);

    /*
     * DCSSOUND.ASM's DO_CHOKE is what starts it, and it is the same
     * global either way -- so the sound is started the way the game
     * starts it rather than poked.
     */
    memset(&env, 0, sizeof env);
    assert(wm_anim_code_run(taker, &env, "DO_CHOKE", NULL, 0));
    assert(wm_anim_code_endless_sound() != 0u);

    /* The hold, one-sided: he is choking, the choker is not attached. */
    victim->player_mode = WM_PMODE_CHOKING;
    victim->attach_proc = taker;
    taker->attach_proc = NULL;

    wm_match_tick(&M, NULL, &in);

    /* #fall_out ran: loose, MODE_NORMAL, every animation flag cleared. */
    assert(victim->attach_proc == NULL);
    /*
     * And the sound stopped. This is the assertion the whole file is
     * for: wm_arcade_mode_choking reported it before and still does,
     * and before this the report went nowhere.
     */
    assert(wm_anim_code_endless_sound() == 0u);

    puts("  choke loop stops when the victim falls out: ok");
}

/*
 * The two tables #dobuck dispatches through, and every animation they
 * name. The old refusal was a claim about this data; this is the claim,
 * measured.
 *
 * FACETBL (MACROS.H:102) is one long per wrestler, so there is no facing
 * column here despite the macro's name. hitonground_tbl declared ten
 * rows and #buckoff_tbl nine; slots 7 and 9 are the cut Adam Bomb and
 * the Referee and are `.long 0` in the source, so a NULL there is the
 * data being right rather than missing.
 */
static void test_the_buckoff_tables_and_every_animation_they_name(void) {
    static const int real[] = { 0, 1, 2, 3, 4, 5, 6, 8 };   /* not 7, not 9 */
    const wm_roster_anim_table *hog = wm_roster_anim_find("hitonground_tbl");
    const wm_roster_anim_table *bck = wm_roster_anim_find("#buckoff_tbl");
    size_t k;

    assert(hog && bck);
    /* REACT1.ASM:1856 and DOINK.ASM:3235, and the row widths that make
       them FACETBL tables rather than facing pairs. */
    assert(hog->slots == 10 && hog->columns == 1);
    assert(bck->slots == 9 && bck->columns == 1);
    assert(hog->kind == WM_ROSTER_COL_SLOT);
    assert(bck->kind == WM_ROSTER_COL_SLOT);

    for (k = 0; k < sizeof real / sizeof real[0]; ++k) {
        int w = real[k];
        const char *h = wm_roster_anim_for(hog, w);
        const char *b = wm_roster_anim_for(bck, w);
        /* Both tables name an animation for all eight real wrestlers... */
        assert(h && b);
        /* ...and every one of those sixteen is a program this port can
           actually start. A label that resolves to nothing would make
           the wiring compile, run, and do nothing -- which is the
           failure the refusal was avoiding, so it is the thing to
           check rather than assume. */
        assert(wm_anim_program_find(h));
        assert(wm_anim_program_find(b));
    }

    /* The cut slots really are empty, in the table that carries them. */
    assert(wm_roster_anim_for(hog, WM_ROSTER_ANIM_ADAM_BOMB) == NULL);
    assert(wm_roster_anim_for(hog, WM_ROSTER_ANIM_REFEREE) == NULL);

    puts("  buckoff tables and all sixteen animations resolve: ok");
}

/*
 * The gate crash's two, WRESTLE.ASM:3695 -- the third of the three, and
 * the one that shows why the shape matters: everything else about
 * running into the fence was applied (velocities, MODE_NORMAL, RUN_TIME,
 * damage, the 0c5h crash sound) and the wrestler kept playing his run.
 *
 * These two tables are deliberately different shapes, which is why the
 * source uses two macros for them. Getting that wrong would read one
 * long where there are two and hand out the wrong column, so the shape
 * is asserted rather than assumed.
 */
static void test_the_gate_crash_tables_are_two_different_shapes(void) {
    static const int real[] = { 0, 1, 2, 3, 4, 5, 6, 8 };
    const wm_roster_anim_table *fb = wm_roster_anim_find("fall_back_tbl");
    const wm_roster_anim_table *bg = wm_roster_anim_find("bncoff_gate");
    size_t k;

    assert(fb && bg);
    /* `FACETBL fall_back_tbl` -- one long per wrestler, nine rows. */
    assert(fb->slots == 9 && fb->columns == 1);
    assert(fb->kind == WM_ROSTER_COL_SLOT);
    /* `FACE24TBL bncoff_gate` -- two longs per wrestler, ten rows. */
    assert(bg->slots == 10 && bg->columns == 2);
    assert(bg->kind == WM_ROSTER_COL_FACING);

    for (k = 0; k < sizeof real / sizeof real[0]; ++k) {
        int w = real[k];
        const char *f = wm_roster_anim_for(fb, w);
        const char *up = wm_roster_anim_facing(bg, w, 1);
        const char *dn = wm_roster_anim_facing(bg, w, 0);
        assert(f && wm_anim_program_find(f));
        /* Both facing columns, because FACE24TBL picks between them and a
           NULL in one would be a crash into the fence that plays nothing
           half the time. */
        assert(up && wm_anim_program_find(up));
        assert(dn && wm_anim_program_find(dn));
    }

    /*
     * And the facing rule is the one FACE24TBL really applies: column 0
     * when MOVE_UP_BIT is SET. MACROS.H:65 tests the bit and adds a long
     * when it is CLEAR, so the up animation is first -- the inversion
     * that was wrong in six dispatchers once.
     */
    assert(wm_roster_anim_facing(bg, 0, 1) == wm_roster_anim_col(bg, 0, 0));
    assert(wm_roster_anim_facing(bg, 0, 0) == wm_roster_anim_col(bg, 0, 1));

    puts("  gate crash tables, both shapes and both columns: ok");
}

int main(void) {
    test_the_choke_loop_stops_when_the_victim_falls_out();
    test_the_buckoff_tables_and_every_animation_they_name();
    test_the_gate_crash_tables_are_two_different_shapes();
    puts("dropped result field tests: PASS");
    return 0;
}
