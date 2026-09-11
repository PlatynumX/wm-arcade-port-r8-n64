/*
 * The seam between the translated match and whatever draws it.
 *
 * Until this existed the N64 renderer and the ported game were two
 * different programs: src/platform/n64/main.c drew wm_demo_fighter
 * through a wm_visual_sequence track and never named
 * wm_arcade_actor_t, wm_anim_exec or wm_wrestler_backend once. Its
 * match renderer was switched off on purpose -- "the existing combat
 * renderer is a development harness only" -- so the animation VM, the
 * 1,532 emitted programs, the per-wrestler tables and the physics
 * never reached a screen.
 *
 * These are the parts of drawing that are game logic with exact
 * answers in the source, which is why they are testable here and not
 * stranded in a file that cannot be compiled without libdragon.
 */
#include "wm/match_display.h"
#include "wm/frame_geometry.h"
#include "wm/arcade/wm_arcade_roster.h"
#include "wm/arcade/wm_arcade_combat_defs.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void stage(wm_match_state *m, int32_t w0, int32_t w1)
{
    memset(m, 0, sizeof(*m));
    m->actor_count = 2;
    m->actors[0].active = 1;
    m->actors[0].wrestler_num = w0;
    m->actors[0].life = 163;
    m->actors[1].active = 1;
    m->actors[1].wrestler_num = w1;
    m->actors[1].life = 163;
}

/* DISPLAY.ASM:1248 obj_yzsort -- ascending Z, then ascending Y. */
static void test_draw_order_is_obj_yzsort(void)
{
    wm_match_state m;
    wm_match_draw_item items[WM_MATCH_DRAW_MAX];
    size_t n;

    /* Actor 1 is further back: he is drawn first, so actor 0 overlaps. */
    stage(&m, WM_ROSTER_TAKER, WM_ROSTER_YOKO);
    m.actors[0].z_int = 900;
    m.actors[1].z_int = 500;
    n = wm_match_build_draw_list(&m, items, WM_MATCH_DRAW_MAX);
    assert(n >= 2);
    assert(items[0].wrestler_num == WM_ROSTER_YOKO);

    /* Swap the depths and the order swaps with them. */
    m.actors[0].z_int = 500;
    m.actors[1].z_int = 900;
    n = wm_match_build_draw_list(&m, items, WM_MATCH_DRAW_MAX);
    assert(n >= 2);
    assert(items[0].wrestler_num == WM_ROSTER_TAKER);

    /* Equal Z falls through to Y, ascending -- the source's second key,
       which the demo renderer this replaces never consulted. */
    m.actors[0].z_int = 700; m.actors[0].y_int = 40;
    m.actors[1].z_int = 700; m.actors[1].y_int = 10;
    n = wm_match_build_draw_list(&m, items, WM_MATCH_DRAW_MAX);
    assert(items[0].wrestler_num == WM_ROSTER_YOKO);

    m.actors[0].y_int = 10;
    m.actors[1].y_int = 40;
    n = wm_match_build_draw_list(&m, items, WM_MATCH_DRAW_MAX);
    assert(items[0].wrestler_num == WM_ROSTER_TAKER);
}

static void test_layers_are_ordered_within_an_actor(void)
{
    wm_match_state m;
    wm_match_draw_item items[WM_MATCH_DRAW_MAX];
    size_t n, i;
    int seen_body = 0;

    stage(&m, WM_ROSTER_TAKER, WM_ROSTER_YOKO);
    m.actors[0].z_int = 500;
    m.actors[1].z_int = 900;
    n = wm_match_build_draw_list(&m, items, WM_MATCH_DRAW_MAX);
    assert(n >= 2);

    /* Every actor puts its shadow down before anything else it draws,
       and a torso never precedes the body it hangs off. */
    for (i = 0; i < n; ++i) {
        if (items[i].layer == WM_DRAW_SHADOW) {
            seen_body = 0;
            continue;
        }
        if (items[i].layer == WM_DRAW_BODY) { seen_body = 1; continue; }
        if (items[i].layer == WM_DRAW_TORSO) assert(seen_body);
    }
}

