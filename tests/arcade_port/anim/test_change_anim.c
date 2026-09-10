/*
 * ANIM.ASM's four change_anim entry points, and the second animation
 * channel the port did not have.
 *
 * Every wrestler runs two animations at once -- a body and a torso --
 * and each *_ani_init starts both. Bret's hand-built backend has had a
 * second visual track all along; the shared backend the other seven use
 * had none, so seven of the eight wrestlers were running with no torso.
 */
#include "wm/wrestler_backend.h"
#include "wm/anim_program.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wm_arcade_veladd.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_ani_init_rows(void)
{
    int i;
    /* Eight wrestlers and the cut slot, in WRESTLERNUM order. */
    assert(strcmp(wm_ani_init_rows[0].stand2, "hrt_stand2_anim") == 0);
    assert(strcmp(wm_ani_init_rows[0].torso4, "hrt_torso4_anim") == 0);
    assert(strcmp(wm_ani_init_rows[8].torso2, "lex_torso2_anim") == 0);
    /* Slot 7 is Adam Bomb, cut from the game. */
    assert(wm_ani_init_rows[7].stand2 == NULL);
    assert(wm_ani_init_rows[7].torso2 == NULL);

    for (i = 0; i < WM_ANI_INIT_SLOTS; ++i) {
        const wm_ani_init_row *r = &wm_ani_init_rows[i];
        if (!r->stand2) {
            assert(i == 7);
            continue;
        }
        /* Each row is one wrestler: all four labels share a prefix, and
         * the stand/torso and 2/4 split is consistent. */
        assert(strncmp(r->stand2, r->torso4, 3) == 0);
        assert(strstr(r->stand2, "_stand2_anim") != NULL);
        assert(strstr(r->stand4, "_stand4_anim") != NULL);
        assert(strstr(r->torso2, "_torso2_anim") != NULL);
        assert(strstr(r->torso4, "_torso4_anim") != NULL);
    }
}

static void test_ani_init_picks_the_pair_by_facing(void)
{
    wm_wrestler_backend_actor st;
    wm_arcade_actor_t a;

    /* Undertaker, slot 2, facing right -- the `2` pair. */
    memset(&st, 0, sizeof(st));
    memset(&a, 0, sizeof(a));
    st.wrestler_num = 2;
    a.facing_dir = WM_MOVE_RIGHT;
    wm_wrestler_backend_ani_init(&st, &a);
    assert(strcmp(st.current_label, "und_stand2_anim") == 0);
    assert(strcmp(st.torso_label, "und_torso2_anim") == 0);

    /* Facing left -- the `4` pair. */
    memset(&st, 0, sizeof(st));
    st.wrestler_num = 2;
    a.facing_dir = WM_MOVE_LEFT;
    wm_wrestler_backend_ani_init(&st, &a);
    assert(strcmp(st.current_label, "und_stand4_anim") == 0);
    assert(strcmp(st.torso_label, "und_torso4_anim") == 0);

    /* A diagonal still carries the horizontal bit. */
    memset(&st, 0, sizeof(st));
    st.wrestler_num = 2;
    a.facing_dir = WM_MOVE_UP_RIGHT;
    wm_wrestler_backend_ani_init(&st, &a);
    assert(strcmp(st.current_label, "und_stand2_anim") == 0);

    /* Adam Bomb has no animations at all; asking must not crash or
     * leave half a selection behind. */
    memset(&st, 0, sizeof(st));
    st.wrestler_num = 7;
    wm_wrestler_backend_ani_init(&st, &a);
    assert(st.current_label == NULL && st.torso_label == NULL);
}

static void test_the_two_channels_are_independent(void)
{
    wm_wrestler_backend_actor st;
    wm_arcade_actor_t a;
    int i;

    memset(&st, 0, sizeof(st));
    memset(&a, 0, sizeof(a));
    st.wrestler_num = 0;                /* Bret's labels, generic backend */
    a.facing_dir = WM_MOVE_RIGHT;
    wm_wrestler_backend_ani_init(&st, &a);

    /* Both channels really are running programs, not one program twice. */
    assert(st.prog.program != NULL);
    assert(st.torso_prog.program != NULL);
    assert(st.prog.program != st.torso_prog.program);

    for (i = 0; i < 40; ++i) {
        wm_wrestler_backend_tick(&st, &a);
    }
    /* And both are producing frames, on their own clocks. */
    assert(wm_wrestler_backend_torso_frame(&st) != NULL);
}

