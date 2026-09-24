/*
 * ANIM.ASM's two primary-animation entry points, and DOINK.ASM:3353
 * mode_headhold.
 *
 * :4532 change_anim1 tests two things before it does anything:
 *
 *      move  *a13(ANIMODE),a2
 *      btst  MODE_END_BIT,a2       ; ended? restart anyway
 *      jrnz  change_anim1a
 *      move  *a13(ANIBASE),a2,L
 *      cmp   a0,a2
 *      jreq  #no_change            ; already playing it: do NOTHING
 *
 * :4542 change_anim1a is the label on the instruction after both, so a
 * caller that enters there always replays from frame 0. The eight
 * wrestler files use both on purpose -- a held block must not retrigger
 * every tick, a mashed stomp must -- and 41 of their roughly 450 call
 * sites are the guarded one.
 *
 * The port used to route every one of them through a single seam that
 * always guarded, which turned every change_anim1a call site into a
 * change_anim1.
 *
 * These tests bind the two seams to DIFFERENT recorders, so a move that
 * picks the wrong entry point is visible rather than merely equal.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_arcade_doink.h"
#include "wm_arcade_bret.h"
#include "wm_arcade_taker.h"
#include "wm_arcade_yoko.h"
#include "wm_arcade_shawn.h"
#include "wm_arcade_bam.h"
#include "wm_arcade_lex.h"
#include "wm_arcade_roster.h"
#include "wmania_ring_geometry.h"

static const char *guarded, *restarted, *sound;
static int guarded_n, restarted_n;
static void cap_guarded(wm_arcade_actor_t *a, const char *l, void *u) {
    (void)a; (void)u; guarded = l; ++guarded_n;
}
static void cap_restart(wm_arcade_actor_t *a, const char *l, void *u) {
    (void)a; (void)u; restarted = l; ++restarted_n;
}
static void cap_sound(wm_arcade_actor_t *a, const char *l, void *u) {
    (void)a; (void)u; sound = l;
}
static void nop_kill(wm_arcade_actor_t *a, void *u) { (void)a; (void)u; }

static wm_arcade_roster_callbacks_t cbs(void) {
    wm_arcade_roster_callbacks_t c;
    memset(&c, 0, sizeof c);
    c.change_anim_label = cap_guarded;
    c.change_anim_restart = cap_restart;
    c.sound_label = cap_sound;
    c.find_and_kill_endless = nop_kill;
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
    guarded = restarted = sound = NULL;
    guarded_n = restarted_n = 0;
}

/*
 * The block animation is change_anim1 in every file that has one --
 * BRET.ASM:1574, TAKER.ASM:1771, BAM.ASM:1534, DOINK.ASM:1949,
 * LEX.ASM:1384 -- because holding the button must not restart it.
 */
static void test_block_takes_the_guarded_entry(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);

    actors(&me, &him, WM_ROSTER_DOINK);
    me.but_val_cur = WM_BTN_BLOCK;
    (void)wm_arcade_move_doink(&me, &him, &e, &c);
    assert(guarded && strcmp(guarded, "dnk_4_block_anim") == 0);
    assert(restarted == NULL);

    actors(&me, &him, WM_ROSTER_LEX);
    me.but_val_cur = WM_BTN_BLOCK;
    (void)wm_arcade_move_lex(&me, &him, &e, &c);
    assert(guarded && strcmp(guarded, "lex_4_block_anim") == 0);
    assert(restarted == NULL);

    actors(&me, &him, WM_ROSTER_BAM);
    me.but_val_cur = WM_BTN_BLOCK;
    (void)wm_arcade_move_bam(&me, &him, &e, &c);
    assert(guarded && strcmp(guarded, "bam_4_block_anim") == 0);
    assert(restarted == NULL);
}

/*
 * DOINK.ASM:2067 #spunch_lbowdrop guards BOTH arms (:2102, :2107) while
 * :1931 #punch_lbowdrop selects the very same lbowdrop pair through
 * change_anim1a. Same animation, two entry points, one file -- which is
 * the clearest possible demonstration that the split is deliberate.
 */
