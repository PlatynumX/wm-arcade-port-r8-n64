/*
 * The head-hold special-move family: 41 monitors that are one routine
 * with different constants, read out of the source by
 * tools/wlsmove.py.
 */
#include <assert.h>
#include <string.h>

#include "wm_arcade_smove.h"
#include "wm_arcade_roster.h"
#include "wm/anim_program.h"
#include "wm/wrestler_anim_tables.h"

static void test_table_is_sane(void) {
    size_t i;
    int with_reversal = 0, with_bonus = 0, with_pair = 0;

    assert(wm_smove_hdhold_count == 42);

    for (i = 0; i < wm_smove_hdhold_count; ++i) {
        const wm_smove_hdhold_t *r = &wm_smove_hdhold[i];
        size_t k;

        assert(r->name && r->file && r->anim);
        /* Three steps: two stick inputs then a button, which is what
           makes this one family rather than 41 routines. */
        for (k = 0; k < 2; ++k) {
            assert(r->step[k].switches != 0);
            /* A stick step's value is in the low nibble; a button's
               is shifted up four. */
            assert(r->step[k].switches <= 0x0f);
        }
        assert(r->step[2].switches >= WM_B_PUNCH);
        /* Usually J_ALL, but not always: BRET.ASM:857's
           hrt_hdhold_combo2 masks only J_DOWN_TOWARD|J_UP_TOWARD, so
           a diagonal held into the button does not spoil it. Reading
           that as J_ALL would have been a transcription error a hand
           copy could not catch. */
        assert(r->step[2].mask != 0);
        /* Almost all of them use a 60-tick window; SHAWN.ASM:780's
           shn_flipslam is the one that does not, at 30. A hand copy
           would very likely have written 60 for all 41. */
        assert(r->timeout == 60 || r->timeout == 30);
        assert(r->bonus >= -1);
        if (r->reversal) ++with_reversal;
        if (r->bonus >= 0) ++with_bonus;
        if (r->anim_flipped) ++with_pair;
    }

    /* All three columns are really used -- a table where every row
       agreed would mean the extractor was reading a constant. */
    {
        int short_window = 0;
        for (i = 0; i < wm_smove_hdhold_count; ++i)
            if (wm_smove_hdhold[i].timeout == 30) ++short_window;
        assert(short_window == 1);
    }
    assert(with_reversal > 0 && with_reversal < (int)wm_smove_hdhold_count);
    assert(with_bonus > 0 && with_bonus < (int)wm_smove_hdhold_count);

    /* The two extra gates, counted: fifteen combo moves need a combo
       running and four of Shawn's need him not getting up. */
    {
        int combo = 0, up = 0;
        for (i = 0; i < wm_smove_hdhold_count; ++i) {
            if (wm_smove_hdhold[i].needs_combo) ++combo;
            if (wm_smove_hdhold[i].needs_getup_clear) ++up;
        }
        /*
         * Fifteen, not sixteen. There are sixteen `*_hdhold_combo1/2`
         * moves -- two per wrestler across all eight -- and fifteen
         * of them are gated on a combo running. SHAWN.ASM:1538 has
         * its `calla CHECK_COMBO_GO / jrlt #lp0` COMMENTED OUT, so
         * shn_hdhold_combo1 is available without one in the shipped
         * game. Reading the raw text instead of the stripped text
         * gives sixteen and is wrong.
         */
        assert(combo == 15);
        /* Five refuse while getting up, all of them Shawn's.
           YOKO.ASM:759 has the same test commented out for two of
           Yokozuna's, which is the other half of the same trap. */
        assert(up == 5);
    }
}

/* The animations they name have to exist, or a move fires into
   nothing. */
static void test_animations_resolve(void) {
    size_t i;
    for (i = 0; i < wm_smove_hdhold_count; ++i) {
        const wm_smove_hdhold_t *r = &wm_smove_hdhold[i];
        assert(wm_anim_program_find(r->anim));
        if (r->anim_flipped) assert(wm_anim_program_find(r->anim_flipped));
    }
}

/* Spot-check one row against the source by hand: TAKER.ASM:748
   und_hdhold_neckbrk is TOWARD, TOWARD, SUPER PUNCH, bonus 1, with a
   reversal, into und_neckbreaker_anim. */
