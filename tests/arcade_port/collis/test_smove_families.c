/*
 * The three special-move families the head-hold reader refuses, and
 * the two monitors that are neither: tools/wlsmove.py reads sixty of
 * the sixty-two out of the source, and these are the eighteen that
 * are not head holds plus the two with a third outcome.
 */
#include <assert.h>
#include <string.h>

#include "wm_arcade_smove.h"
#include "wm_arcade_roster.h"
#include "wm/anim_program.h"
#include "wm/arcade/wm_arcade_damage.h"
#include "wm/wrestler_anim_tables.h"

/* ---- every animation any family names has to exist -------------- */

static void test_animations_resolve(void) {
    size_t i;
    for (i = 0; i < wm_smove_charge_count; ++i) {
        assert(wm_anim_program_find(wm_smove_charge[i].anim));
        if (wm_smove_charge[i].anim_running)
            assert(wm_anim_program_find(wm_smove_charge[i].anim_running));
    }
    for (i = 0; i < wm_smove_grab_count; ++i) {
        const wm_smove_grab_t *r = &wm_smove_grab[i];
        assert(wm_anim_program_find(r->near_anim));
        assert(wm_anim_program_find(r->air_anim));
        if (r->near_anim_flipped)
            assert(wm_anim_program_find(r->near_anim_flipped));
        if (r->air_anim_flipped)
            assert(wm_anim_program_find(r->air_anim_flipped));
    }
    for (i = 0; i < wm_smove_free_count; ++i)
        assert(wm_anim_program_find(wm_smove_free[i].anim));
    assert(wm_anim_program_find("shn_flipslam_anim"));
    assert(wm_anim_program_find("shn_super_speedkick_anim"));
    assert(wm_anim_program_find("shn_flying_kick_anim"));
}

/* ---- the charge family ------------------------------------------ */

static void test_charge_table(void) {
    size_t i;
    int running = 0, inair = 0, with_opp = 0, with_ignore = 0;

    assert(wm_smove_charge_count == 6);
    for (i = 0; i < wm_smove_charge_count; ++i) {
        const wm_smove_charge_t *r = &wm_smove_charge[i];
        size_t g;
        assert(r->name && r->file && r->anim);
        /* All six charge to 100. That they agree is a reading, not an
           assumption: the column is in the table. */
        assert(r->threshold == 100);
        assert(r->button == WM_BTN_PUNCH || r->button == WM_BTN_SPUNCH ||
               r->button == WM_BTN_SKICK);
        assert(r->guards > 0 && r->guards <= WM_SMOVE_MAX_GUARDS);
        if (r->anim_running) ++running;
        if (r->set_mode == WM_PMODE_INAIR) ++inair;
        for (g = 0; g < r->guards; ++g) {
            if (r->guard[g].kind == WM_SMOVE_G_OPP_MODE) { ++with_opp; break; }
        }
        for (g = 0; g < r->guards; ++g) {
            if (r->guard[g].kind == WM_SMOVE_G_CK_IGNORE) {
                ++with_ignore;
                break;
            }
        }
    }
    /* Shawn's suplex and Bam Bam's neckbreaker have a running form;
       the two flying kicks go into the air; the same two are the ones
       that check the opponent's mode and call ck_ignore_a8. Four
       columns that would all read the same if the guard lists were
       assumed rather than read. */
    assert(running == 2);
    assert(inair == 2);
    assert(with_opp == 2);
    assert(with_ignore == 2);
}

/* BRET.ASM:543, by hand: hold SUPER KICK for 100 ticks, release, and
   the mode guards decide. */