static void test_one_animation_two_entry_points(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);

    /* The super-punch route, against a man on the mat: guarded. */
    actors(&me, &him, WM_ROSTER_DOINK);
    him.player_mode = WM_PMODE_ONGROUND;
    me.but_val_down = WM_BTN_SPUNCH;
    me.closest_xdist = 10; me.closest_zdist = 10;
    (void)wm_arcade_move_doink(&me, &him, &e, &c);
    assert(guarded && strcmp(guarded, "dnk_4_lbowdrop_anim") == 0);
    assert(restarted == NULL);

    /* The plain-punch route to the same animation: unguarded. */
    actors(&me, &him, WM_ROSTER_DOINK);
    him.player_mode = WM_PMODE_ONGROUND;
    me.but_val_down = WM_BTN_PUNCH;
    me.closest_xdist = 10; me.closest_zdist = 10;
    (void)wm_arcade_move_doink(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "dnk_4_lbowdrop_anim") == 0);
    assert(guarded == NULL);
}

/*
 * Shawn and Bam guard the hair pickup and NOT the arm beside it
 * (SHAWN.ASM:2113 vs :2118, BAM.ASM:1672 vs :1678), where Bret, Taker,
 * Yoko, Lex and Doink guard both. A per-wrestler difference, so a rule
 * applied uniformly would be wrong for somebody either way.
 */
static void test_the_hair_grab_split_is_per_wrestler(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);

    /* Not a hair grab: same FLIPH, so the fallback arm runs. */
    actors(&me, &him, WM_ROSTER_SHAWN);
    him.player_mode = WM_PMODE_ONGROUND;
    me.but_val_down = WM_BTN_SPUNCH;
    me.closest_xdist = 10; me.closest_zdist = 10;
    (void)wm_arcade_move_shawn(&me, &him, &e, &c);
    assert(restarted != NULL);          /* :2118, change_anim1a */
    assert(guarded == NULL);

    actors(&me, &him, WM_ROSTER_BAM);
    him.player_mode = WM_PMODE_ONGROUND;
    me.but_val_down = WM_BTN_SPUNCH;
    me.closest_xdist = 10; me.closest_zdist = 10;
    (void)wm_arcade_move_bam(&me, &him, &e, &c);
    assert(restarted != NULL);          /* :1678, change_anim1a */
    assert(guarded == NULL);

    /* Yoko guards the same arm (:1535), so his fallback is the other way. */
    actors(&me, &him, WM_ROSTER_YOKO);
    him.player_mode = WM_PMODE_ONGROUND;
    me.but_val_down = WM_BTN_SPUNCH;
    me.closest_xdist = 10; me.closest_zdist = 10;
    (void)wm_arcade_move_yoko(&me, &him, &e, &c);
    assert(guarded != NULL);
    assert(restarted == NULL);
}

/*
 * Doink's #ck_up uppercut is change_anim1 (:2061) and Bret's is
 * change_anim1a (:1666). Same move, same trigger -- super punch with
 * the stick held DOWN -- and opposite entry points.
 */
static void test_the_uppercut_differs_between_doink_and_bret(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);

    actors(&me, &him, WM_ROSTER_DOINK);
    me.but_val_down = WM_BTN_SPUNCH;
    me.stick_val_cur = WM_MOVE_DOWN;
    me.closest_xdist = 20; me.closest_zdist = 20;
    (void)wm_arcade_move_doink(&me, &him, &e, &c);
    assert(guarded && strcmp(guarded, "dnk_4_uppercut_anim") == 0);
    assert(restarted == NULL);
}

/* ---- DOINK.ASM:3353 mode_headhold ------------------------------- */

/*
 * `#exit`: the opponent is not HEADHELD, so drop six units of Z, face
 * MOVE_DOWN_RIGHT (or _LEFT with B_FLIPH set) in BOTH facing fields,
 * and only then SETMODE NORMAL. The facing write was missing.
 */
