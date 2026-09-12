#include "wm/arcade/wm_arcade_lifebar.h"
#include "wm/arcade/wm_arcade_life_data.h"

void wm_arcade_adjust_health(wm_arcade_actor_t *victim, int16_t delta,
                             wm_arcade_actor_t *damage_source,
                             bool attract_mode, uint32_t pcnt,
                             int32_t *dam_mult,
                             const wm_arcade_death_anim_callback_t *death_anim) {
    int32_t life;
    int32_t d = delta;

    if (!victim) return;

    if (damage_source && damage_source->combo_count != 0) {
        /* LIFEBAR.ASM:1429-1447: "doing a combo" damage entirely replaces
           the original hit's delta. */
        int32_t magnitude = 10 - damage_source->combo_count;
        if (magnitude < 4) magnitude = 4;
        d = -magnitude;
        if (dam_mult) *dam_mult = 0;
    } else if (dam_mult && *dam_mult != 0) {
        /* LIFEBAR.ASM:1449-1465: damage *= (1+DAM_MULT)/2, e.g. DAM_MULT=2
           -> x1.5, DAM_MULT=4 -> x2.5. sra 1 floors toward -infinity for
           negative values, matching C's >> on a negative signed int here. */
        d = d * (1 + *dam_mult);
        d >>= 1;
        *dam_mult = 0;
    }

    if (d < 0) {
        /* LIFEBAR.ASM:1471-1521 damage_mod_table lookup ("unless we're
           adding life"). wm_match's actors[] never holds more than the
           fixed pair wm_match_start_attract/selected create, so the
           source's active-drone count (is_8_on_1/buddy_mode_on/
           royal_rumble, none of which exist in this port, or else a count
           of process slots beyond the first two) always resolves to the
           table's first row here -- and that row's drone/player columns
           are both _85PCT, so no PLYR_TYPE branch is needed either. */
        d = (d * WM_ARCADE_DAMAGE_MOD_85PCT) >> 8;
    }

    /* LIFEBAR.ASM:1524-1528 speed_adjustment scaling, applied unconditionally
       (unlike the damage_mod_table step above, this one also runs on
       positive/healing deltas): d = (d * speed_adjustment) >> 16, speed_
       adjustment a Q16.16 fixed value. WM_ARCADE_SPEED_ADJUSTMENT_16_16 is
       exactly 0x10000 (1.0x, a mathematical identity), so this line changes
       nothing today -- see its own comment for why that is the real,
       sourced factory-default value, not an invented placeholder. */
    d = (int32_t)(((int64_t)d * WM_ARCADE_SPEED_ADJUSTMENT_16_16) >> 16);

    life = (int32_t)victim->life + d;

    if (life <= 0) {
        if (life > -10 && d <= -20) {
            /* LIFEBAR.ASM:1561-1569 fudge. */
            life = 5;
        } else {
            /* LIFEBAR.ASM:1575-1581. */
            life = attract_mode ? WM_ARCADE_LIFE_MAX : 0;
            if (life == 0) {
                /* LIFEBAR.ASM:1659-1670: genuinely dying -- a live combo
                   on the attacker defers it instead. */
                if (damage_source && damage_source->combo_count != 0) {
                    life = 1;
                    victim->i_will_die = 1;
                } else {
                    /* LIFEBAR.ASM:1591-1725 death-dispatch tail -- see
                       wm_arcade_lifebar.h for the full derivation. Reads
                       victim's own player_mode/attack_mode below before
                       overwriting it to WM_PMODE_DEAD at the end. */
                    uint16_t old_mode = victim->player_mode;
                    bool play_death_anim = true;

                    victim->roll_pos = 0;

                    if (damage_source) {
                        uint16_t am = damage_source->attack_mode;
                        if (am == WM_AMODE_BLBOWDROP || am == WM_AMODE_BSTOMP ||
                            am == WM_AMODE_BUTTSTOMP) {
                            play_death_anim = false;
                        } else if (am == WM_AMODE_BUZZ && old_mode != WM_PMODE_BLOCK) {
                            play_death_anim = false;
                        }
                    }
                    if (victim->status_flags & WM_STATUS_DEAD_ANIM) {
                        play_death_anim = false;
                    }

                    if (play_death_anim) {
                        switch (old_mode) {
                        case WM_PMODE_NORMAL: case WM_PMODE_RUNNING:
                        case WM_PMODE_INAIR: case WM_PMODE_INAIR2:
                        case WM_PMODE_BOUNCING: case WM_PMODE_ONTURNBKL:
                        case WM_PMODE_BLOCK: case WM_PMODE_DIZZY:
                        case WM_PMODE_CLIMBTURNBKL: {
                            /* LIFEBAR.ASM #fallbk: knock the corpse away
                               from whoever killed it, unless it's already
                               moving away faster than that. */
                            const int32_t away = 2 << 16;
                            if (victim->x_vel <= away) {
                                victim->x_vel = (damage_source &&
                                    damage_source->x_int < victim->x_int)
                                    ? away : -away;
                            }
                            if (death_anim && death_anim->change_anim) {
                                death_anim->change_anim(victim,
                                    WM_R1_ANIM_FALL_BACK, death_anim->user);
                            }
                            break;
                        }
                        default:
                            /* LIFEBAR.ASM's own unmatched-mode catch-all
                               (also covers HEADHELD's real #will_die
                               deferral and ONGROUND/DEAD's real convulse_t
                               dispatch -- see wm_arcade_lifebar.h for why
                               neither is reachable in this port and isn't
                               guessed at here): zero all velocities, no
                               anim change. */
                            victim->x_vel = 0;
                            victim->y_vel = 0;
                            victim->z_vel = 0;
                            break;
                        }
                    }

                    /* LIFEBAR.ASM:1723-1725 SETMODE DEAD + wres_collis_off. */
                    victim->player_mode = WM_PMODE_DEAD;
                    wm_arcade_wrestler_collisions_off(victim);
                }
            }
        }
    } else if (life > WM_ARCADE_LIFE_MAX) {
        life = WM_ARCADE_LIFE_MAX;
    }

    victim->life = life;

    /* LIFEBAR.ASM:1593-1595, unconditional on every call. */
    victim->last_damage = (uint16_t)pcnt;
}

