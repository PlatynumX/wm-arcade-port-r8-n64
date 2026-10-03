/*
 * WRESTLE2.ASM:950 getup_meter and the three routines around it.
 * See wm/arcade/wm_arcade_getup_meter.h for what is and is not here.
 */
#include "wm/arcade/wm_arcade_getup_meter.h"

#include "wm/arcade/wm_arcade_combat_defs.h"

#include <string.h>

wm_getup_meter_eligibility_t wm_arcade_getup_meter_eligible(
    wm_arcade_actor_t *actor, wm_arcade_actor_t *const *actors,
    size_t actor_count, bool drone_meters_on, int32_t num_opps) {
    size_t i;

    if (!actor) return WM_GETUP_METER_NO;

    /* "humans get getup meters" */
    if (actor->plyr_type == WM_PTYPE_PLAYER) return WM_GETUP_METER_YES;

    /* `#lp0` over process_ptrs: skip inactive, skip self, skip the other
       team, and a human teammate is the end of it. */
    for (i = 0; i < actor_count; ++i) {
        const wm_arcade_actor_t *o = actors ? actors[i] : NULL;
        if (!o || !o->active) continue;
        if (o == actor) continue;
        if (o->player_side != actor->player_side) continue;
        if (o->plyr_type == WM_PTYPE_PLAYER) {
            /* `#die`: "we have a human teammate so we don't get one" --
               and METER_PROC is cleared before the DIE. */
            actor->meter_proc = NULL;
            return WM_GETUP_METER_NO;
        }
    }

    /* "we got through the loop and found no teammates, so we're a lone
       drone and we get a meter only if GETUP_POWER is set." The live
       read is drone_meters_on; GETUP_POWER is commented out above it. */
    if (!drone_meters_on) {
        actor->meter_proc = NULL;
        return WM_GETUP_METER_NO;
    }
    /* `move @NUM_OPPS,a1 / cmpi 2,a1 / jrge #die` */
    if (num_opps >= 2) {
        actor->meter_proc = NULL;
        return WM_GETUP_METER_NO;
    }
    return WM_GETUP_METER_YES;
}

int32_t wm_arcade_getup_meter_side(int32_t player_side, int32_t player_num,
                                   bool royal_rumble) {
    /* "HACK ALERT: In royal rumble mode, player 1 is on PLAYER 0's TEAM
       ... we temporarily put him on the other team." */
    if (royal_rumble && player_num == 1) return 1;
    return player_side;
}

int32_t wm_arcade_getup_meter_offscr_x(int32_t side) {
    int32_t x = (int32_t)((uint32_t)WM_GETUP_OFFSCR_X << 16);
    if (side == 0) {
        --x;                    /* `dec a10` */
        x = -x;                 /* `neg a10` */
    }
    return x;
}

int32_t wm_arcade_getup_meter_target_x(int32_t side, bool onscreen) {
    /* #set_x: `movi [ONSCR_X,0],a0` (or OFFSCR_X, from #update), then
       `move a9,a9 / jrnz #p22 / neg a0`, then `addi [200-1,0],a0`. */
    int32_t a0 = (int32_t)((uint32_t)(onscreen ? WM_GETUP_ONSCR_X
                                               : WM_GETUP_OFFSCR_X) << 16);
    if (side == 0) a0 = -a0;
    return a0 + (int32_t)((uint32_t)(200 - 1) << 16);
}

int32_t wm_arcade_getup_meter_x_step(int32_t target, int32_t current) {
    /* `sub a1,a0 / sra 2,a0` -- a quarter of what is left, arithmetic,
       so it eases in from either side. */
    return (target - current) >> 2;
}

bool wm_arcade_getup_meter_may_show(const wm_arcade_actor_t *actor,
                                    const wm_getup_meter_env_t *env) {
    int32_t health;

    if (!actor) return false;

    /* `move *a10(WHOHITME),A0,L / move *a0(COMBO_COUNT),A14 / jrnz #cont`.
       The source does not check WHOHITME for zero first, and reads
       COMBO_COUNT off address 0 when nobody has hit him -- which on this
       hardware is the start of RAM and reliably reads as something. Here
       there is no such address, so no attacker is no combo. */
    if (actor->who_hit_me && actor->who_hit_me->combo_count != 0)
        return false;
    /* `move *a10(DELAY_METER),a14 / jrnz #cont` */
    if (actor->delay_meter != 0) return false;
    /* `cmpi MODE_DEAD,a14 / jrz #cont` */
    if (actor->player_mode == WM_PMODE_DEAD) return false;

    /*
     * The health arm, called exactly where the source calls it and
     * deciding exactly as much as it does there, which is nothing --
     * see the header. Both branches end at the same GETUP_TIME test.
     */
    health = env && env->get_health ? env->get_health(actor, env->user) : 0;
    if (health <= 20 && actor->getup_time == WM_FLUNG_TIME)
        return true;                       /* `jrz #onscr` */

    /* `#norm`: `move *a10(GETUP_TIME),a14 / jrnz #onscr` */
    return actor->getup_time != 0;
}

