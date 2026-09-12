/*
 * FIREWORK.ASM -- the winner's fireworks. See wm/arcade/
 * wm_arcade_firework.h; the tables are src/generated/firework_tables.c.
 */
#include "wm/arcade/wm_arcade_firework.h"

/* ---- the camera -------------------------------------------------- */

void wm_fw_calc_dxdy(wm_fw_camera_t *c) {
    if (!c) return;
    /*
     * `sub a11,a9 / divs a10,a9`. A zero tick count would be a divide
     * by zero on hardware; the table has none, and this refuses to
     * invent a behaviour for one.
     */
    if (c->ticks == 0) {
        c->dx = 0;
        c->dy = 0;
        return;
    }
    c->dx = (c->p2x - c->p1x) / c->ticks;
    c->dy = (c->p2y - c->p1y) / c->ticks;
}

void wm_fw_move_camera(wm_fw_camera_t *c) {
    if (!c) return;
    c->worldtlx += c->dx;
    c->worldtly += c->dy;
}

/* `sra 16` -- arithmetic, so a negative world Y floors. */
static int32_t whole_pixels(int32_t fixed) {
    return fixed >> 16;
}

static int32_t iabs32(int32_t v) {
    return v < 0 ? -v : v;
}

void wm_fw_check_camera_position(wm_fw_camera_t *c) {
    if (!c) return;
    /* `cmpi 3,a14 / jrgt` -- within three INCLUSIVE stops the axis. */
    if (iabs32(whole_pixels(c->worldtlx) - (int32_t)c->targ_x) <= 3) {
        c->dx = 0;
    }
    if (iabs32(whole_pixels(c->worldtly) - (int32_t)c->targ_y) <= 3) {
        c->dy = 0;
    }
}

bool wm_fw_segment_done(const wm_fw_camera_t *c) {
    if (!c) return true;
    /* `or a0,a14 / jrnz` -- both, not either. */
    return (c->dx | c->dy) == 0;
}

int wm_fw_congrats_index(bool royal_rumble, bool is_8_on_1) {
    /* `move @royal_rumble,a14 / jrnz #do_8_on_2`, then
       `calla is_8_on_1 / jrc #do_8_on_1`, else #do_3_on_1. */
    if (royal_rumble) return WM_FW_CONGRATS_2V8;
    if (is_8_on_1) return WM_FW_CONGRATS_1V8;
    return WM_FW_CONGRATS_1V3;
}

/* ---- pan_around -------------------------------------------------- */

/* `#p1on8_lp`: load the waypoint, set the targets, work out the
   deltas. Costs no tick. */
static void load_waypoint(wm_fw_pan_t *p) {
    const wm_fw_waypoint_t *w = &wm_fw_panning_points[p->waypoint];
    p->cam.ticks = w->ticks;
    p->cam.p2x = w->x << 16;
    p->cam.p2y = w->y << 16;
    /* `srl 16,a14 / move a14,@targ_x` -- a LOGICAL shift into a
       16-bit cell, so a negative Y's top word is what lands there.
       Sign-extending it back out of that 16-bit cell is what
       check_camera_position then compares, and for these values
       (x 700..1000, y -308..-212) that is the same number the
       arithmetic shift would have given. */
    p->cam.targ_x = (int16_t)(uint16_t)(((uint32_t)p->cam.p2x) >> 16);
    p->cam.targ_y = (int16_t)(uint16_t)(((uint32_t)p->cam.p2y) >> 16);
    wm_fw_calc_dxdy(&p->cam);
}

void wm_fw_pan_begin(wm_fw_pan_t *p, int32_t x0, int32_t y0) {
    if (!p) return;
    p->phase = WM_FW_PAN_START;
    p->timer = 53 / 2;          /* `SLEEP TSEC/2` */
    p->waypoint = 0;
    p->text_shown = false;
    p->pan_down = false;
    p->laps = 0;
    p->update_background = false;
    p->cheer = false;
    p->show_congrats = false;
    p->delete_text = false;
    p->congrats_set = -1;

    p->cam.worldtlx = x0;
    p->cam.worldtly = y0;
    p->cam.dx = 0;
    p->cam.dy = 0;
    p->cam.ticks = 0;
    /* `move a14,@final_x / move a14,@p1x` -- both, from WORLDTLX. */
    p->cam.final_x = x0;
    p->cam.final_y = y0;
    p->cam.p1x = x0;
    p->cam.p1y = y0;
    p->cam.p2x = x0;
    p->cam.p2y = y0;
    p->cam.targ_x = 0;
    p->cam.targ_y = 0;
}

void wm_fw_pan_come_down(wm_fw_pan_t *p) {
    if (p) p->pan_down = true;
}

/* One flying tick, shared by #next_move and #npb_move: move, update
   the background, sleep one, cheer, then re-test the deltas. */
