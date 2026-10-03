/*
 * FINISEQ.ASM's Undertaker coffin sequence -- the state machine, the
 * velocities, the ramps and the draws. Every expectation below is the
 * source's arithmetic worked through by hand, not the port's output
 * recorded back.
 */
#include <assert.h>
#include <string.h>

#include "wm_arcade_coffin.h"
#include "wm/anim_program.h"
#include "wm/anim_puppet.h"
#include "wm_arcade_roster.h"

/* ---- the handshake ---------------------------------------------- */

static void test_handshake(void) {
    wm_coffin_state_t st;
    const char *anim = (const char *)1;

    memset(&st, 0xAB, sizeof st);
    wm_coffin_reset(&st);
    assert(st.close_the_door == 0);
    assert(st.close_the_floor == 0);
    assert(!st.guy_in);
    assert(st.finish_completed == 0);

    /* is_door_open is `jrz`, so every non-zero state reads as open --
       including 2 ("please close it") and 3 ("shut"). */
    assert(!wm_coffin_is_door_open(&st));
    st.close_the_door = 1;
    assert(wm_coffin_is_door_open(&st));
    st.close_the_door = 3;
    assert(wm_coffin_is_door_open(&st));

    /* close_door stores exactly 2, whatever was there. */
    wm_coffin_close_door(&st);
    assert(st.close_the_door == 2);

    assert(!wm_coffin_is_he_in(&st));
    wm_coffin_guy_is_in(&st);
    assert(wm_coffin_is_he_in(&st));

    /* make_wres_disappear answers only at exactly 3. */
    st.close_the_door = 2;
    assert(!wm_coffin_make_wres_disappear(&st, &anim));
    assert(anim == NULL);
    st.close_the_door = 3;
    assert(wm_coffin_make_wres_disappear(&st, &anim));
    assert(anim && strcmp(anim, "disappear_wrestler") == 0);

    /* A NULL state -- no finish running -- answers no to everything
       rather than faulting, which is what BSS zeroed to anyway. */
    assert(!wm_coffin_is_door_open(NULL));
    assert(!wm_coffin_is_he_in(NULL));
    assert(!wm_coffin_make_wres_disappear(NULL, &anim));
}

static void test_guy_up(void) {
    wm_arcade_actor_t dead;
    memset(&dead, 0, sizeof dead);
    assert(!wm_coffin_is_guy_up(&dead));
    dead.status_flags |= WM_STATUS_GUY_UP;
    assert(wm_coffin_is_guy_up(&dead));
    assert(!wm_coffin_is_guy_up(NULL));
}

/* ---- set_speeds -------------------------------------------------- */

static void test_set_speeds(void) {
    wm_arcade_actor_t dead;

    /* The equates the source computes, checked here so that a change to
       RING_Z_CENTER cannot silently move the coffin. */
    assert(WM_COFFIN_MAT_BACK_Z == 0x14b4);
    assert(WM_COFFIN_BACK_Z == 0x14d0);
    assert(WM_COFFIN_WRES_Z == 0x4d0);
    assert(WM_COFFIN_XPOS == 1257);

    /* Coming from the left of the coffin, in front of WRES_Z. */
    memset(&dead, 0, sizeof dead);
    dead.x_int = 1157;               /* 100 pixels short */
    dead.z_int = WM_COFFIN_WRES_Z + 32;
    wm_coffin_set_speeds(&dead);
    /* (1257-1157) << 16 / 16 == 100/16 of a pixel per tick. */
    assert(dead.x_vel == (100 << 16) / 16);
    /* Z runs BACKWARDS to WRES_Z: -32 pixels over 16 ticks. */
    assert(dead.z_vel == -(32 << 16) / 16);
    assert(dead.z_int == WM_COFFIN_WRES_Z + 32);   /* not clamped */

    /* Coming from the right: a negative X velocity, and the divide
       truncates toward zero the way DIVS does. */
    memset(&dead, 0, sizeof dead);
    dead.x_int = 1300;
    dead.z_int = WM_COFFIN_WRES_Z;
    wm_coffin_set_speeds(&dead);
    assert(dead.x_vel == -(43 << 16) / 16);
    assert(dead.z_vel == 0);

    /*
     * Behind WRES_Z: the source SNAPS his Z first and then measures
     * from the snapped value, so he teleports the whole distance in
     * one tick and the velocity it computes is zero.
     */
    memset(&dead, 0, sizeof dead);
    dead.x_int = WM_COFFIN_XPOS;
    dead.z_int = WM_COFFIN_WRES_Z - 60;
    dead.z_fixed = (int32_t)(WM_COFFIN_WRES_Z - 60) << 16;
    wm_coffin_set_speeds(&dead);
    assert(dead.x_int == WM_COFFIN_XPOS);
    assert(dead.x_vel == 0);
    assert(dead.z_int == WM_COFFIN_WRES_Z);
    assert(dead.z_fixed == (int32_t)WM_COFFIN_WRES_Z << 16);
    assert(dead.z_vel == 0);

    wm_coffin_set_speeds(NULL);      /* must not fault */
}

