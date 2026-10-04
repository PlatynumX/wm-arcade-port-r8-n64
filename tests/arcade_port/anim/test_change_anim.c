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
#include "wm/arcade/wm_arcade_roster.h"

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

    /*
     * A stand frame is one piece. hrt_stand2_anim's H2ST2A05 is the
     * whole wrestler, 116 high, with no channel-2 attachment, so
     * set_images' #no_2nd_piece (ANIM.ASM:4818) ends the torso the first
     * time the body shows it: ANIMODE2 = MODE_END, "don't bother
     * animating if no 2nd piece".
     */
    for (i = 0; i < 40; ++i) {
        wm_wrestler_backend_tick(&st, &a);
    }
    assert(wm_anim_frame_ends_secondary("H2ST2A05"));
    assert(st.torso_prog.ended);
    assert(wm_wrestler_backend_torso_frame(&st) == NULL);

    /*
     * A walk frame is two. H4WL4A01 is legs only, 77 high, with the
     * torso hanging off it at (16,5) -- and change_anim2 restarts the
     * ended torso, which then runs on its own clock beside the legs.
     */
    {
        wm_arcade_roster_callbacks_t cb = wm_wrestler_roster_callbacks(&st);
        cb.change_anim_restart(&a, "hrt_walk4_f4_anim", cb.user);
        cb.change_torso_label(&a, "hrt_torso4_anim", cb.user);
    }
    assert(!wm_anim_frame_ends_secondary("H4WL4A01"));
    for (i = 0; i < 40; ++i) {
        wm_wrestler_backend_tick(&st, &a);
    }
    assert(!st.torso_prog.ended);
    assert(wm_wrestler_backend_torso_frame(&st) != NULL);
    assert(st.prog.program != st.torso_prog.program);
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

static void test_ifrope_actually_branches(void)
{
    /*
     * ANI_IFROPE's destination was being dropped by the emitter -- the
     * fixup resolved `#fall_back` and the renderer then wrote -1,
     * because _c_op only reads a target for ops listed in BRANCH_OPS
     * and IFROPE was not one. In the interpreter that -1 becomes
     * SIZE_MAX, fails `pc < op_count`, and falls out of the dispatch
     * loop into `ended = true`.
     *
     * So a wrestler within 100 of a rope did not fall back: his
     * animation stopped dead, before its first frame. Seven programs,
     * every wrestler's break_neck among them.
     */
    wm_anim_exec exec;
    wm_arcade_actor_t a;
    const wm_anim_program *prog = wm_anim_program_find("hrt_break_neck_anim");
    const char *near_rope;
    const char *away;
    int i;

    assert(prog != NULL);

    /* In the ring and near a rope: the branch is taken. */
    memset(&exec, 0, sizeof(exec));
    memset(&a, 0, sizeof(a));
    a.life = 163;
    a.in_ring = 1;
    a.facing_dir = WM_MOVE_RIGHT;
    wm_anim_exec_start(&exec, prog, &a, 0, NULL);
    assert(wm_anim_exec_frame(&exec) != NULL);   /* it used to be gone */
    for (i = 0; i < 12; ++i) {
        wm_anim_exec_tick(&exec, &a, (uint16_t)i);
    }
    assert(!exec.ended);
    near_rope = wm_anim_exec_frame(&exec);
    assert(near_rope != NULL);

    /*
     * Outside the ring the source jumps to `#definitly_too_far`, which
     * falls THROUGH rather than branching -- so the two paths must end
     * up somewhere different. Equal frames here would mean the branch
     * is not being taken at all.
     */
    memset(&exec, 0, sizeof(exec));
    a.in_ring = 0;
    wm_anim_exec_start(&exec, prog, &a, 0, NULL);
    for (i = 0; i < 12; ++i) {
        wm_anim_exec_tick(&exec, &a, (uint16_t)i);
    }
    away = wm_anim_exec_frame(&exec);
    assert(away != NULL);
    assert(strcmp(near_rope, away) != 0);
}

/*
 * ANIM.ASM:2497 _ani_end -- the opcode ORs MODE_END into the mode word
 * rather than assigning it, and hrt_bounce_anim (HRTSEQ1.ASM:575) is
 * the animation that makes the difference visible: it ends on
 *
 *     .word  ANI_SETMODE,MODE_NORMAL
 *     .word  ANI_END
 *
 * so ANIMODE lands at exactly MODE_END, not at 0. Getting this wrong
 * is not cosmetic -- see the bouncing test below.
 */
static void test_ani_end_sets_mode_end(void)
{
    const wm_anim_program *prog = wm_anim_program_find("hrt_bounce_anim");
    wm_anim_exec exec;
    wm_arcade_actor_t a;
    int i;

    assert(prog != NULL);
    memset(&exec, 0, sizeof(exec));
    memset(&a, 0, sizeof(a));

    wm_anim_exec_start(&exec, prog, &a, 0, NULL);
    /* Its own header sets UNINT|OVERLAP|NOAUTOFLIP|NOCONFINE, so the
       mode word is NOT MODE_END while it is still running. */
    assert((a.anim_mode & WM_MODE_END) == 0);

    for (i = 0; i < 400 && !exec.ended; ++i)
        wm_anim_exec_tick(&exec, &a, (uint16_t)i);
    assert(exec.ended);

    /* The trailing ANI_SETMODE,MODE_NORMAL wrote 0 and then ANI_END
       ORed the bit in on top of it: exactly MODE_END, nothing else. */
    assert(a.anim_mode == WM_MODE_END);
}