static void test_an_inactive_actor_is_not_drawn(void)
{
    wm_match_state m;
    wm_match_draw_item items[WM_MATCH_DRAW_MAX];
    size_t n, i;

    stage(&m, WM_ROSTER_TAKER, WM_ROSTER_YOKO);
    m.actors[1].active = 0;
    n = wm_match_build_draw_list(&m, items, WM_MATCH_DRAW_MAX);
    for (i = 0; i < n; ++i)
        assert(items[i].wrestler_num != WM_ROSTER_YOKO);
}

static void test_the_shadow_goes_down_even_with_no_frame(void)
{
    wm_match_state m;
    wm_match_draw_item items[WM_MATCH_DRAW_MAX];
    size_t n;

    /* A wrestler with no animation started yet is still standing
       there, so his shadow is still drawn -- and it carries no frame
       of its own. */
    stage(&m, WM_ROSTER_TAKER, WM_ROSTER_YOKO);
    n = wm_match_build_draw_list(&m, items, WM_MATCH_DRAW_MAX);
    assert(n == 2);
    assert(items[0].layer == WM_DRAW_SHADOW && items[0].frame == NULL);
    assert(items[1].layer == WM_DRAW_SHADOW && items[1].frame == NULL);
}

static void test_world_position_and_flip_come_from_the_actor(void)
{
    wm_match_state m;
    wm_match_draw_item items[WM_MATCH_DRAW_MAX];
    size_t n;

    stage(&m, WM_ROSTER_TAKER, WM_ROSTER_YOKO);
    m.actors[0].x_int = 123;
    m.actors[0].y_int = 45;
    m.actors[0].z_int = 678;
    m.actors[0].obj_control |= WM_OBJ_FLIPH;
    m.actors[1].active = 0;

    n = wm_match_build_draw_list(&m, items, WM_MATCH_DRAW_MAX);
    assert(n >= 1);
    assert(items[0].world_x == 123);
    assert(items[0].world_y == 45);
    assert(items[0].world_z == 678);
    assert(items[0].flip_x);

    m.actors[0].obj_control &= (uint16_t)~WM_OBJ_FLIPH;
    n = wm_match_build_draw_list(&m, items, WM_MATCH_DRAW_MAX);
    assert(!items[0].flip_x);
}

/*
 * The torso only hangs where the body frame says it can. 748 of the
 * roster's 5,126 frames carry a channel-2 attachment pair; on the rest
 * it is (-1, -1) and the source draws no second layer.
 */
static void test_the_attachment_pair_gates_the_torso(void)
{
    const wm_frame_geometry_t *walk = wm_frame_geometry_find("H2WL1A01");
    const wm_frame_geometry_t *stand = wm_frame_geometry_find("H2ST2A05");

    assert(walk != NULL && stand != NULL);
    /* A walk frame has somewhere to hang a torso... */
    assert(!(walk->attach_x == -1 && walk->attach_y == -1));
    /* ...and this stand frame does not. */
    assert(stand->attach_x == -1 && stand->attach_y == -1);
}

static void test_it_refuses_to_overrun_the_caller(void)
{
    wm_match_state m;
    wm_match_draw_item items[2];
    size_t n;

    stage(&m, WM_ROSTER_TAKER, WM_ROSTER_YOKO);
    n = wm_match_build_draw_list(&m, items, 1);
    assert(n == 1);
    n = wm_match_build_draw_list(&m, items, 2);
    assert(n == 2);
    assert(wm_match_build_draw_list(&m, NULL, 4) == 0);
    assert(wm_match_build_draw_list(NULL, items, 2) == 0);
    assert(wm_match_build_draw_list(&m, items, 0) == 0);
}

int main(void)
{
    test_draw_order_is_obj_yzsort();
    test_layers_are_ordered_within_an_actor();
    test_an_inactive_actor_is_not_drawn();
    test_the_shadow_goes_down_even_with_no_frame();
    test_world_position_and_flip_come_from_the_actor();
    test_the_attachment_pair_gates_the_torso();
    test_it_refuses_to_overrun_the_caller();
    printf("match display list: all checks passed\n");
    return 0;
}