static void test_charge_runs(void) {
    const wm_smove_monitor_t *m =
        wm_smove_monitor_find("hrt_charge_flying_kick");
    wm_arcade_actor_t me, him;
    wm_smove_env_t env;
    wm_smove_run_t run;
    wm_smove_fire_t out;
    int t;

    assert(m && m->charge && m->steps == 0);
    assert(m->charge->button == WM_BTN_SKICK);

    memset(&me, 0, sizeof me);
    memset(&him, 0, sizeof him);
    memset(&env, 0, sizeof env);
    memset(&run, 0, sizeof run);
    run.monitor = m;
    env.closest = &him;
    /* Facing his opponent, so ck_ignore_a8 does not refuse. */
    me.facing_dir = WM_MOVE_RIGHT;
    me.new_facing_dir = WM_MOVE_RIGHT;
    me.move_dir = 0;

    /*
     * The counter goes up BEFORE the button is read -- `inc a14 /
     * move a14,*a13(#CHARGE_TIME) / move *a8(BUT_VAL_CUR),a0 / btst
     * ... / jrz #p1` -- so the tick the button comes UP on is counted
     * too, and ninety-eight held ticks plus the release is ninety-
     * nine. Short of the hundred, and the count starts again from
     * zero rather than banking what it had.
     */
    me.but_val_cur = WM_BTN_SKICK;
    for (t = 0; t < 98; ++t)
        assert(!wm_smove_tick(&run, &me, &env, &out));
    me.but_val_cur = 0;
    assert(!wm_smove_tick(&run, &me, &env, &out));
    assert(run.countdown == 0);

    /* One more tick of holding is the hundred. */
    me.but_val_cur = WM_BTN_SKICK;
    for (t = 0; t < 99; ++t)
        assert(!wm_smove_tick(&run, &me, &env, &out));
    me.but_val_cur = 0;
    assert(wm_smove_tick(&run, &me, &env, &out));
    assert(strcmp(out.anim, "hrt_flying_kick_anim") == 0);
    /* SETMODE INAIR is part of the move. */
    assert(me.player_mode == WM_PMODE_INAIR);
}

static void test_charge_guards_refuse(void) {
    const wm_smove_monitor_t *m =
        wm_smove_monitor_find("hrt_charge_flying_kick");
    wm_arcade_actor_t me, him;
    wm_smove_env_t env;
    wm_smove_fire_t out;
    int32_t held;

    memset(&him, 0, sizeof him);
    memset(&env, 0, sizeof env);
    env.closest = &him;

    /* A downed opponent refuses it: `move *a8(CLOSEST_NUM),a0 / ... /
       cmpi MODE_ONGROUND,a0 / jrz #start_over`. This is the guard the
       face-rake charge does NOT have, which is why the list is data. */
    memset(&me, 0, sizeof me);
    me.facing_dir = WM_MOVE_RIGHT;
    me.new_facing_dir = WM_MOVE_RIGHT;
    held = 100;
    him.player_mode = WM_PMODE_ONGROUND;
    assert(!wm_smove_charge_tick(m->charge, &held, &me, &env, &out));
    him.player_mode = WM_PMODE_NORMAL;

    /* And getting up refuses it. */
    memset(&me, 0, sizeof me);
    me.facing_dir = WM_MOVE_RIGHT;
    me.new_facing_dir = WM_MOVE_RIGHT;
    me.getup_time = 3;
    held = 100;
    assert(!wm_smove_charge_tick(m->charge, &held, &me, &env, &out));

    /* The face rake has neither guard, so the same state lets it go. */
    {
        const wm_smove_monitor_t *rake =
            wm_smove_monitor_find("hrt_charge_face_rake");
        assert(rake && rake->charge);
        memset(&me, 0, sizeof me);
        me.facing_dir = WM_MOVE_RIGHT;
        me.new_facing_dir = WM_MOVE_RIGHT;
        held = 100;
        him.player_mode = WM_PMODE_ONGROUND;
        assert(wm_smove_charge_tick(rake->charge, &held, &me, &env, &out));
        assert(strcmp(out.anim, "hrt_rake_face_anim") == 0);
    }
}

/* ---- the grab_toss_air family ----------------------------------- */

static void test_grab_table(void) {
    size_t i;
    int pairs = 0, thresholds[3] = {0, 0, 0};

    assert(wm_smove_grab_count == 8);
    for (i = 0; i < wm_smove_grab_count; ++i) {
        const wm_smove_grab_t *r = &wm_smove_grab[i];
        assert(r->name && r->file);
        /* Away, away, punch, on a 40-tick window: the one part of
           this family that really is the same eight times. */
        assert(r->step[0].switches == WM_J_AWAY && r->step[0].mask == 0);
        assert(r->step[1].switches == WM_J_AWAY && r->step[1].mask == 0);
        assert(r->step[2].switches == WM_B_PUNCH);
        assert(r->step[2].mask == WM_J_ALL);
        assert(r->timeout == 40);
        assert(r->cooldown == 20);
        /* The distance is not. */
        if (r->near_dist == 0x68) ++thresholds[0];
        else if (r->near_dist == 0x6c) ++thresholds[1];
        else if (r->near_dist == 0x70) ++thresholds[2];
        else assert(0);
        /* And nor is how the animation is picked: five use FACE24,
           three name one animation outright -- and Lex's FACE24 lines
           are COMMENTED OUT in the source, so reading the raw text
           gives him a pair he does not have. */
        if (r->near_anim_flipped) {
            assert(r->air_anim_flipped);
            ++pairs;
        } else {
            assert(!r->air_anim_flipped);
        }
    }
    assert(pairs == 5);
    assert(thresholds[0] == 3 && thresholds[1] == 4 && thresholds[2] == 1);

    /* Only the Undertaker lets go and drops back to MODE_NORMAL. */
    {
        int attach = 0;
        for (i = 0; i < wm_smove_grab_count; ++i)
            if (wm_smove_grab[i].clear_attach) ++attach;
        assert(attach == 1);
    }
}

