/*
 * The two carry modes: MODE_OPPOVERHEAD (10) and MODE_CHOKEHOLD (24).
 *
 * Sixteen routines across the eight wrestler files, and only FOUR have a
 * body: YOKO.ASM:2231, LEX.ASM:2219 and BAM.ASM:2402 mode_oppoverhead,
 * and TAKER.ASM:2940 mode_chokehold. The other twelve are a bare `rets`
 * and are right to be -- `ANI_SETPLYRMODE,MODE_OPPOVERHEAD` appears only
 * in BAMSEQ3, LEXSEQ3 and YOKSEQ3, and `MODE_CHOKEHOLD` only in
 * UNDSEQ3, so no other wrestler can ever be in those modes.
 *
 * Every dispatcher used to fall through both to STEP_IDLE. That was not
 * harmless: the generated programs carry ten `SETPLYRMODE 10` ops and
 * one `SETPLYRMODE 24`, so a wrestler who lifted somebody overhead
 * really did stand frozen with no controls.
 *
 * The two modes hold the attachment to DIFFERENT standards, and these
 * tests pin that difference because it is the easiest thing to flatten
 * by accident.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_arcade_yoko.h"
#include "wm_arcade_lex.h"
#include "wm_arcade_bam.h"
#include "wm_arcade_taker.h"
#include "wm_arcade_shawn.h"
#include "wm_arcade_doink.h"
#include "wm_arcade_bret.h"
#include "wm_arcade_razor.h"
#include "wm_arcade_roster.h"
#include "wmania_ring_geometry.h"

static const char *guarded, *restarted, *torso, *sound;
static int kills;
static void cap_guarded(wm_arcade_actor_t *a, const char *l, void *u) {
    (void)a; (void)u; guarded = l;
}
static void cap_restart(wm_arcade_actor_t *a, const char *l, void *u) {
    (void)a; (void)u; restarted = l;
}
static void cap_torso(wm_arcade_actor_t *a, const char *l, void *u) {
    (void)a; (void)u; torso = l;
}
static void cap_sound(wm_arcade_actor_t *a, const char *l, void *u) {
    (void)a; (void)u; sound = l;
}
static void cap_kill(wm_arcade_actor_t *a, void *u) { (void)a; (void)u; ++kills; }
static int walks;
static void cap_walk(wm_arcade_actor_t *a, void *u) { (void)a; (void)u; ++walks; }

static wm_arcade_roster_callbacks_t cbs(void) {
    wm_arcade_roster_callbacks_t c;
    memset(&c, 0, sizeof c);
    c.change_anim_label = cap_guarded;
    c.change_anim_restart = cap_restart;
    c.change_torso_label = cap_torso;
    c.change_torso_restart = cap_torso;
    c.sound_label = cap_sound;
    c.find_and_kill_endless = cap_kill;
    c.execute_walk = cap_walk;
    return c;
}

/* A carrier in mode 10 with `victim` attached both ways. */
static void carry(wm_arcade_actor_t *me, wm_arcade_actor_t *him, int who) {
    memset(me, 0, sizeof *me);
    memset(him, 0, sizeof *him);
    me->active = him->active = 1;
    me->wrestler_num = who;
    me->player_mode = WM_PMODE_OPPOVERHEAD;
    me->facing_dir = me->new_facing_dir = WM_MOVE_UP_RIGHT;
    me->x_int = WM_RING_X_MID;
    him->player_side = 1;
    him->player_mode = WM_PMODE_PUPPET;
    me->attach_proc = him;
    him->attach_proc = me;
    guarded = restarted = torso = sound = NULL;
    kills = walks = 0;
}

/*
 * `andni MOVE_UP,a0 / ori MOVE_DOWN,a0` into BOTH facing fields.
 * Carrying a man overhead forces you to face down-screen, keeping the
 * left/right bit -- so UP_RIGHT becomes DOWN_RIGHT and not plain DOWN.
 */
static void test_carrying_forces_the_facing_down(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);

    carry(&me, &him, WM_ROSTER_YOKO);
    (void)wm_arcade_move_yoko(&me, &him, &e, &c);
    assert(me.facing_dir == WM_MOVE_DOWN_RIGHT);
    assert(me.new_facing_dir == WM_MOVE_DOWN_RIGHT);

    carry(&me, &him, WM_ROSTER_LEX);
    me.facing_dir = me.new_facing_dir = WM_MOVE_UP_LEFT;
    (void)wm_arcade_move_lex(&me, &him, &e, &c);
    assert(me.facing_dir == WM_MOVE_DOWN_LEFT);
    assert(me.new_facing_dir == WM_MOVE_DOWN_LEFT);
}

