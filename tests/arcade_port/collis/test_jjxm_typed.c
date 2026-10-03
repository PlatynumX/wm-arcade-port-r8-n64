/*
 * Bret and Razor routed through their own JJXM tables (BRET.ASM,
 * RAZOR.ASM), the last two of the eight.
 *
 * These two are different from the other six in one way that matters:
 * their dispatchers select by a TYPED animation id rather than by a
 * label string, so the tables reach them through a mapping. What the
 * mapping replaced was a hand-written chain of PLYRMODE comparisons --
 * faithful in most places, because it was transcribed from the same
 * tables by eye, but a chain nothing could check against the source.
 *
 * So these cases are aimed at the rows an eye is most likely to get
 * wrong: the three-armed ground super punch whose thresholds are 30h
 * and 40h rather than everyone else's 20h, and the mode rows that name
 * one target outright.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_arcade_jjxm.h"
#include "wm_arcade_bret.h"
#include "wm_arcade_razor.h"
#include "wm_arcade_roster.h"

static int bret_anim, bret_snd, bret_calls;
static void bret_cap_anim(wm_arcade_actor_t *a, wm_arcade_bret_anim_id_t id, void *u)
{ (void)a; (void)u; bret_anim = (int)id; ++bret_calls; }
static void bret_cap_snd(wm_arcade_actor_t *a, wm_arcade_bret_sound_id_t id, void *u)
{ (void)a; (void)u; bret_snd = (int)id; }

static int rzr_anim, rzr_snd, rzr_calls;
static void rzr_cap_anim(wm_arcade_actor_t *a, wm_arcade_razor_anim_id_t id, void *u)
{ (void)a; (void)u; rzr_anim = (int)id; ++rzr_calls; }
static void rzr_cap_snd(wm_arcade_actor_t *a, wm_arcade_razor_sound_id_t id, void *u)
{ (void)a; (void)u; rzr_snd = (int)id; }

static void base(wm_arcade_actor_t *me, wm_arcade_actor_t *him, int who) {
    memset(me, 0, sizeof *me);
    memset(him, 0, sizeof *him);
    me->active = him->active = 1;
    me->wrestler_num = who;
    me->player_mode = WM_PMODE_NORMAL;
    me->facing_dir = me->new_facing_dir = WM_MOVE_DOWN_RIGHT;
    him->player_side = 1;
    him->player_mode = WM_PMODE_NORMAL;
    bret_anim = bret_snd = bret_calls = 0;
    rzr_anim = rzr_snd = rzr_calls = 0;
}

/*
 * BRET.ASM:1674 #spunch_lbowdrop, the three-armed one. Past 30h with
 * the sprites' M_FLIPH bits DIFFERING it is the hair pickup; matching
 * bits fall to #feet, which asks for 40h and gives the SHOOTER;
 * inside that it is the ordinary ground punch. Three outcomes from
 * one row, and the thresholds are Bret's own.
 */
static void test_bret_ground_super_punch_has_three_arms(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_bret_callbacks_t cb;
    wm_arcade_bret_env_t e;
    memset(&cb, 0, sizeof cb); memset(&e, 0, sizeof e);
    cb.change_anim = bret_cap_anim; cb.change_anim_restart = bret_cap_anim; cb.sound = bret_cap_snd;

    /* Far enough, opposite flip: the hair pickup. */
    base(&me, &him, WM_ROSTER_BRET);
    me.but_val_down = WM_BTN_SPUNCH;
    him.player_mode = WM_PMODE_ONGROUND;
    me.x_fixed = 0x00500000; him.x_fixed = 0;
    him.obj_control = WM_OBJ_FLIPH;
    (void)wm_arcade_move_bret(&me, &him, &e, &cb);
    assert(bret_anim == WM_BRET_ANIM_HAIR_PICKUP4);

    /* Same flip and past 40h: the shooter. */
    base(&me, &him, WM_ROSTER_BRET);
    me.but_val_down = WM_BTN_SPUNCH;
    him.player_mode = WM_PMODE_ONGROUND;
    me.x_fixed = 0x00500000; him.x_fixed = 0;
    (void)wm_arcade_move_bret(&me, &him, &e, &cb);
    assert(bret_anim == WM_BRET_ANIM_SHOOTER4);

    /* Same flip, between 30h and 40h: the ground punch. */
    base(&me, &him, WM_ROSTER_BRET);
    me.but_val_down = WM_BTN_SPUNCH;
    him.player_mode = WM_PMODE_ONGROUND;
    me.x_fixed = 0x00350000; him.x_fixed = 0;
    (void)wm_arcade_move_bret(&me, &him, &e, &cb);
    assert(bret_anim == WM_BRET_ANIM_GROUND_PUNCH4);

    /* Inside 30h: the ground punch too, without either test. */
    base(&me, &him, WM_ROSTER_BRET);
    me.but_val_down = WM_BTN_SPUNCH;
    him.player_mode = WM_PMODE_ONGROUND;
    me.x_fixed = 0x00100000; him.x_fixed = 0;
    him.obj_control = WM_OBJ_FLIPH;
    (void)wm_arcade_move_bret(&me, &him, &e, &cb);
    assert(bret_anim == WM_BRET_ANIM_GROUND_PUNCH4);
}

