/*
 * The music the arcade mutes, and the music it plays.
 *
 * Four places send a wrestler's theme to the DCS board and this port
 * reached none of them; one word of BSS decides when two of the four
 * are allowed to. wm/arcade/wm_arcade_music.h has the whole chain.
 */
#include <assert.h>
#include <string.h>

#include "wm_arcade_music.h"
#include "wm_arcade_match_end.h"
#include "wm_arcade_roster.h"
#include "wm/app.h"
#include "wm/match.h"
#include "wm/pregame.h"

/* ---- ADJMUSIC, and which way round it reads --------------------- */

static void test_adjmusic_factory_value_is_one_and_one_means_off(void) {
    /*
     * AUDIT.ASM:2927 `.word 1 ;ADJMUSIC 17 ;attract mode music = off`.
     * The literal, asserted as a literal: a test that only compared
     * the constant against itself could not catch the port picking the
     * other value, which is exactly what src/core/app.c's comment used
     * to do when it said "this frontend's default is enabled".
     */
    assert(WM_MUSIC_ADJMUSIC_FACTORY == 1);

    /* And the polarity. Non-zero is OFF, so the factory value mutes. */
    assert(!wm_music_attract_allowed(0u, WM_MUSIC_ADJMUSIC_FACTORY));
    assert(wm_music_attract_allowed(0u, 0u));
}

static void test_the_gate_is_two_tests_not_one(void) {
    /*
     * `MOVE @AMODE_LOOPS,A0 / CMPI 2,A0 / JRGE no` then `ADJUST
     * ADJMUSIC / JRNZ no` -- ATTRACT.ASM:3004-3009 and :2294-2299.
     * All four combinations, because the port used to make only the
     * first of the two.
     */
    assert(wm_music_attract_allowed(0u, 0u));
    assert(wm_music_attract_allowed(1u, 0u));
    assert(!wm_music_attract_allowed(2u, 0u));
    assert(!wm_music_attract_allowed(7u, 0u));
    assert(!wm_music_attract_allowed(0u, 1u));
    assert(!wm_music_attract_allowed(1u, 1u));
    assert(!wm_music_attract_allowed(2u, 1u));

    /* `CMPI 2,A0 / JRGE` -- 2 itself is already too late. */
    assert(wm_music_attract_allowed(1u, 0u));
    assert(!wm_music_attract_allowed(2u, 0u));

    /* Any non-zero ADJMUSIC, not just 1. */
    assert(!wm_music_attract_allowed(0u, 5u));
    assert(!wm_music_attract_allowed(0u, 0xffffu));
}

static void test_turn_sounds_off_if_need_is_the_same_gate_inverted(void) {
    /*
     * ATTRACT.ASM:670-675 makes the same two tests in the other order
     * with the branches swapped and reaches the opposite conclusion,
     * so the two must agree exactly for every input.
     */
    unsigned loops;
    uint16_t adj;
    for (loops = 0u; loops < 10u; ++loops)
        for (adj = 0u; adj < 4u; ++adj)
            assert(wm_music_demo_suppresses_sound(loops, adj) ==
                   !wm_music_attract_allowed(loops, adj));

    /* The value it stores is the literal 2 (`MOVK 2,A0` at :677). */
    assert(WM_MUSIC_SOUNDSUP_DEMO == 2);
}

/* ---- SNDSND's own two refusals ----------------------------------- */

static void test_sndsnd_refuses_on_soundsup_and_on_negatives_only(void) {
    /* `move @SOUNDSUP,a0 / jrnz sendx` (DCSSOUND.ASM:2523-2524). */
    assert(!wm_music_board_call_allowed(2u, 1005));
    assert(!wm_music_board_call_allowed(1u, 1005));
    assert(wm_music_board_call_allowed(0u, 1005));

    /* `move a3,a3 / jrn sendx` (:2526-2527) -- NEGATIVE, not zero. */
    assert(!wm_music_board_call_allowed(0u, -1));
    assert(!wm_music_board_call_allowed(0u, -32768));

    /*
     * Zero is a command: `clr a3 / callr SNDSND ; silence the music
     * board` (DCSSOUND.ASM:2466-2467). Conflating it with the null
     * call would silently drop every silence command in the attract.
     */
    assert(wm_music_board_call_allowed(0u, 0));
    /* ...and once SOUNDSUP is set, even that one is dropped, which is
       why nosounds cannot silence a muted board. */
    assert(!wm_music_board_call_allowed(2u, 0));
}

