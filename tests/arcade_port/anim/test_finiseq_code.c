/*
 * FINISEQ.ASM's three ANI_CODE routines, which every wrestler's
 * stand_anim and fdizzy_anim run -- the animations the Undertaker's
 * coffin finish plays over a dead opponent.
 */
#include "wm/anim_program.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wmania_ring_geometry.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_check_roll_reports_backwards(void)
{
    wm_arcade_actor_t a;

    /*
     * The animation spins on it:
     *   #rl_loop  ANI_PAUSE,1 / ANI_CODE check_roll / ANI_IFSTATUS #rl_loop
     * so a SET MODE_STATUS means "still rolling, come back" -- the
     * opposite of the usual "found it" reading.
     */
    memset(&a, 0, sizeof(a));
    a.z_int = WM_RING_Z_CENTER;          /* not far enough down yet */
    assert(wm_anim_code_run(&a, NULL, "check_roll", "FINISEQ.ASM", 0));
    assert((a.anim_mode & WM_MODE_STATUS) != 0);

    /* Past the threshold: flag clear, and the loop falls out. */
    memset(&a, 0, sizeof(a));
    a.z_int = WM_RING_Z_CENTER + 21;
    a.anim_mode = (uint16_t)WM_MODE_STATUS;
    assert(wm_anim_code_run(&a, NULL, "check_roll", "FINISEQ.ASM", 0));
    assert((a.anim_mode & WM_MODE_STATUS) == 0);

    /* Exactly at the threshold still rolls -- `jrgt` is strict. */
    memset(&a, 0, sizeof(a));
    a.z_int = WM_RING_Z_CENTER + 20;
    assert(wm_anim_code_run(&a, NULL, "check_roll", "FINISEQ.ASM", 0));
    assert((a.anim_mode & WM_MODE_STATUS) != 0);
}

static void test_check_roll_steers_by_joystick(void)
{
    /*
     * The dead man is rolled by FORCING MOVE_DOWN into his
     * STICK_VAL_CUR and calling do_roll -- the finish drives a corpse
     * through the same input path a player would use.
     */
    wm_arcade_actor_t a;

    memset(&a, 0, sizeof(a));
    a.z_int = WM_RING_Z_CENTER;
    assert((a.stick_val_cur & WM_MOVE_DOWN) == 0);
    assert(wm_anim_code_run(&a, NULL, "check_roll", "FINISEQ.ASM", 0));
    assert((a.stick_val_cur & WM_MOVE_DOWN) != 0);

    /* It ORs, so whatever else was held survives. */
    memset(&a, 0, sizeof(a));
    a.z_int = WM_RING_Z_CENTER;
    a.stick_val_cur = (uint16_t)WM_MOVE_LEFT;
    assert(wm_anim_code_run(&a, NULL, "check_roll", "FINISEQ.ASM", 0));
    assert(a.stick_val_cur == (uint16_t)(WM_MOVE_LEFT | WM_MOVE_DOWN));
}

static void test_guy_is_up(void)
{
    wm_arcade_actor_t a;
    memset(&a, 0, sizeof(a));
    assert(wm_anim_code_run(&a, NULL, "guy_is_up", "FINISEQ.ASM", 0));
    assert(a.status_flags & WM_STATUS_GUY_UP);
}

static void test_the_facing_pair_forces_rather_than_flips(void)
{
    /*
     * FINISEQ.ASM:1018 and :1032 are exact mirrors, and neither
     * toggles: each ORs one pair of direction bits in and masks the
     * other pair out. Running either twice is the same as once, which
     * is what the coffin sequence needs -- both men must end up
     * facing each other however they arrived.
     */
    wm_arcade_actor_t a;
    int32_t after;

    memset(&a, 0, sizeof(a));
    a.facing_dir = WM_MOVE_RIGHT | WM_MOVE_UP;
    assert(wm_anim_code_run(&a, NULL, "adjust_facing", "FINISEQ.ASM", 0));
    assert(a.facing_dir == (WM_MOVE_LEFT | WM_MOVE_DOWN));
    assert(a.obj_control & WM_OBJ_FLIPH);

    after = a.facing_dir;
    assert(wm_anim_code_run(&a, NULL, "adjust_facing", "FINISEQ.ASM", 0));
    assert(a.facing_dir == after);             /* idempotent */
    assert(a.obj_control & WM_OBJ_FLIPH);

    /* The Undertaker's is the mirror image. */
    memset(&a, 0, sizeof(a));
    a.facing_dir = WM_MOVE_LEFT | WM_MOVE_DOWN;
    a.obj_control = (uint16_t)WM_OBJ_FLIPH;
    assert(wm_anim_code_run(&a, NULL, "adjust_taker_facing", "FINISEQ.ASM", 0));
    assert(a.facing_dir == (WM_MOVE_RIGHT | WM_MOVE_UP));
    assert(!(a.obj_control & WM_OBJ_FLIPH));

    assert(wm_anim_code_run(&a, NULL, "adjust_taker_facing", "FINISEQ.ASM", 0));
    assert(a.facing_dir == (WM_MOVE_RIGHT | WM_MOVE_UP));

    /* And they really are opposites: one undoes the other. */
    assert(wm_anim_code_run(&a, NULL, "adjust_facing", "FINISEQ.ASM", 0));
    assert(a.facing_dir == (WM_MOVE_LEFT | WM_MOVE_DOWN));
}

int main(void)
{
    test_check_roll_reports_backwards();
    test_check_roll_steers_by_joystick();
    test_guy_is_up();
    test_the_facing_pair_forces_rather_than_flips();
    printf("FINISEQ ANI_CODE routines: all checks passed\n");
    return 0;
}