static void test_one_row_by_hand(void) {
    const wm_smove_monitor_t *m = wm_smove_monitor_find("und_hdhold_neckbrk");
    const wm_smove_hdhold_t *r;
    assert(m && m->hdhold);
    r = m->hdhold;
    assert(r->step[0].switches == WM_J_TOWARD && r->step[0].mask == 0);
    assert(r->step[1].switches == WM_J_TOWARD && r->step[1].mask == 0);
    assert(r->step[2].switches == WM_B_SPUNCH);
    assert(r->step[2].mask == WM_J_ALL);
    assert(r->timeout == 60);
    assert(r->bonus == 1);
    assert(r->reversal);
    assert(strcmp(r->anim, "und_neckbreaker_anim") == 0);
    /* It loops rather than dying: SLEEPK 20 then `jruc #lp`, so a
       head hold can be turned into move after move. */
    assert(!m->one_shot);
}

/* The shared tail: who the move belongs to is decided by PLYRMODE at
   the moment the sequence completes, not by who started the hold. */
static void test_fire(void) {
    const wm_smove_monitor_t *m = wm_smove_monitor_find("und_hdhold_neckbrk");
    wm_arcade_actor_t me, him;
    wm_arcade_actor_t *victim;
    wm_smove_fire_t out;

    assert(m && m->hdhold);
    memset(&me, 0, sizeof me);
    memset(&him, 0, sizeof him);
    me.who_i_hit = &him;
    me.who_hit_me = &him;
    me.facing_dir = WM_MOVE_RIGHT;

    /* Neither mode: nothing. */
    me.player_mode = WM_PMODE_NORMAL;
    assert(wm_smove_hdhold_fire(m->hdhold, &me, &victim, &out)
           == WM_SMOVE_HH_NOTHING);

    /* I have him: the move is mine, WHOIHIT is the target, and the
       bonus message goes up. */
    me.player_mode = WM_PMODE_HEADHOLD;
    assert(wm_smove_hdhold_fire(m->hdhold, &me, &victim, &out)
           == WM_SMOVE_HH_SLAM);
    assert(victim == &him);
    assert(me.smart_target == &him);
    assert(strcmp(out.anim, "und_neckbreaker_anim") == 0);
    assert(out.bonus == 1);
    assert(out.victim_immobilize == 15);

    /* He has me: the SAME input is a reversal, targeting WHOHITME,
       and it awards no bonus message -- DO_REVERSAL_MESS is a
       different message. */
    me.player_mode = WM_PMODE_HEADHELD;
    assert(wm_smove_hdhold_fire(m->hdhold, &me, &victim, &out)
           == WM_SMOVE_HH_REVERSAL);
    assert(victim == &him);
    assert(out.bonus == -1);

    /* A man on his way out cannot reverse -- and that guard is on the
       reversal path only, not on the slam. */
    me.i_will_die = 1;
    assert(wm_smove_hdhold_fire(m->hdhold, &me, &victim, &out)
           == WM_SMOVE_HH_NOTHING);
    me.player_mode = WM_PMODE_HEADHOLD;
    assert(wm_smove_hdhold_fire(m->hdhold, &me, &victim, &out)
           == WM_SMOVE_HH_SLAM);
    me.i_will_die = 0;

    /* Immobilised refuses either way -- "ignore", says the source. */
    me.immobilize_time = 5;
    assert(wm_smove_hdhold_fire(m->hdhold, &me, &victim, &out)
           == WM_SMOVE_HH_NOTHING);
    me.immobilize_time = 0;

    /* A move with no reversal path declines when he is the one held. */
    {
        size_t i;
        const wm_smove_hdhold_t *plain = NULL;
        for (i = 0; i < wm_smove_hdhold_count; ++i)
            if (!wm_smove_hdhold[i].reversal) {
                plain = &wm_smove_hdhold[i];
                break;
            }
        assert(plain);
        me.player_mode = WM_PMODE_HEADHELD;
        assert(wm_smove_hdhold_fire(plain, &me, &victim, &out)
               == WM_SMOVE_HH_NOTHING);
    }
}

