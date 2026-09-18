/*
 * The three PLYRMODE handlers every wrestler shares, and the three
 * dispatcher seams that had nothing behind them.
 *
 *   mode_puppet   ;20  DOINK.ASM:3550  "(used by everyone)"
 *   mode_inair2   ;21  DOINK.ASM:3611
 *   mode_choking  ;25  TAKER.ASM:3110
 *
 * All eight dispatchers called these three callbacks and neither
 * backend supplied any of them. That is worse than it sounds because,
 * unlike keep_attached and master_keep_attached, none of the three has
 * a fallback behind the NULL check -- so a wrestler who entered any of
 * these modes was simply never ticked again, and nothing else in the
 * game takes him out of them.
 *
 * Two of the three exist precisely to stop that happening. mode_puppet
 * is a watchdog whose own comment is "bark! Been here too long" and
 * mode_choking is a bail-out; the port had the way in and neither way
 * out.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/arcade/wm_arcade_modes.h"
#include "wm/arcade/wm_arcade_roster_anims.h"
#include "wm/wrestler_backend.h"
#include "wm/bret_backend.h"

/* A real actor: active, with a facing, standing somewhere. */
static void actor(wm_arcade_actor_t *a, int wrestler) {
    memset(a, 0, sizeof *a);
    a->active = 1;
    a->wrestler_num = wrestler;
    a->facing_dir = WM_MOVE_DOWN_RIGHT;     /* MOVE_UP clear -> column 1 */
    a->x_int = 1000; a->x_fixed = 1000 << 16;
    a->z_int = 1100; a->z_fixed = 1100 << 16;
}

/*
 * mode_puppet's guard, and the thing that reads backwards: a NULL
 * ATTACH_PROC is NOT the safe case. Only "attached, and he points
 * back" is.
 */
static void test_the_puppet_guard_is_mutual(void) {
    wm_arcade_actor_t me, master;
    wm_mode_puppet_result_t r;

    actor(&me, 0);
    actor(&master, 2);
    me.attach_proc = &master;
    master.attach_proc = &me;               /* mutual -- he is driven */

    r = wm_arcade_mode_puppet(&me, 500u);
    assert(!r.glitched_to_stand);
    /* It returned before `#check`, so it did not even stamp the clock. */
    assert(me.puppet_time == 0);
    assert(me.puppet_ticks == 0);

    /* One-sided: the master has let go and is pointing elsewhere. This
       is the stranding the watchdog is for, and it counts. */
    master.attach_proc = NULL;
    r = wm_arcade_mode_puppet(&me, 500u);
    assert(!r.glitched_to_stand);
    assert(me.puppet_time == 500);
    assert(me.puppet_ticks == 1);

    /* No attachment at all counts the same way. */
    actor(&me, 0);
    r = wm_arcade_mode_puppet(&me, 77u);
    assert(!r.glitched_to_stand);
    assert(me.puppet_ticks == 1);
}

/*
 * The count is a RUN length, not a total: only a PCNT exactly one
 * greater than the last continues it.
 */
static void test_the_puppet_count_is_a_run(void) {
    wm_arcade_actor_t me;
    uint32_t t;

    actor(&me, 0);
    for (t = 100u; t < 110u; ++t) {
        wm_mode_puppet_result_t r = wm_arcade_mode_puppet(&me, t);
        assert(!r.glitched_to_stand);
    }
    assert(me.puppet_ticks == 10);
    assert(me.puppet_time == 109);

    /* A skipped tick -- he was something else in between -- restarts. */
    (void)wm_arcade_mode_puppet(&me, 120u);
    assert(me.puppet_ticks == 1);

    /* And so does the same tick twice, since the difference is 0. */
    (void)wm_arcade_mode_puppet(&me, 121u);
    assert(me.puppet_ticks == 2);
    (void)wm_arcade_mode_puppet(&me, 121u);
    assert(me.puppet_ticks == 1);
}