static void test_leaving_a_head_hold_sets_the_facing(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    int32_t z0;
    memset(&e, 0, sizeof e);

    actors(&me, &him, WM_ROSTER_DOINK);
    me.player_mode = WM_PMODE_HEADHOLD;
    me.facing_dir = me.new_facing_dir = WM_MOVE_UP;
    him.player_mode = WM_PMODE_NORMAL;          /* NOT headheld */
    z0 = me.z_fixed;
    (void)wm_arcade_move_doink(&me, &him, &e, &c);
    assert(me.z_fixed == z0 - (6 << 16));
    assert(me.facing_dir == WM_MOVE_DOWN_RIGHT);
    assert(me.new_facing_dir == WM_MOVE_DOWN_RIGHT);
    assert(me.player_mode == WM_PMODE_NORMAL);

    actors(&me, &him, WM_ROSTER_DOINK);
    me.player_mode = WM_PMODE_HEADHOLD;
    me.obj_control = WM_OBJ_FLIPH;
    him.player_mode = WM_PMODE_NORMAL;
    (void)wm_arcade_move_doink(&me, &him, &e, &c);
    assert(me.facing_dir == WM_MOVE_DOWN_LEFT);
    assert(me.new_facing_dir == WM_MOVE_DOWN_LEFT);
}

/*
 * The mode reads WHOIHIT, not the caller's opponent. With a third
 * wrestler in the ring the man Doink holds need not be the closest one,
 * and the port used to test the wrong actor.
 */
static void test_head_hold_reads_who_i_hit(void) {
    wm_arcade_actor_t me, near, held;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);

    actors(&me, &near, WM_ROSTER_DOINK);
    memset(&held, 0, sizeof held);
    held.active = 1;
    held.player_mode = WM_PMODE_HEADHELD;
    me.player_mode = WM_PMODE_HEADHOLD;
    me.who_i_hit = &held;
    near.player_mode = WM_PMODE_NORMAL;         /* would have exited */
    me.but_val_down = WM_BTN_PUNCH;
    me.stick_val_cur = (uint16_t)(me.new_facing_dir & 0x0c);
    (void)wm_arcade_move_doink(&me, &near, &e, &c);
    assert(me.player_mode == WM_PMODE_HEADHOLD);   /* did NOT exit */
    assert(restarted && strcmp(restarted, "dnk_uppercuts_to_head_anim") == 0);
    assert(sound && strcmp(sound, "UPRCUT") == 0);
}

/*
 * :3430 #punch. Toward the opponent is the PLURAL uppercuts animation
 * -- the combo -- and anything else is the singular one. Both are
 * change_anim1a, which is what lets a mashed punch replay: the port
 * played dnk_combo_uppercut_to_head_anim for the super punch instead,
 * a label that belongs to the dnk_hdhold_combo monitor at :954 and
 * appears nowhere in this routine.
 */
static void test_head_hold_punch_picks_by_stick(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);

    actors(&me, &him, WM_ROSTER_DOINK);
    me.player_mode = WM_PMODE_HEADHOLD;
    him.player_mode = WM_PMODE_HEADHELD;
    me.who_i_hit = &him;
    me.but_val_down = WM_BTN_PUNCH;
    me.stick_val_cur = 0;                        /* not toward him */
    (void)wm_arcade_move_doink(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "dnk_uppercut_to_head_anim") == 0);
    assert(guarded == NULL);
}

/*
 * :3451 #super_punch is do_pile, and USR_VAR2 -- the repeated-uppercut
 * flag -- gates the whole routine: clear, it falls on #z and does
 * NOTHING. Set, the stick decides: DOWN is the pile driver, anything
 * else falls through into #punch.
 */
static void test_do_pile_is_gated_on_the_combo_flag(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);

    actors(&me, &him, WM_ROSTER_DOINK);
    me.player_mode = WM_PMODE_HEADHOLD;
    him.player_mode = WM_PMODE_HEADHELD;
    me.who_i_hit = &him;
    me.but_val_down = WM_BTN_SPUNCH;
    me.stick_val_cur = WM_MOVE_DOWN;
    me.usr_var2 = 0;
    (void)wm_arcade_move_doink(&me, &him, &e, &c);
    assert(guarded == NULL && restarted == NULL && sound == NULL);

    actors(&me, &him, WM_ROSTER_DOINK);
    me.player_mode = WM_PMODE_HEADHOLD;
    him.player_mode = WM_PMODE_HEADHELD;
    me.who_i_hit = &him;
    me.but_val_down = WM_BTN_SPUNCH;
    me.stick_val_cur = WM_MOVE_DOWN;
    me.usr_var2 = 1;
    (void)wm_arcade_move_doink(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "dnk_3_pile_driver_anim") == 0);

    /* Stick not DOWN: falls into #punch. */
    actors(&me, &him, WM_ROSTER_DOINK);
    me.player_mode = WM_PMODE_HEADHOLD;
    him.player_mode = WM_PMODE_HEADHELD;
    me.who_i_hit = &him;
    me.but_val_down = WM_BTN_SPUNCH;
    me.stick_val_cur = 0;
    me.usr_var2 = 1;
    (void)wm_arcade_move_doink(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "dnk_uppercut_to_head_anim") == 0);
}

