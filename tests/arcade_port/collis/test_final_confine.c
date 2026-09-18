/*
 * WRESTLE.ASM:6163 final_confine, :5789 get_rope_x, WRESTLE2.ASM:1270
 * inc_getup_time, and the two ANI_CODE routines at the head of
 * xxx_dead_anim.
 */
#include "wm/arcade/wm_arcade_confine.h"
#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wmania_ring_geometry.h"
#include "wm/anim_program.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void place(wm_arcade_actor_t *a, int32_t x, int32_t z)
{
    memset(a, 0, sizeof(*a));
    a->active = 1;
    a->in_ring = 1;
    a->life = 163;
    a->x_int = x;
    a->x_fixed = x << 16;
    a->z_int = z;
    a->z_fixed = z << 16;
}

static void test_final_confine_only_touches_the_attached(void)
{
    wm_arcade_actor_t master, puppet, bystander;
    wm_arcade_actor_t *actors[3];
    wm_arcade_frame_box_t frame;
    int32_t dragged_x;

    memset(&frame, 0, sizeof(frame));
    frame.iani3x = 10;
    frame.iani3y = 60;
    frame.iani3id = 60;

    place(&master, WM_RING_X_CENTER, WM_RING_TOP + 50);
    place(&puppet, WM_RING_X_CENTER, WM_RING_TOP + 50);
    place(&bystander, WM_RING_X_CENTER, WM_RING_TOP + 50);
    actors[0] = &master;
    actors[1] = &puppet;
    actors[2] = &bystander;

    /* Everyone has been through set_collision_boxes this tick. */
    wm_arcade_set_hurt_box(&master, &frame);
    wm_arcade_set_hurt_box(&puppet, &frame);
    wm_arcade_set_hurt_box(&bystander, &frame);
    assert(puppet.hurt_frame_valid);

    /* The master then drags his puppet clean out of the ring, which is
       the ordering hole final_confine exists to close: the puppet was
       already confined this tick, before he was moved. */
    master.attach_proc = &puppet;
    puppet.attach_proc = &master;
    dragged_x = WM_RING_X_CENTER + 4000;
    puppet.x_int = dragged_x;
    puppet.x_fixed = dragged_x << 16;
    bystander.x_int = dragged_x;
    bystander.x_fixed = dragged_x << 16;

    wm_arcade_final_confine(actors, 3);

    /* The puppet's boxes were recomputed at his new position -- that is
       what set_collision_boxes did, and it needed the frame the actor
       had kept from the first pass. */
    assert(puppet.hurt_box.x1 != 0 || puppet.hurt_box.x2 != 0);
    assert(puppet.hurt_box.x2 >= dragged_x - 4000);

    /*
     * The bystander has no ATTACH_PROC, so the `jrz #no_attach` skips
     * him entirely and his stale boxes stay stale. Confining everybody
     * would be the easy mistake here and it is not what the source
     * does.
     */
    assert(bystander.hurt_box.x2 < dragged_x);

    wm_arcade_final_confine(NULL, 3);
}

static void test_get_rope_x(void)
{
    /*
     * `cmpi RING_X_CENTER,a0 / jrgt #right` -- strictly greater picks
     * the right rope, so the centre line itself is the left one's.
     */
    int32_t z = WM_RING_TOP + 40;
    int32_t left = wm_ring_get_rope_x(WM_RING_X_CENTER - 200, z);
    int32_t centre = wm_ring_get_rope_x(WM_RING_X_CENTER, z);
    int32_t right = wm_ring_get_rope_x(WM_RING_X_CENTER + 200, z);

    assert(left == centre);
    assert(right != left);
    assert(left < WM_RING_X_CENTER);
    assert(right > WM_RING_X_CENTER);

    /* Past either end in Z, calc_line_x's own out-of-range answer. */
    assert(wm_ring_get_rope_x(WM_RING_X_CENTER - 200, WM_RING_TOP - 500) == 0);
    assert(wm_ring_get_rope_x(WM_RING_X_CENTER + 200, WM_RING_BOT + 500) == 0);
}

static void test_inc_getup_time(void)
{
    wm_arcade_actor_t a;
    memset(&a, 0, sizeof(a));

    a.getup_time = 40;
    wm_arcade_inc_getup_time(&a, 30);
    assert(a.getup_time == 70);

    /* At 20 it still counts -- the test is `jrlt`, not `jrle`. */
    a.getup_time = 20;
    wm_arcade_inc_getup_time(&a, 5);
    assert(a.getup_time == 25);

    /* Below 20 he is already on his way up and cannot be held down. */
    a.getup_time = 19;
    wm_arcade_inc_getup_time(&a, 100);
    assert(a.getup_time == 19);

    a.getup_time = 0;
    wm_arcade_inc_getup_time(&a, 100);
    assert(a.getup_time == 0);

    wm_arcade_inc_getup_time(NULL, 5);
}

static void test_xxx_dead_anim_head(void)
{
    wm_arcade_actor_t a;

    /* #set_pinable_bit: one OR, and it is what lets a corpse be pinned. */
    memset(&a, 0, sizeof(a));
    assert(wm_anim_code_run(&a, NULL, "#set_pinable_bit", "WRESTLE2.ASM", 0));
    assert(a.status_flags & WM_STATUS_PINABLE);

    /* #ko_if_drone: a dead drone is out for good. */
    memset(&a, 0, sizeof(a));
    a.plyr_type = WM_PTYPE_DRONE;
    assert(wm_anim_code_run(&a, NULL, "#ko_if_drone", "WRESTLE2.ASM", 0));
    assert(a.status_flags & WM_STATUS_KOD);

    /* A human is not -- he may still buy back in. */
    memset(&a, 0, sizeof(a));
    a.plyr_type = WM_PTYPE_PLAYER;
    assert(wm_anim_code_run(&a, NULL, "#ko_if_drone", "WRESTLE2.ASM", 0));
    assert(!(a.status_flags & WM_STATUS_KOD));

    /* Two refusals: already pinned (the pin is the ending), and NO_KO. */
    memset(&a, 0, sizeof(a));
    a.plyr_type = WM_PTYPE_DRONE;
    a.status_flags = WM_STATUS_PINNED;
    assert(wm_anim_code_run(&a, NULL, "#ko_if_drone", "WRESTLE2.ASM", 0));
    assert(!(a.status_flags & WM_STATUS_KOD));

    memset(&a, 0, sizeof(a));
    a.plyr_type = WM_PTYPE_DRONE;
    a.status_flags = WM_STATUS_NO_KO;
    assert(wm_anim_code_run(&a, NULL, "#ko_if_drone", "WRESTLE2.ASM", 0));
    assert(!(a.status_flags & WM_STATUS_KOD));
}

int main(void)
{
    test_final_confine_only_touches_the_attached();
    test_get_rope_x();
    test_inc_getup_time();
    test_xxx_dead_anim_head();
    printf("final_confine, get_rope_x, inc_getup_time, dead-anim head: "
           "all checks passed\n");
    return 0;
}
