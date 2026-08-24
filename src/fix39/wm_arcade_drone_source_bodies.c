#include "wm_arcade_drone_source_bodies.h"
#include "wm_arcade_drone_source_scripts.h"
#include "wm_arcade_drone_source_services.h"
#include "wmania_ring_geometry.h"
#include "wmania_rng.h"

#include <stdlib.h>
#include <string.h>

typedef struct WmFix39DroneGeneratedBody {
    const char *label;
    wm_arcade_drone_source_service_handler_t handler;
} WmFix39DroneGeneratedBody;

#define WM_DRONE_TSEC 60

enum {
    WM_DRONE_CHARGE_NONE = 0,
    WM_DRONE_CHARGE_RUN = 1,
    WM_DRONE_CHARGE_YOKO_RUN = 2,
    WM_DRONE_CHARGE_BAM_PUNCH = 3,
    WM_DRONE_CHARGE_DOINK_LEAP = 4
};

static uint32_t wm_fix39_drone_body_rnd(void *user, uint32_t mask)
{
    return user ? wm_rng_rnd_mask((WmRng *)user, mask) : 0u;
}

static uint32_t wm_fix39_drone_body_rndrng0(void *user, uint32_t maxv)
{
    return user ? wm_rng_rndrng0((WmRng *)user, maxv) : 0u;
}

static uint16_t source_flip_lr(uint16_t dir)
{
    uint16_t out = (uint16_t)(dir & (WM_MOVE_UP | WM_MOVE_DOWN));
    if (dir & WM_MOVE_LEFT) out = (uint16_t)(out | WM_MOVE_RIGHT);
    if (dir & WM_MOVE_RIGHT) out = (uint16_t)(out | WM_MOVE_LEFT);
    return out;
}

static void source_input(wm_arcade_actor_t *self,
                         wm_arcade_drone_state_t *d,
                         uint16_t joy,
                         uint16_t buttons,
                         int32_t delay)
{
    if (!self || !d) return;
    joy = (uint16_t)(joy & 0x0fu);
    if ((self->facing_dir & WM_MOVE_RIGHT) == 0u)
        joy = source_flip_lr(joy);
    d->joy = joy;
    d->but = (uint16_t)(buttons & WM_BTN_ATTACK_MASK);
    d->delay = delay;
}

static void native_reset_service(wm_arcade_drone_state_t *d)
{
    if (!d) return;
    d->service_pc = 0;
    d->service_aux0 = 0;
    d->service_aux1 = 0;
}

static void native_end(wm_arcade_drone_state_t *d)
{
    if (!d) return;
    d->script = 0;
    d->script_pc = 0;
    native_reset_service(d);
}

static void native_redirect(wm_arcade_actor_t *self,
                            wm_arcade_drone_state_t *d,
                            const char *label)
{
    if (!d) return;
    d->script = label;
    d->script_pc = 0;
    d->script_mode = self ? self->player_mode : WM_PMODE_NORMAL;
    native_reset_service(d);
}

static int32_t xvel_word(const wm_arcade_actor_t *a)
{
    if (!a) return 0;
    return (int32_t)(int16_t)(((uint32_t)a->x_vel) >> 16);
}

/*
 * Exact DRONE.ASM #sine_t word data.  Each DRN_SEEKDIST row is 20 words so
 * direction+4 can read the quadrature X component without wrap code.
 */
static const int16_t source_sine_t[6][20] = {
    {-50,-46,-35,-19,0,19,35,46,50,46,35,19,0,-19,-35,-46,-50,-46,-35,-19},
    {-100,-92,-71,-38,0,38,71,92,100,92,71,38,0,-38,-71,-92,-100,-92,-71,-38},
    {-150,-139,-106,-57,0,57,106,139,150,139,106,57,0,-57,-106,-139,-150,-139,-106,-57},
    {-200,-185,-141,-76,0,76,141,185,200,185,141,76,0,-76,-141,-185,-200,-185,-141,-76},
    {-250,-231,-177,-95,0,95,177,231,250,231,177,95,0,-95,-177,-231,-250,-231,-177,-95},
    {-300,-277,-212,-115,0,114,212,277,300,277,212,114,0,-114,-212,-277,-300,-277,-212,-115}
};