/* ---- the rise and the fall --------------------------------------- */

static void test_ramps(void) {
    int32_t sizey = WM_COFFIN_VEL;   /* `movk COFFIN_VEL,a14` at :723 */
    int32_t ypos = 1000;
    int steps = 0;

    while (wm_coffin_rise_step(&sizey, &ypos)) {
        ++steps;
        assert(steps < 1000);
    }
    /* 4 -> 136 in steps of 4 is 33 steps, and the top edge has moved
       the same 132 pixels upward. */
    assert(steps == 33);
    assert(sizey == WM_COFFIN_FULL_SIZEY);
    assert(ypos == 1000 - 132);

    /* Down again: `cmpi 1 / jrle`, so it stops at 0 after 34 steps. */
    sizey = WM_COFFIN_FULL_SIZEY;
    ypos = 1000;
    steps = 0;
    while (wm_coffin_fall_step(&sizey, &ypos)) {
        ++steps;
        assert(steps < 1000);
    }
    assert(steps == 34);
    assert(sizey == 0);
    assert(ypos == 1000 + 136);

    /* The rise tests equality, so a size that is not on the 4-grid
       would never terminate -- recorded here because it is the
       source's own fragility, and the one caller does start at 4. */
    sizey = 3;
    ypos = 0;
    assert(wm_coffin_rise_step(&sizey, &ypos));
    assert(sizey == 7);
}

/* ---- hover_coffin ------------------------------------------------ */

static void test_hover(void) {
    int32_t delta = WM_COFFIN_HOVER_DOWN;
    int32_t back = 500, front = 600;
    const int32_t front0 = 600;
    int puffs;

    /* `not 1` is -2, not the -1 the comment beside it claims. */
    assert(WM_COFFIN_HOVER_UP == -2);

    /* +1, +2, +3: the first two are inside the +-2 band, the third
       breaks out and turns the delta around with eight puffs. */
    puffs = wm_coffin_hover_step(&delta, &back, &front, front0);
    assert(puffs == 0 && front == 601 && back == 501 && delta == 1);
    puffs = wm_coffin_hover_step(&delta, &back, &front, front0);
    assert(puffs == 0 && front == 602 && delta == 1);
    puffs = wm_coffin_hover_step(&delta, &back, &front, front0);
    assert(front == 603);
    assert(puffs == WM_COFFIN_HOVER_PUFFS);
    assert(delta == -2);

    /* Back up in twos: 601, 599 (still inside), 597 -- that breaks out
       downward and resets to +1, and makes NO smoke. */
    puffs = wm_coffin_hover_step(&delta, &back, &front, front0);
    assert(puffs == 0 && front == 601 && delta == -2);
    puffs = wm_coffin_hover_step(&delta, &back, &front, front0);
    assert(puffs == 0 && front == 599 && delta == -2);
    puffs = wm_coffin_hover_step(&delta, &back, &front, front0);
    assert(front == 597);
    assert(puffs == 0);
    assert(delta == WM_COFFIN_HOVER_DOWN);

    /* Both objects always move together by the same delta, which is
       what keeps the two halves of the coffin in register. */
    assert(back - 500 == front - front0);
}

/* ---- the shake and the puffs ------------------------------------- */

/* The RNG needs real HCOUNT/SP entropy or it degenerates -- see the
   warning in wm/arcade/wmania_rng.h. These two stand in for it. */
static uint32_t test_hcount(void *user) {
    uint32_t *t = (uint32_t *)user;
    return (*t += 0x0139u) & 0x1ffu;
}
static uint32_t test_sp(void *user) {
    uint32_t *t = (uint32_t *)user;
    return 0x01000000u + ((*t * 7u) & 0x3fffu);
}