static void fly_one_tick(wm_fw_pan_t *p) {
    wm_fw_move_camera(&p->cam);
    p->update_background = true;
    p->cheer = true;
    wm_fw_check_camera_position(&p->cam);
}

/*
 * `#text_is_up` onward, shared by the ordinary end of a segment and
 * the one that has just put the text up: roll p1 forward to p2, then
 * either head home or take the next waypoint.
 */
static void advance_segment(wm_fw_pan_t *p) {
    /* `move @p2x,a14,L / move a14,@p1x,L` and the same for Y. */
    p->cam.p1x = p->cam.p2x;
    p->cam.p1y = p->cam.p2y;

    /* `move @pan_down,a14 / jrnz #pan_done` -- read only here, so a
       segment always finishes before the camera turns for home. */
    if (p->pan_down) {
        p->delete_text = true;              /* `obj_del1c CLSMK3` */
        p->cam.p2x = p->cam.final_x;
        p->cam.p2y = p->cam.final_y;
        p->cam.targ_x = (int16_t)(uint16_t)(((uint32_t)p->cam.p2x) >> 16);
        p->cam.targ_y = (int16_t)(uint16_t)(((uint32_t)p->cam.p2y) >> 16);
        p->cam.ticks = 53;                  /* INIT_PAN_SPEED */
        wm_fw_calc_dxdy(&p->cam);
        p->phase = WM_FW_PAN_BACK;
        return;
    }

    p->waypoint++;
    if (p->waypoint >= wm_fw_panning_point_count) {
        /* `move *a8,a14,L / jrnz #p1on8_lp` hit the terminator, and
           `move @pan_down,a14 / jrz #pan_again` sends it round again. */
        p->laps++;
        p->waypoint = 0;
    }
    load_waypoint(p);
    p->phase = WM_FW_PAN_SEGMENT;
}

bool wm_fw_pan_tick(wm_fw_pan_t *p, bool royal_rumble, bool is_8_on_1) {
    bool spent = false;

    if (!p) return false;
    p->update_background = false;
    p->cheer = false;
    p->show_congrats = false;
    p->delete_text = false;

    for (;;) {
        switch (p->phase) {
        case WM_FW_PAN_START:
            /* `SLEEP TSEC/2` before the camera is even read. */
            if (p->timer > 0) {
                if (spent) return true;
                p->timer--;
                spent = true;
                continue;
            }
            p->waypoint = 0;
            load_waypoint(p);
            p->phase = WM_FW_PAN_SEGMENT;
            continue;

        case WM_FW_PAN_SEGMENT:
            /* `#next_move`: move, BGND_UD1, SLEEPK 1, cheer, check. */
            if (spent) return true;
            fly_one_tick(p);
            spent = true;
            if (!wm_fw_segment_done(&p->cam)) continue;
            /* `move @pan_status,a14 / jrnz #text_is_up` -- once. */
            if (!p->text_shown) {
                p->text_shown = true;
                p->timer = 5;               /* `SLEEPK 5` */
                p->phase = WM_FW_PAN_TEXT_DELAY;
                continue;
            }
            advance_segment(p);
            continue;

        case WM_FW_PAN_TEXT_DELAY:
            if (p->timer > 0) {
                if (spent) return true;
                p->timer--;
                spent = true;
                continue;
            }
            /* The three-way choice, then `callr print_congrats`. */
            p->congrats_set = wm_fw_congrats_index(royal_rumble, is_8_on_1);
            p->show_congrats = true;
            p->timer = 2;                   /* `SLEEPK 2` */
            p->phase = WM_FW_PAN_TEXT_HOLD;
            continue;

        case WM_FW_PAN_TEXT_HOLD:
            if (p->timer > 0) {
                if (spent) return true;
                p->timer--;
                spent = true;
                continue;
            }
            advance_segment(p);
            continue;

        case WM_FW_PAN_BACK:
            /* `#npb_move`, the same loop flying home. */
            if (spent) return true;
            fly_one_tick(p);
            spent = true;
            if (!wm_fw_segment_done(&p->cam)) continue;
            p->phase = WM_FW_PAN_DONE;      /* `DIE` */
            continue;

        case WM_FW_PAN_DONE:
        default:
            return spent;
        }
    }
}

/* ---- firework_flare ---------------------------------------------- */

void wm_fw_flare_begin(wm_fw_flare_t *f, wm_fw_point_t pos,
                       int32_t delay, int32_t palette) {
    if (!f) return;
    f->phase = WM_FW_FLARE_DELAY;
    f->timer = delay;
    f->at = 0;
    f->palette = palette;
    /*
     * a9 holds the position packed as one long: `move a9,a0 /
     * srl 16,a0 / sll 16,a0` is X, `move a9,a1 / sll 16,a1` is Y.
     */
    f->x = pos.x;
    f->y = pos.y;
    f->changed = false;
    f->frame = 0;
    f->created = false;
}