/* The bark itself: two seconds, then MODE_NORMAL and his own stand. */
static void test_the_puppet_watchdog_barks(void) {
    wm_arcade_actor_t me;
    wm_mode_puppet_result_t r;
    uint32_t t;

    assert(WM_PUPPET_TIMEOUT == 106);       /* TSEC*2, DISPLAY.EQU:46 */

    actor(&me, 0);                          /* Bret */
    me.player_mode = WM_PMODE_PUPPET;
    memset(&r, 0, sizeof r);
    for (t = 1u; t <= (uint32_t)WM_PUPPET_TIMEOUT; ++t) {
        r = wm_arcade_mode_puppet(&me, t);
        if (t < (uint32_t)WM_PUPPET_TIMEOUT) {
            assert(!r.glitched_to_stand);
            assert(me.player_mode == WM_PMODE_PUPPET);
        }
    }
    /* The 106th consecutive tick is the one that fires. */
    assert(r.glitched_to_stand);
    assert(me.player_mode == WM_PMODE_NORMAL);
    assert(r.stand_anim != NULL);
    assert(strcmp(r.stand_anim, "hrt_stand4_anim") == 0);

    /* FACE24TBL over #stand_tbl: column 0 when MOVE_UP is set. */
    actor(&me, 0);
    me.facing_dir = WM_MOVE_UP_RIGHT;
    for (t = 1u; t <= (uint32_t)WM_PUPPET_TIMEOUT; ++t)
        r = wm_arcade_mode_puppet(&me, t);
    assert(r.glitched_to_stand);
    assert(strcmp(r.stand_anim, "hrt_stand2_anim") == 0);

    /* Each wrestler gets his own, out of the real table. */
    actor(&me, 3);                          /* Yokozuna */
    for (t = 1u; t <= (uint32_t)WM_PUPPET_TIMEOUT; ++t)
        r = wm_arcade_mode_puppet(&me, t);
    assert(strcmp(r.stand_anim, "yok_stand4_anim") == 0);

    /* Adam Bomb's row is `.long 0,0`, so there is genuinely nothing to
       play -- the mode change still happens, which is the point. */
    actor(&me, WM_ROSTER_ANIM_ADAM_BOMB);
    me.player_mode = WM_PMODE_PUPPET;
    for (t = 1u; t <= (uint32_t)WM_PUPPET_TIMEOUT; ++t)
        r = wm_arcade_mode_puppet(&me, t);
    assert(r.glitched_to_stand);
    assert(r.stand_anim == NULL);
    assert(me.player_mode == WM_PMODE_NORMAL);
}

/*
 * mode_inair2: the turnbuckle float. Drift per axis, straight onto
 * POSITION -- both velocity forms are in the source and both are
 * commented out.
 */
static void test_inair2_drifts_position_not_velocity(void) {
    wm_arcade_actor_t a;
    int32_t z0, x0;

    actor(&a, 0);
    z0 = a.z_fixed; x0 = a.x_fixed;
    a.stick_val_cur = WM_MOVE_UP;
    wm_arcade_mode_inair2(&a);
    assert(a.z_fixed == z0 - WM_INAIR2_ZDRIFT);
    assert(a.z_int == a.z_fixed >> 16);     /* OBJ_ZPOSINT is its high word */
    assert(a.x_fixed == x0);                /* X untouched */
    /* The commented-out half: no velocity is written at all. */
    assert(a.z_vel == 0);
    assert(a.x_vel == 0);

    actor(&a, 0);
    a.stick_val_cur = WM_MOVE_DOWN;
    wm_arcade_mode_inair2(&a);
    assert(a.z_fixed == (1100 << 16) + WM_INAIR2_ZDRIFT);

    actor(&a, 0);
    a.stick_val_cur = WM_MOVE_LEFT;
    wm_arcade_mode_inair2(&a);
    assert(a.x_fixed == (1000 << 16) - WM_INAIR2_XDRIFT);
    assert(a.z_fixed == 1100 << 16);

    actor(&a, 0);
    a.stick_val_cur = WM_MOVE_RIGHT;
    wm_arcade_mode_inair2(&a);
    assert(a.x_fixed == (1000 << 16) + WM_INAIR2_XDRIFT);

    /* A diagonal moves both, because the two tests are independent. */
    actor(&a, 0);
    a.stick_val_cur = WM_MOVE_UP_LEFT;
    wm_arcade_mode_inair2(&a);
    assert(a.z_fixed == (1100 << 16) - WM_INAIR2_ZDRIFT);
    assert(a.x_fixed == (1000 << 16) - WM_INAIR2_XDRIFT);

    /* Up beats down and left beats right: each pair is tested in that
       order and the second is only reached when the first is clear. */
    actor(&a, 0);
    a.stick_val_cur = (uint16_t)(WM_MOVE_UP | WM_MOVE_DOWN |
                                 WM_MOVE_LEFT | WM_MOVE_RIGHT);
    wm_arcade_mode_inair2(&a);
    assert(a.z_fixed == (1100 << 16) - WM_INAIR2_ZDRIFT);
    assert(a.x_fixed == (1000 << 16) - WM_INAIR2_XDRIFT);

    /* Nothing held, nothing moves -- so the drift does not persist. */
    actor(&a, 0);
    wm_arcade_mode_inair2(&a);
    assert(a.z_fixed == 1100 << 16);
    assert(a.x_fixed == 1000 << 16);

    /* And the real per-tick magnitudes, from DOINK.ASM:3617-3618. */
    assert(WM_INAIR2_ZDRIFT == 0x58000);
    assert(WM_INAIR2_XDRIFT == 0x30000);
}