/*
 * Stick idle is `#stand`: velocities zeroed and the STANDING carry on
 * channel one, guarded. Stick pushed walks and writes the moving carry
 * to the TORSO channel instead -- the only place outside *_ani_init
 * that touches channel two.
 */
static void test_standing_and_walking_use_different_channels(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);

    carry(&me, &him, WM_ROSTER_YOKO);
    me.x_vel = me.z_vel = 0x10000;
    (void)wm_arcade_move_yoko(&me, &him, &e, &c);
    assert(guarded && strcmp(guarded, "yok_stndholdoh_anim") == 0);
    assert(torso == NULL);
    assert(me.x_vel == 0 && me.z_vel == 0 && me.move_dir == 0);
    assert(walks == 0);

    carry(&me, &him, WM_ROSTER_YOKO);
    me.stick_val_cur = WM_MOVE_RIGHT;
    (void)wm_arcade_move_yoko(&me, &him, &e, &c);
    assert(torso && strcmp(torso, "yok_holdoh_anim") == 0);
    assert(guarded == NULL);
    assert(me.move_dir == WM_MOVE_RIGHT);
    assert(walks == 1);
}

/*
 * The attachment tests differ, and it is not a slip.
 *
 * mode_oppoverhead: `move *a2(ATTACH_PROC),a0,L / jrnz #still_attached`
 * -- the victim's attachment need only be NON-ZERO. So an overhead
 * carry survives its victim being grabbed by a third wrestler.
 *
 * mode_chokehold: `cmp a13,a0 / jrne #lost_him` -- it must point back at
 * ME. The same situation ends the choke.
 */
static void test_the_two_modes_hold_the_attachment_differently(void) {
    wm_arcade_actor_t me, him, third;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);
    memset(&third, 0, sizeof third);
    third.active = 1;

    /* Overhead: the victim now points at somebody else. Carry holds. */
    carry(&me, &him, WM_ROSTER_YOKO);
    him.attach_proc = &third;
    (void)wm_arcade_move_yoko(&me, &him, &e, &c);
    assert(me.player_mode == WM_PMODE_OPPOVERHEAD);
    assert(guarded && strcmp(guarded, "yok_stndholdoh_anim") == 0);

    /* Chokehold, same situation: lost him. */
    carry(&me, &him, WM_ROSTER_TAKER);
    me.player_mode = WM_PMODE_CHOKEHOLD;
    him.attach_proc = &third;
    (void)wm_arcade_move_taker(&me, &him, &e, &c);
    assert(me.player_mode == WM_PMODE_NORMAL);
    assert(me.attach_proc == NULL);
    assert(me.anim_mode == 0);
    assert(kills == 1);

    /* And pointing back at me keeps it. */
    carry(&me, &him, WM_ROSTER_TAKER);
    me.player_mode = WM_PMODE_CHOKEHOLD;
    me.but_val_down = WM_BTN_PUNCH;
    (void)wm_arcade_move_taker(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "und_2_butt_anim") == 0);
}

/*
 * `#not_attached` lets him go -- but MODE_UNINT is checked FIRST, so an
 * uninterruptible animation keeps the mode until it ends.
 */
static void test_losing_the_carry_waits_for_mode_unint(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);

    carry(&me, &him, WM_ROSTER_BAM);
    him.attach_proc = NULL;
    me.anim_mode = WM_MODE_UNINT;
    (void)wm_arcade_move_bam(&me, &him, &e, &c);
    assert(me.player_mode == WM_PMODE_OPPOVERHEAD);
    assert(me.attach_proc == &him);
    assert(kills == 0);

    carry(&me, &him, WM_ROSTER_BAM);
    him.attach_proc = NULL;
    (void)wm_arcade_move_bam(&me, &him, &e, &c);
    assert(me.player_mode == WM_PMODE_NORMAL);
    assert(me.attach_proc == NULL);
    assert(me.anim_mode == 0);
    assert(kills == 1);
}

/*
 * The three overhead slams, each with its own gate. Yoko's shared body
 * splits on DOWN and is the only one of the three that does NOT call
 * FIND_AND_KILL_ENDLESS; Lex gates on UP; Bam gates on DOWN and falls
 * back to #kick rather than #punch.
 */