void wm_arcade_clear_lifebar(wm_arcade_actor_t *a) {
    if (!a) return;
    a->life = 0;
}

/*
 * LIFEBAR.ASM:3650 is_perfect. See wm/arcade/wm_arcade_lifebar.h --
 * carry means perfect, and the final battle is never perfect.
 */
bool wm_arcade_is_perfect(const wm_arcade_actor_t *winner,
                          const wm_arcade_actor_t *const *actors,
                          size_t actor_count, bool eight_on_one) {
    size_t i;

    if (!winner || !actors) return false;
    if (eight_on_one) return false;     /* `jrc #final` -> clrc */

    for (i = 0; i < actor_count; ++i) {
        const wm_arcade_actor_t *a = actors[i];
        if (!a) continue;                            /* `jrz #nxt` */
        if (a->player_side != winner->player_side) continue;
        if (a->life != WM_LIFE_MAX) return false;    /* an injured one */
    }
    return true;                                     /* `setc` */
}

/* ---- LIFEBAR.ASM:4039 SHIFT_BARS_IN_Z ---------------------------- */

void wm_zflip_begin(wm_zflip_t *z) {
    if (!z) return;
    /* `CLR A0 / MOVE A0,@LAST_FLIP` before the loop is entered. */
    z->flipped = false;
    z->timer = 0;
    z->shifted = false;
    z->shift_by = 0;
}

bool wm_zflip_tick(wm_zflip_t *z, int32_t worldtly, int32_t threshold) {
    if (!z) return false;
    z->shifted = false;
    z->shift_by = 0;

    /*
     * `SLOOP 4` is a SLEEPK 4 and a jump back, so the body runs
     * every FOURTH tick: the tick it runs on is the first of the
     * four, not an extra one in front of them.
     */
    if (z->timer > 0) {
        z->timer--;
        return false;
    }
    z->timer = WM_ZFLIP_POLL_TICKS - 1;

    if (worldtly < threshold) {
        /* CHECK_IF_WE_ALREADY_FLIPPED_BACK. */
        if (!z->flipped) return false;
        z->flipped = false;
        z->shifted = true;
        z->shift_by = -WM_ZFLIP_FORWARD;
        return true;
    }
    /* At or above: forward, if it is not forward already. */
    if (z->flipped) return false;
    z->flipped = true;
    z->shifted = true;
    z->shift_by = WM_ZFLIP_FORWARD;
    return true;
}

bool wm_zflip_in_band(int32_t z, int32_t shift_by) {
    /*
     * SHIFT_BARS_FORWARD tests [bak_z, name_z]; SHIFT_BARS_BACK
     * tests [bak_z + Z_FORWARD, name_z + Z_FORWARD] -- the band
     * where the forward shift left them. Both compares are
     * inclusive: `CMP a2,A1 / JRLT skip` and `CMP a3,A1 / JRGT skip`.
     */
    int32_t base = (shift_by < 0) ? WM_ZFLIP_FORWARD : 0;
    return z >= WM_ZFLIP_BAK_Z + base && z <= WM_ZFLIP_NAME_Z + base;
}