static void test_shake_and_puffs(void) {
    WmRng rng;
    uint32_t tick = 1;
    int i;
    int saw_x_low = 0, saw_x_high = 0, saw_y_zero = 0;

    wm_rng_init(&rng, 0x12345678u, test_hcount, test_sp, &tick);
    for (i = 0; i < 4000; ++i) {
        int32_t x = 0, y = 0;
        wm_coffin_shake_offset(&rng, 1257, 300, &x, &y);
        /* X spans -2..+2 about the base; Y only -2..0. */
        assert(x >= 1255 && x <= 1259);
        assert(y >= 298 && y <= 300);
        if (x == 1255) saw_x_low = 1;
        if (x == 1259) saw_x_high = 1;
        if (y == 300) saw_y_zero = 1;
    }
    assert(saw_x_low && saw_x_high && saw_y_zero);

    tick = 1;
    wm_rng_init(&rng, 0x2468ACE0u, test_hcount, test_sp, &tick);
    for (i = 0; i < 4000; ++i) {
        wm_coffin_puff_t p;
        wm_coffin_ltl_exp_params(&rng, &p);
        assert(p.delay >= 1 && p.delay <= 11);
        assert(p.x >= WM_COFFIN_HOLE_XPOS - 40 &&
               p.x <= WM_COFFIN_HOLE_XPOS + 40);
        /* RNDRNG0(30) - 25: mostly above the hole, up to 5 below. */
        assert(p.y >= WM_COFFIN_HOLE_YPOS - 25 &&
               p.y <= WM_COFFIN_HOLE_YPOS + 5);
        assert(p.z >= WM_COFFIN_EXP_Z && p.z <= WM_COFFIN_EXP_Z + 10);
        assert(p.vel >= 2 && p.vel <= 9);
        assert(p.which == 0 || p.which == 1);
    }
}

/* ---- the frame lists --------------------------------------------- */

static void test_frames(void) {
    assert(wm_coffin_mat_frame_count == 4);
    assert(strcmp(wm_coffin_mat_frames[0], "MATCOF01") == 0);
    assert(strcmp(wm_coffin_mat_frames[3], "MATCOF04") == 0);

    /* #fc_loop and #close_loop walk the SAME list backwards from the
       zero terminator that follows it -- there is no second list. */
    assert(strcmp(wm_coffin_close_frame(wm_coffin_mat_frames,
                                        wm_coffin_mat_frame_count, 0),
                  "MATCOF04") == 0);
    assert(strcmp(wm_coffin_close_frame(wm_coffin_mat_frames,
                                        wm_coffin_mat_frame_count, 3),
                  "MATCOF01") == 0);
    assert(wm_coffin_close_frame(wm_coffin_mat_frames,
                                 wm_coffin_mat_frame_count, 4) == NULL);

    assert(wm_coffin_door_frame_count == 4);
    assert(strcmp(wm_coffin_door_frames[0], "COFFIN02") == 0);
    assert(strcmp(wm_coffin_door_frames[3], "COFFIN05") == 0);
    /* The close ends on COFFIN02, which is the frame that also drops
       the front half and moves the Z in front of the wrestler. */
    assert(strcmp(wm_coffin_close_frame(wm_coffin_door_frames,
                                        wm_coffin_door_frame_count, 3),
                  "COFFIN02") == 0);

    /* The shipped tombstone starts at TMBSTN02: `movi #tstone_anim,a9`
       is commented out at FINISEQ.ASM:668 in favour of #tstone_test,
       and #tstone_test begins one frame in. */
    assert(wm_coffin_tstone_frame_count == 7);
    assert(strcmp(wm_coffin_tstone_frames[0], "TMBSTN02") == 0);
    assert(strcmp(WM_COFFIN_TSTONE_UNUSED_FIRST, "TMBSTN01") == 0);

    assert(wm_coffin_exp_frame_count == 10);
    assert(strcmp(wm_coffin_exp1_frames[9], "SMOKE10") == 0);
    assert(strcmp(wm_coffin_exp2_frames[0], "SMOKEB01") == 0);
}

/* ---- the ANI_CODE seam ------------------------------------------- */

static wm_arcade_actor_t *g_opp;
static const char *g_opp_label;
static void record_opp_anim(wm_arcade_actor_t *opp, const char *label,
                            void *user) {
    (void)user;
    g_opp = opp;
    g_opp_label = label;
}

