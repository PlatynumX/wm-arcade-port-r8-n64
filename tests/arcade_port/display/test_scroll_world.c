/*
 * WRESTLE2.ASM:1681 scroll_world and WRESTLE.ASM:2238
 * update_positions -- the camera, which wm/match_display.h has been
 * saying this port did not have.
 */
#include "wm/arcade/wm_arcade_scroll.h"
#include "wm/match_display.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void put(wm_arcade_actor_t *a, int32_t x, int32_t z, int32_t ground)
{
    memset(a, 0, sizeof(*a));
    a->active = 1;
    a->x_int = x;  a->x_fixed = x << 16;
    a->z_int = z;  a->z_fixed = z << 16;
    a->ground_y = ground;
}

static void test_update_positions(void)
{
    wm_arcade_actor_t a;
    wm_scroll_point p;

    /*
     * The Y it publishes is GROUND_Y, not OBJ_YPOS -- the source's own
     * `move *a13(OBJ_YPOS),a14,L` is commented out. So the camera
     * follows the mat a wrestler stands on and a jump does not make
     * the view lurch.
     */
    put(&a, 1000, 1100, 150);
    a.y_int = 20;                /* airborne */
    a.y_fixed = 20 << 16;
    wm_scroll_update_positions(&a, &p);
    assert(p.x == (1000 << 16));
    assert(p.z == (1100 << 16));
    assert(p.y == (150 << 16));  /* the mat, not the man */

    /* `jrnn #ok / clr a14` -- floored at zero. */
    a.ground_y = -40;
    wm_scroll_update_positions(&a, &p);
    assert(p.y == 0);

    wm_scroll_update_positions(NULL, &p);
    wm_scroll_update_positions(&a, NULL);
}

static void test_the_x_dead_band(void)
{
    wm_scroll_state s;
    wm_scroll_point p1, p2;
    int32_t start = 0x02000000;
    int32_t want;

    memset(&p1, 0, sizeof(p1));
    memset(&p2, 0, sizeof(p2));

    /*
     * The camera wants the midpoint less half a screen. Put it exactly
     * there and it must not move at all.
     */
    want = start + (WM_SCROLL_HALF_W << 16);
    p1.x = want; p2.x = want;
    s.worldtlx = start; s.worldtly = 0;
    wm_scroll_world(&s, &p1, &p2, NULL, 0);
    assert(s.worldtlx == start);

    /*
     * `#BUFFER equ [20,0]` -- twenty pixels of slack either side, and
     * nothing inside it moves the camera. Nineteen is inside.
     */
    p1.x = want + (19 << 16); p2.x = want + (19 << 16);
    s.worldtlx = start;
    wm_scroll_world(&s, &p1, &p2, NULL, 0);
    assert(s.worldtlx == start);

    p1.x = want - (19 << 16); p2.x = want - (19 << 16);
    s.worldtlx = start;
    wm_scroll_world(&s, &p1, &p2, NULL, 0);
    assert(s.worldtlx == start);

    /* Past it, and the camera takes an eighth of what is left over. */
    p1.x = want + (100 << 16); p2.x = want + (100 << 16);
    s.worldtlx = start;
    wm_scroll_world(&s, &p1, &p2, NULL, 0);
    assert(s.worldtlx == start + (((100 - 20) << 16) >> 3));

    /* And symmetrically the other way. */
    p1.x = want - (100 << 16); p2.x = want - (100 << 16);
    s.worldtlx = start;
    wm_scroll_world(&s, &p1, &p2, NULL, 0);
    assert(s.worldtlx == start + ((-((100 - 20) << 16)) >> 3));
}

static void test_the_x_limits_stop_rather_than_clamp(void)
{
    /*
     * `cmpi [12fh,0],a2 / jrlt #wide` does not clamp -- it skips the
     * store. So a camera pushed out of range holds its last legal
     * value instead of sitting on the limit, which is a different
     * thing and visible if the target keeps moving.
     */
    wm_scroll_state s;
    wm_scroll_point p1, p2;
    int32_t start = WM_SCROLL_X_MIN + (4 << 16);

    memset(&p1, 0, sizeof(p1));
    memset(&p2, 0, sizeof(p2));

    p1.x = 0; p2.x = 0;           /* miles to the left */
    s.worldtlx = start; s.worldtly = 0;
    wm_scroll_world(&s, &p1, &p2, NULL, 0);
    assert(s.worldtlx == start);   /* refused, not clamped to the minimum */
    assert(s.worldtlx != WM_SCROLL_X_MIN);
}

