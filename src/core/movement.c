#include "wm/movement.h"
#include "wm/arcade/wm_arcade_bret.h"
#include <stdbool.h>
#include <stddef.h>

/* WRESTLE.ASM:4513-4529 #convert_table, indexed by WM_MOVE_* (0-10). */
static const int convert_table[16] = {
    -1, 0, 4, -1, 6, 7, 5, -1, 2, 1, 3, -1, -1, -1, -1, -1
};

int wm_convert_facing(int32_t move_dir_bits) {
    uint32_t idx = (uint32_t)move_dir_bits & 0x0fu;
    return convert_table[idx];
}

/* BRET.ASM:2848 hrt_velocity_table. #VEL=WM_BRET_WALK_VEL,
   #DVEL=WM_BRET_WALK_DVEL (wm/arcade/wm_arcade_bret.h). */
const wm_move_velocity_entry wm_bret_velocity_table[8] = {
    {  0,                    -WM_BRET_WALK_VEL }, /* 0 UP */
    {  WM_BRET_WALK_DVEL,    -WM_BRET_WALK_DVEL }, /* 1 UP-RIGHT */
    {  WM_BRET_WALK_VEL,      0 },                 /* 2 RIGHT */
    {  WM_BRET_WALK_DVEL,     WM_BRET_WALK_DVEL }, /* 3 DOWN-RIGHT */
    {  0,                     WM_BRET_WALK_VEL },  /* 4 DOWN */
    { -WM_BRET_WALK_DVEL,     WM_BRET_WALK_DVEL }, /* 5 DOWN-LEFT */
    { -WM_BRET_WALK_VEL,      0 },                 /* 6 LEFT */
    { -WM_BRET_WALK_DVEL,    -WM_BRET_WALK_DVEL }, /* 7 UP-LEFT */
};

/* WRESTLE.ASM:5431-5432. 256 == 1.0 in this 8-bit fixed scale. */
#define WM_MOVE_MULT_BACKWARD 230      /* 256*90/100 */
#define WM_MOVE_MULT_GROUND   384      /* 256*150/100 */

static int32_t scale(int32_t v, int32_t mult) {
    return (int32_t)(((int64_t)v * mult) >> 8);
}

void wm_set_velocities(wm_arcade_actor_t *actor,
                       const wm_arcade_actor_t *opponent,
                       const wm_move_velocity_entry table[8]) {
    int facing;
    int32_t velx, velz;
    int32_t move_or_facing;
    bool ongrnd;

    if (!actor || !table) return;
    facing = wm_convert_facing(actor->move_dir);
    if (facing < 0) return;

    velx = table[facing].x;
    velz = table[facing].z;

    /* WRESTLE.ASM:5450-5459. */
    ongrnd = actor->walk_fast != 0;
    if (!ongrnd && opponent &&
        (opponent->player_mode == WM_PMODE_ONGROUND ||
         opponent->player_mode == WM_PMODE_DEAD))
        ongrnd = true;

    move_or_facing = actor->move_dir | actor->facing_dir;
    if (ongrnd) {
        velx = scale(velx, WM_MOVE_MULT_GROUND);
    } else if ((move_or_facing & (WM_MOVE_LEFT | WM_MOVE_RIGHT)) ==
               (WM_MOVE_LEFT | WM_MOVE_RIGHT)) {
        velx = scale(velx, WM_MOVE_MULT_BACKWARD);
    }
    actor->x_vel = velx;

    /* WRESTLE.ASM:5483-5495: Z never gets the ground/walk-fast boost. */
    if ((move_or_facing & (WM_MOVE_UP | WM_MOVE_DOWN)) ==
        (WM_MOVE_UP | WM_MOVE_DOWN))
        velz = scale(velz, WM_MOVE_MULT_BACKWARD);
    actor->z_vel = velz;
}

void wm_execute_walk(wm_arcade_actor_t *actor,
                     const wm_arcade_actor_t *opponent,
                     const wm_move_velocity_entry table[8]) {
    int32_t dir;
    if (!actor || !table) return;
    dir = actor->move_dir;

    switch (dir) {
        case WM_MOVE_ZIP:
            actor->move_dir = 0;
            actor->x_vel = 0;
            actor->z_vel = 0;
            /* WRESTLE.ASM:5286 `callr set_rotate_anim ;or stance`, called
               every idle (#zip) tick: set_rotate_anim's own body (WRESTLE.
               ASM:5082-5083) does this copy unconditionally and
               synchronously, before/regardless of which turn animation it
               separately picks -- see wm/bret_backend.h for that
               still-unported half. */
            actor->facing_dir = actor->new_facing_dir;
            return;
        case WM_MOVE_UP:
        case WM_MOVE_DOWN:
            break; /* WRESTLE.ASM #up/#down: OBJ_CONTROL untouched. */
        case WM_MOVE_UP_RIGHT:
        case WM_MOVE_RIGHT:
        case WM_MOVE_DOWN_RIGHT:
            actor->obj_control = (uint16_t)(actor->obj_control & ~WM_OBJ_FLIPH);
            break;
        case WM_MOVE_DOWN_LEFT:
        case WM_MOVE_LEFT:
        case WM_MOVE_UP_LEFT:
            actor->obj_control = (uint16_t)(actor->obj_control | WM_OBJ_FLIPH);
            break;
        default:
            /* Not one of WRESTLE.ASM's 9 walk_table entries with real
               bodies (3/7/11-15 all alias #zip); treat the same way,
               including the FACING_DIR catch-up above. */
            actor->move_dir = 0;
            actor->x_vel = 0;
            actor->z_vel = 0;
            actor->facing_dir = actor->new_facing_dir;
            return;
    }

    actor->move_dir = dir;
    wm_set_velocities(actor, opponent, table);
}

void wm_integrate_position(wm_arcade_actor_t *actor) {
    if (!actor) return;
    actor->x_fixed += actor->x_vel;
    actor->z_fixed += actor->z_vel;
    actor->x_int = actor->x_fixed >> 16;
    actor->z_int = actor->z_fixed >> 16;
}