static void test_the_overhead_slams(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);

    /* Yoko, plain punch with no stick: the plain slam, and no kill. */
    carry(&me, &him, WM_ROSTER_YOKO);
    me.but_val_down = WM_BTN_PUNCH;
    (void)wm_arcade_move_yoko(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "yok_overhd_slam_anim") == 0);
    assert(sound && strcmp(sound, "HIPTOSS") == 0);
    assert(kills == 0);

    /* Yoko, stick DOWN: the SPIN slam. */
    carry(&me, &him, WM_ROSTER_YOKO);
    me.but_val_down = WM_BTN_PUNCH;
    me.stick_val_cur = WM_MOVE_DOWN;
    (void)wm_arcade_move_yoko(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "yok_spinslam_anim") == 0);
    assert(kills == 0);

    /* Yoko, super punch with UP: the second slam, and this one DOES kill. */
    carry(&me, &him, WM_ROSTER_YOKO);
    me.but_val_down = WM_BTN_SPUNCH;
    me.stick_val_cur = WM_MOVE_UP;
    (void)wm_arcade_move_yoko(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "yok_overhd_slam2_anim") == 0);
    assert(kills == 1);

    /* Yoko, super punch WITHOUT up: falls into #punch. */
    carry(&me, &him, WM_ROSTER_YOKO);
    me.but_val_down = WM_BTN_SPUNCH;
    (void)wm_arcade_move_yoko(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "yok_overhd_slam_anim") == 0);
    assert(kills == 0);

    /* Lex: super kick with UP is the backbreaker, on the MIXED sound pair. */
    carry(&me, &him, WM_ROSTER_LEX);
    me.but_val_down = WM_BTN_SKICK;
    me.stick_val_cur = WM_MOVE_UP;
    (void)wm_arcade_move_lex(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "lex_backbreaker_anim") == 0);
    assert(sound && strcmp(sound, "HIPTOSS_PUNCH") == 0);

    /* Lex: super kick without UP falls to the ohslam, plain PUNCH. */
    carry(&me, &him, WM_ROSTER_LEX);
    me.but_val_down = WM_BTN_SKICK;
    (void)wm_arcade_move_lex(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "lex_ohslam_anim") == 0);
    assert(sound && strcmp(sound, "PUNCH") == 0);

    /* Bam: DOWN, not UP -- the opposite gate to Lex's on the same button. */
    carry(&me, &him, WM_ROSTER_BAM);
    me.but_val_down = WM_BTN_SKICK;
    me.stick_val_cur = WM_MOVE_UP;
    (void)wm_arcade_move_bam(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "bam_slamdwn_anim") == 0);

    carry(&me, &him, WM_ROSTER_BAM);
    me.but_val_down = WM_BTN_SKICK;
    me.stick_val_cur = WM_MOVE_DOWN;
    (void)wm_arcade_move_bam(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "bam_backbreaker_anim") == 0);
    assert(sound && strcmp(sound, "HIPTOSS_PUNCH") == 0);
}

/*
 * BLOCK is not a `rets` in mode_oppoverhead. It is an alias of the slam
 * body in all three files, so blocking while you hold a man overhead
 * drops him -- the opposite of mode_headhold, where #block does nothing.
 */
static void test_block_drops_him(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);

    carry(&me, &him, WM_ROSTER_YOKO);
    me.but_val_down = WM_BTN_BLOCK;
    (void)wm_arcade_move_yoko(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "yok_overhd_slam_anim") == 0);

    carry(&me, &him, WM_ROSTER_LEX);
    me.but_val_down = WM_BTN_BLOCK;
    (void)wm_arcade_move_lex(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "lex_ohslam_anim") == 0);

    carry(&me, &him, WM_ROSTER_BAM);
    me.but_val_down = WM_BTN_BLOCK;
    (void)wm_arcade_move_bam(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "bam_slamdwn_anim") == 0);
}

/*
 * mode_chokehold's button gate sits BEFORE the action table, unlike
 * every other mode: `andi 011111b,a0 / jrz #no_interrupt`. On an empty
 * button word nothing happens at all -- not even the IMMOBILIZE_TIME
 * refresh, which is the thing that keeps the victim helpless.
 */