/* ---- the three tables -------------------------------------------- */

static void test_three_tables_two_shapes_one_set_of_numbers(void) {
    static const uint8_t NINE[9] = { 5, 2, 1, 7, 6, 4, 8, 0, 3 };
    static const uint8_t EIGHT[8] = { 5, 2, 1, 7, 6, 4, 8, 3 };
    const uint8_t *t0, *t1, *t2;
    size_t n0 = 0, n1 = 0, n2 = 0;
    size_t i;

    t0 = wm_music_table(0u, &n0);   /* LIFEBAR.ASM:3016 */
    t1 = wm_music_table(1u, &n1);   /* PROGRESS.ASM:2862 */
    t2 = wm_music_table(2u, &n2);   /* ATTRACT.ASM:2817 */
    assert(t0 && t1 && t2);

    /* The shapes, which are the whole reason there are three. */
    assert(n0 == 9u);
    assert(n1 == 9u);
    assert(n2 == 8u);

    /* The literal rows. */
    assert(memcmp(t0, NINE, sizeof NINE) == 0);
    assert(memcmp(t1, NINE, sizeof NINE) == 0);
    assert(memcmp(t2, EIGHT, sizeof EIGHT) == 0);

    /* The two nine-entry tables really are identical -- transcribed
       separately so this is a check, not a restatement. */
    assert(memcmp(t0, t1, n0) == 0);

    /*
     * And the eight-entry one is the nine with slot 7 REMOVED, not
     * zeroed: every row past Adam Bomb has moved down one, which is
     * why reading it with a WRESTLERNUM gives the wrong man's music.
     */
    for (i = 0u; i < 7u; ++i) assert(t2[i] == t0[i]);
    assert(t0[7] == 0);            /* Adam Bomb, cut */
    assert(t2[7] == t0[8]);        /* Lex's theme, one slot earlier */

    /* The accessors, by their source index meanings. */
    assert(wm_music_wrestler_tune(WM_ROSTER_BRET) == 5);
    assert(wm_music_wrestler_tune(WM_ROSTER_RAZOR) == 2);
    assert(wm_music_wrestler_tune(WM_ROSTER_TAKER) == 1);
    assert(wm_music_wrestler_tune(7) == 0);
    assert(wm_music_wrestler_tune(8) == 3);
    assert(wm_music_which_music(8) == 3);
    assert(wm_music_attract_tune(7) == 3);
    /* Which is the point: index 7 means two different wrestlers. */
    assert(wm_music_wrestler_tune(7) != wm_music_attract_tune(7));

    /* Out of range is silence, not a read past the table. */
    assert(wm_music_wrestler_tune(-1) == 0);
    assert(wm_music_wrestler_tune(9) == 0);
    assert(wm_music_which_music(9) == 0);
    assert(wm_music_attract_tune(8) == 0);
    assert(wm_music_table(3u, &n0) == NULL);
    assert(n0 == 0u);

    /* wm_arcade_match_end.h's published name is the same routine. */
    assert(wm_match_wrestler_tune(WM_ROSTER_TAKER) ==
           wm_music_wrestler_tune(WM_ROSTER_TAKER));
}

/* ---- the latch and its 55-tick process --------------------------- */

