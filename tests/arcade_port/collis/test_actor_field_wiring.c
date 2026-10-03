/*
 * One source field, two port fields, and the writer and the reader each
 * picked a different one.
 *
 * Sweeping wm_arcade_actor_t for fields that src/ READS and never WRITES
 * turned up ten. Five were the button counters, incremented through a
 * pointer the sweep could not see. Two were boxes written field by field.
 * One is a genuine renderer gap. The other two were the same mistake
 * twice:
 *
 *   PLYR.EQU HITBLOCKER. Every one of the source's five writes is
 *   translated -- COLLIS.ASM:637 in check_collis, ANIM.ASM:445 and :968
 *   clearing it as each attack starts, REACT5.ASM:517 and :585 -- and all
 *   five write `hit_blocker`. ANIM.ASM:83's ANI_IFBLOCKED, the only
 *   reader, read `hitblocker`. So every attack animation that asks "was I
 *   blocked?" was answered no, and the field carrying the real answer was
 *   read by nobody. The header note on the dead field even explained the
 *   silence: "Nothing sets it yet -- the blocked-reaction dispatch that
 *   would is still unwired."
 *
 *   PLYR.EQU PLYR_DIZZY, the same shape with a quieter symptom. Both
 *   readers -- ANIM.ASM:3937 _ani_immobilize and REACT5.ASM:250's
 *   bounce-off -- read a duplicate `dizzy`. plyr_dizzy is genuinely always
 *   zero in the shipped game (check_dizzy is commented out), so both
 *   fields read the same value and nothing was visibly wrong. It would
 *   have stayed wrong the moment anything set the real one.
 *
 * Reading _ani_immobilize to settle which field it wanted also showed it
 * was missing half its body: the source stamps IMMOBILIZE_TIME and then
 * clears the victim's three velocities, under its own comment "clear his
 * velocities too". The port stamped the counter and let him slide.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/anim_program.h"
#include "wm/arcade/wm_arcade_combat.h"

/*
 * COLLIS.ASM:632 `clr a14 / move *a13(PLYRMODE),a0 / cmpi MODE_BLOCK,a0 /
 * jrne #noblock / inc a14 / #noblock / move a14,*a10(HITBLOCKER)`.
 *
 * a13 is the victim and a10 the attacker, so it is the ATTACKER's field
 * that records whether the man he reached was blocking.
 */
static void attacker_and_victim(wm_arcade_actor_t *a, wm_arcade_actor_t *v) {
    memset(a, 0, sizeof *a);
    memset(v, 0, sizeof *v);
    a->active = v->active = 1;
    a->anim_mode = WM_MODE_CHECKHIT;
    a->attack_mode = WM_AMODE_PUNCH;
    /* Overlapping boxes, so the collision is reached at all. */
    a->attack_box.x1 = 0;   a->attack_box.x2 = 40;
    a->attack_box.y1 = 0;   a->attack_box.y2 = 40;
    a->attack_box.z1 = 0;   a->attack_box.z2 = 40;
    v->hurt_box = a->attack_box;
    a->who_i_hit = v;
}

static void test_check_collis_records_the_block_on_the_attacker(void) {
    wm_arcade_actor_t a, v;

    attacker_and_victim(&a, &v);
    v.player_mode = WM_PMODE_BLOCK;
    (void)wm_arcade_try_attack_hit(&a, &v, NULL);
    assert(a.hit_blocker == 1);
    /* And it is the attacker's, not the victim's. */
    assert(v.hit_blocker == 0);

    attacker_and_victim(&a, &v);
    v.player_mode = WM_PMODE_NORMAL;
    /*
     * Pre-set, because `clr a14` before the test is the whole point: a
     * non-blocking victim CLEARS the field rather than leaving whatever
     * the last collision put there. Starting from a zeroed actor would
     * pass whether the clear happened or not, which is how this
     * assertion was decorative when it was first written.
     */
    a.hit_blocker = 1;
    (void)wm_arcade_try_attack_hit(&a, &v, NULL);
    assert(a.hit_blocker == 0);

    puts("  check_collis writes hit_blocker on the attacker: ok");
}

/*
 * ANIM.ASM:83 ANI_IFBLOCKED. Branch when set, fall through when clear --
 * and the field it reads has to be the one check_collis wrote.
 */
