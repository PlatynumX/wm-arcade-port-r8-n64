/*
 * Running into somebody is an attack, and in this port it was not one.
 *
 * Three separate pieces were each real and none of them joined up.
 *
 * WRESTLE2.ASM:3443 start_run_anim has no WL frames at all: SETMODE,
 * DETACH, an ANI_CODE to #setup_run, END. #setup_run picks the run
 * direction, clears the getup and run timers, enters MODE RUNNING, and
 * ENDS by indexing #run_anims[WRESTLERNUM] and changing to that
 * animation. Every one of those eight run animations opens with
 * `ANI_ATTACK_ON,AMODE_RUN`, and HRTSEQ1.ASM says why in its own words:
 * "I'm turning on an attack box for the entire run sequence. I never
 * turn it off."
 *
 * The port had start_run_anim's program, had all eight run animations'
 * programs, and had REACT5.ASM:127 good_run_hit translated since fix38
 * -- but #setup_run had no ANI_CODE row, so the program ran its three
 * opcodes and stopped. No run animation, no attack box, and AMODE_RUN
 * set by nothing anywhere in the game. Razor was special-cased into
 * calling the state half directly and returning, which put him in MODE
 * RUNNING and then left him standing.
 *
 * And at the other end, wm_arcade_react_callbacks_t::good_run_hit was
 * never assigned by the match, so REACT1.ASM:428's first act --
 * `calla good_run_hit / jrc #good_hit` -- had nothing to call and
 * wm_arcade_wrestler_hit returned WM_WRESTLER_HIT_NEEDS_RUN_HOOK.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/anim_program.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wm_arcade_drone.h"
#include "wm/arcade/wm_arcade_drone_data.h"
#include "wm/arcade/wm_arcade_react.h"
#include "wm/arcade/wm_arcade_react5_core.h"
#include "wm/arcade/wm_arcade_roster_anims.h"
#include "wm/match.h"
#include "wm/wrestler_backend.h"

#define FX16(x) ((int32_t)((x) << 16))

/* WRESTLE2.ASM:3560 #run_anims, transcribed here so a change to the
   extraction has to disagree with the source rather than with itself. */
static const char *const SOURCE_RUN_ANIMS[10] = {
    "hrt_run_anim",     /* 0 Bret Hart */
    "rzr_run_anim",     /* 1 Razor Ramon */
    "und_run_anim",     /* 2 Undertaker */
    "yok_run_anim",     /* 3 Yokozuna */
    "shn_run_anim",     /* 4 Shawn Michaels */
    "bam_run_anim",     /* 5 Bam Bam */
    "dnk_run_anim",     /* 6 Doink */
    "dnk_run_anim",     /* 7 spare -- Doink's, not nothing */
    "lex_run_anim",     /* 8 Lex Luger */
    "dnk_run_anim"      /* 9 Referee -- Doink's again */
};

static void test_the_run_table_matches_the_source(void)
{
    const wm_roster_anim_table *t = wm_roster_anim_find("#run_anims");
    int i;

    assert(t != NULL);
    assert(t->columns == 1);
    assert(t->slots == 10);              /* it really does declare ten */
    assert(strcmp(t->file, "WRESTLE2.ASM") == 0);
    for (i = 0; i < 10; ++i) {
        const char *row = wm_roster_anim_for(t, i);
        assert(row != NULL);
        assert(strcmp(row, SOURCE_RUN_ANIMS[i]) == 0);
    }
    /* Slot 7 is Adam Bomb, cut from the game -- and unlike most of these
       tables this one does NOT leave him empty. */
    assert(strcmp(wm_roster_anim_for(t, 7), "dnk_run_anim") == 0);
    /* Every row names a program that exists, so the become cannot dangle. */
    for (i = 0; i < 10; ++i)
        assert(wm_anim_program_find(SOURCE_RUN_ANIMS[i]) != NULL);
}

/*
 * #setup_run itself: it is translated (wm_anim_code_run finds it), it
 * does the state half, and it names the right animation for each
 * wrestler.
 */