static void test_the_latch(void) {
    wm_music_state_t m;

    wm_music_init(&m);
    assert(!wm_music_started(&m));
    assert(wm_music_take(&m) == -1);
    assert(m.sends == 0u);

    /* DO_RIGHT_MUSIC2 is the body: MUSIC_HAP set, then the SNDSND. */
    wm_music_do_right_music2(&m, 1);
    assert(wm_music_started(&m));
    assert(m.music_hap == 1u);          /* the source's literal 1 */
    assert(wm_music_take(&m) == 1);
    assert(wm_music_take(&m) == -1);    /* drained once */
    assert(m.sends == 1u);

    /* The three clear sites put the word back to zero and nothing
       else. */
    wm_music_clear(&m);
    assert(!wm_music_started(&m));
    assert(m.music_hap == 0u);

    /* Every entry point survives a NULL. */
    wm_music_init(NULL);
    wm_music_clear(NULL);
    wm_music_do_right_music(NULL, 3);
    wm_music_do_right_music2(NULL, 3);
    wm_music_tick(NULL);
    assert(!wm_music_started(NULL));
    assert(wm_music_take(NULL) == -1);
}

static void test_do_right_music_sleeps_fifty_five_ticks_first(void) {
    wm_music_state_t m;
    int i;

    assert(WM_MUSIC_DO_RIGHT_SLEEP == 55);

    wm_music_init(&m);
    wm_music_do_right_music(&m, 6);
    /* `SLEEP 55` happens BEFORE the body, so nothing is set and
       nothing is sent until it runs out. */
    for (i = 0; i < 54; ++i) {
        wm_music_tick(&m);
        assert(!wm_music_started(&m));
        assert(wm_music_take(&m) == -1);
    }
    wm_music_tick(&m);
    assert(wm_music_started(&m));
    assert(wm_music_take(&m) == 6);
    assert(m.sends == 1u);

    /* And it is one process, not a repeating one. */
    for (i = 0; i < 200; ++i) {
        wm_music_tick(&m);
        assert(wm_music_take(&m) == -1);
    }
    assert(m.sends == 1u);

    /* A tick with nothing armed does nothing at all. */
    wm_music_init(&m);
    for (i = 0; i < 100; ++i) wm_music_tick(&m);
    assert(m.sends == 0u);
    assert(!wm_music_started(&m));
}

static void test_a_skip_inside_the_sleep_sends_the_theme_twice(void) {
    wm_music_state_t m;
    int i;

    /*
     * The race the latch exists for, and the one case it does NOT
     * cover. `#end` arms DO_RIGHT_MUSIC (LIFEBAR.ASM:2933); if the
     * player gets to :2979 in under 55 ticks MUSIC_HAP is still clear,
     * so :2981 creates DO_RIGHT_MUSIC2 and the theme starts now --
     * and nothing kills the first process, which fires anyway when
     * its sleep runs out and sends the same command again.
     */
    wm_music_init(&m);
    wm_music_do_right_music(&m, 1);
    for (i = 0; i < 10; ++i) wm_music_tick(&m);
    assert(!wm_music_started(&m));          /* the sleep is not up */

    wm_music_do_right_music2(&m, 1);        /* the catch-up create */
    assert(wm_music_take(&m) == 1);
    assert(m.sends == 1u);

    for (i = 0; i < 45; ++i) wm_music_tick(&m);
    assert(m.sends == 2u);                  /* the sleep expired too */
    assert(wm_music_take(&m) == 1);

    /*
     * And clearing the latch does not disarm an armed process either
     * -- no source site kills it -- so DO_WAIT's clear leaves a
     * pending send pending.
     */
    wm_music_init(&m);
    wm_music_do_right_music(&m, 4);
    wm_music_clear(&m);
    for (i = 0; i < 55; ++i) wm_music_tick(&m);
    assert(m.sends == 1u);
    assert(wm_music_take(&m) == 4);
}

/* ---- the match-over tail ----------------------------------------- */

