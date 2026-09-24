/*
 * The other five shared dispatchers routed through their own JJXM
 * tables: LEX.ASM, TAKER.ASM, YOKO.ASM, SHAWN.ASM and BAM.ASM.
 *
 * Doink's own tests are in test_jjxm.c and cover the machinery. This
 * file covers what the machinery bought: rows that the hand-written
 * approximation could not express, and the handful of places where
 * reading the source contradicted what the approximation had assumed.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_arcade_jjxm.h"
#include "wm_arcade_lex.h"
#include "wm_arcade_taker.h"
#include "wm_arcade_yoko.h"
#include "wm_arcade_shawn.h"
#include "wm_arcade_bam.h"
#include "wmania_ring_geometry.h"

static const char *last_label;
static const char *last_sound;
static int label_calls;
static void cap_anim(wm_arcade_actor_t *a, const char *l, void *u) {
    (void)a; (void)u; last_label = l; ++label_calls;
}
static void cap_snd(wm_arcade_actor_t *a, const char *l, void *u) {
    (void)a; (void)u; last_sound = l;
}
/* ck_ignore that always refuses, for the paths whose behaviour on a
   refusal is the point. */
static int refuse(wm_arcade_actor_t *a, void *u) { (void)a; (void)u; return 1; }

static wm_arcade_roster_callbacks_t cbs(void) {
    wm_arcade_roster_callbacks_t c;
    memset(&c, 0, sizeof c);
    c.change_anim_label = cap_anim;
    c.sound_label = cap_snd;
    return c;
}
static void actors(wm_arcade_actor_t *me, wm_arcade_actor_t *him, int who) {
    memset(me, 0, sizeof *me);
    memset(him, 0, sizeof *him);
    me->active = him->active = 1;
    me->wrestler_num = who;
    me->player_mode = WM_PMODE_NORMAL;
    me->facing_dir = me->new_facing_dir = WM_MOVE_DOWN_RIGHT;
    me->x_int = WM_RING_X_MID;
    him->player_side = 1;
    him->player_mode = WM_PMODE_NORMAL;
    last_label = NULL; last_sound = NULL; label_calls = 0;
}

/*
 * `JJXM RUNNING, #skick_bigboot` and `JJXM INAIR2, #kick_TB` -- rows
 * with no distance test, which `super_kick(){basic_kick();}` could not
 * have reached however close anybody stood.
 */
static void test_lex_super_kick(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e; memset(&e, 0, sizeof e);

    actors(&me, &him, WM_ROSTER_LEX);
    me.but_val_down = WM_BTN_SKICK;
    him.player_mode = WM_PMODE_RUNNING;
    (void)wm_arcade_move_lex(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "lex_4_bigboot_anim") == 0);
    assert(last_sound && strcmp(last_sound, "FLYKICK") == 0);

    actors(&me, &him, WM_ROSTER_LEX);
    me.but_val_down = WM_BTN_SKICK;
    him.player_mode = WM_PMODE_INAIR2;
    (void)wm_arcade_move_lex(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "lex_kick_TB_anim") == 0);
}

/* LEX.ASM:1861 is `;TODO - fix this / ;HACK!!! / jruc #kick_flyingkick`
   -- Lex has no clothesline at all, the label falls into the flying
   kick, and the old code played his clobber instead. */
static void test_lex_running_punch_is_the_flying_kick(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e; memset(&e, 0, sizeof e);

    actors(&me, &him, WM_ROSTER_LEX);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.but_val_down = WM_BTN_PUNCH;
    (void)wm_arcade_move_lex(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "lex_flying_kick_anim") == 0);
    assert(me.player_mode == WM_PMODE_INAIR);

    /* Against a man on the mat it is the flying ground punch, which is
       one of the five labels REACT4's hit_stomp keys on. */
    actors(&me, &him, WM_ROSTER_LEX);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.but_val_down = WM_BTN_PUNCH;
    him.player_mode = WM_PMODE_ONGROUND;
    (void)wm_arcade_move_lex(&me, &him, &e, &c);
    assert(last_label &&
           strcmp(last_label, "lex_flying_ground_punch_anim") == 0);
}

/*
 * TAKER.ASM:2283's clothesline has NO ring-position test -- only
 * "don't do it if you're running away from your opponent" -- and
 * inside 70 pixels it is a headbutt instead.
 */
static void test_taker_running_punch(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e; memset(&e, 0, sizeof e);

    actors(&me, &him, WM_ROSTER_TAKER);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.closest_xdist = 200;
    me.but_val_down = WM_BTN_PUNCH;
    (void)wm_arcade_move_taker(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "und_2_run_slap_anim") == 0);
    assert(me.player_mode == WM_PMODE_NORMAL);
    assert(me.run_time == 0);

    /* Inside 70 it is the headbutt. */
    actors(&me, &him, WM_ROSTER_TAKER);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.closest_xdist = 40;
    me.but_val_down = WM_BTN_PUNCH;
    (void)wm_arcade_move_taker(&me, &him, &e, &c);
    assert(last_sound && strcmp(last_sound, "HDBUTT") == 0);

    /* Facing right, moving left: running away, so nothing at all. */
    actors(&me, &him, WM_ROSTER_TAKER);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.facing_dir = WM_MOVE_DOWN_RIGHT;
    me.new_facing_dir = WM_MOVE_DOWN_LEFT;
    me.but_val_down = WM_BTN_PUNCH;
    (void)wm_arcade_move_taker(&me, &him, &e, &c);
    assert(label_calls == 0);
}