/* Direct port of DRONE.ASM::drone_seekxz. Returns the source DRN_JOY value. */
static uint16_t native_seekxz(wm_arcade_actor_t *self,
                              wm_arcade_drone_state_t *d,
                              int32_t target_x,
                              int32_t target_z,
                              int32_t range)
{
    int32_t dx, dz;
    uint16_t joy = WM_MOVE_ZIP;
    if (!self || !d) return 0;

    dx = self->x_int - target_x;
    if (abs((int)dx) > range)
        joy = (uint16_t)(joy | (dx < 0 ? WM_MOVE_RIGHT : WM_MOVE_LEFT));

    dz = self->z_int - target_z;
    if (abs((int)dz) > range)
        joy = (uint16_t)(joy | (dz < 0 ? WM_MOVE_DOWN : WM_MOVE_UP));

    d->joy = joy;
    return joy;
}

static uint16_t native_seek2(wm_arcade_actor_t *self,
                             wm_arcade_actor_t *opp,
                             wm_arcade_drone_state_t *d,
                             int32_t range)
{
    if (!opp) {
        if (d) d->joy = 0;
        return 0;
    }
    return native_seekxz(self, d, opp->x_int, opp->z_int, range);
}

/* Direct port of #drn_getxz's valid-target test. */
static int native_seek_dir_target(const wm_arcade_actor_t *opp,
                                  int dist,
                                  int dir,
                                  int32_t *tx,
                                  int32_t *tz)
{
    int32_t x, z;
    if (!opp || !tx || !tz) return 0;
    if (dist < 0) dist = 0;
    if (dist > 5) dist = 5;
    dir &= 15;

    z = opp->z_int + source_sine_t[dist][dir];
    x = opp->x_int + source_sine_t[dist][dir + 4];

    if (x < WM_RING_X_CENTER - 220 || x > WM_RING_X_CENTER + 220) return 0;
    if (z < WM_RING_TOP || z > WM_RING_BOT) return 0;

    *tx = x;
    *tz = z;
    return 1;
}

/* Direct port of DRONE.ASM::drone_seekdirdist. */
static void native_seekdirdist(wm_arcade_actor_t *self,
                               wm_arcade_actor_t *opp,
                               wm_arcade_drone_state_t *d,
                               void *user)
{
    int dist, dir, plus, minus, i;
    int32_t tx = 0, tz = 0;
    if (!self || !opp || !d) return;

    dist = d->seek_dist;
    if (dist < 0) dist = 0;
    if (dist > 5) dist = 5;
    dir = d->seek_dir & 15;

    if (!native_seek_dir_target(opp, dist, dir, &tx, &tz)) {
        plus = dir;
        minus = dir;
        for (i = 0; i < 7; ++i) {
            plus = (plus + 1) & 15;
            if (native_seek_dir_target(opp, dist, plus, &tx, &tz)) {
                dir = plus;
                d->seek_dir = dir;
                break;
            }
            minus = (minus - 1) & 15;
            if (native_seek_dir_target(opp, dist, minus, &tx, &tz)) {
                dir = minus;
                d->seek_dir = dir;
                break;
            }
        }
        if (i == 7) {
            d->joy = 0;
            return;
        }
    }

    if (native_seekxz(self, d, tx, tz, 30) != 0)
        return;

    /* Source changes direction only for DRN_MODE <= -2. */
    if (d->mode <= -2) {
        int delta;
        uint32_t r = wm_fix39_drone_body_rnd(user, 3u);
        d->joy = d->source_old_joy;
        switch (r & 3u) {
            case 0u: delta = -2; break;
            case 1u: delta = -3; break;
            case 2u: delta = 2; break;
            default: delta = 3; break;
        }
        d->seek_dir = (d->seek_dir + delta) & 15;
    }
}

/* Direct port of DRONE.ASM::drone_chkrun; nonzero means source #bad. */
static int native_chkrun_bad(const wm_arcade_actor_t *self,
                             const wm_arcade_actor_t *opp)
{
    int32_t x, z;
    if (!self || !opp) return 0;
    x = self->x_int;
    z = self->z_int;

    if (self->in_ring == 0) {
        if (opp->player_mode == WM_PMODE_ONGROUND) return 0;
        if (self->closest_zdist >= 70) return 0;
        if (self->closest_zdist <= 30) return 0;
        if (self->closest_xdist >= 150) return 0;
        return 1;
    }

    if (self->facing_dir & WM_MOVE_RIGHT) {
        if (x >= WM_RING_X_CENTER + 500) return 1;
        if (z < WM_RING_TOP - 10 || z > WM_RING_BOT + 10) return 0;
        if (x >= WM_RING_X_CENTER) return 0;
        if (x >= WM_RING_X_CENTER - 300) return 1;
        return 0;
    }

    if (x <= WM_RING_X_CENTER - 500) return 1;
    if (z < WM_RING_TOP - 10 || z > WM_RING_BOT + 10) return 0;
    if (x <= WM_RING_X_CENTER) return 0;
    if (x <= WM_RING_X_CENTER + 300) return 1;
    return 0;
}