static void test_match_end_arms_at_the_award_bar_and_gates_at_the_tip(void) {
    wm_match_end_t st;
    wm_match_streaks_t streaks;
    wm_arcade_match_score_t sc;
    wm_match_end_ctx_t ctx;
    wm_music_state_t music;
    int i;
    bool done = false;

    memset(&streaks, 0, sizeof streaks);
    memset(&sc, 0, sizeof sc);
    sc.p1rounds = 2;
    sc.match_winner = 1;
    wm_music_init(&music);
    /* As if the previous screen had left it set, so the DO_WAIT clear
       has something to do. */
    music.music_hap = 1u;

    memset(&ctx, 0, sizeof ctx);
    ctx.score = &sc;
    ctx.streaks = &streaks;
    ctx.pstatus = 1;
    ctx.winner_wrestler_num = WM_ROSTER_TAKER;
    ctx.awards_done = true;
    ctx.music = &music;

    wm_match_end_init(&st);
    assert(st.tune == -1);
    assert(!st.music_catch_up);
    wm_match_end_start(&st);

    for (i = 0; i < 2000 && !done; ++i) {
        done = wm_match_end_tick(&st, &ctx);
        /* The SOUND_PID process runs beside the sequence, as the app
           runs it beside snd_update. */
        wm_music_tick(&music);
    }
    assert(done);

    /* DO_WAIT's `CLR A0 / MOVE A0,@MUSIC_HAP` really ran -- the 1 it
       started with is gone. */
    assert(st.tune == 1);                  /* the Undertaker's */
    /*
     * The 55-tick sleep armed at `#end` runs out during the award bar
     * and the three-second wait, so MUSIC_HAP is set again by the time
     * the tip phase reads it and the catch-up create is skipped. That
     * is the ordinary case, and it is the one the port could not
     * reach before: there was no first create at all.
     */
    assert(wm_music_started(&music));
    assert(!st.music_catch_up);
    assert(music.sends == 1u);

    /* With no latch at all the tune is still decided and no create is
       claimed, which is what the award-sequence tests want. */
    {
        wm_match_end_t st2;
        memset(&ctx.music, 0, sizeof ctx.music);
        memset(&streaks, 0, sizeof streaks);
        wm_match_end_init(&st2);
        wm_match_end_start(&st2);
        done = false;
        for (i = 0; i < 2000 && !done; ++i)
            done = wm_match_end_tick(&st2, &ctx);
        assert(done);
        assert(st2.tune == 1);
        assert(!st2.music_catch_up);
    }
}

static void test_the_tip_phase_catches_up_when_the_sleep_has_not_run(void) {
    wm_match_end_t st;
    wm_match_streaks_t streaks;
    wm_arcade_match_score_t sc;
    wm_match_end_ctx_t ctx;
    wm_music_state_t music;
    int i;
    bool done = false;

    memset(&streaks, 0, sizeof streaks);
    memset(&sc, 0, sizeof sc);
    sc.p1rounds = 2;
    sc.match_winner = 1;
    wm_music_init(&music);

    memset(&ctx, 0, sizeof ctx);
    ctx.score = &sc;
    ctx.streaks = &streaks;
    ctx.pstatus = 1;
    ctx.winner_wrestler_num = WM_ROSTER_BRET;
    ctx.awards_done = true;
    ctx.music = &music;
    /*
     * Set, as the previous match's own theme would have left it. That
     * makes this test the one that can see DO_WAIT's clear: with the
     * SOUND_PID process never ticked, the only thing that can put
     * MUSIC_HAP back to zero before :2979 reads it is
     * LIFEBAR.ASM:2853-2854, so the catch-up firing at all is the
     * proof that the clear ran.
     */
    music.music_hap = 1u;

    wm_match_end_init(&st);
    wm_match_end_start(&st);
    /*
     * Same sequence, but the SOUND_PID process is NOT ticked -- which
     * is the shape of a player who skipped from `#end` to the tip
     * check faster than 55 ticks. MUSIC_HAP is clear at :2979, so
     * :2981's create fires.
     */
    for (i = 0; i < 2000 && !done; ++i)
        done = wm_match_end_tick(&st, &ctx);
    assert(done);
    assert(st.music_catch_up);
    assert(wm_music_started(&music));
    assert(music.sends == 1u);
    assert(wm_music_take(&music) == 5);     /* Bret's */
}