int32_t wm_arcade_getup_meter_smooth(int32_t display_val, int32_t scaled) {
    /* `add a0,a7 / srl 1,a7` -- a LOGICAL shift on a non-negative word. */
    uint32_t sum = (uint32_t)scaled + (uint32_t)display_val;
    return (int32_t)(sum >> 1);
}

int32_t wm_arcade_getup_meter_crop(int32_t display_val) {
    /* `neg a1 / addi GETUP_SIZE,a1 / jrp #ok / clr a1`, then the
       can't-be-taller clamp. */
    int32_t a1 = WM_GETUP_SIZE - display_val;
    if (a1 < 0) a1 = 0;
    if (a1 > WM_GETUP_SIZE) a1 = WM_GETUP_SIZE;
    return a1;
}

int32_t wm_arcade_getup_meter_scale(int32_t getup_time, int32_t display_val,
                                    int32_t *start_getup) {
    int32_t a7 = getup_time;
    int32_t a11;

    if (!start_getup) return 0;
    a11 = *start_getup;

    /* "if a7 (current getup) is greater than a11 (starting getup), our
       scale will be messed up. In this case, just move a7 into a11." */
    if (a7 > a11) a11 = a7;

    if (a11 == 0) { *start_getup = a11; return 0; }
    a7 = (int32_t)(((uint32_t)a7 * (uint32_t)WM_GETUP_SIZE) / (uint32_t)a11);

    /* "has getup been incremented?" -- `cmp a0,a7 / jrle #ok1`, so this
       arm is taken when the scaled value is ABOVE DISPLAY_VAL, and its
       repair sets a11 to the already-scaled a7 and divides by it. The
       result is GETUP_SIZE, and a11 is left holding a scaled value for
       every later tick to divide by. Transcribed, not repaired. */
    if (a7 > display_val) {
        a11 = a7;
        if (a11 != 0)
            a7 = (int32_t)(((uint32_t)a7 * (uint32_t)WM_GETUP_SIZE) /
                           (uint32_t)a11);
    }

    *start_getup = a11;
    return a7;
}

bool wm_arcade_getup_meter_start(wm_getup_meter_t *st,
                                 wm_arcade_actor_t *actor,
                                 wm_arcade_actor_t *const *actors,
                                 size_t actor_count,
                                 bool drone_meters_on, int32_t num_opps,
                                 bool royal_rumble) {
    if (!st) return false;
    memset(st, 0, sizeof(*st));
    st->phase = (uint8_t)WM_GETUP_PHASE_NONE;
    if (!actor) return false;

    /* The `SLEEPK 2` before the eligibility test buys the rest of the
       round's processes a chance to exist, and changes no answer that
       can be reached from here. */
    if (wm_arcade_getup_meter_eligible(actor, actors, actor_count,
                                       drone_meters_on,
                                       num_opps) != WM_GETUP_METER_YES)
        return false;

    /* `#yes`: DISPLAY_VAL cleared, the side settled, the objects made,
       and `move a13,*a10(METER_PROC),L`. */
    st->display_val = 0;
    st->side = wm_arcade_getup_meter_side(actor->player_side,
                                          actor->player_num, royal_rumble);
    actor->meter_proc = st;

    /* And then straight into slide_offscr, which is a SUBR the process
       falls into rather than calls. */
    st->phase = (uint8_t)WM_GETUP_PHASE_OFFSCR;
    st->grace = WM_GETUP_METER_GRACE;
    actor->delay_meter = WM_GETUP_METER_DELAY;
    return true;
}