/* The two-way choice, driven through the engine. */
static void test_grab_picks_its_animation(void) {
    const wm_smove_monitor_t *m = wm_smove_monitor_find("und_grab_toss_air");
    wm_arcade_actor_t me, him;
    wm_smove_env_t env;
    wm_smove_fire_t out;

    assert(m && m->grab);
    memset(&me, 0, sizeof me);
    memset(&him, 0, sizeof him);
    memset(&env, 0, sizeof env);
    env.hdhold_row = m->grab;
    env.closest = &him;
    /* FACE24 keeps the `_2_` form when MOVE_UP_BIT is set. */
    me.facing_dir = WM_MOVE_UP;
    me.attach_proc = &him;

    /* Standing opponent, too far away: nothing. */
    him.player_mode = WM_PMODE_NORMAL;
    me.closest_dist = 0x69;
    assert(!m->fire(&me, &env, &out));

    /* Close enough: the near animation. */
    me.closest_dist = 0x68;
    assert(m->fire(&me, &env, &out));
    assert(strcmp(out.anim, "und_2_snapmirror_anim") == 0);
    /* The Undertaker's own two extras. */
    assert(me.attach_proc == NULL);
    assert(me.player_mode == WM_PMODE_NORMAL);

    /* Airborne: the OTHER animation, and distance stops mattering. */
    me.closest_dist = 0x400;
    him.player_mode = WM_PMODE_INAIR;
    assert(m->fire(&me, &env, &out));
    assert(strcmp(out.anim, "und_2_snapmirror2_anim") == 0);

    /* So does a leaping attack from an opponent still on his feet. */
    him.player_mode = WM_PMODE_NORMAL;
    him.attack_type = WM_AT_LEAPING;
    assert(m->fire(&me, &env, &out));
    assert(strcmp(out.anim, "und_2_snapmirror2_anim") == 0);
    him.attack_type = 0;

    /* Facing the other way picks the `_4_` forms. */
    me.facing_dir = 0;
    me.closest_dist = 0x10;
    assert(m->fire(&me, &env, &out));
    assert(strcmp(out.anim, "und_4_snapmirror_anim") == 0);

    /* A downed opponent refuses either way, and so does a wrestler
       holding a head -- these belong to the head-hold monitors. */
    him.player_mode = WM_PMODE_ONGROUND;
    assert(!m->fire(&me, &env, &out));
    him.player_mode = WM_PMODE_NORMAL;
    me.player_mode = WM_PMODE_HEADHOLD;
    assert(!m->fire(&me, &env, &out));
}

/* ---- the free-move family --------------------------------------- */

static void test_free_table(void) {
    size_t i;
    int gated = 0, stops_run = 0;

    assert(wm_smove_free_count == 6);
    for (i = 0; i < wm_smove_free_count; ++i) {
        const wm_smove_free_t *r = &wm_smove_free[i];
        assert(r->name && r->file && r->anim);
        assert(r->guards > 0 && r->guards <= WM_SMOVE_MAX_GUARDS);
        assert(r->gates <= WM_SMOVE_MAX_GUARDS);
        assert(r->step[2].switches >= WM_B_PUNCH);
        if (r->gates) ++gated;
        if (r->clear_run_time) ++stops_run;
    }
    /* The Undertaker's two spirit moves are the ones with a gate, and
       it is the OPPOSITE of the head-hold family's: they refuse while
       anyone has a head hold. Four of the six were read as head-hold
       monitors before the gate's direction was checked. */
    assert(gated == 2);
    assert(stops_run == 3);
}

/*
 * TAKER.ASM:1036 und_spirit_push, which the first reader could not
 * even find: SPECIAL.ASM:1541 defines a GLOBAL routine of the same
 * name -- the spirit object an animation creates -- and the smove
 * table binds to TAKER.ASM's local one.
 */