static void test_the_y_comes_from_depth(void)
{
    /*
     * The camera's Y is `mean_z / 4.794 - mean_y - 216`, the same
     * depth-to-screen mapping #set_image uses. Height only enters as
     * the mat the pair stands on.
     */
    wm_scroll_state s;
    wm_scroll_point p1, p2;
    int32_t z = 1100;
    int32_t want, expect;

    memset(&p1, 0, sizeof(p1));
    memset(&p2, 0, sizeof(p2));
    p1.z = z << 16; p2.z = z << 16;
    p1.y = 0; p2.y = 0;

    want = (int32_t)((uint32_t)z * (uint32_t)WM_Y_SCALE_MULTIPLIER)
         - (WM_SCROLL_HALF_H << 16);

    /* A quarter of the error each tick, from zero. */
    s.worldtlx = 0x02000000; s.worldtly = 0;
    expect = (want - 0) >> 2;
    wm_scroll_world(&s, &p1, &p2, NULL, 0);
    assert(s.worldtly == expect);

    /* Deeper in the ring means further down the screen. */
    {
        wm_scroll_state deep;
        wm_scroll_point d1 = p1, d2 = p2;
        d1.z = (z + 200) << 16; d2.z = d1.z;
        deep.worldtlx = 0x02000000; deep.worldtly = 0;
        wm_scroll_world(&deep, &d1, &d2, NULL, 0);
        assert(deep.worldtly > s.worldtly);
    }
}

static void test_scroll_ctrl_pulls_the_camera_up(void)
{
    wm_scroll_state a, b;
    wm_scroll_point p1, p2;
    wm_arcade_actor_t hi;
    const wm_arcade_actor_t *actors[1];
    int32_t z = 1100;

    memset(&p1, 0, sizeof(p1));
    memset(&p2, 0, sizeof(p2));
    p1.z = z << 16; p2.z = z << 16;

    a.worldtlx = 0x02000000; a.worldtly = 0;
    wm_scroll_world(&a, &p1, &p2, NULL, 0);

    /* Someone way up in the air, inside the horizontal window, with
       the bit set: the camera follows him instead. */
    put(&hi, (0x0200 + 200), z, 0);
    hi.y_fixed = 400 << 16;
    hi.status_flags = WM_STATUS_SCROLL_CTRL;
    actors[0] = &hi;

    b.worldtlx = 0x02000000; b.worldtly = 0;
    wm_scroll_world(&b, &p1, &p2, actors, 1);
    assert(b.worldtly < a.worldtly);

    /* Without the bit he is ignored, however high he is. */
    hi.status_flags = 0;
    b.worldtlx = 0x02000000; b.worldtly = 0;
    wm_scroll_world(&b, &p1, &p2, actors, 1);
    assert(b.worldtly == a.worldtly);

    /* And with the bit but off the side of the window, also ignored. */
    hi.status_flags = WM_STATUS_SCROLL_CTRL;
    hi.x_fixed = 0x02000000 - (5000 << 16);
    b.worldtlx = 0x02000000; b.worldtly = 0;
    wm_scroll_world(&b, &p1, &p2, actors, 1);
    assert(b.worldtly == a.worldtly);
}

static void test_the_front_fence(void)
{
    /* `cmpi [97h,0],a1 / jrgt #low` -- past the fence, don't store. */
    wm_scroll_state s;
    wm_scroll_point p1, p2;

    memset(&p1, 0, sizeof(p1));
    memset(&p2, 0, sizeof(p2));
    p1.z = 4000 << 16; p2.z = 4000 << 16;     /* absurdly deep */

    s.worldtlx = 0x02000000;
    s.worldtly = WM_SCROLL_Y_MAX;
    wm_scroll_world(&s, &p1, &p2, NULL, 0);
    assert(s.worldtly <= WM_SCROLL_Y_MAX);
}

static void test_pick_pair(void)
{
    wm_arcade_actor_t a, b;
    const wm_arcade_actor_t *actors[2];
    size_t i, j;

    put(&a, 1000, 1100, 150);
    put(&b, 1200, 1100, 150);
    a.smart_target = &b;
    b.smart_target = &a;
    actors[0] = &a; actors[1] = &b;

    assert(wm_scroll_pick_pair(actors, 2, true, &i, &j));
    assert(i == 0 && j == 1);

    /* Attract mode: the first live one and his closest. */
    assert(wm_scroll_pick_pair(actors, 2, false, &i, &j));
    assert(i == 0 && j == 1);

    /* A dead first actor is skipped in the no-humans sweep. */
    a.player_mode = (uint16_t)WM_PMODE_DEAD;
    assert(wm_scroll_pick_pair(actors, 2, false, &i, &j));
    assert(i == 1);

    /* Everybody gone. */
    actors[0] = NULL; actors[1] = NULL;
    assert(!wm_scroll_pick_pair(actors, 2, false, &i, &j));
    assert(!wm_scroll_pick_pair(actors, 2, true, &i, &j));
    assert(!wm_scroll_pick_pair(NULL, 2, true, &i, &j));
}