static void test_the_chokehold_button_gate_comes_first(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);

    carry(&me, &him, WM_ROSTER_TAKER);
    me.player_mode = WM_PMODE_CHOKEHOLD;
    me.but_val_down = 0;
    him.immobilize_time = 0;
    (void)wm_arcade_move_taker(&me, &him, &e, &c);
    assert(him.immobilize_time == 0);
    assert(guarded == NULL && restarted == NULL && sound == NULL);
    assert(me.player_mode == WM_PMODE_CHOKEHOLD);

    /* Any attack button refreshes it to 30. */
    carry(&me, &him, WM_ROSTER_TAKER);
    me.player_mode = WM_PMODE_CHOKEHOLD;
    me.but_val_down = WM_BTN_PUNCH;
    (void)wm_arcade_move_taker(&me, &him, &e, &c);
    assert(him.immobilize_time == 30);
}

/*
 * The Undertaker's stick-UP punch converts the choke into a HEAD HOLD by
 * hand: SETMODE HEADHOLD overriding the SETMODE NORMAL just above, and
 * then WHOIHIT's PLYRMODE written straight to MODE_HEADHELD with his
 * MODE_NOGRAVITY cleared. Without that last clear he would still be
 * answering mode_choking -- "make sure victim knows he is not in
 * chokehold anymore" is the source's own comment on the line.
 */
static void test_the_choke_converts_to_a_head_hold(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);

    carry(&me, &him, WM_ROSTER_TAKER);
    me.player_mode = WM_PMODE_CHOKEHOLD;
    me.who_i_hit = &him;
    him.anim_mode = (uint16_t)(WM_MODE_NOGRAVITY | WM_MODE_CHECKHIT);
    me.but_val_down = WM_BTN_PUNCH;
    me.stick_val_cur = WM_MOVE_UP;
    (void)wm_arcade_move_taker(&me, &him, &e, &c);
    assert(me.player_mode == WM_PMODE_HEADHOLD);
    assert(him.player_mode == WM_PMODE_HEADHELD);
    assert((him.anim_mode & WM_MODE_NOGRAVITY) == 0);
    assert((him.anim_mode & WM_MODE_CHECKHIT) != 0);   /* only that bit */
    assert(restarted && strcmp(restarted, "und_4_knee_butts_anim") == 0);
    assert(sound && strcmp(sound, "HDBUTT") == 0);
}

/*
 * The chokeslam is the one arm that does NOT clear ATTACH_PROC -- the
 * slam carries the victim down with it. And the two knee arms are the
 * only ones in the mode with no sound at all.
 */
static void test_the_chokeslam_keeps_him_attached(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);

    carry(&me, &him, WM_ROSTER_TAKER);
    me.player_mode = WM_PMODE_CHOKEHOLD;
    me.but_val_down = WM_BTN_SKICK;
    me.stick_val_cur = WM_MOVE_DOWN;
    (void)wm_arcade_move_taker(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "und_chokeslam_anim") == 0);
    assert(sound && strcmp(sound, "KICK") == 0);
    assert(me.attach_proc == &him);            /* NOT cleared */
    assert(me.player_mode == WM_PMODE_NORMAL);

    /* Super kick without DOWN: the silent knee, and he IS released. */
    carry(&me, &him, WM_ROSTER_TAKER);
    me.player_mode = WM_PMODE_CHOKEHOLD;
    me.but_val_down = WM_BTN_SKICK;
    (void)wm_arcade_move_taker(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "und_2_knee_anim") == 0);
    assert(sound == NULL);
    assert(me.attach_proc == NULL);

    /* Plain kick: the same silent knee. */
    carry(&me, &him, WM_ROSTER_TAKER);
    me.player_mode = WM_PMODE_CHOKEHOLD;
    me.but_val_down = WM_BTN_KICK;
    (void)wm_arcade_move_taker(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "und_2_knee_anim") == 0);
    assert(sound == NULL);
}

/*
 * The uppercut arm is the one GUARDED selection in either mode
 * (TAKER.ASM:3038 `calla change_anim1`), and it is a FACE24 pair.
 */