/* ---- the app: what a factory cabinet actually plays -------------- */

static uint32_t RNG_TICK;
static uint32_t hc(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ffu; }
static uint32_t spf(void *u) {
    uint32_t *t = u;
    return 0x01000000u + ((*t * 7u) & 0x3fffu);
}

/* Run the attract, draining the audio queue, and report whether a
   given command was ever sent and when the first gameplay demo
   suppressed sound. */
typedef struct {
    unsigned n1005;
    unsigned nzero;
    unsigned ntune;           /* commands 1..8, the themes */
    bool suppressed_in_demo;
    bool suppressed_after_reset;
} attract_log;

static void run_attract(wm_app *app, unsigned ticks, attract_log *log) {
    wm_input_state in;
    unsigned t;
    memset(&in, 0, sizeof in);
    memset(log, 0, sizeof *log);
    for (t = 0; t < ticks; ++t) {
        wm_audio_event ev;
        wm_app_tick(app, &in);
        while (wm_audio_pop_event(&app->audio, &ev)) {
            if (ev.command == 1005u) ++log->n1005;
            else if (ev.command == 0u) ++log->nzero;
            else if (ev.command >= 1u && ev.command <= 8u) ++log->ntune;
        }
        if (app->attract.call == WM_ATTRACT_SHOW_GAMEPLAY &&
            app->sound.suppressed)
            log->suppressed_in_demo = true;
        /* The eighth loop's reset clears AMODE_LOOPS and SOUNDSUP
           together (ATTRACT.ASM:268-270). */
        if (app->attract.amode_loops == 0u && log->suppressed_in_demo &&
            !app->sound.suppressed)
            log->suppressed_after_reset = true;
    }
}

static void test_a_factory_cabinet_plays_no_attract_music(void) {
    static wm_app app;
    attract_log log;

    wm_app_init(&app);
    wm_rng_init(&app.rng, 0x12345678u, hc, spf, &RNG_TICK);

    /* The boot values. */
    assert(app.adj_music == WM_MUSIC_ADJMUSIC_FACTORY);
    assert(!app.sound.suppressed);        /* UTIL.ASM:157-158 */
    assert(!wm_music_started(&app.music));

    /*
     * Long enough to reach the eighth loop's reset, which a probe put
     * at tick 65856: suppression starts in the first gameplay demo at
     * tick 2850 and holds until then.
     */
    run_attract(&app, 70000u, &log);

    /*
     * No DCS_LOGO bang and no bio theme, on any loop. The port used to
     * send 1005 on the first two loops of every eight, with the real
     * recovered DCS asset behind it on hardware.
     */
    assert(log.n1005 == 0u);
    assert(log.ntune == 0u);
    /* The silence commands still go out while sound is on, and the
       attract makes at least the startup one. */
    assert(log.nzero >= 1u);
    /* And the gameplay demo muted the board, because ADJMUSIC alone
       is enough -- it did not wait for the third loop. */
    assert(log.suppressed_in_demo);
    /*
     * `clr a0 / move a0,@AMODE_LOOPS / move a0,@SOUNDSUP`
     * (ATTRACT.ASM:268-270): the eighth loop turns sound back on. One
     * register is written to both, and leaving the SOUNDSUP half out
     * would mute a cabinet for ever after its first gameplay demo.
     */
    assert(log.suppressed_after_reset);
}

/*
 * AUDIT.ASM:457-458 `CLR A0 / MOVE A0,@SOUNDSUP`, on the coin drop.
 * Whatever the attract muted, a paying player hears -- which matters
 * because TURN_SOUNDS_OFF_IF_NEED has no "back on" branch of its own
 * and the eighth loop's reset could be an hour away.
 */