size_t wm_zflip_apply(int32_t *zpos, size_t count, int32_t shift_by) {
    size_t i, moved = 0;

    if (!zpos || shift_by == 0) return 0;
    for (i = 0; i < count; ++i) {
        if (!wm_zflip_in_band(zpos[i], shift_by)) continue;
        zpos[i] += shift_by;
        moved++;
    }
    return moved;
}

/* ---- LIFEBAR.ASM:880 rewire_monitor ------------------------------ */

wm_rewire_mode_t wm_rewire_mode(bool buddy_mode_on, bool royal_rumble,
                                int32_t pstatus, int32_t num_opps) {
    /* `move @buddy_mode_on,a14 / jrnz #buddy`, then royal_rumble,
       then the two `jreq #die` tests -- in that order. */
    if (buddy_mode_on) return WM_REWIRE_MODE_BUDDY;
    if (royal_rumble) return WM_REWIRE_MODE_RUMBLE;
    if (pstatus == 3) return WM_REWIRE_MODE_DIE;
    if (num_opps == 1) return WM_REWIRE_MODE_DIE;
    return WM_REWIRE_MODE_NORMAL;
}

static const wm_rewire_actor_t *actor_at(const wm_rewire_actor_t *actors,
                                         size_t count, int32_t num) {
    /* `calla get_process_ptr` -- process_ptrs indexed by PLYRNUM. */
    if (!actors || num < 0 || (size_t)num >= count) return NULL;
    return &actors[num];
}

static bool is_dead(const wm_rewire_actor_t *a) {
    /* A slot with no process cannot be MODE_DEAD; the source would
       read through a null pointer, which this refuses to model. */
    return a != NULL && a->dead;
}

bool wm_rewire_begin(wm_rewire_t *r, wm_rewire_mode_t mode,
                     const wm_rewire_actor_t *actors, size_t count) {
    size_t i;

    if (!r) return false;
    r->mode = mode;
    r->timer = 0;
    r->started = false;
    r->watcher = -1;
    r->showing = -1;
    r->last_rewire = 0;
    r->have_last_rewire = false;
    r->rumble_at = 3;               /* `movk 3,a9` */
    r->rumble_hold = 0;
    r->buddy_show0 = 0;             /* `clr a8` */
    r->buddy_show1 = 1;             /* `movk 1,a9` */
    r->rewire_count = 0;

    if (mode == WM_REWIRE_MODE_DIE) return false;

    if (mode == WM_REWIRE_MODE_NORMAL) {
        /*
         * `#lp0: move *a1+,a2,L / jrnz #found / dsj a0,#lp0` -- the
         * FIRST live process, whoever that is, and the loop falls
         * through to #die if there is none (LOCKUP under DEBUG).
         */
        for (i = 0; i < count; ++i) {
            if (!actors || !actors[i].active) continue;
            r->watcher = actors[i].plyrnum;
            r->showing = actors[i].closest_num;
            r->started = true;
            return true;
        }
        return false;
    }
    r->started = true;
    return true;
}

/* The three loops all poll on `SLOOP 10`; the tick the body runs on
   is the first of the ten. */
static bool poll_due(wm_rewire_t *r) {
    if (r->timer > 0) {
        r->timer--;
        return false;
    }
    r->timer = WM_REWIRE_POLL_TICKS - 1;
    return true;
}

static void do_rewire(wm_rewire_t *r, int32_t display, int32_t wrestler) {
    if (r->rewire_count >= 2) return;
    r->rewires[r->rewire_count].display = display;
    r->rewires[r->rewire_count].wrestler = wrestler;
    r->rewire_count++;
}

