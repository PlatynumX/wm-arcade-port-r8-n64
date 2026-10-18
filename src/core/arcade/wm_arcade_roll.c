/* WRESTLE2.ASM:1290 do_roll -- see wm/arcade/wm_arcade_roll.h. */
#include "wm/arcade/wm_arcade_roll.h"
#include "wm/roll_frames.h"
#include "wm/arcade/wm_arcade_combat_defs.h"

static int32_t iabs32(int32_t v) { return v < 0 ? -v : v; }

int wm_arcade_do_roll(wm_arcade_actor_t *a) {
    const wm_roll_table *t;
    int32_t speed, zvel;
    uint32_t pos, index;

    if (!a) return 0;

    /*
     * `move *a13(Z_BOUND),a14 / jrz #reg` -- a zero bound means no bound.
     * Otherwise, once he is within 6 of it he has arrived and stops.
     */
    if (a->z_bound != 0 && iabs32(a->z_bound - a->z_int) <= 6) goto no_roll;

    /* No up or down on the stick, no roll. */
    if (!(a->stick_val_cur & (WM_MOVE_UP | WM_MOVE_DOWN))) goto no_roll;

    if (a->wrestler_num < 0 || a->wrestler_num >= WM_ROLL_SLOTS) goto no_roll;
    t = &wm_roll_tables[a->wrestler_num];
    /* Adam Bomb's slot is a plain `.long 0` in the source's own table. */
    if (!t->frames || t->frame_count == 0) goto no_roll;

    speed = t->speed;
    zvel = t->zvel;
    /* `btst MOVE_DOWN_BIT,a0 / jrnz #down` -- down is the table as
       written; up negates both the frame advance and the velocity, so he
       rolls the other way and his artwork runs backwards. */
    if (!(a->stick_val_cur & WM_MOVE_DOWN)) {
        speed = -speed;
        zvel = -zvel;
    }
    a->z_vel = zvel;

    pos = (uint32_t)(a->roll_pos + speed) & 0xffu;
    a->roll_pos = (int32_t)pos;

    index = (pos * (uint32_t)t->multiplier) >> 16;
    if (index >= t->frame_count) index = (uint32_t)t->frame_count - 1u;
    a->roll_frame = t->frames[index];
    return 1;

no_roll:
    a->z_vel = 0;
    return 0;
}

void wm_arcade_tick_getup_time(wm_arcade_actor_t *a) {
    uint16_t pressed;
    int had_press_last;

    if (!a) return;
    if (a->getup_time == 0) return;

    /* `move *a13(DELAY_METER),a14 / jrz #reg` -- set means wipe it. */
    if (a->delay_meter) {
        a->getup_time = 0;
        return;
    }

    --a->getup_time;
    if (a->getup_time == 0) {
        /* #clr_dizzy */
        a->plyr_dizzy = 0;
        a->stars_flag = 0;
        return;
    }

    /*
     * The mash. The source ORs this tick's button-downs with its
     * stick-downs, records whether anything was down in M_PRESS_LAST, and
     * deducts three if anything is down NOW or was down LAST tick -- so a
     * release counts as well as a press.
     */
    pressed = (uint16_t)(a->but_val_down | a->stick_val_down);
    had_press_last = (a->status_flags & WM_STATUS_PRESS_LAST) != 0;
    if (pressed)
        a->status_flags |= (uint32_t)WM_STATUS_PRESS_LAST;
    else
        a->status_flags &= ~(uint32_t)WM_STATUS_PRESS_LAST;

    if (!pressed && !had_press_last) return;

    a->getup_time -= 3;
    if (a->getup_time < 0) a->getup_time = 0;
}

void wm_arcade_tick_wrestler_timers(wm_arcade_actor_t *a) {
    if (!a) return;
    if (a->delay_butns > 0) --a->delay_butns;
    if (a->safe_time > 0) --a->safe_time;
    if (a->delay_meter > 0) --a->delay_meter;
    if (a->immobilize_time > 0) --a->immobilize_time;
    /* `jrz #skp6 / jrn #skp6` -- zero AND negative both skip, so a
       negative WALK_FAST is left alone rather than counted further down. */
    if (a->walk_fast > 0) --a->walk_fast;
}