/*
 * ANIM.ASM:4547 change_anim1a `clr a0 / move a0,*a13(ANIMODE)`.
 *
 * Selecting an animation starts from a blank mode word, which is what
 * retires the MODE_END the last one left behind. Without it a wrestler
 * whose animation had ended once would answer "ended" to every
 * MODE_END test for the rest of the round.
 *
 * The animation has to be one that never writes ANIMODE itself, or the
 * clear proves nothing: 1401 of the 1555 emitted programs carry an
 * ANI_SETMODE somewhere, and an absolute write of the whole word hides
 * a missing clear. A walk is one of the 154 that carry none, which is
 * also why it matters -- a wrestler who walks out of an uninterruptable
 * animation must not stay uninterruptable while he walks.
 */
static void test_starting_an_animation_clears_mode_end(void)
{
    const wm_anim_program *prog = wm_anim_program_find("bam_walk1_f2_anim");
    wm_anim_exec exec;
    wm_arcade_actor_t a;
    size_t i;

    assert(prog != NULL);
    /* The premise: this program writes no mode bits of its own. */
    for (i = 0; i < prog->op_count; ++i)
        assert(prog->ops[i].op != WM_AOP_SETMODE);

    memset(&exec, 0, sizeof(exec));
    memset(&a, 0, sizeof(a));

    /* Whatever the previous animation left, including MODE_END. */
    a.anim_mode = (uint16_t)(WM_MODE_END | WM_MODE_UNINT | WM_MODE_NOCONFINE);
    wm_anim_exec_start(&exec, prog, &a, 0, NULL);
    assert(a.anim_mode == 0);
}

/*
 * _ani_end ORs rather than assigns, so an animation whose last mode
 * write leaves bits standing keeps them alongside MODE_END.
 *
 * wres_slave_anim (ANIM.ASM:4603) is the whole of that case:
 *
 *     .word  ANI_SETMODE,MODE_UNINT+MODE_NOAUTOFLIP+MODE_NOGRAVITY
 *     .word  ANI_ZEROVELS
 *     .word  ANI_SETSPEED,100h
 *     .word  ANI_END
 *
 * It must finish holding four bits, not one. A puppet left without
 * MODE_NOGRAVITY would start falling the moment his animation ended;
 * one left without MODE_UNINT would become interruptable while still
 * being carried. wm_arcade_anim_enter_slave_idle hand-executes this
 * same animation and agrees on all four.
 */
static void test_ani_end_keeps_the_mode_bits_already_set(void)
{
    const wm_anim_program *prog = wm_anim_program_find("wres_slave_anim");
    wm_anim_exec exec;
    wm_arcade_actor_t a;
    int i;

    assert(prog != NULL);
    memset(&exec, 0, sizeof(exec));
    memset(&a, 0, sizeof(a));

    wm_anim_exec_start(&exec, prog, &a, 0, NULL);
    for (i = 0; i < 64 && !exec.ended; ++i)
        wm_anim_exec_tick(&exec, &a, (uint16_t)i);
    assert(exec.ended);

    assert(a.anim_mode == (uint16_t)(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP |
                                     WM_MODE_NOGRAVITY | WM_MODE_END));
}

/*
 * ANIM.ASM:79 and :84 -- the runner's a10 is `a13+ANIMODE` for the
 * primary channel and `a13+ANIMODE2` for the secondary, so every
 * OANIMODE write lands on whichever word the caller chose. The actor
 * here carries ANIMODE only; a torso animation reaching its ANI_END
 * must therefore leave the body's mode word alone rather than tell the
 * dispatcher the BODY animation has ended.
 */
static void test_only_the_primary_channel_writes_mode_end(void)
{
    const wm_anim_program *prog = wm_anim_program_find("hrt_bounce_anim");
    wm_anim_exec exec;
    wm_arcade_actor_t a;
    int i;

    assert(prog != NULL);
    memset(&exec, 0, sizeof(exec));
    memset(&a, 0, sizeof(a));

    wm_anim_exec_start_secondary(&exec, prog, &a, 0, NULL);
    for (i = 0; i < 400 && !exec.ended; ++i)
        wm_anim_exec_tick(&exec, &a, (uint16_t)i);
    assert(exec.ended);
    assert((a.anim_mode & WM_MODE_END) == 0);
}

int main(void)
{
    test_ani_init_rows();
    test_ani_init_picks_the_pair_by_facing();
    test_the_two_channels_are_independent();
    test_change_anim1_guard();
    test_only_the_primary_channel_resets_gravity();
    test_ani_repeat_loops_rather_than_ending();
    test_ifrope_actually_branches();
    test_ani_end_sets_mode_end();
    test_starting_an_animation_clears_mode_end();
    test_ani_end_keeps_the_mode_bits_already_set();
    test_only_the_primary_channel_writes_mode_end();
    printf("ANIM.ASM change_anim and the torso channel: all checks passed\n");
    return 0;
}