/* ------------------------------------------------------------------------- */
/* Fifteen explicit DRONE executable-service translations.                  */
/* ------------------------------------------------------------------------- */

static int wm_fix39_native_drn_seek(wm_arcade_actor_t *self,
                                    wm_arcade_actor_t *opp,
                                    wm_arcade_drone_state_t *d,
                                    void *user)
{
    if (!self || !opp || !d) return 0;
    if (self->player_mode != WM_PMODE_NORMAL &&
        self->player_mode != WM_PMODE_BLOCK) {
        native_end(d);
        return 1;
    }
    if (wm_fix39_drone_body_rnd(user, 0x3fu) == 0u) {
        native_end(d);
        return 1;
    }
    if (native_seek2(self, opp, d, 70) != 0)
        return 0; /* #lp -> DS_SLP1 */
    native_end(d);
    return 1;
}

static int wm_fix39_native_drn_seekclose(wm_arcade_actor_t *self,
                                         wm_arcade_actor_t *opp,
                                         wm_arcade_drone_state_t *d,
                                         void *user)
{
    int32_t tx;
    if (!self || !opp || !d) return 0;
    if (self->player_mode != WM_PMODE_NORMAL &&
        self->player_mode != WM_PMODE_BLOCK) {
        native_end(d);
        return 1;
    }
    if (wm_fix39_drone_body_rnd(user, 0x3fu) == 0u) {
        native_end(d);
        return 1;
    }

    tx = opp->x_int + (self->x_int >= opp->x_int ? 32 : -32);
    if (native_seekxz(self, d, tx, opp->z_int, 23) != 0)
        return 0;
    native_end(d);
    return 1;
}

static int wm_fix39_native_drn_retreat(wm_arcade_actor_t *self,
                                       wm_arcade_actor_t *opp,
                                       wm_arcade_drone_state_t *d,
                                       void *user)
{
    if (!self || !opp || !d) return 0;
    if (d->service_pc == 0) {
        d->seek_dist = 4;
        d->service_pc = 1;
        native_seekdirdist(self, opp, d, user);
        return 0;
    }

    if (wm_fix39_drone_body_rnd(user, 0x1fu) != 0u) {
        native_seekdirdist(self, opp, d, user);
        return 0;
    }

    native_end(d);
    return 1;
}

static int wm_fix39_native_drone_chrg(wm_arcade_actor_t *self,
                                      wm_arcade_actor_t *opp,
                                      wm_arcade_drone_state_t *d,
                                      void *user)
{
    uint32_t second;
    int w;
    (void)opp;
    if (!self || !d) return 0;
    if (d->but_charge != 0u) return 1;

    second = wm_fix39_drone_body_rnd(user, 0x20u);
    w = self->wrestler_num;

    d->charge_kind = WM_DRONE_CHARGE_RUN;
    d->charge_pc = 0;
    d->charge_aux = 0;
    d->charge_script = 0;

    switch (w) {
        case 0: /* Bret: both entries are #brt = SK */
            d->but_charge = WM_BTN_SKICK;
            break;
        case 1: /* Razor: SP / SK */
            d->but_charge = second ? WM_BTN_SKICK : WM_BTN_SPUNCH;
            break;
        case 2: /* Undertaker: P / SK */
            d->but_charge = second ? WM_BTN_SKICK : WM_BTN_PUNCH;
            break;
        case 3: /* Yoko: P / P, continuation sets DRN_MODE=1 */
            d->but_charge = WM_BTN_PUNCH;
            d->charge_kind = WM_DRONE_CHARGE_YOKO_RUN;
            break;
        case 4: /* Shawn: P / SK */
            d->but_charge = second ? WM_BTN_SKICK : WM_BTN_PUNCH;
            break;
        case 5: /* Bam Bam: fire-punch continuation / SP run */
            if (second) {
                d->but_charge = WM_BTN_SPUNCH;
            } else {
                d->but_charge = WM_BTN_PUNCH;
                d->charge_kind = WM_DRONE_CHARGE_BAM_PUNCH;
            }
            break;
        case 6: /* Doink: buzzer / leap-buzzer continuation */
            d->but_charge = WM_BTN_PUNCH;
            if (second) d->charge_kind = WM_DRONE_CHARGE_DOINK_LEAP;
            break;
        case 7: /* Spare source table contains 0,0. */
            d->charge_kind = WM_DRONE_CHARGE_NONE;
            d->but_charge = 0;
            d->but_charge_delay = 0;
            return 1;
        case 8: /* Lex: P / SK */
            d->but_charge = second ? WM_BTN_SKICK : WM_BTN_PUNCH;
            break;
        default:
            d->charge_kind = WM_DRONE_CHARGE_NONE;
            d->but_charge = 0;
            d->but_charge_delay = 0;
            return 1;
    }

    d->but_charge_delay = WM_DRONE_TSEC * 2;
    return 1;
}