static void test_ani_code(void) {
    wm_arcade_actor_t taker, dead;
    wm_coffin_state_t st;
    wm_anim_env env;

    memset(&taker, 0, sizeof taker);
    memset(&dead, 0, sizeof dead);
    memset(&env, 0, sizeof env);
    wm_coffin_reset(&st);
    env.coffin = &st;
    env.change_opp_anim = record_opp_anim;
    taker.who_i_hit = &dead;
    taker.wrestler_num = WM_ROSTER_TAKER;
    dead.wrestler_num = WM_ROSTER_RAZOR;

    /* is_door_open: MODE_STATUS off, then on once the door opens. */
    taker.anim_mode = (uint16_t)WM_MODE_STATUS;
    assert(wm_anim_code_run(&taker, &env, "is_door_open", "FINISEQ.ASM", 0));
    assert((taker.anim_mode & WM_MODE_STATUS) == 0);
    st.close_the_door = 1;
    assert(wm_anim_code_run(&taker, &env, "is_door_open", "FINISEQ.ASM", 0));
    assert(taker.anim_mode & WM_MODE_STATUS);

    /* is_guy_up reads the flag on WHOIHIT, not on the caller. */
    taker.anim_mode = 0;
    assert(wm_anim_code_run(&taker, &env, "is_guy_up", "FINISEQ.ASM", 0));
    assert((taker.anim_mode & WM_MODE_STATUS) == 0);
    assert(wm_anim_code_run(&dead, &env, "guy_is_up", "FINISEQ.ASM", 0));
    assert(dead.status_flags & WM_STATUS_GUY_UP);
    assert(wm_anim_code_run(&taker, &env, "is_guy_up", "FINISEQ.ASM", 0));
    assert(taker.anim_mode & WM_MODE_STATUS);

    /* close_door then guy_is_in, from the two different animations. */
    assert(wm_anim_code_run(&taker, &env, "close_door", "FINISEQ.ASM", 0));
    assert(st.close_the_door == 2);
    assert(wm_anim_code_run(&dead, &env, "guy_is_in", "FINISEQ.ASM", 0));
    assert(st.guy_in);
    taker.anim_mode = 0;
    assert(wm_anim_code_run(&taker, &env, "is_he_in", "FINISEQ.ASM", 0));
    assert(taker.anim_mode & WM_MODE_STATUS);

    /* push_to_coffin starts push_in_anim on the dead man. */
    g_opp = NULL;
    g_opp_label = NULL;
    assert(wm_anim_code_run(&taker, &env, "push_to_coffin", "FINISEQ.ASM", 0));
    assert(g_opp == &dead);
    assert(g_opp_label && strcmp(g_opp_label, "push_in_anim") == 0);

    /* make_wres_disappear: nothing at 2, everything at 3. */
    g_opp = NULL;
    g_opp_label = NULL;
    taker.anim_mode = (uint16_t)WM_MODE_STATUS;
    assert(wm_anim_code_run(&taker, &env, "make_wres_disappear",
                            "FINISEQ.ASM", 0));
    assert((taker.anim_mode & WM_MODE_STATUS) == 0);
    assert(g_opp_label == NULL);
    st.close_the_door = 3;
    assert(wm_anim_code_run(&taker, &env, "make_wres_disappear",
                            "FINISEQ.ASM", 0));
    assert(taker.anim_mode & WM_MODE_STATUS);
    assert(g_opp == &dead);
    assert(g_opp_label && strcmp(g_opp_label, "disappear_wrestler") == 0);

    /* set_speeds runs on the caller: it is push_in_anim's own opcode. */
    dead.x_int = 1157;
    dead.z_int = WM_COFFIN_WRES_Z;
    assert(wm_anim_code_run(&dead, &env, "set_speeds", "FINISEQ.ASM", 0));
    assert(dead.x_vel == (100 << 16) / 16);

    /* stand_wrestler / dizzy_wrestler index FINISEQ's two FACETBLs by
       the caller's own wrestler number and ask for a self-change. */
    dead.change_anim_label = NULL;
    assert(wm_anim_code_run(&dead, &env, "stand_wrestler", "FINISEQ.ASM", 0));
    assert(dead.change_anim_label &&
           strcmp(dead.change_anim_label, "rzr_stand_anim") == 0);
    dead.change_anim_label = NULL;
    assert(wm_anim_code_run(&dead, &env, "dizzy_wrestler", "FINISEQ.ASM", 0));
    assert(dead.change_anim_label &&
           strcmp(dead.change_anim_label, "rzr_fdizzy_anim") == 0);

    /*
     * @dead_wrestler is a latch, not a re-read: once a finish sets it,
     * the routines follow the latch even if the Undertaker's WHOIHIT
     * moves. Until one does, WHOIHIT is the answer -- which is exactly
     * where TAKER.ASM:661 latched it from.
     */
    {
        wm_arcade_actor_t other;
        memset(&other, 0, sizeof other);
        assert(wm_coffin_dead_wrestler(&st, &taker) == &dead);
        st.dead_wrestler = &other;
        assert(wm_coffin_dead_wrestler(&st, &taker) == &other);
        g_opp = NULL;
        assert(wm_anim_code_run(&taker, &env, "push_to_coffin",
                                "FINISEQ.ASM", 0));
        assert(g_opp == &other);
        /* is_guy_up follows the latch too. */
        other.status_flags |= WM_STATUS_GUY_UP;
        taker.anim_mode = 0;
        assert(wm_anim_code_run(&taker, &env, "is_guy_up", "FINISEQ.ASM", 0));
        assert(taker.anim_mode & WM_MODE_STATUS);
        st.dead_wrestler = NULL;
    }

    /* The Undertaker's own rows, since his is the finish that ships. */
    taker.change_anim_label = NULL;
    assert(wm_anim_code_run(&taker, &env, "stand_wrestler", "FINISEQ.ASM", 0));
    assert(taker.change_anim_label &&
           strcmp(taker.change_anim_label, "und_stand_anim") == 0);
}