static void test_setup_run_does_the_state_half_and_picks_the_anim(void)
{
    int w;

    for (w = 0; w < 9; ++w) {
        wm_arcade_actor_t a;
        memset(&a, 0, sizeof a);
        a.active = 1;
        a.wrestler_num = w;
        a.in_ring = 1;
        a.facing_dir = WM_MOVE_RIGHT;
        a.new_facing_dir = WM_MOVE_RIGHT;
        a.getup_time = 30;
        a.run_time = 7;

        /* The `a` operand is #setup_run's own definition line; the row is
           registered against the file with no line, so either resolves. */
        assert(wm_anim_code_run(&a, NULL, "#setup_run", "WRESTLE2.ASM",
                                3451));

        assert(a.player_mode == (uint16_t)WM_PMODE_RUNNING);
        assert(a.getup_time == 0);       /* #dorun: "in control" */
        assert(a.run_time == 0);
        assert(a.delay_butns == 1);
        assert(a.change_anim_label != NULL);
        assert(strcmp(a.change_anim_label, SOURCE_RUN_ANIMS[w]) == 0);
    }
}

/*
 * And the whole chain through the shared backend: select start_run_anim
 * the way a dispatcher does, and the wrestler ends up running his own
 * animation with a live AMODE_RUN attack box.
 *
 * Razor (slot 1) is in this loop deliberately: he is the one who used to
 * be special-cased out of it.
 */
static void test_selecting_start_run_turns_on_a_run_attack_box(void)
{
    static const int SLOTS[7] = { 1, 2, 3, 4, 5, 6, 8 };
    unsigned k;

    for (k = 0; k < sizeof SLOTS / sizeof *SLOTS; ++k) {
        wm_arcade_actor_t a;
        wm_wrestler_backend_actor st;
        wm_arcade_roster_callbacks_t cb;
        int w = SLOTS[k];
        int i;

        memset(&a, 0, sizeof a);
        memset(&st, 0, sizeof st);
        a.active = 1;
        a.wrestler_num = w;
        a.in_ring = 1;
        a.life = 100;
        a.facing_dir = WM_MOVE_RIGHT;
        a.new_facing_dir = WM_MOVE_RIGHT;
        st.wrestler_num = w;

        cb = wm_wrestler_roster_callbacks(&st);
        assert(cb.change_anim_label != NULL);
        cb.change_anim_label(&a, "start_run_anim", &st);

        /* The state half lands as soon as the program is selected... */
        assert(a.player_mode == (uint16_t)WM_PMODE_RUNNING);
        /* ...and the program has already asked to become the run anim. */
        assert(st.prog.become != NULL);
        assert(strcmp(st.prog.become, SOURCE_RUN_ANIMS[w]) == 0);

        for (i = 0; i < 3; ++i) {
            st.pcnt = (uint32_t)i;
            wm_wrestler_backend_tick(&st, &a);
        }

        assert(st.current_label != NULL);
        assert(strcmp(st.current_label, SOURCE_RUN_ANIMS[w]) == 0);
        assert(a.attack_mode == WM_AMODE_RUN);
        /* ATTACK_ON sets CHECKHIT too -- without it the box is inert. */
        assert(a.anim_mode & WM_MODE_CHECKHIT);
        assert(a.player_mode == (uint16_t)WM_PMODE_RUNNING);
    }
}

/* ------------------------------------------------------------------ */

static int HEALTH_CALLS;
static void count_health(wm_arcade_actor_t *v, int16_t d,
                         wm_arcade_actor_t *src, void *user)
{
    (void)v; (void)d; (void)src; (void)user;
    ++HEALTH_CALLS;
}

static int REACTION_CALLS;
static wm_arcade_reaction_id_t LAST_REACTION;
static void record_reaction(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                            wm_arcade_reaction_id_t r, int16_t *pending,
                            int16_t *newdir, void *user)
{
    (void)a; (void)v; (void)pending; (void)newdir; (void)user;
    ++REACTION_CALLS;
    LAST_REACTION = r;
}

static wm_arcade_wrestler_hit_result_t run_hit(int attacker_num,
                                               int32_t dz_pixels,
                                               int32_t getup_time,
                                               int with_hook)
{
    wm_arcade_actor_t att, vic;
    wm_arcade_react_callbacks_t cb;
    wm_arcade_combat_runtime_t rt;

    memset(&att, 0, sizeof att);
    memset(&vic, 0, sizeof vic);
    att.active = 1; vic.active = 1;
    att.x_int = 100; vic.x_int = 130;
    att.player_side = 0; vic.player_side = 1;
    att.wrestler_num = attacker_num;
    vic.wrestler_num = 0;
    vic.life = 100;
    att.attack_mode = WM_AMODE_RUN;
    att.getup_time = getup_time;
    att.z_fixed = 0;
    vic.z_fixed = FX16(dz_pixels);

    memset(&cb, 0, sizeof cb);
    cb.adjust_health = count_health;
    cb.reaction = record_reaction;
    if (with_hook) cb.good_run_hit = wm_arcade_react5_good_run_hit_callback;
    memset(&rt, 0, sizeof rt);
    HEALTH_CALLS = 0;
    REACTION_CALLS = 0;
    LAST_REACTION = WM_RXN_HITCHECK;
    return wm_arcade_wrestler_hit(&att, &vic, &rt, &cb);
}