static int wm_fix39_native_drn_climbtb(wm_arcade_actor_t *self,
                                       wm_arcade_actor_t *opp,
                                       wm_arcade_drone_state_t *d,
                                       void *user)
{
    (void)opp;
    if (!self || !d) return 0;

    if (d->source_alive_team > 1 &&
        wm_fix39_drone_body_rnd(user, 1u) != 0u) {
        native_end(d);
        return 1;
    }

    /* #lp is one DS_SLP1, then source falls directly into drn_ontb. */
    native_redirect(self, d, "drn_ontb");
    return 1;
}

static int wm_fix39_native_drn_ontb(wm_arcade_actor_t *self,
                                    wm_arcade_actor_t *opp,
                                    wm_arcade_drone_state_t *d,
                                    void *user)
{
    int32_t tx;
    (void)user;
    if (!self || !opp || !d) return 0;

    if (self->player_mode == WM_PMODE_ONTURNBKL) {
        if (opp->player_mode == WM_PMODE_ONTURNBKL) {
            source_input(self, d, WM_MOVE_DOWN, 0, 0);
        } else {
            d->force_old_but_zero = 1u; /* source CLR A6 at #jmp */
            source_input(self, d, 0, WM_BTN_KICK, 0);
        }
        native_end(d);
        return 1;
    }

    if (self->in_ring != 0) {
        native_redirect(self, d, "drn_enterring");
        return 1;
    }

    if (self->wrestler_num == 3 && opp->in_ring != 0) {
        native_end(d);
        return 1;
    }
    if (opp->player_mode == WM_PMODE_ONTURNBKL ||
        opp->player_mode == WM_PMODE_INAIR2) {
        native_end(d);
        return 1;
    }

    tx = self->x_int <= WM_RING_X_CENTER
        ? WM_RING_X_CENTER - 225
        : WM_RING_X_CENTER + 225;

    if (native_seekxz(self, d, tx, WM_RING_TOP, 32) == 0)
        (void)native_seekxz(self, d, tx, WM_RING_TOP - 10, 0);

    if (self->closest_xdist > 120 || self->closest_zdist > 70)
        return 0;

    native_end(d);
    return 1;
}

static int wm_fix39_native_drn_inair(wm_arcade_actor_t *self,
                                     wm_arcade_actor_t *opp,
                                     wm_arcade_drone_state_t *d,
                                     void *user)
{
    (void)user;
    if (!self || !opp || !d) return 0;
    (void)native_seek2(self, opp, d, 0);
    if (self->player_mode == WM_PMODE_INAIR2)
        return 0;
    native_end(d);
    return 1;
}

static int wm_fix39_native_drn_opinair(wm_arcade_actor_t *self,
                                       wm_arcade_actor_t *opp,
                                       wm_arcade_drone_state_t *d,
                                       void *user)
{
    int32_t big;
    if (!self || !opp || !d) return 0;

    if (d->service_pc == 0) {
        if (wm_fix39_drone_body_rnd(user, 1u) != 0u) {
            source_input(self, d, WM_MOVE_LEFT,
                         (uint16_t)(WM_BTN_PUNCH | WM_BTN_KICK), 2);
            native_end(d);
            return 1;
        }
        d->service_pc = 1;
    }

    if (opp->player_mode != WM_PMODE_INAIR2) {
        source_input(self, d, 0, WM_BTN_KICK, 0);
        native_end(d);
        return 1;
    }

    big = self->closest_xdist > self->closest_zdist
        ? self->closest_xdist : self->closest_zdist;
    if (big > 150)
        return 0;

    source_input(self, d, 0, WM_BTN_KICK, 0);
    native_end(d);
    return 1;
}

static int wm_fix39_native_drn_enterring(wm_arcade_actor_t *self,
                                         wm_arcade_actor_t *opp,
                                         wm_arcade_drone_state_t *d,
                                         void *user)
{
    int32_t tx, tz;
    (void)user;
    if (!self || !opp || !d) return 0;

    if (opp->in_ring != 0 || self->in_ring == 0) {
        native_end(d);
        return 1;
    }

    if (self->x_int <= WM_RING_X_CENTER - 260) {
        tx = WM_RING_X_CENTER - 260;
        tz = WM_RING_Z_CENTER;
    } else if (self->x_int >= WM_RING_X_CENTER + 260) {
        tx = WM_RING_X_CENTER + 260;
        tz = WM_RING_Z_CENTER;
    } else {
        tx = WM_RING_X_CENTER;
        tz = self->z_int <= WM_RING_TOP - 10
            ? WM_RING_TOP - 10
            : WM_RING_BOT + 10;
    }

    if (native_seekxz(self, d, tx, tz, 10) != 0)
        return 0;

    source_input(self, d, 0, WM_BTN_ATTACK_MASK, 0);
    native_end(d);
    return 1;
}