static void tick_normal(wm_rewire_t *r, const wm_rewire_actor_t *actors,
                        size_t count, int32_t pcnt) {
    const wm_rewire_actor_t *me = actor_at(actors, count, r->watcher);
    const wm_rewire_actor_t *shown;
    const wm_rewire_actor_t *target;
    int32_t want;

    if (!me) return;
    /* `move *a6(CLOSEST_NUM),a1 / cmp a11,a1 / jreq #no`. */
    want = me->closest_num;
    if (want == r->showing) return;

    /*
     * `move a11,a1 / get_process_ptr / cmpi MODE_DEAD / jreq
     * #rewire` -- the dead test is on who is being SHOWN, not on
     * who the human has moved on to, which is exactly what the
     * comment says: the latency is waived when the displayed
     * wrestler is dead.
     */
    shown = actor_at(actors, count, r->showing);
    if (!is_dead(shown)) {
        /* `move @PCNT,a0,L / move @#LAST_REWIRE,a1,L / addi
           #LATENCY,a1 / cmp a1,a0 / jrle #no` -- at or before the
           deadline is too soon. */
        if (r->have_last_rewire &&
            pcnt <= r->last_rewire + WM_REWIRE_LATENCY) {
            return;
        }
    }

    /* `#rewire`: re-read CLOSEST_NUM rather than reusing a1. */
    target = actor_at(actors, count, me->closest_num);
    if (!target) return;
    do_rewire(r, target->plyr_side, target->plyrnum);
    r->last_rewire = pcnt;
    r->have_last_rewire = true;
    r->showing = me->closest_num;
}

/*
 * `#rumble` / `#rloop` / `#toggle`. a9 starts at 3 and the routine
 * jumps STRAIGHT to #toggle, so the first thing it does is switch to
 * wrestler 2 -- if wrestler 2 is alive.
 */
static void rumble_toggle(wm_rewire_t *r, const wm_rewire_actor_t *actors,
                          size_t count) {
    /* `move a9,a1 / xori 1,a1` -- the other of the pair. */
    int32_t other = r->rumble_at ^ 1;
    const wm_rewire_actor_t *a = actor_at(actors, count, other);

    if (is_dead(a) || a == NULL) return;    /* `#notogl` */
    r->rumble_at = other;
    /* `movk 1,a0 / callr rewire_meter` -- always display 1. */
    do_rewire(r, 1, other);
    r->rumble_hold = WM_REWIRE_RUMBLE_HOLD;
}

static void tick_rumble(wm_rewire_t *r, const wm_rewire_actor_t *actors,
                        size_t count) {
    const wm_rewire_actor_t *a;

    if (!r->started) {
        /* `jruc #toggle` before the loop is ever entered. */
        r->started = true;
        rumble_toggle(r, actors, count);
        return;
    }
    /* `#rloop`: the one on display dies, or the hold runs out. */
    a = actor_at(actors, count, r->rumble_at);
    if (is_dead(a)) {
        rumble_toggle(r, actors, count);
        return;
    }
    /* `dec a8 / jrle #toggle`. */
    r->rumble_hold--;
    if (r->rumble_hold <= 0) rumble_toggle(r, actors, count);
}

/*
 * `#buddy` / `#bloop`. Two independent displays. Display 0 wants
 * wrestler 0 and falls back to 2; display 1 wants 1 and falls back
 * to 3. A display already showing its fallback is left alone, and a
 * fallback that is also dead means no change at all -- the bar keeps
 * showing the dead wrestler rather than going blank.
 */
static void buddy_side(wm_rewire_t *r, const wm_rewire_actor_t *actors,
                       size_t count, int32_t display,
                       int32_t primary, int32_t fallback,
                       int32_t *showing) {
    const wm_rewire_actor_t *p = actor_at(actors, count, primary);

    if (!is_dead(p)) {
        /* `#p1liv: TEST a8 / jrz #ckp2` -- already on the primary? */
        if (*showing == primary) return;
        *showing = primary;
        do_rewire(r, display, primary);
        return;
    }
    /* `#p1ded`: the primary is dead. */
    {
        const wm_rewire_actor_t *f = actor_at(actors, count, fallback);
        if (is_dead(f) || f == NULL) return;   /* both gone: leave it */
        if (*showing == fallback) return;
        *showing = fallback;
        do_rewire(r, display, fallback);
    }
}

bool wm_rewire_tick(wm_rewire_t *r, const wm_rewire_actor_t *actors,
                    size_t count, int32_t pcnt) {
    if (!r) return false;
    r->rewire_count = 0;
    if (r->mode == WM_REWIRE_MODE_DIE) return false;
    if (!poll_due(r)) return true;

    switch (r->mode) {
    case WM_REWIRE_MODE_NORMAL:
        tick_normal(r, actors, count, pcnt);
        break;
    case WM_REWIRE_MODE_RUMBLE:
        tick_rumble(r, actors, count);
        break;
    case WM_REWIRE_MODE_BUDDY:
        /* `#bloop` checks display 0 then display 1 in one pass,
           and both can rewire on the same tick. */
        buddy_side(r, actors, count, 0, 0, 2, &r->buddy_show0);
        buddy_side(r, actors, count, 1, 1, 3, &r->buddy_show1);
        break;
    default:
        return false;
    }
    return true;
}