/*
 * REACT5.ASM:127 good_run_hit: "Ignore most running collisions." Two
 * pixels of Z separation for everybody, and -- "FIXX!! Unless it is
 * Yoko!" -- under five for Yokozuna, who is also refused outright while
 * his GETUP_TIME is still running, which the source's own comment says
 * is what stops him gut-hitting you after he has been flung.
 */
static void test_good_run_hit_gates_the_collision(void)
{
    /* Ordinary wrestler: `cmpi 2,a1 / jrgt #bad`, so 2 is good, 3 is not. */
    assert(run_hit(0, 2, 0, 1).status == WM_WRESTLER_HIT_OK);
    /*
     * A good run hit costs no health at all -- DAMAGE.EQU:52 is
     * `D_RUN .equ 0`. What it produces is the REACTION: REACT5.ASM's
     * hit_run, which stops the runner, bounces him off, and makes the
     * victim lose his balance (or, for Yoko, fall back with a gut-push).
     * So the reaction hook is what proves the hit landed, and asserting
     * on damage here would assert on nothing.
     */
    assert(HEALTH_CALLS == 0);
    assert(REACTION_CALLS == 1);
    assert(LAST_REACTION == WM_RXN_RUN);
    assert(run_hit(0, 3, 0, 1).status == WM_WRESTLER_HIT_IGNORED_RUN);
    assert(REACTION_CALLS == 0);
    /* Sign does not matter -- the source takes ABS first. */
    assert(run_hit(0, -2, 0, 1).status == WM_WRESTLER_HIT_OK);
    assert(run_hit(0, -3, 0, 1).status == WM_WRESTLER_HIT_IGNORED_RUN);

    /* Yoko is slot 3: `cmpi 5,a1 / jrlt #good`, so 4 is good and 5 is not. */
    assert(run_hit(3, 4, 0, 1).status == WM_WRESTLER_HIT_OK);
    assert(run_hit(3, 5, 0, 1).status == WM_WRESTLER_HIT_IGNORED_RUN);
    /* ...and a Yoko still getting up is refused at any distance. */
    assert(run_hit(3, 0, 1, 1).status == WM_WRESTLER_HIT_IGNORED_RUN);
    /* That check is Yoko's alone. */
    assert(run_hit(0, 0, 1, 1).status == WM_WRESTLER_HIT_OK);

    /*
     * And with no hook supplied it refuses everything, which is what the
     * whole game did until the match assigned one. Pinned so the old
     * behaviour cannot come back silently.
     */
    assert(run_hit(0, 0, 0, 0).status == WM_WRESTLER_HIT_NEEDS_RUN_HOOK);
    assert(REACTION_CALLS == 0);
}

/*
 * The wiring, end to end: a real match really does produce a wrestler
 * carrying AMODE_RUN, and the hit path really does accept it. Before
 * this change the first of those never happened and the second always
 * returned NEEDS_RUN_HOOK.
 */
static void test_a_live_match_reaches_a_run_attack(void)
{
    wm_match_state m;
    wm_arcade_drone_callbacks_t cb;
    WmRng rng;
    int i;
    size_t k;
    bool saw_run_attack = false;

    memset(&rng, 0, sizeof rng);
    wm_rng_init(&rng, 0x1234u, NULL, NULL, NULL);
    memset(&m, 0, sizeof m);
    wm_match_init(&m);
    wm_match_start_attract(&m, &rng);
    m.anim_rng = &rng;

    cb = wm_arcade_drone_data_callbacks(&rng);
    for (i = 0; i < 600 && !saw_run_attack; ++i) {
        wm_match_tick(&m, &cb, NULL);
        for (k = 0; k < m.actor_count; ++k)
            if (m.actors[k].attack_mode == WM_AMODE_RUN &&
                (m.actors[k].anim_mode & WM_MODE_CHECKHIT))
                saw_run_attack = true;
    }
    assert(saw_run_attack);
}

int main(void)
{
    test_the_run_table_matches_the_source();
    test_setup_run_does_the_state_half_and_picks_the_anim();
    test_selecting_start_run_turns_on_a_run_attack_box();
    test_good_run_hit_gates_the_collision();
    test_a_live_match_reaches_a_run_attack();
    printf("run attack tests passed\n");
    return 0;
}
