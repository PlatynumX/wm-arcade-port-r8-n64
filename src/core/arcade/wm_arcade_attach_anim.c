#include "wm/arcade/wm_arcade_attach_anim.h"

#include <stddef.h>

static int reciprocal(const wm_arcade_actor_t *a, const wm_arcade_actor_t *b)
{
    return a && b && b->attach_proc == a;
}

void wm_arcade_anim_enter_slave_idle(wm_arcade_actor_t *a)
{
    if (!a) return;
    /* wres_slave_anim: SETMODE UNINT|NOAUTOFLIP|NOGRAVITY, ZEROVELS, SPEED 100h, END. */
    a->anim_mode = (uint16_t)(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP | WM_MODE_NOGRAVITY | WM_MODE_END);
    a->x_vel = a->y_vel = a->z_vel = 0;
    a->ani_speed = 0x0100u;
    /* ANI_SETMODE also clears SF_CLEAR_BITS. */
    a->status_flags &= ~(WM_STATUS_SCROLL_CTRL | WM_STATUS_DEAD_ANIM |
                         WM_STATUS_DID_RAISEARM | WM_STATUS_KOD |
                         WM_STATUS_COMBO_BROKEN | WM_STATUS_PUSH);
    if (a->ptime != 0) a->ptime = 1;
}

wm_arcade_attach_status_t wm_arcade_anim_detach(wm_arcade_actor_t *a)
{
    wm_arcade_actor_t *opp;
    if (!a || !a->attach_proc) return WM_ATTACH_NO_TARGET;
    opp = a->attach_proc;
    a->attach_proc = NULL;
    if (!reciprocal(a, opp)) return WM_ATTACH_NOT_RECIPROCAL;
    opp->attach_proc = NULL;
    if (opp->player_mode == WM_PMODE_PUPPET ||
        opp->player_mode == WM_PMODE_PUPPET2 ||
        opp->player_mode == WM_PMODE_ATTACHED)
        opp->player_mode = WM_PMODE_ONGROUND;
    return WM_ATTACH_OK;
}

wm_arcade_attach_status_t wm_arcade_anim_set_attach_from_whoihit(wm_arcade_actor_t *a)
{
    wm_arcade_actor_t *opp;
    if (!a || !a->who_i_hit) return WM_ATTACH_NO_TARGET;
    opp = a->who_i_hit;
    a->attach_proc = opp;
    opp->attach_proc = a;
    return WM_ATTACH_OK;
}

wm_arcade_attach_status_t wm_arcade_anim_set_opp_mode_bits(wm_arcade_actor_t *a, uint16_t bits)
{
    wm_arcade_actor_t *opp;
    if (!a || !a->attach_proc) return WM_ATTACH_NO_TARGET;
    opp = a->attach_proc;
    /* Source command 80 only requires both ATTACH_PROC values to be non-null. */
    if (!opp->attach_proc) return WM_ATTACH_NOT_RECIPROCAL;
    opp->anim_mode = (uint16_t)(opp->anim_mode | bits);
    return WM_ATTACH_OK;
}

wm_arcade_attach_status_t wm_arcade_anim_clear_opp_mode_bits(wm_arcade_actor_t *a, uint16_t bits)
{
    wm_arcade_actor_t *opp;
    if (!a || !a->attach_proc) return WM_ATTACH_NO_TARGET;
    opp = a->attach_proc;
    if (!opp->attach_proc) return WM_ATTACH_NOT_RECIPROCAL;
    opp->anim_mode = (uint16_t)(opp->anim_mode & (uint16_t)~bits);
    return WM_ATTACH_OK;
}

wm_arcade_attach_status_t wm_arcade_anim_set_opp_player_mode(wm_arcade_actor_t *a, uint16_t mode)
{
    wm_arcade_actor_t *opp;
    if (!a || !a->attach_proc) return WM_ATTACH_NO_TARGET;
    opp = a->attach_proc;
    if (!reciprocal(a, opp)) return WM_ATTACH_NOT_RECIPROCAL;
    if (opp->player_mode != WM_PMODE_DEAD) opp->player_mode = mode;
    return WM_ATTACH_OK;
}

wm_arcade_attach_status_t wm_arcade_anim_xflip_opp(wm_arcade_actor_t *a)
{
    wm_arcade_actor_t *opp;
    if (!a || !a->attach_proc) return WM_ATTACH_NO_TARGET;
    opp = a->attach_proc;
    if (!reciprocal(a, opp)) return WM_ATTACH_NOT_RECIPROCAL;
    opp->obj_control ^= WM_OBJ_FLIPH;
    return WM_ATTACH_OK;
}

wm_arcade_attach_status_t wm_arcade_anim_set_opp_vels(wm_arcade_actor_t *a,
                                                       int32_t xvel,
                                                       int32_t yvel,
                                                       int32_t zvel)
{
    wm_arcade_actor_t *opp = NULL;
    if (!a) return WM_ATTACH_NO_TARGET;
    if (a->attach_proc && reciprocal(a, a->attach_proc)) opp = a->attach_proc;
    else opp = a->who_i_hit; /* exact command 110 fallback */
    if (!opp) return WM_ATTACH_NO_TARGET;

    opp->y_vel = yvel;
    if ((a->facing_dir & WM_MOVE_RIGHT) == 0) xvel = -xvel;
    if ((a->facing_dir & WM_MOVE_DOWN) == 0) zvel = -zvel;
    opp->x_vel = xvel;
    opp->z_vel = zvel;
    return WM_ATTACH_OK;
}

wm_arcade_attach_status_t wm_arcade_master_keep_attached(wm_arcade_actor_t *a)
{
    wm_arcade_actor_t *opp;
    int32_t xoff;
    int32_t new_floor;
    if (!a || !a->attach_proc) return WM_ATTACH_NO_TARGET;
    opp = a->attach_proc;
    if (!opp->attach_proc) return WM_ATTACH_NOT_RECIPROCAL;

    /* If the attached opponent is grounded and not ghosted, master cannot
       sink below the floor implied by ATTACH_YOFF. */
    if (opp->y_fixed <= (opp->ground_y * 65536) &&
        (opp->anim_mode & WM_MODE_GHOST) == 0) {
        new_floor = opp->y_fixed - (a->attach_yoff * 65536);
        if (a->y_fixed < new_floor) a->y_fixed = new_floor;
    }

    opp->y_vel = 0;
    opp->z_fixed = a->z_fixed + (a->attach_zoff * 65536);
    opp->y_fixed = a->y_fixed + (a->attach_yoff * 65536);
    xoff = a->attach_xoff * 65536;
    if ((a->facing_dir & WM_MOVE_RIGHT) == 0) xoff = -xoff;
    opp->x_fixed = a->x_fixed + xoff;
    return WM_ATTACH_OK;
}

wm_arcade_attach_status_t wm_arcade_keep_attached(wm_arcade_actor_t *a)
{
    wm_arcade_actor_t *master;
    int32_t xoff;
    if (!a || !a->attach_proc) return WM_ATTACH_NO_TARGET;
    master = a->attach_proc;
    if (!master->attach_proc) return WM_ATTACH_NOT_RECIPROCAL;

    a->y_vel = 0;
    a->z_fixed = master->z_fixed + (master->attach_zoff * 65536);
    a->y_fixed = master->y_fixed + (master->attach_yoff * 65536);
    xoff = master->attach_xoff * 65536;
    if ((master->facing_dir & WM_MOVE_RIGHT) == 0) xoff = -xoff;
    a->x_fixed = master->x_fixed + xoff;
    return WM_ATTACH_OK;
}