void wm_arcade_getup_meter_tick(wm_getup_meter_t *st,
                                wm_arcade_actor_t *actor,
                                const wm_getup_meter_env_t *env,
                                wm_getup_meter_frame_t *out) {
    wm_getup_meter_frame_t f;

    memset(&f, 0, sizeof(f));
    if (!st || !actor) { if (out) *out = f; return; }

    switch ((wm_getup_meter_phase_t)st->phase) {
    case WM_GETUP_PHASE_NONE:
        break;

    case WM_GETUP_PHASE_OFFSCR:
        f.x = wm_arcade_getup_meter_target_x(st->side, false);
        f.green_height = st->display_val;
        f.crop_rows = wm_arcade_getup_meter_crop(st->display_val);
        /* `move a11,a11 / jrz #update / dec a11 / jruc #cont`: ten ticks
           of not asking, and then no reload, so every tick after. */
        if (st->grace > 0) { --st->grace; break; }
        if (!wm_arcade_getup_meter_may_show(actor, env)) break;
        /* `#onscr`: a11 is the GETUP_TIME the bar will be scaled
           against -- a14, which on both arms into #onscr holds
           GETUP_TIME. */
        st->phase = (uint8_t)WM_GETUP_PHASE_ONSCR;
        st->start_getup = actor->getup_time;
        st->display_val = WM_GETUP_SIZE;
        st->dufus_countdown = WM_GETUP_DUFUS_TICK;
        st->getup_at_onscr = actor->getup_time;
        f.announce_sound = true;            /* `triple_sound 0BDh` */
        f.x = wm_arcade_getup_meter_target_x(st->side, true);
        f.visible = true;
        f.green_height = st->display_val;
        f.crop_rows = wm_arcade_getup_meter_crop(st->display_val);
        break;

    case WM_GETUP_PHASE_ONSCR: {
        int32_t scaled;
        f.visible = true;
        f.x = wm_arcade_getup_meter_target_x(st->side, true);

        scaled = wm_arcade_getup_meter_scale(actor->getup_time,
                                             st->display_val,
                                             &st->start_getup);

        /* `subk 1,a6 / jrnz #dont_bother`: once, on the tick it hits
           zero, and never again -- it keeps counting down past it. */
        if (st->dufus_countdown > 0 && --st->dufus_countdown == 0) {
            int32_t burned = st->getup_at_onscr - actor->getup_time;
            st->getup_at_onscr = burned;     /* `sub a0,a5` writes a5 */
            if (burned <= WM_GETUP_DUFUS_BURN) f.dufus_message = true;
        }

        st->display_val = wm_arcade_getup_meter_smooth(st->display_val,
                                                       scaled);
        f.green_height = st->display_val;
        f.crop_rows = wm_arcade_getup_meter_crop(st->display_val);

        /* `move a7,a7 / jrz slide_offscr` -- and a7 is the AVERAGED
           value by then, because #update_meter wrote it back into the
           register and the caller did not save it. */
        if (st->display_val == 0 ||
            actor->player_mode == WM_PMODE_DEAD) {
            st->phase = (uint8_t)WM_GETUP_PHASE_OFFSCR;
            st->grace = WM_GETUP_METER_GRACE;
            actor->delay_meter = WM_GETUP_METER_DELAY;
        }
        break;
    }
    }

    if (out) *out = f;
}

bool wm_arcade_ditch_getup_meter(wm_arcade_actor_t *actor,
                                 wm_getup_meter_t *st) {
    if (!actor) return false;
    /* `move *a13(GETUP_TIME),a0 / jrz #cont` */
    if (actor->getup_time == 0) return false;
    /* `move *a13(PLYR_DIZZY),a0 / jrnz #cont` */
    if (actor->plyr_dizzy != 0) return false;
    /* `move *a13(METER_PROC),a0,L / jrz #cont` */
    if (!actor->meter_proc) return false;
    if (!st || actor->meter_proc != st) return false;

    /* `movi slide_offscr,a7 / calla XFERPROC`: the process restarts at
       the top of slide_offscr, which is where the eighteen seconds are
       stamped. */
    st->phase = (uint8_t)WM_GETUP_PHASE_OFFSCR;
    st->grace = WM_GETUP_METER_GRACE;
    actor->delay_meter = WM_GETUP_METER_DELAY;
    return true;
}

bool wm_arcade_ditch_getup_meter_a9(wm_arcade_actor_t *other,
                                    wm_getup_meter_t *st) {
    /* `PUSH a13 / move a9,a13 / callr ditch_getup_meter / PULL a13` */
    return wm_arcade_ditch_getup_meter(other, st);
}