static void test_starting_a_game_turns_the_sound_back_on(void) {
    static wm_app app;
    wm_input_state in;
    int i;

    wm_app_init(&app);
    wm_rng_init(&app.rng, 0x12345678u, hc, spf, &RNG_TICK);
    memset(&in, 0, sizeof in);

    /* Far enough for the first gameplay demo to have muted the board,
       and then on to the title where this port's Start bridge waits. */
    for (i = 0; i < 70000; ++i) {
        wm_audio_event ev;
        wm_app_tick(&app, &in);
        while (wm_audio_pop_event(&app.audio, &ev)) {}
        if (app.sound.suppressed &&
            app.attract.call == WM_ATTRACT_SHOW_TITLE &&
            app.attract.call_ticks >= WM_TITLE_BUTTON_ENABLE_TICKS)
            break;
    }
    assert(app.sound.suppressed);
    assert(app.attract.call == WM_ATTRACT_SHOW_TITLE);

    in.start = true;
    wm_app_tick(&app, &in);
    assert(app.mode == WM_APP_MODE_SELECT);
    assert(!app.sound.suppressed);
}

static void test_turning_adjmusic_on_restores_both_sites(void) {
    static wm_app app;
    attract_log log;

    wm_app_init(&app);
    wm_rng_init(&app.rng, 0x12345678u, hc, spf, &RNG_TICK);
    /* What an operator who turns attract music on has done. */
    app.adj_music = 0u;

    run_attract(&app, 30000u, &log);

    /* DCS_LOGO's 1005 and the bio screens' themes both come back. */
    assert(log.n1005 >= 1u);
    assert(log.ntune >= 1u);
}

/*
 * The latch's three clear sites, each watched where only it can act.
 *
 * MUSIC_HAP is one word with three writers and no reader inside the
 * attract at all, so a clear that stopped happening would be invisible
 * until some later screen played a theme it should not have. Each of
 * these mutations survived until this test existed.
 */
static void test_the_three_clear_sites(void) {
    static wm_app app;
    wm_input_state in;
    unsigned t;

    /* ATTRACT.ASM:157, in the run of clears beside AMODE_LOOPS at
       :156. Set before the attract starts, so only the attract's own
       clear can be what zeroes it. */
    wm_app_init(&app);
    wm_rng_init(&app.rng, 0x12345678u, hc, spf, &RNG_TICK);
    memset(&in, 0, sizeof in);
    app.music.music_hap = 1u;
    assert(!app.attract_started);
    for (t = 0; t <= WM_ATTRACT_BOOT_DELAY_TICKS; ++t)
        wm_app_tick(&app, &in);
    assert(app.attract_started);
    assert(!wm_music_started(&app.music));

    /*
     * WRESTLE.ASM:1679, in start_match's own run of clears -- and the
     * pointer that carries it, which wm_app_bind_anim_env assigns just
     * before the attract's gameplay demo starts a match. Set AFTER the
     * attract's clear has already run, so the only thing left that can
     * zero it is the match.
     */
    app.music.music_hap = 1u;
    for (t = 0; t < 70000u; ++t) {
        wm_audio_event ev;
        wm_app_tick(&app, &in);
        while (wm_audio_pop_event(&app.audio, &ev)) {}
        if (app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY &&
            app.match.active)
            break;
    }
    assert(app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY);
    assert(app.match.active);
    assert(app.match.music == &app.music);
    assert(!wm_music_started(&app.music));

    /* LIFEBAR.ASM:2853-2854, DO_WAIT's own -- which
       test_the_tip_phase_catches_up_when_the_sleep_has_not_run watches
       from the other side, by the catch-up create it makes possible. */
}

/* ---- the pregame's theme ----------------------------------------- */