static void test_ifblocked_reads_the_field_check_collis_writes(void) {
    static const wm_anim_op ops[] = {
        { WM_AOP_IFBLOCKED, 0,  2, 0, 0, 0, 0, 0, 0, NULL },
        { WM_AOP_END,       0, -1, 0, 0, 0, 0, 0, 0, NULL },
        { WM_AOP_ZEROVELS,  0, -1, 0, 0, 0, 0, 0, 0, NULL },
        { WM_AOP_END,       0, -1, 0, 0, 0, 0, 0, 0, NULL }
    };
    static const wm_anim_program prog = {
        "test_ifblocked", "ANIM.ASM", ops, 4, 0, NULL, 0
    };
    wm_arcade_actor_t a;
    wm_anim_env env;
    wm_anim_exec ex;
    int i;

    /* Not blocked: falls through to the END at index 1, so ZEROVELS at
       index 2 never runs and the velocity stands. */
    memset(&env, 0, sizeof env);
    memset(&a, 0, sizeof a);
    a.hit_blocker = 0;
    a.x_vel = 0x1234;
    wm_anim_exec_start(&ex, &prog, &a, 0, &env);
    for (i = 0; i < 8; ++i) wm_anim_exec_tick(&ex, &a, 0);
    assert(a.x_vel == 0x1234);

    /* Blocked: branches to index 2 and ZEROVELS runs. This is the
       assertion the whole file exists for -- it fails if IFBLOCKED goes
       back to reading a field nothing writes. */
    memset(&a, 0, sizeof a);
    a.hit_blocker = 1;
    a.x_vel = 0x1234;
    wm_anim_exec_start(&ex, &prog, &a, 0, &env);
    for (i = 0; i < 8; ++i) wm_anim_exec_tick(&ex, &a, 0);
    assert(a.x_vel == 0);

    puts("  ANI_IFBLOCKED branches on hit_blocker: ok");
}

/*
 * ANIM.ASM:3933 _ani_immobilize: the PLYR_DIZZY gate, the don't-immobilize-
 * blockers gate, the IMMOBILIZE_TIME stamp, and the three velocity clears
 * that were missing.
 */
static void test_immobilize_gates_and_clears_velocities(void) {
    static const wm_anim_op ops[] = {
        { WM_AOP_IMMOBILIZE, 0, -1, 20, 0, 0, 0, 0, 0, NULL },
        { WM_AOP_END,        0, -1,  0, 0, 0, 0, 0, 0, NULL }
    };
    static const wm_anim_program prog = {
        "test_immobilize", "ANIM.ASM", ops, 2, 0, NULL, 0
    };
    wm_arcade_actor_t a, v;
    wm_anim_env env;
    wm_anim_exec ex;
    int i;

    memset(&env, 0, sizeof env);

    /* The ordinary case: stamped, and all three velocities cleared. */
    memset(&a, 0, sizeof a); memset(&v, 0, sizeof v);
    a.who_i_hit = &v;
    v.x_vel = 111; v.y_vel = 222; v.z_vel = 333;
    wm_anim_exec_start(&ex, &prog, &a, 0, &env);
    for (i = 0; i < 4; ++i) wm_anim_exec_tick(&ex, &a, 0);
    assert(v.immobilize_time == 20);
    assert(v.x_vel == 0 && v.y_vel == 0 && v.z_vel == 0);

    /* `move *a13(PLYR_DIZZY),a1 / jrnz #skip` -- a dizzy attacker
       immobilises nobody, and the field is plyr_dizzy. */
    memset(&a, 0, sizeof a); memset(&v, 0, sizeof v);
    a.who_i_hit = &v;
    a.plyr_dizzy = 1;
    v.x_vel = 111;
    wm_anim_exec_start(&ex, &prog, &a, 0, &env);
    for (i = 0; i < 4; ++i) wm_anim_exec_tick(&ex, &a, 0);
    assert(v.immobilize_time == 0 && v.x_vel == 111);

    /* "don't immobilize blockers!" -- and the velocities stand too,
       because the whole arm is skipped rather than just the stamp. */
    memset(&a, 0, sizeof a); memset(&v, 0, sizeof v);
    a.who_i_hit = &v;
    v.player_mode = WM_PMODE_BLOCK;
    v.x_vel = 111;
    wm_anim_exec_start(&ex, &prog, &a, 0, &env);
    for (i = 0; i < 4; ++i) wm_anim_exec_tick(&ex, &a, 0);
    assert(v.immobilize_time == 0 && v.x_vel == 111);

    puts("  _ani_immobilize gates on plyr_dizzy and clears velocities: ok");
}

int main(void) {
    test_check_collis_records_the_block_on_the_attacker();
    test_ifblocked_reads_the_field_check_collis_writes();
    test_immobilize_gates_and_clears_velocities();
    puts("actor field wiring tests: PASS");
    return 0;
}