bool wm_fw_flare_tick(wm_fw_flare_t *f, bool fizzle) {
    bool spent = false;

    if (!f) return false;
    f->changed = false;

    for (;;) {
        switch (f->phase) {
        case WM_FW_FLARE_DELAY:
            /* `PRCSLP` with a0 = 1..26. */
            if (f->timer > 0) {
                if (spent) return true;
                f->timer--;
                spent = true;
                if (f->timer > 0) return true;
            }
            /* BEGINOBJP with the first image of #flare_anim. */
            f->created = true;
            f->at = 0;
            f->frame = 0;
            f->phase = WM_FW_FLARE_RUN;
            continue;

        case WM_FW_FLARE_RUN:
        case WM_FW_FLARE_LOOP:
            /* `#flare_loop: SLEEPK 2 / #fl_loop: move *a9+,a0,L`. */
            if (spent) return true;
            spent = true;
            f->timer = WM_FW_FLARE_STEP_TICKS;
            if ((size_t)f->at < wm_fw_flare_anim_count) {
                f->frame = f->at;
                f->at++;
                f->changed = true;
                return true;
            }
            /*
             * `#reset_flare`. Reached with no sleep of its own --
             * `jruc #fl_loop` lands past the SLEEPK -- so the first
             * image of the next pass shows on this same tick.
             */
            if (fizzle) {
                f->phase = WM_FW_FLARE_FIZZLE;
                f->at = WM_FW_FLARE_ANIM2;
                continue;
            }
            f->at = WM_FW_FLARE_ANIM2;
            f->phase = WM_FW_FLARE_LOOP;
            f->frame = f->at;
            f->at++;
            f->changed = true;
            return true;

        case WM_FW_FLARE_FIZZLE:
            /*
             * `#ff_loop: SLEEPK 4 / cmpi #flare_anim,a9 / jrz #ff_exit
             * / move -*a9,a0,L`. Pre-decrement, so it plays index 4
             * down to 0 and then exits on the comparison.
             */
            if (spent) return true;
            spent = true;
            if (f->at == 0) {
                f->phase = WM_FW_FLARE_DEAD;   /* DELOBJA8 / DIE */
                return true;
            }
            f->at--;
            f->frame = f->at;
            f->changed = true;
            return true;

        case WM_FW_FLARE_DEAD:
        default:
            return false;
        }
    }
}

/* ---- animate_fwexp ----------------------------------------------- */

void wm_fw_explosion_begin(wm_fw_explosion_t *e, int32_t rnd_y,
                           int32_t rnd_x, int32_t rnd_z, int32_t which,
                           int32_t palette) {
    if (!e) return;
    /* `movi 96,a0 / RNDRNG0 / addi EXP_FWY-48,a0`. */
    e->y = rnd_y + (WM_FW_EXP_FWY - 48);
    /* `movi 350,a0 / RNDRNG0 / addi 850,a0`. */
    e->x = rnd_x + 850;
    /* `movi 200h,a0 / RNDRNG0 / addi 700h,a0`. */
    e->z = rnd_z + 0x700;
    e->which = which;
    e->palette = palette;
    e->at = 0;
    e->changed = false;
    e->created = true;
    e->dead = false;
}

bool wm_fw_explosion_tick(wm_fw_explosion_t *e) {
    const char *const *list;
    size_t count;

    if (!e || e->dead) return false;
    e->changed = false;
    list = e->which ? wm_fw_fwexb_anim : wm_fw_fwexa_anim;
    count = e->which ? wm_fw_fwexb_anim_count : wm_fw_fwexa_anim_count;
    (void)list;

    /*
     * `#animate_loop: SLEEPK 1 / move *a11+,a0,L / jrz #fwanim_done`.
     * The object was created with image [0] already up, so the loop
     * starts by showing image [1].
     */
    e->at++;
    if ((size_t)e->at >= count) {
        e->dead = true;            /* DELOBJA8 / DIE */
        return false;
    }
    e->changed = true;
    /* `addk 4,a9 / move a9,*a8(OYPOS)` -- it falls as it burns. */
    e->y += WM_FW_EXPLOSION_DRIFT;
    return true;
}

/* ---- knockout_drones --------------------------------------------- */

size_t wm_fw_knockout_drones(wm_fw_process_t *procs, size_t live) {
    size_t i, n = 0;

    if (!procs) return 0;
    for (i = 0; i < live; ++i) {
        /* `move *a8(PLYR_TYPE),a14 / jrz #next`. */
        if (procs[i].plyr_type == 0) continue;
        /* `movi 7fffh,a14 / move a14,*a8(PTIME)` -- asleep, not dead. */
        procs[i].ptime = WM_FW_DRONE_SLEEP;
        n++;
    }
    return n;
}