static void test_the_pregame_plays_the_human_s_theme_once(void) {
    static wm_pregame_state s;
    static wm_music_state_t music;
    wm_audio_state audio;
    wm_input_state in;
    wm_audio_event ev;
    WmRng rng;
    int t;
    unsigned found = 0u, progress_sound = 0u, wrong = 0u;

    wm_rng_init(&rng, 0x1234u, hc, spf, &RNG_TICK);
    wm_audio_init(&audio);
    wm_music_init(&music);
    memset(&in, 0, sizeof in);

    /*
     * wm_pregame_init defaults the music index to the player's own,
     * which is the answer whenever PSTATUS's player-one bit is set.
     */
    wm_pregame_init(&s, 2u, WM_WRESTLER_BRET, &rng);
    assert(s.music_source_wrestler == 2u);
    /*
     * ...and this is the other case. PROGRESS.ASM:2699-2704 takes
     * @index2 when that bit is CLEAR, so the theme is player two's
     * wrestler and not the one the rest of the pregame is built
     * around. Source wrestler 8 is Lex, whose theme is
     * WHICH_MUSIC[8] = 3; the player the pregame is drawing is 2,
     * whose theme is 1, so a port that read the wrong field would
     * play the wrong man's music and this test would see 1.
     */
    s.music = &music;
    s.music_source_wrestler = 8u;
    assert(wm_music_which_music(2) == 1);

    for (t = 0; t < 4000 && !s.finished; ++t) {
        wm_pregame_tick(&s, &in, &audio);
        while (wm_audio_pop_event(&audio, &ev)) {
            if (ev.command == 3u) ++found;
            if (ev.command == 1u) ++wrong;
            if (ev.command == 2056u) ++progress_sound;
        }
    }
    /* PROGRESS.ASM:2600's own sound, and then the theme. */
    assert(progress_sound == 1u);
    assert(found == 1u);
    assert(wrong == 0u);

    /*
     * `MOVE @MUSIC_HAP,A0 / JRNZ MUSIC_ALREADY_GOING` -- with the
     * latch set, the previous match's theme is still going and the
     * pregame adds nothing. The progress sound is NOT behind that
     * gate and still plays.
     */
    wm_music_init(&music);
    music.music_hap = 1u;
    wm_pregame_init(&s, 2u, WM_WRESTLER_BRET, &rng);
    s.music = &music;
    s.music_source_wrestler = 8u;
    found = progress_sound = 0u;
    for (t = 0; t < 4000 && !s.finished; ++t) {
        wm_pregame_tick(&s, &in, &audio);
        while (wm_audio_pop_event(&audio, &ev)) {
            if (ev.command == 3u) ++found;
            if (ev.command == 2056u) ++progress_sound;
        }
    }
    assert(progress_sound == 1u);
    assert(found == 0u);
}

static void test_the_next_match_keeps_the_latch_and_the_index(void) {
    static wm_pregame_state s;
    static wm_music_state_t music;
    WmRng rng;

    /*
     * wm_pregame_next_match memsets the whole state and restores a
     * named list. A field the app set once and this list forgot would
     * be silently dropped on the second match of a game, which is
     * where every other restore on that list came from.
     */
    wm_rng_init(&rng, 0x1234u, hc, spf, &RNG_TICK);
    wm_music_init(&music);
    wm_pregame_init(&s, 4u, WM_WRESTLER_BRET, &rng);
    s.music = &music;
    s.music_source_wrestler = 6u;        /* as PSTATUS bit 0 clear would */

    wm_pregame_next_match(&s, 3u);
    assert(s.music == &music);
    assert(s.music_source_wrestler == 6u);
}

/* ---- a live match sends the winner's theme to the board ---------- */

/*
 * The deferred process is a SOUND_PID process and the app is what
 * runs it, beside snd_update and the bell. Nothing tested that until
 * a mutation that deleted the wm_music_tick call from
 * wm_app_tick_dual survived: every other test here ticked the latch
 * itself, so the app could have stopped running it and the theme
 * would simply never have arrived.
 *
 * wm_music_tick and the drain sit above the mode dispatch, so this
 * needs no particular mode -- only a real wm_app_tick.
 */