static void test_spirit_gate_is_inverted(void) {
    const wm_smove_monitor_t *m = wm_smove_monitor_find("und_spirit_push");
    wm_arcade_actor_t me, him;
    wm_smove_env_t env;
    wm_smove_run_t run;
    wm_smove_fire_t out;

    assert(m && m->freemove);
    assert(strcmp(m->file, "TAKER.ASM") == 0);
    /* `SLEEP 3*60` before it loops. */
    assert(m->cooldown == 180);

    memset(&me, 0, sizeof me);
    memset(&him, 0, sizeof him);
    memset(&env, 0, sizeof env);
    memset(&run, 0, sizeof run);
    run.monitor = m;
    env.closest = &him;

    /* Holding a head: the gate refuses, where a head-hold monitor's
       gate would require exactly this. */
    me.player_mode = WM_PMODE_HEADHOLD;
    assert(!wm_smove_tick(&run, &me, &env, &out));
    assert(!run.armed);
    me.player_mode = WM_PMODE_HEADHELD;
    assert(!wm_smove_tick(&run, &me, &env, &out));
    assert(!run.armed);

    /* Neutral: it arms. */
    me.player_mode = WM_PMODE_NORMAL;
    assert(!wm_smove_tick(&run, &me, &env, &out));
    assert(run.armed);

    /* DOWN, TOWARD, KICK. */
    me.stick_rel_new = WM_J_DOWN;
    assert(!wm_smove_tick(&run, &me, &env, &out));
    assert(run.countdown == 60);
    me.stick_rel_new = WM_J_TOWARD;
    assert(!wm_smove_tick(&run, &me, &env, &out));
    me.stick_rel_new = 0;
    me.but_val_down = WM_BTN_KICK;
    me.run_time = 40;
    assert(wm_smove_tick(&run, &me, &env, &out));
    assert(strcmp(out.anim, "und_spirit_push_anim") == 0);
    /* `clr a0 / move a0,*a8(RUN_TIME)` and `SETMODE NORMAL`. */
    assert(me.run_time == 0);
    assert(me.player_mode == WM_PMODE_NORMAL);
    /* And now it sleeps for three seconds rather than re-firing. */
    assert(run.cooldown == 180);
    me.but_val_down = 0;
    assert(!wm_smove_tick(&run, &me, &env, &out));
    assert(run.cooldown == 179);
}

/* A guard list really refuses. TAKER.ASM:984's choke slide will not
   go while the opponent is already choking. */
static void test_free_guards_refuse(void) {
    const wm_smove_monitor_t *m = wm_smove_monitor_find("und_choke_slide");
    wm_arcade_actor_t me, him;
    wm_smove_env_t env;
    wm_smove_fire_t out;

    assert(m && m->freemove && m->freemove->gates == 0);
    memset(&me, 0, sizeof me);
    memset(&him, 0, sizeof him);
    memset(&env, 0, sizeof env);
    env.hdhold_row = m->freemove;
    env.closest = &him;

    assert(m->fire(&me, &env, &out));
    assert(strcmp(out.anim, "und_sliding_choke_anim") == 0);

    him.player_mode = WM_PMODE_CHOKING;
    assert(!m->fire(&me, &env, &out));
    him.player_mode = WM_PMODE_NORMAL;

    /* And not off the turnbuckle, or mid-run, or on his way out. */
    me.player_mode = WM_PMODE_ONTURNBKL;
    assert(!m->fire(&me, &env, &out));
    me.player_mode = WM_PMODE_RUNNING;
    assert(!m->fire(&me, &env, &out));
    me.player_mode = WM_PMODE_NORMAL;
    me.i_will_die = 1;
    assert(!m->fire(&me, &env, &out));
}

/* ---- the two with three outcomes -------------------------------- */