/* mode_choking: held while mutual, loose the moment it is not. */
static void test_choking_holds_then_lets_go(void) {
    wm_arcade_actor_t me, choker;
    wm_mode_choking_result_t r;

    actor(&me, 0);
    actor(&choker, 2);                      /* the Undertaker */
    me.attach_proc = &choker;
    choker.attach_proc = &me;
    me.player_mode = WM_PMODE_CHOKING;
    me.anim_mode = WM_MODE_UNINT | WM_MODE_NOGRAVITY;

    r = wm_arcade_mode_choking(&me);
    assert(!r.fell_out);
    assert(!r.kill_endless_sound);
    assert(me.attach_proc == &choker);
    assert(me.player_mode == WM_PMODE_CHOKING);
    assert(me.anim_mode == (WM_MODE_UNINT | WM_MODE_NOGRAVITY));

    /* The choker lets go -- or dies, or is pulled away. */
    choker.attach_proc = NULL;
    r = wm_arcade_mode_choking(&me);
    assert(r.fell_out);
    assert(r.kill_endless_sound);
    assert(me.attach_proc == NULL);
    assert(me.player_mode == WM_PMODE_NORMAL);
    /* `movi MODE_NORMAL,a0 / move a0,*a13(ANIMODE)` is the whole word,
       so every flag he was carrying goes with it. */
    assert(me.anim_mode == 0);

    /* Never attached at all falls out the same way, immediately. */
    actor(&me, 0);
    me.player_mode = WM_PMODE_CHOKING;
    r = wm_arcade_mode_choking(&me);
    assert(r.fell_out);
    assert(me.player_mode == WM_PMODE_NORMAL);
}

/*
 * The seams, on both backends: all three set, and driving one really
 * does take the wrestler out of the mode he was stuck in.
 */
static void test_the_three_seams_are_wired(void) {
    wm_wrestler_backend_actor st;
    wm_bret_backend_actor bva;
    wm_arcade_roster_callbacks_t roster_cb;
    wm_arcade_razor_callbacks_t razor_cb;
    wm_arcade_bret_callbacks_t bret_cb;
    wm_arcade_actor_t a;
    uint32_t t;

    memset(&st, 0, sizeof st);
    st.wrestler_num = 3;                    /* Yokozuna */
    roster_cb = wm_wrestler_roster_callbacks(&st);
    razor_cb = wm_wrestler_razor_callbacks(&st);
    wm_bret_backend_init(&bva);
    bret_cb = wm_bret_backend_callbacks(&bva);

    assert(roster_cb.mode_puppet && roster_cb.mode_inair2 &&
           roster_cb.mode_choking);
    assert(razor_cb.mode_puppet && razor_cb.mode_inair2 &&
           razor_cb.mode_choking);
    assert(bret_cb.mode_puppet && bret_cb.mode_inair2 &&
           bret_cb.mode_choking);

    /* The watchdog, through the callback, with the animation started on
       the backend's own program channel. */
    actor(&a, 3);
    a.player_mode = WM_PMODE_PUPPET;
    for (t = 1u; t <= (uint32_t)WM_PUPPET_TIMEOUT; ++t) {
        st.pcnt = t;
        roster_cb.mode_puppet(&a, roster_cb.user);
    }
    assert(a.player_mode == WM_PMODE_NORMAL);
    assert(st.current_label != NULL);
    assert(strcmp(st.current_label, "yok_stand4_anim") == 0);

    /* The float and the bail-out, through theirs. */
    actor(&a, 3);
    a.stick_val_cur = WM_MOVE_DOWN;
    roster_cb.mode_inair2(&a, roster_cb.user);
    assert(a.z_fixed == (1100 << 16) + WM_INAIR2_ZDRIFT);

    actor(&a, 3);
    a.player_mode = WM_PMODE_CHOKING;
    a.anim_mode = WM_MODE_UNINT;
    roster_cb.mode_choking(&a, roster_cb.user);
    assert(a.player_mode == WM_PMODE_NORMAL);
    assert(a.anim_mode == 0);

    /* Bret's, which route the stand animation the other way. */
    actor(&a, 0);
    a.player_mode = WM_PMODE_PUPPET;
    for (t = 1u; t <= (uint32_t)WM_PUPPET_TIMEOUT; ++t) {
        bva.round_tickcount = (uint16_t)t;
        bret_cb.mode_puppet(&a, bret_cb.user);
    }
    assert(a.player_mode == WM_PMODE_NORMAL);

    actor(&a, 0);
    a.player_mode = WM_PMODE_CHOKING;
    bret_cb.mode_choking(&a, bret_cb.user);
    assert(a.player_mode == WM_PMODE_NORMAL);
}

int main(void) {
    test_the_puppet_guard_is_mutual();
    test_the_puppet_count_is_a_run();
    test_the_puppet_watchdog_barks();
    test_inair2_drifts_position_not_velocity();
    test_choking_holds_then_lets_go();
    test_the_three_seams_are_wired();
    printf("shared modes ok\n");
    return 0;
}