static int wm_fix39_native_drn_taunt(wm_arcade_actor_t *self,
                                     wm_arcade_actor_t *opp,
                                     wm_arcade_drone_state_t *d,
                                     void *user)
{
    static const char *const taunts[9] = {
        "hrt_4_taunt_anim", "rzr_4_taunt_anim", "und_4_taunt_anim",
        "yok_4_taunt_anim", "shn_4_taunt_anim", "bam_4_taunt_anim",
        "dnk_4_taunt_anim", 0, "lex_4_taunt_anim"
    };
    int32_t dz, dx;
    (void)user;
    if (!self || !opp || !d) return 0;

    dz = opp->z_int - self->z_int;
    if (dz < 100) {
        native_end(d);
        return 1;
    }
    dx = abs((int)(opp->x_int - self->x_int));
    if (dx > 300) {
        native_end(d);
        return 1;
    }

    self->risk = (uint16_t)(0x8000u + 6u * 60u);
    if (self->wrestler_num >= 0 && self->wrestler_num < 9)
        d->anim_request = taunts[self->wrestler_num];
    native_end(d);
    return 1;
}

static int wm_fix39_native_drn_oppdead(wm_arcade_actor_t *self,
                                       wm_arcade_actor_t *opp,
                                       wm_arcade_drone_state_t *d,
                                       void *user)
{
    int32_t threshold;
    if (!self || !opp || !d) return 0;

    if (d->service_pc == 0) {
        d->seek_dir = 0;
        d->seek_dist = 0;
        native_seekdirdist(self, opp, d, user);
        d->service_pc = 1;
        return 0;
    }

    threshold = self->wrestler_num == 2 ? 32 : 90;
    if (self->closest_dist > threshold) {
        opp->anim_mode = (uint16_t)(opp->anim_mode | WM_ARCADE_MODE_OVERLAP);
        if (d->joy != 0u) {
            d->service_pc = 0;
            return 0;
        }
    }

    source_input(self, d, 0, WM_BTN_PUNCH, 0);
    native_end(d);
    return 1;
}

static uint16_t combo_open_button(int wrestler, uint32_t alt)
{
    switch (wrestler) {
        case 0: return alt ? WM_BTN_SKICK : WM_BTN_PUNCH;
        case 1: return alt ? WM_BTN_KICK : WM_BTN_SPUNCH;
        case 2: return alt ? WM_BTN_KICK : WM_BTN_SKICK;
        case 3: return alt ? WM_BTN_PUNCH : WM_BTN_SPUNCH;
        case 4: return alt ? WM_BTN_KICK : WM_BTN_PUNCH;
        case 5: return alt ? WM_BTN_PUNCH : WM_BTN_SPUNCH;
        case 6: return alt ? WM_BTN_SKICK : WM_BTN_SPUNCH;
        case 8: return alt ? WM_BTN_KICK : WM_BTN_SKICK;
        default: return WM_BTN_PUNCH;
    }
}

static uint16_t combo_cycle_button(void *user)
{
    if (wm_fix39_drone_body_rndrng0(user, 99u) < 25u) return WM_BTN_SKICK;
    if (wm_fix39_drone_body_rndrng0(user, 99u) < 25u) return WM_BTN_PUNCH;
    if (wm_fix39_drone_body_rndrng0(user, 99u) < 25u) return WM_BTN_KICK;
    return WM_BTN_SPUNCH;
}