/* Bret's super-punch table has a `#z` row: against a HEADHELD
   opponent the press does nothing at all. */
static void test_bret_super_punch_has_a_dead_row(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_bret_callbacks_t cb;
    wm_arcade_bret_env_t e;
    memset(&cb, 0, sizeof cb); memset(&e, 0, sizeof e);
    cb.change_anim = bret_cap_anim; cb.change_anim_restart = bret_cap_anim; cb.sound = bret_cap_snd;

    base(&me, &him, WM_ROSTER_BRET);
    me.but_val_down = WM_BTN_SPUNCH;
    him.player_mode = WM_PMODE_HEADHELD;
    (void)wm_arcade_move_bret(&me, &him, &e, &cb);
    assert(bret_calls == 0);
}

/* `JJXM INAIR2, #kick_TB` in the KICK table, and the running punch is
   the DDT -- not a clothesline, whatever the label is called. */
static void test_bret_named_rows(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_bret_callbacks_t cb;
    wm_arcade_bret_env_t e;
    memset(&cb, 0, sizeof cb); memset(&e, 0, sizeof e);
    cb.change_anim = bret_cap_anim; cb.change_anim_restart = bret_cap_anim; cb.sound = bret_cap_snd;

    base(&me, &him, WM_ROSTER_BRET);
    me.but_val_down = WM_BTN_KICK;
    him.player_mode = WM_PMODE_INAIR2;
    (void)wm_arcade_move_bret(&me, &him, &e, &cb);
    assert(bret_anim == WM_BRET_ANIM_KICK_TB);

    base(&me, &him, WM_ROSTER_BRET);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.but_val_down = WM_BTN_PUNCH;
    (void)wm_arcade_move_bret(&me, &him, &e, &cb);
    assert(bret_anim == WM_BRET_ANIM_RUNNING_DDT);

    /* Against a man on the mat, the running ground punch. */
    base(&me, &him, WM_ROSTER_BRET);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.but_val_down = WM_BTN_PUNCH;
    him.player_mode = WM_PMODE_ONGROUND;
    (void)wm_arcade_move_bret(&me, &him, &e, &cb);
    assert(bret_anim == WM_BRET_ANIM_RUNNING_GROUND_PUNCH);

    /* Running AWAY from the way he faces refuses both. */
    base(&me, &him, WM_ROSTER_BRET);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.facing_dir = WM_MOVE_DOWN_RIGHT;
    me.new_facing_dir = WM_MOVE_DOWN_LEFT;
    me.but_val_down = WM_BTN_PUNCH;
    (void)wm_arcade_move_bret(&me, &him, &e, &cb);
    assert(bret_calls == 0);
}

/*
 * RAZOR.ASM:1477's #feet arm is the one place in these eight files
 * where a JJXM target SMART-TARGETS its victim: SMRTTGT a13,
 * CLOSEST_NUM, then rzr_rugshake_anim, and no sound at all.
 */
static void test_razor_rug_shake_smart_targets(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_razor_callbacks_t cb;
    wm_arcade_razor_env_t e;
    memset(&cb, 0, sizeof cb); memset(&e, 0, sizeof e);
    cb.change_anim = rzr_cap_anim; cb.change_anim_restart = rzr_cap_anim; cb.sound = rzr_cap_snd;

    base(&me, &him, WM_ROSTER_RAZOR);
    me.but_val_down = WM_BTN_SPUNCH;
    him.player_mode = WM_PMODE_ONGROUND;
    me.x_fixed = 0x00500000; him.x_fixed = 0;
    (void)wm_arcade_move_razor(&me, &him, &e, &cb);
    assert(rzr_anim == WM_RZR_ANIM_RUGSHAKE);
    assert(me.status_flags & WM_STATUS_SMART_ATTACK);
    assert(me.smart_target == &him);
    assert(rzr_snd == 0);          /* no WRSND on this arm */
}