/* `JJXM DEAD, 176,176, attack_bellyflop, attack_bellyflop` -- the
   Undertaker's flying butt drop, the label REACT4 bounces on. */
static void test_taker_running_kick_on_a_downed_man(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e; memset(&e, 0, sizeof e);

    actors(&me, &him, WM_ROSTER_TAKER);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.but_val_down = WM_BTN_KICK;
    him.player_mode = WM_PMODE_DEAD;
    (void)wm_arcade_move_taker(&me, &him, &e, &c);
    assert(last_label &&
           strcmp(last_label, "und_flying_butt_drop_anim") == 0);
}

/*
 * Yoko's super punch had been given a 50/92 distance split and a
 * SPUNCH sound. The source has neither: #spunch_special is only the
 * stick-down uppercut test, and every arm of his super punch plays
 * HDBUTT or PUNCH.
 */
static void test_yoko_super_punch_has_no_distance_test(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e; memset(&e, 0, sizeof e);

    actors(&me, &him, WM_ROSTER_YOKO);
    me.but_val_down = WM_BTN_SPUNCH;
    me.closest_xdist = 10; me.closest_zdist = 10;
    (void)wm_arcade_move_yoko(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "yok_2_jabs_anim") == 0);
    assert(last_sound && strcmp(last_sound, "HDBUTT") == 0);

    /* At the very edge of the table's 95/45 row it is still the jabs,
       because #spunch_special itself does not look at distance -- and
       because 95/45 is the CLOSED interval. */
    actors(&me, &him, WM_ROSTER_YOKO);
    me.but_val_down = WM_BTN_SPUNCH;
    me.closest_xdist = 95; me.closest_zdist = 45;
    (void)wm_arcade_move_yoko(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "yok_2_jabs_anim") == 0);

    /* Stick down is the uppercut. */
    actors(&me, &him, WM_ROSTER_YOKO);
    me.but_val_down = WM_BTN_SPUNCH;
    me.stick_val_cur = WM_MOVE_DOWN;
    (void)wm_arcade_move_yoko(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "yok_4_uppercut_anim") == 0);
}

/* Yoko's running kick against a downed opponent is the BUTT DROP, the
   same target his running punch reaches -- both running tables name
   #punch_buttdrop, and it plays GRABTHROW. */
static void test_yoko_running_kick_is_the_butt_drop(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e; memset(&e, 0, sizeof e);

    actors(&me, &him, WM_ROSTER_YOKO);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.but_val_down = WM_BTN_KICK;
    him.player_mode = WM_PMODE_ONGROUND;
    (void)wm_arcade_move_yoko(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "yok_3_butt_drop_anim") == 0);
    assert(last_sound && strcmp(last_sound, "GRABTHROW") == 0);
}

/*
 * Shawn has EIGHT tables: his running super punch is a different table
 * from his running punch, so the two buttons do different things.
 */
static void test_shawn_running_punch_and_super_punch_differ(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e; memset(&e, 0, sizeof e);

    /* Against a STANDING opponent the running punch is `#punch_rets`
       -- Shawn simply has no running plain punch, which is exactly the
       kind of row the two-arm approximation could not represent. */
    actors(&me, &him, WM_ROSTER_SHAWN);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.but_val_down = WM_BTN_PUNCH;
    (void)wm_arcade_move_shawn(&me, &him, &e, &c);
    assert(label_calls == 0);

    /* Against a man on the mat it is the run stomp. */
    actors(&me, &him, WM_ROSTER_SHAWN);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.but_val_down = WM_BTN_PUNCH;
    him.player_mode = WM_PMODE_ONGROUND;
    (void)wm_arcade_move_shawn(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "shn_run_stomp_anim") == 0);

    actors(&me, &him, WM_ROSTER_SHAWN);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.but_val_down = WM_BTN_SPUNCH;
    (void)wm_arcade_move_shawn(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "shn_flipslam_anim") == 0);
    assert(last_sound && strcmp(last_sound, "HIPTOSS_PUNCH") == 0);
}

/* SHAWN.ASM:2262 #skick_frank: ck_ignore does NOT refuse the move, it
   falls through to the spin kick. Every other ck_ignore in these files
   drops the press. */
static void test_shawn_frankensteiner_falls_through(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e; memset(&e, 0, sizeof e);

    /* `JJXM RUNNING, #skick_frank` -- against a RUNNING or BOUNCING
       opponent, and against no other. */
    actors(&me, &him, WM_ROSTER_SHAWN);
    me.but_val_down = WM_BTN_SKICK;
    him.player_mode = WM_PMODE_RUNNING;
    (void)wm_arcade_move_shawn(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "shn_fstein_anim") == 0);

    c.ck_ignore = refuse;
    actors(&me, &him, WM_ROSTER_SHAWN);
    me.but_val_down = WM_BTN_SKICK;
    him.player_mode = WM_PMODE_RUNNING;
    (void)wm_arcade_move_shawn(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "shn_spinkick_anim") == 0);
}