static void test_flipslam_has_three_answers(void) {
    const wm_smove_monitor_t *m = wm_smove_monitor_find("shn_flipslam");
    wm_arcade_actor_t me, him;
    wm_smove_env_t env;
    wm_smove_fire_t out;

    assert(m && !m->hdhold && !m->grab && !m->freemove && !m->charge);
    /* `#TIMEOUT .equ 30`, alone among Shawn's. */
    assert(m->timeout == 30);

    memset(&me, 0, sizeof me);
    memset(&him, 0, sizeof him);
    memset(&env, 0, sizeof env);
    me.who_i_hit = &him;
    me.who_hit_me = &him;

    /* Holding: the slam, bonus 39, pinning WHOIHIT. */
    me.player_mode = WM_PMODE_HEADHOLD;
    assert(m->fire(&me, &env, &out));
    assert(out.bonus == 39);
    assert(out.victim == &him && out.victim_immobilize == 15);
    assert(strcmp(out.anim, "shn_flipslam_anim") == 0);

    /* Held: a reversal onto WHOHITME, and no bonus. */
    me.player_mode = WM_PMODE_HEADHELD;
    assert(m->fire(&me, &env, &out));
    assert(out.bonus == -1);
    assert(out.victim == &him);

    /* And from a neutral stance -- "Can do from head hold also!" says
       the source, meaning the head hold is the EXTRA. Same animation,
       nobody targeted, nobody pinned. */
    me.player_mode = WM_PMODE_NORMAL;
    assert(m->fire(&me, &env, &out));
    assert(out.bonus == -1);
    assert(out.victim == NULL);
    assert(out.victim_immobilize == 0);
    assert(strcmp(out.anim, "shn_flipslam_anim") == 0);
}

static void test_swirl_knee_is_a_different_move(void) {
    const wm_smove_monitor_t *m =
        wm_smove_monitor_find("shn_swirl_speedkick");
    wm_arcade_actor_t me, him;
    wm_smove_env_t env;
    wm_smove_run_t run;
    wm_smove_fire_t out;

    assert(m);
    memset(&me, 0, sizeof me);
    memset(&him, 0, sizeof him);
    memset(&env, 0, sizeof env);
    memset(&run, 0, sizeof run);
    run.monitor = m;
    env.pcnt = 1000;
    me.who_i_hit = &him;
    me.who_hit_me = &him;

    /* Neutral, and while being held, both give the speed kick -- the
       reversal path's `jruc #is_reversal` is commented out, so it
       falls through into the slam. */
    me.player_mode = WM_PMODE_NORMAL;
    assert(m->fire(&me, &env, &out));
    assert(strcmp(out.anim, "shn_super_speedkick_anim") == 0);
    me.player_mode = WM_PMODE_HEADHELD;
    assert(m->fire(&me, &env, &out));
    assert(strcmp(out.anim, "shn_super_speedkick_anim") == 0);
    assert(me.smart_target == &him);

    /* Holding a head turns it into the flying knee, with its own
       damage window and DELAY_METER on the victim. */
    memset(&me, 0, sizeof me);
    me.who_i_hit = &him;
    me.player_mode = WM_PMODE_HEADHOLD;
    assert(m->fire(&me, &env, &out));
    assert(strcmp(out.anim, "shn_flying_kick_anim") == 0);
    assert(me.player_mode == WM_PMODE_INAIR);
    assert(him.delay_meter == 6 * 60);
    assert(me.next_damage == 14);          /* D_FLYKICK/2, and D_FLYKICK=28 */
    assert(me.special_damage_time == 1020u);

    /* And the lift comes five ticks in, not at once: `SLEEPK 5 / movi
       40000h,a0 / move a0,*a8(OBJ_YVEL),L / SLEEPK 20`. */
    run.cooldown = 25;
    assert(me.y_vel == 0);
    assert(!wm_smove_tick(&run, &me, &env, &out));   /* 24 */
    assert(me.y_vel == 0);
    run.cooldown = 21;
    assert(!wm_smove_tick(&run, &me, &env, &out));   /* -> 20 */
    assert(me.y_vel == 0x40000);
}

/* Every one of the 79 table entries now finds a monitor. */
static void test_nothing_is_left(void) {
    wm_smove_run_t runs[WM_SMOVE_MAX_PER_WRESTLER];
    size_t total = 0, missing = 0, n, gone;
    int w;

    for (w = 0; w < WM_WRESTLER_ANIM_SLOTS; ++w) {
        n = wm_smove_init(w, false, runs, WM_SMOVE_MAX_PER_WRESTLER, &gone);
        total += n;
        missing += gone;
    }
    assert(missing == 0);
    assert(total + missing == 79);
}

int main(void) {
    test_animations_resolve();
    test_charge_table();
    test_charge_runs();
    test_charge_guards_refuse();
    test_grab_table();
    test_grab_picks_its_animation();
    test_free_table();
    test_spirit_gate_is_inverted();
    test_free_guards_refuse();
    test_flipslam_has_three_answers();
    test_swirl_knee_is_a_different_move();
    test_nothing_is_left();
    return 0;
}