/*
 * :3467 #kick/#punchkick is the knee to the head with a KICK; :3477
 * #super_kick is the PLURAL knees and only when the stick is held
 * toward him, otherwise #z and nothing. The port answered all three
 * of punch, kick and punchkick with one label and played no sound at
 * all in this mode.
 */
static void test_head_hold_kicks(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    memset(&e, 0, sizeof e);

    actors(&me, &him, WM_ROSTER_DOINK);
    me.player_mode = WM_PMODE_HEADHOLD;
    him.player_mode = WM_PMODE_HEADHELD;
    me.who_i_hit = &him;
    me.but_val_down = WM_BTN_KICK;
    (void)wm_arcade_move_doink(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "dnk_3_knee_to_head_anim") == 0);
    assert(sound && strcmp(sound, "KICK") == 0);

    /* Super kick, stick NOT toward him: #z, nothing. */
    actors(&me, &him, WM_ROSTER_DOINK);
    me.player_mode = WM_PMODE_HEADHOLD;
    him.player_mode = WM_PMODE_HEADHELD;
    me.who_i_hit = &him;
    me.but_val_down = WM_BTN_SKICK;
    me.stick_val_cur = 0;
    (void)wm_arcade_move_doink(&me, &him, &e, &c);
    assert(guarded == NULL && restarted == NULL && sound == NULL);

    /* Toward him: the plural knees. */
    actors(&me, &him, WM_ROSTER_DOINK);
    me.player_mode = WM_PMODE_HEADHOLD;
    him.player_mode = WM_PMODE_HEADHELD;
    me.who_i_hit = &him;
    me.but_val_down = WM_BTN_SKICK;
    me.stick_val_cur = (uint16_t)(me.new_facing_dir & 0x0c);
    (void)wm_arcade_move_doink(&me, &him, &e, &c);
    assert(restarted && strcmp(restarted, "dnk_3_knees_to_head_anim") == 0);
    assert(sound && strcmp(sound, "KICK") == 0);
}

/*
 * mode_headhold's own #action_table sends #block, #graboh and #z to one
 * `rets`, so three of the eight actions do nothing at all in this mode.
 */
static void test_head_hold_block_and_graboh_do_nothing(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_roster_callbacks_t c = cbs();
    wm_arcade_roster_env_t e;
    unsigned b;
    static const unsigned nothing[] = { 0, 2, 3, 6, 7, 10, 11, 14, 15,
                                        18, 19, 20, 21, 22, 23, 26, 27,
                                        28, 29, 30, 31 };
    memset(&e, 0, sizeof e);

    for (b = 0; b < sizeof nothing / sizeof nothing[0]; ++b) {
        actors(&me, &him, WM_ROSTER_DOINK);
        me.player_mode = WM_PMODE_HEADHOLD;
        him.player_mode = WM_PMODE_HEADHELD;
        me.who_i_hit = &him;
        me.but_val_down = (uint16_t)nothing[b];
        (void)wm_arcade_move_doink(&me, &him, &e, &c);
        assert(guarded == NULL);
        assert(restarted == NULL);
        assert(sound == NULL);
    }
}

int main(void) {
    test_block_takes_the_guarded_entry();
    test_one_animation_two_entry_points();
    test_the_hair_grab_split_is_per_wrestler();
    test_the_uppercut_differs_between_doink_and_bret();
    test_leaving_a_head_hold_sets_the_facing();
    test_head_hold_reads_who_i_hit();
    test_head_hold_punch_picks_by_stick();
    test_do_pile_is_gated_on_the_combo_flag();
    test_head_hold_kicks();
    test_head_hold_block_and_graboh_do_nothing();
    printf("change_anim seam + mode_headhold tests passed\n");
    return 0;
}