static void test_the_app_runs_the_deferred_music_process(void) {
    static wm_app app;
    wm_input_state in;
    wm_audio_event ev;
    int i;
    unsigned seen = 0u;

    wm_app_init(&app);
    wm_rng_init(&app.rng, 0x12345678u, hc, spf, &RNG_TICK);
    memset(&in, 0, sizeof in);

    /* As `#end` does. 6 is WRESTLER_TUNES[4]; any legal command would
       serve, and using a real one keeps it recognisable in the log. */
    wm_music_do_right_music(&app.music, 6);

    for (i = 0; i < WM_MUSIC_DO_RIGHT_SLEEP - 1; ++i) {
        wm_app_tick(&app, &in);
        while (wm_audio_pop_event(&app.audio, &ev))
            if (ev.command == 6u) ++seen;
    }
    assert(seen == 0u);                 /* the sleep is not up */
    assert(!wm_music_started(&app.music));

    wm_app_tick(&app, &in);
    while (wm_audio_pop_event(&app.audio, &ev))
        if (ev.command == 6u) ++seen;
    assert(seen == 1u);
    assert(wm_music_started(&app.music));

    /* And it is not re-sent on later ticks. */
    for (i = 0; i < 200; ++i) {
        wm_app_tick(&app, &in);
        while (wm_audio_pop_event(&app.audio, &ev))
            if (ev.command == 6u) ++seen;
    }
    assert(seen == 1u);
}

static void test_a_live_match_ends_with_the_winner_s_theme_sent(void) {
    static wm_app app;
    wm_input_state in;
    wm_audio_event ev;
    int i;
    unsigned theme = 0u;
    int want;

    wm_app_init(&app);
    wm_rng_init(&app.rng, 0x12345678u, hc, spf, &RNG_TICK);
    memset(&in, 0, sizeof in);

    /* Straight into a match, the way test_match_end's live test does,
       but through the app so the SOUND_PID process is ticked and the
       queue is real. */
    app.match.music = &app.music;
    wm_match_start_selected(&app.match, &app.rng, (uint8_t)WM_ROSTER_TAKER);
    want = wm_music_wrestler_tune(WM_ROSTER_TAKER);

    for (i = 0; i < 6000 && app.match.match_end.match_over == 0; ++i) {
        unsigned k;
        for (k = 0; k < app.match.actor_count; ++k)
            if (app.match.actors[k].player_side == 1) {
                app.match.actors[k].player_mode = (uint16_t)WM_PMODE_DEAD;
                app.match.actors[k].life = 0;
            }
        wm_match_tick(&app.match, NULL, &in);
        wm_music_tick(&app.music);
        {
            int tune = wm_music_take(&app.music);
            if (tune >= 0) (void)wm_audio_send_command(&app.audio, (uint16_t)tune);
        }
        while (wm_audio_pop_event(&app.audio, &ev))
            if ((int)ev.command == want) ++theme;
    }
    assert(app.match.match_end.match_over == 2);
    /* Decided at the award bar, and sent -- once. Before this the
       match computed `tune` and nobody played it. */
    assert(app.match.match_end.tune == want);
    assert(theme == 1u);
    assert(app.music.sends == 1u);
}

int main(void) {
    test_adjmusic_factory_value_is_one_and_one_means_off();
    test_the_gate_is_two_tests_not_one();
    test_turn_sounds_off_if_need_is_the_same_gate_inverted();
    test_sndsnd_refuses_on_soundsup_and_on_negatives_only();
    test_three_tables_two_shapes_one_set_of_numbers();
    test_the_latch();
    test_do_right_music_sleeps_fifty_five_ticks_first();
    test_a_skip_inside_the_sleep_sends_the_theme_twice();
    test_match_end_arms_at_the_award_bar_and_gates_at_the_tip();
    test_the_tip_phase_catches_up_when_the_sleep_has_not_run();
    test_a_factory_cabinet_plays_no_attract_music();
    test_starting_a_game_turns_the_sound_back_on();
    test_turning_adjmusic_on_restores_both_sites();
    test_the_three_clear_sites();
    test_the_app_runs_the_deferred_music_process();
    test_the_pregame_plays_the_human_s_theme_once();
    test_the_next_match_keeps_the_latch_and_the_index();
    test_a_live_match_ends_with_the_winner_s_theme_sent();
    return 0;
}
