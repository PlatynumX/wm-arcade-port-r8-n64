/*
 * COLLIS.ASM:56 overlap_collision -- the loop that keeps two wrestlers
 * from standing inside one another.
 *
 * The port has had this routine's BODY for a long time:
 * wm_arcade_resolve_overlap is a faithful, complete translation of the
 * push, down to halving the X penetration when the other man is on the
 * ground and forcing the Z axis past the source's 0x3d glitch bound.
 * Nothing called it. Not one caller anywhere in the port, so the code
 * that separates two overlapping wrestlers was never reached and they
 * could stand in the same space indefinitely.
 *
 * The loop is what was missing, and this is what it does.
 */
#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wm_arcade_combat_defs.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void box(wm_arcade_actor_t *a, int32_t x, int32_t z)
{
    memset(a, 0, sizeof(*a));
    a->active = 1;
    a->life = 163;
    a->x_int = x;
    a->z_int = z;
    a->player_mode = WM_PMODE_NORMAL;
    /* A 40-wide, 20-deep body around (x, z), the shape a hurt box takes
       once set_collision_boxes' half has run in the backend tick. */
    a->hurt_box.x1 = x - 20; a->hurt_box.x2 = x + 20;
    a->hurt_box.z1 = z - 10; a->hurt_box.z2 = z + 10;
    a->hurt_box.y1 = 0;      a->hurt_box.y2 = 100;
}

static void test_the_loop_separates_them(void)
{
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *ptrs[2];
    int32_t bx, bz;
    int moved;

    /* Bodies 40 wide and 20 deep, 10 apart on X and dead level on Z:
       30 units of X penetration against 20 of Z. */
    box(&a, 100, 500);
    box(&b, 110, 500);
    ptrs[0] = &a; ptrs[1] = &b;

    bx = a.x_int; bz = a.z_int;
    moved = wm_arcade_overlap_collision(&a, ptrs, 2);
    assert(moved == 1);

    /* And the axis it resolves is Z, not X. The source takes the
       SMALLER penetration -- `if (!(min_x > min_z || min_z > 0x3d))`
       skips the X push whenever X is the deeper overlap -- so two
       wrestlers standing side by side are separated along the axis
       they are least buried in, which here is depth. */
    assert(a.x_int == bx);
    assert(a.z_int == bz - 20);
}

static void test_x_is_resolved_when_x_is_the_shallower_axis(void)
{
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *ptrs[2];
    int32_t bx, bz;

    /* Same bodies, but now barely touching on X and buried on Z:
       5 units of X penetration against 20 of Z. */
    box(&a, 100, 500);
    box(&b, 135, 500);
    ptrs[0] = &a; ptrs[1] = &b;

    bx = a.x_int; bz = a.z_int;
    assert(wm_arcade_overlap_collision(&a, ptrs, 2) == 1);
    assert(a.x_int == bx - 5);      /* pushed left, out of the overlap */
    assert(a.z_int == bz);          /* Z untouched */
}

static void test_it_skips_the_pairs_the_source_skips(void)
{
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *ptrs[2];
    int32_t before;

    /* Self is skipped -- `cmp a11,a13 / jreq #skip`. */
    box(&a, 100, 500);
    ptrs[0] = &a; ptrs[1] = &a;
    before = a.x_int;
    assert(wm_arcade_overlap_collision(&a, ptrs, 2) == 0);
    assert(a.x_int == before);

    /* An empty process slot -- `jrz #inactive`. */
    box(&a, 100, 500);
    ptrs[0] = NULL; ptrs[1] = NULL;
    before = a.x_int;
    assert(wm_arcade_overlap_collision(&a, ptrs, 2) == 0);
    assert(a.x_int == before);

    /* You walk through the dead. */
    box(&a, 100, 500);
    box(&b, 110, 500);
    b.player_mode = WM_PMODE_DEAD;
    ptrs[0] = &a; ptrs[1] = &b;
    before = a.x_int;
    assert(wm_arcade_overlap_collision(&a, ptrs, 2) == 0);
    assert(a.x_int == before);

    /* ...and through anyone who set MODE_OVERLAP. */
    box(&a, 100, 500);
    box(&b, 110, 500);
    b.anim_mode |= WM_MODE_OVERLAP;
    ptrs[0] = &a; ptrs[1] = &b;
    before = a.x_int;
    assert(wm_arcade_overlap_collision(&a, ptrs, 2) == 0);
    assert(a.x_int == before);

    /* A man on the ground cannot be pushed. */
    box(&a, 100, 500);
    box(&b, 110, 500);
    a.player_mode = WM_PMODE_ONGROUND;
    ptrs[0] = &a; ptrs[1] = &b;
    before = a.x_int;
    assert(wm_arcade_overlap_collision(&a, ptrs, 2) == 0);
    assert(a.x_int == before);

    /* And a runner runs THROUGH a downed opponent. */
    box(&a, 100, 500);
    box(&b, 110, 500);
    a.player_mode = WM_PMODE_RUNNING;
    b.player_mode = WM_PMODE_ONGROUND;
    ptrs[0] = &a; ptrs[1] = &b;
    before = a.x_int;
    assert(wm_arcade_overlap_collision(&a, ptrs, 2) == 0);
    assert(a.x_int == before);
}

static void test_no_overlap_is_no_push(void)
{
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *ptrs[2];
    int32_t bx, bz;

    /* Well clear of each other on X. */
    box(&a, 100, 500);
    box(&b, 400, 500);
    ptrs[0] = &a; ptrs[1] = &b;
    bx = a.x_int; bz = a.z_int;
    assert(wm_arcade_overlap_collision(&a, ptrs, 2) == 0);
    assert(a.x_int == bx && a.z_int == bz);

    /* Overlapping on X but not on Z. */
    box(&a, 100, 500);
    box(&b, 110, 900);
    ptrs[0] = &a; ptrs[1] = &b;
    bx = a.x_int; bz = a.z_int;
    assert(wm_arcade_overlap_collision(&a, ptrs, 2) == 0);
    assert(a.x_int == bx && a.z_int == bz);
}

static void test_it_separates_against_several(void)
{
    wm_arcade_actor_t a, b, c;
    wm_arcade_actor_t *ptrs[3];

    /* The source walks NUM_WRES slots, not just one opponent. */
    box(&a, 100, 500);
    box(&b, 110, 500);
    box(&c, 95, 500);
    ptrs[0] = &a; ptrs[1] = &b; ptrs[2] = &c;
    assert(wm_arcade_overlap_collision(&a, ptrs, 3) == 2);
}

static void test_a_null_list_is_harmless(void)
{
    wm_arcade_actor_t a;
    box(&a, 100, 500);
    assert(wm_arcade_overlap_collision(&a, NULL, 2) == 0);
    assert(wm_arcade_overlap_collision(NULL, NULL, 0) == 0);
}

int main(void)
{
    test_the_loop_separates_them();
    test_x_is_resolved_when_x_is_the_shallower_axis();
    test_it_skips_the_pairs_the_source_skips();
    test_no_overlap_is_no_push();
    test_it_separates_against_several();
    test_a_null_list_is_harmless();
    printf("COLLIS.ASM overlap_collision: all checks passed\n");
    return 0;
}