/* Driven through the sequence engine end to end. */
static void test_sequence(void) {
    const wm_smove_monitor_t *m = wm_smove_monitor_find("und_hdhold_neckbrk");
    wm_arcade_actor_t me, him;
    wm_smove_run_t run;
    wm_smove_env_t env;
    wm_smove_fire_t out;

    memset(&me, 0, sizeof me);
    memset(&him, 0, sizeof him);
    memset(&run, 0, sizeof run);
    memset(&env, 0, sizeof env);
    me.who_i_hit = &him;
    me.facing_dir = WM_MOVE_RIGHT;
    me.player_mode = WM_PMODE_HEADHOLD;
    run.monitor = m;

    /* Not in a hold: the gate never arms it. */
    me.player_mode = WM_PMODE_NORMAL;
    assert(!wm_smove_tick(&run, &me, &env, &out));
    assert(!run.armed);

    me.player_mode = WM_PMODE_HEADHOLD;
    assert(!wm_smove_tick(&run, &me, &env, &out));   /* the SLEEPK */
    assert(run.armed);

    me.but_val_down = 0; me.stick_rel_new = WM_J_TOWARD;
    assert(!wm_smove_tick(&run, &me, &env, &out));
    assert(run.countdown == 60);
    assert(!wm_smove_tick(&run, &me, &env, &out));   /* second TOWARD */
    me.but_val_down = WM_BTN_SPUNCH; me.stick_rel_new = 0;
    assert(wm_smove_tick(&run, &me, &env, &out));

    assert(out.anim && strcmp(out.anim, "und_neckbreaker_anim") == 0);
    assert(out.bonus == 1);
    /* It loops: still alive, ready for the next one. */
    assert(!run.dead);
}

/* init_smoves now makes far more of a wrestler's table. */
static void test_init_covers_more(void) {
    wm_smove_run_t runs[WM_SMOVE_MAX_PER_WRESTLER];
    size_t n, missing;
    int w;
    size_t total_made = 0, total_missing = 0;

    for (w = 0; w < WM_WRESTLER_ANIM_SLOTS; ++w) {
        n = wm_smove_init(w, false, runs, WM_SMOVE_MAX_PER_WRESTLER,
                          &missing);
        total_made += n;
        total_missing += missing;
        assert(n + missing == (size_t)wm_wrestler_smoves[w].count);
    }
    /* 79 table entries across the roster; the ones still missing are
       the six charge monitors, the eight grab_toss_air, and a handful
       of one-offs. */
    assert(total_made + total_missing == 79);
    assert(total_made > 50);
}

/* The two extra gates really refuse. */
static void test_extra_gates(void) {
    const wm_smove_monitor_t *combo = NULL, *up = NULL;
    size_t i;
    wm_arcade_actor_t a;
    wm_smove_env_t env;
    wm_smove_run_t run;
    wm_smove_fire_t out;

    for (i = 0; i < wm_smove_hdhold_count; ++i) {
        if (!combo && wm_smove_hdhold[i].needs_combo)
            combo = wm_smove_monitor_find(wm_smove_hdhold[i].name);
        if (!up && wm_smove_hdhold[i].needs_getup_clear)
            up = wm_smove_monitor_find(wm_smove_hdhold[i].name);
    }
    assert(combo && up);

    memset(&env, 0, sizeof env);

    /* A combo move will not arm with no combo running. */
    memset(&a, 0, sizeof a);
    memset(&run, 0, sizeof run);
    a.player_mode = WM_PMODE_HEADHOLD;
    run.monitor = combo;
    a.combo_count = 0;
    assert(!wm_smove_tick(&run, &a, &env, &out));
    assert(!run.armed);
    a.combo_count = 3;
    assert(!wm_smove_tick(&run, &a, &env, &out));
    assert(run.armed);

    /* And Shawn's four will not arm while he is getting up. */
    memset(&a, 0, sizeof a);
    memset(&run, 0, sizeof run);
    a.player_mode = WM_PMODE_HEADHOLD;
    run.monitor = up;
    a.getup_time = 4;
    assert(!wm_smove_tick(&run, &a, &env, &out));
    assert(!run.armed);
    a.getup_time = 0;
    assert(!wm_smove_tick(&run, &a, &env, &out));
    assert(run.armed);
}

int main(void) {
    test_table_is_sane();
    test_animations_resolve();
    test_one_row_by_hand();
    test_fire();
    test_extra_gates();
    test_sequence();
    test_init_covers_more();
    return 0;
}
