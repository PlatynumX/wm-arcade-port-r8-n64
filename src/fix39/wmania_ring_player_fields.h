#ifndef WMANIA_RING_PLAYER_FIELDS_H
#define WMANIA_RING_PLAYER_FIELDS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Ring-relevant subset of the live PLYR.EQU wrestler PDATA.
 *
 * This is a merge-facing portable shape, not a claim that the arcade
 * structure is packed identically in C.  Names/semantics are direct source.
 */
typedef struct {
    int16_t can_move_dir;       /* CAN_MOVE_DIR */
    int16_t can_move_temp;      /* CAN_MOVE_TEMP */
    int16_t x_bound;            /* X_BOUND */
    int16_t z_bound;            /* Z_BOUND */
    int16_t move_dir;           /* MOVE_DIR */

    int16_t inring;             /* INRING: 0=in ring, 1=outside */
    int16_t ground_y;           /* GROUND_Y */
    int16_t rope_x_left;        /* PLYR_ROPE_X_LEFT */
    int16_t rope_x_right;       /* PLYR_ROPE_X_RIGHT */

    int16_t climbing_thru;      /* CLIMBING_THRU: 1=climbing thru ropes */
    int16_t outside_alone;      /* OUTSIDE_ALONE */

    int16_t ring_time;          /* RING_TIME: ring-out damage timing */
    int16_t climb_start;        /* CLIMB_START */
    int16_t climb_last;         /* CLIMB_LAST */

    int16_t player_mode;        /* PLYRMODE */
} WmRingPlayerFields;

/* Direct PLYR.EQU mode IDs consumed by ring/turnbuckle behavior. */
enum {
    WM_RING_MODE_NORMAL = 0,
    WM_RING_MODE_RUNNING = 1,
    WM_RING_MODE_INAIR = 2,
    WM_RING_MODE_ATTACHED = 3,
    WM_RING_MODE_ONGROUND = 4,
    WM_RING_MODE_BOUNCING = 5,
    WM_RING_MODE_ONTURNBUCKLE = 6,
    WM_RING_MODE_DEAD = 9,
    WM_RING_MODE_OPPOVERHEAD = 10,
    WM_RING_MODE_CLIMBTURNBUCKLE = 11,
    WM_RING_MODE_WAITANIM = 12,
    WM_RING_MODE_HEADHOLD = 16,
    WM_RING_MODE_HEADHELD = 19,
    WM_RING_MODE_INAIR2 = 21
};

#define WM_RING_ANIM_MODE_UNINT_BIT 2u
#define WM_RING_ANIM_MODE_UNINT 0x0004u
#define WM_RING_ANIM_MODE_NOCONFINE_BIT 7u
#define WM_RING_ANIM_MODE_NOCONFINE 0x0080u

#define WM_RING_STATUS_ZOMBIE_BIT 3u
#define WM_RING_STATUS_ZOMBIE (1u << WM_RING_STATUS_ZOMBIE_BIT)


#ifdef __cplusplus
}
#endif

#endif