static int wm_fix39_native_drn_combo(wm_arcade_actor_t *self,
                                     wm_arcade_actor_t *opp,
                                     wm_arcade_drone_state_t *d,
                                     void *user)
{
    int32_t pct;
    int op;
    if (!self || !opp || !d) return 0;

    /* Initial wrestler-specific path. */
    if (d->service_pc == 0) {
        switch (d->service_aux0) {
            case 0: source_input(self, d, WM_MOVE_RIGHT, 0, 2); d->service_aux0++; return 0;
            case 1: source_input(self, d, 0, 0, 2); d->service_aux0++; return 0;
            case 2: source_input(self, d, WM_MOVE_RIGHT, 0, 2); d->service_aux0++; return 0;
            case 3: source_input(self, d, 0, 0, 2); d->service_aux0++; return 0;
            default:
                source_input(self, d, 0,
                    combo_open_button(self->wrestler_num,
                        wm_fix39_drone_body_rndrng0(user, 99u) < 50u),
                    2);
                d->service_pc = 1;
                d->service_aux0 = 0;
                return 0;
        }
    }

    /* #cstrt: zero for two ticks, choose a repeated attack, four pulses. */
    if (d->service_pc == 1) {
        if (d->service_aux0 == 0) {
            source_input(self, d, 0, 0, 2);
            d->service_aux0 = 1;
            return 0;
        }
        if (d->service_aux0 == 1) {
            d->service_aux1 = (int32_t)combo_cycle_button(user);
            d->service_aux0 = 2;
        }

        op = d->service_aux0 - 2;
        if (op < 8) {
            source_input(self, d, 0,
                (op & 1) ? 0u : (uint16_t)d->service_aux1,
                6);
            d->service_aux0++;
            return 0;
        }

        /* DS_RNDA sklrep_t */
        if (opp->player_mode == WM_PMODE_BLOCK) {
            native_end(d);
            return 1;
        }
        pct = wm_arcade_drone_source_script_skill_pct("sklrep_t",
                                                       d->skill, user);
        if ((int32_t)wm_fix39_drone_body_rndrng0(user, 99u) < pct) {
            d->service_aux0 = 0;
            return wm_fix39_native_drn_combo(self, opp, d, user);
        }

        d->delay = 10;
        native_end(d);
        return 1;
    }

    native_end(d);
    return 1;
}

static int wm_fix39_native_drn_oprun(wm_arcade_actor_t *self,
                                     wm_arcade_actor_t *opp,
                                     wm_arcade_drone_state_t *d,
                                     void *user)
{
    int32_t dx, signv;
    if (!self || !opp || !d) return 0;

    if (wm_fix39_drone_body_rnd(user, 7u) != 0u) {
        native_end(d);
        return 1;
    }

    dx = opp->x_int - self->x_int;
    signv = dx ^ xvel_word(opp);

    if (self->closest_zdist > abs((int)dx)) {
        native_end(d);
        return 1;
    }

    if (signv < 0 || opp->getup_time > 0) {
        source_input(self, d, 0,
                     (uint16_t)(WM_BTN_PUNCH | WM_BTN_KICK), 0);
        native_end(d);
        return 1;
    }

    native_end(d);
    return 1;
}

static int wm_fix39_native_drn_roll(wm_arcade_actor_t *self,
                                    wm_arcade_actor_t *opp,
                                    wm_arcade_drone_state_t *d,
                                    void *user)
{
    if (!self || !opp || !d) return 0;

    if (d->service_pc == 0) {
        (void)wm_fix39_native_drone_chrg(self, opp, d, user);
        d->service_pc = 1;
    }

    if (self->player_mode == WM_PMODE_ONGROUND &&
        self->closest_xdist <= 150 &&
        self->closest_zdist <= 70) {
        uint16_t joy = native_seek2(self, opp, d, 0);
        if (joy != 0u) {
            d->joy = (uint16_t)(joy ^ (WM_MOVE_UP | WM_MOVE_DOWN));
            if (wm_fix39_drone_body_rnd(user, 0x7fu) != 0u)
                return 0;
        }
    }

    source_input(self, d, 0, WM_BTN_BLOCK, WM_DRONE_TSEC - 10);
    native_end(d);
    return 1;
}