static void test_the_chokehold_uppercut_is_guarded_and_faces(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);

    /* Facing UP -> the "2" form (MACROS.H:51 tests MOVE_UP_BIT). */
    carry(&me, &him, WM_ROSTER_TAKER);
    me.player_mode = WM_PMODE_CHOKEHOLD;
    me.facing_dir = me.new_facing_dir = WM_MOVE_UP_RIGHT;
    me.but_val_down = WM_BTN_SPUNCH;
    me.stick_val_cur = WM_MOVE_DOWN;
    (void)wm_arcade_move_taker(&me, &him, &e, &c);
    assert(guarded && strcmp(guarded, "und_2_uppercut_anim") == 0);
    assert(restarted == NULL);
    assert(sound && strcmp(sound, "HDBUTT") == 0);
    assert(me.attach_proc == NULL);

    /* Not facing UP -> the "4" form. */
    carry(&me, &him, WM_ROSTER_TAKER);
    me.player_mode = WM_PMODE_CHOKEHOLD;
    me.facing_dir = me.new_facing_dir = WM_MOVE_DOWN_RIGHT;
    me.but_val_down = WM_BTN_SPUNCH;
    me.stick_val_cur = WM_MOVE_DOWN;
    (void)wm_arcade_move_taker(&me, &him, &e, &c);
    assert(guarded && strcmp(guarded, "und_4_uppercut_anim") == 0);

    /* Super punch without DOWN falls into #punch. */
    carry(&me, &him, WM_ROSTER_TAKER);
    me.player_mode = WM_PMODE_CHOKEHOLD;
    me.but_val_down = WM_BTN_SPUNCH;
    (void)wm_arcade_move_taker(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "und_2_butt_anim") == 0);
}

/*
 * The twelve stubs. A wrestler who cannot enter the mode must do
 * nothing when put in it anyway -- which is what `rets` means, and is
 * distinct from "the port forgot this mode".
 */
static void test_the_twelve_stubs_do_nothing(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    unsigned i;
    static const uint16_t modes[2] = { WM_PMODE_OPPOVERHEAD,
                                       WM_PMODE_CHOKEHOLD };
    memset(&e, 0, sizeof e);

    for (i = 0; i < 2; ++i) {
        /* Shawn and Doink have neither; Taker has no overhead; Yoko, Bam
           and Lex have no chokehold. */
        carry(&me, &him, WM_ROSTER_SHAWN);
        me.player_mode = modes[i];
        me.but_val_down = WM_BTN_PUNCH;
        assert(wm_arcade_move_shawn(&me, &him, &e, &c) == WM_SHAWN_STEP_IDLE);
        assert(guarded == NULL && restarted == NULL && sound == NULL);
        assert(me.player_mode == modes[i]);

        carry(&me, &him, WM_ROSTER_DOINK);
        me.player_mode = modes[i];
        me.but_val_down = WM_BTN_PUNCH;
        assert(wm_arcade_move_doink(&me, &him, &e, &c) == WM_DOINK_STEP_IDLE);
        assert(guarded == NULL && restarted == NULL && sound == NULL);
    }

    carry(&me, &him, WM_ROSTER_TAKER);
    me.player_mode = WM_PMODE_OPPOVERHEAD;
    me.but_val_down = WM_BTN_PUNCH;
    assert(wm_arcade_move_taker(&me, &him, &e, &c) == WM_TAKER_STEP_IDLE);
    assert(guarded == NULL && restarted == NULL);

    carry(&me, &him, WM_ROSTER_YOKO);
    me.player_mode = WM_PMODE_CHOKEHOLD;
    me.but_val_down = WM_BTN_PUNCH;
    assert(wm_arcade_move_yoko(&me, &him, &e, &c) == WM_YOKO_STEP_IDLE);
    assert(guarded == NULL && restarted == NULL);

    carry(&me, &him, WM_ROSTER_BAM);
    me.player_mode = WM_PMODE_CHOKEHOLD;
    me.but_val_down = WM_BTN_PUNCH;
    assert(wm_arcade_move_bam(&me, &him, &e, &c) == WM_BAM_STEP_IDLE);
    assert(guarded == NULL && restarted == NULL);

    carry(&me, &him, WM_ROSTER_LEX);
    me.player_mode = WM_PMODE_CHOKEHOLD;
    me.but_val_down = WM_BTN_PUNCH;
    assert(wm_arcade_move_lex(&me, &him, &e, &c) == WM_LEX_STEP_IDLE);
    assert(guarded == NULL && restarted == NULL);
}

int main(void) {
    test_carrying_forces_the_facing_down();
    test_standing_and_walking_use_different_channels();
    test_the_two_modes_hold_the_attachment_differently();
    test_losing_the_carry_waits_for_mode_unint();
    test_the_overhead_slams();
    test_block_drops_him();
    test_the_chokehold_button_gate_comes_first();
    test_the_choke_converts_to_a_head_hold();
    test_the_chokeslam_keeps_him_attached();
    test_the_chokehold_uppercut_is_guarded_and_faces();
    test_the_twelve_stubs_do_nothing();
    printf("carry mode tests passed\n");
    return 0;
}