static void test_change_anim1_guard(void)
{
    /*
     * change_anim1's guard: selecting the animation already playing
     * does nothing, unless it has ended, in which case it restarts.
     * Every dispatcher calls it every tick it stays in one mode, so
     * without it a wrestler restarts on frame 0 forever.
     */
    wm_anim_exec exec;
    wm_arcade_actor_t a;
    const wm_anim_program *prog = wm_anim_program_find("hrt_stand2_anim");

    assert(prog != NULL);
    memset(&exec, 0, sizeof(exec));
    memset(&a, 0, sizeof(a));

    assert(wm_anim_exec_start_if_new(&exec, prog, &a, 0, NULL));
    /* Same program, still running: refused. */
    assert(!wm_anim_exec_start_if_new(&exec, prog, &a, 0, NULL));
    /* Ended: restarted regardless -- `btst MODE_END_BIT / jrnz`. */
    exec.ended = true;
    assert(wm_anim_exec_start_if_new(&exec, prog, &a, 0, NULL));
    assert(!exec.ended);
    /* A different program always starts. */
    {
        const wm_anim_program *other = wm_anim_program_find("hrt_stand4_anim");
        assert(other && other != prog);
        assert(wm_anim_exec_start_if_new(&exec, other, &a, 0, NULL));
    }
}

static void test_only_the_primary_channel_resets_gravity(void)
{
    /*
     * change_anim1a does `movi GRAVITY,a0 / move a0,*a13(OBJ_GRAVITY)`
     * and change_anim2a does not. The torso does not fall, and resetting
     * gravity from it would undo whatever the body animation just set.
     */
    wm_anim_exec exec;
    wm_arcade_actor_t a;
    const wm_anim_program *prog = wm_anim_program_find("hrt_torso2_anim");

    assert(prog != NULL);

    memset(&exec, 0, sizeof(exec));
    memset(&a, 0, sizeof(a));
    a.gravity = 12345;
    wm_anim_exec_start(&exec, prog, &a, 0, NULL);
    assert(a.gravity == WM_GRAVITY);

    memset(&exec, 0, sizeof(exec));
    a.gravity = 12345;
    wm_anim_exec_start_secondary(&exec, prog, &a, 0, NULL);
    assert(a.gravity == 12345);

    /* The guarded secondary form behaves the same way. */
    memset(&exec, 0, sizeof(exec));
    a.gravity = 999;
    assert(wm_anim_exec_start_secondary_if_new(&exec, prog, &a, 0, NULL));
    assert(a.gravity == 999);
    assert(!wm_anim_exec_start_secondary_if_new(&exec, prog, &a, 0, NULL));
}

static void test_ani_repeat_loops_rather_than_ending(void)
{
    /*
     * ANIM.ASM:364 _ani_repeat rewinds OANIPC to OANIBASE and jumps to
     * _next_command1 -- it is a loop. The port had it sharing a case
     * with ANI_END and stopping the program, which killed every
     * animation that cycles this way after a single pass: 169 of the
     * 1,525 emitted programs, including every idle stance and torso.
     *
     * Nothing caught it because nothing ran a looping animation long
     * enough, and because the flat wm_visual track beside the VM
     * handles `repeat` correctly -- the two disagreed, and the one
     * Bret's display uses was the right one.
     */
    wm_anim_exec exec;
    wm_arcade_actor_t a;
    const wm_anim_program *prog = wm_anim_program_find("hrt_torso2_anim");
    const char *first;
    int i;
    bool came_back = false;

    assert(prog != NULL);
    memset(&exec, 0, sizeof(exec));
    memset(&a, 0, sizeof(a));
    wm_anim_exec_start(&exec, prog, &a, 0, NULL);
    first = wm_anim_exec_frame(&exec);
    assert(first != NULL);

    /* Six frames of four ticks: it used to die at tick 24. */
    for (i = 0; i < 400; ++i) {
        wm_anim_exec_tick(&exec, &a, (uint16_t)i);
        assert(!exec.ended);
        if (i > 24 && wm_anim_exec_frame(&exec) &&
            strcmp(wm_anim_exec_frame(&exec), first) == 0) {
            came_back = true;
        }
    }
    assert(came_back);          /* it really cycled, not just survived */

    /* ANI_END still ends -- the two were sharing a case and only one
     * of them was wrong. */
    {
        const wm_anim_program *ends = wm_anim_program_find("hrt_2_punch_anim");
        assert(ends != NULL);
        memset(&exec, 0, sizeof(exec));
        wm_anim_exec_start(&exec, ends, &a, 0, NULL);
        for (i = 0; i < 4000 && !exec.ended; ++i) {
            wm_anim_exec_tick(&exec, &a, (uint16_t)i);
        }
        assert(exec.ended);
    }
}

int main(void)
{
    test_ani_init_rows();
    test_ani_init_picks_the_pair_by_facing();
    test_the_two_channels_are_independent();
    test_change_anim1_guard();
    test_only_the_primary_channel_resets_gravity();
    test_ani_repeat_loops_rather_than_ending();
    printf("ANIM.ASM change_anim and the torso channel: all checks passed\n");
    return 0;
}