static int wm_fix39_native_drn_run(wm_arcade_actor_t *self,
                                   wm_arcade_actor_t *opp,
                                   wm_arcade_drone_state_t *d,
                                   void *user)
{
    int32_t selfv, oppv, opp_pred, self_pred, delta, signv;
    int32_t xdist, zdist;
    uint16_t attack;

    if (!self || !opp || !d) return 0;

    if (d->service_pc == 100) {
        native_redirect(self, d, "drn_enterring");
        return 1;
    }
    if (d->service_pc == 101) {
        native_redirect(self, d, "drn_seek");
        return 1;
    }

    if (self->player_mode != WM_PMODE_RUNNING &&
        self->player_mode != WM_PMODE_BOUNCING) {
        native_end(d);
        return 1;
    }

    if (wm_fix39_drone_body_rnd(user, 0x1ffu) == 0u) {
        source_input(self, d, WM_MOVE_LEFT, 0, 0);
        native_end(d);
        return 1;
    }

    selfv = xvel_word(self) << 3;
    oppv = xvel_word(opp) << 5;
    opp_pred = opp->x_int + oppv;
    self_pred = self->x_int + selfv;
    delta = opp_pred - self_pred;
    signv = selfv ^ delta;
    xdist = abs((int)delta);
    zdist = self->closest_zdist;

    if (self->in_ring != 0) {
        if (xdist > 300) {
            source_input(self, d, WM_MOVE_LEFT, 0, 2);
            d->service_pc = 100;
            return 0;
        }
        if (signv < 0) {
            source_input(self, d, WM_MOVE_LEFT, 0, 0);
            native_end(d);
            return 1;
        }
        if (zdist > 30) {
            source_input(self, d, WM_MOVE_LEFT, 0, 2);
            d->service_pc = 101;
            return 0;
        }
    } else {
        int check_opp = 0;
        if (selfv >= 0) {
            if (self_pred >= WM_RING_X_CENTER + 210) check_opp = 1;
        } else {
            if (self_pred <= WM_RING_X_CENTER - 210) check_opp = 1;
        }
        if (check_opp &&
            opp->getup_time <= 0 &&
            opp->player_mode != WM_PMODE_ONGROUND &&
            xdist <= 300 &&
            (opp->player_mode == WM_PMODE_RUNNING || xdist <= 180) &&
            zdist < 90) {
            source_input(self, d, WM_MOVE_LEFT, 0, 0);
            native_end(d);
            return 1;
        }
    }

    /* Source order matters here: running away seeks first, but an airborne
       opponent breaks the run before the Z/X seek tests. */
    if (signv < 0) {
        uint16_t joy = native_seek2(self, opp, d, 10);
        d->joy = (uint16_t)(joy & ~(WM_MOVE_LEFT | WM_MOVE_RIGHT));
        return 0;
    }

    if (opp->player_mode == WM_PMODE_INAIR2 ||
        opp->player_mode == WM_PMODE_PUPPET2 ||
        opp->player_mode == WM_PMODE_PUPPET ||
        opp->player_mode == WM_PMODE_HEADHELD ||
        opp->player_mode == WM_PMODE_HEADHOLD ||
        opp->player_mode == WM_PMODE_ATTACHED) {
        source_input(self, d, WM_MOVE_LEFT, 0, 0);
        native_end(d);
        return 1;
    }

    if (zdist > 30 ||
        xdist > 250 ||
        xdist > (int32_t)wm_fix39_drone_body_rndrng0(user, 120u) + 130) {
        uint16_t joy = native_seek2(self, opp, d, 10);
        d->joy = (uint16_t)(joy & ~(WM_MOVE_LEFT | WM_MOVE_RIGHT));
        return 0;
    }

    if (d->but_charge != 0u && d->but_charge_delay <= 0) {
        d->but_charge = 0u;
        d->charge_kind = WM_DRONE_CHARGE_NONE;
        d->charge_pc = 0;
        native_end(d);
        return 1;
    }

    if (wm_fix39_drone_body_rndrng0(user, 99u) < 33u)
        attack = WM_BTN_KICK;
    else if (wm_fix39_drone_body_rndrng0(user, 99u) < 33u)
        attack = WM_BTN_SKICK;
    else
        attack = WM_BTN_SPUNCH;

    source_input(self, d, 0, attack, 0);
    native_end(d);
    return 1;
}

/* ------------------------------------------------------------------------- */
/* Exact DRN_BUTCHRG_p continuations created by drone_chrg.                 */
/* ------------------------------------------------------------------------- */