/*
 * The animations the sequence hands around have to exist, or the
 * handshake above points at nothing. disappear_wrestler in particular
 * is named by no table and no opcode -- only by make_wres_disappear's
 * `movi disappear_wrestler,a0 / calla change_anim1a`.
 */
static void test_programs_exist(void) {
    static const char *const needed[] = {
        "und_2_raise_dead_anim", "raise_dead_anim", "push_in_anim",
        "disappear_wrestler", "und_stand_anim", "und_fdizzy_anim",
    };
    size_t i;
    for (i = 0; i < sizeof needed / sizeof needed[0]; ++i)
        assert(wm_anim_program_find(needed[i]) != NULL);

    /* Every roster row of both FACETBLs resolves, except the cut
       seventh slot, which is a literal 0 in the source. */
    for (i = 0; i < WM_ANIM_ROSTER_SLOTS; ++i) {
        const char *s = wm_anim_code_roster_label("stand_wrestler",
                                                  (int32_t)i);
        const char *d = wm_anim_code_roster_label("dizzy_wrestler",
                                                  (int32_t)i);
        if (i == 7) {
            assert(s == NULL && d == NULL);
            continue;
        }
        assert(s && wm_anim_program_find(s));
        assert(d && wm_anim_program_find(d));
    }
}

/*
 * The self-change seam, driven through the VM rather than poked at.
 * FINISEQ.ASM:1512 raise_dead_anim is the whole of it: set some modes,
 * pause half a second, then `WL ANI_CODE,stand_wrestler` -- and the
 * animation has no frames of its own, so unless that one call turns
 * into a real hand-off the dead man never stands up.
 */
static void test_self_change_through_vm(void) {
    wm_arcade_actor_t dead;
    wm_anim_env env;
    wm_anim_exec exec;
    const wm_anim_program *p = wm_anim_program_find("raise_dead_anim");
    int t;

    assert(p);
    memset(&dead, 0, sizeof dead);
    memset(&env, 0, sizeof env);
    memset(&exec, 0, sizeof exec);
    dead.wrestler_num = WM_ROSTER_RAZOR;

    wm_anim_exec_start(&exec, p, &dead, 0, &env);
    for (t = 0; t < 200 && !exec.ended; ++t)
        wm_anim_exec_tick(&exec, &dead, (uint16_t)t);

    assert(exec.ended);
    assert(exec.become && strcmp(exec.become, "rzr_stand_anim") == 0);
    /* The VM consumed the request rather than leaving it to fire again
       on whatever animation runs next. */
    assert(dead.change_anim_label == NULL);
    /* ANI_PAUSE,(TSEC/2) with DISPLAY.EQU:46's TSEC of 53 is 26 ticks,
       so the hand-off cannot have come any sooner than that. */
    assert(t >= 26);
}

int main(void) {
    test_handshake();
    test_guy_up();
    test_set_speeds();
    test_ramps();
    test_hover();
    test_shake_and_puffs();
    test_frames();
    test_ani_code();
    test_programs_exist();
    test_self_change_through_vm();
    return 0;
}
