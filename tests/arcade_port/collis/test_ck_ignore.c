/*
 * WRESTLE.ASM:6044 ck_ignore_a8, :5704 get_box_overlap, and
 * DRONE.ASM:2092 drn_combo's eight per-wrestler branches.
 */
#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wmania_ring_geometry.h"
#include "wm/arcade/wm_arcade_drone_data.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_ck_ignore(void)
{
    wm_arcade_actor_t a;

    memset(&a, 0, sizeof(a));

    /*
     * Facing left (5 = up-left, 6 = down-left): moving RIGHT is moving
     * away, and the flying kick is refused.
     */
    a.new_facing_dir = WM_MOVE_UP | WM_MOVE_LEFT;
    a.move_dir = WM_MOVE_RIGHT;
    assert(wm_arcade_ck_ignore(&a));
    a.move_dir = WM_MOVE_LEFT;
    assert(!wm_arcade_ck_ignore(&a));

    a.new_facing_dir = WM_MOVE_DOWN | WM_MOVE_LEFT;
    a.move_dir = WM_MOVE_RIGHT;
    assert(wm_arcade_ck_ignore(&a));

    /* Facing right (9, 10): moving LEFT is away. */
    a.new_facing_dir = WM_MOVE_UP | WM_MOVE_RIGHT;
    a.move_dir = WM_MOVE_LEFT;
    assert(wm_arcade_ck_ignore(&a));
    a.move_dir = WM_MOVE_RIGHT;
    assert(!wm_arcade_ck_ignore(&a));

    a.new_facing_dir = WM_MOVE_DOWN | WM_MOVE_RIGHT;
    a.move_dir = WM_MOVE_LEFT;
    assert(wm_arcade_ck_ignore(&a));

    /* Standing still is allowed -- the routine's own summary says
       "or standing still", but the code only tests the away bit, and
       a zero MOVE_DIR has it clear. */
    a.move_dir = 0;
    assert(!wm_arcade_ck_ignore(&a));

    /* Moving up or down, neither toward nor away, is allowed. */
    a.move_dir = WM_MOVE_UP;
    assert(!wm_arcade_ck_ignore(&a));

    /*
     * A facing the table does not cover. mv_tbl's zero would be read
     * as BIT NUMBER zero -- MOVE_UP_BIT -- so the arcade would refuse
     * a man moving up. The source's own comment says the facing is
     * "(9,10,6,5 only)", so this port answers "allow" rather than
     * reproducing a lookup the comment says cannot happen.
     */
    a.new_facing_dir = WM_MOVE_UP;
    a.move_dir = WM_MOVE_UP;
    assert(!wm_arcade_ck_ignore(&a));

    assert(!wm_arcade_ck_ignore(NULL));
}

static void test_get_box_overlap(void)
{
    const WmRingBoundarySeed *left =
        wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_ROPE);
    const WmRingBoundarySeed *right =
        wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_ROPE);
    int32_t z = WM_RING_TOP + 40;
    int32_t lx = wm_ring_calc_line_x(left, z);
    int32_t rx = wm_ring_calc_line_x(right, z);
    wm_ring_pushout out;

    assert(lx != 0 && rx != 0 && lx < rx);

    /* Just inside the left edge: pushed left, by that little. */
    out = wm_ring_box_overlap(left, right, lx + 3, z);
    assert(out.zoff == 0);
    assert(out.xoff == -3);

    /* Just inside the right edge: pushed right. */
    out = wm_ring_box_overlap(left, right, rx - 5, z);
    assert(out.zoff == 0);
    assert(out.xoff == 5);

    /* Outside altogether, either side: nothing. */
    out = wm_ring_box_overlap(left, right, lx - 10, z);
    assert(out.xoff == 0 && out.zoff == 0);
    out = wm_ring_box_overlap(left, right, rx + 10, z);
    assert(out.xoff == 0 && out.zoff == 0);

    /*
     * Near the top edge in Z, in the middle horizontally: the Z push
     * wins because it is the smaller of the four.
     */
    {
        int32_t ztop = (int32_t)left->top_z + 2;
        int32_t mid = (wm_ring_calc_line_x(left, ztop) +
                       wm_ring_calc_line_x(right, ztop)) / 2;
        out = wm_ring_box_overlap(left, right, mid, ztop);
        assert(out.xoff == 0);
        assert(out.zoff == -2);
    }

    /* A Z off either end of the line is "out of range", which reports
       the same (0,0) as being outside the box -- the caller cannot
       tell them apart. */
    out = wm_ring_box_overlap(left, right, lx + 3, WM_RING_TOP - 500);
    assert(out.xoff == 0 && out.zoff == 0);

    out = wm_ring_box_overlap(NULL, right, 0, 0);
    assert(out.xoff == 0 && out.zoff == 0);
}

static void test_drn_combo_branches(void)
{
    /*
     * DRONE.ASM:2101 #wres_t. Eight branches, one per wrestler, and
     * slot 7 -- the cut one -- is given Doink's by the table itself.
     */
    static const char *const want[9] = {
        "drn_combo/brt", "drn_combo/raz", "drn_combo/ut", "drn_combo/yok",
        "drn_combo/shn", "drn_combo/bam", "drn_combo/dnk", "drn_combo/dnk",
        "drn_combo/lex"
    };
    int i;

    for (i = 0; i <= 8; ++i) {
        const char *got = wm_arcade_drone_combo_script(i);
        assert(got && strcmp(got, want[i]) == 0);
    }
    assert(wm_arcade_drone_combo_script(-1) == NULL);
    assert(wm_arcade_drone_combo_script(9) == NULL);

    /* Slot 7 really is Doink's, not its own. */
    assert(wm_arcade_drone_combo_script(7) == wm_arcade_drone_combo_script(6) ||
           strcmp(wm_arcade_drone_combo_script(7),
                  wm_arcade_drone_combo_script(6)) == 0);
}

int main(void)
{
    test_ck_ignore();
    test_get_box_overlap();
    test_drn_combo_branches();
    printf("ck_ignore_a8, get_box_overlap, drn_combo: all checks passed\n");
    return 0;
}