static wm_arcade_drone_step_result_t charge_bam_punch(
    wm_arcade_actor_t *self,
    wm_arcade_actor_t *opp,
    wm_arcade_drone_state_t *d,
    const wm_arcade_drone_callbacks_t *cb)
{
    for (;;) {
        int limit;
        if (d->charge_pc == 0) {
            /* #bam continuation starts with DS_CODE clearing DRN_BUTCHRG,
               then a zero-input four-tick delay. */
            d->but_charge = 0u;
            d->charge_pc = 1;
            d->charge_aux = -1;
        }

        if (d->charge_pc >= 1 && d->charge_pc <= 3) {
            if (d->charge_aux < 0) {
                source_input(self, d, 0, 0, 4);
                d->charge_aux = 0;
                return WM_DRONE_STEP_INPUT;
            }

            /* After the second SKLREP gate the source has two contiguous
               three-punch rows plus the final P,0: 13 input/delay pairs. */
            limit = d->charge_pc < 3 ? 6 : 13;
            if (d->charge_aux < limit) {
                int final = (d->charge_pc == 3 && d->charge_aux == 12);
                source_input(self, d, 0,
                    (d->charge_aux & 1) ? 0u : WM_BTN_PUNCH,
                    final ? 0 : 4);
                d->charge_aux++;
                return WM_DRONE_STEP_INPUT;
            }

            if (d->charge_pc < 3) {
                int32_t pct;
                if (opp->player_mode == WM_PMODE_BLOCK) {
                    d->charge_kind = WM_DRONE_CHARGE_NONE;
                    d->charge_pc = 0;
                    d->delay = 10;
                    return WM_DRONE_STEP_ABORT_SCRIPT;
                }
                pct = cb && cb->script_skill_pct
                    ? cb->script_skill_pct("sklrep_t", d->skill, cb->user)
                    : 0;
                if ((int32_t)wm_fix39_drone_body_rndrng0(
                        cb ? cb->user : 0, 99u) >= pct) {
                    d->delay = 10;
                    d->charge_kind = WM_DRONE_CHARGE_NONE;
                    d->charge_pc = 0;
                    return WM_DRONE_STEP_ABORT_SCRIPT;
                }
                d->charge_pc++;
                d->charge_aux = 0;
                continue;
            }

            d->charge_kind = WM_DRONE_CHARGE_NONE;
            d->charge_pc = 0;
            return WM_DRONE_STEP_INPUT;
        }

        d->charge_kind = WM_DRONE_CHARGE_NONE;
        d->charge_pc = 0;
        return WM_DRONE_STEP_IDLE;
    }
}

static wm_arcade_drone_step_result_t charge_doink_leap(
    wm_arcade_actor_t *self,
    wm_arcade_drone_state_t *d)
{
    if (d->charge_pc == 0) {
        source_input(self, d, WM_MOVE_RIGHT, 0, 2);
        d->charge_pc = 1;
        return WM_DRONE_STEP_INPUT;
    }
    if (d->charge_pc == 1) {
        d->but_charge = 0u;
        source_input(self, d, WM_MOVE_RIGHT, 0, 2);
        d->charge_pc = 2;
        return WM_DRONE_STEP_INPUT;
    }

    /* DS_END clears A9 but does not write DRN_JOY. Preserve the last R_M
       value instead of inventing a zero-input frame. */
    d->charge_kind = WM_DRONE_CHARGE_NONE;
    d->charge_pc = 0;
    return WM_DRONE_STEP_SCRIPT;
}

wm_arcade_drone_step_result_t wm_arcade_drone_native_charge_step(
    wm_arcade_actor_t *self,
    wm_arcade_actor_t *opp,
    wm_arcade_drone_state_t *d,
    const wm_arcade_drone_callbacks_t *cb)
{
    int32_t dx, dz;
    if (!self || !opp || !d || d->charge_kind == WM_DRONE_CHARGE_NONE)
        return WM_DRONE_STEP_IDLE;

    if (d->charge_kind == WM_DRONE_CHARGE_BAM_PUNCH)
        return charge_bam_punch(self, opp, d, cb);

    if (d->charge_kind == WM_DRONE_CHARGE_DOINK_LEAP)
        return charge_doink_leap(self, d);

    if (d->charge_kind == WM_DRONE_CHARGE_YOKO_RUN)
        d->mode = 1; /* exact source inline continuation */

    if (!native_chkrun_bad(self, opp)) {
        source_input(self, d, 0,
                     (uint16_t)(WM_BTN_PUNCH | WM_BTN_KICK), 0);
        return WM_DRONE_STEP_INPUT;
    }

    /* Source skips the run buttons and only releases the charged button when
       the opponent is within this exact X/Z window. */
    dx = abs((int)(self->x_int - opp->x_int));
    dz = abs((int)(self->z_int - opp->z_int));
    if (dx < 150 && dz < 40) {
        d->but_charge = 0u;
        d->charge_kind = WM_DRONE_CHARGE_NONE;
        d->charge_pc = 0;
    }
    return WM_DRONE_STEP_SCRIPT;
}

/*
 * Generated at build time from the exact 15 labels recovered from DRONE.ASM.
 * Combat2EG's translator emits bindings to the native functions above only
 * after all source body windows are present.
 */
#include "wm_arcade_drone_source_bodies_generated.h"

int wm_arcade_drone_source_generated_body_count(void)
{
    return WM_FIX39_DRONE_TRANSLATED_BODY_COUNT;
}

int wm_arcade_drone_source_install_generated_bodies(void)
{
    int i, n = 0;
    for (i = 0; i < WM_FIX39_DRONE_TRANSLATED_BODY_COUNT; ++i)
        n += wm_arcade_drone_source_service_attach(
            wm_fix39_generated_bodies[i].label,
            wm_fix39_generated_bodies[i].handler) ? 1 : 0;
    return n;
}
