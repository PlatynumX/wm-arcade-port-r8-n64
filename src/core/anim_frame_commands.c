#include "wm/anim_frame_commands.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/movement.h"

#include <string.h>

/* ANIM.ASM:1626 _ani_set_xvel / :1829 _ani_set_zvel: the value is used as
   written when absolute, and negated when the direction bit the mode names
   is clear. The bit differs per axis -- x tests MOVE_RIGHT, z tests
   MOVE_DOWN -- which is why this takes the bit rather than assuming one. */
static int32_t directional(const wm_arcade_actor_t *actor, int32_t value,
                           uint8_t mode, uint16_t bit) {
    uint16_t dir;
    switch (mode) {
        case 1: dir = (uint16_t)actor->facing_dir; break;        /* AM_FACE_REL */
        case 2: dir = (uint16_t)actor->hit_side; break;     /* AM_HIT_REL */
        case 3: dir = (uint16_t)actor->new_facing_dir; break;    /* AM_NEWFACE_REL */
        default: return value;                         /* AM_ABS */
    }
    return (dir & bit) ? value : -value;
}

const char *wm_anim_apply_frame_commands(wm_arcade_actor_t *actor,
                                         const char *source_label,
                                         size_t frame_index) {
    const wm_anim_frame_command *cmds;
    size_t count, i;
    const char *become = NULL;

    if (!actor || !source_label) return NULL;
    cmds = wm_anim_frame_commands(&count);

    for (i = 0; i < count; ++i) {
        const wm_anim_frame_command *c = &cmds[i];
        if (c->frame_index != frame_index) continue;
        if (strcmp(c->source_label, source_label) != 0) continue;

        switch (c->kind) {
            case WM_ANICMD_ZEROVELS:
                actor->x_vel = 0;
                actor->y_vel = 0;
                actor->z_vel = 0;
                break;
            case WM_ANICMD_ZERO_XZVELS:
                actor->x_vel = 0;
                actor->z_vel = 0;
                break;
            case WM_ANICMD_SET_XVEL:
                actor->x_vel = directional(actor, c->a, c->mode,
                                           (uint16_t)WM_MOVE_RIGHT);
                break;
            case WM_ANICMD_SET_YVEL:
                actor->y_vel = c->a;
                break;
            case WM_ANICMD_SET_ZVEL:
                actor->z_vel = directional(actor, c->a, c->mode,
                                           (uint16_t)WM_MOVE_DOWN);
                break;
            case WM_ANICMD_MIN_YVEL:
                /* "sets YVEL to given value, UNLESS it's already higher" */
                if (actor->y_vel < c->a) actor->y_vel = c->a;
                break;
            case WM_ANICMD_FRICTION:
                actor->friction = c->a;
                actor->anim_mode |= (uint16_t)WM_MODE_FRICTION;
                break;
            case WM_ANICMD_OFFSET: {
                /* x is negated when not facing right; y and z are absolute.
                   Positions are integer here and mirrored into the 16.16
                   twins the movement code integrates. */
                int32_t dx = (actor->facing_dir & WM_MOVE_RIGHT) ? c->a : -c->a;
                actor->x_int += dx;
                actor->y_int += c->b;
                actor->z_int += c->c;
                actor->x_fixed = actor->x_int << 16;
                actor->y_fixed = actor->y_int << 16;
                actor->z_fixed = actor->z_int << 16;
                break;
            }
            case WM_ANICMD_IFBUTTONS:
                /* "and a1,a0 / cmp a1,a0 / jrne #fail": every named button
                   must be held, not just any of them. */
                if (c->target &&
                    ((uint32_t)actor->but_val_cur & (uint32_t)c->a) ==
                        (uint32_t)c->a)
                    become = c->target;
                break;
            default:
                break;
        }
    }
    return become;
}