static void test_keep_onscreen(void)
{
    wm_arcade_actor_t a, b;
    int32_t allow = 0;
    int32_t centre, left, right;
    wm_keep_onscreen_result r;

    /* Both buffers are 185 in the shipped build. */
    assert(WM_KEEP_ONSCREEN_BUFF1 == 185);
    assert(WM_KEEP_ONSCREEN_BUFF2 == 185);

    centre = 1000;
    left = centre - 185;
    right = centre + 185;

    /* Inside the window: nothing happens however he is moving. */
    put(&a, centre, 1100, 150);
    a.x_vel = -0x00040000;
    r = wm_keep_onscreen_one(&a, left, right);
    assert(!r.stopped && a.x_vel != 0);

    /* Past the left edge and still heading left: stopped. */
    put(&a, left - 10, 1100, 150);
    a.x_vel = -0x00040000;
    r = wm_keep_onscreen_one(&a, left, right);
    assert(r.stopped && a.x_vel == 0);

    /*
     * Past the left edge but heading BACK: left alone. It only ever
     * stops a man going further out, never pushes him in.
     */
    put(&a, left - 10, 1100, 150);
    a.x_vel = 0x00040000;
    r = wm_keep_onscreen_one(&a, left, right);
    assert(!r.stopped && a.x_vel == 0x00040000);

    /* And the same on the right. */
    put(&a, right + 10, 1100, 150);
    a.x_vel = 0x00040000;
    r = wm_keep_onscreen_one(&a, left, right);
    assert(r.stopped && a.x_vel == 0);

    put(&a, right + 10, 1100, 150);
    a.x_vel = -0x00040000;
    r = wm_keep_onscreen_one(&a, left, right);
    assert(!r.stopped);

    /* Climbing through the ropes is exempt. */
    put(&a, left - 10, 1100, 150);
    a.x_vel = -0x00040000;
    a.climbing_thru = 1;
    r = wm_keep_onscreen_one(&a, left, right);
    assert(!r.stopped && a.x_vel == -0x00040000);

    /* A stopped run is over -- and ANIMODE is written whole, so every
       other animation-mode bit goes with it. */
    put(&a, left - 10, 1100, 150);
    a.x_vel = -0x00040000;
    a.player_mode = (uint16_t)WM_PMODE_RUNNING;
    a.anim_mode = (uint16_t)(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP);
    r = wm_keep_onscreen_one(&a, left, right);
    assert(r.stopped && r.dropped_run);
    assert(a.player_mode == (uint16_t)WM_PMODE_NORMAL);
    assert(a.anim_mode == 0);

    r = wm_keep_onscreen_one(NULL, left, right);
    assert(!r.stopped);

    /* The gates. One player: never confined. */
    put(&a, 0, 1100, 150);
    put(&b, 0, 1100, 150);
    a.x_vel = -0x00040000;
    a.in_ring = 1;
    wm_keep_onscreen(&a, &b, 0x02000000, false, &allow);
    assert(a.x_vel != 0);

    /* Both inside the ring: the ropes are doing the confining. */
    a.in_ring = 0; b.in_ring = 0;
    wm_keep_onscreen(&a, &b, 0x02000000, true, &allow);
    assert(a.x_vel != 0);

    /*
     * allow_offscrn is a COUNTDOWN, not a flag: it is decremented on
     * the way past and only lets him go while it is still nonzero
     * afterwards.
     */
    a.in_ring = 1;
    allow = 3;
    wm_keep_onscreen(&a, &b, 0x02000000, true, &allow);
    assert(allow == 2);
    assert(a.x_vel != 0);          /* skipped */

    allow = 1;
    wm_keep_onscreen(&a, &b, 0x02000000, true, &allow);
    assert(allow == 0);
    /* The last tick of the countdown does NOT skip -- the source
       decrements first and only branches away on a nonzero result. */
}

int main(void)
{
    test_update_positions();
    test_the_x_dead_band();
    test_the_x_limits_stop_rather_than_clamp();
    test_the_y_comes_from_depth();
    test_scroll_ctrl_pulls_the_camera_up();
    test_the_front_fence();
    test_pick_pair();
    test_keep_onscreen();
    printf("scroll_world and update_positions: all checks passed\n");
    return 0;
}