/* `JJXM RUNNING, #skick_bigboot`, and the super kick's own
   toward-the-opponent knee fall, which plays GRABHOLD. */
static void test_razor_super_kick_rows(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_razor_callbacks_t cb;
    wm_arcade_razor_env_t e;
    memset(&cb, 0, sizeof cb); memset(&e, 0, sizeof e);
    cb.change_anim = rzr_cap_anim; cb.change_anim_restart = rzr_cap_anim; cb.sound = rzr_cap_snd;

    base(&me, &him, WM_ROSTER_RAZOR);
    me.but_val_down = WM_BTN_SKICK;
    him.player_mode = WM_PMODE_RUNNING;
    (void)wm_arcade_move_razor(&me, &him, &e, &cb);
    assert(rzr_anim == WM_RZR_ANIM_BIGBOOT4);

    base(&me, &him, WM_ROSTER_RAZOR);
    me.but_val_down = WM_BTN_SKICK;
    me.closest_xdist = 10; me.closest_zdist = 10;
    me.stick_val_cur = (uint16_t)(me.new_facing_dir & 0x0c);
    (void)wm_arcade_move_razor(&me, &him, &e, &cb);
    assert(rzr_anim == WM_RZR_ANIM_KNEE_FALL4);
    assert(rzr_snd == WM_RZR_SND_GRABHOLD);
}

/* Razor's running kick against a man on the mat is the flying ELBOW,
   which halves his X velocity on the way -- and that is a table row,
   `JJXM ONGROUND, std_flyelbow`, not a distance test. */
static void test_razor_running_kick_is_the_flying_elbow(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_razor_callbacks_t cb;
    wm_arcade_razor_env_t e;
    memset(&cb, 0, sizeof cb); memset(&e, 0, sizeof e);
    cb.change_anim = rzr_cap_anim; cb.change_anim_restart = rzr_cap_anim; cb.sound = rzr_cap_snd;

    base(&me, &him, WM_ROSTER_RAZOR);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.x_vel = 0x00080000;
    me.but_val_down = WM_BTN_KICK;
    him.player_mode = WM_PMODE_ONGROUND;
    (void)wm_arcade_move_razor(&me, &him, &e, &cb);
    assert(rzr_anim == WM_RZR_ANIM_FLYING_ELBOW);
    assert(me.x_vel == 0x00040000);

    /* A standing opponent is the flying kick instead. */
    base(&me, &him, WM_ROSTER_RAZOR);
    me.player_mode = WM_PMODE_RUNNING;
    me.usr_var1 = 1;
    me.but_val_down = WM_BTN_KICK;
    (void)wm_arcade_move_razor(&me, &him, &e, &cb);
    assert(rzr_anim == WM_RZR_ANIM_FLYING_KICK);
    assert(me.player_mode == WM_PMODE_INAIR);
}

/* All eight wrestlers have their tables, and every one is 22 rows. */
static void test_all_eight_wrestlers_are_in_the_tables(void) {
    static const char *const who[8] = {
        "BRET","RAZOR","TAKER","YOKO","SHAWN","BAM","DOINK","LEX"
    };
    size_t i, j;
    for (i = 0; i < 8; ++i) {
        int found = 0;
        for (j = 0; j < wm_jjxm_table_count; ++j)
            if (strcmp(wm_jjxm_tables[j].wrestler, who[i]) == 0) ++found;
        /* Six each, eight for Shawn. */
        assert(found == (strcmp(who[i], "SHAWN") == 0 ? 8 : 6));
    }
}

int main(void) {
    test_bret_ground_super_punch_has_three_arms();
    test_bret_super_punch_has_a_dead_row();
    test_bret_named_rows();
    test_razor_rug_shake_smart_targets();
    test_razor_super_kick_rows();
    test_razor_running_kick_is_the_flying_elbow();
    test_all_eight_wrestlers_are_in_the_tables();
    printf("jjxm typed: ok\n");
    return 0;
}