/* SHAWN.ASM:2293 attack_flykick looks at the opponent's mode a SECOND
   time, after the table has already keyed on it -- "don't do it if the
   bad guy is on the ground". */
static void test_shawn_flying_kick_refuses_a_downed_man(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e; memset(&e, 0, sizeof e);

    actors(&me, &him, WM_ROSTER_SHAWN);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.but_val_down = WM_BTN_KICK;
    (void)wm_arcade_move_shawn(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "shn_flying_kick_anim") == 0);

    /* The routine's second look at the opponent's mode cannot be
       reached from this table -- every mode that would fail it
       (ONGROUND, DEAD) is already routed to #kick_runstomp by the
       table itself. So what is checkable here is that the ONGROUND row
       IS the run stomp, and the redundant guard is translated for
       fidelity rather than for a branch anything can take. */
    actors(&me, &him, WM_ROSTER_SHAWN);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.but_val_down = WM_BTN_KICK;
    him.player_mode = WM_PMODE_ONGROUND;
    (void)wm_arcade_move_shawn(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "shn_run_stomp_anim") == 0);
}

/* Bam Bam's super kick is ONE animation either side of the table's
   distance split -- #skick_special and #skick_kick are two labels on
   one instruction -- and it plays SPUNCH, not FLYKICK. */
static void test_bam_super_kick_is_one_move(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e; memset(&e, 0, sizeof e);

    actors(&me, &him, WM_ROSTER_BAM);
    me.but_val_down = WM_BTN_SKICK;
    me.closest_xdist = 10; me.closest_zdist = 10;
    (void)wm_arcade_move_bam(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "bam_2_superkick_anim") == 0);
    assert(last_sound && strcmp(last_sound, "SPUNCH") == 0);

    actors(&me, &him, WM_ROSTER_BAM);
    me.but_val_down = WM_BTN_SKICK;
    me.closest_xdist = 200; me.closest_zdist = 200;
    (void)wm_arcade_move_bam(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "bam_2_superkick_anim") == 0);
}

/* Bam Bam's clothesline is Doink's, gates and all. */
static void test_bam_clothesline_gates(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e; memset(&e, 0, sizeof e);

    actors(&me, &him, WM_ROSTER_BAM);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.move_dir = WM_MOVE_RIGHT;
    me.but_val_down = WM_BTN_PUNCH;
    (void)wm_arcade_move_bam(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "bam_fly_cline_anim") == 0);
    assert(me.player_mode == WM_PMODE_INAIR && me.run_time == 0);

    actors(&me, &him, WM_ROSTER_BAM);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.move_dir = WM_MOVE_RIGHT;
    me.x_int = WM_RING_X_MID + 0x70;
    me.but_val_down = WM_BTN_PUNCH;
    (void)wm_arcade_move_bam(&me, &him, &e, &c);
    assert(label_calls == 0);
}

/* Bam Bam's do_pile runs FIND_AND_KILL_ENDLESS BEFORE its USR_VAR2
   gate, where Doink's runs it after -- so a headheld opponent ends an
   endless combo either way. */
static int kills;
static void cap_kill(wm_arcade_actor_t *a, void *u) { (void)a; (void)u; ++kills; }

static void test_bam_do_pile_kills_endless_first(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e; memset(&e, 0, sizeof e);
    c.find_and_kill_endless = cap_kill;

    actors(&me, &him, WM_ROSTER_BAM);
    kills = 0;
    me.but_val_down = WM_BTN_SPUNCH;
    him.player_mode = WM_PMODE_HEADHELD;
    me.usr_var2 = 0;
    (void)wm_arcade_move_bam(&me, &him, &e, &c);
    assert(kills == 1 && label_calls == 0);

    actors(&me, &him, WM_ROSTER_BAM);
    kills = 0;
    me.but_val_down = WM_BTN_SPUNCH;
    him.player_mode = WM_PMODE_HEADHELD;
    me.usr_var2 = 1;
    me.stick_val_cur = WM_MOVE_DOWN;
    (void)wm_arcade_move_bam(&me, &him, &e, &c);
    assert(last_label && strcmp(last_label, "bam_3_pile_driver_anim") == 0);
}

int main(void) {
    test_lex_super_kick();
    test_lex_running_punch_is_the_flying_kick();
    test_taker_running_punch();
    test_taker_running_kick_on_a_downed_man();
    test_yoko_super_punch_has_no_distance_test();
    test_yoko_running_kick_is_the_butt_drop();
    test_shawn_running_punch_and_super_punch_differ();
    test_shawn_frankensteiner_falls_through();
    test_shawn_flying_kick_refuses_a_downed_man();
    test_bam_super_kick_is_one_move();
    test_bam_clothesline_gates();
    test_bam_do_pile_kills_endless_first();
    printf("jjxm roster: ok\n");
    return 0;
}
